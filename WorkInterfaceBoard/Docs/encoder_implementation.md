# Encoder Implementation

This document describes the encoder implementation for the Work Interface Board.

## Overview

The encoder implementation provides GPIO-based quadrature encoder reading for two encoders without using interrupts. It reads encoder position, velocity, and direction, then transmits the data over CAN.

## Hardware Configuration

Based on the pin configuration file, the following pins are used:

### Encoder 1
- **A Channel**: PA5 (TIOA0) - Input with pull-up
- **B Channel**: PA1 (TIOB0) - Input with pull-up  
- **Enable**: PD17 - Output (active high)

### Encoder 2
- **A Channel**: PA15 (TIOA1) - Input with pull-up
- **B Channel**: PA16 (TIOB1) - Input with pull-up
- **Enable**: PD27 - Output (active high)

## Software Implementation

### Files
- `src/encoder.h` - Header file with function declarations and data structures
- `src/encoder.c` - Implementation of encoder reading functions
- `src/tasks.c` - Updated to include encoder task
- `src/can_app.h` - Updated with encoder CAN IDs

### Key Features

1. **Quadrature Decoding**: Uses a lookup table for efficient state transition decoding
2. **No Interrupts**: Polling-based implementation for simplicity
3. **Direction Detection**: Tracks forward/reverse movement
4. **Velocity Calculation**: Position change per sample period
5. **CAN Transmission**: Sends data at 1000 Hz (1ms period)

### CAN Message Format

#### Encoder 1 (CAN ID: 0x124)
- Bytes 0-3: Position (32-bit signed integer, little-endian)
- Bytes 4-5: Velocity (16-bit signed integer, little-endian)
- Byte 6: Status (bit 0 = direction, bit 1 = enabled)
- Byte 7: Sample count (8-bit, wraps at 255)

#### Encoder 2 (CAN ID: 0x125)
- Same format as Encoder 1

### Quadrature Decoding

The implementation uses a 16-entry lookup table to decode quadrature signals:
- Index: (last_state << 2) | current_state
- Value: position change (-1, 0, +1)
- Handles invalid transitions gracefully (returns 0)

### Usage

1. Call `encoder_init()` during system initialization
2. The encoder task runs automatically and transmits data over CAN
3. Use `encoder_enable_encoder1()` and `encoder_enable_encoder2()` to control enable pins
4. Read individual encoder data using `encoder_read_encoder1()` and `encoder_read_encoder2()`

### Performance

- Sampling rate: 1000 Hz (1ms period)
- CPU usage: Minimal (simple GPIO reads and table lookup)
- Memory usage: ~100 bytes for state variables
- CAN bandwidth: 2 messages × 8 bytes × 1000 Hz = 16 kbps

### Testing

The implementation includes debug features:
- Status messages sent every 1000 iterations
- Sample counters for each encoder
- Enable/disable status tracking

## Integration

The encoder task is automatically created in `create_application_tasks()` and runs at priority level 3 (higher than other tasks for responsive reading).