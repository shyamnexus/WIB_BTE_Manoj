# Encoder CAN Message Format

## Overview
The encoder system transmits data from two encoders over CAN bus at 50Hz. Each encoder sends its data in a separate CAN message.

## CAN Message IDs
- **Encoder 1**: CAN ID `0x130` (304 decimal)
- **Encoder 2**: CAN ID `0x131` (305 decimal)

## Message Format
Each CAN message contains 8 bytes of data:

| Byte | Field | Description | Type |
|------|-------|-------------|------|
| 0-3  | Position | 32-bit position in counts (little-endian) | int32_t |
| 4-6  | Speed | 24-bit speed in counts per second (little-endian) | uint24_t |
| 7    | Direction | Direction indicator | uint8_t |

## Direction Values
- `0` = Stopped (speed < 5 counts/sec)
- `1` = Forward (positive velocity)
- `2` = Reverse (negative velocity)

## Data Interpretation

### Position
- 32-bit signed integer representing the current encoder position
- Increments for forward rotation, decrements for reverse rotation
- Can be used to track absolute position or calculate relative movement

### Speed
- 24-bit unsigned integer representing absolute speed in counts per second
- Always positive (0 to 16,777,215 counts/sec)
- Calculated as absolute value of velocity

### Direction
- 8-bit value indicating movement direction
- Provides quick identification of movement state without calculating sign

## Example CAN Message Decoding

### Encoder 1 Message (ID: 0x130)
```
Raw Data: [0x34, 0x12, 0x00, 0x00, 0x64, 0x00, 0x00, 0x01]

Decoded:
- Position: 0x00001234 = 4660 counts
- Speed: 0x000064 = 100 counts/sec
- Direction: 0x01 = Forward
```

### Encoder 2 Message (ID: 0x131)
```
Raw Data: [0xCC, 0xFF, 0xFF, 0xFF, 0x32, 0x00, 0x00, 0x02]

Decoded:
- Position: 0xFFFFFFCC = -52 counts (signed)
- Speed: 0x000032 = 50 counts/sec
- Direction: 0x02 = Reverse
```

## Usage Notes
- Messages are transmitted at 50Hz (every 20ms)
- Position is cumulative and can overflow after extended operation
- Speed calculation has a 5 counts/sec threshold to avoid noise
- Direction is determined from the sign of velocity, not position change
- All multi-byte values are transmitted in little-endian format

## Integration with Other Systems
- Position data can be used for closed-loop control
- Speed data is useful for velocity control and monitoring
- Direction data enables quick state determination for UI or safety systems
- The 8-byte format fits standard CAN frame size efficiently