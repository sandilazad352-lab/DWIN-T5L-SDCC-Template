# Debugging Guide

**Comprehensive troubleshooting and diagnostic procedures for T5L firmware development**

---

## Overview

This guide covers systematic approaches to identifying and resolving common issues in T5L HMI development. Organized by symptom and root cause analysis.

---

## 1. Communication Issues

### 1.1 No Response to Read Requests (0x82)

**Symptom**: Sends `5A A5 04 82 00 84...` → No response received

####  Diagnostic Flowchart

```
No Response?
    │
    ├─ Check 1: Baud Rate Match
    │  ├─ [ ] Open serial monitor
    │  ├─ [ ] Set to 9600 baud (low speed test)
    │  ├─ [ ] Send: 0x5A 0xA5 0x04 0x82 0x00 0x84
    │  └─ [ ] Look for garbage or readable text?
    │     ├─ Garbage → Baud rate mismatch
    │     └─ Nothing → Cable issue
    │
    ├─ Check 2: Cable Connection
    │  ├─ [ ] Verify RX/TX not swapped
    │  ├─ [ ] Check continuity with multimeter
    │  ├─ [ ] Try Write first (0x83) instead
    │  └─ [ ] Use loop-back test (TX tied to RX)
    │
    ├─ Check 3: VP Address Validity
    │  ├─ [ ] Try system VP: 0x0084 (page ID)
    │  ├─ [ ] Check address is not write-only
    │  └─ [ ] Verify address in design file
    │
    └─ Check 4: Display State
       ├─ [ ] Is display powered on?
       ├─ [ ] Is display in boot/startup?
       └─ [ ] Try hard reset of display
```

#### Quick Fix Checklist

**Baud Rate Issue** (most common):
```c
// Verify configuration
// In include/config.h:
#define BAUD_UART2  115200  // Check this matches display

// Calculate in code:
u16 timer_reload = 256 - (11059200 / (12 * 32 * 115200));
// Should be: 256 - 24 = 232 = 0xE8 for TH1 register
```

**Cable Issue**:
```
Test wire continuity:
  MCU UART2 TX → Display RX
  MCU UART2 RX ← Display TX
  MCU GND      → Display GND
```

**Write-Only VP**:
```c
// Try reading different address first
u16 val = DGUS_Read_VP(0x0084);  // System status (should be readable)

// If that works, original VP might be write-only
```

#### Serial Monitor Verification

```bash
# Using minicom or similar
minicom -b 115200 -D /dev/ttyUSB0

# Send byte by byte:
$ Ctrl+A, Z for commands
$ Send ASCII: 5A A5 04 82 00 84
# Watch for response characters
```

---

### 1.2 Corrupted Responses (CRC Errors, Partial Frames)

**Symptom**: Receives partial frame or CRC mismatch

#### Root Causes

| Symptom | Likely Cause | Check |
|---------|--------------|-------|
| First byte 0x5A arrives, rest garbage | Baud rate 2x wrong | Measure bit time with scope |
| Random bit flips | Electrical noise | Add ferrite core on cable |
| Frame incomplete (timeout) | Cable too long or slow | Reduce baud rate to 9600 |
| CRC mismatch every N frames | Buffer overflow | Check UART RX buffer size |

#### Debugging Steps

**1. Capture Hex Dump**
```bash
# Using minicom with log:
$ minicom -C /tmp/com.log -b 115200 -D /dev/ttyUSB0
# Wait for transmission
# File: /tmp/com.log contains hex data
# grep -o '[0-9A-Fa-f][0-9A-Fa-f]' /tmp/com.log | tr '\n' ' '
# Output: 5A A5 06 83 ... (shows raw bytes)
```

**2. Frame Structure Validation**
```c
// In debug function:
void validate_frame(u8 *frame, u8 len) {
    // Check sync
    if (frame[0] != 0x5A || frame[1] != 0xA5) {
        uart_send_str("SYNC ERROR\r\n");
        return;
    }
    
    // Check length
    u8 stated_len = frame[2];
    if (stated_len > len - 5) {  // -2 header -1 len -2 CRC
        uart_send_str("LENGTH ERROR\r\n");
        return;
    }
    
    // Check CRC if enabled
    if (USE_CRC) {
        u16 rx_crc = (frame[len-1] << 8) | frame[len-2];
        u16 calc = crc16_modbus(&frame[3], stated_len);
        if (rx_crc != calc) {
            uart_send_str("CRC ERROR\r\n");
            uart_send_hex(rx_crc);
            uart_send_str(" vs ");
            uart_send_hex(calc);
            uart_send_str("\r\n");
        }
    }
}
```

**3. Electromagnetic Interference (EMI) Test**
```c
// Add shielding and observe changes
// Cable changes to test:
//   - Add ferrite core near MCU
//   - Length reduction (shorter cable = less noise)
//   - Shield wrapping (foil around cable)
//   - Reduced baud rate (slower = more margin)

// Verify performance with:
void frame_error_statistics(void) {
    static u32 total_frames = 0;
    static u32 error_frames = 0;
    static u8 stat_timer = 0;
    
    if (frame_received) {
        total_frames++;
        if (frame_has_error) {
            error_frames++;
        }
    }
    
    // Report every 10 seconds
    if (++stat_timer == 100) {
        float error_rate = (float)error_frames / total_frames * 100;
        uart_send_str("Error rate: ");
        uart_send_dec((u16)error_rate);
        uart_send_str("%\r\n");
        
        // < 0.1%: Good
        // 0.1-1%: Acceptable
        // > 1%: Needs fixing
    }
}
```

---

### 1.3 Timeout on Response Wait

**Symptom**: Frame sent, wait for response, timeout (>100ms, no data)

#### Debugging

**Check 1: Verify Frame Sent**
```c
void debug_uart_send(u8 *data, u8 len) {
    u8 i;
    
    // Log what we're about to send
    uart_send_str("TX: ");
    for (i = 0; i < len; i++) {
        uart_send_hex(data[i]);
        uart_send_str(" ");
    }
    uart_send_str("\r\n");
    
    // Send it
    uart_send_buffer(data, len);
    
    // Verify TX complete (optional, depends on UART)
    while (!uart_tx_complete()) {
        // Wait
    }
    uart_send_str("TX_COMPLETE\r\n");
}
```

**Check 2: Monitor RX Line**
```c
// If display sends response but MCU misses it:
void debug_uart_receive(void) {
    // Enable RX interrupt
    ES0 = 1;  // Enable UART0 interrupt
    
    // OR use polling
    void uart_poll_example(void) {
        if (RI0) {  // RX interrupt flag
            u8 byte = SBUF;
            RI0 = 0;
            
            uart_send_str("RX: ");
            uart_send_hex(byte);
            uart_send_str("\r\n");
        }
    }
}
```

**Check 3: Frame Assembly Issue**
```c
// Maybe frame not being built correctly
void test_read_request(void) {
    u8 frame[8];
    
    // Build manually
    frame[0] = 0x5A;
    frame[1] = 0xA5;
    frame[2] = 0x04;      // Length
    frame[3] = 0x82;      // Read cmd
    frame[4] = 0x00;      // Addr high
    frame[5] = 0x84;      // Addr low
    
    // Send
    uart_send_buffer(frame, 6);
    
    // Wait for response (with debug)
    u8 response[16];
    u16 timeout = 10000;  // 10 seconds in 1ms ticks
    
    while (timeout--) {
        if (uart_has_data()) {
            u8 byte = uart_get_byte();
            uart_send_hex(byte);
            uart_send_str(" ");
            break;
        }
        delay_1ms();
    }
    
    if (timeout == 0) {
        uart_send_str("TIMEOUT\r\n");
    }
}
```

---

## 2. Memory Issues

### 2.1 IRAM Stack Overflow

**Symptom**: Random crashes, system hangs, program counter jumps randomly

#### Detection

```c
// Detect stack overflow at runtime
u8 check_stack_depth(void) {
    u8 sp_current;
    
    // Get current stack pointer
    _asm {
        mov A, SP
        mov SP_current, A
    }
    
    // If SP < 0x30, stack is using register area (BAD)
    if (sp_current < 0x40) {
        uart_send_str("STACK OVERFLOW WARNING: SP=");
        uart_send_hex(sp_current);
        uart_send_str("\r\n");
        return 1;  // Error
    }
    
    // If SP > 0x7F, stack wrapped (CRITICAL)
    if (sp_current > 0x7F) {
        uart_send_str("STACK WRAP DETECTED\r\n");
        return 1;
    }
    
    return 0;
}

// Call from main loop periodically
if (check_stack_depth()) {
    // Emergency shutdown or recovery
    uart_send_str("HALTING\r\n");
    while(1);  // Hang safely
}
```

#### Solutions

**Option 1: Reduce Local Variables**
```c
// BEFORE (bad - uses lots of stack)
void process_data(void) {
    u8 temp_buffer1[128];
    u8 temp_buffer2[128];
    u8 temp_buffer3[128];  // 384 bytes on stack!
    u16 calculations[50];  // Even more!
    
    // Process...
}

// AFTER (good - move to XDATA)
xdata u8 temp_buffer1[128];
xdata u8 temp_buffer2[128];
xdata u8 temp_buffer3[128];

void process_data(void) {
    // Use global buffers instead
    // Minimal stack usage
}
```

**Option 2: Limit Function Nesting**
```c
// BEFORE (deep nesting = deep stack)
void level1(void) { level2(); }
void level2(void) { level3(); }
void level3(void) { level4(); }
void level4(void) { level5(); }
void level5(void) { / *actual work*/ }

// AFTER (flat structure = shallow stack)
void level1(void) {
    level2_action();
    level3_action();
    level4_action();
    level5_action();
}
```

**Option 3: Use State Machine**
```c
// BEFORE (recursive calls)
void handle_frame(void) {
    if (x) {
        handle_subframe_a();  // nested call
    }
}

// AFTER (state machine)
enum { STATE_MAIN, STATE_SUBFRAME_A, STATE_DONE };
static u8 state = STATE_MAIN;

void handle_frame_sm(void) {
    switch (state) {
        case STATE_MAIN:
            if (x) state = STATE_SUBFRAME_A;
            break;
        case STATE_SUBFRAME_A:
            // do work
            state = STATE_DONE;
            break;
        case STATE_DONE:
            state = STATE_MAIN;
            break;
    }
}
```

---

### 2.2 XDATA Buffer Overflow

**Symptom**: UART data corrupted, frames partially lost, random memory overwrites

#### Detection

```c
// Monitor buffer fill level
void monitor_uart_buffers(void) {
    u16 fill_uart2 = uart2_buffer_count();
    u16 fill_uart3 = uart3_buffer_count();
    u16 fill_uart4 = uart4_buffer_count();
    u16 fill_uart5 = uart5_buffer_count();
    
    // Warn if approaching capacity
    if (fill_uart2 > 900) {  // Out of 1024
        uart_send_str("UART2_BUFFER_90%_FULL\r\n");
    }
    
    if (fill_uart2 > 1000) {  // Critical
        uart_send_str("UART2_BUFFER_OVERFLOW\r\n");
        // Take action: drop oldest data or halt
    }
}
```

#### Solutions

**Option 1: Increase Buffer Size**
```c
// In include/config.h
#define UART2_RX_LENTH 2048  // Double from 1024

// Wait for XDATA to have space (total must remain < 32KB)
```

**Option 2: Increase Polling Rate**
```c
// Process frames faster
// Instead of once per 10ms, process every 2ms
void faster_processing(void) {
    // Call DGUS_ProcessAllUarts() more frequently
    // Drains buffer before overflow occurs
}
```

**Option 3: Reduce Baud Rate**
```c
// In include/config.h
// Lower baud = fewer bytes per unit time
// Reduces peak buffer fill rate

#define BAUD_UART2 57600  // Half of 115200
```

---

## 3. Hardware Configuration Issues

### 3.1 Timer Tick Not Firing (1ms interrupts missing)

**Symptom**: delay_ms() doesn't work, delays way shorter than expected

#### Root Causes

| Cause | Indicator | Fix |
|-------|-----------|-----|
| Timer not started (TR0=0) | No tick counter increment | Set TCON bit 4 (TR0=1) |
| Wrong reload value | Tick too fast/slow | See calculation below |
| Interrupt not enabled (ET0=0) | No ISR calls | Set IE bit 1 (ET0=1) |
| Wrong oscillator frequency | Tick off by 2x or more | Verify crystal 11.0592 MHz |

#### Debug Verification

```c
void verify_timer_config(void) {
    u8 tmod, tcon, ie;
    
    // Read SFR values
    tmod = TMOD;  // Should be 0x01 (mode 1 for Timer0)
    tcon = TCON;  // Should have bit 4 set (TR0)
    ie = IE;      // Should have bit 1 set (ET0)
    
    uart_send_str("TMOD=");
    uart_send_hex(tmod);
    uart_send_str(" (expect 0x01)\r\n");
    
    uart_send_str("TCON=");
    uart_send_hex(tcon);
    uart_send_str(" (expect bit 4 set)\r\n");
    
    uart_send_str("IE=");
    uart_send_hex(ie);
    uart_send_str(" (expect bit 1 set)\r\n");
    
    // Check reload value
    uart_send_str("TH0=");
    uart_send_hex(TH0);
    uart_send_str(" TL0=");
    uart_send_hex(TL0);
    uart_send_str("\r\n");
    
    // For 1ms tick at 11.0592 MHz:
    // Need: 11059 CPU cycles per ms
    // Timer counts: 65536 - 11059 = 54477 = 0xFC18
    uart_send_str("Expected TH0=0xFC TL0=0x18 (for 11.0592MHz 1ms tick)\r\n");
}
```

#### Timer Reload Calculation

```c
// Different oscillator frequencies
#define XTAL 11059200  // 11.0592 MHz (standard)
// #define XTAL 12000000  // 12 MHz alternative
// #define XTAL 14745600  // 14.7456 MHz alternative

#define TIMER_1MS_RELOAD (65536 - XTAL / 12000)

// Example: 11.0592 MHz
// 11059200 / 12000 = 921.6 → 922 (rounded)
// Actual reload: 65536 - 922 = 64614 = 0xFC18 ✓
```

#### Diagnostic ISR

```c
// Verify interrupt fires
volatile u16 tick_counter = 0;

void timer0_isr(void) __interrupt(1) {
    tick_counter++;
    TH0 = 0xFC;  // Reload for next ms
    TL0 = 0x18;
}

void verify_tick_running(void) {
    u16 last_count = 0;
    u8 i;
    
    for (i = 0; i < 10; i++) {
        u16 current = tick_counter;
        
        uart_send_str("Tick: ");
        uart_send_dec(current);
        uart_send_str("\r\n");
        
        if (current == last_count) {
            uart_send_str("TIMER NOT RUNNING\r\n");
            return;
        }
        
        last_count = current;
        delay_no_timer(1000);  // Use independent delay
    }
    
    uart_send_str("TIMER OK - Incrementing normally\r\n");
}
```

---

### 3.2 UART Baud Rate Incorrect

**Symptom**: Garbage characters sent/received, frame corruption

#### Verification

```bash
# Method 1: Serial monitor test
$ Send: "U" (ASCII 0x55 = 01010101 alternating)
$ Display: Readable 'U'?
  ├─ Yes → Baud rate OK
  └─ No → Baud rate wrong

# Method 2: Scope measurement
$ Measure bit time between rising edges
$ Bit time = 1 / 115200 = 8.68 microseconds
$ Can be X2 off if using SMOD=1 incorrectly
```

#### Baud Rate Calculation Debug

```c
void show_baud_calculation(void) {
    // User entered settings
    u32 desired_baud = 115200;
    u32 oscillator = 11059200;
    
    // Calculate required TH1
    // Formula: BR = Osc / (12 × 32 × (256-TH1)) × (1+SMOD)
    // Rearrange: TH1 = 256 - Osc / (12 × 32 × BR × (1+SMOD))
    
    u8 smod = 1;  // Enable double baud rate
    u32 divisor = 12 * 32 * desired_baud * (1 + smod);
    u16 th1_value = 256 - (oscillator / divisor);
    
    uart_send_str("Desired Baud: ");
    uart_send_dec(desired_baud);
    uart_send_str("\r\n");
    
    uart_send_str("Oscillator: ");
    uart_send_dec(oscillator);
    uart_send_str("\r\n");
    
    uart_send_str("SMOD: ");
    uart_send_dec(smod);
    uart_send_str("\r\n");
    
    uart_send_str("Calculated TH1: 0x");
    uart_send_hex(th1_value);
    uart_send_str(" (");
    uart_send_dec(th1_value);
    uart_send_str(")\r\n");
    
    // Verify against actual
    uart_send_str("Actual TH1: 0x");
    uart_send_hex(TH1);
    uart_send_str("\r\n");
    
    if ((u8)th1_value != TH1) {
        uart_send_str("ERROR: TH1 MISMATCH!\r\n");
    }
}
```

---

## 4. Display Interface Issues

### 4.1 Display Not Responding to Writes (0x83)

**Symptom**: Writes accepted (no errors) but display doesn't update

#### Diagnosis

```c
// Step 1: Verify write frame structure
void debug_write_operation(u16 addr, u16 value) {
    u8 frame[10];
    
    // Build frame manually to inspect
    frame[0] = 0x5A;
    frame[1] = 0xA5;
    frame[2] = 0x06;                    // Length
    frame[3] = 0x83;                    // Write command
    frame[4] = (u8)(addr >> 8);         // Addr MSB
    frame[5] = (u8)(addr & 0xFF);       // Addr LSB
    frame[6] = (u8)(value >> 8);        // Val MSB
    frame[7] = (u8)(value & 0xFF);      // Val LSB
    
    // Log what we're sending
    uart_send_str("WRITE: ");
    for (u8 i = 0; i < 8; i++) {
        uart_send_hex(frame[i]);
        uart_send_str(" ");
    }
    uart_send_str("| Addr=0x");
    uart_send_hex(addr >> 8);
    uart_send_hex(addr & 0xFF);
    uart_send_str(" Value=0x");
    uart_send_hex(value >> 8);
    uart_send_hex(value & 0xFF);
    uart_send_str("\r\n");
    
    // Send it
    uart_send_buffer(frame, 8);
    
    // Try to verify with read-back
    delay_ms(10);  // Wait for display to process
    u16 readback = DGUS_Read_VP(addr);
    
    if (readback != value) {
        uart_send_str("READBACK_MISMATCH: Expected 0x");
        uart_send_hex(value >> 8);
        uart_send_hex(value & 0xFF);
        uart_send_str(" Got 0x");
        uart_send_hex(readback >> 8);
        uart_send_hex(readback & 0xFF);
        uart_send_str("\r\n");
    }
}
```

#### Solutions

**Issue 1: Wrong VP Address**
```c
// Try writing to known address first
DGUS_Write_VP(0x0084, 1);  // Try switching page

// If page switches, address was valid
// If not, check design file for correct address
```

**Issue 2: Read-Only Address**
```c
// Some VPs accept writes but don't do anything
// E.g., status VPs are read-only

// Solution: Use design tool to verify VP is writable
// Or try different address in same object
```

**Issue 3: Page Mismatch**
```c
// Writing to VP on different active page
// Display: Page 0 active, VP only exists on Page 2

// Solution: Switch to correct page first
DGUS_Write_VP(0x0084, 2);  // Switch to page 2
delay_ms(50);              // Wait for switch
DGUS_Write_VP(target_vp, value);  // Now write
```

---

## 5. Application State Issues

### 5.1 Double Events or Missed Events

**Symptom**: Button press counted twice or sometimes not detected

#### Root Cause Analysis

```c
// Issue: Auto-upload received but processed multiple times

static u16 last_button_count = 0;

void buggy_button_handling(void) {
    // WRONG: Process every time, not just on change
    if (last_vp == VP_BUTTON) {
        process_button_press();  // Called many times!
    }
}

void correct_button_handling(void) {
    u16 current_count = last_value;
    
    // RIGHT: Only process when count increments
    if (current_count != last_button_count) {
        last_button_count = current_count;
        process_button_press();  // Called once only
    }
    
    last_vp = 0;  // Clear for next event
}
```

#### Event Debouncing

```c
void debounced_event_handler(void) {
    static u16 last_event_time = 0;
    static u16 last_event_value = 0;
    
    #define DEBOUNCE_MS 50  // 50ms debounce window
    
    u16 current_time = get_tick_count();
    u16 current_value = DGUS_Read_VP(VP_BUTTON);
    
    // Only process if enough time passed AND value changed
    if (current_value != last_event_value) {
        if ((current_time - last_event_time) > DEBOUNCE_MS) {
            last_event_time = current_time;
            last_event_value = current_value;
            process_button_press();
        }
    }
}
```

---

## 6. Performance Analysis

### 6.1 Main Loop Timing

**Monitor loop execution time:**

```c
void loop_timing_analysis(void) {
    static u32 loop_count = 0;
    static u32 last_tick = 0;
    u32 current_tick;
    
    // Measure every 1000 loops
    if (++loop_count >= 1000) {
        current_tick = get_tick_count();
        u32 time_elapsed = current_tick - last_tick;
        
        uart_send_str("1000 loops in ");
        uart_send_dec(time_elapsed);
        uart_send_str(" ms\r\n");
        
        // Calculate loop frequency
        float loop_rate = 1000000.0 / time_elapsed;
        u16 loop_rate_hz = (u16)loop_rate;
        
        uart_send_str("Loop rate: ~");
        uart_send_dec(loop_rate_hz);
        uart_send_str(" Hz\r\n");
        
        // Expected at 100 cycles/loop:  ~11 kHz
        // < 1 kHz: Loop has blocking calls
        // > 50 kHz: Loop very fast (good for responsiveness)
        
        last_tick = current_tick;
        loop_count = 0;
    }
}
```

### 6.2 UART Processing Load

```c
void uart_load_analysis(void) {
    static u32 frame_count = 0;
    static u32 byte_count = 0;
    static u32 last_tick = 0;
    
    // Sample every 1000ms
    if ((get_tick_count() - last_tick) >= 1000) {
        float bytes_per_frame = frame_count > 0 ? 
            (float)byte_count / frame_count : 0;
        
        uart_send_str("Frames/sec: ");
        uart_send_dec(frame_count);
        uart_send_str(" Bytes/Frame: ");
        uart_send_dec((u16)bytes_per_frame);
        uart_send_str("\r\n");
        
        // Bandwidth usage (at 115200 baud)
        // 115200 bits/sec = 11520 bytes/sec
        // 1000 frames × 8 bytes = 8000 bytes = 69% normal
        
        last_tick = get_tick_count();
        frame_count = 0;
        byte_count = 0;
    }
    
    // Increment counters
    frame_count++;
    byte_count += received_frame_length;
}
```

---

## 7. Systematic Debugging Process

### General Approach

1. **Isolate the Problem**
   - Does it happen with one UART or all?
   - Specific VP address or all addresses?
   - Specific display page or all pages?

2. **Add Logging**
   ```c
   #define DEBUG 1
   
   #if DEBUG
   #define debug_print(x) uart_send_str(x)
   #else
   #define debug_print(x)  // Disabled
   #endif
   
   debug_print("Function start\r\n");
   ```

3. **Simplify Test Case**
   - Reduce baud rate to 9600
   - Use one UART only
   - Write single byte tests
   - Progress incrementally

4. **Measure Everything**
   - Time between events
   - Buffer fill levels
   - Stack depth
   - Loop frequency

5. **Hypothesis Testing**
   - Formulate theory
   - Disable/enable one thing
   - Observe result
   - Accept/reject theory

---

## 8. Advanced Diagnostics

### 8.1 Memory Dump Utility

```c
void memory_dump(u8 *addr, u16 length) {
    u16 i;
    
    uart_send_str("Address: ");
    uart_send_hex((u16)addr >> 8);
    uart_send_hex((u16)addr & 0xFF);
    uart_send_str("\r\n");
    
    for (i = 0; i < length; i++) {
        uart_send_hex(addr[i]);
        uart_send_str(" ");
        
        if ((i + 1) % 16 == 0) {
            uart_send_str("\r\n");
        }
    }
    uart_send_str("\r\n");
}

// Usage:
// xdata u8 buffer[256];
// memory_dump(buffer, 256);
```

### 8.2 CRC Debugging

```c
void debug_crc_frame(u8 *frame, u8 len) {
    u16 calc_crc;
    u16 rx_crc;
    
    uart_send_str("Frame: ");
    for (u8 i = 0; i < len; i++) {
        uart_send_hex(frame[i]);
        uart_send_str(" ");
    }
    uart_send_str("\r\n");
    
    // Extract received CRC
    rx_crc = (frame[len-1] << 8) | frame[len-2];
    
    // Calculate CRC over payload
    calc_crc = crc16_modbus(&frame[3], len - 5);
    
    uart_send_str("RX CRC: 0x");
    uart_send_hex(rx_crc >> 8);
    uart_send_hex(rx_crc & 0xFF);
    uart_send_str("\r\n");
    
    uart_send_str("Calc CRC: 0x");
    uart_send_hex(calc_crc >> 8);
    uart_send_hex(calc_crc & 0xFF);
    uart_send_str("\r\n");
    
    if (rx_crc == calc_crc) {
        uart_send_str("CRC: OK\r\n");
    } else {
        uart_send_str("CRC: ERROR!\r\n");
    }
}
```

---

## Related Documentation

| Document | Purpose |
|----------|---------|
| [PROTOCOL_EXAMPLES.md](PROTOCOL_EXAMPLES.md) | Working code examples |
| [DGUS_PROTOCOL_GUIDE.md](DGUS_PROTOCOL_GUIDE.md) | Protocol specification |
| [MEMORY_MANAGEMENT.md](MEMORY_MANAGEMENT.md) | Memory issues root cause |
| [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md) | Hardware configuration |

---

**Document Version**: 1.0  
**Last Updated**: May 7, 2026  
**Status**: Complete - Ready for Phase 2 (v0.3.0)
