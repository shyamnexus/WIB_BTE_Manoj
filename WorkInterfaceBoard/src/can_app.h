#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_BAUD_KBPS          500u // CAN bus bitrate in kbps

/* Default CAN IDs */
#define CAN_ID_LOADCELL        0x120u // ID for load cell measurements
#define CAN_ID_ACCELEROMETER   0x121u // ID for accelerometer XYZ bytes
#define CAN_ID_TEMPERATURE     0x122u // ID for temperature readings
#define CAN_ID_TOOLTYPE        0x123u // ID for tool type status
#define CAN_ID_ENCODER1        0x130u // ID for encoder 1 position/velocity data
#define CAN_ID_ENCODER2        0x131u // ID for encoder 2 position/velocity data
#define CAN_ID_STATUS          0x200u // ID for system status messages
#define CAN_ID_POT_COMMAND     0x220u // ID for potentiometer control/telemetry

bool can_app_init(void); // Initialize CAN controller and RX mailbox
bool can_app_tx(uint32_t id, const uint8_t *data, uint8_t len); // Transmit a CAN frame
bool can_app_get_status(void); // Get CAN controller status
bool can_app_test_loopback(void); // Test CAN communication with loopback mode
bool can_app_simple_test(void); // Simple CAN controller state test for debugging
bool can_verify_bitrate(uint32_t expected_kbps); // Verify CAN bit rate configuration
bool can_app_reset(void); // Reset CAN controller and mailboxes
void CAN0_Handler(void); // CAN0 interrupt handler to prevent system deadlock
void can_disable_interrupts(void); // Disable CAN interrupts
void can_enable_interrupts(void); // Enable CAN interrupts
void can_clear_piob_interrupts(void); // Clear PIOB interrupts and prevent system deadlock
void can_init_piob_safety(void); // Initialize PIOB interrupt handling for safety
void can_rx_task(void *arg); // FreeRTOS task for CAN RX and command handling
void can_status_task(void *arg); // FreeRTOS task for periodic CAN status monitoring
void can_diagnostic_info(void); // Comprehensive CAN diagnostic information

#ifdef __cplusplus
}
#endif