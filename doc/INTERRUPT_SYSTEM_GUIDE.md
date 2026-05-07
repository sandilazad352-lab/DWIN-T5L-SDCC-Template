# Interrupt System Architecture Guide

**Complete interrupt management system for T5L HMI controllers**

---

## Overview

The T5L uses the standard 8051 interrupt architecture with enhancements:
- **8 prioritized interrupt sources** (some re-mapped for T5L)
- **Two priority levels** (high/low)
- **Programmable interrupt enable and priority**

This guide covers interrupt configuration, ISR best practices, and real-world patterns.

---

## 1. 8051 Interrupt Architecture

### 1.1 Interrupt Vector Table

The 8051 has fixed interrupt locations in memory:

| Vector | Address | Priority | Source | Notes |
|--------|---------|----------|--------|-------|
| INT0 | 0x0003 | Level 0 | External interrupt 0 | P3.2 pin |
| Timer0 | 0x000B | Level 1 | Timer0 overflow | System tick |
| INT1 | 0x0013 | Level 2 | External interrupt 1 | P3.3 pin |
| Timer1 | 0x001B | Level 3 | Timer1 overflow | Baud rate (auto) |
| UART | 0x0023 | Level 4 | UART0 Tx/Rx | Multi-source |
| Timer2 | 0x002B | Level 5 | Timer2 overflow | Application timer |
| Reserved | 0x0033 | Level 6 | - | T5L: UART2 ISR |
| Reserved | 0x003B | Level 7 | - | T5L: UART3 ISR |

**Hardware Priority (Fixed)**:
1. INT0 (highest)
2. Timer0
3. INT1
4. Timer1
5. UART
6. Timer2
7. UART2 (T5L)
8. UART3 (T5L) (lowest)

### 1.2 Interrupt Enable Register (IE)

```c
// IE register bits:
// IE.7 = EA  (Enable All interrupts)
// IE.6 = -   (Unused)
// IE.5 = ET2 (Enable Timer2)
// IE.4 = ES  (Enable UART)
// IE.3 = ET1 (Enable Timer1)
// IE.2 = EX1 (Enable INT1)
// IE.1 = ET0 (Enable Timer0)
// IE.0 = EX0 (Enable INT0)

#define EA_BIT   0x80  // Enable All
#define ET2_BIT  0x20  // Enable Timer2
#define ES_BIT   0x10  // Enable UART
#define ET1_BIT  0x08  // Enable Timer1
#define EX1_BIT  0x04  // Enable INT1
#define ET0_BIT  0x02  // Enable Timer0
#define EX0_BIT  0x01  // Enable INT0

// Set up typical usage: Timer0 and UART enabled
void interrupts_enable_all(void) {
    IE = 0x92;  // EA=1, ET2=0, ES=1, ET1=0, EX1=0, ET0=1, EX0=0
}

// Enable specific interrupt
void enable_interrupt(u8 ie_mask) {
    IE |= ie_mask;
}

// Disable specific interrupt
void disable_interrupt(u8 ie_mask) {
    IE &= ~ie_mask;
}

// Atomically disable all
void disable_all_interrupts(void) {
    EA = 0;
}

// Atomically enable all
void enable_all_interrupts(void) {
    EA = 1;
}
```

### 1.3 Interrupt Priority Register (IP)

```c
// IP register bits (only set if need to change default priority):
// IP.7 = -   (Unused)
// IP.6 = -   (Unused)
// IP.5 = PT2 (Priority Timer2)
// IP.4 = PS  (Priority UART)
// IP.3 = PT1 (Priority Timer1)
// IP.2 = PX1 (Priority INT1)
// IP.1 = PT0 (Priority Timer0)
// IP.0 = PX0 (Priority INT0)

// Setting bit = 1: High priority
// Clearing bit = 0: Low priority

#define PT2_HIGH  0x20
#define PS_HIGH   0x10  // Timer1 (UART)
#define PT1_HIGH  0x08
#define PX1_HIGH  0x04
#define PT0_HIGH  0x02
#define PX0_HIGH  0x01

// Example: Make Timer0 high priority
void set_timer0_high_priority(void) {
    IP |= PT0_HIGH;
}

// Make Timer2 low priority (default)
void set_timer2_low_priority(void) {
    IP &= ~PT2_HIGH;
}

// Typical configuration: Timer0 high, Timer2 low
void interrupts_setup_priority(void) {
    IP = 0x00;  // All low priority by default
    IP |= PT0_HIGH;  // Except Timer0 = high priority
}
```

---

## 2. ISR Context Switching

### 2.1 Register Preservation

When an interrupt occurs, the CPU **automatically**:
1. Finishes current instruction
2. Saves PC to stack
3. Loads PC from interrupt vector
4. **Does NOT automatically save register banks**

```c
// When ISR is called, registers R0-R7 NOT automatically saved
// Must save if using same register bank as interrupted code

// REGISTER BANK SELECTION IN ISR:
// If interrupted code uses Bank 0, ISR should use different bank

void timer0_isr(void) __interrupt(1) __using(1) {
    // __using(1) = Use Register Bank 1 for this ISR
    // R0-R7 in ISR = Bank 1, not Bank 0
    // Original Bank 0 preserved!
    
    // ISR code here - can freely use R0-R7
    TH0 = 0xFC;
    TL0 = 0x18;
    system_ticks++;
}

// MANUAL REGISTER SAVE (if must use same bank):
void uart_isr_manual_save(void) __interrupt(4) {
    // Save registers used
    __asm 
        PUSH ACC
        PUSH B
        PUSH DPH
        PUSH DPL
    __endasm;
    
    // ISR code here
    uart_process();
    
    // Restore registers
    __asm
        POP DPL
        POP DPH
        POP B
        POP ACC
    __endasm;
}
```

### 2.2 Stack Considerations

The stack grows downward from IRAM top (SP initially 0x07):

```
INITIAL STATE:
   Offset:  Register Bank Area:
   0x00-0x07   Bank 0
   0x08-0x0F   Bank 1
   0x10-0x17   Bank 2
   0x18-0x1F   Bank 3
   0x20-0x2F   Bit-addressable RAM
   0x30-0x7F   General purpose
   0x80-0xFF   SFR area

STACK: Grows downward from 0x7F (typically set to 0x60 initially)
```

**Stack Depth Calculation**:
```c
// Monitor stack usage
u8 get_stack_depth(void) {
    // Rough estimate: 0x80 - SP = used stack bytes
    return (0x80 - SP);
}

// Check for stack overflow
void check_stack_overflow(void) {
    if (SP < 0x30) {
        // Stack near bit-addressable area!
        // Risk of corruption
        uart_send_str("STACK OVERFLOW\r\n");
    }
}
```

---

## 3. ISR Patterns and Best Practices

### 3.1 Minimal Fast ISR

```c
// Timer0: Highest priority interrupt - keep VERY small
volatile u32 tick_count = 0;

void timer0_isr(void) __interrupt(1) __using(1) {
    // Reload timer (must be first!)
    TH0 = 0xFC;
    TL0 = 0x18;
    
    // Single increment
    tick_count++;
    
    // Set flag for main loop
    static bit timer_fired = 0;
    timer_fired = 1;
}
// ISR Duration: ~2-4 μs
```

### 3.2 Multi-source ISR (UART)

```c
// UART can interrupt on either Tx or Rx
void uart_isr(void) __interrupt(4) __using(2) {
    u8 sbuf;
    
    // Check interrupt source
    if (RI) {
        // Reception complete
        RI = 0;  // Clear flag
        sbuf = SBUF;
        
        // Add to receive buffer
        uart_rx_buffer[uart_rx_tail++] = sbuf;
        if (uart_rx_tail >= RX_BUFFER_SIZE) {
            uart_rx_tail = 0;
        }
    }
    
    if (TI) {
        // Transmission complete
        TI = 0;  // Clear flag
        
        // Load next byte if available
        if (uart_tx_head != uart_tx_tail) {
            SBUF = uart_tx_buffer[uart_tx_head++];
            if (uart_tx_head >= TX_BUFFER_SIZE) {
                uart_tx_head = 0;
            }
        }
    }
}
```

### 3.3 External Interrupt Handler

```c
// INT0 = External interrupt (e.g., button press on P3.2)
volatile u8 button_pressed = 0;

void int0_isr(void) __interrupt(0) __using(1) {
    // Debounce delay
    {
        u8 i = 50;  // ~50 CPU cycles = ~5μs
        while (i--) ;
    }
    
    // Check if still pressed
    if (P3_2 == 0) {
        button_pressed = 1;
    }
    
    // Clear interrupt flag (low-to-high transition triggers next)
}
```

---

## 4. Interrupt Priority Management

### 4.1 Priority Conflict Resolution

When both high and low priority interrupts pending:

```
TIME:  0    10    20    30    40    50    60
ISR:  INT0  INT0  INT0  INT1  INT1  INT0  INT0
      High  High  High  (Wait) Low  High  High
                        (Int0 done, now Int1 runs)
```

**Problem**: Lower priority interrupt blocked while higher still pending

**Solution 1: Increase ISR Priority**
```c
void int1_isr(void) __interrupt(2) __using(2) {
    // Make INT1 high priority if it's time-critical
    IP |= 0x04;  // PX1_HIGH
    
    // Now INT1 can interrupt other low-priority ISRs
    // ... interrupt code ...
}
```

**Solution 2: Reduce Main Interrupt Latency**
```c
// Keep ISRs small - most work in main loop
void timer0_isr(void) __interrupt(1) __using(1) {
    TH0 = 0xFC;
    TL0 = 0x18;
    system_ticks++;
    
    // Don't do heavy processing here!
}

// Heavy work done in main loop
while (1) {
    if (system_event_flag) {
        system_event_flag = 0;
        heavy_processing();  // Done outside ISR
    }
}
```

### 4.2 Nested Interrupt Handling

```c
// High-priority interrupt CAN interrupt low-priority ISR
// But low-priority CANNOT interrupt high-priority

volatile u8 in_high_priority_isr = 0;

void timer0_isr_high(void) __interrupt(1) __using(1) {
    // This is high priority
    in_high_priority_isr = 1;
    
    TH0 = 0xFC;
    TL0 = 0x18;
    system_ticks++;
    
    in_high_priority_isr = 0;
}

void uart_isr_low(void) __interrupt(4) __using(2) {
    // This is low priority
    // Can be interrupted by Timer0 (high priority)
    
    if (RI) {
        RI = 0;
        uart_process();
        // If Timer0 fires here, it interrupts this code
        // But Timer0 ISR is very short, so acceptable
    }
}
```

---

## 5. Critical Sections

### 5.1 Atomic Operations

Variables accessed from both main and ISR need protection:

```c
// NON-ATOMIC: Multiple instructions
volatile u16 counter = 0;

void increment_counter(void) {
    counter++;  // NOT atomic:
                // load counter into ACC
                // increment ACC
                // store ACC to counter
}

void timer_isr(void) __interrupt(1) __using(1) {
    // Could interrupt main's increment
    // Result: May lose one count!
}

// ATOMIC: Disable interrupts
void increment_counter_safe(void) {
    EA = 0;       // Disable interrupts
    counter++;    // Now atomic
    EA = 1;       // Re-enable
}

// Or use smaller unit:
volatile u8 counter_low = 0, counter_high = 0;

void increment_counter_atomic(void) {
    if (++counter_low == 0) {
        counter_high++;  // Each line atomic
    }
}
```

### 5.2 Critical Section Pattern

```c
// Define critical section macro
#define ENTER_CRITICAL() { u8 _ea = (IE & 0x80); EA = 0;
#define EXIT_CRITICAL()  if (_ea) EA = 1; }

// Usage
{
    ENTER_CRITICAL();
    
    // Code here with interrupts disabled
    global_state.counter++;
    global_state.flag = 1;
    
    EXIT_CRITICAL();
}

// Saves/restores previous state
```

---

## 6. Practical DGUS Interrupt Patterns

### 6.1 Multi-UART Reception (UART2-5)

```c
// DGUS can use multiple UARTs on T5L variant
// Each needs interrupt handler

// UART2 ISR
void uart2_isr(void) __interrupt(6) __using(1) {
    // Check interrupt source
    if (S2CON & 0x01) {  // RI bit
        S2CON &= ~0x01;
        u8 byte = S2BUF;
        dgus_rx_queue2[tail2++] = byte;
    }
}

// UART3 ISR
void uart3_isr(void) __interrupt(7) __using(1) {
    if (S3CON & 0x01) {  // RI bit
        S3CON &= ~0x01;
        u8 byte = S3BUF;
        dgus_rx_queue3[tail3++] = byte;
    }
}

// Process all UARTs in main loop
void process_dgus_all(void) {
    process_dgus_uart1();
    process_dgus_uart2();
    process_dgus_uart3();
    process_dgus_uart4();
}
```

### 6.2 DGUS Response Handling with Interrupt Synchronization

```c
// Flag set by ISR
volatile u8 dgus_response_ready = 0;
volatile u16 dgus_response_ticks = 0;

void uart_isr(void) __interrupt(4) __using(2) {
    if (RI) {
        RI = 0;
        dgus_parse_byte(SBUF);
        
        // Check if complete frame received
        if (dgus_frame_complete) {
            dgus_response_ready = 1;
            dgus_response_ticks = system_ticks;
        }
    }
}

// Main loop waits for response
int dgus_read_vp_wait(u16 vp, u16 *value) {
    u32 timeout_ticks = system_ticks + 100;  // 100ms timeout
    
    // Send read command
    dgus_response_ready = 0;
    dgus_send_read_vp(vp);
    
    // Wait for response
    while (!dgus_response_ready) {
        if (system_ticks > timeout_ticks) {
            return -1;  // Timeout
        }
    }
    
    // Parse response
    *value = dgus_response_value;
    dgus_response_ready = 0;
    return 0;
}
```

---

## 7. Debugging Interrupt Issues

### 7.1 ISR Not Firing

```c
void debug_interrupt_not_firing(u8 interrupt_type) {
    // Check IE register
    u8 ie = IE;
    uart_send_str("IE=");
    uart_send_hex(ie);
    uart_send_str(" EA=");
    uart_send_dec((ie >> 7) & 1);
    uart_send_str("\r\n");
    
    if (!(ie & 0x80)) {
        uart_send_str("GLOBAL INTERRUPTS DISABLED (EA=0)\r\n");
        return;
    }
    
    // Check specific interrupt
    switch (interrupt_type) {
        case INT_TIMER0:
            uart_send_str("Timer0: ");
            uart_send_dec((ie >> 1) & 1);
            uart_send_str(" TR0=");
            uart_send_dec(TR0);
            uart_send_str(" TMOD=");
            uart_send_hex(TMOD);
            uart_send_str("\r\n");
            break;
            
        case INT_UART:
            uart_send_str("UART: ");
            uart_send_dec((ie >> 4) & 1);
            uart_send_str(" TI=");
            uart_send_dec(TI);
            uart_send_str(" RI=");
            uart_send_dec(RI);
            uart_send_str("\r\n");
            break;
    }
}
```

### 7.2 ISR Latency Measurement

```c
// Measure time from interrupt source to ISR execution
volatile u8 trigger_pin = 0;
volatile u32 isr_latency = 0;

void timer0_isr(void) __interrupt(1) __using(1) {
    TH0 = 0xFC;
    TL0 = 0x18;
    
    // Record counter value
    u16 timer_val = (TH0 << 8) | TL0;
    
    // Latency = initial_reload - current_value
    // Shorter ISR latency = better response time
}

// Alternative: Toggle pin, measure with frequency counter
void int0_isr(void) __interrupt(0) __using(1) {
    // Trigger occurred
    P3_0 = 1;  // Toggle output pin
    P3_0 = 0;
    
    // Time from P3.2 edge to P3.0 edge = ISR latency
}
```

### 7.3 Interrupt Priority Conflicts

```c
// Log which ISRs executed
volatile u8 last_isr = 0;

#define ISR_TIMER0  1
#define ISR_UART    4
#define ISR_TIMER2  5

void timer0_isr(void) __interrupt(1) __using(1) {
    TH0 = 0xFC;
    TL0 = 0x18;
    
    uart_send_char('T');  // Mark Timer0
    last_isr = ISR_TIMER0;
}

void uart_isr(void) __interrupt(4) __using(2) {
    uart_send_char('U');  // Mark UART
    last_isr = ISR_UART;
    
    // ... process ...
}

void timer2_isr(void) __interrupt(5) __using(1) {
    uart_send_char('2');  // Mark Timer2
    last_isr = ISR_TIMER2;
}

// Output pattern shows execution order:
// T U T U T ...  = Expected
// T 2 T U ...    = Timer2 incorrectly fired before UART
//                  (priority issue)
```

---

## 8. Interrupt Optimization

### 8.1 Reduce ISR Overhead

```c
// SLOW: Complex logic in ISR
void uart_isr_slow(void) __interrupt(4) {
    if (RI) {
        RI = 0;
        u8 b = SBUF;
        
        // Parse frame
        if (dgus_parse_byte(b) && dgus_frame_complete()) {
            // Process entire DGUS frame
            dgus_handle_response();  // <- Complex!
        }
    }
}

// FAST: Minimal in ISR
volatile u8 dgus_byte_received = 0;

void uart_isr_fast(void) __interrupt(4) __using(2) {
    if (RI) {
        RI = 0;
        dgus_byte_received = SBUF;
        
        // Just store, don't process
    }
}

// Main loop handles parsing
void main(void) {
    while (1) {
        if (dgus_byte_received) {
            u8 b = dgus_byte_received;
            dgus_byte_received = 0;
            
            if (dgus_parse_byte(b) && dgus_frame_complete()) {
                dgus_handle_response();
            }
        }
    }
}
```

### 8.2 Interrupt-Free Sections

```c
// For timing-critical non-interrupt code
bit interrupts_enabled = 0;

void critical_computation(void) {
    // Disable interrupts temporarily
    u8 ie_save = IE;
    EA = 0;
    
    // Fast computation without ISR interference
    for (u16 i = 0; i < 1000; i++) {
        compute_value(i);
    }
    
    // Restore interrupts
    IE = ie_save;
}

// Alternative: Mark as critical section
#pragma SRC
// Compiler-specific "no interrupt allowed" directive
```

---

## 9. ISR Development Checklist

- [ ] ISR uses dedicated register bank (__using)
- [ ] ISR completes in <100 μs
- [ ] All ISR-modified globals are declared volatile
- [ ] Critical sections protected with EA disable/enable
- [ ] Stack depth monitored during testing
- [ ] ISR tested with all other interrupts active
- [ ] Interrupt frequency verified
- [ ] Priority ordering logical for application
- [ ] Watchdog timer considered (may need kick in ISR)
- [ ] ISR performance measured and logged

---

## Related Documentation

| Document | Purpose |
|----------|---------|
| [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md) | IE/IP register details, SFR definitions |
| [TIMER_CLOCK_GUIDE.md](TIMER_CLOCK_GUIDE.md) | Timer interrupt setup |
| [DEBUGGING_GUIDE.md](DEBUGGING_GUIDE.md) | Interrupt-related debugging |
| [PROTOCOL_EXAMPLES.md](PROTOCOL_EXAMPLES.md) | UART ISR patterns |

---

## Key References

- **T5L ASIC Datasheet**: Section 3 - Interrupt System
- **8051 Architecture**: Standard interrupt vector table
- See [include/t5l1.h](../include/t5l1.h) for SFR definitions
- See [startup/startup_T5L.s](../startup/startup_T5L.s) for vector initialization

---

**Document Version**: 1.0  
**Last Updated**: May 7, 2026  
**Status**: Complete - Ready for Phase 2 (v0.3.0)
