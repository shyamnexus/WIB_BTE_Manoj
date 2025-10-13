/**
 * \file workhead_can.h
 *
 * \brief Workhead Interface Board CAN Communication Module
 *
 * This module provides CAN communication functionality for the workhead interface board.
 * It handles sending workhead status data and receiving commands over CAN bus.
 */

#ifndef WORKHEAD_CAN_H
#define WORKHEAD_CAN_H

#include <asf.h>

// CAN Configuration
#define CAN_BAUDRATE           500000  // 500 kbps
#define CAN_NODE_ID            0x10    // Workhead interface board node ID

// CAN Message IDs
#define CAN_MSG_STATUS         0x100   // Workhead status message (sent by this board)
#define CAN_MSG_COMMAND        0x200   // Command message (received by this board)
#define CAN_MSG_ACK            0x300   // Acknowledgment message
#define CAN_MSG_ERROR          0x400   // Error message

// Workhead Status Structure
typedef struct {
    uint8_t workhead_id;       // Workhead identifier
    uint8_t status;            // Current status (0=idle, 1=working, 2=error, 3=maintenance)
    uint8_t position;          // Current position (0-100%)
    uint8_t speed;             // Current speed (0-100%)
    uint8_t temperature;       // Temperature in Celsius
    uint8_t vibration;         // Vibration level (0-100%)
    uint8_t error_code;        // Error code if status is error
    uint8_t reserved;          // Reserved for future use
} workhead_status_t;

// Command Structure
typedef struct {
    uint8_t command_id;        // Command identifier
    uint8_t workhead_id;       // Target workhead ID
    uint8_t parameter1;        // Command parameter 1
    uint8_t parameter2;        // Command parameter 2
    uint8_t parameter3;        // Command parameter 3
    uint8_t parameter4;        // Command parameter 4
    uint8_t checksum;          // Command checksum
    uint8_t reserved;          // Reserved for future use
} workhead_command_t;

// Command IDs
#define CMD_START              0x01    // Start workhead
#define CMD_STOP               0x02    // Stop workhead
#define CMD_SET_POSITION       0x03    // Set position
#define CMD_SET_SPEED          0x04    // Set speed
#define CMD_EMERGENCY_STOP     0x05    // Emergency stop
#define CMD_RESET              0x06    // Reset workhead
#define CMD_GET_STATUS         0x07    // Get status
#define CMD_CALIBRATE          0x08    // Calibrate workhead

// Function prototypes
void workhead_can_init(void);
void can_send_status(const workhead_status_t *status);
void can_send_ack(uint8_t command_id, uint8_t result);
void can_send_error(uint8_t error_code);
bool can_receive_command(workhead_command_t *command);
void can_process_messages(void);
uint8_t calculate_checksum(const uint8_t *data, uint8_t length);

// Interrupt handler
void CAN0_Handler(void);

#endif // WORKHEAD_CAN_H