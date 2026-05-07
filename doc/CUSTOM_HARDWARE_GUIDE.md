# Custom Hardware Drivers Guide

**Implementation guide for custom hardware additions: LEDs, Buttons, RFID, and WiFi**

---

## Overview

This document describes the custom hardware driver libraries added to your T5L-based board design:

- **LED Driver** - GPIO-based LED control with blinking support
- **Button Driver** - Debounced button input with event detection
- **RFID Driver** - UART-based RFID reader communication
- **WiFi Driver** - UART-based WiFi module (ESP8266/ESP32) support

All drivers follow the existing SDCC/8051 project conventions and integrate with the core system.

---

## 1. LED Driver (`lib/led/`)

### Overview
Manages GPIO-connected LEDs with support for on/off/blink states.

### Configuration

#### 1.1 Configure LED Pins

Edit:
```c
// Example: Configure LEDs on Port 1
void setup_leds(void) {
    led_init();  // Initialize LED subsystem
    
    // Configure individual LEDs
    led_config(LED_POWER, 1, 0, 1, "Power");      // P1.0, active high
    led_config(LED_COMM, 1, 1, 1, "Comm");        // P1.1
    led_config(LED_RFID, 1, 2, 1, "RFID");        // P1.2
    led_config(LED_WIFI, 1, 3, 1, "WiFi");        // P1.3
    led_config(LED_ERROR, 1, 4, 1, "Error");      // P1.4
}
```

### Usage Examples

#### Turn LED On/Off
```c
led_on(LED_COMM);     // Turn communication LED on
led_off(LED_ERROR);   // Turn error LED off
```

#### Blinking Pattern
```c
// Set error LED to blink at 200ms rate
led_set_blink_rate(LED_ERROR, 200);
led_set_state(LED_ERROR, LED_BLINK);

// Set WiFi LED to slow blink when disconnected
if (!wifi_is_connected()) {
    led_set_blink_rate(LED_WIFI, 1000);
    led_set_state(LED_WIFI, LED_BLINK);
}
```

#### Update LED States (in Timer ISR)
```c
void timer0_isr(void) __interrupt(1) __using(1) {
    TH0 = 0xFC;
    TL0 = 0x18;
    
    system_ticks++;
    
    // Update LED blinking (every 1ms)
    led_update();
}
```

### Status Indication Patterns

```c
// Power up indicator
led_on(LED_POWER);

// During RFID reading
led_set_error(0);  // Clear error
led_set_rfid(1);   // Start RFID blink

// When RFID reading complete
led_set_rfid(0);   // Stop RFID LED

// Communication active
led_set_comm(1);   // On during transmission
led_set_comm(0);   // Off after done

// WiFi connection status
led_set_wifi(wifi_is_connected());  // Solid=connected, slow blink=disconnected

// Error condition
led_set_error(1);  // Rapid blink
```

---

## 2. Button Driver (`lib/button/`)

### Overview
Manages GPIO-connected buttons with debouncing and event detection.

### Configuration

#### 2.1 Configure Button Pins

```c
void setup_buttons(void) {
    button_init();  // Initialize button subsystem
    
    // Configure individual buttons
    button_config(BTN_START, 2, 0, 1, "Start");      // P2.0, active low
    button_config(BTN_STOP, 2, 1, 1, "Stop");        // P2.1
    button_config(BTN_RESET, 2, 2, 1, "Reset");      // P2.2
    button_config(BTN_CONFIG, 2, 3, 1, "Config");    // P2.3
    
    // Set debounce and hold detection
    button_set_debounce(BTN_START, 20);       // 20ms debounce
    button_set_hold_threshold(BTN_CONFIG, 2000);  // 2 second hold
}
```

### Usage Examples

#### Main Loop Button Polling

```c
int main(void) {
    setup_buttons();
    timer0_init_1ms();
    interrupts_enable_all();
    
    while (1) {
        // Scan buttons (call every 10-20ms)
        button_scan();
        
        // Check for button events
        if (button_has_event(BTN_START)) {
            u8 event = button_get_event(BTN_START);
            
            if (event == BTN_EVENT_PRESS) {
                // Start button pressed (short)
                start_operation();
            } else if (event == BTN_EVENT_HOLD) {
                // Start button held (> 1 second)
                emergency_stop();
            }
        }
        
        // Check other buttons...
        if (button_has_event(BTN_STOP)) {
            u8 event = button_get_event(BTN_STOP);
            if (event == BTN_EVENT_CLICK) {
                stop_operation();
            }
        }
    }
}
```

#### Blocking Button Wait

```c
// Wait for Reset button press (with 5 second timeout)
if (button_wait_press(BTN_RESET, 5000)) {
    system_reset();
}
```

#### Debugging Button Status

```c
// Print all button status
button_print_all_status();

// Output:
// Button 0 (Start): RELEASED - Event: 0
// Button 1 (Stop): PRESSED - Event: 1
// ...
```

### Button Event Types

| Event | Value | Meaning |
|-------|-------|---------|
| BTN_EVENT_NONE | 0 | No event |
| BTN_EVENT_PRESS | 1 | Button just pressed |
| BTN_EVENT_RELEASE | 2 | Button just released |
| BTN_EVENT_HOLD | 3 | Button held > threshold |
| BTN_EVENT_CLICK | 4 | Short press (< 1 sec) |

---

## 3. RFID Reader Driver (`lib/rfid/`)

### Overview
Manages UART-based RFID reader communication (EM18, EM4100 format).

### Hardware Connection

```
T5L MCU        RFID Reader
─────────────────────────
GND  ─────────  GND
P3.x (UART TX) ─ RX
P3.y (UART RX) ─ TX
VCC  ─────────  VCC (3.3V or 5V depending on reader)
```

### Configuration

#### 3.1 Initialize RFID Reader

```c
void setup_rfid(void) {
    // Initialize on UART2 at 9600 baud (typical RFID baud rate)
    rfid_init(2, 9600);
    
    // Or configure UART manually before calling rfid_init
    uart2_init(9600);
}
```

### Usage Examples

#### Simple Tag Reading

```c
int main(void) {
    setup_rfid();
    
    while (1) {
        // Process incoming RFID data
        rfid_process_data();
        
        // Check for new tag
        if (rfid_has_event()) {
            u8 event = rfid_get_event();
            
            if (event == RFID_EVENT_TAG_READ) {
                u8 uid[20];
                u8 uid_len;
                
                if (rfid_get_last_tag(uid, &uid_len)) {
                    uart_send_str("Tag found: ");
                    rfid_print_uid(uid, uid_len);
                    
                    // Check if this is an authorized tag
                    u8 authorized_tag[] = {1, 2, 3, 4, 5, 0xFF, 0xFE, 0xFD, 0xFC, 0xFB};
                    if (rfid_uid_match(uid, uid_len, authorized_tag, 10)) {
                        uart_send_str("Access granted!\r\n");
                    }
                }
            }
        }
    }
}
```

#### Blocking Tag Wait

```c
// Wait for RFID tag (5 second timeout)
u8 uid[20];
u8 uid_len;

if (rfid_wait_tag(5000, uid, &uid_len)) {
    uart_send_str("Tag read successfully\r\n");
    rfid_print_uid(uid, uid_len);
} else {
    uart_send_str("No tag detected (timeout)\r\n");
}
```

#### RFID Status and Statistics

```c
void print_rfid_status(void) {
    uart_send_str("RFID Reader Status:\r\n");
    uart_send_str("  State: ");
    
    switch (rfid_get_state()) {
        case RFID_STATE_IDLE:      uart_send_str("IDLE"); break;
        case RFID_STATE_READING:   uart_send_str("READING"); break;
        case RFID_STATE_TAG_FOUND: uart_send_str("TAG_FOUND"); break;
        case RFID_STATE_ERROR:     uart_send_str("ERROR"); break;
    }
    uart_send_str("\r\n");
    
    uart_send_str("  Tags read: ");
    uart_send_dec((u8)(rfid_get_read_count() & 0xFF));
    uart_send_str("\r\n");
    
    uart_send_str("  Errors: ");
    uart_send_dec((u8)(rfid_get_error_count() & 0xFF));
    uart_send_str("\r\n");
}
```

### RFID Frame Format (EM18/EM4100)

```
Frame: [STX][UID(10 hex)][Checksum(2)][ETX]
       0x02  DDEEFF1122    44          0x03

Example: 02 31 32 33 34 35 36 37 38 39 41 42 43 44 03
         (Tag UID: 1234567890ABCD)
```

---

## 4. WiFi Module Driver (`lib/wifi/`)

### Overview
Manages UART-based WiFi module (ESP8266/ESP32) via AT commands.

### Hardware Connection

```
T5L MCU        WiFi Module (ESP8266/ESP32)
────────────────────────────────
GND    ────────  GND
P3.x (UART TX) ── RX (with level shifter if needed)
P3.y (UART RX) ── TX (with level shifter if needed)
VCC    ────────  VCC (3.3V for ESP modules)
```

**Note**: Use a level shifter (3.3V ↔ 5V) if your T5L runs at 5V and WiFi module at 3.3V.

### Configuration

#### 4.1 Initialize WiFi Module

```c
void setup_wifi(void) {
    // Initialize on UART3 at 115200 baud
    wifi_init(3, 115200);
    
    // Then send initialization commands
    wifi_send_command("+RST", NULL, 1000);  // Reset module
    
    // Set to Station mode
    wifi_send_command("+CWMODE=1", NULL, 1000);
}
```

### Usage Examples

#### Connect to WiFi

```c
int main(void) {
    setup_wifi();
    
    // Connect to WiFi network (timeout 10 seconds)
    if (wifi_connect("MyNetwork", "password123", 10000)) {
        uart_send_str("WiFi connected!\r\n");
        
        // Print status
        wifi_print_status();
    } else {
        uart_send_str("Failed to connect\r\n");
    }
}
```

#### Send Data over TCP

```c
void send_sensor_data_tcp(u16 temperature) {
    u8 data[20];
    u16 len;
    
    // Format data: "TEMP=<value>"
    data[0] = 'T'; data[1] = 'E'; data[2] = 'M'; data[3] = 'P';
    data[4] = '=';
    len = 5;
    
    // Add temperature value (simplified)
    if (temperature >= 100) {
        data[len++] = '0' + (temperature / 100);
        temperature %= 100;
    }
    if (temperature >= 10) {
        data[len++] = '0' + (temperature / 10);
        temperature %= 10;
    }
    data[len++] = '0' + temperature;
    
    // Send to server
    if (wifi_send_tcp("192.168.1.100", 5000, data, len)) {
        uart_send_str("Data sent\r\n");
    } else {
        uart_send_str("Send failed\r\n");
    }
}
```

#### WiFi Status Monitoring

```c
void monitor_wifi_connection(void) {
    if (wifi_is_connected()) {
        led_on(LED_WIFI);  // WiFi connected LED on
        
        // Get current status
        u8 rssi = wifi_get_rssi();
        if (rssi < -80) {
            // Weak signal, warn user
            led_set_blink_rate(LED_WIFI, 500);
            led_set_state(LED_WIFI, LED_BLINK);
        } else {
            led_on(LED_WIFI);  // Strong signal, solid on
        }
    } else {
        led_set_blink_rate(LED_WIFI, 1000);
        led_set_state(LED_WIFI, LED_BLINK);  // Disconnected, slow blink
    }
}
```

### WiFi AT Commands Reference

| Command | Purpose | Example |
|---------|---------|---------|
| AT+RST | Reset module | AT+RST |
| AT+CWMODE | Set WiFi mode | AT+CWMODE=1 (station) |
| AT+CWJAP | Connect to AP | AT+CWJAP="SSID","PASSWORD" |
| AT+CWQAP | Disconnect | AT+CWQAP |
| AT+CIFSR | Get IP address | AT+CIFSR |
| AT+CIPSTART | Establish connection | AT+CIPSTART="TCP","192.168.1.1",80 |
| AT+CIPSEND | Send data | AT+CIPSEND=10 (then send 10 bytes) |
| AT+CWLAP | Scan networks | AT+CWLAP |

---

## 5. Integration with Main Application

### 5.1 Complete Initialization Example

```c
#include "../../lib/led/led.h"
#include "../../lib/button/button.h"
#include "../../lib/rfid/rfid.h"
#include "../../lib/wifi/wifi.h"

void hardware_init(void) {
    // Initialize core system
    Sys_Init();
    Uart_Init();
    timer0_init_1ms();
    
    // Initialize custom hardware
    setup_leds();
    setup_buttons();
    setup_rfid();
    setup_wifi();
    
    // Enable interrupts
    interrupts_enable_all();
    
    uart_send_str("All hardware initialized\r\n");
}

int main(void) {
    hardware_init();
    
    while (1) {
        // Main application loop
        
        // 1. Process buttons
        button_scan();
        if (button_has_event(BTN_START)) {
            // Handle button event
        }
        
        // 2. Process RFID
        rfid_process_data();
        if (rfid_has_event()) {
            // Handle RFID event
        }
        
        // 3. Process WiFi
        wifi_process_data();
        if (wifi_has_event()) {
            // Handle WiFi event
        }
        
        // 4. Update LED status
        // (LED update handled in Timer ISR)
        
        // 5. Do application work
        // ...
    }
}
```

### 5.2 Compilation

Add to your Makefile:

```makefile
# Custom hardware drivers
SOURCES += lib/led/led.c
SOURCES += lib/button/button.c
SOURCES += lib/rfid/rfid.c
SOURCES += lib/wifi/wifi.c

# Include paths
CFLAGS += -I./lib/led
CFLAGS += -I./lib/button
CFLAGS += -I./lib/rfid
CFLAGS += -I./lib/wifi
```

---

## 6. Pin Assignment Reference

Edit this based on your actual board design:

```
Port 0 (P0):
  - P0.0-P0.7: Available (LEDs 6-13 if needed)

Port 1 (P1):
  - P1.0-P1.4: LEDs (Power, Comm, RFID, WiFi, Error)
  - P1.5-P1.7: Reserved

Port 2 (P2):
  - P2.0-P2.3: Buttons (Start, Stop, Reset, Config)
  - P2.4-P2.7: Available

Port 3 (P3):
  - P3.0: RX (UART/Debug)
  - P3.1: TX (UART/Debug)
  - P3.2: INT0
  - P3.3: INT1
  - P3.4: T0 (Timer0)
  - P3.5: T1 (Timer1)
  - P3.6: /WR
  - P3.7: /RD

UART Assignments:
  - UART0 (P3.0/P3.1): Debug console
  - UART2 (P1.2/P1.3): RFID reader
  - UART3 (P1.4/P1.5): WiFi module
  - UART4 (P1.0/P1.1): RS-485 (if used)
```

---

## 7. Troubleshooting

### LED Issues

- **LED won't turn on**: Check active_high configuration, verify port/pin connections
- **LED always on**: Check if active_high is inverted for your circuit
- **Blinking erratic**: Ensure led_update() is called from 1ms timer ISR

### Button Issues

- **Button bouncing**: Increase debounce_delay_ms (try 20-50ms)
- **No button events**: Check button_scan() is called periodically
- **Hold detection not working**: Verify hold_threshold_ms is set and button_scan() is called frequently enough

### RFID Issues

- **No tag detection**: Check UART baud rate (9600 typical), verify RX/TX connections
- **Corrupted UID data**: Check signal integrity, may need capacitors near reader
- **Frame errors**: Verify frame start/end bytes (0x02/0x03), check checksum validation

### WiFi Issues

- **Won't connect to network**: Check SSID/password, verify WiFi module baud rate (115200)
- **No response to AT commands**: Check UART connection, may need level shifter
- **Slow data transmission**: May be limited by UART buffer size (increase WIFI_BUFFER_SIZE if needed)

---

## Related Documentation

| Document | Purpose |
|----------|---------|
| [HARDWARE_ARCHITECTURE.md](../phase1/HARDWARE_ARCHITECTURE.md) | GPIO and port configuration |
| [INTERRUPT_SYSTEM_GUIDE.md](../phase2/INTERRUPT_SYSTEM_GUIDE.md) | Timer ISR for LED updates |
| [PROTOCOL_EXAMPLES.md](../phase2/PROTOCOL_EXAMPLES.md) | UART communication patterns |

---

**Document Version**: 1.0  
**Last Updated**: May 7, 2026  
**Status**: Complete - Custom Hardware Implementation Guide
