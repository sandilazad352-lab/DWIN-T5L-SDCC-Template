/*
 * RFID Reader Driver Implementation
 */

#include "rfid.h"

/* Global RFID reader structure */
rfid_reader_t rfid_reader;

/* RFID enabled flag */
static u8 rfid_enabled = 1;

/**
 * Initialize RFID reader
 */
void rfid_init(u8 uart_port, u32 baud_rate) {
    // Initialize UART for RFID communication
    // Note: Configure the appropriate UART based on uart_port parameter
    
    rfid_reader.uart_port = uart_port;
    rfid_reader.baud_rate = baud_rate;
    rfid_reader.state = RFID_STATE_IDLE;
    rfid_reader.last_tag_found = 0;
    rfid_reader.tag_uid_len = 0;
    rfid_reader.rx_head = 0;
    rfid_reader.rx_tail = 0;
    rfid_reader.rx_complete = 0;
    rfid_reader.tags_read_count = 0;
    rfid_reader.read_errors = 0;
    rfid_reader.pending_event = RFID_EVENT_NONE;
}

/**
 * Get current state
 */
u8 rfid_get_state(void) {
    return rfid_reader.state;
}

/**
 * Get last read tag
 */
u8 rfid_get_last_tag(u8 *uid_buffer, u8 *uid_len) {
    if (rfid_reader.tag_uid_len == 0) {
        return 0;
    }
    
    u8 i;
    for (i = 0; i < rfid_reader.tag_uid_len; i++) {
        uid_buffer[i] = rfid_reader.tag_uid[i];
    }
    *uid_len = rfid_reader.tag_uid_len;
    
    return 1;
}

/**
 * Check if tag is present
 */
u8 rfid_tag_present(void) {
    return rfid_reader.last_tag_found;
}

/**
 * Get pending event
 */
u8 rfid_get_event(void) {
    u8 event = rfid_reader.pending_event;
    rfid_reader.pending_event = RFID_EVENT_NONE;
    return event;
}

/**
 * Check for pending event
 */
u8 rfid_has_event(void) {
    return rfid_reader.pending_event != RFID_EVENT_NONE;
}

/**
 * Calculate checksum (simple XOR of all data bytes)
 */
static u8 rfid_calculate_checksum(u8 *data, u8 len) {
    u8 i, checksum = 0;
    for (i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * Verify checksum
 */
u8 rfid_verify_checksum(u8 *data, u8 len) {
    if (len < 3) return 0;
    
    u8 calculated = rfid_calculate_checksum(data, len - 2);
    u8 received = (data[len - 2] << 4) | (data[len - 1] & 0x0F);
    
    return calculated == received;
}

/**
 * Parse EM18/EM4100 format: [STX][10 hex digits][2 checksum][ETX]
 */
static void rfid_parse_frame(u8 *frame, u8 frame_len) {
    if (frame_len < 14) {
        // Too short
        rfid_reader.read_errors++;
        rfid_reader.pending_event = RFID_EVENT_ERROR;
        return;
    }
    
    if (frame[0] != RFID_FRAME_START || frame[frame_len - 1] != RFID_FRAME_END) {
        // Invalid frame markers
        rfid_reader.read_errors++;
        rfid_reader.pending_event = RFID_EVENT_ERROR;
        return;
    }
    
    // Extract UID (10 hex characters)
    u8 i;
    for (i = 0; i < 10; i++) {
        u8 hex_char = frame[i + 1];
        if (hex_char >= '0' && hex_char <= '9') {
            rfid_reader.tag_uid[i] = hex_char - '0';
        } else if (hex_char >= 'A' && hex_char <= 'F') {
            rfid_reader.tag_uid[i] = hex_char - 'A' + 10;
        } else if (hex_char >= 'a' && hex_char <= 'f') {
            rfid_reader.tag_uid[i] = hex_char - 'a' + 10;
        } else {
            // Invalid character
            rfid_reader.read_errors++;
            rfid_reader.pending_event = RFID_EVENT_ERROR;
            return;
        }
    }
    
    rfid_reader.tag_uid_len = 10;
    rfid_reader.tags_read_count++;
    rfid_reader.tag_read_time = 0;  // In real code: system_ticks
    
    if (!rfid_reader.last_tag_found) {
        rfid_reader.pending_event = RFID_EVENT_TAG_READ;
        rfid_reader.last_tag_found = 1;
    }
    
    rfid_reader.state = RFID_STATE_TAG_FOUND;
}

/**
 * Process incoming RFID data
 */
void rfid_process_data(void) {
    if (!rfid_enabled) return;
    
    // In real implementation, process data from UART buffer
    // This is pseudocode showing the concept
    
    // Example: Check if complete frame received
    if (rfid_reader.rx_complete) {
        // Extract frame from buffer
        u8 frame[RFID_BUFFER_SIZE];
        u8 len = rfid_reader.rx_tail - rfid_reader.rx_head;
        
        if (len > 0 && len < RFID_BUFFER_SIZE) {
            u8 i;
            for (i = 0; i < len; i++) {
                frame[i] = rfid_reader.rx_buffer[(rfid_reader.rx_head + i) % RFID_BUFFER_SIZE];
            }
            
            // Parse frame
            rfid_parse_frame(frame, len);
        }
        
        rfid_reader.rx_complete = 0;
        rfid_reader.rx_head = rfid_reader.rx_tail;
    }
}

/**
 * Wait for tag (blocking)
 */
u8 rfid_wait_tag(u16 timeout_ms, u8 *uid_buffer, u8 *uid_len) {
    u32 start_time = 0;  // Use system_ticks in real code
    
    while (1) {
        rfid_process_data();
        
        if (rfid_get_event() == RFID_EVENT_TAG_READ) {
            return rfid_get_last_tag(uid_buffer, uid_len);
        }
        
        if (timeout_ms > 0 && (start_time++) > timeout_ms) {
            return 0;
        }
    }
}

/**
 * Compare two UIDs
 */
u8 rfid_uid_match(u8 *uid1, u8 len1, u8 *uid2, u8 len2) {
    if (len1 != len2) return 0;
    
    u8 i;
    for (i = 0; i < len1; i++) {
        if (uid1[i] != uid2[i]) return 0;
    }
    
    return 1;
}

/**
 * Print UID (user can add uart output here)
 */
void rfid_print_uid(u8 *uid, u8 uid_len) {
    // User-space function: implement UART output here if needed
}

/**
 * Get read count
 */
u32 rfid_get_read_count(void) {
    return rfid_reader.tags_read_count;
}

/**
 * Get error count
 */
u32 rfid_get_error_count(void) {
    return rfid_reader.read_errors;
}

/**
 * Reset reader
 */
void rfid_reset(void) {
    rfid_reader.state = RFID_STATE_IDLE;
    rfid_reader.last_tag_found = 0;
    rfid_reader.tag_uid_len = 0;
    rfid_reader.rx_head = 0;
    rfid_reader.rx_tail = 0;
    rfid_reader.rx_complete = 0;
    rfid_reader.pending_event = RFID_EVENT_NONE;
}

/**
 * Enable/disable RFID
 */
void rfid_set_enabled(u8 enable) {
    rfid_enabled = enable;
    
    if (!enable) {
        rfid_reset();
    }
}
