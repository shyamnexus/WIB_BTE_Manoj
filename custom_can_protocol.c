/**
 * @file custom_can_protocol.c
 * @brief Custom CAN Protocol Implementation
 * @version 1.0
 * @date 2024
 */

#include "custom_can_protocol.h"
#include <string.h>
#include <stdio.h>

// Static variables
static uint8_t g_node_id = 0;
static node_info_t g_nodes[MAX_NODES];
static protocol_stats_t g_stats = {0};
static can_message_callback_t g_message_callback = NULL;
static uint8_t g_sequence_counter = 0;

// Private function prototypes
static void update_node_info(uint8_t node_id, const can_message_t* message);
static bool is_valid_node_id(uint8_t node_id);
static uint32_t get_timestamp_ms(void);
static void handle_system_command(const can_message_t* message);
static void handle_data_command(const can_message_t* message);
static void handle_control_command(const can_message_t* message);
static void handle_error_command(const can_message_t* message);

bool can_protocol_init(uint8_t node_id) {
    if (node_id == 0 || node_id > NODE_ID_SLAVE_END) {
        return false;
    }
    
    g_node_id = node_id;
    
    // Initialize node information
    memset(g_nodes, 0, sizeof(g_nodes));
    for (int i = 0; i < MAX_NODES; i++) {
        g_nodes[i].node_id = i;
        g_nodes[i].online = false;
        g_nodes[i].last_heartbeat = 0;
    }
    
    // Mark current node as online
    g_nodes[node_id].node_id = node_id;
    g_nodes[node_id].online = true;
    g_nodes[node_id].protocol_version = CAN_PROTOCOL_VERSION;
    g_nodes[node_id].last_heartbeat = get_timestamp_ms();
    
    // Reset statistics
    memset(&g_stats, 0, sizeof(g_stats));
    
    return true;
}

void can_protocol_deinit(void) {
    g_node_id = 0;
    g_message_callback = NULL;
    memset(g_nodes, 0, sizeof(g_nodes));
    memset(&g_stats, 0, sizeof(g_stats));
}

bool can_protocol_send(const can_message_t* message) {
    if (!message || !is_valid_node_id(message->dest_id)) {
        return false;
    }
    
    can_frame_t frame;
    priority_level_t priority = PRIORITY_NORMAL;
    
    // Determine priority based on command
    if (message->command <= CMD_SYSTEM_RESET + 5) {
        priority = PRIORITY_CRITICAL;
    } else if (message->command >= CMD_ERROR_REPORT) {
        priority = PRIORITY_HIGH;
    }
    
    can_protocol_message_to_frame(message, &frame, priority);
    
    // Here you would call your actual CAN driver to send the frame
    // For now, we'll simulate it
    printf("Sending CAN frame: ID=0x%03X, DLC=%d, Data=", frame.id, frame.dlc);
    for (int i = 0; i < frame.dlc; i++) {
        printf("%02X ", frame.data[i]);
    }
    printf("\n");
    
    g_stats.messages_sent++;
    return true;
}

bool can_protocol_receive(can_message_t* message, uint32_t timeout_ms) {
    if (!message) {
        return false;
    }
    
    // Here you would call your actual CAN driver to receive a frame
    // For now, we'll simulate it
    static uint32_t last_receive_time = 0;
    uint32_t current_time = get_timestamp_ms();
    
    if (current_time - last_receive_time < 100) { // Simulate no message
        return false;
    }
    
    // Simulate receiving a message
    message->source_id = 0x02;
    message->dest_id = g_node_id;
    message->command = CMD_HEARTBEAT;
    message->sequence = 1;
    message->data_length = 2;
    message->data[0] = 0x01;
    message->data[1] = 0x02;
    message->checksum = can_protocol_calculate_checksum(message);
    
    last_receive_time = current_time;
    g_stats.messages_received++;
    
    return true;
}

bool can_protocol_send_command(can_command_t cmd, const uint8_t* data, 
                              uint8_t data_len, uint8_t dest_id,
                              can_message_t* response, uint32_t timeout_ms) {
    if (data_len > 4 || !is_valid_node_id(dest_id)) {
        return false;
    }
    
    can_message_t message;
    message.source_id = g_node_id;
    message.dest_id = dest_id;
    message.command = cmd;
    message.sequence = ++g_sequence_counter;
    message.data_length = data_len;
    
    if (data && data_len > 0) {
        memcpy(message.data, data, data_len);
    }
    
    message.checksum = can_protocol_calculate_checksum(&message);
    
    // Send the command
    if (!can_protocol_send(&message)) {
        return false;
    }
    
    // Wait for response
    uint32_t start_time = get_timestamp_ms();
    while ((get_timestamp_ms() - start_time) < timeout_ms) {
        can_message_t received_message;
        if (can_protocol_receive(&received_message, 10)) {
            // Check if this is a response to our command
            if (received_message.source_id == dest_id &&
                received_message.dest_id == g_node_id &&
                received_message.sequence == message.sequence) {
                if (response) {
                    *response = received_message;
                }
                return true;
            }
        }
    }
    
    g_stats.timeouts_count++;
    return false;
}

bool can_protocol_broadcast(can_command_t cmd, const uint8_t* data, uint8_t data_len) {
    can_message_t message;
    message.source_id = g_node_id;
    message.dest_id = NODE_ID_BROADCAST;
    message.command = cmd;
    message.sequence = ++g_sequence_counter;
    message.data_length = data_len;
    
    if (data && data_len > 0) {
        memcpy(message.data, data, data_len);
    }
    
    message.checksum = can_protocol_calculate_checksum(&message);
    
    return can_protocol_send(&message);
}

bool can_protocol_get_node_info(uint8_t node_id, node_info_t* info) {
    if (!is_valid_node_id(node_id) || !info) {
        return false;
    }
    
    *info = g_nodes[node_id];
    return true;
}

uint8_t can_protocol_discover_nodes(uint8_t* nodes, uint8_t max_nodes) {
    if (!nodes || max_nodes == 0) {
        return 0;
    }
    
    uint8_t count = 0;
    for (int i = 0; i < MAX_NODES && count < max_nodes; i++) {
        if (g_nodes[i].online && g_nodes[i].node_id != g_node_id) {
            nodes[count++] = g_nodes[i].node_id;
        }
    }
    
    return count;
}

void can_protocol_get_stats(protocol_stats_t* stats) {
    if (stats) {
        *stats = g_stats;
    }
}

void can_protocol_reset_stats(void) {
    memset(&g_stats, 0, sizeof(g_stats));
}

uint8_t can_protocol_calculate_checksum(const can_message_t* message) {
    if (!message) {
        return 0;
    }
    
    uint8_t checksum = 0;
    checksum ^= message->source_id;
    checksum ^= message->dest_id;
    checksum ^= message->command;
    checksum ^= message->sequence;
    checksum ^= message->data_length;
    
    for (int i = 0; i < message->data_length; i++) {
        checksum ^= message->data[i];
    }
    
    return checksum;
}

bool can_protocol_verify_checksum(const can_message_t* message) {
    if (!message) {
        return false;
    }
    
    uint8_t calculated_checksum = can_protocol_calculate_checksum(message);
    return (calculated_checksum == message->checksum);
}

void can_protocol_message_to_frame(const can_message_t* message, 
                                  can_frame_t* frame, priority_level_t priority) {
    if (!message || !frame) {
        return;
    }
    
    // Construct CAN ID
    frame->id = (priority << 8) | (message->source_id << 4) | (message->command & 0x0F);
    frame->rtr = false;
    frame->extended = false;
    frame->dlc = 8; // Always use full 8 bytes
    
    // Pack message into CAN data
    frame->data[0] = message->source_id;
    frame->data[1] = message->dest_id;
    frame->data[2] = message->command;
    frame->data[3] = message->sequence;
    frame->data[4] = message->data_length;
    
    // Copy data payload
    for (int i = 0; i < 4 && i < message->data_length; i++) {
        frame->data[5 + i] = message->data[i];
    }
    
    // Add checksum
    frame->data[7] = message->checksum;
}

bool can_protocol_frame_to_message(const can_frame_t* frame, can_message_t* message) {
    if (!frame || !message || frame->dlc != 8) {
        return false;
    }
    
    // Extract message from CAN data
    message->source_id = frame->data[0];
    message->dest_id = frame->data[1];
    message->command = frame->data[2];
    message->sequence = frame->data[3];
    message->data_length = frame->data[4];
    
    // Copy data payload
    for (int i = 0; i < 4 && i < message->data_length; i++) {
        message->data[i] = frame->data[5 + i];
    }
    
    // Extract checksum
    message->checksum = frame->data[7];
    
    return true;
}

void can_protocol_process_frame(const can_frame_t* frame) {
    if (!frame) {
        return;
    }
    
    can_message_t message;
    if (!can_protocol_frame_to_message(frame, &message)) {
        g_stats.errors_detected++;
        return;
    }
    
    // Verify checksum
    if (!can_protocol_verify_checksum(&message)) {
        g_stats.errors_detected++;
        return;
    }
    
    // Check if message is for this node or broadcast
    if (message.dest_id != g_node_id && message.dest_id != NODE_ID_BROADCAST) {
        return;
    }
    
    // Update node information
    update_node_info(message.source_id, &message);
    
    // Process message based on command type
    if (message.command <= CMD_SYNC_TIME) {
        handle_system_command(&message);
    } else if (message.command <= CMD_DATA_STREAM_DATA) {
        handle_data_command(&message);
    } else if (message.command <= CMD_QUERY_STATUS) {
        handle_control_command(&message);
    } else if (message.command <= CMD_ERROR_CLEAR) {
        handle_error_command(&message);
    }
    
    // Call user callback if registered
    if (g_message_callback) {
        g_message_callback(&message);
    }
    
    g_stats.messages_received++;
}

void can_protocol_register_callback(can_message_callback_t callback) {
    g_message_callback = callback;
}

uint8_t can_protocol_get_node_id(void) {
    return g_node_id;
}

bool can_protocol_is_node_online(uint8_t node_id) {
    if (!is_valid_node_id(node_id)) {
        return false;
    }
    
    uint32_t current_time = get_timestamp_ms();
    return (g_nodes[node_id].online && 
            (current_time - g_nodes[node_id].last_heartbeat) < 5000); // 5 second timeout
}

void can_protocol_update_heartbeat(uint8_t node_id) {
    if (is_valid_node_id(node_id)) {
        g_nodes[node_id].last_heartbeat = get_timestamp_ms();
        g_nodes[node_id].online = true;
    }
}

void can_protocol_send_heartbeat(void) {
    can_protocol_broadcast(CMD_HEARTBEAT, NULL, 0);
}

// Private function implementations

static void update_node_info(uint8_t node_id, const can_message_t* message) {
    if (!is_valid_node_id(node_id)) {
        return;
    }
    
    g_nodes[node_id].node_id = node_id;
    g_nodes[node_id].online = true;
    g_nodes[node_id].last_heartbeat = get_timestamp_ms();
    
    // Update protocol version if available
    if (message->data_length >= 1) {
        g_nodes[node_id].protocol_version = message->data[0];
    }
}

static bool is_valid_node_id(uint8_t node_id) {
    return (node_id >= NODE_ID_BROADCAST && node_id <= NODE_ID_SLAVE_END);
}

static uint32_t get_timestamp_ms(void) {
    // This should be replaced with actual system time function
    static uint32_t timestamp = 0;
    return ++timestamp;
}

static void handle_system_command(const can_message_t* message) {
    switch (message->command) {
        case CMD_SYSTEM_RESET:
            // Handle system reset
            break;
        case CMD_SYSTEM_STATUS:
            // Send system status response
            break;
        case CMD_NODE_DISCOVERY:
            // Handle node discovery
            break;
        case CMD_HEARTBEAT:
            can_protocol_update_heartbeat(message->source_id);
            break;
        case CMD_SYNC_TIME:
            // Handle time synchronization
            break;
        default:
            break;
    }
}

static void handle_data_command(const can_message_t* message) {
    switch (message->command) {
        case CMD_DATA_REQUEST:
            // Handle data request
            break;
        case CMD_DATA_RESPONSE:
            // Handle data response
            break;
        case CMD_DATA_BROADCAST:
            // Handle data broadcast
            break;
        default:
            break;
    }
}

static void handle_control_command(const can_message_t* message) {
    switch (message->command) {
        case CMD_SET_PARAMETER:
            // Handle parameter setting
            break;
        case CMD_GET_PARAMETER:
            // Handle parameter query
            break;
        case CMD_EXECUTE_ACTION:
            // Handle action execution
            break;
        default:
            break;
    }
}

static void handle_error_command(const can_message_t* message) {
    switch (message->command) {
        case CMD_ERROR_REPORT:
            // Handle error report
            g_stats.errors_detected++;
            break;
        case CMD_ERROR_ACK:
            // Handle error acknowledgment
            break;
        case CMD_ERROR_CLEAR:
            // Handle error clear
            break;
        default:
            break;
    }
}