/**
 * \file
 *
 * \brief Encoder GPIO Test Application
 *
 * This application tests encoder inputs using simple GPIO edge detection
 * to verify that encoder signals are reaching the MCU.
 * 
 * Pin Configuration:
 * - PA0: Encoder A input (GPIO input with interrupt)
 * - PA1: Encoder B input (GPIO input with interrupt)  
 * - PD17: Enable pin (GPIO output)
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
#include <asf.h>
#include "encoder_gpio_test.h"

int main (void)
{
    /* Insert system clock initialization code here (sysclk_init()). */
    sysclk_init();
    board_init();
    
    /* Initialize GPIO-based encoder test */
    if (!encoder_gpio_test_init()) {
        // Initialization failed - handle error
        while(1); // Stop execution if initialization fails
    }
    
    /* Run pin verification test first */
    encoder_gpio_test_pin_verification();
    
    /* Enable encoder and start continuous monitoring */
    encoder_gpio_test_enable(true);
    
    /* Run continuous test for encoder pulse counting */
    /* This will continuously monitor encoder inputs and count pulses */
    encoder_gpio_test_continuous();
    
    /* Should never reach here */
    while(1);
} // End of main