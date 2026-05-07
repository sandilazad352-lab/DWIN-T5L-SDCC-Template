/*
 * RFID Reader Driver Header
 * 
 * Support for RFID readers via UART communication
 * Typical readers: EM18, EM4100, RC522 via UART module
 */

#ifndef __RFID_H__
#define __RFID_H__

#include "t5l1.h"

/* RFID Protocol Configuration */
#define RFID_UART_PORT    2          // UART2 for RFID (configurable)
#define RFID_UART_BAUD    9600       // Standard RFID reader baud rate
#define RFID_MAX_UID_LEN  20         // Maximum UID length in bytes
#define RFID_BUFFER_SIZE  64         // RX buffer size
#define RFID_TIMEOUT_MS   1000       // Read timeout

/* RFID Frame Format (for EM18/EM4100 style readers) */
#define RFID_FRAME_START  0x02       // STX
#define RFID_FRAME_END    0x03       // ETX
#define RFID_UID_SIZE     10         // 10 hex digits = 5 bytes
#define RFID_CHECKSUM_SIZE 2         // Checksum field

/* RFID Reader States */
#define RFID_STATE_IDLE       0
#define RFID_STATE_READING    1
#define RFID_STATE_TAG_FOUND  2
#define RFID_STATE_ERROR      3

/* RFID Event Types */
#define RFID_EVENT_NONE       0
#define RFID_EVENT_TAG_READ   1
#define RFID_EVENT_TAG_LOST   2
#define RFID_EVENT_ERROR      3

/* RFID Control Structure */
typedef struct {
    u8 uart_port;           // UART port number
    u32 baud_rate;          // Baud rate (9600 typical)
    u8 state;               // Current state
    u8 last_tag_found;      // 1 if tag currently detected
    
    /* Current tag info */
    u8 tag_uid[RFID_MAX_UID_LEN];  // Tag UID in hex
    u8 tag_uid_len;                // Actual UID length
    u32 tag_read_time;             // When tag was read
    
    /* Reception buffer */
    u8 rx_buffer[RFID_BUFFER_SIZE];
    u8 rx_head, rx_tail;
    u8 rx_complete;
    
    /* Statistics */
    u32 tags_read_count;
    u32 read_errors;
    
    /* Events */
    u8 pending_event;
} rfid_reader_t;

/* Global RFID reader */
extern rfid_reader_t rfid_reader;

/*
 * RFID Driver Functions
 */

/**
 * Initialize RFID reader
 * @param uart_port: UART port to use (typically 2-5)
 * @param baud_rate: Communication speed (default 9600)
 */
void rfid_init(u8 uart_port, u32 baud_rate);

/**
 * Get current RFID reader state
 * @return: RFID_STATE_* value
 */
u8 rfid_get_state(void);

/**
 * Get last read tag UID
 * @param uid_buffer: Buffer to store UID (at least RFID_MAX_UID_LEN bytes)
 * @param uid_len: Pointer to store actual UID length
 * @return: 1 if valid tag available, 0 otherwise
 */
u8 rfid_get_last_tag(u8 *uid_buffer, u8 *uid_len);

/**
 * Check if tag is currently present (continuous reading)
 * @return: 1 if tag detected, 0 if not
 */
u8 rfid_tag_present(void);

/**
 * Get pending RFID event
 * @return: RFID_EVENT_* type (clears after reading)
 */
u8 rfid_get_event(void);

/**
 * Check if RFID has pending event
 * @return: 1 if event pending, 0 otherwise
 */
u8 rfid_has_event(void);

/**
 * Process incoming RFID data
 * Call from UART ISR or main loop
 */
void rfid_process_data(void);

/**
 * Wait for tag (blocking)
 * @param timeout_ms: Maximum wait time (0 = no timeout)
 * @param uid_buffer: Buffer for tag UID
 * @param uid_len: Pointer to store UID length
 * @return: 1 if tag read, 0 if timeout
 */
u8 rfid_wait_tag(u16 timeout_ms, u8 *uid_buffer, u8 *uid_len);

/**
 * Check if two UIDs match
 * @param uid1: First UID buffer
 * @param len1: First UID length
 * @param uid2: Second UID buffer
 * @param len2: Second UID length
 * @return: 1 if match, 0 otherwise
 */
u8 rfid_uid_match(u8 *uid1, u8 len1, u8 *uid2, u8 len2);

/**
 * Print tag UID to UART
 * @param uid: UID buffer
 * @param uid_len: UID length
 */
void rfid_print_uid(u8 *uid, u8 uid_len);

/**
 * Get statistics
 * @return: Number of tags successfully read
 */
u32 rfid_get_read_count(void);

/**
 * Get error count
 * @return: Number of read errors
 */
u32 rfid_get_error_count(void);

/**
 * Reset RFID reader (clear buffers, reinitialize)
 */
void rfid_reset(void);

/**
 * Enable/disable RFID reader
 * @param enable: 1 to enable, 0 to disable
 */
void rfid_set_enabled(u8 enable);

/**
 * Verify checksum of read tag
 * @param data: Tag data buffer
 * @param len: Data length
 * @return: 1 if checksum valid, 0 otherwise
 */
u8 rfid_verify_checksum(u8 *data, u8 len);

#endif // __RFID_H__
