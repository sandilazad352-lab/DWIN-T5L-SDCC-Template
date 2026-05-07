/*
 * WiFi Module Driver Header
 * 
 * Support for WiFi modules via UART (e.g., ESP8266, ESP32)
 * Implements basic AT command interface
 */

#ifndef __WIFI_H__
#define __WIFI_H__

#include "t5l1.h"

/* WiFi Configuration */
#define WIFI_UART_PORT    3          // UART3 for WiFi (configurable)
#define WIFI_UART_BAUD    115200     // Standard WiFi module baud rate
#define WIFI_BUFFER_SIZE  256        // RX/TX buffer size
#define WIFI_TIMEOUT_MS   5000       // Command timeout
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64
#define WIFI_IP_MAX_LEN   16

/* WiFi States */
#define WIFI_STATE_OFFLINE       0
#define WIFI_STATE_CONNECTING    1
#define WIFI_STATE_CONNECTED     2
#define WIFI_STATE_ERROR         3
#define WIFI_STATE_INIT          4

/* WiFi Connection Modes */
#define WIFI_MODE_STATION    0  // Connect to access point
#define WIFI_MODE_SOFTAP     1  // Create access point
#define WIFI_MODE_BOTH       2  // Both station and AP

/* WiFi Event Types */
#define WIFI_EVENT_NONE          0
#define WIFI_EVENT_CONNECTED     1
#define WIFI_EVENT_DISCONNECTED  2
#define WIFI_EVENT_DATA_RX       3
#define WIFI_EVENT_ERROR         4

/* WiFi Control Structure */
typedef struct {
    u8 uart_port;           // UART port number
    u32 baud_rate;          // Baud rate
    u8 state;               // Current connection state
    u8 mode;                // Station or AP mode
    
    /* Connection info */
    u8 ssid[WIFI_SSID_MAX_LEN];
    u8 password[WIFI_PASS_MAX_LEN];
    u8 ip_address[WIFI_IP_MAX_LEN];
    u8 mac_address[18];     // MAC in AA:BB:CC:DD:EE:FF format
    
    /* Buffers */
    u8 rx_buffer[WIFI_BUFFER_SIZE];
    u8 tx_buffer[WIFI_BUFFER_SIZE];
    u8 rx_head, rx_tail;
    u8 tx_head, tx_tail;
    
    /* Statistics */
    u32 data_rx_count;
    u32 data_tx_count;
    u32 connection_attempts;
    u32 errors;
    
    /* Events */
    u8 pending_event;
    u8 rssi;  // Signal strength (-100 to 0, higher is better)
} wifi_module_t;

/* Global WiFi module */
extern wifi_module_t wifi_module;

/*
 * WiFi Driver Functions
 */

/**
 * Initialize WiFi module
 * @param uart_port: UART port (typically 3-5)
 * @param baud_rate: Communication speed (typically 115200)
 */
void wifi_init(u8 uart_port, u32 baud_rate);

/**
 * Get WiFi connection state
 * @return: WIFI_STATE_* value
 */
u8 wifi_get_state(void);

/**
 * Get state as string for debugging
 * @return: Pointer to state string ("OFFLINE", "CONNECTING", etc.)
 */
char *wifi_get_state_str(void);

/**
 * Connect to WiFi network (Station mode)
 * @param ssid: Network name
 * @param password: Network password
 * @param timeout_ms: Maximum time to wait for connection
 * @return: 1 if connected, 0 if timeout or error
 */
u8 wifi_connect(const char *ssid, const char *password, u16 timeout_ms);

/**
 * Disconnect from WiFi
 */
void wifi_disconnect(void);

/**
 * Check if currently connected
 * @return: 1 if connected, 0 otherwise
 */
u8 wifi_is_connected(void);

/**
 * Send AT command and wait for response
 * @param command: Command string (without CR/LF)
 * @param response: Buffer for response (can be NULL)
 * @param timeout_ms: Response timeout
 * @return: 1 if "OK" received, 0 otherwise
 */
u8 wifi_send_command(const char *command, u8 *response, u16 timeout_ms);

/**
 * Send TCP data to remote host
 * @param host: Destination IP or hostname
 * @param port: Destination port
 * @param data: Data buffer
 * @param length: Data length
 * @return: 1 if successful, 0 otherwise
 */
u8 wifi_send_tcp(const char *host, u16 port, u8 *data, u16 length);

/**
 * Send UDP data to remote host
 * @param host: Destination IP or hostname
 * @param port: Destination port
 * @param data: Data buffer
 * @param length: Data length
 * @return: 1 if successful, 0 otherwise
 */
u8 wifi_send_udp(const char *host, u16 port, u8 *data, u16 length);

/**
 * Get pending WiFi event
 * @return: WIFI_EVENT_* type (clears after reading)
 */
u8 wifi_get_event(void);

/**
 * Check if WiFi has pending event
 * @return: 1 if event pending, 0 otherwise
 */
u8 wifi_has_event(void);

/**
 * Process incoming WiFi data/responses
 * Call from UART ISR or main loop
 */
void wifi_process_data(void);

/**
 * Get signal strength
 * @return: RSSI value (-100 to 0, higher is better)
 */
u8 wifi_get_rssi(void);

/**
 * Get IP address
 * @param ip_buffer: Buffer for IP string (at least WIFI_IP_MAX_LEN bytes)
 * @return: 1 if available, 0 otherwise
 */
u8 wifi_get_ip_address(u8 *ip_buffer);

/**
 * Get MAC address
 * @param mac_buffer: Buffer for MAC address (at least 18 bytes)
 * @return: 1 if available, 0 otherwise
 */
u8 wifi_get_mac_address(u8 *mac_buffer);

/**
 * Reset WiFi module (hardware or software reset)
 */
void wifi_reset(void);

/**
 * Get statistics
 */
u32 wifi_get_rx_count(void);
u32 wifi_get_tx_count(void);
u32 wifi_get_error_count(void);

/**
 * Set WiFi module to low power mode
 */
void wifi_set_sleep_mode(u8 enable);

/**
 * Print connection status for debugging
 */
void wifi_print_status(void);

#endif // __WIFI_H__
