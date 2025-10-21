#include "tasks.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "can_app.h"
#include "spi0.h"
#include "mc3419.h"

#include "FreeRTOS.h"
#include "task.h"
#include "WIB_Init.h"

#ifndef TickType_t
typedef portTickType TickType_t; // Backward-compatible alias if TickType_t isn't defined
#endif
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_PERIOD_MS)) // Convert milliseconds to OS ticks
#endif

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS portTICK_RATE_MS // Legacy macro mapping
#endif


void task_test(void *arg){
	(void)arg; // Unused parameter
	uint8_t msb=0xAA, lsb=0x55; 
	while(1) {
		uint8_t payload[2] = { msb, lsb };
		can_app_tx(CAN_ID_LOADCELL, payload, 2); // Publish loadcell sample over CAN
		vTaskDelay(pdMS_TO_TICKS(100)); // Sample at ~500 Hz  == 2 milli seconds
		}
}

void task_MC3419DAQ (void *arg)
{
	(void)arg;
	
	mc3419_data_t sensor_data;
	uint8_t accel_payload[6];
	uint8_t temp_payload[2];
	uint32_t init_retry_count = 0;
	const uint32_t max_init_retries = 5;
	
	// Initialize MC3419 sensor with retry logic
	while (init_retry_count < max_init_retries) {
		if (mc3419_init()) {
			break; // Success
		}
		
		init_retry_count++;
		delay_ms(1000); // Wait 1 second before retry
	}
	
	// If initialization failed after all retries, set debug flag and continue
	if (init_retry_count >= max_init_retries) {
		volatile uint32_t debug_mc3419_init_failed = 1;
		// Continue anyway - task will keep trying to read data
	}
	
	// Main task loop
	while (1) {
		// Read sensor data
		if (mc3419_read_data(&sensor_data) && sensor_data.valid) {
			// Prepare accelerometer data payload (6 bytes: X, Y, Z)
			accel_payload[0] = (uint8_t)(sensor_data.x & 0xFF);        // X LSB
			accel_payload[1] = (uint8_t)((sensor_data.x >> 8) & 0xFF); // X MSB
			accel_payload[2] = (uint8_t)(sensor_data.y & 0xFF);        // Y LSB
			accel_payload[3] = (uint8_t)((sensor_data.y >> 8) & 0xFF); // Y MSB
			accel_payload[4] = (uint8_t)(sensor_data.z & 0xFF);        // Z LSB
			accel_payload[5] = (uint8_t)((sensor_data.z >> 8) & 0xFF); // Z MSB
			
			// Prepare temperature data payload (2 bytes)
			temp_payload[0] = (uint8_t)(sensor_data.temp & 0xFF);        // Temp LSB
			temp_payload[1] = (uint8_t)((sensor_data.temp >> 8) & 0xFF); // Temp MSB
			
			// Send accelerometer data over CAN (ID: 0x121)
			can_app_tx(CAN_ID_ACCELEROMETER, accel_payload, 6);
			
			// Send temperature data over CAN (ID: 0x122)
			can_app_tx(CAN_ID_TEMPERATURE, temp_payload, 2);
			
			// Debug: Store conversion values for monitoring
			volatile float debug_x_g = mc3419_convert_accel_to_g(sensor_data.x, MC3419_RANGE_8G);
			volatile float debug_y_g = mc3419_convert_accel_to_g(sensor_data.y, MC3419_RANGE_8G);
			volatile float debug_z_g = mc3419_convert_accel_to_g(sensor_data.z, MC3419_RANGE_8G);
			volatile float debug_temp_c = mc3419_convert_temp_to_celsius(sensor_data.temp);
		} else {
			// Data read failed - set debug flag
			volatile uint32_t debug_mc3419_read_failed = 1;
		}
		
		// Task delay for 100Hz sampling rate (10ms)
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
void create_application_tasks(void)
{
	
	xTaskCreate(can_rx_task, "canrx", 512, 0, tskIDLE_PRIORITY+2, 0); // CAN RX handler task
	xTaskCreate(can_status_task, "canstatus", 256, 0, tskIDLE_PRIORITY+1, 0); // CAN status monitoring task
	//xTaskCreate(task_test, "testTask", 512, 0, tskIDLE_PRIORITY+2, 0); // Load cell sampling task
	xTaskCreate (task_MC3419DAQ, "MC3419 Data  Acquisition" , 512 , 0,tskIDLE_PRIORITY+3 , 0);
} // End create_application_tasks