/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h> // Aggregate include for ASF drivers and services
#include "WIB_Init.h"
#include "can_app.h"
#include "tasks.h"
#include "encoder.h"
#include "encoder_gpio_test.h"

int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */
	/* Initialize TIB hardware */
	WIB_Init();
	
	/* Initialize CAN controller */
	if (!can_app_init()) {
		// CAN initialization failed - handle error
		while(1); // Stop execution if CAN fails
	}
	
	/* Test encoder initialization before starting tasks */
	encoder1_init();
	encoder1_enable(true);
	encoder1_simple_test();
	
	/* Run encoder pin toggle test for oscilloscope verification */
	/* This will toggle PA0, PA1, and PD17 in a specific pattern */
	/* Connect oscilloscope probes to these pins to verify soldering */
	encoder1_test_all_pins_sequence();
	
	/* Initialize GPIO-based encoder test for pulse counting */
	/* This will verify that encoder inputs are reaching the MCU */
	encoder_gpio_test_init();
	encoder_gpio_test_enable(true);
	
	/* Run GPIO encoder test for 10 seconds to count pulses */
	encoder_gpio_test_run_duration(10000);
	
	/* Create FreeRTOS tasks */
	create_application_tasks();
	
	/* Start FreeRTOS scheduler */
	vTaskStartScheduler();

	/* Should never reach here */
	while(1);
} // End of main
