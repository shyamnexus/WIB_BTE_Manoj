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

void task_accelerometer(void *arg)
{
	(void)arg; // Unused parameter
	mc3419_data_t accel_data;
	
	// Initialize MC3419 accelerometer
	if (!mc3419_init()) {
		// Accelerometer initialization failed - task will not function
		volatile uint32_t debug_accel_init_failed = 1;
		while(1) {
			vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second and retry
			if (mc3419_init()) {
				break; // Success, exit retry loop
			}
		}
	}
	
	while(1) {
		// Read accelerometer data
		if (mc3419_read_data(&accel_data) && accel_data.valid) {
			// Prepare CAN payload with accelerometer data
			uint8_t payload[6];
			payload[0] = (uint8_t)(accel_data.x & 0xFF);        // X LSB
			payload[1] = (uint8_t)((accel_data.x >> 8) & 0xFF); // X MSB
			payload[2] = (uint8_t)(accel_data.y & 0xFF);        // Y LSB
			payload[3] = (uint8_t)((accel_data.y >> 8) & 0xFF); // Y MSB
			payload[4] = (uint8_t)(accel_data.z & 0xFF);        // Z LSB
			payload[5] = (uint8_t)((accel_data.z >> 8) & 0xFF); // Z MSB
			
			// Send over CAN
			can_app_tx(CAN_ID_ACCELEROMETER, payload, 6);
		}
		
		vTaskDelay(pdMS_TO_TICKS(10)); // Sample at 100 Hz
	}
}

void task_temperature(void *arg)
{
	(void)arg; // Unused parameter
	mc3419_data_t temp_data;
	
	// Initialize MC3419 for temperature reading
	if (!mc3419_init()) {
		// Accelerometer initialization failed - task will not function
		volatile uint32_t debug_temp_init_failed = 1;
		while(1) {
			vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second and retry
			if (mc3419_init()) {
				break; // Success, exit retry loop
			}
		}
	}
	
	while(1) {
		// Read temperature data from MC3419
		if (mc3419_read_data(&temp_data) && temp_data.valid) {
			// Prepare CAN payload with temperature data
			uint8_t payload[2];
			payload[0] = (uint8_t)(temp_data.temp & 0xFF);        // Temperature LSB
			payload[1] = (uint8_t)((temp_data.temp >> 8) & 0xFF); // Temperature MSB
			
			// Send over CAN
			can_app_tx(CAN_ID_TEMPERATURE, payload, 2);
		}
		
		vTaskDelay(pdMS_TO_TICKS(100)); // Sample at 10 Hz
	}
}

void task_accelerometer_temperature(void *arg)
{
	(void)arg; // Unused parameter
	mc3419_data_t sensor_data;
	
	// Initialize MC3419 accelerometer
	if (!mc3419_init()) {
		// Accelerometer initialization failed - task will not function
		volatile uint32_t debug_combined_init_failed = 1;
		while(1) {
			vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second and retry
			if (mc3419_init()) {
				break; // Success, exit retry loop
			}
		}
	}
	
	while(1) {
		// Read both accelerometer and temperature data
		if (mc3419_read_data(&sensor_data) && sensor_data.valid) {
			// Send accelerometer data
			uint8_t accel_payload[6];
			accel_payload[0] = (uint8_t)(sensor_data.x & 0xFF);        // X LSB
			accel_payload[1] = (uint8_t)((sensor_data.x >> 8) & 0xFF); // X MSB
			accel_payload[2] = (uint8_t)(sensor_data.y & 0xFF);        // Y LSB
			accel_payload[3] = (uint8_t)((sensor_data.y >> 8) & 0xFF); // Y MSB
			accel_payload[4] = (uint8_t)(sensor_data.z & 0xFF);        // Z LSB
			accel_payload[5] = (uint8_t)((sensor_data.z >> 8) & 0xFF); // Z MSB
			can_app_tx(CAN_ID_ACCELEROMETER, accel_payload, 6);
			
			// Send temperature data
			uint8_t temp_payload[2];
			temp_payload[0] = (uint8_t)(sensor_data.temp & 0xFF);        // Temperature LSB
			temp_payload[1] = (uint8_t)((sensor_data.temp >> 8) & 0xFF); // Temperature MSB
			can_app_tx(CAN_ID_TEMPERATURE, temp_payload, 2);
		}
		
		vTaskDelay(pdMS_TO_TICKS(10)); // Sample at 100 Hz
	}
}
void create_application_tasks(void)
{
	
	xTaskCreate(can_rx_task, "canrx", 512, 0, tskIDLE_PRIORITY+2, 0); // CAN RX handler task
	xTaskCreate(can_status_task, "canstatus", 256, 0, tskIDLE_PRIORITY+1, 0); // CAN status monitoring task
	xTaskCreate(task_test, "testTask", 512, 0, tskIDLE_PRIORITY+2, 0); // Load cell sampling task
	
	// MC3419 Accelerometer and Temperature tasks
	xTaskCreate(task_accelerometer_temperature, "accel_temp", 1024, 0, tskIDLE_PRIORITY+3, 0); // Combined accelerometer and temperature task
	// Alternative: Use separate tasks if needed
	// xTaskCreate(task_accelerometer, "accelerometer", 512, 0, tskIDLE_PRIORITY+3, 0); // Accelerometer only task
	// xTaskCreate(task_temperature, "temperature", 512, 0, tskIDLE_PRIORITY+3, 0); // Temperature only task
} // End create_application_tasks