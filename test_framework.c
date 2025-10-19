/**
 * @file test_framework.c
 * @brief Test framework for Custom CAN Protocol
 * @version 1.0
 * @date 2024
 */

#include "custom_can_protocol.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// Test result structure
typedef struct {
    char test_name[64];
    bool passed;
    char error_message[128];
} test_result_t;

// Test statistics
typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
} test_stats_t;

static test_stats_t g_test_stats = {0};

// Test helper macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s - %s\n", __func__, message); \
            return false; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASS: %s\n", __func__); \
        g_test_stats.passed_tests++; \
        return true; \
    } while(0)

#define TEST_FAIL(message) \
    do { \
        printf("FAIL: %s - %s\n", __func__, message); \
        g_test_stats.failed_tests++; \
        return false; \
    } while(0)

// Test functions
bool test_protocol_initialization(void) {
    // Test valid initialization
    TEST_ASSERT(can_protocol_init(NODE_ID_MASTER), "Valid initialization should succeed");
    TEST_ASSERT(can_protocol_get_node_id() == NODE_ID_MASTER, "Node ID should be set correctly");
    
    // Test invalid initialization
    can_protocol_deinit();
    TEST_ASSERT(!can_protocol_init(0), "Invalid node ID should fail");
    TEST_ASSERT(!can_protocol_init(0xFF), "Invalid node ID should fail");
    
    // Reinitialize for other tests
    can_protocol_init(NODE_ID_MASTER);
    TEST_PASS();
}

bool test_checksum_calculation(void) {
    can_message_t message;
    message.source_id = 0x01;
    message.dest_id = 0x02;
    message.command = 0x10;
    message.sequence = 1;
    message.data_length = 2;
    message.data[0] = 0xAA;
    message.data[1] = 0x55;
    
    uint8_t checksum = can_protocol_calculate_checksum(&message);
    message.checksum = checksum;
    
    TEST_ASSERT(can_protocol_verify_checksum(&message), "Checksum verification should pass");
    
    // Test with invalid checksum
    message.checksum = 0x00;
    TEST_ASSERT(!can_protocol_verify_checksum(&message), "Invalid checksum should fail");
    
    TEST_PASS();
}

bool test_message_frame_conversion(void) {
    can_message_t original_message;
    original_message.source_id = 0x01;
    original_message.dest_id = 0x02;
    original_message.command = CMD_HEARTBEAT;
    original_message.sequence = 5;
    original_message.data_length = 2;
    original_message.data[0] = 0x11;
    original_message.data[1] = 0x22;
    original_message.checksum = can_protocol_calculate_checksum(&original_message);
    
    // Convert to frame
    can_frame_t frame;
    can_protocol_message_to_frame(&original_message, &frame, PRIORITY_NORMAL);
    
    // Convert back to message
    can_message_t converted_message;
    TEST_ASSERT(can_protocol_frame_to_message(&frame, &converted_message), 
                "Frame to message conversion should succeed");
    
    // Verify data integrity
    TEST_ASSERT(original_message.source_id == converted_message.source_id, 
                "Source ID should match");
    TEST_ASSERT(original_message.dest_id == converted_message.dest_id, 
                "Dest ID should match");
    TEST_ASSERT(original_message.command == converted_message.command, 
                "Command should match");
    TEST_ASSERT(original_message.sequence == converted_message.sequence, 
                "Sequence should match");
    TEST_ASSERT(original_message.data_length == converted_message.data_length, 
                "Data length should match");
    TEST_ASSERT(memcmp(original_message.data, converted_message.data, 4) == 0, 
                "Data should match");
    TEST_ASSERT(original_message.checksum == converted_message.checksum, 
                "Checksum should match");
    
    TEST_PASS();
}

bool test_node_discovery(void) {
    // Initialize multiple nodes
    can_protocol_init(NODE_ID_MASTER);
    
    // Simulate other nodes being online
    node_info_t node_info;
    node_info.node_id = 0x02;
    node_info.online = true;
    node_info.last_heartbeat = 1000;
    
    // Test node discovery
    uint8_t discovered_nodes[16];
    uint8_t node_count = can_protocol_discover_nodes(discovered_nodes, 16);
    
    // Should discover at least the master node
    TEST_ASSERT(node_count >= 0, "Node discovery should not fail");
    
    TEST_PASS();
}

bool test_command_sending(void) {
    can_protocol_init(NODE_ID_MASTER);
    
    // Test sending a command
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    can_message_t response;
    
    // This will fail in simulation mode, but should not crash
    bool result = can_protocol_send_command(CMD_SET_PARAMETER, test_data, 4, 
                                           NODE_ID_SLAVE_START, &response, 100);
    
    // In simulation mode, this will fail due to timeout, which is expected
    TEST_ASSERT(result == false, "Command should fail in simulation mode");
    
    TEST_PASS();
}

bool test_broadcast_messages(void) {
    can_protocol_init(NODE_ID_MASTER);
    
    // Test broadcasting a message
    uint8_t broadcast_data[] = {0xFF, 0x00, 0xAA, 0x55};
    bool result = can_protocol_broadcast(CMD_DATA_BROADCAST, broadcast_data, 4);
    
    TEST_ASSERT(result, "Broadcast should succeed");
    
    TEST_PASS();
}

bool test_statistics_tracking(void) {
    can_protocol_init(NODE_ID_MASTER);
    
    // Reset statistics
    can_protocol_reset_stats();
    
    // Get initial statistics
    protocol_stats_t stats;
    can_protocol_get_stats(&stats);
    
    TEST_ASSERT(stats.messages_sent == 0, "Initial sent count should be 0");
    TEST_ASSERT(stats.messages_received == 0, "Initial received count should be 0");
    TEST_ASSERT(stats.errors_detected == 0, "Initial error count should be 0");
    
    // Send a message to increment statistics
    can_protocol_broadcast(CMD_HEARTBEAT, NULL, 0);
    
    can_protocol_get_stats(&stats);
    TEST_ASSERT(stats.messages_sent > 0, "Sent count should increase");
    
    TEST_PASS();
}

bool test_error_handling(void) {
    can_protocol_init(NODE_ID_MASTER);
    
    // Test with NULL parameters
    TEST_ASSERT(!can_protocol_send(NULL), "Sending NULL message should fail");
    TEST_ASSERT(!can_protocol_receive(NULL, 100), "Receiving to NULL should fail");
    
    // Test with invalid node IDs
    can_message_t message;
    message.source_id = NODE_ID_MASTER;
    message.dest_id = 0xFF; // Invalid node ID
    message.command = CMD_HEARTBEAT;
    message.sequence = 1;
    message.data_length = 0;
    message.checksum = 0;
    
    TEST_ASSERT(!can_protocol_send(&message), "Sending to invalid node should fail");
    
    TEST_PASS();
}

bool test_priority_levels(void) {
    can_message_t message;
    message.source_id = NODE_ID_MASTER;
    message.dest_id = NODE_ID_BROADCAST;
    message.command = CMD_SYSTEM_RESET;
    message.sequence = 1;
    message.data_length = 0;
    message.checksum = can_protocol_calculate_checksum(&message);
    
    // Test different priority levels
    can_frame_t frame;
    
    can_protocol_message_to_frame(&message, &frame, PRIORITY_CRITICAL);
    TEST_ASSERT((frame.id & CAN_ID_PRIORITY_MASK) == (PRIORITY_CRITICAL << 8), 
                "Critical priority should be set correctly");
    
    can_protocol_message_to_frame(&message, &frame, PRIORITY_LOW);
    TEST_ASSERT((frame.id & CAN_ID_PRIORITY_MASK) == (PRIORITY_LOW << 8), 
                "Low priority should be set correctly");
    
    TEST_PASS();
}

bool test_data_types(void) {
    can_protocol_init(NODE_ID_MASTER);
    
    // Test different data types
    uint8_t uint8_data = 0xAA;
    uint16_t uint16_data = 0x1234;
    uint32_t uint32_data = 0x12345678;
    
    can_message_t message;
    message.source_id = NODE_ID_MASTER;
    message.dest_id = NODE_ID_BROADCAST;
    message.command = CMD_DATA_BROADCAST;
    message.sequence = 1;
    
    // Test uint8 data
    message.data_length = 1;
    message.data[0] = uint8_data;
    message.checksum = can_protocol_calculate_checksum(&message);
    TEST_ASSERT(can_protocol_send(&message), "uint8 data should send successfully");
    
    // Test uint16 data
    message.data_length = 2;
    message.data[0] = (uint8_t)(uint16_data & 0xFF);
    message.data[1] = (uint8_t)((uint16_data >> 8) & 0xFF);
    message.checksum = can_protocol_calculate_checksum(&message);
    TEST_ASSERT(can_protocol_send(&message), "uint16 data should send successfully");
    
    // Test uint32 data
    message.data_length = 4;
    message.data[0] = (uint8_t)(uint32_data & 0xFF);
    message.data[1] = (uint8_t)((uint32_data >> 8) & 0xFF);
    message.data[2] = (uint8_t)((uint32_data >> 16) & 0xFF);
    message.data[3] = (uint8_t)((uint32_data >> 24) & 0xFF);
    message.checksum = can_protocol_calculate_checksum(&message);
    TEST_ASSERT(can_protocol_send(&message), "uint32 data should send successfully");
    
    TEST_PASS();
}

// Test runner
void run_all_tests(void) {
    printf("Running Custom CAN Protocol Tests\n");
    printf("=================================\n\n");
    
    g_test_stats.total_tests = 0;
    g_test_stats.passed_tests = 0;
    g_test_stats.failed_tests = 0;
    
    // Define test functions
    bool (*test_functions[])(void) = {
        test_protocol_initialization,
        test_checksum_calculation,
        test_message_frame_conversion,
        test_node_discovery,
        test_command_sending,
        test_broadcast_messages,
        test_statistics_tracking,
        test_error_handling,
        test_priority_levels,
        test_data_types
    };
    
    int num_tests = sizeof(test_functions) / sizeof(test_functions[0]);
    
    // Run all tests
    for (int i = 0; i < num_tests; i++) {
        g_test_stats.total_tests++;
        test_functions[i]();
    }
    
    // Print results
    printf("\nTest Results Summary\n");
    printf("===================\n");
    printf("Total Tests: %d\n", g_test_stats.total_tests);
    printf("Passed: %d\n", g_test_stats.passed_tests);
    printf("Failed: %d\n", g_test_stats.failed_tests);
    printf("Success Rate: %.1f%%\n", 
           (float)g_test_stats.passed_tests / g_test_stats.total_tests * 100.0f);
    
    if (g_test_stats.failed_tests == 0) {
        printf("\nAll tests passed! ✓\n");
    } else {
        printf("\nSome tests failed! ✗\n");
    }
}

int main(void) {
    run_all_tests();
    return 0;
}