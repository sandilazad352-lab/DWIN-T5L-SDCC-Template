/*
 * Arduino-Style LED Driver
 * Simple Digital I/O for LEDs
 */

#ifndef __LED_H__
#define __LED_H__

#include "t5l1.h"

/* High and Low values */
#define HIGH 1
#define LOW  0

/**
 * Arduino-style digitalWrite for LEDs
 * @param port: GPIO port (0-3 for P0-P3)
 * @param pin: GPIO pin (0-7)
 * @param value: HIGH (1) or LOW (0)
 * 
 * Usage:
 *   digitalWrite(1, 0, HIGH);  // P1.0 = HIGH
 *   digitalWrite(1, 0, LOW);   // P1.0 = LOW
 */
void digitalWrite(u8 port, u8 pin, u8 value);

/**
 * Read pin state (Arduino-style)
 * @param port: GPIO port (0-3)
 * @param pin: GPIO pin (0-7)
 * @return: HIGH (1) or LOW (0)
 */
u8 digitalRead(u8 port, u8 pin);

/**
 * Toggle pin state
 * @param port: GPIO port (0-3)
 * @param pin: GPIO pin (0-7)
 */
void digitalWrite_toggle(u8 port, u8 pin);

/* Convenient macros */
#define LED_ON(port, pin)      digitalWrite(port, pin, HIGH)
#define LED_OFF(port, pin)     digitalWrite(port, pin, LOW)
#define LED_TOGGLE(port, pin)  digitalWrite_toggle(port, pin)

#endif // __LED_H__
