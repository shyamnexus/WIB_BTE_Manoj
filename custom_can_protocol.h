/**
 * @file custom_can_protocol.h
 * @brief Custom CAN Protocol Implementation
 * @version 1.0
 * @date 2024
 */

#ifndef CUSTOM_CAN_PROTOCOL_H
#define CUSTOM_CAN_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Protocol Configuration
#define CAN_PROTOCOL_VERSION         0x01
#define MAX_NODES                    16
#define MAX_RETRIES                  3
#define RESPONSE_TIMEOUT_MS          1000
#define MAX_MESSAGE_SIZE             8

// Node IDs
#define NODE_ID_BROADCAST            0x00
#define NODE_ID_MASTER               0x01
#define NODE_ID_SLAVE_START          0x02
#define NODE_ID_SLAVE_END            0x0F

// CAN ID Configuration (11-bit)
#define CAN_ID_PRIORITY_MASK         0x700
#define CAN_ID_SOURCE_MASK           0x0F0
#define CAN_ID_COMMAND_MASK          0x00F

// Priority Levels
typedef enum {
    PRIORITY_CRITICAL    = 0x0,  // System critical messages
    PRIORITY_HIGH        = 0x1,  // Important control messages
    PRIORITY_NORMAL      = 0x2,  // Regular data messages
    PRIORITY_LOW         = 0x3,  // Status and diagnostic messages
    PRIORITY_DEBUG       = 0x4   // Debug and logging messages
} priority_level_t;

// Command Types
typedef enum {
    // System Commands (0x00-0x0F)
    CMD_SYSTEM_RESET         = 0x00,
    CMD_SYSTEM_STATUS        = 0x01,
    CMD_NODE_DISCOVERY       = 0x02,
    CMD_NODE_INFO            = 0x03,
    CMD_HEARTBEAT            = 0x04,
    CMD_SYNC_TIME            = 0x05,
    
    // Data Transfer Commands (0x10-0x1F)
    CMD_DATA_REQUEST         = 0x10,
    CMD_DATA_RESPONSE        = 0x11,
    CMD_DATA_BROADCAST       = 0x12,
    CMD_DATA_STREAM_START    = 0x13,
    CMD_DATA_STREAM_STOP     = 0x14,
    CMD_DATA_STREAM_DATA     = 0x15,
    
    // Control Commands (0x20-0x2F)
    CMD_SET_PARAMETER        = 0x20,
    CMD_GET_PARAMETER        = 0x21,
    CMD_EXECUTE_ACTION       = 0x22,
    CMD_ABORT_ACTION         = 0x23,
    CMD_QUERY_STATUS         = 0x24,
    
    // Error Handling (0x30-0x3F)
    CMD_ERROR_REPORT         = 0x30,
    CMD_ERROR_ACK            = 0x31,
    CMD_ERROR_CLEAR          = 0x32,
    
    // Custom Commands (0x40-0xFF)
    CMD_CUSTOM_START         = 0x40
} can_command_t;

// Data Types
typedef enum {
    DATA_TYPE_UINT8          = 0x01,
    DATA_TYPE_UINT16         = 0x02,
    DATA_TYPE_UINT32         = 0x03,
    DATA_TYPE_INT8           = 0x04,
    DATA_TYPE_INT16          = 0x05,
    DATA_TYPE_INT32          = 0x06,
    DATA_TYPE_FLOAT          = 0x07,
    DATA_TYPE_STRING         = 0x08,
    DATA_TYPE_BINARY         = 0x09
} data_type_t;

// Error Codes
typedef enum {
    ERROR_NONE               = 0x00,
    ERROR_INVALID_CMD        = 0x01,
    ERROR_INVALID_DATA       = 0x02,
    ERROR_CHECKSUM           = 0x03,
    ERROR_TIMEOUT            = 0x04,
    ERROR_BUSY               = 0x05,
    ERROR_NOT_SUPPORTED      = 0x06,
    ERROR_ACCESS_DENIED      = 0x07,
    ERROR_HARDWARE           = 0x08,
    ERROR_MEMORY             = 0x09,
    ERROR_COMMUNICATION      = 0x0A
} error_code_t;

// Message Structure
typedef struct {
    uint8_t  source_id;      // Source node ID
    uint8_t  dest_id;        // Destination node ID
    uint8_t  command;        // Command type
    uint8_t  sequence;       // Sequence number
    uint8_t  data_length;    // Data length (0-4 bytes)
    uint8_t  data[4];        // Data payload
    uint8_t  checksum;       // Checksum for error detection
} can_message_t;

// CAN Frame Structure
typedef struct {
    uint32_t id;             // CAN ID (11-bit)
    uint8_t  dlc;            // Data Length Code
    uint8_t  data[8];        // CAN data
    bool     rtr;            // Remote Transmission Request
    bool     extended;       // Extended frame format
} can_frame_t;

// Node Information
typedef struct {
    uint8_t  node_id;
    uint8_t  protocol_version;
    uint8_t  hardware_version;
    uint8_t  software_version;
    uint32_t capabilities;
    uint32_t last_heartbeat;
    bool     online;
} node_info_t;

// Protocol Statistics
typedef struct {
    uint32_t messages_sent;
    uint32_t messages_received;
    uint32_t errors_detected;
    uint32_t retries_count;
    uint32_t timeouts_count;
} protocol_stats_t;

// Function Prototypes

/**
 * @brief Initialize the CAN protocol
 * @param node_id Local node ID
 * @return true if initialization successful, false otherwise
 */
bool can_protocol_init(uint8_t node_id);

/**
 * @brief Deinitialize the CAN protocol
 */
void can_protocol_deinit(void);

/**
 * @brief Send a CAN message
 * @param message Pointer to message structure
 * @return true if message sent successfully, false otherwise
 */
bool can_protocol_send(const can_message_t* message);

/**
 * @brief Receive a CAN message
 * @param message Pointer to message structure to fill
 * @param timeout_ms Timeout in milliseconds
 * @return true if message received, false on timeout
 */
bool can_protocol_receive(can_message_t* message, uint32_t timeout_ms);

/**
 * @brief Send a command and wait for response
 * @param cmd Command to send
 * @param data Data to send with command
 * @param data_len Length of data
 * @param dest_id Destination node ID
 * @param response Pointer to response message
 * @param timeout_ms Timeout in milliseconds
 * @return true if command sent and response received, false otherwise
 */
bool can_protocol_send_command(can_command_t cmd, const uint8_t* data, 
                              uint8_t data_len, uint8_t dest_id,
                              can_message_t* response, uint32_t timeout_ms);

/**
 * @brief Broadcast a message to all nodes
 * @param cmd Command to broadcast
 * @param data Data to broadcast
 * @param data_len Length of data
 * @return true if broadcast sent successfully, false otherwise
 */
bool can_protocol_broadcast(can_command_t cmd, const uint8_t* data, uint8_t data_len);

/**
 * @brief Get node information
 * @param node_id Node ID to query
 * @param info Pointer to node info structure
 * @return true if node info retrieved, false otherwise
 */
bool can_protocol_get_node_info(uint8_t node_id, node_info_t* info);

/**
 * @brief Discover nodes on the network
 * @param nodes Array to store discovered node IDs
 * @param max_nodes Maximum number of nodes to discover
 * @return Number of nodes discovered
 */
uint8_t can_protocol_discover_nodes(uint8_t* nodes, uint8_t max_nodes);

/**
 * @brief Get protocol statistics
 * @param stats Pointer to statistics structure
 */
void can_protocol_get_stats(protocol_stats_t* stats);

/**
 * @brief Reset protocol statistics
 */
void can_protocol_reset_stats(void);

/**
 * @brief Calculate checksum for a message
 * @param message Pointer to message structure
 * @return Calculated checksum
 */
uint8_t can_protocol_calculate_checksum(const can_message_t* message);

/**
 * @brief Verify checksum of a message
 * @param message Pointer to message structure
 * @return true if checksum is valid, false otherwise
 */
bool can_protocol_verify_checksum(const can_message_t* message);

/**
 * @brief Convert message to CAN frame
 * @param message Pointer to message structure
 * @param frame Pointer to CAN frame structure
 * @param priority Message priority
 */
void can_protocol_message_to_frame(const can_message_t* message, 
                                  can_frame_t* frame, priority_level_t priority);

/**
 * @brief Convert CAN frame to message
 * @param frame Pointer to CAN frame structure
 * @param message Pointer to message structure
 * @return true if conversion successful, false otherwise
 */
bool can_protocol_frame_to_message(const can_frame_t* frame, can_message_t* message);

/**
 * @brief Process received CAN frame
 * @param frame Pointer to received CAN frame
 */
void can_protocol_process_frame(const can_frame_t* frame);

/**
 * @brief Callback function type for message received
 * @param message Pointer to received message
 */
typedef void (*can_message_callback_t)(const can_message_t* message);

/**
 * @brief Register message callback function
 * @param callback Callback function pointer
 */
void can_protocol_register_callback(can_message_callback_t callback);

/**
 * @brief Get current node ID
 * @return Current node ID
 */
uint8_t can_protocol_get_node_id(void);

/**
 * @brief Check if node is online
 * @param node_id Node ID to check
 * @return true if node is online, false otherwise
 */
bool can_protocol_is_node_online(uint8_t node_id);

/**
 * @brief Update node heartbeat
 * @param node_id Node ID
 */
void can_protocol_update_heartbeat(uint8_t node_id);

/**
 * @brief Send heartbeat message
 */
void can_protocol_send_heartbeat(void);

#endif // CUSTOM_CAN_PROTOCOL_H