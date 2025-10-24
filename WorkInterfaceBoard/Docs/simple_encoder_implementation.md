# Simple Encoder Implementation

## Overview
This document describes the simplified encoder implementation that replaces the complex TC (Timer Counter) quadrature decoder with a simple GPIO-based polling approach.

## Why This Approach?

The original implementation used the SAM4E's TC peripheral in quadrature decoder mode, which was:
- Complex to configure correctly
- Prone to configuration errors
- Difficult to debug
- Not working reliably

The new approach uses:
- Simple GPIO polling
- Basic state machine for direction detection
- Easy to understand and debug
- More reliable

## How It Works

### 1. GPIO Configuration
- Encoder A and B pins are configured as inputs with pull-ups
- PA5 (A) and PA1 (B) for Encoder 1
- PA15 (A) and PA16 (B) for Encoder 2
- Enable pins (PD17, PD27) are configured as outputs and set low to enable encoders

### 2. State Machine
The quadrature encoder uses a 4-state system:
- State 0: A=0, B=0
- State 1: A=0, B=1  
- State 2: A=1, B=0
- State 3: A=1, B=1

### 3. Direction Detection
A lookup table determines direction based on state transitions:
- Forward: 00→01→11→10→00
- Reverse: 00→10→11→01→00

### 4. Position and Velocity
- Position is incremented/decremented based on direction
- Velocity is calculated by counting pulses over a time window (100ms)
- Direction is determined by the sign of velocity

## CAN Message Format

Each encoder sends an 8-byte CAN message:
- Byte 0: Direction (0=stopped, 1=forward, 2=reverse)
- Bytes 1-3: Velocity (signed 24-bit, pulses per second)
- Bytes 4-7: Position (signed 32-bit, pulse count)

## CAN IDs
- Encoder 1: 0x130
- Encoder 2: 0x131

## Polling Rate
- 50Hz (20ms intervals)
- This provides good responsiveness while not overloading the CPU

## Advantages
1. **Simple**: Easy to understand and debug
2. **Reliable**: No complex peripheral configuration
3. **Flexible**: Easy to modify or extend
4. **Debuggable**: Can easily add debug output
5. **Robust**: Handles noise and glitches better

## Testing
The implementation includes a test program that verifies the state machine logic works correctly for both forward and reverse movement detection.