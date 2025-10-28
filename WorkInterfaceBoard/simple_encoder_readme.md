# Simple Encoder Implementation

This implementation provides a simple quadrature encoder interface for PA0 (TIOA0) and PA1 (TIOB0) pins on the SAM4E microcontroller.

## Features

- **Single Encoder Support**: Only handles one encoder using PA0 and PA1 pins
- **Interrupt-Driven**: Uses PIO interrupts for responsive quadrature decoding
- **Direction Detection**: Automatically detects forward/reverse rotation
- **Velocity Calculation**: Calculates velocity in pulses per second
- **CAN Communication**: Sends encoder data over CAN bus
- **Simple API**: Easy-to-use functions for encoder control

## Files

- `simple_encoder.c` - Main encoder implementation
- `simple_encoder.h` - Header file with definitions and function prototypes
- `test_simple_encoder.c` - Simple test program demonstrating usage

## Pin Configuration

- **PA0 (TIOA0)**: Encoder A signal (Channel A)
- **PA1 (TIOB0)**: Encoder B signal (Channel B)

## CAN Message Format

### Direction, Velocity, and Position (ID: 0x130)
- **Byte 0**: Direction (0=stopped, 1=forward, 2=reverse)
- **Bytes 1-3**: Velocity (signed 24-bit, pulses per second)
- **Bytes 4-7**: Position (signed 32-bit, pulse count)

### Pin States (ID: 0x188)
- **Byte 0**: Pin A state (0 or 1)
- **Byte 1**: Pin B state (0 or 1)

## Usage

### Basic Usage

```c
#include "simple_encoder.h"

// Initialize encoder
if (!simple_encoder_init()) {
    // Handle initialization error
}

// Poll encoder data
simple_encoder_poll();

// Get encoder data
simple_encoder_data_t* enc_data = simple_encoder_get_data();
printf("Position: %ld, Velocity: %ld, Direction: %d\n", 
       enc_data->position, enc_data->velocity, enc_data->direction);
```

### FreeRTOS Task Usage

```c
// Create encoder task
xTaskCreate(simple_encoder_task, "encoder", 512, 0, tskIDLE_PRIORITY+2, 0);
```

## Configuration

### Polling Rate
- Default: 50ms (20Hz)
- Modify `ENCODER_POLLING_RATE_MS` in `simple_encoder.c`

### Velocity Calculation Window
- Default: 200ms
- Modify `VELOCITY_WINDOW_MS` in `simple_encoder.c`

## Quadrature Decoding

The encoder uses a state table for quadrature decoding:

- **Forward**: 00→01→11→10→00
- **Reverse**: 00→10→11→01→00

Each state transition increments or decrements the position counter based on the direction.

## Interrupt Handling

The encoder uses PIO interrupts on both PA0 and PA1 pins for responsive quadrature decoding. The interrupt handler:

1. Reads current pin states
2. Calculates direction using state table
3. Updates position counter
4. Calculates velocity
5. Sends immediate CAN message for pin state changes

## Error Handling

- Initialization failures are handled gracefully
- Encoder data is validated before transmission
- Timeout detection for stopped state (100ms)

## Dependencies

- ASF (Atmel Software Framework)
- FreeRTOS
- CAN application layer (`can_app.h`)

## Example Output

When the encoder is rotating forward:
- Direction: 1
- Velocity: Positive value (pulses per second)
- Position: Incrementing counter

When the encoder is rotating reverse:
- Direction: 2
- Velocity: Negative value (pulses per second)
- Position: Decrementing counter

When the encoder is stopped:
- Direction: 0
- Velocity: 0
- Position: Last known position
