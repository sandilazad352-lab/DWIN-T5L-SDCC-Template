/* -----------------------------------------------------------------------------
 *  Project : DWIN-T5L-SDCC-Template
 *  File    : main.c
 *  Version : 0.1.1
 *  Author  : Recep Şenbaş (https://github.com/recepsenbas)
 *  License : CC BY-NC-SA 4.0 (https://creativecommons.org/licenses/by-nc-sa/4.0/)
 *  Contact : recepsenbas@gmail.com
 *  Description : Main entry point for DWIN T5L firmware using SDCC.
 *                Initializes system, UART, and DGUS handling loop.
 * ----------------------------------------------------------------------------- */

#include "t5l1.h"
#include "uart.h"
#include "sys.h"
#include "timer.h"
#include "rtc.h"
#include "led.h"
#include "vp_flags.h"
#include "uart_flags.h"
#include "app_defs.h"
#include "app.c"

/* LED Blinking variables */
static u16 blink_counter = 0;
static const u16 BLINK_INTERVAL = 500; /* 500ms on/off */

/**
 * Blink LED - toggles LED state every BLINK_INTERVAL milliseconds
 * Uses P1.0 as LED pin (adjust port/pin as needed for your hardware)
 */
void Blink_LED(void)
{
    blink_counter++;
    if (blink_counter >= BLINK_INTERVAL)
    {
        blink_counter = 0;
        LED_TOGGLE(1, 0); /* Toggle LED on port P1, pin 0 */
    }
}

void main(void)
{
    Sys_Init();
    Uart_Init();
    RTC_Service();
    App_Init();
    
    /* Initialize LED pin as output */
    LED_OFF(1, 0); /* Set P1.0 to LOW initially */

    while (1)
    {
        /* Blink LED */
        Blink_LED();
        
        /* Monitor DGUS register and send updates */
        DGUS_MonitorAndSendUpdates();

        /* Check for incoming UART data and process it */
        DGUS_ProcessAllUarts();
    }
}
