# T5L ASIC Features and Peripherals Guide

**Advanced peripheral configuration and T5L-specific enhancements**

---

## Overview

The T5L ASIC variant includes several enhancements beyond standard 8051:
- **RS-485 support** (differential serial with automatic direction control)
- **Multiple UARTs** (UART2-5 on dedicated pins)
- **Enhanced timer modes** (Timer2-3 with capture/compare)
- **Power management** (sleep modes, clock gating)
- **Integrated peripherals** (I2C optional, SPI optional)

This guide covers practical implementation of these features.

---

## 1. RS-485 Interface

### 1.1 RS-485 Hardware Architecture

RS-485 provides differential signaling for noise-immune long-distance communication:

```
┌─────────────────┐
│   T5L MCU       │
├─────────────────┤
│  UART4/UART5    │ ← DE (Driver Enable)
│  TXD/RXD        │ ← Transmit/Receive data
└────────┬────────┘
         │
    ┌────┴──────────────────┐
    │   RS-485 Transceiver  │
    │   (e.g., MAX485)      │
    ├────────────────────────┤
    │  A ─────────────────→  │ Differential pair (twisted)
    │  B ─────────────────→  │ Differential pair (twisted)
    │ GND ─────────────────→ │ Common ground
    └────────────────────────┘
           ↓
    Multi-drop bus (up to 32 nodes)
    See Fig. 1.1 below
```

**Topology**:
```
        ┌─ Node 1 (MCU1)
        │
Master ─┼─ Node 2 (MCU2)
Bus     │
        └─ Node 3 (Sensor)

Each node connects to same A/B pair with 120Ω termination
```

### 1.2 Pinout and Direction Control

**Standard T5L RS-485 Configuration**:

| Signal | Port | Function | Notes |
|--------|------|----------|-------|
| UART4_TXD | P1.0 | RS-485 Data Out | Drives A/B pair |
| UART4_RXD | P1.1 | RS-485 Data In | From A/B pair via receiver |
| DE (Driver Enable) | P3.x | RS-485 Direction | 1=Transmit, 0=Receive |
| RE (Receiver Enable) | P3.y | RS-485 Receiver | Active low (optional) |

### 1.3 RS-485 Driver Implementation

```c
// RS-485 configuration
#define RS485_DE_PIN    P3_4  // Driver Enable
#define RS485_RE_PIN    P3_5  // Receiver Enable

// Initialize RS-485 interface
void rs485_init(u32 baud_rate) {
    // Initialize UART4 with specified baud rate
    uart4_init(baud_rate);
    
    // Configure control pins as outputs
    P3_4 = 0;  // DE = Receive mode
    P3_5 = 1;  // RE = Enabled
    
    // Delay for transceiver settle
    delay_ms_busy(10);
}

// Enable transmitter (set to transmit mode)
void rs485_transmit_enable(void) {
    P3_4 = 1;  // DE high = Transmit
    P3_5 = 0;  // RE low = Receiver enabled during TX
    
    // Allow settling time
    {
        u8 i = 20;  // ~2μs for transceiver response
        while (i--) ;
    }
}

// Disable transmitter (set to receive mode)
void rs485_transmit_disable(void) {
    // Brief delay to ensure last bit transmitted
    delay_ms_busy(1);
    
    P3_4 = 0;  // DE low = Receive
    P3_5 = 1;  // RE high = Receiver disabled (optional)
    
    // Allow settling time
    {
        u8 i = 20;
        while (i--) ;
    }
}

// Send frame with automatic direction control
int rs485_send_frame(u8 *frame, u8 len) {
    u8 i;
    
    // Enable transmission
    rs485_transmit_enable();
    
    // Send all bytes
    for (i = 0; i < len; i++) {
        uart4_send_byte(frame[i]);
    }
    
    // Wait for transmission to complete (TI flag)
    // Delay for typical UART turnaround
    delay_ms(1);
    
    // Disable transmission
    rs485_transmit_disable();
    
    return 0;
}
```

### 1.4 Multi-Node Addressing

RS-485 allows multiple nodes on same bus. Use addressing protocol:

```c
// RS-485 Frame with addressing
typedef struct {
    u8 dest_addr;    // Destination node address
    u8 src_addr;     // Source node address
    u8 length;       // Data length
    u8 cmd;          // Command
    u8 data[32];     // Payload
    u16 crc;         // CRC-16
} rs485_frame_t;

// Node addressing scheme
#define RS485_BROADCAST  0xFF  // All nodes
#define RS485_MASTER     0x00  // Master controller
#define RS485_NODE1      0x01
#define RS485_NODE2      0x02
// ... etc

// Process received frame
void rs485_process_frame(rs485_frame_t *frame) {
    // Check if addressed to this node
    if (frame->dest_addr != local_node_addr && 
        frame->dest_addr != RS485_BROADCAST) {
        return;  // Not for us
    }
    
    // Process command
    switch (frame->cmd) {
        case 0x10:
            handle_read_command(frame);
            break;
        case 0x11:
            handle_write_command(frame);
            break;
    }
}

// Send addressed response
void rs485_send_response(u8 dest_addr, u8 *response, u8 len) {
    rs485_frame_t frame;
    
    frame.dest_addr = dest_addr;
    frame.src_addr = local_node_addr;
    frame.length = len;
    
    // Copy response data
    u8 i;
    for (i = 0; i < len; i++) {
        frame.data[i] = response[i];
    }
    
    // Calculate CRC (see DGUS_PROTOCOL_GUIDE.md)
    frame.crc = crc16_modbus((u8 *)&frame, len + 4);
    
    // Send
    rs485_send_frame((u8 *)&frame, len + 6);
}
```

### 1.5 Collision Detection and Arbitration

```c
// Simple bus arbitration: if transmission fails, retry with backoff
#define RS485_MAX_RETRIES  3
#define RS485_RETRY_BASE_MS  10

int rs485_send_with_retry(u8 *frame, u8 len) {
    u8 retry = 0;
    u32 backoff;
    
    while (retry < RS485_MAX_RETRIES) {
        // Send frame
        if (rs485_send_frame(frame, len) == 0) {
            return 0;  // Success
        }
        
        // Transmission failed (possible collision)
        retry++;
        
        // Exponential backoff: 10ms, 20ms, 40ms
        backoff = RS485_RETRY_BASE_MS << retry;
        delay_ms(backoff);
    }
    
    return -1;  // Failed after retries
}

// Check for collision by monitoring receive during transmit
// (Some RS-485 transducers support echo detection)
int rs485_detect_collision(void) {
    // While transmitting, if we receive data different from what we sent
    // then collision detected
    
    static u8 expected_byte = 0;
    u8 received_byte;
    
    // During transmission, compare what we sent vs what we receive
    if (uart4_receive_available()) {
        received_byte = uart4_receive_byte();
        
        if (received_byte != expected_byte) {
            uart_send_str("RS485 COLLISION DETECTED\r\n");
            return 1;
        }
    }
    
    return 0;
}
```

---

## 2. Multi-UART Configuration

### 2.1 UART Port Mapping (T5L Variant)

T5L provides up to 4 UARTs:

| UART | TXD | RXD | Typical Use | Baud |
|------|-----|-----|-------------|------|
| UART0 | P3.1 | P3.0 | Debug/Console | 115200 |
| UART1 | P3.3 | P3.2 | Reserved | - |
| UART2 | P1.2 | P1.3 | DGUS Protocol | 115200 |
| UART3 | P1.4 | P1.5 | Sensor/Modbus | 9600 |
| UART4 | P1.0 | P1.1 | RS-485 | Variable |
| UART5 | P1.6 | P1.7 | Extra | 115200 |

### 2.2 Initializing Multiple UARTs

```c
void multiserial_init(void) {
    // UART0 (Debug): 115200 baud
    uart0_init(115200);
    uart_send_str("System initialized\r\n");
    
    // UART2 (DGUS): 115200 baud, 8-bit packets
    uart2_init(115200);
    dgus_rx_head = dgus_rx_tail = 0;
    
    // UART3 (Sensor): 9600 baud, Modbus RTU
    uart3_init(9600);
    modbus_rx_head = modbus_rx_tail = 0;
    
    // UART4 (RS-485): 19200 baud with DE control
    rs485_init(19200);
    
    // Enable all UART interrupts
    ES0 = 1;  // UART0
    ES1 = 1;  // UART1 (if used)
    ES2 = 1;  // UART2
    ES3 = 1;  // UART3
}

// Demultiplexed ISR: Identify which UART interrupted
void uart_demux_isr(void) __interrupt(4) __using(2) {
    // Check UART0
    if (S0CON & 0x01) {
        S0CON &= ~0x01;
        uart0_rx_queue[uart0_tail++] = S0BUF;
    }
    
    // Could check other UARTs here
    // In practice, use separate ISR per UART
}

// Better: Separate ISR per UART
void uart0_isr(void) __interrupt(4) __using(1) {
    if (RI) {
        RI = 0;
        uart0_rx_queue[uart0_tail++] = SBUF;
    }
}

void uart2_isr(void) __interrupt(6) __using(1) {
    u8 byte;
    if (S2CON & 0x01) {
        S2CON &= ~0x01;
        byte = S2BUF;
        
        // Parse DGUS frame byte
        if (dgus_parse_byte(byte)) {
            dgus_frame_complete = 1;
        }
    }
}
```

### 2.3 Switching Active UART

```c
// Global for which UART is "active" for printf-style output
volatile u8 active_uart = UART0;

#define UART0  0
#define UART2  2
#define UART3  3
#define UART4  4

// Send character to active UART
void putchar_active(u8 c) {
    switch (active_uart) {
        case UART0:
            uart0_send_byte(c);
            break;
        case UART2:
            uart2_send_byte(c);
            break;
        case UART3:
            uart3_send_byte(c);
            break;
        case UART4:
            uart4_send_byte(c);
            break;
    }
}

// Switch debugging output to different UART
void debug_switch_uart(u8 uart_num) {
    active_uart = uart_num;
    putchar_active('\r');
    putchar_active('\n');
}

// Usage: Switch debug to RS-485 output
debug_switch_uart(UART4);
uart_send_str("Switched to RS-485 debug\r\n");
```

---

## 3. Power Management

### 3.1 Sleep Modes

The T5L supports several power-saving modes:

| Mode | CPU Clock | Timers | UART | Current | Wake Sources |
|------|-----------|--------|------|---------|------|
| Active | Full | Running | Active | ~15 mA | - |
| Idle | Stopped | Running | Active | ~5 mA | Timer, UART, INT0/INT1 |
| Stop | Off | Off | Standby | <100 μA | Reset only |

### 3.2 Idle Mode

```c
// Enter Idle Mode (CPU stops, timers continue)
void enter_idle_mode(void) {
    // Ensure important data saved
    // Interrupts still enabled to wake up
    
    PCON |= 0x01;  // Set IDLE bit
    
    // CPU halts here
    // ISR execution wakes CPU
    
    // Continue from here when interrupted
}

// Usage: Main loop waiting for event
int main(void) {
    timer0_init_1ms();
    interrupts_enable_all();
    
    watchdog_kick();
    
    while (1) {
        // Do some work
        process_background_tasks();
        
        // Nothing to do?
        if (!event_pending) {
            // Enter Idle mode to save power
            enter_idle_mode();
            
            // Wake up will occur on next interrupt
            // (Timer0 fires every 1ms)
        }
    }
}

// With watchdog (1 second timeout)
void enter_idle_with_watchdog(u32 max_idle_ms) {
    u32 sleep_start = system_ticks;
    
    while (1) {
        enter_idle_mode();
        
        // Check if awakened
        if (event_pending) {
            break;  // Got event
        }
        
        // Check timeout
        if ((system_ticks - sleep_start) > max_idle_ms) {
            break;  // Timeout exit
        }
        
        // Periodically kick watchdog (if used)
        watchdog_kick();
    }
}
```

### 3.3 Stop Mode (Deep Sleep)

```c
// Enter Stop Mode (CPU and oscillator stop)
void enter_stop_mode(void) {
    // CAUTION: Only wake via external reset or power cycle
    // Timer0 ISR will NOT fire!
    
    // Save critical state
    u8 ie_save = IE;
    u8 ip_save = IP;
    
    // Disable most interrupts (only external can wake)
    IE = 0x05;  // Enable INT0/INT1 only
    
    // Enter Stop mode
    PCON |= 0x02;  // Set STOP bit
    
    // CPU halts here
    // Only external interrupt (INT0/INT1) wakes it
    
    // Restore state when woken
    IE = ie_save;
    IP = ip_save;
}

// More practical: Use Idle mode instead of Stop
// Idle mode preserves timer operation and easier to debug
```

### 3.4 Clock Gating

```c
// Some T5L variants support peripheral clock gating
// to reduce power while keeping CPU active

#define ENABLE_TIMER_CLOCK   (PCON &= ~0x20)
#define DISABLE_TIMER_CLOCK  (PCON |= 0x20)

// Example: Disable Timer2 to save power
void sleep_timer2(void) {
    // Suspend Timer2 by gating its clock
    DISABLE_TIMER_CLOCK;
    TR2 = 0;  // Also disable
}

void wake_timer2(void) {
    ENABLE_TIMER_CLOCK;
    TR2 = 1;
}
```

---

## 4. Brownout and Power Reset Handling

### 4.1 Power Supply Monitoring

```c
// T5L may include brownout detection
// Monitor power supply voltage

#define BROWNOUT_THRESHOLD  4.5  // Volts

// Check power supply health
int check_power_supply(void) {
    // Method 1: Internal ADC (if available)
    // u16 vcc_mv = read_adc_vcc();
    
    // Method 2: External monitor via GPIO
    // u8 power_good = P3_6;  // Power good indicator
    
    // Method 3: Check PCON register
    u8 pcon = PCON;
    
    if (pcon & 0x40) {  // Power-on reset flag
        uart_send_str("Power-on detected\r\n");
        return 1;
    }
    
    return 0;
}

// Handle power failure gracefully
void power_monitor_init(void) {
    // Could enable external brownout interrupt
    // EX0 = 1;  // Enable INT0 for power good signal
    
    uart_send_str("Power monitoring enabled\r\n");
}

void power_fail_isr(void) __interrupt(0) __using(1) {
    // Power failure detected (INT0 from power monitor)
    
    // Save critical state to XDATA
    save_critical_state();
    
    // Possible actions:
    // 1. Wait for power restore
    // 2. Enter low-power mode
    // 3. Request display standby
    
    uart_send_str("POWER FAILURE - entering safe mode\r\n");
    
    // Enter Idle mode waiting for restore
    while (1) {
        enter_idle_mode();
    }
}
```

---

## 5. I2C Interface (Optional T5L Feature)

### 5.1 I2C Initialization

Some T5L variants include I2C for RTC and sensors:

```c
// I2C pins (if available)
#define I2C_SDA   P1.6  // Data line
#define I2C_SCL   P1.7  // Clock line

// Bit-bang I2C (simplest implementation)
void i2c_init(void) {
    // Set as inputs (open-drain)
    I2C_SDA = 1;
    I2C_SCL = 1;
    
    uart_send_str("I2C initialized\r\n");
}

// I2C Start condition
void i2c_start(void) {
    I2C_SDA = 1;
    I2C_SCL = 1;
    delay_ms_busy(2);
    
    I2C_SDA = 0;  // SDA low while SCL high = START
    delay_ms_busy(2);
    
    I2C_SCL = 0;  // Pull clock low
    delay_ms_busy(2);
}

// I2C Stop condition
void i2c_stop(void) {
    I2C_SDA = 0;
    delay_ms_busy(2);
    
    I2C_SCL = 1;  // SCL high while SDA low
    delay_ms_busy(2);
    
    I2C_SDA = 1;  // SDA high = STOP
    delay_ms_busy(2);
}

// I2C Byte transmission
int i2c_send_byte(u8 byte) {
    u8 bit;
    
    // Send 8 bits MSB first
    for (bit = 0; bit < 8; bit++) {
        if (byte & 0x80) {
            I2C_SDA = 1;
        } else {
            I2C_SDA = 0;
        }
        delay_ms_busy(1);
        
        // Clock pulse
        I2C_SCL = 1;
        delay_ms_busy(2);
        I2C_SCL = 0;
        delay_ms_busy(1);
        
        byte <<= 1;
    }
    
    // Read ACK bit
    I2C_SDA = 1;  // Release SDA
    delay_ms_busy(1);
    I2C_SCL = 1;
    delay_ms_busy(2);
    
    u8 ack = !I2C_SDA;  // ACK = 0, NAK = 1
    
    I2C_SCL = 0;
    delay_ms_busy(1);
    
    return ack;
}

// Use case: Read from RTC
void rtc_read_time(u8 *hour, u8 *minute, u8 *second) {
    // I2C address of RTC device
    #define RTC_ADDR  0x32  // RX8130 (example)
    
    u8 addr_byte = (RTC_ADDR << 1) | 0;  // Write address
    
    i2c_start();
    i2c_send_byte(addr_byte);
    i2c_send_byte(0x00);  // Register address (time)
    i2c_stop();
    
    // Read back
    i2c_start();
    addr_byte = (RTC_ADDR << 1) | 1;  // Read address
    i2c_send_byte(addr_byte);
    
    *second = i2c_read_byte();
    *minute = i2c_read_byte();
    *hour = i2c_read_byte();
    
    i2c_stop();
}
```

---

## 6. SPI Interface (Optional T5L Feature)

### 6.1 SPI Mode Configuration

Some T5L variants support SPI for high-speed peripheral communication:

```c
// SPI pins
#define SPI_CLK   P1.0  // Serial Clock
#define SPI_MOSI  P1.1  // Master Out
#define SPI_MISO  P1.2  // Master In
#define SPI_CS    P1.3  // Chip Select

void spi_init(void) {
    // Configure pins
    P1 = 0xF0;  // Set as I/O
    
    // SPI mode: CPOL=0, CPHA=0 (most common)
    //
}

// SPI byte transaction
u8 spi_transfer(u8 out_byte) {
    u8 in_byte = 0;
    u8 bit;
    
    for (bit = 0; bit < 8; bit++) {
        // Output bit
        if (out_byte & 0x80) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        
        // Clock pulse
        SPI_CLK = 1;
        delay_ms_busy(1);
        
        // Input bit
        in_byte <<= 1;
        in_byte |= SPI_MISO;
        
        SPI_CLK = 0;
        delay_ms_busy(1);
        
        out_byte <<= 1;
    }
    
    return in_byte;
}
```

---

## 7. Peripheral Troubleshooting

### 7.1 RS-485 Issues

```c
void diagnose_rs485(void) {
    uart_send_str("RS-485 Diagnostic:\r\n");
    
    // Check DE pin
    uart_send_str("DE pin: ");
    uart_send_dec(P3_4);
    uart_send_str("\r\n");
    
    // Check RE pin
    uart_send_str("RE pin: ");
    uart_send_dec(P3_5);
    uart_send_str("\r\n");
    
    // Test transmission
    uart_send_str("Sending test frame...\r\n");
    u8 test_frame[] = {0x5A, 0xA5, 0x00, 0x04, 0x82, 0x00, 0x84, 0x00, 0x00};
    
    rs485_send_frame(test_frame, 9);
    
    uart_send_str("Test complete\r\n");
}
```

### 7.2 Multi-UART Conflicts

```c
void diagnose_uarts(void) {
    uart_send_str("UART Status:\r\n");
    
    // Check SCON registers
    uart_send_str("S0CON: ");
    uart_send_hex(S0CON);
    uart_send_str("\r\n");
    
    uart_send_str("S2CON: ");
    uart_send_hex(S2CON);
    uart_send_str("\r\n");
    
    // Check IE register
    uart_send_str("IE: ");
    uart_send_hex(IE);
    uart_send_str("\r\n");
    
    // Check if any UART has pending data
    uart_send_str("Pending data - ");
    if (S0CON & 0x01) uart_send_str("UART0 ");
    if (S2CON & 0x01) uart_send_str("UART2 ");
    uart_send_str("\r\n");
}
```

---

## Related Documentation

| Document | Purpose |
|----------|---------|
| [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md) | SFR definitions for peripherals |
| [TIMER_CLOCK_GUIDE.md](TIMER_CLOCK_GUIDE.md) | Baud rate configuration |
| [INTERRUPT_SYSTEM_GUIDE.md](INTERRUPT_SYSTEM_GUIDE.md) | Multi-UART ISR patterns |
| [DEBUGGING_GUIDE.md](DEBUGGING_GUIDE.md) | Peripheral troubleshooting |

---

## Key References

- **T5L ASIC Datasheet**: Section 5 - RS-485 and Peripherals
- **DGUS II Development Guide**: Multi-UART communication
- See [lib/uart/uart.c](../lib/uart/uart.c) for driver implementations
- See [examples/examples.c](../examples/examples.c) for usage patterns

---

**Document Version**: 1.0  
**Last Updated**: May 7, 2026  
**Status**: Complete - Phase 3 (v0.4.0)
