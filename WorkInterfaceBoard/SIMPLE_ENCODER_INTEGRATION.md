# Simple Encoder Integration Guide

## Overview
This simple encoder implementation uses your existing CAN code and focuses only on PA0 (TIOA0) and PA1 (TIOB0) pins.

## Files
- `src/simple_encoder.c` - Main encoder implementation
- `src/simple_encoder.h` - Header file
- `test_simple_encoder.c` - Test program

## CAN Message IDs Used
- **CAN_ID_ENCODER1_DIR_VEL (0x130)**: Direction, velocity, and position data
- **CAN_ID_ENCODER1_PINS (0x188)**: Pin states (A and B pins)

## Integration Steps

### 1. Replace the encoder task in tasks.c
```c
// In tasks.c, replace the encoder_task with simple_encoder_task
xTaskCreate(simple_encoder_task, "encoder", 512, 0, tskIDLE_PRIORITY+2, 0);
```

### 2. Include the simple encoder header
```c
#include "simple_encoder.h"
```

### 3. Build with the simple encoder files
Add `src/simple_encoder.c` to your build configuration.

## Usage
The simple encoder will automatically:
- Read PA0 and PA1 pins
- Calculate direction and velocity
- Send data over CAN using existing message IDs
- Handle quadrature decoding with interrupts

## Message Format
- **Direction**: 0=stopped, 1=forward, 2=reverse
- **Velocity**: Signed 24-bit value (pulses per second)
- **Position**: Signed 32-bit value (pulse count)
