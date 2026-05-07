# DWIN-T5L-SDCC-Template - Project Documentation

## Overview

**DWIN-T5L-SDCC-Template** is an open-source firmware template for DWIN T5L/T5L51 HMI (Human Machine Interface) controllers. It provides a cross-platform alternative to Keil-based development, using the SDCC (Small Device C Compiler) toolchain for 8051 architecture.

- **Version**: 0.1.1
- **Architecture**: 8051 (mcs51 large model)
- **Compiler**: SDCC
- **License**: CC BY-NC-SA 4.0
- **Author**: Recep Şenbaş

---

## Project Structure

```
DWIN-T5L-SDCC-Template/
├── src/                        # Application source code
│   ├── main.c                  # Entry point - initializes system and runs DGUS loop
│   └── app/                    # Application layer
│       ├── app.c               # User-defined application logic
│       ├── app_defs/           # Application definitions (VP addresses, constants)
│       ├── vp_flags.h          # DGUS VP change flag handler
│       └── uart_flags.h        # UART-based event flag handler
│
├── lib/                        # Reusable library modules
│   ├── sys/                    # System-level DGUS RAM/Flash access
│   ├── uart/                   # UART2-5 driver with DGUS protocol handling
│   ├── timer/                  # Timer0-3 management (1ms tick, delays)
│   ├── rtc/                    # RTC support (RX8130, SD2058)
│   └── crc16/                  # CRC-16/MODBUS checksum
│
├── include/                    # Global headers
│   ├── t5l1.h                  # T5L SFR and bit definitions for SDCC
│   ├── config.h                # Central configuration (UART, baud, CRC, RTC)
│   └── addresses.h             # DGUS VP address definitions
│
├── startup/                    # Assembly startup code
│   └── startup_T5L.s           # T5L MCU initialization
│
├── examples/                   # Optional usage examples
│   └── examples.c              # API demonstration code
│
├── doc/                        # Reference documentation
│   ├── T5L_DGUSII_V2.9-0207.pdf
│   └── T5L-ASIC-2024-09-25.pdf
│
├── build/                      # Build output (generated)
│   ├── obj/                    # Compiled .rel object files
│   └── dist/                   # Final firmware binaries
│
├── artifacts/                  # Release packages
├── prebronchial/               # (Purpose unclear from analysis)
├── Makefile                    # Cross-platform build system
├── ReadMe.md                   # User-facing documentation
└── ChangeLog.md                # Version history
```

---

## Core Libraries

### 1. System Library (`lib/sys/`)

Provides low-level DGUS interface functions for interacting with the HMI display.

**Key Functions:**
- `DGUS_Read_VP(addr)` - Read 16-bit value from VP address
- `DGUS_Write_VP(addr, val)` - Write 16-bit value to VP address
- `DGUS_WriteBytes(addr, buf, len)` - Write byte array to DGUS RAM
- `DGUS_ReadBytes(addr, buf, words)` - Read byte array from DGUS RAM
- `DGUS_WriteText(addr, text)` - Write null-terminated string to DGUS RAM
- `DGUS_GetPageID()` / `DGUS_SetPageID(page)` - Page navigation
- `DGUS_NOR_Write/Read()` - NOR Flash read/write operations
- `DGUS_GraphPush(channel, value)` - Push data point to curve widget
- `DGUS_GraphClear(channel)` - Clear curve channel buffer
- `DGUS_ResetHmi()` - Reset the HMI controller

### 2. UART Library (`lib/uart/`)

Multi-UART driver supporting UART2 through UART5 with DGUS protocol parsing.

**Features:**
- Configurable baud rates (default: 115200)
- RX buffer management (default: 1024 bytes per UART)
- DGUS frame parsing (0x5A 0xA5 protocol)
- Command 0x82 (read VP) and 0x83 (write VP) handling
- Optional CRC-16 validation
- Optional OK response (0x4F 0x4B)
- Auto-upload mode for VP changes
- RS-485 direction control for UART4/5

**Key Functions:**
- `Uart_Init()` - Initialize enabled UARTs
- `uart_send_byte/word/str/arr()` - Send data over specified UART
- `DGUS_ProcessAllUarts()` - Parse incoming frames from all UARTs
- `DGUS_MonitorAndSendUpdates()` - Monitor and send VP updates

### 3. Timer Library (`lib/timer/`)

System timing using Timer0, Timer1, and Timer2.

**Features:**
- 1ms system tick (Timer0)
- `delay_ms(ms)` - Blocking millisecond delay
- UART timeout counters
- RTC service timing

### 4. RTC Library (`lib/rtc/`)

Real-time clock support via I2C bit-banging.

**Supported Chips:**
- RX8130 (SELECT_RTC_TYPE=1)
- SD2058 (SELECT_RTC_TYPE=2)
- No RTC (SELECT_RTC_TYPE=0)

**Key Functions:**
- `Clock()` - Update DGUS VP with current time

### 5. CRC-16 Library (`lib/crc16/`)

CRC-16/MODBUS checksum implementation.

- Polynomial: 0xA001 (LSB-first)
- Initial value: 0xFFFF
- Function: `crc16table(ptr, len)`

---

## Configuration (`include/config.h`)

All project configuration is centralized in `config.h`:

| Macro | Default | Description |
|-------|---------|-------------|
| `SELECT_RTC_TYPE` | 0 | RTC chip selection (0=none, 1=RX8130, 2=SD2058) |
| `UART2_ENABLE` - `UART5_ENABLE` | 1 | Enable/disable each UART |
| `UARTx_RX_LENTH` | 1024 | RX buffer size per UART |
| `BAUD_UARTx` | 115200 | Baud rate per UART |
| `RESPONSE_UARTx` | 1 | Enable OK response (0x4F4B) |
| `DATA_UPLOAD_UARTx` | 1 | Enable auto-upload of VP changes |
| `USE_CRC` | 0 | Enable CRC-16 validation |

---

## DGUS VP Addresses (`include/addresses.h`)

Predefined VP addresses following the DGUS development guide:

| Macro | Address | Description |
|-------|---------|-------------|
| `PIC_Now_VP` | 0x0014 | Current page ID (read-only) |
| `PIC_Set_VP` | 0x0084 | Set page ID |
| `Curve_Data_VP` | 0x0310 | Curve buffer write start |
| `Curve_Ch0_VP` - `Curve_Ch7_VP` | 0x0301-0x030F | Clear curve channels |
| `NOR_FLASH_RW_VP` | 0x0008 | NOR Flash read/write |
| `System_Reset_VP` | 0x0004 | System reset (write 0x5A) |

---

## Flag Systems

### VP Flags (`src/app/vp_flags.h`)

Event-driven detection of DGUS VP changes. Monitors VPs: `0x2100`, `0x2102`, `0x2104`, `0x2106`, `0x3000`.

**Usage:**
```c
// In main loop
if (vp_start_button_flag) {
    vp_start_button_flag = 0;  // consume flag
    // handle event
}
```

### UART Flags (`src/app/uart_flags.h`)

Similar mechanism for UART-based events. Same monitored addresses as VP flags.

---

## Build System

### Requirements
- **SDCC** (Small Device C Compiler) - mcs51 target
- **make** - GNU Make
- **packihx** - Intel HEX packer (included with SDCC)
- **makebin** - Binary converter

### Commands

```bash
make        # Build firmware (output: build/dist/T5L51.bin)
make clean  # Remove build artifacts
```

### Build Output

| File | Description |
|------|-------------|
| `build/dist/T5L51.bin` | Final binary for SD card flashing |
| `build/dist/output.ihx` | Intel HEX format |
| `build/dist/output.hex` | Packed HEX format |
| `build/dist/output.map` | Linker map (memory usage) |

### Memory Layout

- **CODE**: 65536 bytes (flash)
- **XDATA**: 32768 bytes (external RAM, starts at 0x8000)
- **DATA**: 256 bytes (internal RAM)
- **IDATA**: 256 bytes (internal RAM, indirect)
- **BIT**: 256 bits (bit-addressable RAM)

---

## Main Execution Flow

```c
void main(void)
{
    Sys_Init();           // Initialize DGUS system interface
    Uart_Init();          // Initialize UARTs per config
    RTC_Service();        // Initialize RTC if enabled
    App_Init();           // User application initialization

    while (1)
    {
        DGUS_MonitorAndSendUpdates();  // Send VP updates to host
        DGUS_ProcessAllUarts();        // Parse incoming UART frames
    }
}
```

---

## DGUS Protocol

The firmware implements the DWIN DGUS II communication protocol:

**Frame Format:**
```
5A A5 [LEN] [CMD] [ADDR_H] [ADDR_L] [DATA...] [CRC_L] [CRC_H]
```

**Supported Commands:**
- `0x82` - Read VP request from host
- `0x83` - Write VP request from host

---

## Getting Started

1. Install SDCC: `sudo apt install sdcc` (Linux) or download from [sdcc.sourceforge.net](https://sdcc.sourceforge.net)
2. Clone the repository
3. Configure options in `include/config.h`
4. Define your VP addresses in `src/app/app_defs/`
5. Implement application logic in `src/app/app.c`
6. Build: `make`
7. Copy `build/dist/T5L51.bin` to SD card as `T5LCFG.BIN` or `DWIN_SET.bin`

---

## Key Files for Customization

| File | Purpose |
|------|---------|
| `include/config.h` | Enable/disable features, set baud rates |
| `src/app/app.c` | Main application logic |
| `src/app/app_defs/app_defs.c` | Application-specific VP definitions |
| `src/app/vp_flags.h` | Configure monitored VP addresses |
| `include/addresses.h` | System VP address definitions |

---

## References

- `doc/T5L_DGUSII_V2.9-0207.pdf` - DGUS II Development Guide
- `doc/T5L-ASIC-2024-09-25.pdf` - T5L ASIC Datasheet

---

## Version History

### [0.1.1] - 2025-10-21

**Fixed:**
- Incorrect LEN byte and CRC handling for `0x83` frames
- Resolved double-CRC issue aligned with DGUS standard
- Fixed packet structure from original DWIN template
