# Encoder Implementation

## Overview
This implementation provides encoder functionality without using interrupts, polling the encoder pins at regular intervals and sending direction and velocity data over CAN.

## Hardware Configuration
Based on `pinconfig_workhead_interface_pinconfig.csv`:

### Encoder 1 (Available)
- **ENC1_A**: PA5 (TIOA0) - Encoder A channel
- **ENC1_B**: PA1 (TIOB0) - Encoder B channel  
- **ENC1_ENABLE**: PD17 - Encoder enable line

### Encoder 2 (Partially Available)
- **ENC2_A**: PA15 (TIOA1) - **NOT AVAILABLE** (conflicts with SPI DRDY)
- **ENC2_B**: PA16 (TIOB1) - Available
- **ENC2_ENABLE**: PD27 - Encoder enable line

**Note**: Encoder 2 is disabled due to PA15 being used for SPI DRDY functionality.

## Software Implementation

### Files
- `src/encoder.h` - Header file with definitions and function prototypes
- `src/encoder.c` - Implementation file with polling logic and CAN transmission

### Key Features
1. **Polling-based**: No interrupts used, polls encoder pins every 10ms
2. **Quadrature decoding**: Properly decodes A/B channel signals for direction detection
3. **Velocity calculation**: Calculates velocity over 100ms windows in degrees per second
4. **CAN transmission**: Sends direction and velocity data over CAN bus

### CAN Message Format
- **CAN ID 0x130**: Encoder 1 direction and velocity
- **CAN ID 0x131**: Encoder 2 direction and velocity (disabled)

#### Message Data (6 bytes):
- Byte 0: Direction (0=stopped, 1=forward, 2=reverse)
- Bytes 1-4: Velocity (32-bit signed integer, degrees per second)
- Byte 5: Position (8-bit position data)

### Configuration
- Polling rate: 10ms
- Velocity calculation window: 100ms
- Encoder pulses per revolution: 1952 (122 poles × 64 interpolation ÷ 4)
- Velocity output: degrees per second

### FreeRTOS Task
The encoder functionality runs in a dedicated FreeRTOS task (`encoder_task`) that:
1. Initializes encoder pins
2. Polls encoder states
3. Calculates direction and velocity
4. Transmits data over CAN
5. Repeats every 10ms

## Usage
The encoder task is automatically created when the application starts. No additional configuration is required - the encoders will begin polling and transmitting data immediately.

## Limitations
- Only Encoder 1 is fully functional due to pin conflicts
- Encoder 2 is disabled (PA15 conflict with SPI DRDY)
- Polling-based approach may miss very fast encoder movements
- Velocity calculation has 100ms resolution