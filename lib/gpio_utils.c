/*
 * Common GPIO Utilities
 * Shared digital I/O functions (used by LED and Button drivers)
 */

#include "t5l1.h"

#define HIGH 1
#define LOW  0

/**
 * Read GPIO pin state (Arduino-style)
 */
u8 digitalRead(u8 port, u8 pin) {
    u8 mask = 1 << pin;
    u8 value;
    
    switch (port) {
        case 0: value = P0 & mask; break;
        case 1: value = P1 & mask; break;
        case 2: value = P2 & mask; break;
        case 3: value = P3 & mask; break;
        default: return 0;
    }
    
    return value ? HIGH : LOW;
}

/**
 * Write GPIO pin state (Arduino-style)
 */
void digitalWrite(u8 port, u8 pin, u8 value) {
    u8 mask = 1 << pin;
    
    switch (port) {
        case 0:
            if (value) P0 |= mask;
            else       P0 &= ~mask;
            break;
        case 1:
            if (value) P1 |= mask;
            else       P1 &= ~mask;
            break;
        case 2:
            if (value) P2 |= mask;
            else       P2 &= ~mask;
            break;
        case 3:
            if (value) P3 |= mask;
            else       P3 &= ~mask;
            break;
    }
}

/**
 * Toggle pin state
 */
void digitalWrite_toggle(u8 port, u8 pin) {
    u8 mask = 1 << pin;
    
    switch (port) {
        case 0: P0 ^= mask; break;
        case 1: P1 ^= mask; break;
        case 2: P2 ^= mask; break;
        case 3: P3 ^= mask; break;
    }
}
