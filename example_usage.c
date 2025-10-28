/**
 * @file example_usage.c
 * @brief Example usage of the Custom CAN Protocol
 * @version 1.0
 * @date 2024
 */

#include "custom_can_protocol.h"
#include <stdio.h>
#include <unistd.h>

// Example callback function
void message_received_callback(const can_message_t* message) {
    printf("Received message from node %d: Command=0x%02X, Data=", 
           message->source_id, message->command);
    
    for (int i = 0; i < message->data_length; i++) {
        printf("%02X ", message->data[i]);
    }
    printf("\n");
}

// Example: Master node implementation
void master_node_example(void) {
    printf("=== Master Node Example ===\n");
    
    // Initialize as master node
    if (!can_protocol_init(NODE_ID_MASTER)) {
        printf("Failed to initialize master node\n");
        return;
    }
    
    // Register message callback
    can_protocol_register_callback(message_received_callback);
    
    // Discover nodes on the network
    uint8_t discovered_nodes[16];
    uint8_t node_count = can_protocol_discover_nodes(discovered_nodes, 16);
    printf("Discovered %d nodes\n", node_count);
    
    // Send heartbeat
    can_protocol_send_heartbeat();
    
    // Send a command to a specific node
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    can_message_t response;
    
    if (can_protocol_send_command(CMD_SET_PARAMETER, test_data, 4, 
                                 NODE_ID_SLAVE_START, &response, 1000)) {
        printf("Command sent and response received\n");
    } else {
        printf("Command failed or timeout\n");
    }
    
    // Broadcast a message
    uint8_t broadcast_data[] = {0xFF, 0x00};
    can_protocol_broadcast(CMD_DATA_BROADCAST, broadcast_data, 2);
    
    // Get protocol statistics
    protocol_stats_t stats;
    can_protocol_get_stats(&stats);
    printf("Messages sent: %d, received: %d, errors: %d\n", 
           stats.messages_sent, stats.messages_received, stats.errors_detected);
}

// Example: Slave node implementation
void slave_node_example(void) {
    printf("=== Slave Node Example ===\n");
    
    // Initialize as slave node
    if (!can_protocol_init(NODE_ID_SLAVE_START)) {
        printf("Failed to initialize slave node\n");
        return;
    }
    
    // Register message callback
    can_protocol_register_callback(message_received_callback);
    
    // Send periodic heartbeat
    for (int i = 0; i < 5; i++) {
        can_protocol_send_heartbeat();
        sleep(1);
    }
    
    // Send status information
    uint8_t status_data[] = {0x01, 0x00, 0x00, 0x00}; // Status: OK
    can_protocol_send_command(CMD_SYSTEM_STATUS, status_data, 4, 
                             NODE_ID_MASTER, NULL, 1000);
    
    // Get node information
    node_info_t node_info;
    if (can_protocol_get_node_info(NODE_ID_MASTER, &node_info)) {
        printf("Master node info: ID=%d, Version=%d, Online=%s\n",
               node_info.node_id, node_info.protocol_version,
               node_info.online ? "Yes" : "No");
    }
}

// Example: Data streaming
void data_streaming_example(void) {
    printf("=== Data Streaming Example ===\n");
    
    // Initialize node
    if (!can_protocol_init(0x03)) {
        printf("Failed to initialize node\n");
        return;
    }
    
    // Start data stream
    can_protocol_broadcast(CMD_DATA_STREAM_START, NULL, 0);
    
    // Send streaming data
    for (int i = 0; i < 10; i++) {
        uint8_t stream_data[] = {i, i * 2, i * 3, i * 4};
        can_message_t message;
        message.source_id = 0x03;
        message.dest_id = NODE_ID_BROADCAST;
        message.command = CMD_DATA_STREAM_DATA;
        message.sequence = i;
        message.data_length = 4;
        memcpy(message.data, stream_data, 4);
        message.checksum = can_protocol_calculate_checksum(&message);
        
        can_protocol_send(&message);
        usleep(100000); // 100ms delay
    }
    
    // Stop data stream
    can_protocol_broadcast(CMD_DATA_STREAM_STOP, NULL, 0);
}

// Example: Error handling
void error_handling_example(void) {
    printf("=== Error Handling Example ===\n");
    
    // Initialize node
    if (!can_protocol_init(0x04)) {
        printf("Failed to initialize node\n");
        return;
    }
    
    // Send error report
    uint8_t error_data[] = {ERROR_HARDWARE, 0x01, 0x02, 0x03};
    can_protocol_send_command(CMD_ERROR_REPORT, error_data, 4, 
                             NODE_ID_MASTER, NULL, 1000);
    
    // Send invalid command to test error handling
    can_message_t invalid_message;
    invalid_message.source_id = 0x04;
    invalid_message.dest_id = NODE_ID_MASTER;
    invalid_message.command = 0xFF; // Invalid command
    invalid_message.sequence = 1;
    invalid_message.data_length = 0;
    invalid_message.checksum = 0x00; // Invalid checksum
    
    can_protocol_send(&invalid_message);
    
    // Get error statistics
    protocol_stats_t stats;
    can_protocol_get_stats(&stats);
    printf("Errors detected: %d\n", stats.errors_detected);
}

// Example: Parameter management
void parameter_management_example(void) {
    printf("=== Parameter Management Example ===\n");
    
    // Initialize node
    if (!can_protocol_init(0x05)) {
        printf("Failed to initialize node\n");
        return;
    }
    
    // Set a parameter
    uint8_t param_data[] = {0x01, 0x00, 0x64, 0x00}; // Parameter ID=1, Value=100
    can_message_t response;
    
    if (can_protocol_send_command(CMD_SET_PARAMETER, param_data, 4, 
                                 NODE_ID_MASTER, &response, 1000)) {
        printf("Parameter set successfully\n");
    }
    
    // Get a parameter
    uint8_t get_param_data[] = {0x01, 0x00, 0x00, 0x00}; // Parameter ID=1
    if (can_protocol_send_command(CMD_GET_PARAMETER, get_param_data, 4, 
                                 NODE_ID_MASTER, &response, 1000)) {
        printf("Parameter value: %d\n", response.data[0]);
    }
}

int main(void) {
    printf("Custom CAN Protocol Examples\n");
    printf("============================\n\n");
    
    // Run examples
    master_node_example();
    printf("\n");
    
    slave_node_example();
    printf("\n");
    
    data_streaming_example();
    printf("\n");
    
    error_handling_example();
    printf("\n");
    
    parameter_management_example();
    printf("\n");
    
    printf("Examples completed\n");
    return 0;
}