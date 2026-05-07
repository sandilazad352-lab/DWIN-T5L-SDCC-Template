/*
 * Arduino-Style Button Driver Implementation
 * Simple Digital I/O
 */

#include "button.h"

/**
 * Check if button is pressed
 * @param port: GPIO port (0-3)
 * @param pin: GPIO pin (0-7)
 * @param active_low: 1 if button press = LOW, 0 if button press = HIGH
 * @return: 1 if pressed, 0 if not pressed
 */
u8 isPressed(u8 port, u8 pin, u8 active_low) {
    u8 pin_state = digitalRead(port, pin);
    
    if (active_low) {
        // Button pressed when pin is LOW
        return (pin_state == LOW) ? 1 : 0;
    } else {
        // Button pressed when pin is HIGH
        return (pin_state == HIGH) ? 1 : 0;
    }
}
