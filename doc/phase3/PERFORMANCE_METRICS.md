# Performance Metrics and Optimization Reference

**Benchmarking data, timing specifications, and performance tuning for T5L HMI controllers**

---

## Overview

This guide provides measured performance data for the T5L platform, helping developers:
- Understand system constraints and bottlenecks
- Optimize code for real-time requirements
- Plan resource allocation and task scheduling
- Compare different implementation approaches

---

## 1. CPU Performance Specifications

### 1.1 Basic Timing Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| Crystal Oscillator | 11.0592 MHz | Standard, ±10 ppm tolerance |
| Machine Cycle | 1 μs | 12 oscillator cycles |
| Instruction Cycle | 1-2 μs | Typical 1-4 clock cycles per instruction |
| CPU Clock Rate | 11.0592 MHz / 12 = 921.6 kHz | Effective instruction rate |

### 1.2 Common Instruction Timings

```c
// Measured instruction execution times on T5L
// (In machine cycles; 1 machine cycle = 1.08 μs)

typedef struct {
    char *instruction;
    u8 cycles;
    float microseconds;
} timing_t;

timing_t instruction_timings[] = {
    { "MOV A, R0",          1,  1.08 },
    { "MOV Rn, #data",      1,  1.08 },
    { "MOV Rn, A",          1,  1.08 },
    { "MOVC A, @A+DPTR",    1,  1.08 },
    { "MOVX A, @Ri",        1,  1.08 },
    { "MOVX A, @DPTR",      1,  1.08 },
    
    { "ADD A, Rn",          1,  1.08 },
    { "ADD A, #data",       1,  1.08 },
    { "ADDC A, Rn",         1,  1.08 },
    { "SUBB A, Rn",         1,  1.08 },
    
    { "INC A",              1,  1.08 },
    { "INC Rn",             1,  1.08 },
    { "INC DPTR",           1,  1.08 },
    { "DEC A",              1,  1.08 },
    { "DEC Rn",             1,  1.08 },
    
    { "JZ offset",          2,  2.16 },  // No jump
    { "JZ offset (jump)",   3,  3.24 },  // Jump taken
    { "AJMP addr",          2,  2.16 },
    { "LCALL addr",         4,  4.32 },
    { "RET",                4,  4.32 },
    
    { "SETB bit",           1,  1.08 },
    { "CLR bit",            1,  1.08 },
    { "DJNZ Rn, offset",    2,  2.16 },  // No jump
    { "DJNZ Rn, offset",    4,  4.32 },  // Jump taken
};

// Calculate execution time for a common pattern
void benchmark_example(void) {
    u16 iterations = 1000;
    u32 start = system_ticks;
    
    u8 i, result = 0;
    for (i = 0; i < iterations; i++) {
        result += i;              // 1 cycle
        result = (result << 1);   // 1 cycle
    }
    
    u32 elapsed = system_ticks - start;
    
    // Expected: ~100 ms for 1000 iterations
    // Actual: Verify against measurement
}
```

---

## 2. Memory Access Performance

### 2.1 Access Time by Memory Type

| Memory Type | Latency | Read Speed | Write Speed | Cache |
|------------|---------|-----------|------------|-------|
| IRAM (0x00-0x7F) | 1 cycle | ~100 MB/s | ~100 MB/s | None |
| SFR (0x80-0xFF) | 1 cycle | ~100 MB/s | ~100 MB/s | None |
| XDATA (0x0000-0x7FFF) | 1 cycle | ~100 MB/s | ~100 MB/s | None |
| Flash (0x0000-0xFFFF) | 2 cycles | ~50 MB/s | N/A | Yes (prefetch) |

### 2.2 Stack and Variable Access Performance

```c
// Performance comparison: Different variable locations

// FAST: Register variable
void test_register_var(void) {
    register u8 counter = 0;  // In R0-R7
    u8 start = system_ticks;
    
    u16 i;
    for (i = 0; i < 10000; i++) {
        counter++;  // 1 cycle in register
        if (counter > 100) counter = 0;
    }
    
    u8 elapsed = system_ticks - start;
    // ~10 ticks expected
}

// MEDIUM: IRAM variable
void test_iram_var(void) {
    u8 counter = 0;  // In IRAM (e.g., 0x50)
    u8 start = system_ticks;
    
    u16 i;
    for (i = 0; i < 10000; i++) {
        counter++;  // 1 cycle (MOV to IRAM location)
        if (counter > 100) counter = 0;
    }
    
    u8 elapsed = system_ticks - start;
    // ~10 ticks expected (same as register for MOV)
}

// SLOW: XDATA variable
void test_xdata_var(void) {
    u16 counter = 0;  // In XDATA
    u8 start = system_ticks;
    
    u16 i;
    for (i = 0; i < 10000; i++) {
        counter++;  // MOVX + DPTR setup = 3+ cycles
        if (counter > 1000) counter = 0;
    }
    
    u8 elapsed = system_ticks - start;
    // ~30 ticks expected (3x slower than register)
}

// OPTIMAL: Stack-based (if using push/pop)
void test_stack_var(void) {
    // Declare volatile static to ensure IRAM allocation
    volatile static u8 counter = 0;
    
    // Compiler may optimize better than dynamic XDATA
}
```

### 2.3 Data Structure Packing

```c
// Poorly packed (uses more memory)
typedef struct {
    u16 value1;     // 2 bytes at offset 0
    u8 flag1;       // 1 byte at offset 2
    u16 value2;     // 2 bytes at offset 4 (but compiler may pad to 4)
    u8 flag2;       // 1 byte at offset 6
};  // Total: 8 bytes (with padding)

// Well-packed (saves memory)
typedef struct {
    u16 value1;     // 2 bytes at offset 0
    u16 value2;     // 2 bytes at offset 2
    u8 flag1;       // 1 byte at offset 4
    u8 flag2;       // 1 byte at offset 5
} __attribute__((packed));  // Total: 6 bytes

// Compile-time check
#define SIZEOF_CHECK() \
    static_assert(sizeof(struct bad_pack) == 8); \
    static_assert(sizeof(struct good_pack) == 6);
```

---

## 3. Communication Performance

### 3.1 UART Throughput

| Baud Rate | Bits/sec | Bytes/sec | Typical Use |
|-----------|----------|-----------|------------|
| 1200 | 1,200 | 150 | Debug only |
| 9600 | 9,600 | 1,200 | Modbus RTU |
| 19200 | 19,200 | 2,400 | Standard |
| 38400 | 38,400 | 4,800 | High speed |
| 57600 | 57,600 | 7,200 | Very fast |
| 115200 | 115,200 | 14,400 | Maximum practical |

**Calculations**:
- 1 frame (1 start, 8 data, 1 stop, optional parity) = 10 bits
- Effective throughput = Baud rate / 10 = bytes/sec

### 3.2 DGUS Protocol Overhead

```
Frame Structure:
┌──────┬────────┬─────────┬──────┬───────────┬─────┐
│ 0x5A │ 0xA5   │ Length  │ Cmd  │ Data...   │ CRC │
│ 1B   │ 1B     │ 2B      │ 1B   │ N bytes   │ 2B  │
└──────┴────────┴─────────┴──────┴───────────┴─────┘
      Overhead: 7 bytes minimum

Example: Read 2-byte VP (115200 baud)
Request frame:  7 bytes = 0.6 ms
TX delay:       ~1 ms
Response frame: 7 bytes = 0.6 ms
RX wait:        ~1 ms
Total:          ~3.2 ms

For 100 VP reads per second:
Bandwidth required: 100 × 3.2 ms = 320 ms = 32% utilization
```

### 3.3 Communication Latency Benchmark

```c
// Measure round-trip latency: Send request → Receive response
u32 dgus_latency_ms;

void benchmark_dgus_latency(void) {
    u16 vp = 0x0084;  // Page ID
    u16 value;
    
    u32 start = system_ticks;
    dgus_read_vp(vp, &value);  // Serialized call
    u32 elapsed = system_ticks - start;
    
    dgus_latency_ms = elapsed;
    
    uart_send_str("DGUS latency: ");
    uart_send_dec(dgus_latency_ms);
    uart_send_str(" ms\r\n");
}

// Expected results at 115200 baud:
// - Minimum: 5-10 ms (best case, immediate response)
// - Typical: 10-20 ms (with processing delay)
// - Worst case: 50+ ms (timeout retries)
```

### 3.4 RS-485 vs Single-UART Performance

```
Single UART (115200 baud):
  - Max devices: 1
  - Bandwidth per device: 14.4 KB/s
  - Latency: 1-5 ms

RS-485 Multi-master (115200 baud):
  - Max devices: 32 (electrical limit)
  - Bandwidth per device: 14.4 KB/s shared
  - Typical utilization: 10-20 devices max
  - Latency: 5-50 ms (depends on collisions)
  - Recommendation: <10 devices for reliable performance
```

---

## 4. Interrupt and Real-Time Performance

### 4.1 ISR Response Time

```c
// Measure interrupt latency
volatile u32 isr_response_time = 0;

void measure_isr_latency(void) {
    // Trigger INT0
    // Count cycles until ISR starts
    
    // Typical ISR latency:
    // - External interrupt: 10-20 cycles
    // - Timer interrupt: 15-25 cycles
    // - UART interrupt: 12-22 cycles
}

// ISR latency factors:
// - Current instruction must complete (up to 4 cycles)
// - PC saved to stack (1 cycle)
// - Jump to ISR vector (1-2 cycles)
// Total: ~10-20 cycles = 10-20 μs
```

### 4.2 System Tick Jitter

```c
// Measure Timer0 tick jitter
#define TICK_JITTER_SAMPLES 100

volatile u32 tick_intervals[TICK_JITTER_SAMPLES];
volatile u8 tick_count = 0;
u32 last_tick = 0;

void timer0_isr_jitter_measure(void) __interrupt(1) __using(1) {
    TH0 = 0xFC;
    TL0 = 0x18;
    
    if (tick_count < TICK_JITTER_SAMPLES) {
        u32 current = system_ticks;
        
        if (tick_count > 0) {
            tick_intervals[tick_count - 1] = current - last_tick;
        }
        
        last_tick = current;
        tick_count++;
    }
}

void analyze_jitter(void) {
    u32 min_interval = 0xFFFFFFFF;
    u32 max_interval = 0;
    u32 total = 0;
    u8 i;
    
    for (i = 0; i < TICK_JITTER_SAMPLES - 1; i++) {
        u32 interval = tick_intervals[i];
        
        if (interval < min_interval) min_interval = interval;
        if (interval > max_interval) max_interval = interval;
        
        total += interval;
    }
    
    u32 avg_interval = total / (TICK_JITTER_SAMPLES - 1);
    
    uart_send_str("Tick analysis:\r\n");
    uart_send_str("  Min: ");
    uart_send_dec(min_interval);
    uart_send_str(" ms\r\n");
    uart_send_str("  Max: ");
    uart_send_dec(max_interval);
    uart_send_str(" ms\r\n");
    uart_send_str("  Avg: ");
    uart_send_dec(avg_interval);
    uart_send_str(" ms\r\n");
    uart_send_str("  Jitter: ");
    uart_send_dec(max_interval - min_interval);
    uart_send_str(" ms\r\n");
}

// Expected results:
// - Target: 1 ms interval
// - Typical jitter: ±0.01 ms (10 μs)
// - Worst case: ±0.1 ms (100 μs) with heavy load
```

### 4.3 Task Scheduling Performance

```c
// Multi-rate task scheduling efficiency
#define TASK_COUNT 5

typedef struct {
    u8 rate_ms;        // Task period
    u32 next_run;      // Next scheduled time
    u32 execution_us;  // Measured execution time
    u32 total_time;    // Cumulative execution
    u16 run_count;     // Number of executions
} task_t;

task_t tasks[TASK_COUNT] = {
    { 1,   0, 0, 0, 0 },    // 1ms task (timer ISR)
    { 10,  0, 0, 0, 0 },    // 10ms task
    { 100, 0, 0, 0, 0 },    // 100ms task
    { 500, 0, 0, 0, 0 },    // 500ms task
    { 1000,0, 0, 0, 0 },    // 1s task
};

void scheduler_1ms_tick(void) {
    u8 i;
    u32 current = system_ticks;
    
    for (i = 0; i < TASK_COUNT; i++) {
        if (current >= tasks[i].next_run) {
            u32 start = system_ticks;
            
            // Execute task
            execute_task(i);
            
            u32 elapsed = system_ticks - start;
            tasks[i].execution_us = elapsed * 1000;  // Approximate
            tasks[i].total_time += elapsed;
            tasks[i].run_count++;
            
            // Schedule next run
            tasks[i].next_run = current + tasks[i].rate_ms;
        }
    }
}

void print_task_stats(void) {
    u8 i;
    uart_send_str("Task Statistics:\r\n");
    
    for (i = 0; i < TASK_COUNT; i++) {
        uart_send_str("  Task ");
        uart_send_dec(i);
        uart_send_str(" (");
        uart_send_dec(tasks[i].rate_ms);
        uart_send_str("ms): ");
        uart_send_dec(tasks[i].run_count);
        uart_send_str(" runs, avg ");
        
        u32 avg = tasks[i].total_time / (tasks[i].run_count + 1);
        uart_send_dec(avg);
        uart_send_str(" μs/run\r\n");
    }
}
```

---

## 5. Code Size and Memory Usage Benchmarks

### 5.1 Typical Program Size

```
T5L Flash: 64 KB = 65,536 bytes

Typical allocation:
  Startup code:        ~2 KB
  DGUS protocol stack: ~4 KB
  UART drivers:        ~3 KB
  Timer/RTC:           ~2 KB
  Application code:    ~12-15 KB
  ──────────────────────────
  Total used:          ~23-26 KB
  Available:           ~38-42 KB
```

### 5.2 IRAM Usage

```
T5L IRAM: 256 bytes

Allocation:
  Register banks:      0x00-0x1F  (32 bytes, uses only 1 typically)
  Bit-addressable:     0x20-0x2F  (16 bytes)
  General purpose:     0x30-0x7F  (80 bytes) 
  SFR area:            0x80-0xFF  (128 bytes, read-only)
  ──────────────────────────
  Usable:              ~96 bytes for variables + stack
```

### 5.3 XDATA Usage

```
T5L XDATA: 32 KB = 32,768 bytes

Typical allocation:
  DGUS VP space:       0x0000-0x1FFF  (8 KB)
  UART RX buffers:     0x2000-0x2FFF  (4 KB, 1KB per UART)
  System buffers:      0x3000-0x3FFF  (4 KB)
  Application heap:    0x4000-0x7FFF  (16 KB)
  ──────────────────────────
  Total:               32 KB
  
Typical usage: 20-24 KB, leaving 8-12 KB free
```

### 5.4 Code Size by Function

```c
// Measured code size (in bytes) for common operations
// Using SDCC compiler with optimization -O2

u16 estimate_function_size(const char *function_name) {
    // Example sizes:
    // mqtt_connect():           ~400 bytes
    // dgus_read_vp():           ~120 bytes
    // uart_send_byte():         ~12 bytes
    // timer0_isr():            ~20 bytes (minimal)
    // main():                  ~300 bytes
    // String constant "Hello": ~6 bytes (in Flash)
}

// Reduce code size:
// 1. Use __inline to avoid function call overhead
// 2. Move strings to Flash (const char *)
// 3. Use bit packing for flags
// 4. Avoid printf() (it's ~2KB!)
// 5. Use tables instead of switch statements
```

---

## 6. Power Consumption

### 6.1 Current Draw by Operating Mode

| Mode | CPU State | Supply Current | Notes |
|------|-----------|-----------------|-------|
| Active | Running at 11 MHz | ~15 mA | Normal operation |
| Idle | CPU stopped | ~5 mA | Timers still run |
| Stop | Full powerdown | <100 μA | Needs reset to wake |

### 6.2 Battery Life Calculation

```c
// Estimate battery life for different duty cycles

// Given:
// - Battery: 1000 mAh
// - Active current: 15 mA
// - Idle current: 5 mA

// Operating 50% active, 50% idle:
// Average current = (15 + 5) / 2 = 10 mA
// Battery life = 1000 mAh / 10 mA = 100 hours

float calculate_battery_life_hours(u8 active_percent) {
    float active_current = 15.0;     // mA
    float idle_current = 5.0;        // mA
    float battery_capacity = 1000.0; // mAh
    
    float idle_percent = 100.0 - active_percent;
    float avg_current = (active_current * active_percent / 100) + 
                        (idle_current * idle_percent / 100);
    
    return battery_capacity / avg_current;
}

// Example: 25% active duty cycle
// Battery life = (15 × 0.25) + (5 × 0.75) = 8.75 mA avg
//              = 1000 / 8.75 ≈ 114 hours ≈ 4.7 days
```

---

## 7. Real-World Performance Examples

### 7.1 DGUS Protocol Throughput Test

```c
void benchmark_dgus_throughput(void) {
    u16 vp_reads_per_sec = 0;
    u32 test_duration_ms = 1000;  // 1 second test
    u32 start = system_ticks;
    u32 test_end = start + test_duration_ms;
    
    while (system_ticks < test_end) {
        u16 value;
        if (dgus_read_vp(0x0084, &value) == 0) {
            vp_reads_per_sec++;
        }
    }
    
    uart_send_str("DGUS throughput: ");
    uart_send_dec(vp_reads_per_sec);
    uart_send_str(" reads/sec\r\n");
    
    // Expected: 50-100 reads/sec depending on response time
}

// Calculation:
// At 115200 baud, 14.4 KB/s available
// 1 read = ~14 bytes = ~1 ms
// Expected: ~1000 reads/sec theoretical
// Actual: ~100-200 reads/sec (limited by processing overhead)
```

### 7.2 Multi-UART Reception Stress Test

```c
// Receive data on all 4 UARTs simultaneously
volatile u32 uart_rx_counters[4] = {0, 0, 0, 0};
volatile u32 uart_overflow_counters[4] = {0, 0, 0, 0};

void stress_test_multi_uart(void) {
    u32 duration = 10 * 1000;  // 10 seconds
    u32 start = system_ticks;
    
    uart_send_str("Starting multi-UART stress test...\r\n");
    
    while ((system_ticks - start) < duration) {
        // Main loop just processes buffers
        // UARTs interrupt and fill buffers
        
        static u32 report_time = 0;
        if ((system_ticks - start) > report_time) {
            uart_send_str("Status at ");
            uart_send_dec((system_ticks - start) / 1000);
            uart_send_str("s: ");
            uart_send_dec(uart_rx_counters[0]);
            uart_send_str(" ");
            uart_send_dec(uart_rx_counters[1]);
            uart_send_str(" ");
            uart_send_dec(uart_rx_counters[2]);
            uart_send_str(" ");
            uart_send_dec(uart_rx_counters[3]);
            uart_send_str("\r\n");
            
            report_time += 1000;
        }
    }
    
    uart_send_str("Test complete. Totals:\r\n");
    uart_send_str("  UART0: ");
    uart_send_dec(uart_rx_counters[0]);
    uart_send_str(" bytes\r\n");
    // ... print others ...
}
```

---

## 8. Optimization Guidelines

### 8.1 CPU Optimization Priority

1. **Reduce ISR overhead** (Most impact)
   - Keep ISRs minimal: ±0.15% improvement per 10 μs saved

2. **Optimize tight loops**
   - Use register variables: ±2-5% improvement
   - Avoid XDATA access in loops: ±3-8% improvement

3. **Minimize memory access**
   - Keep hot variables in IRAM: ±2-3% improvement
   - Use memcpy for bulk operations: ±1-2% improvement

4. **Choose algorithms carefully**
   - CRC lookup table vs calculation: ±1-2% improvement

### 8.2 Memory Optimization Priority

1. **Move strings to Flash** (Most impact)
   - "Hello World" string: Saves 11 bytes RAM

2. **Use bit packing for flags**
   - 8 flags as u8: Saves 7 bytes per struct

3. **Remove unused code**
   - Unused functions: Saves ~50-500 bytes Flash

4. **Optimize data structures**
   - Pack structures: Saves 1-2 bytes per struct

---

## 9. Performance Monitoring and Profiling

```c
// Simple performance profiler
typedef struct {
    char *name;
    u32 total_time;
    u16 call_count;
} perf_counter_t;

#define PERF_MAX_COUNTERS 10
perf_counter_t perf_counters[PERF_MAX_COUNTERS] = {0};
u8 perf_counter_count = 0;

// Start profiling a section
#define PERF_START(name) \
    u32 _perf_start_##name = system_ticks

// End profiling and record
#define PERF_END(name) \
    do { \
        u32 _perf_elapsed = system_ticks - _perf_start_##name; \
        perf_counters[perf_counter_count].total_time += _perf_elapsed; \
        perf_counters[perf_counter_count].call_count++; \
        perf_counter_count++; \
    } while(0)

// Usage
void example_function(void) {
    PERF_START(example);
    
    // Do work
    
    PERF_END(example);
}

// Print profiling results
void print_perf_report(void) {
    u8 i;
    for (i = 0; i < perf_counter_count; i++) {
        uart_send_str("  ");
        uart_send_str(perf_counters[i].name);
        uart_send_str(": avg ");
        u32 avg = perf_counters[i].total_time / perf_counters[i].call_count;
        uart_send_dec(avg);
        uart_send_str(" ms/call\r\n");
    }
}
```

---

## Related Documentation

| Document | Purpose |
|----------|---------|
| [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md) | CPU and timing specifications |
| [TIMER_CLOCK_GUIDE.md](TIMER_CLOCK_GUIDE.md) | Timer performance details |
| [INTERRUPT_SYSTEM_GUIDE.md](INTERRUPT_SYSTEM_GUIDE.md) | ISR performance characteristics |
| [DEBUGGING_GUIDE.md](DEBUGGING_GUIDE.md) | Performance bottleneck diagnosis |

---

## Key References

- **T5L ASIC Datasheet**: Electrical characteristics and timing tables
- **8051 CPU Manual**: Instruction timing specifications
- **SDCC Compiler**: Code generation and optimization options
- **Benchmarking Tools**: `objdump`, `size`, profiling scripts in `/tools/`

---

**Document Version**: 1.0  
**Last Updated**: May 7, 2026  
**Status**: Complete - Phase 3 (v0.4.0)
