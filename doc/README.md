# Documentation Index

**Quick reference guide to Phase 1 (v0.2.0) Documentation**

---

## 📚 Getting Started

Welcome to the DWIN-T5L-SDCC-Template documentation! This index helps you navigate the complete reference materials for T5L HMI development.

### New to T5L? Start Here
1. **[Hardware Architecture Guide](HARDWARE_ARCHITECTURE.md)** - Understand the processor, memory layout, and initialization
2. **[Memory Management Guide](MEMORY_MANAGEMENT.md)** - Learn how memory is organized and optimized
3. **[DGUS Protocol Guide](DGUS_PROTOCOL_GUIDE.md)** - Master communication with the display
4. **[VP Address Map](VP_ADDRESS_MAP.md)** - Complete reference of all accessible memory addresses

---

## 📖 Phase 1 Documentation

### Core Technical Guides

#### 1. **[HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md)**
   - **Topics**: T5L processor, SFR registers, memory organization, initialization sequence, interrupt system
   - **Key Sections**:
     - 1.1: Processor Architecture (CPU, clock generation)
     - 2.1: Complete Memory Map
     - 3.1: Essential SFRs for DGUS
     - 4.1: Pin Configuration and I/O Mapping
     - 5.1: Reset and Initialization Sequence
     - 6.1: UART Configuration and Baud Rate Calculation
     - 7.1: Interrupt Priority and Nesting
     - 8.1: Watchdog Timer Configuration
   - **When to Use**: Setting up the microcontroller, configuring timers and UARTs, understanding hardware details
   - **Audience**: Hardware engineers, firmware developers
   - **Time to Read**: 30-45 minutes

#### 2. **[DGUS_PROTOCOL_GUIDE.md](DGUS_PROTOCOL_GUIDE.md)**
   - **Topics**: Frame structure, command reference, VP addressing, CRC calculation, communication patterns
   - **Key Sections**:
     - 1.1: Protocol Overview
     - 2.1: Frame Structure (syntax, length calculation, CRC)
     - 3.1: Command 0x82 (Read VP)
     - 3.2: Command 0x83 (Write VP)
     - 4.1: VP Address Space Architecture
     - 5.1: Request/Response Cycles
     - 7: Practical Protocol Examples
     - 8: CRC-16/MODBUS Calculation
   - **When to Use**: Implementing communication code, debugging protocol issues, understanding frame format
   - **Audience**: Firmware developers, debugging specialists
   - **Time to Read**: 45-60 minutes

#### 3. **[MEMORY_MANAGEMENT.md](MEMORY_MANAGEMENT.md)**
   - **Topics**: IRAM/XDATA layout, buffer allocation, optimization strategies, memory bottlenecks
   - **Key Sections**:
     - 1.1: Memory Architecture Overview
     - 2.1: IRAM Organization and optimization
     - 3.1: XDATA Physical Organization
     - 3.3: UART RX Buffer Organization (circular buffers)
     - 4.1: Flash Memory Layout
     - 5.1: Memory Usage Patterns
     - 6.1: Advanced Topics (bank switching, malloc)
     - 9: Memory Bottlenecks & Solutions
   - **When to Use**: Allocating variables, optimizing code size, adding features
   - **Audience**: Firmware developers, system architects
   - **Time to Read**: 40-50 minutes

#### 4. **[VP_ADDRESS_MAP.md](VP_ADDRESS_MAP.md)**
   - **Topics**: Virtual Processor addressing, widget VPs, application VP space, address allocation
   - **Key Sections**:
     - 1.2: How VPs Work
     - 2: System VP Addresses (0x0000-0x00FF)
     - 3: DGUS Built-in Widget VPs (0x0100-0x0FFF)
     - 4: User Application VP Space (0x4000-0x7FFF)
     - 5: VP Access Patterns and Best Practices
     - 6: Multi-Page VP Organization
     - 7: VP Address Allocation Strategy
     - 8: Common VP Value Ranges and Encodings
   - **When to Use**: Designing display layouts, defining application variables, mapping VP addresses
   - **Audience**: Application developers, UI designers
   - **Time to Read**: 35-45 minutes

---

## 🔍 Quick Reference Cards

### Command Reference
- **Read VP (0x82)**: Request 4 bytes → Response 6 bytes
- **Write VP (0x83)**: Request 6 bytes → Response OK (optional)
- Complete details: [DGUS_PROTOCOL_GUIDE.md § 3.1-3.2](DGUS_PROTOCOL_GUIDE.md#31-command-0x82-read-vp-host--display)

### Memory Quick Lookup
- **IRAM**: 256 bytes total (80-100B typically free)
- **XDATA**: 32KB total (4.5KB fixed buffers, 27.5KB available)
- **Flash**: 64KB total (12KB code, 52KB available)
- Full details: [MEMORY_MANAGEMENT.md § 2-3](MEMORY_MANAGEMENT.md#2-internal-ram-iram---0x00-to-0xff)

### VP Address Ranges
- **System**: 0x0000-0x00FF (reserved)
- **Built-in**: 0x0100-0x0FFF (design tool allocated)
- **Application**: 0x4000-0x7FFF (user space)
- Full details: [VP_ADDRESS_MAP.md § 2-4](VP_ADDRESS_MAP.md#2-system-vp-addresses-0x0000-0x00ff)

### Hardware Quick Facts
- **CPU**: 8051 core, 11.0592 MHz oscillator
- **Baud Rates**: 115200 at 11.0592 MHz (TH1=0xFF with SMOD=1)
- **System Tick**: 1ms from Timer0 overflow
- Full details: [HARDWARE_ARCHITECTURE.md § 1-2](HARDWARE_ARCHITECTURE.md#1-processor-architecture)

---

## 📋 Topic Finder

### By Problem or Task

**I need to...**

**Configure Hardware**
- [ ] Set up oscillator → [HARDWARE_ARCHITECTURE.md § 1.2](HARDWARE_ARCHITECTURE.md#12-clock-generation)
- [ ] Initialize timers → [HARDWARE_ARCHITECTURE.md § 6.2](HARDWARE_ARCHITECTURE.md#62-uart2-5-configuration-registers)
- [ ] Configure UART baud rates → [HARDWARE_ARCHITECTURE.md § 6](HARDWARE_ARCHITECTURE.md#6-uart-configuration-and-baud-rate-calculation)
- [ ] Set up interrupts → [HARDWARE_ARCHITECTURE.md § 7](HARDWARE_ARCHITECTURE.md#7-interrupt-priority-and-nesting)

**Implement Communication**
- [ ] Read a VP value → [DGUS_PROTOCOL_GUIDE.md § 3.1](DGUS_PROTOCOL_GUIDE.md#31-command-0x82-read-vp-host--display)
- [ ] Write a VP value → [DGUS_PROTOCOL_GUIDE.md § 3.2](DGUS_PROTOCOL_GUIDE.md#32-command-0x83-write-vp-host--display)
- [ ] Calculate CRC → [DGUS_PROTOCOL_GUIDE.md § 8](DGUS_PROTOCOL_GUIDE.md#8-crc-16modbus-calculation-details)
- [ ] Handle frame errors → [DGUS_PROTOCOL_GUIDE.md § 6](DGUS_PROTOCOL_GUIDE.md#6-error-handling-and-edge-cases)

**Manage Memory**
- [ ] Allocate buffer in XDATA → [MEMORY_MANAGEMENT.md § 3.5](MEMORY_MANAGEMENT.md#35-application-data-space-0x3400-0x7fff)
- [ ] Optimize IRAM usage → [MEMORY_MANAGEMENT.md § 2.5](MEMORY_MANAGEMENT.md#25-optimizing-iram-usage)
- [ ] Understand UART buffers → [MEMORY_MANAGEMENT.md § 3.4](MEMORY_MANAGEMENT.md#34-circular-buffer-implementation)
- [ ] Reduce code size → [MEMORY_MANAGEMENT.md § 4.4](MEMORY_MANAGEMENT.md#44-reducing-code-size)

**Design Application**
- [ ] Allocate VP addresses → [VP_ADDRESS_MAP.md § 7](VP_ADDRESS_MAP.md#7-vp-address-allocation-strategy)
- [ ] Define sensor VP → [VP_ADDRESS_MAP.md § 4.2](VP_ADDRESS_MAP.md#42-application-vp-example-temperature-sensor)
- [ ] Implement multi-page UI → [VP_ADDRESS_MAP.md § 6](VP_ADDRESS_MAP.md#6-multi-page-vp-organization)
- [ ] Encode VP values → [VP_ADDRESS_MAP.md § 8](VP_ADDRESS_MAP.md#8-common-vp-value-ranges-and-encodings)

---

## 🎯 Common Tasks with Examples

### Task 1: Initialize a Sensor Display

**Requirements**: Read temperature, display on screen

**Steps**:
1. Define VP for temperature → [VP_ADDRESS_MAP.md § 4.2](VP_ADDRESS_MAP.md#42-application-vp-example-temperature-sensor)
2. Understand temperature encoding → [VP_ADDRESS_MAP.md § 8.1](VP_ADDRESS_MAP.md#81-temperature-encoding-example)
3. Read sensor value → Application code (see examples)
4. Send to display using 0x83 Write VP → [DGUS_PROTOCOL_GUIDE.md § 3.2](DGUS_PROTOCOL_GUIDE.md#32-command-0x83-write-vp-host--display)

**Related Documentation**:
- [DGUS_PROTOCOL_GUIDE.md § 5.1](DGUS_PROTOCOL_GUIDE.md#51-typical-requestresponse-cycle) - Protocol example
- [VP_ADDRESS_MAP.md § 8.1](VP_ADDRESS_MAP.md#81-temperature-encoding-example) - Value encoding

### Task 2: Handle User Button Press

**Requirements**: Detect user pressing "START" button, update display

**Steps**:
1. Enable auto-upload → [include/config.h](../include/config.h) (set `DATA_UPLOAD_UART=1`)
2. Define button VP → [VP_ADDRESS_MAP.md § 3.4](VP_ADDRESS_MAP.md#324-button-press-counters-0x0400-0x04ff)
3. Monitor auto-upload data → [DGUS_PROTOCOL_GUIDE.md § 5.2](DGUS_PROTOCOL_GUIDE.md#52-auto-upload-mode-display--host)
4. Process button frame → Application code checks VP counter increment
5. Update display status → Send response via 0x83 Write VP

**Related Documentation**:
- [DGUS_PROTOCOL_GUIDE.md § 5.2](DGUS_PROTOCOL_GUIDE.md#52-auto-upload-mode-display--host) - Auto-upload pattern
- [VP_ADDRESS_MAP.md § 5.4](VP_ADDRESS_MAP.md#54-vp-polling-vs-auto-upload) - Polling vs. auto-upload comparison

### Task 3: Debug Communication Issues

**Symptoms**: Display not responding, frames getting corrupted

**Debugging Steps**:
1. Verify baud rate → [HARDWARE_ARCHITECTURE.md § 6.1-6.2](HARDWARE_ARCHITECTURE.md#61-uart-baud-rate-formula)
2. Check frame structure → [DGUS_PROTOCOL_GUIDE.md § 2.1](DGUS_PROTOCOL_GUIDE.md#21-general-frame-format)
3. Validate CRC if enabled → [DGUS_PROTOCOL_GUIDE.md § 8](DGUS_PROTOCOL_GUIDE.md#8-crc-16modbus-calculation-details)
4. Monitor VP access logging → [VP_ADDRESS_MAP.md § 11.1](VP_ADDRESS_MAP.md#111-vp-readwrite-logging)
5. Check buffer overflow → [MEMORY_MANAGEMENT.md § 5.2](MEMORY_MANAGEMENT.md#52-memory-stress-points)

**Related Documentation**:
- [DGUS_PROTOCOL_GUIDE.md § 6](DGUS_PROTOCOL_GUIDE.md#6-error-handling-and-edge-cases) - Troubleshooting guide
- [MEMORY_MANAGEMENT.md § 3.4](MEMORY_MANAGEMENT.md#34-circular-buffer-implementation) - Buffer implementation

---

## 📚 Reading Paths by Role

### For Embedded Systems Engineers
1. **[HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md)** (complete read)
   - Sections: 1, 2, 3, 5, 6, 7
   - Focus: Processor details, memory layout, register configuration

2. **[MEMORY_MANAGEMENT.md](MEMORY_MANAGEMENT.md)** (complete read)
   - Sections: 1-6, 9
   - Focus: IRAM/XDATA layout, optimization, debugging

3. **[DGUS_PROTOCOL_GUIDE.md](DGUS_PROTOCOL_GUIDE.md)** (sections 1-4)
   - Focus: Protocol overview, frame structure, addressing

### For Firmware Developers
1. **[DGUS_PROTOCOL_GUIDE.md](DGUS_PROTOCOL_GUIDE.md)** (complete read)
   - Focus: Implementation details, examples, CRC

2. **[VP_ADDRESS_MAP.md](VP_ADDRESS_MAP.md)** (sections 1-5, 8)
   - Focus: VP allocation, access patterns, encodings

3. **[HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md)** (sections 2, 6, 7)
   - Focus: Memory map, baud rates, interrupts

4. **[MEMORY_MANAGEMENT.md](MEMORY_MANAGEMENT.md)** (sections 2-3, 5-6)
   - Focus: Buffer allocation, optimization

### For Application Development
1. **[VP_ADDRESS_MAP.md](VP_ADDRESS_MAP.md)** (complete read)
   - Focus: VP allocation, multi-page design, encodings

2. **[DGUS_PROTOCOL_GUIDE.md](DGUS_PROTOCOL_GUIDE.md)** (sections 5-7)
   - Focus: Communication patterns, practical examples

3. **[MEMORY_MANAGEMENT.md](MEMORY_MANAGEMENT.md)** (sections 3.5)
   - Focus: Application space allocation

### For System Architects / Project Managers
1. **[HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md)** (sections 1, 2.1)
   - Focus: System overview, memory capacity

2. **[memory_MANAGEMENT.md](MEMORY_MANAGEMENT.md)** (sections 1, 5)
   - Focus: Resource bottlenecks, stress points

3. **[VP_ADDRESS_MAP.md](VP_ADDRESS_MAP.md)** (sections 7)
   - Focus: Scalability, address space planning

---

## � Phase 2 Documentation (v0.3.0) - Practical Guides

Practical implementation examples, debugging techniques, and system configuration.

### [PROTOCOL_EXAMPLES.md](PROTOCOL_EXAMPLES.md)
Real-world DGUS protocol communication examples with working C code and hex frame analysis.

**Includes**:
- 7 complete communication scenarios
- Hex frame dumps and byte-by-byte analysis
- Working C code implementations
- Timing analysis and optimization tips
- CRC validation procedures
- Quick reference patterns for common operations

### [DEBUGGING_GUIDE.md](DEBUGGING_GUIDE.md)
Systematic troubleshooting and diagnostic procedures for T5L applications.

**Includes**:
- 8 major issue categories with diagnostics
- Communication issues, memory problems, hardware faults
- Display interface and state machine debugging
- Performance analysis tools
- Root cause analysis flowcharts
- Diagnostic code snippets and utilities

### [TIMER_CLOCK_GUIDE.md](TIMER_CLOCK_GUIDE.md)
Complete timer system setup and optimization for real-time applications.

**Includes**:
- Oscillator configuration and crystal specifications
- Timer0-3 setup with working code examples
- 1ms system tick generation and calibration
- Baud rate calculations for different frequencies
- Multi-rate scheduling patterns
- PWM generation and watchdog implementation
- Timer frequency troubleshooting

### [INTERRUPT_SYSTEM_GUIDE.md](INTERRUPT_SYSTEM_GUIDE.md)
Interrupt handling architecture and ISR best practices.

**Includes**:
- 8051 interrupt vector table and T5L enhancements
- IE/IP register configuration
- Register bank preservation strategies
- Critical section protection and atomic operations
- Multi-UART ISR patterns
- Interrupt priority conflicts and resolution
- ISR latency measurement and optimization
- Debugging interrupt issues

---

## 📖 Phase 3 Documentation (v0.4.0) - Advanced Topics

Advanced peripheral configuration and performance optimization.

### [ASIC_FEATURES_GUIDE.md](ASIC_FEATURES_GUIDE.md)
Practical guide to T5L-specific ASIC enhancements and peripheral configuration.

**Includes**:
- RS-485 interface architecture and multi-node addressing
- RS-485 driver implementation with collision detection
- Multi-UART configuration and demultiplexing
- Power management modes (Idle, Stop, clock gating)
- Brownout detection and power failure handling
- I2C and SPI optional peripherals
- Peripheral troubleshooting and diagnostics

### [PERFORMANCE_METRICS.md](PERFORMANCE_METRICS.md)
Comprehensive benchmarking data and performance tuning reference.

**Includes**:
- CPU performance specifications and instruction timings
- Memory access latency by storage type
- UART and DGUS protocol throughput metrics
- ISR response time and system tick jitter analysis
- Code size and memory usage benchmarks
- Power consumption and battery life calculations
- Real-world performance testing examples
- Optimization guidelines with priority ranking
- Performance monitoring and profiling tools

---

## �🔗 Cross-References

### By Topic

**Memory Organization**
- IRAM: [HARDWARE_ARCHITECTURE.md § 2.1](HARDWARE_ARCHITECTURE.md#21-iram-organization), [MEMORY_MANAGEMENT.md § 2](MEMORY_MANAGEMENT.md#2-internal-ram-iram---0x00-to-0xff)
- XDATA: [HARDWARE_ARCHITECTURE.md § 2.3](HARDWARE_ARCHITECTURE.md#23-xdata-layout-for-this-project), [MEMORY_MANAGEMENT.md § 3](MEMORY_MANAGEMENT.md#3-external-data-ram-xdata---0x0000-to-0x7fff)
- Flash: [HARDWARE_ARCHITECTURE.md § 2.4](HARDWARE_ARCHITECTURE.md#24-flash-rom-organization), [MEMORY_MANAGEMENT.md § 4](MEMORY_MANAGEMENT.md#4-flash-memory-program-memory)

**Communication Protocol**
- Frame Format: [DGUS_PROTOCOL_GUIDE.md § 2](DGUS_PROTOCOL_GUIDE.md#2-frame-structure)
- Commands: [DGUS_PROTOCOL_GUIDE.md § 3](DGUS_PROTOCOL_GUIDE.md#3-command-set-reference)
- Examples: [DGUS_PROTOCOL_GUIDE.md § 7](DGUS_PROTOCOL_GUIDE.md#7-practical-protocol-examples)

**Virtual Processor (VP) Addressing**
- Overview: [VP_ADDRESS_MAP.md § 1](VP_ADDRESS_MAP.md#overview)
- System VPs: [VP_ADDRESS_MAP.md § 2](VP_ADDRESS_MAP.md#2-system-vp-addresses-0x0000-0x00ff)
- Application VPs: [VP_ADDRESS_MAP.md § 4](VP_ADDRESS_MAP.md#4-user-application-vp-space-0x4000-0x7fff)

**Hardware Control**
- Registers: [HARDWARE_ARCHITECTURE.md § 3](HARDWARE_ARCHITECTURE.md#3-special-function-registers-sfr)
- Clock: [HARDWARE_ARCHITECTURE.md § 1.2](HARDWARE_ARCHITECTURE.md#12-clock-generation), [HARDWARE_ARCHITECTURE.md § 6](HARDWARE_ARCHITECTURE.md#6-uart-configuration-and-baud-rate-calculation)
- Interrupts: [HARDWARE_ARCHITECTURE.md § 7](HARDWARE_ARCHITECTURE.md#7-interrupt-priority-and-nesting)

---

## 📞 Getting Help

### Troubleshooting Guide
- **Communication Issues** → [DGUS_PROTOCOL_GUIDE.md § 11](DGUS_PROTOCOL_GUIDE.md#11-troubleshooting-protocol-issues)
- **Memory Problems** → [MEMORY_MANAGEMENT.md § 9](MEMORY_MANAGEMENT.md#9-memory-bottlenecks--solutions)
- **Hardware Configuration** → [HARDWARE_ARCHITECTURE.md § 10](HARDWARE_ARCHITECTURE.md#10-verification-checklist)
- **VP Address Conflicts** → [VP_ADDRESS_MAP.md § 9](VP_ADDRESS_MAP.md#9-vp-address-conflicts-and-resolution)

### Related Project Files
- Configuration: [include/config.h](../include/config.h)
- Addresses: [include/addresses.h](../include/addresses.h)
- Examples: [examples/examples.c](../examples/examples.c)
- Startup: [startup/startup_T5L.s](../startup/startup_T5L.s)

---

## 📝 Document Information

| Document | Version | Date | Status |
|----------|---------|------|--------|
| HARDWARE_ARCHITECTURE.md | 1.0 | May 7, 2026 | Phase 1 ✓ |
| DGUS_PROTOCOL_GUIDE.md | 1.0 | May 7, 2026 | Phase 1 ✓ |
| MEMORY_MANAGEMENT.md | 1.0 | May 7, 2026 | Phase 1 ✓ |
| VP_ADDRESS_MAP.md | 1.0 | May 7, 2026 | Phase 1 ✓ |
| README.md (this file) | 1.0 | May 7, 2026 | Phase 1 ✓ |
| PROTOCOL_EXAMPLES.md | 1.0 | May 7, 2026 | Phase 2 ✓ |
| DEBUGGING_GUIDE.md | 1.0 | May 7, 2026 | Phase 2 ✓ |
| TIMER_CLOCK_GUIDE.md | 1.0 | May 7, 2026 | Phase 2 ✓ |
| INTERRUPT_SYSTEM_GUIDE.md | 1.0 | May 7, 2026 | Phase 2 ✓ |
| ASIC_FEATURES_GUIDE.md | 1.0 | May 7, 2026 | Phase 3 ✓ |
| PERFORMANCE_METRICS.md | 1.0 | May 7, 2026 | Phase 3 ✓ |

---

## 📖 How to Use This Documentation

1. **First Time?** → Start with "Getting Started" section above
2. **Looking for specific info?** → Use "Topic Finder" or "Quick Reference Cards"
3. **Working on a task?** → Check "Common Tasks with Examples"
4. **Stuck?** → Look in "Troubleshooting Guide" or use cross-references
5. **Want to go deep?** → Follow your role's "Reading Path"

---

**Last Updated**: May 7, 2026  
**Phase**: 3 (v0.4.0) - Complete Documentation Suite  
**Status**: All Phases Complete (Phase 1, 2, 3) - Ready for Production

For contribution or feedback, please refer to the project's GitHub repository.
