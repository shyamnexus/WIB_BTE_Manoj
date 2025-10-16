#include "tasks.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "can_app.h"
#include "spi0.h"
#include "lis2dh.h"

#include "FreeRTOS.h"
#include "task.h"
#include "TIB_Init.h"

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
	lis2dh_accel_data_t accel_data;
	
	while(1) {
		if (lis2dh_read_accelerometer(&accel_data)) {
			// Pack accelerometer data into CAN payload (6 bytes: X, Y, Z as 16-bit values)
			uint8_t payload[6];
			payload[0] = (uint8_t)(accel_data.x & 0xFF);        // X low byte
			payload[1] = (uint8_t)((accel_data.x >> 8) & 0xFF); // X high byte
			payload[2] = (uint8_t)(accel_data.y & 0xFF);        // Y low byte
			payload[3] = (uint8_t)((accel_data.y >> 8) & 0xFF); // Y high byte
			payload[4] = (uint8_t)(accel_data.z & 0xFF);        // Z low byte
			payload[5] = (uint8_t)((accel_data.z >> 8) & 0xFF); // Z high byte
			
			can_app_tx(CAN_ID_ACCELEROMETER, payload, 6);
		}
		vTaskDelay(pdMS_TO_TICKS(50)); // Sample at 20Hz
	}
}

void task_temperature(void *arg)
{
	(void)arg; // Unused parameter
	lis2dh_temp_data_t temp_data;
	
	while(1) {
		if (lis2dh_read_temperature(&temp_data)) {
			// Pack temperature data into CAN payload (2 bytes: 16-bit temperature value)
			uint8_t payload[2];
			payload[0] = (uint8_t)(temp_data.raw & 0xFF);        // Temperature low byte
			payload[1] = (uint8_t)((temp_data.raw >> 8) & 0xFF); // Temperature high byte
			
			can_app_tx(CAN_ID_TEMPERATURE, payload, 2);
		}
		vTaskDelay(pdMS_TO_TICKS(1000)); // Sample at 1Hz
	}
}

void task_accelerometer_temperature(void *arg)
{
	(void)arg; // Unused parameter
	lis2dh_accel_data_t accel_data;
	lis2dh_temp_data_t temp_data;
	
	while(1) {
		if (lis2dh_read_accelerometer_and_temperature(&accel_data, &temp_data)) {
			// Send accelerometer data
			uint8_t accel_payload[6];
			accel_payload[0] = (uint8_t)(accel_data.x & 0xFF);
			accel_payload[1] = (uint8_t)((accel_data.x >> 8) & 0xFF);
			accel_payload[2] = (uint8_t)(accel_data.y & 0xFF);
			accel_payload[3] = (uint8_t)((accel_data.y >> 8) & 0xFF);
			accel_payload[4] = (uint8_t)(accel_data.z & 0xFF);
			accel_payload[5] = (uint8_t)((accel_data.z >> 8) & 0xFF);
			can_app_tx(CAN_ID_ACCELEROMETER, accel_payload, 6);
			
			// Send temperature data
			uint8_t temp_payload[2];
			temp_payload[0] = (uint8_t)(temp_data.raw & 0xFF);
			temp_payload[1] = (uint8_t)((temp_data.raw >> 8) & 0xFF);
			can_app_tx(CAN_ID_TEMPERATURE, temp_payload, 2);
		}
		vTaskDelay(pdMS_TO_TICKS(100)); // Sample at 10Hz
	}
}
void create_application_tasks(void)
{
	
	xTaskCreate(can_rx_task, "canrx", 512, 0, tskIDLE_PRIORITY+2, 0); // CAN RX handler task
	xTaskCreate(can_status_task, "canstatus", 256, 0, tskIDLE_PRIORITY+1, 0); // CAN status monitoring task
	xTaskCreate(task_test, "testTask", 512, 0, tskIDLE_PRIORITY+2, 0); // Load cell sampling task
	
	// Sensor tasks - choose one of the following approaches:
	// Option 1: Separate tasks for accelerometer and temperature
	// xTaskCreate(task_accelerometer, "accel", 512, 0, tskIDLE_PRIORITY+2, 0);
	// xTaskCreate(task_temperature, "temp", 256, 0, tskIDLE_PRIORITY+2, 0);
	
	// Option 2: Combined task for both accelerometer and temperature (recommended)
	xTaskCreate(task_accelerometer_temperature, "accel_temp", 512, 0, tskIDLE_PRIORITY+2, 0);
} // End create_application_tasks