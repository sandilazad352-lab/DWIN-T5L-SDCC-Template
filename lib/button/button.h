/*
 * Arduino-Style Button Driver
 * Simple Digital I/O for Buttons
 */

#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "t5l1.h"

/* High and Low values */
#define HIGH 1
#define LOW  0

/**
 * Arduino-style digitalRead for buttons
 * @param port: GPIO port (0-3 for P0-P3)
 * @param pin: GPIO pin (0-7)
 * @return: HIGH (1) or LOW (0)
 * 
 * Usage:
 *   if (digitalRead(2, 0) == HIGH) { ... }  // Check if P2.0 is HIGH
 */
u8 digitalRead(u8 port, u8 pin);

/**
 * Check if button is pressed (simple version)
 * @param port: GPIO port (0-3)
 * @param pin: GPIO pin (0-7)
 * @param active_low: 1 if button press = LOW, 0 if button press = HIGH
 * @return: 1 if pressed, 0 if not pressed
 * 
 * Usage:
 *   if (isPressed(2, 0, 1)) { ... }  // P2.0 is active low button
 */
u8 isPressed(u8 port, u8 pin, u8 active_low);

#endif // __BUTTON_H__
