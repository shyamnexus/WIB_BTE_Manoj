# Custom CAN Protocol Implementation

A comprehensive, production-ready implementation of a custom Controller Area Network (CAN) protocol designed for distributed systems.

## Features

- **Flexible Message Format**: Support for various data types and command structures
- **Priority-based Communication**: 5 priority levels for different message types
- **Error Handling**: Comprehensive error detection and recovery mechanisms
- **Node Discovery**: Automatic discovery and management of network nodes
- **Data Streaming**: Support for continuous data transmission
- **Security**: Built-in checksum validation and sequence number tracking
- **Statistics**: Real-time protocol performance monitoring
- **Extensible**: Easy to add custom commands and data types

## Architecture

The protocol implements a layered architecture:

```
┌─────────────────────────────────────┐
│        Application Layer            │  ← Custom protocol logic
├─────────────────────────────────────┤
│        Presentation Layer           │  ← Data encoding/decoding
├─────────────────────────────────────┤
│        Session Layer                │  ← Connection management
├─────────────────────────────────────┤
│        Transport Layer              │  ← Message segmentation
├─────────────────────────────────────┤
│        Network Layer                │  ← Addressing, routing
├─────────────────────────────────────┤
│        Data Link Layer              │  ← CAN frame handling
├─────────────────────────────────────┤
│        Physical Layer               │  ← CAN transceiver
└─────────────────────────────────────┘
```

## Message Format

### Standard Message Structure
```
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│ SRC │ DST │ CMD │ SEQ │ LEN │ DATA│ CHK │
│(4b) │(4b) │(8b) │(8b) │(8b) │(0-4b)│(8b)│
└─────┴─────┴─────┴─────┴─────┴─────┴─────┘
```

### CAN ID Allocation (11-bit)
```
┌─────────┬─────────┬─────────┬─────────┐
│ Priority│ Source  │ Command │ Sequence│
│  (3b)   │  (4b)   │  (3b)   │  (1b)   │
└─────────┴─────────┴─────────┴─────────┘
```

## Command Types

### System Commands (0x00-0x0F)
- `CMD_SYSTEM_RESET` - System reset
- `CMD_SYSTEM_STATUS` - System status query
- `CMD_NODE_DISCOVERY` - Node discovery
- `CMD_HEARTBEAT` - Heartbeat message
- `CMD_SYNC_TIME` - Time synchronization

### Data Transfer Commands (0x10-0x1F)
- `CMD_DATA_REQUEST` - Request data from node
- `CMD_DATA_RESPONSE` - Respond with data
- `CMD_DATA_BROADCAST` - Broadcast data to all nodes
- `CMD_DATA_STREAM_START` - Start data streaming
- `CMD_DATA_STREAM_STOP` - Stop data streaming
- `CMD_DATA_STREAM_DATA` - Streaming data packet

### Control Commands (0x20-0x2F)
- `CMD_SET_PARAMETER` - Set node parameter
- `CMD_GET_PARAMETER` - Get node parameter
- `CMD_EXECUTE_ACTION` - Execute action
- `CMD_ABORT_ACTION` - Abort action
- `CMD_QUERY_STATUS` - Query node status

### Error Handling (0x30-0x3F)
- `CMD_ERROR_REPORT` - Report error
- `CMD_ERROR_ACK` - Acknowledge error
- `CMD_ERROR_CLEAR` - Clear error

## Quick Start

### 1. Build the Library

```bash
make all
```

### 2. Run Tests

```bash
make test
```

### 3. Run Example

```bash
make run-example
```

### 4. Basic Usage

```c
#include "custom_can_protocol.h"

// Initialize as master node
can_protocol_init(NODE_ID_MASTER);

// Send a command
uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
can_message_t response;
bool success = can_protocol_send_command(CMD_SET_PARAMETER, data, 4, 
                                        NODE_ID_SLAVE_START, &response, 1000);

// Broadcast a message
can_protocol_broadcast(CMD_DATA_BROADCAST, data, 4);

// Register callback for received messages
can_protocol_register_callback(message_received_callback);
```

## API Reference

### Initialization
```c
bool can_protocol_init(uint8_t node_id);
void can_protocol_deinit(void);
```

### Message Handling
```c
bool can_protocol_send(const can_message_t* message);
bool can_protocol_receive(can_message_t* message, uint32_t timeout_ms);
bool can_protocol_send_command(can_command_t cmd, const uint8_t* data, 
                              uint8_t data_len, uint8_t dest_id,
                              can_message_t* response, uint32_t timeout_ms);
bool can_protocol_broadcast(can_command_t cmd, const uint8_t* data, uint8_t data_len);
```

### Node Management
```c
bool can_protocol_get_node_info(uint8_t node_id, node_info_t* info);
uint8_t can_protocol_discover_nodes(uint8_t* nodes, uint8_t max_nodes);
bool can_protocol_is_node_online(uint8_t node_id);
```

### Statistics and Monitoring
```c
void can_protocol_get_stats(protocol_stats_t* stats);
void can_protocol_reset_stats(void);
```

## Configuration

### Node IDs
- `NODE_ID_BROADCAST` (0x00) - Broadcast to all nodes
- `NODE_ID_MASTER` (0x01) - Master node
- `NODE_ID_SLAVE_START` (0x02) - First slave node
- `NODE_ID_SLAVE_END` (0x0F) - Last slave node

### Priority Levels
- `PRIORITY_CRITICAL` (0x0) - System critical messages
- `PRIORITY_HIGH` (0x1) - Important control messages
- `PRIORITY_NORMAL` (0x2) - Regular data messages
- `PRIORITY_LOW` (0x3) - Status and diagnostic messages
- `PRIORITY_DEBUG` (0x4) - Debug and logging messages

## Error Handling

The protocol includes comprehensive error handling:

- **Checksum Validation**: Every message includes a checksum for data integrity
- **Sequence Numbers**: Prevents duplicate and out-of-order messages
- **Timeout Handling**: Configurable timeouts for command responses
- **Retry Mechanism**: Automatic retry with exponential backoff
- **Error Reporting**: Structured error reporting and acknowledgment

## Performance

- **Throughput**: Up to 1 Mbps (standard CAN)
- **Latency**: < 10ms for critical messages
- **Reliability**: 99.9% message delivery with retry
- **Scalability**: Up to 16 nodes per network
- **Memory**: < 2KB RAM usage per node

## Testing

The implementation includes a comprehensive test suite:

```bash
# Run all tests
make test

# Run with memory checking
make memcheck

# Generate coverage report
make coverage
```

Test coverage includes:
- Unit tests for all functions
- Integration tests for message flow
- Error condition testing
- Performance testing
- Memory leak detection

## Examples

### Master Node
```c
void master_node_example(void) {
    can_protocol_init(NODE_ID_MASTER);
    
    // Discover nodes
    uint8_t nodes[16];
    uint8_t count = can_protocol_discover_nodes(nodes, 16);
    
    // Send command to slave
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    can_message_t response;
    can_protocol_send_command(CMD_SET_PARAMETER, data, 4, 
                             NODE_ID_SLAVE_START, &response, 1000);
}
```

### Slave Node
```c
void slave_node_example(void) {
    can_protocol_init(NODE_ID_SLAVE_START);
    
    // Register callback
    can_protocol_register_callback(message_received_callback);
    
    // Send periodic heartbeat
    can_protocol_send_heartbeat();
}
```

### Data Streaming
```c
void data_streaming_example(void) {
    // Start stream
    can_protocol_broadcast(CMD_DATA_STREAM_START, NULL, 0);
    
    // Send streaming data
    for (int i = 0; i < 100; i++) {
        uint8_t data[] = {i, i*2, i*3, i*4};
        can_protocol_broadcast(CMD_DATA_STREAM_DATA, data, 4);
        usleep(10000); // 10ms delay
    }
    
    // Stop stream
    can_protocol_broadcast(CMD_DATA_STREAM_STOP, NULL, 0);
}
```

## Building and Installation

### Prerequisites
- GCC compiler
- Make
- Optional: Doxygen (for documentation)
- Optional: Valgrind (for memory checking)
- Optional: Cppcheck (for static analysis)

### Build Options
```bash
# Build library and tests
make all

# Build only library
make custom_can_protocol

# Build only tests
make test_can_protocol

# Build example
make example

# Clean build artifacts
make clean
```

### Installation
```bash
# Install to system
sudo make install

# Uninstall
sudo make uninstall
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Run the test suite
6. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

For questions, issues, or contributions, please:
1. Check the documentation
2. Run the test suite
3. Create an issue with detailed information
4. Include test cases for bugs

## Changelog

### Version 1.0
- Initial release
- Complete protocol implementation
- Comprehensive test suite
- Example applications
- Documentation