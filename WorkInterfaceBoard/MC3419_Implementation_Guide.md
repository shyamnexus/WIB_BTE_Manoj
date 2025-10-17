# MC3419 Accelerometer and Temperature Sensor Implementation

## Overview
This implementation provides a complete solution for reading accelerometer and temperature data from the MC3419 sensor and transmitting it over CAN bus using FreeRTOS tasks.

## Hardware Configuration
- **MC3419 I2C Address**: 0x4C (7-bit address)
- **I2C Pins**: 
  - SDA: PA3
  - SCL: PA4
- **CAN IDs**:
  - Accelerometer: 0x121 (6 bytes: X, Y, Z data)
  - Temperature: 0x122 (2 bytes: temperature data)

## Files Added/Modified

### New Files
1. **`src/i2c0.h`** - I2C driver header
2. **`src/i2c0.c`** - I2C driver implementation
3. **`src/mc3419.h`** - MC3419 accelerometer driver header
4. **`src/mc3419.c`** - MC3419 accelerometer driver implementation

### Modified Files
1. **`src/tasks.c`** - Added accelerometer and temperature tasks
2. **`src/tasks.h`** - Updated task declarations
3. **`src/WIB_Init.c`** - Updated initialization comments
4. **`Release/Makefile`** - Added mc3419.c to build

## Task Implementation

### Combined Task: `task_accelerometer_temperature`
- **Priority**: `tskIDLE_PRIORITY+3`
- **Stack Size**: 1024 bytes
- **Function**: Reads both accelerometer and temperature data from MC3419
- **Sampling Rate**: 100 Hz
- **CAN Messages**:
  - Accelerometer data (ID: 0x121, 6 bytes)
  - Temperature data (ID: 0x122, 2 bytes)

### Individual Tasks (Alternative)
- **`task_accelerometer`**: Accelerometer only (100 Hz)
- **`task_temperature`**: Temperature only (10 Hz)

## Data Format

### Accelerometer Data (CAN ID: 0x121)
```
Byte 0: X-axis LSB
Byte 1: X-axis MSB
Byte 2: Y-axis LSB
Byte 3: Y-axis MSB
Byte 4: Z-axis LSB
Byte 5: Z-axis MSB
```

### Temperature Data (CAN ID: 0x122)
```
Byte 0: Temperature LSB
Byte 1: Temperature MSB
```

## MC3419 Configuration
- **Mode**: Wake mode (active measurement)
- **Sample Rate**: 100 Hz
- **Range**: ±8g
- **Data Format**: 16-bit signed values

## Conversion Functions
- **Acceleration**: `mc3419_convert_accel_to_g()` - Converts raw values to g-force
- **Temperature**: `mc3419_convert_temp_to_celsius()` - Converts raw values to Celsius

## Error Handling
- Automatic retry on initialization failure
- Data validity checking
- I2C communication error handling
- Debug variables for troubleshooting

## Usage
The implementation is automatically integrated into the main application. The combined task `task_accelerometer_temperature` is created in `create_application_tasks()` and will start running when the FreeRTOS scheduler starts.

## Debugging
Debug variables are available for troubleshooting:
- `debug_accel_init_failed` - Accelerometer initialization failure
- `debug_temp_init_failed` - Temperature sensor initialization failure
- `debug_combined_init_failed` - Combined task initialization failure

## Dependencies
- FreeRTOS
- ASF (Atmel Software Framework)
- I2C driver (TWI)
- CAN driver
- System clock and delay functions