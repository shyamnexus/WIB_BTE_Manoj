#include "tasks.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "can_app.h"
#include "spi0.h"
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

void task_encoder1_pin_monitor(void *arg){
	(void)arg; // Unused parameter
	
	// Enable PIOA clock for encoder pins
	pmc_enable_periph_clk(ID_PIOA);
	
	// Configure encoder 1 pins as inputs with pull-ups
	// Encoder 1: PA5 (A), PA1 (B)
	pio_configure(PIOA, PIO_INPUT, PIO_PA5, PIO_PULLUP);
	pio_configure(PIOA, PIO_INPUT, PIO_PA1, PIO_PULLUP);
	
	// Configure enable pin as output and set it low (enable encoder 1)
	pmc_enable_periph_clk(ID_PIOD);
	pio_configure(PIOD, PIO_OUTPUT_0, PIO_PD17, PIO_DEFAULT);
	pio_clear(PIOD, PIO_PD17);  // Enable encoder 1 (set low)
	
	while(1) {
		// Read encoder 1 pin states
		uint8_t pin_a_state = pio_get(PIOA, PIO_INPUT, PIO_PA5) ? 1 : 0;
		uint8_t pin_b_state = pio_get(PIOA, PIO_INPUT, PIO_PA1) ? 1 : 0;
		
		// Prepare CAN message with pin states
		uint8_t payload[2] = { pin_a_state, pin_b_state };
		
		// Send over CAN with ID 0x188
		can_app_tx(CAN_ID_ENCODER1_PINS, payload, 2);
		
		// Poll at 10ms intervals (100 Hz)
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
void create_application_tasks(void)
{
	// Create CAN tasks and interrupt-driven encoder task
	xTaskCreate(can_rx_task, "canrx", 512, 0, tskIDLE_PRIORITY+2, 0); // CAN RX handler task
	xTaskCreate(can_status_task, "canstatus", 256, 0, tskIDLE_PRIORITY+1, 0); // CAN status monitoring task
	xTaskCreate(encoder_task, "encoder", 512, 0, tskIDLE_PRIORITY+2, 0); // Interrupt-driven encoder task
	
	// All other tasks disabled as requested
	// xTaskCreate(task_test, "testTask", 512, 0, tskIDLE_PRIORITY+2, 0); // Load cell sampling task - DISABLED
	// xTaskCreate(task_encoder1_pin_monitor, "enc1pins", 512, 0, tskIDLE_PRIORITY+2, 0); // Encoder 1 pin monitoring task - DISABLED
} // End create_application_tasks