# DGUS II Protocol Guide

**Comprehensive reference for DWIN DGUS II communication protocol**

---

## 1. Protocol Overview

DGUS II is a serial communication protocol used by DWIN HMI displays to exchange data with microcontrollers. It uses a robust frame-based format with optional CRC validation and supports both reactive (request/response) and reactive (automatic upload) modes.

### 1.1 Key Characteristics

- **Frame-based**: All communication uses frames with sync bytes and length fields
- **Big-endian**: Multi-byte values transmitted MSB first
- **16-bit addressing**: 65536 unique 16-bit Virtual Processor (VP) addresses
- **Optional CRC**: CRC-16/MODBUS checksum for noise immunity
- **Multi-UART**: Display supports multiple serial ports (UART2-5 on T5L)
- **Asynchronous**: No clock signal; uses baud-rate synchronization
- **Full-duplex**: Host and display can communicate simultaneously

### 1.2 Communication Topology

```
┌────────────────────┐              ┌─────────────────────┐
│   Microcontroller  │ UART2/3/4/5  │  DWIN T5L Display   │
│    (8051-based)    │◄────────────►│  (DGUS II HMI)      │
│                    │   115200bps  │                     │
└────────────────────┘              └─────────────────────┘
    • Initiates reads                   • Stores VP data
    • Initiates writes                  • Triggers auto-upload
    • Processes responses               • Manages UI state
```

---

## 2. Frame Structure

### 2.1 General Frame Format

```
┌──────┬────────┬─────────┬──────────────────────────┬─────┐
│Header│ Length │ Command │       Data Payload       │ CRC │
│2 Bytes│1 Byte │ 1 Byte  │    Variable Length       │ 2B* │
└──────┴────────┴─────────┴──────────────────────────┴─────┘
       ▲         ▲         ▲                          ▲
       │         │         │                          └─ Optional
       │         │         │                             (if CRC enabled)
       │         │         └─ Command type (0x82-0x84)
       │         └─ Payload bytes (data field length)
       └─ Sync: 0x5A 0xA5
```

### 2.2 Length Field Calculation

The **Length** byte represents the number of bytes from command through the last data byte (inclusive):

```
Length = 1 (command) + Data Length

Examples:
  • Read VP: Length = 1 + 2 (addr bytes) = 0x03
  • Write VP: Length = 1 + 4 (addr + val) = 0x05
  • Write text: Length varies with string length
```

**Important**: Length does NOT include header (0x5A 0xA5) or CRC bytes

### 2.3 CRC Field (Optional)

Only present when `USE_CRC = 1` in config.h:

```
CRC-16/MODBUS Calculation:
  Polynomial: 0xA001 (reflected)
  Initial: 0xFFFF
  Calculation: Over all bytes from Command through last Data byte
  Transmission: Low byte first, then high byte (little-endian)

Implementation:
  u16 crc = 0xFFFF;
  for each byte in [command, data...]:
      byte = crc ^ byte;
      for i in 0..7:
          crc = (crc >> 1) ^ (byte & 1 ? 0xA001 : 0);
          byte >>= 1;
  transmission order: crc_low, crc_high
```

---

## 3. Command Set Reference

### 3.1 Command 0x82: Read VP (Host → Display)

**Purpose**: Read current value of one VP address from display

#### Request Format
```
0x5A 0xA5 0x04 0x82 <ADDR_H> <ADDR_L> [CRC_L CRC_H]
```

| Field | Bytes | Value | Purpose |
|-------|-------|-------|---------|
| Header | 2 | 0x5A 0xA5 | Sync bytes |
| Length | 1 | 0x04 | Fixed (1 cmd + 3 data) |
| Command | 1 | 0x82 | Read VP command |
| Addr High | 1 | 0x00-0xFF | VP address MSB |
| Addr Low | 1 | 0x00-0xFF | VP address LSB |
| CRC Low* | 1 | 0x00-0xFF | CRC-16 LSB (if enabled) |
| CRC High* | 1 | 0x00-0xFF | CRC-16 MSB (if enabled) |

#### Response Format
```
0x5A 0xA5 0x06 0x83 <ADDR_H> <ADDR_L> <VAL_H> <VAL_L> [CRC_L CRC_H]
```

| Field | Bytes | Value | Purpose |
|-------|-------|-------|---------|
| Header | 2 | 0x5A 0xA5 | Sync bytes |
| Length | 1 | 0x06 | Fixed (1 cmd + 5 data) |
| Command | 1 | 0x83 | Write VP response |
| Addr High | 1 | Echo | Echoed address MSB |
| Addr Low | 1 | Echo | Echoed address LSB |
| Val High | 1 | 0x00-0xFF | Value MSB |
| Val Low | 1 | 0x00-0xFF | Value LSB |
| CRC Low* | 1 | 0x00-0xFF | CRC-16 LSB (if enabled) |
| CRC High* | 1 | 0x00-0xFF | CRC-16 MSB (if enabled) |

#### Practical Example

Read VP address 0x0010 (page ID):
```
Request:  5A A5 04 82 00 10
Response: 5A A5 06 83 00 10 00 02  
          ├─ VP 0x0010 contains value 0x0002 (Page 2 active)
```

#### Timing Characteristics

| Metric | Value | Notes |
|--------|-------|-------|
| Response Time | <10ms | Typical at 115200 baud |
| 95-percentile | <15ms | Display processing variation |
| Error Response | None | Invalid VP silently ignored |
| Timeout | 100ms | Recommended host timeout |

---

### 3.2 Command 0x83: Write VP (Host → Display)

**Purpose**: Write value to display memory at VP address

#### Request Format
```
0x5A 0xA5 0x06 0x83 <ADDR_H> <ADDR_L> <VAL_H> <VAL_L> [CRC_L CRC_H]
```

| Field | Bytes | Value | Purpose |
|-------|-------|-------|---------|
| Header | 2 | 0x5A 0xA5 | Sync bytes |
| Length | 1 | 0x06 | 1 cmd + 5 data (or variable for strings) |
| Command | 1 | 0x83 | Write VP command |
| Addr High | 1 | 0x00-0xFF | VP address MSB |
| Addr Low | 1 | 0x00-0xFF | VP address LSB |
| Val High | 1 | 0x00-0xFF | Value MSB |
| Val Low | 1 | 0x00-0xFF | Value LSB |
| CRC Low* | 1 | 0x00-0xFF | CRC-16 LSB (if enabled) |
| CRC High* | 1 | 0x00-0xFF | CRC-16 MSB (if enabled) |

#### Response Format (Optional)

Only sent when `RESPONSE_UART = 1` in config.h:

```
0x4F 0x4B
```

| Field | Bytes | Value | Purpose |
|-------|-------|-------|---------|
| Char 1 | 1 | 0x4F | ASCII 'O' |
| Char 2 | 1 | 0x4B | ASCII 'K' |

**Response Timing**:
- Sent by display ~1-5ms after receiving write command
- Indicates successful frame parsing
- Does NOT confirm successful display update

#### Practical Example

Write value 0x1234 to VP 0x0020 (example: motor speed):
```
Request:  5A A5 06 83 00 20 12 34
Response: 4F 4B  (if RESPONSE_UART=1)
```

#### Multi-Byte Data Write (Text Strings)

For writing text, length byte increases:

```
Example: Write "TEST" (4 bytes) to VP 0x1000

Request Frame:
  0x5A 0xA5                    ; Header
  0x08                         ; Length (1 cmd + 2 addr + 4 data + 1 term)
  0x83                         ; Write command
  0x10 0x00                    ; VP address 0x1000
  0x54 0x45 0x53 0x54         ; "TEST" in ASCII
  0x00                         ; String terminator
  [CRC_L CRC_H]               ; Optional CRC
```

---

### 3.3 Command 0x84: Write Text with Formatting

**Purpose**: Write formatted text with newlines and color codes (less common)

**Note**: Implementation depends on display firmware version. Check `T5L_DGUSII_V2.9-0207.pdf` for full specification.

#### Request Format (Variable)
```
0x5A 0xA5 <LEN> 0x84 <ADDR_H> <ADDR_L> <TEXT_DATA...> [CRC]
```

- **Length**: Calculated as 1 (cmd) + 2 (addr) + text length
- **Text Data**: String bytes + formatting codes
- **Formatting**: Non-ASCII bytes for color, newline, etc. (device-specific)

---

### 3.4 Command 0x85: Graph Data Update (Optional)

Some T5L variants support graph/curve channel updates:

```
0x5A 0xA5 <LEN> 0x85 <CHANNEL> <VAL_H> <VAL_L> [CRC]
```

**Check Device Manual**: Verify support before using in production code.

---

## 4. VP Address Space Architecture

### 4.1 Address Range Organization

```
0x0000 ┌─────────────────────────────────────┐
       │  System VP Addresses (Reserved)     │
       │  - Current Page ID (0x0084)         │
       │  - Touch Coordinates (0x00A0-A3)    │
       │  - RTC Data (varies)                │
       │  - Device Status                    │
0x1000 ├─────────────────────────────────────┤
       │  DGUS Built-In Widgets              │
       │  - Slider values                    │
       │  - Button states                    │
       │  - Graph data channels              │
       │  - Text buffers                     │
0x4000 ├─────────────────────────────────────┤
       │  User Application VP Space          │
       │  - Sensor readings                  │
       │  - Control parameters               │
       │  - Application state                │
       │  - Custom variables                 │
0xF000 ├─────────────────────────────────────┤
       │  NOR Flash Storage (Persistent)     │
       │  - Stored configuration             │
       │  - Calibration data                 │
       │  - User preferences                 │
0xFFFF └─────────────────────────────────────┘
       End of address space
```

### 4.2 System VP Addresses (Reserved)

| Address | Name | Type | Access | Purpose |
|---------|------|------|--------|---------|
| 0x0084 | Page ID | u16 | R/W | Current active page (0-N) |
| 0x0088 | System Status | u16 | R | Device status flags |
| 0x00A0 | Touch X | u16 | R | Touch panel X coordinate |
| 0x00A2 | Touch Y | u16 | R | Touch panel Y coordinate |
| 0x00AE | Time (HH) | u16 | R/W | Hours (RTC) |
| 0x00B0 | Time (MM) | u16 | R/W | Minutes (RTC) |
| 0x00B2 | Time (SS) | u16 | R/W | Seconds (RTC) |

**Note**: Exact addresses vary by display model. Verify in project's [include/addresses.h](../include/addresses.h).

### 4.3 DGUS Built-in Widget VP Ranges

These are allocated by the DGUS design tool for UI elements:

| Widget Type | Typical Range | Count | Notes |
|-------------|---------------|-------|-------|
| Text Input | 0x0100-0x01FF | 256 | Text buffer areas |
| Sliders | 0x0200-0x02FF | 256 | Slider positions (0-N) |
| Buttons | 0x0300-0x03FF | 256 | Button press counters |
| Graphs | 0x0400-0x04FF | 256+ | Graph/chart data points |
| Switches | 0x0500-0x05FF | 256 | Toggle states (0/1) |

**Exact mapping**: Defined in DGUS HMI design file or project documentation.

### 4.4 User Application VP Space (0x4000-0x7FFF)

Typically available for custom application data:

```c
// Example allocation in include/addresses.h
#define VP_TEMPERATURE    0x4000  // Current temperature (°C × 10)
#define VP_HUMIDITY       0x4002  // Humidity percentage
#define VP_PRESSURE       0x4004  // Pressure (hPa)
#define VP_MOTOR_SPEED    0x4006  // Motor PWM (0-100%)
#define VP_ALARM_STATUS   0x4008  // Alarm flags
#define VP_ERROR_CODE     0x400A  // Last error code
// ...
#define VP_USER_END       0x7FFF  // End of user space
```

---

## 5. Communication Patterns and Workflows

### 5.1 Typical Request/Response Cycle

```
Host (MCU)                              Display
    │                                      │
    ├─ Send: 0x82 read VP 0x0010 ───────>│
    │                                      │
    │<───── Response: 0x83 value 0x0002 ──┤
    │                                      │
    ├─ Parse VP and value                 │
    ├─ Update application state           │
    │                                      │
    ├─ Send: 0x83 write VP 0x0020 ──────>│
    │                                      │
    │ (wait for optional 0x4F 0x4B)       │
    │<───── OK response (optional) ───────┤
    │                                      │
    ├─ Continue main loop                 │
```

### 5.2 Auto-Upload Mode (Display → Host)

When enabled (`DATA_UPLOAD_UART = 1`), display automatically sends VP changes:

```
User touches slider on display
         │
         ├─ Display detects change
         ├─ Reads new slider value
         │
         ├─ Display sends: 0x83 05 <VP_H> <VP_L> <VAL_H> <VAL_L>
         └─────────────────────────────────────>   Host MCU
                                                      │
                                                      ├─ Receive frame
                                                      ├─ Parse VP address
                                                      ├─ Extract data value
                                                      ├─ Update application
                                                      └─ Send OK (if enabled)

Result: MCU automatically notified of UI changes without polling
```

### 5.3 Burst Write Pattern (Multiple VPs)

For efficiency, host can queue multiple writes:

```
Write 10 VP values in rapid succession:
  1. Send: 0x83 VP1 VAL1
  2. Send: 0x83 VP2 VAL2
  3. Send: 0x83 VP3 VAL3
  ...
  10. Send: 0x83 VP10 VAL10
  
All responses received asynchronously:
  Responses may arrive out of order: OK, OK, OK, ...
  
Advantage: Lower latency for bulk updates
Disadvantage: More complex state tracking
```

### 5.4 Multi-Page Navigation Pattern

```
// Initial state: Page 1 active
// User clicks "Next Page" button on display
// Display sends auto-upload: VP 0x0084 (page ID) = 2

Host receives page change:
  1. Parse VP 0x0084 = 0x0002
  2. Recognize page switch
  3. Update application to "Page 2" mode
  4. Prepare Page 2 data
  5. Send all Page 2 VP values:
     - 0x0100 = "Page 2 Title"
     - 0x0200 = Slider value for Page 2
     - 0x0300 = Button state
     etc.

Result: Display shows Page 2 with updated values
```

---

## 6. Error Handling and Edge Cases

### 6.1 Frame Corruptions

#### Invalid Sync Bytes
```
Received: 0x5B 0xA5 ...  (first byte wrong)
Action:   Discard frame, search for next 0x5A
Recovery: Resynchronize on next valid frame
```

#### Invalid Length Field
```
Received: 0x5A 0xA5 0xFF 0x82 ...  (length 0xFF exceeds buffer)
Action:   Timeout and discard
Recovery: Skip to next frame, possibly perform UART reset
```

#### CRC Mismatch (if USE_CRC=1)
```
Received frame with CRC error:
Action:   Discard frame
Transmit: Send nothing (no NAK in DGUS II)
Recovery: Host timeout, automatic retry at host level
```

### 6.2 Timeout Behavior

| Scenario | Timeout | Action |
|----------|---------|--------|
| Waiting for VP read response | 100ms | Retry or handle as error |
| Waiting for write OK (0x4F 0x4B) | 50ms | Continue without confirmation |
| Frame reception (mid-frame) | 10ms | If no new bytes, abort frame |
| RX buffer overflow | N/A | Drop oldest data, buffer warning |

### 6.3 Invalid VP Address Write

When host attempts to write to invalid VP (e.g., read-only address):

```
Behavior: Display silently ignores write
Host:     Frame is accepted (no error response)
Result:   Write has no effect on display state
Detection: Only via explicit read of same VP afterward
```

### 6.4 Broadcast Write (Some Displays)

On multi-UART displays, can broadcast to all UARTs:

```
Address: 0xFFFF  (special broadcast address)
Effect:  All UARTs receive write update
```

Check display manual for support.

---

## 7. Practical Protocol Examples

### Example 1: Simple Sensor Reading Display

```
Scenario: Update temperature display every 100ms

Host Code:
  1. Read temperature from sensor: 27.5°C
  2. Convert to VP format: 27.5 × 10 = 275 = 0x0113
  3. Send: 0x5A 0xA5 0x06 0x83 0x40 0x00 0x01 0x13
            └─ Write 0x0113 to VP 0x4000 (temperature)
  4. Display updates text field showing "27.5"

Display:
  1. Receives frame
  2. Parses: Write 0x0113 to VP 0x4000
  3. Updates temperature text object
  4. Refreshes screen (if automated)
  5. Sends: 0x4F 0x4B (if enabled)
```

### Example 2: Button Press Response with Multiply VP Reads

```
Display detects "START" button press:

Display sends (auto-upload):
  0x5A 0xA5 0x05 0x83 0x03 0x00 0x00 0x01
  ├─ Button VP 0x0300 changed to 0x0001 (pressed)

Host processes:
  1. Parse VP 0x0300 = 0x0001 (button pressed)
  2. Update button state in application
  3. Respond with OK: 0x4F 0x4B
  4. Initiate sequence:
     - Read system status: 0x5A 0xA5 0x04 0x82 0x00 0x88
     - Read alarm state: 0x5A 0xA5 0x04 0x82 0x40 0x08
     - Write motor speed: 0x5A 0xA5 0x06 0x83 0x40 0x06 0x00 0x64

Result: Motor starts at 100% speed after user presses button
```

### Example 3: Text String Write

```
Write "ERROR CODE: 42" to display text field at VP 0x1000:

Frame Structure:
  0x5A 0xA5           ; Header
  0x11                ; Length: 1 (0x83) + 2 (addr) + 14 (string)
  0x83                ; Write command
  0x10 0x00           ; VP address 0x1000
  0x45 0x52 0x52 0x4F 0x52 0x20 0x43 0x4F 0x44 0x45 0x3A 0x20 0x34 0x32 0x00
  ├─ ASCII: "ERROR CODE: 42" + null terminator

Transmission (hex):
  5A A5 11 83 10 00 45 52 52 4F 52 20 43 4F 44 45 3A 20 34 32 00
```

### Example 4: Graph/Chart Data Point Addition

```
Send data point to live graph (VP 0x0400):

Request:
  0x5A 0xA5 0x06 0x83 0x04 0x00 0x00 0x2A
  ├─ Add point with value 42 (0x002A) to graph

Display:
  1. Receives new point
  2. Plots on graph (auto-scrolling)
  3. Updates screen
  
Host: Repeat every 100ms for continuous streaming
```

---

## 8. CRC-16/MODBUS Calculation Details

### 8.1 Algorithm Implementation

```c
u16 crc16_modbus(u8 *data, u8 len)
{
    u16 crc = 0xFFFF;
    u8 i, j;
    
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;  // Reflected polynomial
            } else {
                crc = crc >> 1;
            }
        }
    }
    
    return crc;  // CRC result (little-endian on wire)
}
```

### 8.2 Example CRC Calculation

```
Frame: 0x5A 0xA5 0x04 0x82 0x00 0x10 (without CRC)
CRC calculation over: 0x82 0x00 0x10 (command + data only)

Step-by-step:
  Initial CRC: 0xFFFF
  
  byte[0] = 0x82:
    CRC ^= 0x82 → 0xFF7D
    [process 8 bits] → result: 0x7F4E
  
  byte[1] = 0x00:
    CRC ^= 0x00 → 0x7F4E
    [process 8 bits] → result: 0x3FA7
  
  byte[2] = 0x10:
    CRC ^= 0x10 → 0x3FB7
    [process 8 bits] → result: 0xBFC4

Final CRC: 0xBFC4
Transmission: 0xC4 0xBF (low byte first)

Complete Frame: 0x5A 0xA5 0x04 0x82 0x00 0x10 0xC4 0xBF
```

### 8.3 CRC Validation at Receiver

```c
// Received frame
u8 frame[] = {0x5A, 0xA5, 0x04, 0x82, 0x00, 0x10, 0xC4, 0xBF};

// Extract CRC from frame
u16 rx_crc = (frame[7] << 8) | frame[6];  // High, low bytes

// Calculate CRC over command + data (indices 3-5)
u8 data[] = {0x82, 0x00, 0x10};
u16 calc_crc = crc16_modbus(data, 3);

// Validate
if (rx_crc == calc_crc) {
    // Valid frame
} else {
    // CRC error - discard frame
}
```

---

## 9. Protocol State Machine

### 9.1 Frame Reception State Machine

```
┌─────────────────┐
│  WAIT_SYNC_1    │  Expecting 0x5A
└────────┬────────┘
         │ byte = 0x5A
         ├──────────────────────┐
         │                      │ byte ≠ 0x5A
         v                      v
┌──────────────────┐   ┌─────────────────┐
│   WAIT_SYNC_2    │◄──┤ Re-sync search  │
│ Expecting 0xA5   │   └─────────────────┘
└────────┬─────────┘
         │ byte = 0xA5
         v
┌─────────────────┐
│   WAIT_LENGTH   │  Expecting/Parse length
└────────┬────────┘
         v
┌─────────────────────────────────────┐
│   WAIT_COMMAND_AND_DATA             │
│   Read (length) bytes               │
└────────┬────────────────────────────┘
         v
  ┌──────────────────────┐
  │  VERIFY_CRC (if en)  │
  └────────┬─────────────┘
           │
           v
  ┌──────────────────────┐
  │  FRAME_COMPLETE      │◄─ Ready for processing
  └──────────────────────┘
```

### 9.2 Frame Transmission State Machine

```
┌─────────────────────┐
│  Build Frame:       │
│  - Sync bytes       │
│  - Length           │
│  - Command          │
│  - Data payload     │
│  - CRC (if enabled) │
└────────┬────────────┘
         v
┌──────────────────────────┐
│  TX_QUEUE_FULL?          │
└────┬───────────────┬─────┘
     │               │
  NO │               │ YES
     v               v
┌──────────────┐    Retry/Error
│  TRANSMIT    │
│  Frame bytes │
└────┬─────────┘
     v
┌──────────────────────┐◄─ Wait for TX complete
│  DONE                │
└──────────────────────┘
```

---

## 10. Protocol Configuration

### 10.1 Configuration Flags (from [include/config.h](../include/config.h))

```c
#define USE_CRC         0   // 1 = enable CRC-16, 0 = disable
#define RESPONSE_UART2  1   // 1 = send 0x4F 0x4B after write
#define RESPONSE_UART3  1
#define RESPONSE_UART4  1
#define RESPONSE_UART5  1

#define DATA_UPLOAD_UART2  1  // 1 = receive auto-uploads from display
#define DATA_UPLOAD_UART3  1
#define DATA_UPLOAD_UART4  1
#define DATA_UPLOAD_UART5  1

#define BAUD_UART2  115200  // Baud rate
#define BAUD_UART3  115200
#define BAUD_UART4  115200
#define BAUD_UART5  115200
```

### 10.2 Runtime Protocol Behavior

| Feature | Enabled | Disabled | Notes |
|---------|---------|----------|-------|
| CRC Validation | Checked | Ignored | Errors detected only with CRC |
| Response (0x4F 0x4B) | Sent | Not sent | Display may time out if expected |
| Auto-upload | Accepted | Ignored | UI changes won't reach MCU |
| Baud Rate Sync | Critical | Critical | Must match display configuration |

---

## 11. Troubleshooting Protocol Issues

### Issue 1: "Display not responding to reads"
```
Symptom: 0x82 read commands get no response
Causes:
  ✓ Baud rate mismatch (verify with serial monitor)
  ✓ Sync byte corruption (check cable quality)
  ✓ Invalid VP address (verify address range)
  ✓ Display firmware issue (try factory reset)
  ✓ UART not connected or disabled
  
Fix: Verify baud rate first, check cable continuity
```

### Issue 2: "CRC errors occurring frequently"
```
Symptom: Received frames with invalid CRC
Causes:
  ✓ Electrical noise on serial line (poor cable)
  ✓ CRC poly mismatch (implementation error)
  ✓ Byte transmission error (EMI/RFI)
  ✓ Display firmware variant (different CRC scheme)
  
Fix: Check cable, add ferrite core, verify CRC algorithm
```

### Issue 3: "Writes not appearing on display"
```
Symptom: 0x83 commands accepted but display doesn't change
Causes:
  ✓ Wrong VP address (check mapping)
  ✓ Read-only address (some VP are read-only)
  ✓ Display buffer full (too many writes)
  ✓ Page mismatch (writing to different page)
  ✓ Display design error (object not mapped to VP)
  
Fix: Verify VP address, check display design file
```

### Issue 4: "Garbled text in display fields"
```
Symptom: Text appears as random characters
Causes:
  ✓ Character encoding mismatch (ASCII vs Unicode)
  ✓ String not null-terminated
  ✓ Data alignment error (writing to wrong offset)
  ✓ Display using UTF-16 (need encoding conversion)
  
Fix: Verify encoding, add null terminator, check alignment
```

---

## 12. Related Documentation

| Document | Purpose |
|----------|---------|
| [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md) | Hardware details and SFR configuration |
| [MEMORY_MANAGEMENT.md](MEMORY_MANAGEMENT.md) | Memory layout for buffers and VP storage |
| [VP_ADDRESS_MAP.md](VP_ADDRESS_MAP.md) | Detailed VP address reference |
| [PROTOCOL_EXAMPLES.md](PROTOCOL_EXAMPLES.md) | Real-world protocol examples |
| [DEBUGGING_GUIDE.md](DEBUGGING_GUIDE.md) | Troubleshooting and diagnostic techniques |

---

## 13. Key References

### Internal Code References
- [lib/uart/uart.c](../lib/uart/uart.c) - Frame parsing implementation
- [lib/sys/sys.c](../lib/sys/sys.c) - VP read/write functions
- [lib/crc16/crc16.c](../lib/crc16/crc16.c) - CRC-16/MODBUS calculation
- [include/config.h](../include/config.h) - Protocol configuration

### External References
- **DGUS II Development Guide** (`T5L_DGUSII_V2.9-0207.pdf`)
  - Chapter 1: Communication Protocol Specification
  - Chapter 2: VP Address Mapping
  - Appendix A: CRC Calculation Method
  
- **T5L ASIC Datasheet** (`T5L-ASIC-2024-09-25.pdf`)
  - Section 6: UART Specifications
  - Section 7: Baud Rate Configuration

---

**Document Version**: 1.0  
**Last Updated**: May 7, 2026  
**Status**: Complete - Ready for Phase 1 (v0.2.0)
