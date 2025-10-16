/*
 * CFile1.c
 *
 * Created: 8/21/2025 10:12:19 AM
 *  Author: MKumar
 */ 
#include "TIB_Init.h"
#include "i2c0.h"
#include "lis2dh.h"
int tool_type = 9;
unsigned char who_lis2 = 0;
int TIB_Init()
{
	SystemInit();
	board_init();
	/* Replace with your application code */
	spi0_init(1000000, false);   // 1 MHz, MSB-first (lsbfirst=false)
								//initialise SPI0 interface for Load Cell ADS1120
	//toolsense_init();			//Initialise GPIOs for sensing tool type
	
	// Debug: Print SystemCoreClock value
	// You can check this value in debugger or via UART
	volatile uint32_t debug_clock = SystemCoreClock;
	
	// Initialize I2C0 at 100kHz
	if (i2c0_init() != I2C_SUCCESS) {
		// I2C initialization failed
		volatile uint32_t debug_i2c_init_failed = 1;
		return -1;
	}
	
	// Initialize LIS2DH accelerometer and temperature sensor
	if (!lis2dh_init()) {
		// LIS2DH initialization failed
		volatile uint32_t debug_lis2dh_init_failed = 1;
		return -1;
	}
	
	// Verify LIS2DH connection
	if (!lis2dh_verify_connection()) {
		// LIS2DH connection verification failed
		volatile uint32_t debug_lis2dh_verify_failed = 1;
		return -1;
	}
	
	// Configure LIS2DH for optimal performance
	lis2dh_set_full_scale(LIS2DH_FS_2G);        // ±2g range
	lis2dh_set_output_data_rate(LIS2DH_ODR_100HZ); // 100Hz sampling rate
	lis2dh_enable_temperature_sensor(true);     // Enable temperature sensor

	return 0;  // Success
}
