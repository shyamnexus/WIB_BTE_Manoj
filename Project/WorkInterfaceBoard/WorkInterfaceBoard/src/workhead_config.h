/**
 * \file workhead_config.h
 *
 * \brief Workhead Interface Board Configuration
 *
 * This file contains configuration parameters for the workhead interface board.
 */

#ifndef WORKHEAD_CONFIG_H
#define WORKHEAD_CONFIG_H

// Board Configuration
#define BOARD_NAME             "Workhead Interface Board"
#define BOARD_VERSION          "1.0.0"
#define FIRMWARE_VERSION       "1.0.0"

// CAN Configuration
#define CAN_BAUDRATE           500000  // 500 kbps
#define CAN_NODE_ID            0x10    // Workhead interface board node ID
#define CAN_RETRY_COUNT        3       // Number of retries for failed transmissions
#define CAN_TIMEOUT_MS         100     // CAN operation timeout

// Workhead Configuration
#define WORKHEAD_MAX_POSITION  100     // Maximum position (0-100%)
#define WORKHEAD_MIN_POSITION  0       // Minimum position (0-100%)
#define WORKHEAD_MAX_SPEED     100     // Maximum speed (0-100%)
#define WORKHEAD_STEP_COUNT    2000    // Total steps for full range
#define WORKHEAD_STEP_DELAY_US 10      // Step pulse width in microseconds

// Timing Configuration
#define STATUS_UPDATE_INTERVAL_MS    100     // Status update interval
#define COMMAND_TIMEOUT_MS           1000    // Command timeout
#define HEARTBEAT_INTERVAL_MS        5000    // Heartbeat interval
#define ERROR_RECOVERY_DELAY_MS      2000    // Error recovery delay

// Error Codes
#define ERROR_NONE              0x00    // No error
#define ERROR_INVALID_CHECKSUM  0x01    // Invalid command checksum
#define ERROR_COMMAND_TIMEOUT   0x02    // Command timeout
#define ERROR_LIMIT_SWITCH      0x03    // Limit switch activated
#define ERROR_EMERGENCY_STOP    0xFF    // Emergency stop activated
#define ERROR_CALIBRATION_FAIL  0x04    // Calibration failed
#define ERROR_SENSOR_FAIL       0x05    // Sensor failure
#define ERROR_COMMUNICATION     0x06    // Communication error

// Status Codes
#define STATUS_IDLE             0x00    // Workhead idle
#define STATUS_WORKING          0x01    // Workhead working
#define STATUS_ERROR            0x02    // Workhead error
#define STATUS_MAINTENANCE      0x03    // Maintenance mode
#define STATUS_CALIBRATING      0x04    // Calibrating

// Command Response Codes
#define RESPONSE_SUCCESS        0x00    // Command successful
#define RESPONSE_ERROR          0x01    // Command failed
#define RESPONSE_INVALID        0x02    // Invalid command
#define RESPONSE_BUSY           0x03    // Workhead busy

// Hardware Configuration
#define ENABLE_DEBUG_LED        1       // Enable debug LED functionality
#define ENABLE_SENSOR_MONITORING 1      // Enable sensor monitoring
#define ENABLE_SAFETY_CHECKS    1       // Enable safety checks

// Debug Configuration
#define DEBUG_ENABLE            1       // Enable debug output
#define DEBUG_LEVEL             2       // Debug level (0-3)

#endif // WORKHEAD_CONFIG_H