# Memory Management Guide

**Comprehensive memory layout and optimization strategies for T5L firmware**

---

## 1. Memory Architecture Overview

The T5L microcontroller provides three distinct memory regions, each with specific characteristics and usage patterns:

### 1.1 Memory Organization Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    PROGRAM MEMORY (Flash)                       │
│                    0x0000 - 0xFFFF (64KB)                       │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ Code, Constants, Initialization Data                   │   │
│  │ SDCC Segments: CODE, CONST, INITIALIZERS               │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│           INTERNAL RAM (IRAM) - Fast Access                    │
│           0x00 - 0xFF (256 bytes total)                        │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ Register Banks, Bit RAM, General-Purpose RAM, SFR Space│   │
│  │ SDCC Segments: DATA, IDATA, BIT, SFR                   │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│       EXTERNAL DATA RAM (XDATA) - Slower but Abundant           │
│       0x0000 - 0x7FFF (32KB typical, device-dependent)         │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ DGUS VP Space, UART RX Buffers, Application Data        │   │
│  │ SDCC Segments: XDATA, PDATA                             │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│              NOR FLASH - Persistent Storage                     │
│              Accessed via DGUS Interface                        │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ User Data, Calibration, Configuration Persistence      │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Internal RAM (IRAM) - 0x00 to 0xFF

### 2.1 IRAM Organization

```
Address Range   Name                 Size    Purpose                Access
────────────────────────────────────────────────────────────────────────────
0x00-0x07       Reg Bank 0           8B      R0-R7 for main code     Fast
0x08-0x0F       Reg Bank 1           8B      R0-R7 for ISR level 1   Fast
0x10-0x17       Reg Bank 2           8B      R0-R7 for ISR level 2   Fast
0x18-0x1F       Reg Bank 3           8B      R0-R7 (backup)          Fast
0x20-0x2F       Bit-Addressable      16B     Bool variables, flags   Fast
0x30-0x7F       General Purpose      80B     Local vars, temp data   Fast
0x80-0xFF       SFR Space           128B     Port, Timer, UART regs  Critical
```

### 2.2 IRAM Usage by SDCC Segments

| SDCC Segment | IRAM Location | Size | Use Case |
|--------------|---------------|------|----------|
| DATA | 0x30-0x7F | ~80B | Fast global variables |
| IDATA | 0x00-0xFF | 256B | Generic addressable data |
| BIT | 0x20-0x2F | 16B (128 bits) | Boolean flags |
| SFR | 0x80-0xFF | 128B | Register definitions |

### 2.3 Recommended IRAM Allocation for DWIN-T5L Project

```
Address    Allocation              Size    SDCC Segment   Usage
───────────────────────────────────────────────────────────────────
0x00-0x1F  Register Banks          32B     Predefined     CPU Registers
0x20-0x2F  Bit-Addressable         16B     BIT            System flags
0x30-0x4F  Common Variables        32B     DATA (fast)    Timers, counters
0x50-0x7F  System State            48B     DATA           UART state, status
0x80-0xFF  SFR + Extended Regs    128B     SFR            Port, Timer, UART
───────────────────────────────────────────────────────────────────
           TOTAL                  256B
```

### 2.4 IRAM Stack Considerations

```
┌─────────────────────┐
│ 0x7F                │  ← END of IRAM
├─────────────────────┤
│  [Stack grows down] │  Stack: Typical max depth ~20-40 bytes
│                     │  (4-6 nested function calls)
├─────────────────────┤
│  [FREE SPACE]       │  ~80-100 bytes available
├─────────────────────┤
│  [Heap/Data]        │  Global variables, static data (~50 bytes used)
├─────────────────────┤
│  Bit-Addressable    │  16 bytes (0x20-0x2F)
├─────────────────────┤
│  Register Banks     │  32 bytes (0x00-0x1F)
└─────────────────────┘
```

**Stack Overflow Risk**: If stack grows into heap (upward), system hangs.
- **Mitigation**: Keep variables small, avoid deep nesting, monitor stack usage

### 2.5 Optimizing IRAM Usage

#### Strategy 1: Use BIT-Addressable Variables for Flags
```c
// INEFFICIENT (8 bytes for 4 booleans)
u8 uart2_active;
u8 uart3_active;
u8 uart4_active;
u8 uart5_active;

// EFFICIENT (2 bytes for 4 booleans, uses BIT segment)
__bit uart2_active;
__bit uart3_active;
__bit uart4_active;
__bit uart5_active;
```

#### Strategy 2: Reuse Variables Across Functions
```c
// Function A needs temporary buffer during processing
// Function B needs temporary buffer during processing
// Solution: Use same location in IRAM, allocated by compiler

void func_a(void) {
    u8 temp_buffer[16];  // Allocated in IRAM
    // ... use temp_buffer
}

void func_b(void) {
    u8 temp_buffer[16];  // Compiler may reuse same location
    // ... use temp_buffer
}
```

#### Strategy 3: Use Register Variables for Tight Loops
```c
void process_frame(void) {
    register u8 i;      // Allocate to CPU register
    register u8 crc;    // CPU register
    
    for (i = 0; i < FRAME_SIZE; i++) {
        crc ^= frame[i];  // Faster than accessing IRAM
    }
}
```

---

## 3. External Data RAM (XDATA) - 0x0000 to 0x7FFF

### 3.1 XDATA Physical Organization (32 KB)

```
Address    Allocated To                          Size    Purpose
───────────────────────────────────────────────────────────────────
0x0000     DGUS VP Space (Display RAM)           8KB     VP 0x0000-0x1FFF
0x2000     UART2 RX Buffer                       1KB     Receive data queue
0x2400     UART3 RX Buffer                       1KB     Receive data queue
0x2800     UART4 RX Buffer                       1KB     Receive data queue
0x2C00     UART5 RX Buffer                       1KB     Receive data queue
0x3000     Frame Parse Buffer                    256B    Temp frame assembly
0x3100     CRC-16 Lookup Table (if enabled)      512B    Optimization table
0x3300     System Control Registers              256B    State machine vars
0x3400     Application Data Space                27.5KB  User allocation
0x7FFF     END of XDATA
───────────────────────────────────────────────────────────────────
           TOTAL ALLOCATED                       32KB
```

### 3.2 DGUS VP Space (0x0000-0x1FFF)

```
VP Address    Purpose                       Access Pattern
──────────────────────────────────────────────────────────────
0x0000-0x00FF System Variables              Read/Write often
0x0100-0x01FF Text Buffers                  Write, read rarely
0x0200-0x02FF Slider/Input Values           Read often (auto-upload)
0x0300-0x03FF Button States                 Read on changes
0x0400-0x04FF Graph/Chart Data              Write rapidly during plots
0x0500-0x0FFF Additional Widget Data        Varies
0x1000-0x1FFF Reserved for expansion        Future use
```

**Note**: Exact usage depends on display design file. See [include/addresses.h](../include/addresses.h) for project mappings.

### 3.3 UART RX Buffer Organization

Each UART (2-5) gets a dedicated 1 KB circular buffer:

```
UART2 RX Buffer (0x2000-0x23FF):

┌──────────────────────────────────────────────────────────┐
│ [Frame 1] [Frame 2] [Frame 3] [Incoming...] [Free Space] │
├──────────────────────────────────────────────────────────┤
│         write_ptr (points here)  read_ptr (points here)  │
└──────────────────────────────────────────────────────────┘

Circular Buffer Mechanics:
  - write_ptr advances as data arrives from UART RX ISR
  - read_ptr advances as frames are processed in main loop
  - Buffer full: write_ptr ≈ read_ptr (circular wrap)
  - Buffer empty: write_ptr = read_ptr
  
Overflow Behavior:
  - If write_ptr catches read_ptr: oldest data discarded
  - Frame loss may occur under high traffic
  - CRC validation helps detect corruption
```

**Configuration**: [include/config.h](../include/config.h)
```c
#define UART2_RX_LENTH 1024
#define UART3_RX_LENTH 1024
#define UART4_RX_LENTH 1024
#define UART5_RX_LENTH 1024
```

### 3.4 Circular Buffer Implementation

```c
typedef struct {
    u8 buffer[1024];
    u16 write_ptr;
    u16 read_ptr;
    u8 is_full;
} UART_RX_BUFFER;

// Add byte to buffer
void buffer_write(UART_RX_BUFFER *buf, u8 byte) {
    buf->buffer[buf->write_ptr++] = byte;
    
    if (buf->write_ptr >= 1024) {
        buf->write_ptr = 0;
    }
    
    if (buf->write_ptr == buf->read_ptr) {
        buf->is_full = 1;  // Buffer overflow!
    }
}

// Read byte from buffer
u8 buffer_read(UART_RX_BUFFER *buf) {
    u8 byte = buf->buffer[buf->read_ptr++];
    
    if (buf->read_ptr >= 1024) {
        buf->read_ptr = 0;
    }
    
    buf->is_full = 0;
    return byte;
}

// Check available bytes
u16 buffer_available(UART_RX_BUFFER *buf) {
    if (buf->is_full) return 1024;
    
    if (buf->write_ptr >= buf->read_ptr) {
        return buf->write_ptr - buf->read_ptr;
    } else {
        return 1024 - (buf->read_ptr - buf->write_ptr);
    }
}
```

### 3.5 Application Data Space (0x3400-0x7FFF)

Available for user application (~27.5 KB):

```
Recommended Structure:
  0x3400: Error history buffer (256 bytes)
  0x3500: Sensor data cache (512 bytes)
  0x3700: Configuration parameters (1 KB)
  0x3B00: State machine variables (1 KB)
  0x3F00: Performance metrics (256 bytes)
  0x4000: End of reserved application
  0x4000-0x7FFF: Free heap for malloc() (~15.5 KB)
```

---

## 4. Flash Memory (Program Memory)

### 4.1 Flash Layout Overview

```
Address Range   Segment Name               Size        Purpose
──────────────────────────────────────────────────────────────────
0x0000          ISR Vector Table           64B         Interrupt vectors
0x0040          Reset/Startup Code         256B        Startup sequence
0x0100          Main Application Code      ~25KB       Code + libs
0x9000          EEPROM Emulation Area      8KB         Persistent data
0xE000          Manufacturing Data        8KB         Reserved data
0xF000          Bootloader (Locked)       4KB         Manufacturer code
──────────────────────────────────────────────────────────────────
                TOTAL                     64KB
```

### 4.2 SDCC Flash Segments

| Segment | Location | Purpose | Usage |
|---------|----------|---------|-------|
| CODE | 0x0100+ | Executable code | Functions, ISRs |
| CONST | 0x2000+ | Read-only data | Strings, tables |
| INIT | 0x3000+ | Init values | Variable initializers |
| XSEG | 0x4000-0x7FFF | XDATA in FLASH | Stored in FLASH, copied to XDATA at startup |
| PSEG | 0x8000+ | Paged XDATA | If paging enabled |

### 4.3 Code Size Characteristics

Current T5L template:
```
Module              Estimated Size    Notes
──────────────────────────────────────────────────────
startup_T5L.s       ~200B             Assembly
main.c              ~500B             Entry point
lib/uart/uart.c     ~3KB              UART drivers, frame parsing
lib/sys/sys.c       ~2KB              DGUS VP interface
lib/timer/timer.c   ~1KB              Timer management
lib/rtc/rtc.c       ~1.5KB            RTC support
lib/crc16/crc16.c   ~0.5KB            CRC calculation
app_defs.c          ~1KB              Application definitions
Constants/Strings   ~2KB              Default configuration

TOTAL TYPICAL       ~11KB (including optimization)
AVAILABLE FOR APP   ~53KB (plenty of room)
```

### 4.4 Reducing Code Size

#### Strategy 1: Use CRC Lookup Table or Calculation
```c
// Option A: Lookup table (fast, uses 512 bytes)
#define USE_CRC_TABLE 1

// Option B: Runtime calculation (slow, saves 512 bytes)  
#define USE_CRC_TABLE 0
```

#### Strategy 2: Remove Unused Libraries
In [Makefile](../Makefile):
```makefile
# Comment out unneeded:
# SOURCES += lib/rtc/rtc.c   # If no RTC needed
# SOURCES += lib/timer/timer.c # If no delays needed
```

#### Strategy 3: Optimize Compiler Settings
In [Makefile](../Makefile):
```makefile
# Enable size optimization
CFLAGS += --opt-code-size  # Optimize for smaller code
CFLAGS += -r             # Use register allocation
```

---

## 5. Memory Usage Patterns

### 5.1 Typical Memory Snapshot (at Runtime)

```
IRAM Usage:
  Register Banks:     32B (always used)
  Bit Variables:      8B (flags and counters)
  Global Vars:        40B (timers, UART state)
  Free Stack:         170B (worst case)
  ─────────────────────
  TOTAL:              ~250B / 256B (97% utilized)

XDATA Usage:
  DGUS VP Space:      8KB (fixed)
  UART RX Buffers:    4KB (fixed)
  System State:       0.5KB
  Application:        2KB (varies)
  Free:               ~17.5KB
  ─────────────────────
  TOTAL:              ~14.5KB / 32KB (45% utilized)

FLASH Usage:
  Code + Libraries:   ~12KB
  Constants:          ~2KB
  Available:          ~50KB
  ─────────────────────
  TOTAL:              ~14KB / 64KB (22% utilized)
```

### 5.2 Memory Stress Points

| Resource | Limit | Warning | Critical |
|----------|-------|---------|----------|
| IRAM Stack | 256B | >180B used | >240B used |
| XDATA RX Buffers | 4KB | Overflow | Frame loss |
| Buffer Depth | 1KB/UART | 80% full | 95% full |
| IRAM Variables | 80B | 100% | Out of memory |

---

## 6. Advanced Memory Topics

### 6.1 Memory Bank Switching

The 8051 supports 4 register banks. SDCC can use them for ISR context:

```c
// Normal interrupt (uses Bank 0)
void timer0_isr(void) __interrupt(1) {
    // Process Timer 0 overflow
    // Uses R0-R7 from Bank 0 (doesn't overlap with main code)
}

// Higher priority interrupt (uses Bank 1)
void uart_isr(void) __interrupt(4) __using(1) {
    // Process UART events
    // Uses R0-R7 from Bank 1 (different from main)
}
```

**Benefit**: Avoids saving/restoring context in tight loops

### 6.2 Paged External Memory (Advanced)

If XDATA > 64K (some variants support this):

```makefile
# Enable paging in Makefile
CFLAGS += --code-page=0x03
```

This requires:
- Port pin to control high address bits
- Duplication of XDATA setup code
- Manual page switching in application

**Current Project**: Not needed (32KB XDATA sufficient)

### 6.3 Malloc() Usage

SDCC provides `malloc()` and `free()`:

```c
// Allocate 256 bytes on heap (in XDATA)
u8 *data = malloc(256);

if (data == NULL) {
    // Out of memory!
    return -1;
}

// Use data
data[0] = 0x42;

// Free when done
free(data);
data = NULL;
```

**Caution**: 
- Heap fragmentation over time
- Not suitable for real-time interrupts
- Overhead: ~6 bytes per allocation
- Best used during initialization only

---

## 7. Recommended XDATA Layout for Applications

### 7.1 Conservative Allocation (Safe for most projects)

```c
// In src/main.c or dedicated memory layout file

// DGUS VP Space
xdata volatile u8 dgus_vp_space[0x2000] __at(0x0000);

// UART RX Buffers (allocated in uart.c)
xdata u8 uart2_rx[UART2_RX_LENTH] __at(0x2000);
xdata u8 uart3_rx[UART3_RX_LENTH] __at(0x2400);
xdata u8 uart4_rx[UART4_RX_LENTH] __at(0x2800);
xdata u8 uart5_rx[UART5_RX_LENTH] __at(0x2C00);

// System Buffers
xdata u8 frame_parse_buf[512]      __at(0x3000);
xdata u8 crc_lookup[512]           __at(0x3200);  // if USE_CRC_TABLE=1
xdata u8 sys_state[256]            __at(0x3400);

// Application starts at 0x3500 (28.5 KB available)
```

### 7.2 Aggressive Allocation (For minimal designs)

```c
// Conservative UART buffers (512 bytes each if link is clean)
xdata u8 uart2_rx[512] __at(0x2000);
xdata u8 uart3_rx[512] __at(0x2200);
xdata u8 uart4_rx[512] __at(0x2400);
xdata u8 uart5_rx[512] __at(0x2600);

// Shared frame buffer (if only one UART active at a time)
xdata u8 frame_parse_buf[1024]     __at(0x2800);

// Application at 0x3000 (~13 KB available)
```

---

## 8. Memory Monitoring and Analysis

### 8.1 Checking Compiled Code Size

```bash
# In terminal:
sdcc --version
ls -lh build/firmware.hex

# Check individual module sizes:
size build/obj/*.rel

# Detailed map file analysis:
grep "CODE" build/firmware.map | tail -20
```

### 8.2 XDATA Usage Verification

Adding debug output to MAIN loop:

```c
void main(void) {
    Sys_Init();
    Uart_Init();
    
    // Check XDATA allocation is valid
    u8 *test_ptr = (u8*)0x3400;
    *test_ptr = 0x55;
    if (*test_ptr != 0x55) {
        // XDATA not responding!
        while(1);
    }
    
    App_Init();
    
    while(1) {
        // Periodically log memory state via UART
        static u8 count = 0;
        if (++count == 100) {  // Every 100 loops (~100ms)
            uart_send_str("XDATA OK\r\n");
            count = 0;
        }
        
        DGUS_MonitorAndSendUpdates();
        DGUS_ProcessAllUarts();
    }
}
```

### 8.3 IRAM Stack Depth Monitoring

```c
// In startup code or ISR:
void check_stack_depth(void) {
    // Read current SP
    u8 sp_value = _asm("mov A, SP");
    
    // If SP < 0x40, stack is deep
    if (sp_value < 0x40) {
        // WARNING: Stack depth dangerous
        // Reduce nested calls or increase buffer sizes
    }
}
```

---

## 9. Memory Bottlenecks & Solutions

### Bottleneck 1: UART RX Buffer Overflow

**Symptom**: Frames lost under high traffic (multiple UARTs active)

**Solution Options**:
1. Increase buffer size to 2KB (uses extra 4KB XDATA)
   - Trade: Lose 4KB of application space
   
2. Process frames faster (reduce main loop delay)
   - Trade: Higher CPU usage
   
3. Reduce baud rate on heavy-traffic UARTs
   - Trade: Slower communication
   
4. Use DMA (if available on variant)
   - Trade: Complex hardware setup

### Bottleneck 2: Limited IRAM for Variables

**Symptom**: Compilation error "Cannot assign data to memory: data space"

**Solutions**:
1. Move variables to XDATA:
   ```c
   // Before (IRAM)
   u8 big_buffer[128];
   
   // After (XDATA)
   xdata u8 big_buffer[128];
   ```

2. Use bitfields for flags:
   ```c
   // Before (16 bytes for 8 booleans)
   u8 flag1, flag2, flag3, flag4, flag5, flag6, flag7, flag8;
   
   // After (1-2 bytes for 8 booleans)
   __bit flag1, flag2, flag3, flag4, flag5, flag6, flag7, flag8;
   ```

3. Reduce stack depth:
   - Avoid very deep function nesting
   - Use loops instead of repeated calls

### Bottleneck 3: Large Application Code

**Symptom**: Flash nearly full, can't add features

**Solutions**:
1. Use separate bank if available (paged ROM)
2. Move constants to external FLASH (NOR)
3. Modularize code (load only needed modules)
4. Optimize CRC (table vs. runtime)

---

## 10. Memory Testing Checklist

When developing new features:

- [ ] Check IRAM usage hasn't exceeded 80% (leave margin for stack)
- [ ] Verify XDATA allocation doesn't overlap critical buffers
- [ ] Test buffer overflow scenarios (fill UART buffers with test data)
- [ ] Monitor stack depth under worst-case nesting
- [ ] Verify malloc() doesn't fragment heap over time
- [ ] Check code size after adding new functions
- [ ] Test all 4 UARTs simultaneously at high baud rates
- [ ] Confirm NULL pointer dereferences are caught early

---

## 11. Related Documentation

| Document | Purpose |
|----------|---------|
| [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md) | IRAM/XDATA physical layout |
| [DGUS_PROTOCOL_GUIDE.md](DGUS_PROTOCOL_GUIDE.md) | VP address space mapping |
| [VP_ADDRESS_MAP.md](VP_ADDRESS_MAP.md) | VP allocation details |

---

## 12. Key References

### Internal Files
- [include/config.h](../include/config.h) - Buffer size configuration
- [lib/uart/uart.c](../lib/uart/uart.c) - Buffer implementation details
- [Makefile](../Makefile) - Compiler memory settings
- [startup/startup_T5L.s](../startup/startup_T5L.s) - Stack initialization

### External References
- SDCC Manual: Memory Models and Segments
- 8051 Microcontroller Datasheet: Memory organization
- **T5L ASIC Datasheet**: Memory specifications (Section 3)

---

**Document Version**: 1.0  
**Last Updated**: May 7, 2026  
**Status**: Complete - Ready for Phase 1 (v0.2.0)
