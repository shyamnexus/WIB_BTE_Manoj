#include "tasks.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "can_app.h"
#include "spi0.h"
#include "mc3419.h"
#include "encoder.h"

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

void task_encoder(void *arg)
{
	(void)arg; // Unused parameter
	
	encoder_data_t enc1_data = {0};
	encoder_data_t enc2_data = {0};
	uint8_t enc1_payload[8];
	uint8_t enc2_payload[8];
	uint32_t init_retry_count = 0;
	const uint32_t max_init_retries = 5;
	
	// Initialize encoder hardware with retry logic
	while (init_retry_count < max_init_retries) {
		if (encoder_init()) {
			break; // Success
		}
		
		init_retry_count++;
		delay_ms(1000); // Wait 1 second before retry
	}
	
	// If initialization failed after all retries, set debug flag and continue
	if (init_retry_count >= max_init_retries) {
		volatile uint32_t debug_encoder_init_failed = 1;
		// Continue anyway - task will keep trying to read data
	} else {
		volatile uint32_t debug_encoder_init_success = 1;
	}
	
	// Wait for system to stabilize before enabling interrupts
	vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2 seconds for system to stabilize
	
	// Enable encoder interrupts now that tasks are running
	encoder_enable_interrupts();
	volatile uint32_t debug_encoder_interrupts_enabled = 1;
	
	// Reset encoder counters to start from zero
	encoder_reset_counters();
	
	// Main task loop
	while (1) {
		// Read encoder data without disabling interrupts to allow continuous updates
		// The encoder_read_data function already handles atomic access to position counters
		if (encoder_read_data(&enc1_data, &enc2_data)) {
			// Prepare encoder 1 data payload (8 bytes: position + velocity)
			enc1_payload[0] = (uint8_t)(enc1_data.position & 0xFF);         // Position LSB
			enc1_payload[1] = (uint8_t)((enc1_data.position >> 8) & 0xFF);  // Position byte 1
			enc1_payload[2] = (uint8_t)((enc1_data.position >> 16) & 0xFF); // Position byte 2
			enc1_payload[3] = (uint8_t)((enc1_data.position >> 24) & 0xFF); // Position MSB
			enc1_payload[4] = (uint8_t)(enc1_data.velocity & 0xFF);         // Velocity LSB
			enc1_payload[5] = (uint8_t)((enc1_data.velocity >> 8) & 0xFF);  // Velocity byte 1
			enc1_payload[6] = (uint8_t)((enc1_data.velocity >> 16) & 0xFF); // Velocity byte 2
			enc1_payload[7] = (uint8_t)((enc1_data.velocity >> 24) & 0xFF); // Velocity MSB
			
			// Prepare encoder 2 data payload (8 bytes: position + velocity)
			enc2_payload[0] = (uint8_t)(enc2_data.position & 0xFF);         // Position LSB
			enc2_payload[1] = (uint8_t)((enc2_data.position >> 8) & 0xFF);  // Position byte 1
			enc2_payload[2] = (uint8_t)((enc2_data.position >> 16) & 0xFF); // Position byte 2
			enc2_payload[3] = (uint8_t)((enc2_data.position >> 24) & 0xFF); // Position MSB
			enc2_payload[4] = (uint8_t)(enc2_data.velocity & 0xFF);         // Velocity LSB
			enc2_payload[5] = (uint8_t)((enc2_data.velocity >> 8) & 0xFF);  // Velocity byte 1
			enc2_payload[6] = (uint8_t)((enc2_data.velocity >> 16) & 0xFF); // Velocity byte 2
			enc2_payload[7] = (uint8_t)((enc2_data.velocity >> 24) & 0xFF); // Velocity MSB
			
			// Send encoder 1 data over CAN (ID: 0x130)
			can_app_tx(CAN_ID_ENCODER1, enc1_payload, 8);
			
			// Send encoder 2 data over CAN (ID: 0x131)
			can_app_tx(CAN_ID_ENCODER2, enc2_payload, 8);
			
			// Debug: Store conversion values for monitoring
			volatile int32_t debug_enc1_pos = enc1_data.position;
			volatile int32_t debug_enc1_vel = enc1_data.velocity;
			volatile int32_t debug_enc2_pos = enc2_data.position;
			volatile int32_t debug_enc2_vel = enc2_data.velocity;
			volatile uint32_t debug_encoder_data_valid = 1;
		} else {
			// Data read failed - set debug flag
			volatile uint32_t debug_encoder_read_failed = 1;
			volatile uint32_t debug_encoder_data_valid = 0;
		}
		
		// Task delay for 100Hz sampling rate (10ms) for better encoder responsiveness
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
void task_interrupt_monitor(void *arg)
{
	(void)arg; // Unused parameter
	uint32_t total_interrupts = 0;
	uint32_t skipped_interrupts = 0;
	uint32_t last_total = 0;
	uint32_t last_skipped = 0;
	
	while (1) {
		// Get current interrupt statistics
		encoder_get_interrupt_stats(&total_interrupts, &skipped_interrupts);
		
		// Calculate interrupt rate (interrupts per second)
		uint32_t interrupts_per_second = total_interrupts - last_total;
		uint32_t skipped_per_second = skipped_interrupts - last_skipped;
		
		// Get encoder connection and interrupt status
		bool encoder_connected = encoder_get_connection_status();
		bool interrupts_enabled = encoder_get_interrupt_status();
		uint32_t debug_interrupts_processed = encoder_get_debug_interrupt_count();
		uint32_t debug_position_changes = encoder_get_debug_position_changes();
		
		// Check and recover from interrupt loops
		encoder_check_and_recover_interrupts();
		
		// Monitor encoder connection and disable interrupts if no encoder detected
		encoder_monitor_connection();
		
		// Store debug information
		volatile uint32_t debug_total_interrupts = total_interrupts;
		volatile uint32_t debug_skipped_interrupts = skipped_interrupts;
		volatile uint32_t debug_interrupts_per_second = interrupts_per_second;
		volatile uint32_t debug_skipped_per_second = skipped_per_second;
		volatile uint32_t debug_encoder_connected = encoder_connected ? 1 : 0;
		volatile uint32_t debug_interrupts_enabled = interrupts_enabled ? 1 : 0;
		volatile uint32_t debug_interrupts_processed_total = debug_interrupts_processed;
		volatile uint32_t debug_position_changes_total = debug_position_changes;
		
		// If interrupt rate is too high, reset statistics to prevent overflow
		if (total_interrupts > 10000) {
			encoder_reset_interrupt_stats();
			last_total = 0;
			last_skipped = 0;
		} else {
			last_total = total_interrupts;
			last_skipped = skipped_interrupts;
		}
		
		// Monitor every 5 seconds
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}

void create_application_tasks(void)
{
	
	xTaskCreate(can_rx_task, "canrx", 512, 0, tskIDLE_PRIORITY+2, 0); // CAN RX handler task
	xTaskCreate(can_status_task, "canstatus", 256, 0, tskIDLE_PRIORITY+1, 0); // CAN status monitoring task
	//xTaskCreate(task_test, "testTask", 512, 0, tskIDLE_PRIORITY+2, 0); // Load cell sampling task
	xTaskCreate (task_MC3419DAQ, "MC3419 Data  Acquisition" , 512 , 0,tskIDLE_PRIORITY+3 , 0);
	xTaskCreate(task_encoder, "encoder", 512, 0, tskIDLE_PRIORITY+2, 0); // Encoder reading task
	xTaskCreate(task_interrupt_monitor, "intmonitor", 256, 0, tskIDLE_PRIORITY+1, 0); // Interrupt monitoring task
} // End create_application_tasks