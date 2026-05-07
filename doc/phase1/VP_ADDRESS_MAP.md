# VP Address Map Reference

**Complete Virtual Processor (VP) address space documentation**

---

## Overview

VP (Virtual Processor) addresses are the primary mechanism for accessing DGUS II display memory. Each VP address corresponds to a 16-bit value (word) in the display's memory space. This guide provides the complete VP address map for typical T5L configurations.

### 1.1 VP Addressing Characteristics

- **Address Range**: 0x0000 - 0xFFFF (65,536 unique addresses)
- **Data Width**: 16-bit values (two bytes, big-endian transmission)
- **Access Modes**: Read, Write, or Read-Only (depends on VP)
- **Space Type**: Primarily RAM (volatile), some Flash (persistent)
- **Update Behavior**: Auto-upload on change (if enabled), or polling

### 1.2 How VPs Work

```
MCU Application              Display (T5L)
      │                            │
      ├─ Read VP 0x0084 ────────>│ 
      │  (Get current page)       ├─ Look up value in DGUS RAM
      │                            │
      │<── Response: 0x0001 ──────┤
      │                            │
      │  [Page ID = 1]             │
      │                            │
      ├─ Write VP 0x0100 ────────>│
      │  (Text buffer)             ├─ Write value to DGUS RAM
      │                            ├─ UI object linked to VP updates
      │                            │
      │<── OK (optional) ──────────┤
      │                            │
      │  [Display shows new text]  │
```

---

## 2. System VP Addresses (0x0000-0x00FF)

Reserved for device control and status.

### 2.1 Core System VPs

| Address | Name | Type | Access | Range | Purpose |
|---------|------|------|--------|-------|---------|
| 0x0084 | Page ID | u16 | R/W | 0-N | Current active page number |
| 0x0086 | Touch Enable | u16 | R/W | 0/1 | Enable/disable touch panel |
| 0x0088 | System Status | u16 | R | Varies | Device status flags |
| 0x008A | VBar Pointer | u16 | R/W | 0-100 | Vertical scroll position |
| 0x008C | HBar Pointer | u16 | R/W | 0-100 | Horizontal scroll position |

### 2.2 Touch Panel Input (0x00A0-0x00A3)

| Address | Name | Type | Access | Range | Purpose |
|---------|------|------|--------|-------|---------|
| 0x00A0 | Touch X | u16 | R | 0-DisplayWidth | Touch X coordinate |
| 0x00A2 | Touch Y | u16 | R | 0-DisplayHeight | Touch Y coordinate |

**Touch Coordinate Notes**:
- Read immediately after touch event
- Coordinates may become stale if not read quickly
- Typically used with object-based touch instead of raw coordinates

### 2.3 Real-Time Clock (RTC) Support (0x00AE-0x00B6)

| Address | Name | Type | Access | 0-59/0-23 | Purpose |
|---------|------|------|--------|-----------|---------|
| 0x00AE | Hours | u16 | R/W | 0-23 | Current hour (24-hour format) |
| 0x00B0 | Minutes | u16 | R/W | 0-59 | Current minute |
| 0x00B2 | Seconds | u16 | R/W | 0-59 | Current second |
| 0x00B4 | Date | u16 | R/W | 1-31 | Day of month |
| 0x00B6 | Month | u16 | R/W | 1-12 | Month number |
| 0x00B8 | Year | u16 | R/W | 2000-2099 | Year (some devices: 0-99) |

**RTC Notes**:
- Only populated if RTC enabled in [include/config.h](../include/config.h)
- External RTC (RX8130, SD2058) synced at startup
- Can be written to set time

### 2.4 Firmware Version / Device Info (0x00FE-0x00FF)

| Address | Name | Type | Access | Purpose |
|---------|------|------|--------|---------|
| 0x00FE | Device Type | u16 | R | Display model code |
| 0x00FF | Firmware Ver | u16 | R | Firmware version number |

---

## 3. DGUS Built-in Widget VPs (0x0100-0x0FFF)

These addresses are automatically created by the DGUS design tool. Exact usage depends on your `.dgus` design file.

### 3.1 Typical Widget Layout

#### Text Input/Display Buffers (0x0100-0x01FF)

| Base Address | Widget Type | Capacity | Purpose |
|--------------|-------------|----------|---------|
| 0x0100 | Text 1 | 32-64B | Line 1 display text |
| 0x0110 | Text 2 | 32-64B | Line 2 display text |
| 0x0120 | Text 3 | 32-64B | Line 3 display text |
| ... | ... | ... | Additional text fields |
| 0x01E0 | Status Text | 32B | Status/error messages |

**Example Usage**:
```c
#define VP_TEXT_LINE1    0x0100
#define VP_TEXT_LINE2    0x0110
#define VP_STATUS_TEXT   0x01E0

// Write "Temperature" to Line 1 text buffer
DGUS_WriteText(VP_TEXT_LINE1, "Temperature");
```

#### Numeric Display Fields (0x0200-0x02FF)

| Base Address | Widget Type | Access | Range | Purpose |
|--------------|-------------|--------|-------|---------|
| 0x0200 | Dial/Gauge | R/W | 0-100 | Gauge value (0-100) |
| 0x0202 | Progress Bar | R/W | 0-100 | Progress (0-100%) |
| 0x0204 | Counter | R/W | 0-9999 | Numeric counter display |
| 0x0206-0x021E | Additional vars | R/W | Varies | Extra numeric displays |

**Example Usage**:
```c
#define VP_GAUGE_TEMP      0x0200
#define VP_PROGRESS_BAR    0x0202
#define VP_COUNTER         0x0204

// Set gauge to 75%
DGUS_Write_VP(VP_GAUGE_TEMP, 75);

// Display counter value 1234
DGUS_Write_VP(VP_COUNTER, 1234);
```

#### Slider / Input Controls (0x0300-0x03FF)

| Base Address | Widget Type | Access | Range | Purpose |
|--------------|-------------|--------|-------|---------|
| 0x0300 | Slider 1 | R/W | 0-100 | Horizontal slider value |
| 0x0302 | Slider 2 | R/W | 0-100 | Another slider (e.g., brightness) |
| 0x0304 | Input Field 1 | R/W | 0-65535 | Numeric input field |
| 0x0306 | Input Field 2 | R/W | 0-65535 | Another input field |

**Example Usage**:
```c
#define VP_SLIDER_SPEED     0x0300
#define VP_BRIGHTNESS       0x0302
#define VP_INPUT_SETPOINT   0x0304

// User moves slider to 65%
// Auto-upload: VP 0x0300 = 65

if (last_vp == VP_SLIDER_SPEED) {
    motor_speed = last_value;  // Range 0-100
    set_motor_pwm(motor_speed);
}
```

#### Button Press Counters (0x0400-0x04FF)

| Base Address | Widget Type | Access | Purpose |
|--------------|-------------|--------|---------|
| 0x0400 | Button 1 Press | R/W | "START" button press counter |
| 0x0402 | Button 2 Press | R/W | "STOP" button press counter |
| 0x0404 | Button 3 Press | R/W | "RESET" button press counter |
| 0x0406-0x04FE | Additional buttons | R/W | Extra buttons (varies) |

**Button Press Pattern**:
```
Display (user presses button):
  ├─ Auto-upload: VP 0x0400 = (previous_value + 1)
  └─ Increment preserved across multiple presses

Host MCU (detects button press):
  ├─ Read VP 0x0400 → current count
  ├─ Compare with last_count
  └─ If different: Button was pressed
     (Either poll periodically or use auto-upload)
```

**Example Usage**:
```c
#define VP_BUTTON_START  0x0400
#define VP_BUTTON_STOP   0x0402
#define VP_BUTTON_RESET  0x0404

static u16 last_button_start = 0;

// In main loop:
current_count = DGUS_Read_VP(VP_BUTTON_START);

if (current_count != last_button_start) {
    last_button_start = current_count;
    // Button was pressed!
    start_motor();
}
```

#### Graph/Chart Data (0x0500-0x05FF)

| Base Address | Channel | Access | Purpose |
|--------------|---------|--------|---------|
| 0x0500 | Graph 1 (Ch 0) | W | Data point for line graph 1 |
| 0x0502 | Graph 2 (Ch 1) | W | Data point for line graph 2 |
| 0x0504 | Graph 3 (Ch 2) | W | Data point for line graph 3 |
| 0x0506-0x05FE | Additional channels | W | More graph channels |

**Graph Streaming Pattern**:
```
Application has new data point: 42

Host sends to each active graph channel:
  VP 0x0500: 42  (Graph 1 gets point)
  VP 0x0502: 42  (Graph 2 gets point)

Display handles:
  ├─ Auto-scroll display
  ├─ Plot new point
  ├─ Update axes if needed
  └─ Redraw screen

Host sends next point ~100ms later (10 Hz sample rate)
```

**Example Usage**:
```c
#define VP_GRAPH_CHANNEL0  0x0500
#define VP_GRAPH_CHANNEL1  0x0502

// Stream temperature data at 10 Hz
static u8 graph_timer = 0;

in main_loop() {
    if (++graph_timer >= 10) {  // 10 loop cycles (~100ms at 100Hz loop)
        graph_timer = 0;
        
        u16 temp_value = read_temperature();  // e.g., 2750 = 27.50°C
        DGUS_Write_VP(VP_GRAPH_CHANNEL0, temp_value);
    }
}
```

---

## 4. User Application VP Space (0x4000-0x7FFF)

Typically available for custom application data (application-specific).

### 4.1 Recommended Application VP Layout

```c
// In include/addresses.h or dedicated app_vp_defines.h

// Sensor Data VPs (0x4000-0x400F)
#define VP_TEMPERATURE      0x4000  // Temperature (°C × 100)
#define VP_HUMIDITY         0x4002  // Humidity (%)
#define VP_PRESSURE         0x4004  // Pressure (hPa)
#define VP_AIRFLOW          0x4006  // Air flow (m³/h)

// Control VPs (0x4010-0x401F)
#define VP_MOTOR_SPEED      0x4010  // Motor speed (0-100%)
#define VP_PUMP_STATE       0x4012  // Pump on/off (0/1)
#define VP_HEATER_LEVEL     0x4014  // Heater power (0-100%)
#define VP_FAN_MODE         0x4016  // Fan mode (0-3)

// Status VPs (0x4020-0x402F)
#define VP_SYSTEM_STATUS    0x4020  // Status flags
#define VP_ERROR_CODE       0x4022  // Last error (0 = no error)
#define VP_RUNTIME_HOURS    0x4024  // Total runtime (hours)
#define VP_TRIP_COUNTER     0x4026  // Number of cycles

// Setpoint VPs (0x4030-0x403F)
#define VP_SETPOINT_TEMP    0x4030  // Target temperature
#define VP_SETPOINT_RH      0x4032  // Target humidity
#define VP_ALARM_TEMP_HIGH  0x4034  // High temperature alarm
#define VP_ALARM_TEMP_LOW   0x4036  // Low temperature alarm

// Configuration VPs (0x4040-0x405F)
#define VP_TIME_OFFSET      0x4040  // GMT offset (hours)
#define VP_BRIGHTNESS       0x4042  // Display brightness (0-255)
#define VP_AUTO_SHUTOFF     0x4044  // Auto shutoff time (minutes)
#define VP_LANGUAGE         0x4046  // UI language (0-5)

// Extended Space (0x4060-0x7FFF) - Free for expansion
```

### 4.2 Application VP Example: Temperature Sensor

```c
// Header: sensor.h
void sensor_init(void);
u16 sensor_read_temperature_raw(void);        // Returns raw ADC value
u16 sensor_convert_temperature(u16 raw);      // Returns °C × 100

// Implementation: sensor.c
u16 sensor_read_temperature_raw(void) {
    // Read ADC channel, apply calibration
    return adc_raw_value;
}

u16 sensor_convert_temperature(u16 raw) {
    // Convert to °C × 100
    // E.g., 25.43°C returned as 2543
    s16 temp_celsius = ((s32)raw * 100) / 1024;
    return (u16)temp_celsius;
}

// Main loop integration:
in main():
    u16 temp_raw = sensor_read_temperature_raw();
    u16 temp_display = sensor_convert_temperature(temp_raw);
    
    // Update display: Temperature field
    DGUS_Write_VP(VP_TEMPERATURE, temp_display);
```

---

## 5. VP Access Patterns and Best Practices

### 5.1 Read-Only VPs (System Status)

These VPs should only be read, not written:

```c
#define VP_TOUCH_X        0x00A0  // Do not write
#define VP_TOUCH_Y        0x00A2  // Do not write
#define VP_SYSTEM_STATUS  0x0088  // Do not write

// Correct usage:
u16 status = DGUS_Read_VP(VP_SYSTEM_STATUS);
if (status & 0x01) {
    // System error detected
}
```

### 5.2 Write-Only VPs (Display Updates)

These are typically updated by MCU, not read:

```c
#define VP_TEXT_DISPLAY  0x0100   // Write only
#define VP_GAUGE         0x0200   // Write only

// Correct usage:
DGUS_WriteText(VP_TEXT_DISPLAY, "Ready");
DGUS_Write_VP(VP_GAUGE, 50);

// Avoid reading back (may not reflect latest write):
// u16 val = DGUS_Read_VP(VP_GAUGE);  // ← NOT RECOMMENDED
```

### 5.3 Read-Write VPs (Dual Direction)

These support both read and write operations:

```c
#define VP_PAGE_ID       0x0084   // Can read current page or write to change
#define VP_SLIDER_VALUE  0x0300   // Can read user input, write to update

// Navigation example:
DGUS_Write_VP(VP_PAGE_ID, 2);     // Switch to page 2
// Display updates
// Later: read to verify
u16 current_page = DGUS_Read_VP(VP_PAGE_ID);
```

### 5.4 VP Polling vs. Auto-Upload

**Polling Approach** (ask display for value):
```c
// Read slider value every 100ms
static u8 poll_timer = 0;

if (++poll_timer >= 10) {  // 10 × 10ms = 100ms
    poll_timer = 0;
    u16 slider_value = DGUS_Read_VP(VP_SLIDER_SPEED);
    apply_motor_speed(slider_value);
}
```

**Auto-Upload Approach** (display tells MCU when changed):
```c
// Display sends VP 0x0300 whenever slider moves
// Handled in DGUS_ProcessAllUarts() and stored in last_vp/last_value

if (last_vp == VP_SLIDER_SPEED) {
    apply_motor_speed(last_value);
    last_vp = 0;  // Mark as processed
}
```

**Comparison**:
| Approach | Pros | Cons |
|----------|------|------|
| Polling | Predictable, simple | Uses more bandwidth |
| Auto-upload | Responsive, efficient | Requires more code |

---

## 6. Multi-Page VP Organization

When using multiple pages in DGUS design:

### 6.1 Page Switching Mechanism

```c
#define VP_PAGE_ID  0x0084

// MCU initiates page change
DGUS_Write_VP(VP_PAGE_ID, PAGE_SETTINGS);  // Switch to settings page

// Display-initiated (user presses button):
// Auto-upload: VP 0x0084 = PAGE_SETTINGS
// MCU detects change:
if (last_vp == VP_PAGE_ID && last_value == PAGE_SETTINGS) {
    current_page = PAGE_SETTINGS;
    // Refresh all page-specific data
    update_page_display();
}
```

### 6.2 Per-Page VP Strategy

```c
// Page 1 (Main Display)
#define VP_PAGE1_SENSOR1   0x4100
#define VP_PAGE1_SENSOR2   0x4102
#define VP_PAGE1_CONTROL   0x4104

// Page 2 (Settings)
#define VP_PAGE2_PARAM1    0x4200
#define VP_PAGE2_PARAM2    0x4202
#define VP_PAGE2_PARAM3    0x4204

// Page 3 (Diagnostics)
#define VP_PAGE3_ERROR1    0x4300
#define VP_PAGE3_ERROR2    0x4302
#define VP_PAGE3_STATUS    0x4304

// In main loop:
switch (current_page) {
    case PAGE_MAIN:
        DGUS_Write_VP(VP_PAGE1_SENSOR1, sensor1_value);
        DGUS_Write_VP(VP_PAGE1_SENSOR2, sensor2_value);
        break;
    case PAGE_SETTINGS:
        // Only update page 2 VPs
        break;
    // etc.
}
```

---

## 7. VP Address Allocation Strategy

### 7.1 Address Range Planning

```
0x0000-0x00FF  System VPs (RESERVED)
               ├─ 0x0000-0x007F: Display control
               ├─ 0x0080-0x00AF: RTC/status
               └─ 0x00B0-0x00FF: Reserved

0x0100-0x0FFF  DGUS Built-in (auto-allocated by design tool)
               ├─ 0x0100-0x01FF: Text buffers
               ├─ 0x0200-0x02FF: Numeric displays
               ├─ 0x0300-0x03FF: Input controls
               ├─ 0x0400-0x04FF: Button counters
               ├─ 0x0500-0x05FF: Graph data
               ?  0x0600-0x0FFF: Additional widgets

0x1000-0x3FFF  Reserved for extension (future displays)

0x4000-0x4FFF  Application VP Space (recommended for most projects)
               ├─ 0x4000-0x40FF: Sensor data (16 sensors)
               ├─ 0x4100-0x41FF: Control commands (16 controls)
               ├─ 0x4200-0x42FF: Status/alarms (16 status)
               └─ 0x4300-0x4FFF: Custom data

0x5000-0x7FFF  Extended application space (if needed)
```

### 7.2 Allocation Pattern

```c
// 16-byte slots for different data types
#define VP_SLOT_SIZE  0x0002  // Each VP holds u16

// Sensor group (0x4000-0x001F)
#define VP_SENSOR_BASE     0x4000
#define VP_SENSOR_TEMP     (VP_SENSOR_BASE + 0x0000)  // 0x4000
#define VP_SENSOR_RH       (VP_SENSOR_BASE + 0x0002)  // 0x4002
#define VP_SENSOR_PRESS    (VP_SENSOR_BASE + 0x0004)  // 0x4004
// ...

// Control group (0x4100-0x411F)
#define VP_CONTROL_BASE    0x4100
#define VP_CONTROL_MOTOR   (VP_CONTROL_BASE + 0x0000)  // 0x4100
#define VP_CONTROL_PUMP    (VP_CONTROL_BASE + 0x0002)  // 0x4102
// ...
```

---

## 8. Common VP Value Ranges and Encodings

### 8.1 Temperature Encoding Example

```c
// Display range: -40°C to +125°C
// Encoding: Actual temperature × 10 (fixed-point, one decimal)

#define TEMP_OFFSET     400  // -40°C = 0 in display
#define TEMP_SCALE      10   // 1 unit = 0.1°C

// Conversion: Raw temperature to VP value
s16 temp_raw = adc_read_temperature();  // e.g., 2500 (25.0°C)
u16 vp_value = temp_raw + TEMP_OFFSET * TEMP_SCALE;
//            = 2500 + 4000 = 6500
DGUS_Write_VP(VP_TEMPERATURE, vp_value);

// Reverse: VP value to display
// Display firmware: vp_value - 4000 = 2500 → show "25.0°C"
```

### 8.2 Percentage Encoding

```c
// Motor speed: 0-100%
// Encoding: Direct percentage (0-100)

#define MOTOR_OFF        0
#define MOTOR_HALF_SPEED 50
#define MOTOR_FULL_SPEED 100

DGUS_Write_VP(VP_MOTOR_SPEED, MOTOR_HALF_SPEED);  // 50% speed
```

### 8.3 Enumeration Encoding

```c
// Fan mode selection
#define FAN_OFF      0
#define FAN_LOW      1
#define FAN_MEDIUM   2
#define FAN_HIGH     3
#define FAN_AUTO     4

DGUS_Write_VP(VP_FAN_MODE, FAN_AUTO);  // Set to auto mode
```

### 8.4 Bit Flags Encoding

```c
#define STATUS_RUNNING    0x01  // Bit 0
#define STATUS_ERROR      0x02  // Bit 1
#define STATUS_ALARM      0x04  // Bit 2
#define STATUS_MAINTENANCE 0x08 // Bit 3

u16 status = 0;
status |= STATUS_RUNNING;   // Set running flag
status |= STATUS_ERROR;     // Set error flag
// status = 0x03

DGUS_Write_VP(VP_STATUS, status);

// Display firmware extracts:
// if (status & STATUS_ERROR) show_error_light();
```

---

## 9. VP Address Conflicts and Resolution

### 9.1 Detecting Address Conflicts

When multiple UI elements map to the same VP:

```
Problem: Two text objects configured to use VP 0x0100
  Text1: "Temperature"  → VP 0x0100
  Text2: "Humidity"     → VP 0x0100  (CONFLICT!)

Result: Writing to VP 0x0100 updates both text objects incorrectly
```

**Resolution**:
1. Verify in DGUS design tool each object has unique VP
2. Check project documentation for allocated VPs
3. Use automated address checker (if available)
4. Manually test each VP read/write

### 9.2 VP Address Collision Examples

```c
// Example: Checking for overlap between DGUS-allocated and app VPs

// DGUS allocates 0x0100-0x03FF for widgets
#define DGUS_VP_START  0x0100
#define DGUS_VP_END    0x03FF

// Application can safely use 0x4000+
#define APP_VP_START   0x4000

// Verification:
if (APP_VP_START <= DGUS_VP_END) {
    #error "VP address space conflict!"
}
```

---

## 10. VP Documentation Template

For your project, create a VP reference table:

### 10.1 Template Table

| Address | Name | Type | R/W | Range | Purpose | Notes |
|---------|------|------|-----|-------|---------|-------|
| 0x4000 | Temp Sensor | u16 | R | 0-9999 | Current temperature (°C×100) | Read from MCU |
| 0x4002 | Motor Speed | u16 | R/W | 0-100 | Motor PWM (0-100%) | Auto-upload on change |
| 0x4004 | Error Code | u16 | R | 0-15 | Last error number | 0=no error |

### 10.2 Real Example for Your Project

Create file: [include/addresses.h](../include/addresses.h) (if not already detailed)

```c
/*
 * VP Address Definitions for DWIN-T5L-SDCC-Template
 * 
 * VP Address Space:
 * 0x0000-0x00FF: System (reserved)
 * 0x0100-0x03FF: DGUS Design Tool (widgets)
 * 0x4000-0x7FFF: Application Space
 */

// System VPs
#define VP_PAGE_ID          0x0084
#define VP_TOUCH_X          0x00A0
#define VP_TOUCH_Y          0x00A2

// Application VPs (example)
#define VP_SENSOR_TEMP      0x4000
#define VP_MOTOR_SPEED      0x4002
#define VP_STATUS_FLAGS     0x4004
```

---

## 11. VP Debugging and Monitoring

### 11.1 VP Read/Write Logging

```c
void log_vp_access(const char *operation, u16 addr, u16 value) {
    // Log to UART for debugging
    uart_send_str("VP_");
    uart_send_str(operation);
    uart_send_hex(addr);
    uart_send_str("=");
    uart_send_hex(value);
    uart_send_str("\r\n");
}

// In read function:
u16 vp_read_logged(u16 addr) {
    u16 val = DGUS_Read_VP(addr);
    log_vp_access("READ", addr, val);
    return val;
}

// In write function:
void vp_write_logged(u16 addr, u16 val) {
    DGUS_Write_VP(addr, val);
    log_vp_access("WRITE", addr, val);
}
```

### 11.2 VP Range Validation

```c
int vp_is_valid(u16 addr) {
    // System VPs
    if (addr <= 0x00FF) return 1;
    
    // DGUS design VPs
    if (addr >= 0x0100 && addr <= 0x03FF) return 1;
    
    // Application VPs
    if (addr >= 0x4000 && addr <= 0x7FFF) return 1;
    
    // All others: invalid
    return 0;
}

// Usage:
if (!vp_is_valid(addr)) {
    uart_send_str("ERROR: Invalid VP address\r\n");
}
```

---

## 12. Related Documentation

| Document | Purpose |
|----------|---------|
| [DGUS_PROTOCOL_GUIDE.md](DGUS_PROTOCOL_GUIDE.md) | How to read/write VPs |
| [MEMORY_MANAGEMENT.md](MEMORY_MANAGEMENT.md) | VP space storage in XDATA |
| [PROTOCOL_EXAMPLES.md](PROTOCOL_EXAMPLES.md) | Real examples of VP reads/writes |

---

## 13. Key References

### Internal References
- [include/addresses.h](../include/addresses.h) - Project-specific VP definitions
- [lib/sys/sys.c](../lib/sys/sys.c) - VP read/write implementations
- [src/app/app_defs/app_defs.h](../src/app/app_defs/app_defs.h) - Application VP constants

### External References
- **DGUS II Development Guide** (`T5L_DGUSII_V2.9-0207.pdf`)
  - Chapter 3: VP Address Mapping
  - Chapter 4: Built-in VP Definitions
  - Appendix B: Default Address Table
  
- **T5L ASIC Datasheet** (`T5L-ASIC-2024-09-25.pdf`)
  - Section 4: Memory Map

### Design Tool Reference
- DWIN HMI Design Software Documentation
- Display model manual (includes default VP mapping)

---

**Document Version**: 1.0  
**Last Updated**: May 7, 2026  
**Status**: Complete - Ready for Phase 1 (v0.2.0)
