# T5L Hardware Architecture Guide

## Overview

The DWIN T5L is an 8051-based HMI (Human Machine Interface) controller with integrated DGUS II display interface. This guide provides detailed hardware specifications, register configurations, and initialization procedures essential for firmware development.

---

## 1. Processor Architecture

### 1.1 CPU Core
- **Architecture**: 8051 (MCS-51 core)
- **Variant**: T5L51 with DGUS II enhancements
- **Instruction Set**: Standard 8051 (compatible with SDCC)
- **Operating Frequency**: 11.0592 MHz (typical)
  - Allows standard baud rates without error
  - Crystal load capacitors: 22pF (typical)
  - Oscillator stabilization: ~100ms after power-on

### 1.2 Clock Generation
```
Crystal (11.0592 MHz) → Oscillator → CPU Clock (11.0592 MHz)
                                   → Timer/UART Clock Base
                                   → Dividers (/12 for timers, /32 for UART)
```

#### PCON Register (0x87) - Power Control
```
Bit 7: SMOD  - Serial port mode (affects UART baud rate)
Bit 6: ~SMOD0- Reserved in standard 8051
Bit 5: GF1   - General-purpose flag 1
Bit 4: GF0   - General-purpose flag 0
Bit 3: PD    - Power-down bit (enter sleep mode)
Bit 2: IDEL  - Idle mode bit
Bit 1: ~     - Reserved
Bit 0: ~     - Reserved
```

**SMOD Behavior** (Affects UART frequency):
- SMOD = 0: Baud rate = Fosc/(12 × 32 × (256 × TH1 + (256-TL1)))
- SMOD = 1: Baud rate = 2 × [Fosc/(12 × 32 × (256 × TH1 + (256-TL1)))]

---

## 2. Memory Organization

### 2.1 Memory Map

```
┌─────────────────────────────────────────┐
│  PROGRAM MEMORY (Flash ROM)             │
│  0x0000 - 0xFFFF (64KB typical)         │
│  Contains: Code + Const Data + Bootloader
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│  INTERNAL RAM (IRAM)                    │
│  0x00 - 0xFF (256 bytes)                │
│  Organized as:                          │
│    0x00-0x1F: Register Banks (8×4 bytes)│
│    0x20-0x2F: Bit-Addressable RAM       │
│    0x30-0x7F: General Purpose RAM       │
│    0x80-0xFF: SFR + Bank 2,3 Registers  │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│  EXTERNAL DATA MEMORY (XDATA)           │
│  0x0000 - 0x7FFF (32KB typical)         │
│  Contains:                              │
│    0x0000-0x1FFF: DGUS RAM (VP space)   │
│    0x2000-0x2FFF: UART RX Buffers (4KB)│
│    0x3000-0x3FFF: System/App Data      │
│    0x4000-0x7FFF: User Application      │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│  NOR FLASH (Persistent Storage)         │
│  Address via DGUS Interface (0x4000+)   │
│  Typical: 64+-512KB for user data       │
└─────────────────────────────────────────┘
```

### 2.2 IRAM Layout for This Project

```
Address    Allocated To              Size    SDCC Segment
─────────────────────────────────────────────────────────
0x00-0x1F  Register Banks (8×4B)      32B     [predefined]
0x20-0x2F  Bit-Addressable RAM        16B     [predefined]
0x30-0x7F  General Purpose             80B     DATA / IDATA
0x80       Port 0 (P0 - SFR)          1B      [SFR]
0x81       SP - Stack Pointer         1B      [SFR]
0x82-0xFF  SFR Registers             126B     [SFR space]
           + Higher bank registers
```

**Stack Allocation**:
- Typically starts at: 0x30 (after bit-addressable area)
- Maximum depth: ~80 bytes (sufficient for 4-5 nested calls)
- Stack overflow into heap: Critical error (hangs system)

### 2.3 XDATA Layout for This Project

```
Address     Purpose                        Size    Notes
──────────────────────────────────────────────────────────
0x0000      DGUS VP Space (RAM)            8KB     0x0000-0x1FFF
0x2000      UART2 RX Buffer                1KB     UART2_RX_LENTH=1024
0x2400      UART3 RX Buffer                1KB     UART3_RX_LENTH=1024
0x2800      UART4 RX Buffer                1KB     UART4_RX_LENTH=1024
0x2C00      UART5 RX Buffer                1KB     UART5_RX_LENTH=1024
0x3000      Frame Parse Buffer             512B    Temporary frame processing
0x3200      System Variables               1KB     System state, flags
0x3400      CRC Lookup Table (optional)    512B    If USE_CRC=1
0x3600      Reserved / Available           9216B   For user application
0x7FFF      End of XDATA
```

**Current Usage**: ~4.5KB fixed (buffers + system), ~27.5KB available

### 2.4 Flash ROM Organization

```
Address     Segment                    Size (typical)
────────────────────────────────────────────────────────
0x0000      ISR Vectors                64B    (8 vectors × 8B)
0x0040      Reset Handler              256B   (startup_T5L.s)
0x0100      Main Code + Libraries      ~30KB  (lib/ + src/)
0x9000      EEPROM Emulation           8KB    (for persistent data)
0xE000      Manufacturing Data         8KB    (reserved)
0xF000      Bootloader (locked)        4KB    (manufacturer)
```

**Typical Program Size**: ~25-30KB (leaves ~25KB for user code)

---

## 3. Special Function Registers (SFR)

### 3.1 Essential SFRs for DGUS Operation

#### Timer/Clock Related
| Register | Addr | Purpose | Bits | Notes |
|----------|------|---------|------|-------|
| TMOD | 0x89 | Timer mode select | 7-0 | Bits 3-0: Timer0, 7-4: Timer1 |
| TCON | 0x88 | Timer control | 7-0 | TF1,TR1,TF0,TR0,IE1,IT1,IE0,IT0 |
| TH0 | 0x8C | Timer 0 high byte | 7-0 | Reload value MSB |
| TL0 | 0x8A | Timer 0 low byte | 7-0 | Reload value LSB |
| PCON | 0x87 | Power control | 7-0 | SMOD, PD, IDEL flags |
| AUXR1 | 0xA2 | Aux register (T5L) | 7-0 | Additional timer/clock control |

#### UART Related
| Register | Addr | Purpose | Bits | Notes |
|----------|------|---------|------|-------|
| SCON | 0x98 | Serial control | 7-0 | RI,TI,RB8,TB8,REN,SM2,SM1,SM0 |
| SBUF | 0x99 | Serial buffer | 7-0 | RX/TX data byte |
| BAUD | 0x9D | Baud rate (T5L) | 7-0 | Baud rate generator |

#### Port/Pin Related
| Register | Addr | Purpose | Bits | Notes |
|----------|------|---------|------|-------|
| P0 | 0x80 | Port 0 | 7-0 | GPIO port |
| P1 | 0x90 | Port 1 | 7-0 | GPIO port |
| P4 | 0xC0 | Port 4 | 7-0 | Additional I/O |

#### Interrupt Related
| Register | Addr | Purpose | Bits | Notes |
|----------|------|---------|------|-------|
| IE | 0xA8 | Interrupt Enable | 7-0 | EA,—,ET2,ES1,ET1,EX1,ET0,EX0 |
| IP | 0xB8 | Interrupt Priority | 7-0 | PT2,—,PS,PT1,PX1,PT0,PX0 |
| EIE | 0xE8 | Extended IE (T5L) | 7-0 | Additional interrupt enables |

### 3.2 DGUS-Specific Registers

The T5L includes specialized registers for DGUS II interface control:

#### DGUS System Registers (via SFR)
These are typically accessed through:
- **DGUS Control Port**: Maps DGUS operations to SFR space
- **VP Address/Data Registers**: Direct DGUS RAM access via SFR

```
Typical DGUS Access Pattern:
1. Set DGUS Address Register → 0x0084 (page ID)
2. Set DGUS Data Register → 0x0001 (new page)
3. DGUS processes command and updates display
```

**Note**: Consult T5L ASIC datasheet Section 5 for exact register mappings

---

## 4. Pin Configuration and I/O Mapping

### 4.1 Common Pin Functions

| Pin | Function | Default | Notes |
|-----|----------|---------|-------|
| P0.0-P0.7 | Port 0 GPIO | GPIO | Multiplexed address/data in ext. memory mode |
| P1.0-P1.7 | Port 1 GPIO | GPIO | General purpose I/O |
| P4.0-P4.3 | Port 4 GPIO | GPIO | Additional general purpose |
| RXD2 | UART2 RX | Input | Inverted (active low) |
| TXD2 | UART2 TX | Output | Inverted (active low) |
| RXD3 | UART3 RX | Input | Standard RS-232 |
| TXD3 | UART3 TX | Output | Standard RS-232 |
| RXD4 | UART4 RX | Input | RS-485 receiver |
| TXD4 | UART4 TX | Output | RS-485 transmitter |
| RXD5 | UART5 RX | Input | RS-485 receiver |
| TXD5 | UART5 TX | Output | RS-485 transmitter |
| DE4, RE4 | UART4 RS-485 Dir | Output | Direction control (active high) |
| DE5, RE5 | UART5 RS-485 Dir | Output | Direction control (active high) |
| SDA | I2C Data | Open-drain | For external RTC |
| SCL | I2C Clock | Open-drain | For external RTC |

### 4.2 RS-485 Configuration

For UART4/5 RS-485 operation:

```
Hardware Connections:
├─ TXD (UART TX) ──→ DI (Driver Input)
├─ RXD (UART RX) ──→ RO (Receiver Output)
├─ DE (Driver Enable) ──→ Transmit request
├─ RE (Receiver Enable) ──→ ~Transmit request
└─ A, B ──→ Differential pair to bus

Termination:
├─ 120Ω between A and B (end of cable)
├─ 560Ω pull-up to +5V on A (optional)
└─ 560Ω pull-down to GND on B (optional)

Timing (Critical):
├─ DE assert before TX: ≥1μs
├─ DE de-assert after TX: ≥1μs
└─ Collision detection: Monitor during transmission
```

---

## 5. Reset and Initialization Sequence

### 5.1 Power-On Reset (POR) Procedure

```
┌─────────────────────────────────────────────────────┐
│ 1. Power Supply Ramps Up                            │
│    ├─ VCC reaches operating voltage (3.3V or 5V)   │
│    └─ Time: ~100ms with proper decoupling          │
├─────────────────────────────────────────────────────┤
│ 2. Oscillator Stabilization                         │
│    ├─ Crystal oscillator starts (11.0592 MHz)      │
│    └─ Stabilization time: ~100ms                   │
├─────────────────────────────────────────────────────┤
│ 3. POR Circuit Triggers                             │
│    ├─ RST pin held high for ~100ms                 │
│    ├─ All SFRs reset to default values             │
│    └─ Program counter set to 0x0000                │
├─────────────────────────────────────────────────────┤
│ 4. Reset Handler Executes (startup_T5L.s)          │
│    ├─ Initialize stack pointer (SP)                │
│    ├─ Configure SFRs (timers, UART, ports)         │
│    └─ Clear IRAM and jump to main()                │
├─────────────────────────────────────────────────────┤
│ 5. DGUS II Handshake                               │
│    ├─ Display ready signal detected                │
│    ├─ Initial VP read confirms sync                │
│    └─ Display ready for commands                   │
└─────────────────────────────────────────────────────┘
```

### 5.2 Startup Sequence for Firmware

Located in [startup/startup_T5L.s](../startup/startup_T5L.s):

1. **Stack Initialization**
   ```
   SP = 0x30  (Stack starts above bit-addressable RAM)
   ```

2. **Timer0 Setup for 1ms Tick**
   ```
   TMOD = 0x01     (Timer0 mode 1: 16-bit counter)
   TH0/TL0 = 0xFC 0x18  (Reload: 65536-11059 = 54477 = 0xFC18)
   TR0 = 1         (Start timer)
   ET0 = 1         (Enable interrupt)
   ```

3. **UART Initialization**
   - See Section 6.2 below

4. **Global Interrupts Enable**
   ```
   EA = 1          (Enable all interrupts)
   ```

5. **DGUS Sync**
   - Perform initial VP read to detect display presence

---

## 6. UART Configuration and Baud Rate Calculation

### 6.1 UART Baud Rate Formula

For 8051 UART (standard mode 1):
```
Baud Rate = FOSC / (12 × 32 × (256 - TH1)) × (1 + SMOD)
```

For T5L (with SMOD support):
```
When SMOD = 0:
  BR = FOSC / (12 × 32 × (256 - TH1))

When SMOD = 1:
  BR = 2 × FOSC / (12 × 32 × (256 - TH1))
```

### 6.2 Common Baud Rate Values (11.0592 MHz)

| Baud Rate | TH1 Value | SMOD | Error | Notes |
|-----------|-----------|------|-------|-------|
| 1200 | 0xFD | 0 | 0% | Standard |
| 2400 | 0xFE | 0 | 0% | Standard |
| 4800 | 0xFE | 1 | 0% | With SMOD |
| 9600 | 0xFF | 1 | 0% | High speed |
| 19200 | 0xFF | 1 | 0% | Requires SMOD |
| 38400 | - | - | - | Difficult at this frequency |
| 57600 | - | - | - | Not recommended |
| 115200 | 0xFF | 1 | 0% | **Default for DGUS** |

**Crystal Dependency**:
These calculations assume 11.0592 MHz. If using different crystal:
- 12 MHz: Slight baud rate errors (~2-3%)
- 14.7456 MHz: Alternative standard
- Other: Must recalculate TH1 for each baud rate

### 6.3 UART2-5 Configuration Registers

Each UART (2-5) has dedicated SFR:

```c
// UART2 Configuration (as example)
SCON0 = 0x10;      // Mode 0: Sync (not used for DGUS)
SCON0 = 0x50;      // Mode 1: 8-bit UART, RX enabled
BAUD2 = 0xFF;      // TH1 replacement for UARTs 2-5
BAUD2 = BAUD2 | 0x80;  // Enable UART2 (if required)
```

---

## 7. Interrupt Priority and Nesting

### 7.1 Standard 8051 Interrupt Structure

```
Interrupt Source | Vector | Default Priority | TCON/IE Bit
─────────────────────────────────────────────────────────────
External 0 (INT0) | 0x03 | Lowest | IT0 / EX0
Timer 0 Overflow  | 0x0B | Low    | TR0 / ET0
External 1 (INT1) | 0x13 | Med    | IT1 / EX1
Timer 1 Overflow  | 0x1B | Med    | TR1 / ET1
Serial (UART0)    | 0x23 | High   | SCON.RI/TI / ES
Timer 2 Overflow  | 0x2B | Highest| T2CON / ET2
```

### 7.2 Interrupt Enable Register (IE - 0xA8)

```
Bit 7: EA   - Enable All Interrupts (global enable)
Bit 6: —    - Reserved
Bit 5: ET2  - Enable Timer 2 Interrupt
Bit 4: ES1  - Enable UART1 Interrupt (Serial port 1)
Bit 3: ET1  - Enable Timer 1 Interrupt
Bit 2: EX1  - Enable External Interrupt 1
Bit 1: ET0  - Enable Timer 0 Interrupt
Bit 0: EX0  - Enable External Interrupt 0
```

**Typical Configuration for DGUS**:
```c
IE = 0x80 | 0x02 | 0x08;  // EA=1, ET0=1, ET1=1 (for Timer+UART1)
// Or for UART0/1:
IE = 0x80 | 0x02 | 0x10;  // EA=1, ET0=1, ES=1 (for Timer+Serial)
```

### 7.3 Interrupt Priority Register (IP - 0xB8)

```
Bit 7: —   - Reserved
Bit 6: —   - Reserved
Bit 5: PT2 - Priority Timer 2 (0=low, 1=high)
Bit 4: PS  - Priority Serial (0=low, 1=high)
Bit 3: PT1 - Priority Timer 1 (0=low, 1=high)
Bit 2: PX1 - Priority External 1 (0=low, 1=high)
Bit 1: PT0 - Priority Timer 0 (0=low, 1=high)
Bit 0: PX0 - Priority External 0 (0=low, 1=high)
```

**Recommended Priority Setup** (for DGUS):
```c
// Timer (system tick) = High priority
// UART = High priority
// External interrupts = Low priority
IP = (1 << 1) | (1 << 4);  // PT0=1, PS=1 (high priority)
```

---

## 8. Watchdog Timer Configuration

### 8.1 Watchdog Initialization

The T5L typically includes a watchdog timer to prevent system hangs:

```c
// Maximum watchdog timeout (device-dependent)
// Usually: ~500ms to 2 seconds

// Watchdog enable (typical SFR location)
// WDT_ENABLE(0x01);  // Enable watchdog
// WDT_RESET();       // Reset counter

// Periodic reset required in main loop
```

**Current Implementation Status**: Check [lib/sys/sys.c](../lib/sys/sys.c) for watchdog handling.

---

## 9. Power Management Modes

### 9.1 Idle Mode

Entered via `PCON.IDEL = 1`:
- **State**: CPU halted, oscillator running
- **Wakeup**: Interrupt or reset
- **Current**: ~50% of active mode
- **Typical Use**: Waiting for external event

```c
// Enter idle mode
PCON |= 0x01;  // Set IDEL bit
```

### 9.2 Power-Down Mode

Entered via `PCON.PD = 1`:
- **State**: CPU halted, oscillator stopped, minimal peripherals active
- **Wakeup**: External interrupt (INT0/INT1) or reset
- **Current**: ~5% of active mode (~5μA typical)
- **Typical Use**: Long-term sleep / battery standby

```c
// Enter power-down mode
PCON |= 0x02;  // Set PD bit
// Wakeup via INT0/INT1
```

### 9.3 Wake-Up Sources

| Source | Configuration | Response Time |
|--------|---------------|----------------|
| INT0 | Edge-triggered | ~10ms (oscillator restart) |
| INT1 | Edge-triggered | ~10ms |
| Reset | External RST | ~100ms (full restart) |
| Serial RX | If enabled | ~10ms |

---

## 10. Verification Checklist

After implementing hardware initialization:

- [ ] Crystal oscillator is stable (scope verification optional)
- [ ] System tick timer (Timer0) generates 1ms interrupts
- [ ] UART baud rate matches expected frequency (serial monitor check)
- [ ] DGUS synchronization successful (display responsive to VP reads)
- [ ] All UARTs initialized at configured baud rates
- [ ] RTC interface operational (if configured)
- [ ] I2C communication functional (if external RTC used)
- [ ] Interrupt priorities set correctly (no infinite loops/hangs)
- [ ] Stack usage within bounds (< 50% allocated)
- [ ] XDATA buffers initialized and accessible

---

## 11. Related Files in This Project

| File | Purpose |
|------|---------|
| [startup/startup_T5L.s](../startup/startup_T5L.s) | Assembly initialization code |
| [include/t5l1.h](../include/t5l1.h) | SFR definitions for SDCC |
| [include/config.h](../include/config.h) | Configuration macros |
| [lib/sys/sys.c](../lib/sys/sys.c) | DGUS system interface |
| [lib/uart/uart.c](../lib/uart/uart.c) | UART driver implementation |
| [lib/timer/timer.c](../lib/timer/timer.c) | Timer and delay functions |

---

## 12. Key References

### Internal References
- See [DGUS_PROTOCOL_GUIDE.md](DGUS_PROTOCOL_GUIDE.md) for communication protocol details
- See [MEMORY_MANAGEMENT.md](MEMORY_MANAGEMENT.md) for detailed memory allocation strategies
- See [TIMER_CLOCK_GUIDE.md](TIMER_CLOCK_GUIDE.md) for timer and clock configuration

### External References
- **DGUS II Development Guide** (`T5L_DGUSII_V2.9-0207.pdf`) - Section 2: Hardware Overview
- **T5L ASIC Datasheet** (`T5L-ASIC-2024-09-25.pdf`) - Sections 2-5: Architecture and Peripherals
- **SDCC Manual** - 8051 Port documentation
- **8051 Microcontroller Manual** - Standard register and SFR definitions

---

**Document Version**: 1.0  
**Last Updated**: May 7, 2026  
**Status**: Complete - Ready for Phase 1 (v0.2.0)
