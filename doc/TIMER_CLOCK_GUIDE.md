# Timer and Clock Configuration Guide

**Complete timer system setup and optimization for T5L HMI controllers**

---

## Overview

The T5L provides multiple timer modules (Timer0-3) for various applications:
- **Timer0-1**: 16-bit counters (standard 8051)
- **Timer2-3**: 16-bit with additional capture/compare modes (T5L enhancement)

This guide covers practical timer configuration for common scenarios.

---

## 1. Oscillator Configuration

### 1.1 Crystal Selection

**Standard T5L Setup**:
- **Oscillator**: 11.0592 MHz crystal
- **Load Capacitors**: 22pF (typical)
- **Frequency Tolerance**: ±10 ppm (tight for baud rates)

**Why 11.0592 MHz?**
```
Divisible by standard baud rates:
  11059200 / (12 × 32 × 3) = 9600 bps (no error)
  11059200 / (12 × 16 × 6) = 9600 bps (alternative)
  11059200 / (12 × 32 × 1) = 28800 bps
  11059200 / (12 × 16 × 1) = 57600 bps
  11059200 / (12 × 32 × 0.5) = 115200 bps (with SMOD=1)

Most frequencies create rounding errors
```

### 1.2 Oscillator Startup Sequence

```
┌─────────────────────────────────────────────┐
│  1. Power supply voltage rises to rated VCC │
│     Time: ~10-50ms with proper decoupling   │
└──────────────┬──────────────────────────────┘
               │
┌──────────────v──────────────────────────────┐
│  2. Crystal oscillator begins oscillation    │
│     Frequency: 11.0592 MHz (unstable)       │
│     Stabilization: ~100ms                   │
└──────────────┬──────────────────────────────┘
               │
┌──────────────v──────────────────────────────┐
│  3. Power-On Reset (POR) circuit triggers   │
│     RST pin held high ~100ms                │
│     All SFRs reset to default               │
│     PC set to 0x0000                        │
└──────────────┬──────────────────────────────┘
               │
┌──────────────v──────────────────────────────┐
│  4. Startup code executes                   │
│     Set up SP, configure timers             │
│     Jump to main()                          │
└─────────────────────────────────────────────┘
```

**Circuit Decoupling** (critical for stability):
```
VCC
 │
 ├─ 100nF (ceramic) ← High frequency noise
 ├─ 10μF (electrolytic) ← Low frequency and bulk
 ├─ 1nF (ceramic) ← Very high frequency EMI filter
 │
GND
```

---

## 2. Timer0 - System Tick Generation

### 2.1 Configuration for 1ms Tick

**Goal**: Generate interrupt every exactly 1 millisecond

#### Setup Code

```c
void timer0_init_1ms(void) {
    // Mode 1: 16-bit timer (count 0x0000-0xFFFF)
    TMOD = (TMOD & 0xF0) | 0x01;  // Keep Timer1 bits, set Timer0 mode 1
    
    // Enable Timer0 interrupt
    ET0 = 1;
    
    // Start Timer0
    TR0 = 1;
    
    // Set reload value for 1ms tick
    // At 11.0592 MHz: Need 11,059 CPU cycles per tick
    // Prescaler is /12: 11059 / 12 ≈ 921 timer ticks
    // Reload: 65536 - 921 = 64,615 = 0xFC18
    
    TH0 = 0xFC;
    TL0 = 0x18;
    
    uart_send_str("Timer0 configured: 1ms tick\r\n");
}

// ISR for Timer0 overflow
void timer0_isr(void) __interrupt(1) {
    // Reload counter
    TH0 = 0xFC;
    TL0 = 0x18;
    
    // Increment system tick counter
    system_ticks++;
    
    // Optional: Watchdog kick
    // kick_watchdog();
}

// Global tick counter
volatile u32 system_ticks = 0;

// Get elapsed time
u32 get_ticks(void) {
    u32 ticks;
    EA = 0;  // Disable interrupts (atomic read)
    ticks = system_ticks;
    EA = 1;
    return ticks;
}
```

#### Timer Reload Calculation (Reference)

For different oscillators:

```c
#define CALC_TH0_TL0(XTAL_HZ, MS) \
    (u16)(65536 - XTAL_HZ / (12000 * MS))

// Examples:
// 11.0592 MHz, 1ms:   0xFC18
// 11.0592 MHz, 10ms:  0xF84F
// 12 MHz, 1ms:        0xFC37
// 14.7456 MHz, 1ms:   0xFB71
```

**Accuracy vs. Crystal**:

| Crystal | 1ms Reload | Error | Usable For |
|---------|-----------|-------|------------|
| 11.0592 MHz | 0xFC18 | 0% | All applications |
| 12.0 MHz | 0xFC37 | 0.0088% | UART may drift |
| 14.7456 MHz | 0xFB71 | 0% | High-speed option |

---

### 2.2 Delay Functions Using Timer0

```c
// Software delay using timer interrupts
void delay_ms(u16 ms) {
    u32 target = system_ticks + ms;
    
    // Wait for timer to reach target
    while (system_ticks < target) {
        // Give up processor time
        // In production, could do other work here
    }
}

// Alternative: Busy-wait (if interrupts disabled)
void delay_ms_busy(u16 ms) {
    u32 cpu_cycles_needed = (u32)ms * 11059;  // cycles per ms
    
    // This is approximate - actual depends on code
    while (cpu_cycles_needed--) {
        ;  // Busy loop
    }
}

// Get current time safely
u32 millis(void) {
    u32 m;
    EA = 0;  // Atomic read
    m = system_ticks;
    EA = 1;
    return m;
}

// Elapsed time since reference point
u32 elapsed_since(u32 reference) {
    return millis() - reference;
}

// Wait with timeout
int wait_for_event(u32 timeout_ms) {
    u32 start = millis();
    
    while (1) {
        if (event_occurred) {
            return 1;  // Event happened
        }
        
        if (elapsed_since(start) > timeout_ms) {
            return 0;  // Timeout
        }
    }
}
```

---

## 3. Timer1 - UART Baud Rate Generation

### 3.1 Baud Rate Generator Setup

Timer1 generates the bit-clock for UART:

```c
void timer1_init_baud(u32 baud_rate) {
    // Mode 2: 8-bit auto-reload counter
    TMOD = (TMOD & 0x0F) | 0x20;  // Keep Timer0, set Timer1 mode 2
    
    // Calculate TH1 reload value
    // Formula: BR = Osc / (12 × 32 × (256 - TH1)) × (1 + SMOD)
    // Example at 115200: 0xFF for TH1
    
    TH1 = 0xFF;  // For 115200 with SMOD=1 at 11.0592 MHz
    TL1 = 0xFF;  // Auto-reloads from TH1
    
    // Enable SMOD for double baud rate (if using 115200)
    PCON |= 0x80;  // Set SMOD bit
    
    // Start Timer1
    TR1 = 1;
}

// Standard baud rates at 11.0592 MHz
const u8 BAUD_TABLE[] = {
    0xFD,  // 1200
    0xFE,  // 2400
    0xFE,  // 4800 (SMOD=1)
    0xFF,  // 9600 (SMOD=1)
    0xFF,  // 19200
    0xFF,  // 38400 (difficult)
    0xFF,  // 57600 (SMOD=1)
    0xFF,  // 115200 (SMOD=1)
};
```

**Note**: Timer1 configuration is typically handled by the UART driver. See [lib/uart/uart.c](../lib/uart/uart.c) for current implementation.

---

## 4. Timer2 - Advanced Features

### 4.1 Capture/Compare Mode (T5L Enhancement)

Timer2 supports:
- Input capture (measure pulse width)
- Output compare (generate pulse)
- 16-bit reload capability

```c
// Timer2 input capture example
void timer2_capture_init(void) {
    // Configure for capture mode
    T2MOD = 0x01;  // Capture mode enabled
    
    // Interrupt on capture event
    ET2 = 1;       // Enable Timer2 interrupt
    
    // Start Timer2
    TR2 = 1;
}

volatile u16 capture_value = 0;
volatile u8 capture_occurred = 0;

void timer2_isr(void) __interrupt(5) {
    // Read captured value
    capture_value = (TH2 << 8) | TL2;
    capture_occurred = 1;
    
    // Could measure pulse width here
    // Compare with previous capture for timing info
}

// Usage: Measure pulse width
void measure_pulse_width(void) {
    u16 pulse_width;
    
    // Wait for capture
    while (!capture_occurred) ;
    
    pulse_width = capture_value;
    capture_occurred = 0;
    
    uart_send_str("Capture: ");
    uart_send_dec(pulse_width);
    uart_send_str(" ticks\r\n");
}
```

### 4.2 Reload Mode (Continuous Counting)

```c
void timer2_reload_mode(void) {
    // 16-bit auto-reload mode
    RCAP2H = 0xFC;
    RCAP2L = 0x18;  // Reload value for 1ms (alternative to Timer0)
    
    // Start Timer2
    TR2 = 1;
    ET2 = 1;  // Interrupt enable
}

void timer2_isr_reload(void) __interrupt(5) {
    // Counter automatically reloads from RCAP2H/RCAP2L
    // Just increment a counter here
    timer2_ticks++;
}
```

---

## 5. Timer3 - Independent Timing

Timer3 provides an independent timing source:

```c
// Timer3 for separate application timing
void timer3_init_10ms(void) {
    // Configure similar to Timer0 but for different interval
    // 10ms reload value: 65536 - (11059 × 10 / 12) ≈ 0xF84F
    
    T3H = 0xF8;
    T3L = 0x4F;
    TR3 = 1;
    ET3 = 1;
}

void timer3_isr(void) __interrupt(?) __using(1) {
    T3H = 0xF8;
    T3L = 0x4F;
    
    // 10ms tick for separate processing
    app_ticks++;
}
```

---

## 6. Practical Timer Applications

### 6.1 Multi-Rate Processing

```c
void scheduler_1ms_tick(void) {
    static u8 div_10 = 0, div_100 = 0;
    
    // Always at 1ms (from Timer0 ISR)
    fast_processing_1ms();
    
    // Every 10ms
    if (++div_10 >= 10) {
        div_10 = 0;
        medium_processing_10ms();
    }
    
    // Every 100ms
    if (++div_100 >= 100) {
        div_100 = 0;
        slow_processing_100ms();
    }
}

void fast_processing_1ms(void) {
    // Real-time critical:
    // - UART frame processing
    // - VP update checks
    // - System watchdog
}

void medium_processing_10ms(void) {
    // Moderately time-sensitive:
    // - Sensor sampling
    // - State machine updates
}

void slow_processing_100ms(void) {
    // Low priority:
    // - Display updates
    // - Diagnostics
    // - Error logging
}
```

### 6.2 PWM Generation Using Timer Overflow

```c
// Software PWM using Timer0 interrupts
volatile u8 pwm_duty = 50;  // 0-100%
volatile u8 pwm_step = 0;

void timer0_isr(void) __interrupt(1) {
    TH0 = 0xFC;
    TL0 = 0x18;
    
    // Simple PWM: PWM high for duty% of period
    if (pwm_step < pwm_duty) {
        PWM_PIN = 1;  // Output high
    } else {
        PWM_PIN = 0;  // Output low
    }
    
    // Increment PWM step (0-100 per cycle)
    if (++pwm_step >= 100) {
        pwm_step = 0;
    }
}

// Usage
void set_pwm(u8 duty_percent) {
    pwm_duty = duty_percent;
}
```

### 6.3 Watchdog Timeout

```c
#define WATCHDOG_TIMEOUT_MS 1000

volatile u32 watchdog_timer = 0;

void watchdog_kick(void) {
    // Reset watchdog timer
    watchdog_timer = system_ticks;
}

// Call periodically from main loop
void check_watchdog(void) {
    if ((system_ticks - watchdog_timer) > WATCHDOG_TIMEOUT_MS) {
        // Watchdog timeout!
        uart_send_str("WATCHDOG TIMEOUT\r\n");
        
        // Recovery actions:
        // - Log error
        // - Reset display
        // - Reset self (via hardware WDT if available)
    }
}

int main(void) {
    timer0_init_1ms();
    Uart_Init();
    
    watchdog_kick();  // Start watchdog
    
    while (1) {
        // Main application
        process_events();
        
        // Periodic watchdog kick
        static u8 kick_timer = 0;
        if (++kick_timer >= 100) {
            kick_timer = 0;
            watchdog_kick();
        }
        
        check_watchdog();
    }
}
```

---

## 7. Clock Configuration Troubleshooting

### 7.1 Timer Not Firing

```c
// Diagnostic: Check if Timer0 is running
void diagnose_timer0(void) {
    u8 th, tl;
    u16 ticks1, ticks2;
    
    // Sample TH0/TL0
    th = TH0;
    tl = TL0;
    
    uart_send_str("TH0=");
    uart_send_hex(th);
    uart_send_str(" TL0=");
    uart_send_hex(tl);
    uart_send_str("\r\n");
    
    delay_ms_busy(10);  // Busy delay
    
    // Sample again
    th = TH0;
    tl = TL0;
    
    uart_send_str("TH0=");
    uart_send_hex(th);
    uart_send_str(" TL0=");
    uart_send_hex(tl);
    uart_send_str("\r\n");
    
    // If same values: Timer0 not running
    // If different: Timer0 is counting
    
    // Check tick counter
    ticks1 = system_ticks;
    delay_ms_busy(100);
    ticks2 = system_ticks;
    
    if (ticks2 == ticks1) {
        uart_send_str("TIMER0 ISR NOT FIRING\r\n");
    } else {
        uart_send_str("Timer ticks: ");
        uart_send_dec(ticks2 - ticks1);
        uart_send_str("\r\n");
    }
}
```

### 7.2 Frequency Too Fast or Too Slow

```c
// Measure actual timer frequency
void frequency_calibration(void) {
    u32 tick_start, tick_end;
    u16 expected_ticks = 100;  // Expect 100ms
    
    tick_start = system_ticks;
    delay_ms_busy(100);  // Busy wait 100ms (reference)
    tick_end = system_ticks;
    
    u32 actual_ticks = tick_end - tick_start;
    
    uart_send_str("Expected: ");
    uart_send_dec(expected_ticks);
    uart_send_str(" Actual: ");
    uart_send_dec(actual_ticks);
    uart_send_str("\r\n");
    
    if (actual_ticks < expected_ticks) {
        uart_send_str("TIMER TOO FAST - Check TH0/TL0 reload\r\n");
        uart_send_str("May need to adjust for crystal tolerance\r\n");
    } else if (actual_ticks > expected_ticks) {
        uart_send_str("TIMER TOO SLOW - Crystal may be slow\r\n");
    }
    
    // Calculate actual frequency
    float actual_hz = actual_ticks * 10;  // 100ms → 10 per second
    uart_send_str("Actual tick freq: ");
    uart_send_dec((u16)actual_hz);
    uart_send_str(" Hz\r\n");
}
```

---

## 8. Optimization Tips

### 8.1 Reduce ISR Overhead

```c
// SLOW: Lots of code in ISR
void timer0_isr_slow(void) __interrupt(1) {
    // Reload
    TH0 = 0xFC;
    TL0 = 0x18;
    
    // Long processing
    system_ticks++;
    check_timers();
    process_events();
    update_displays();
    // ... more code ...
}

// FAST: Minimal ISR
void timer0_isr_fast(void) __interrupt(1) __using(1) {
    TH0 = 0xFC;
    TL0 = 0x18;
    system_ticks++;
    
    // Set flag for main loop
    timer_event = 1;
}

// Main loop handles heavy work
void main(void) {
    while (1) {
        if (timer_event) {
            timer_event = 0;
            check_timers();
            process_events();
            update_displays();
        }
    }
}
```

### 8.2 Use Register Bank for ISR

```c
// Switch to different register bank in ISR
void timer0_isr(void) __interrupt(1) __using(1) {
    // Uses Register Bank 1 (R0-R7)
    // Doesn't save/restore R0-R7 from main
    
    TH0 = 0xFC;
    TL0 = 0x18;
    system_ticks++;
}
```

---

## Related Documentation

| Document | Purpose |
|----------|---------|
| [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md) | SFR registers for timers |
| [PROTOCOL_EXAMPLES.md](PROTOCOL_EXAMPLES.md) | Practical timing in code |
| [INTERRUPT_SYSTEM.md](INTERRUPT_SYSTEM.md) | ISR priority and nesting |

---

## Key References

- [startup/startup_T5L.s](../startup/startup_T5L.s) - Hardware initialization
- [lib/timer/timer.c](../lib/timer/timer.c) - Timer implementation
- **T5L ASIC Datasheet**: Section 4 - Timer Modules
- **DGUS II Development Guide**: Timing specifications

---

**Document Version**: 1.0  
**Last Updated**: May 7, 2026  
**Status**: Complete - Ready for Phase 2 (v0.3.0)
