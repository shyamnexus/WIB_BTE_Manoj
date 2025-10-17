# Sensor Integration Documentation

## Overview
This document describes the integration of accelerometer and temperature sensors with the existing CAN communication system for the Work Interface Board.

## Hardware Configuration

### Pin Assignment (from PinMUX file)
- **I2C SCL**: PA4 (Pin 55) - I2C clock line
- **I2C SDA**: PA3 (Pin 64) - I2C data line  
- **Accelerometer Control 1**: PA19 (Pin 14) - ACCEL_CTRL_1
- **Accelerometer Control 2**: PA20 (Pin 13) - ACCEL_CTRL_2

### Sensor Details
- **Device**: LIS2DH (STMicroelectronics)
- **Interface**: I2C (100 kHz)
- **Address**: 0x19 (7-bit)
- **Features**: 3-axis accelerometer + temperature sensor

## Software Architecture

### New Files Added
1. **i2c0.h/c** - I2C driver for SAM4E
2. **lis2dh.h/c** - LIS2DH sensor driver
3. **sensor_test.h/c** - Sensor testing and validation functions

### Modified Files
1. **tasks.c/h** - Added sensor tasks
2. **TIB_Init.c** - Added sensor initialization
3. **main.c** - Added sensor testing

## CAN Message Format

### Accelerometer Data (CAN ID: 0x121)
- **Length**: 6 bytes
- **Format**: 
  - Byte 0-1: X-axis (16-bit signed integer, little-endian)
  - Byte 2-3: Y-axis (16-bit signed integer, little-endian)
  - Byte 4-5: Z-axis (16-bit signed integer, little-endian)
- **Units**: Raw ADC values (converted to g in software)
- **Range**: ±2g (configurable)

### Temperature Data (CAN ID: 0x122)
- **Length**: 2 bytes
- **Format**: 16-bit signed integer (little-endian)
- **Units**: Raw ADC value (converted to °C in software)
- **Range**: -40°C to +85°C (typical)

## Task Configuration

### Available Tasks
1. **task_accelerometer()** - Reads only accelerometer data (20Hz)
2. **task_temperature()** - Reads only temperature data (1Hz)
3. **task_accelerometer_temperature()** - Combined task (10Hz) - **Recommended**

### Task Selection
In `tasks.c`, choose one of the following approaches:

```c
// Option 1: Separate tasks
xTaskCreate(task_accelerometer, "accel", 512, 0, tskIDLE_PRIORITY+2, 0);
xTaskCreate(task_temperature, "temp", 256, 0, tskIDLE_PRIORITY+2, 0);

// Option 2: Combined task (recommended)
xTaskCreate(task_accelerometer_temperature, "accel_temp", 512, 0, tskIDLE_PRIORITY+2, 0);
```

## Configuration Parameters

### Accelerometer Settings
- **Full Scale**: ±2g (configurable: ±2g, ±4g, ±8g, ±16g)
- **Output Data Rate**: 100Hz (configurable: 1Hz to 5376Hz)
- **Resolution**: High resolution mode

### Temperature Sensor
- **Enabled**: Yes
- **Sensitivity**: 1 LSB/°C
- **Formula**: Temperature = (raw_value / 8) + 25°C

## Testing and Validation

### Built-in Tests
1. **Connection Test**: `sensor_test_lis2dh_connection()`
2. **Accelerometer Test**: `sensor_test_accelerometer_reading()`
3. **Temperature Test**: `sensor_test_temperature_reading()`

### Debug Variables
The following volatile variables are available for debugging:
- `debug_i2c_init_failed` - I2C initialization status
- `debug_lis2dh_init_failed` - LIS2DH initialization status
- `debug_lis2dh_verify_failed` - LIS2DH connection verification
- `debug_accel_test_failed` - Accelerometer test status
- `debug_temp_test_failed` - Temperature test status

## Usage Example

### Reading Sensor Data
```c
lis2dh_accel_data_t accel_data;
lis2dh_temp_data_t temp_data;

if (lis2dh_read_accelerometer_and_temperature(&accel_data, &temp_data)) {
    // Use accel_data.x_g, accel_data.y_g, accel_data.z_g for acceleration in g
    // Use temp_data.celsius for temperature in Celsius
}
```

### Sending CAN Messages
```c
// Accelerometer data
uint8_t accel_payload[6];
accel_payload[0] = (uint8_t)(accel_data.x & 0xFF);
accel_payload[1] = (uint8_t)((accel_data.x >> 8) & 0xFF);
// ... pack Y and Z similarly
can_app_tx(CAN_ID_ACCELEROMETER, accel_payload, 6);

// Temperature data
uint8_t temp_payload[2];
temp_payload[0] = (uint8_t)(temp_data.raw & 0xFF);
temp_payload[1] = (uint8_t)((temp_data.raw >> 8) & 0xFF);
can_app_tx(CAN_ID_TEMPERATURE, temp_payload, 2);
```

## Troubleshooting

### Common Issues
1. **I2C Initialization Failed**: Check pin configuration and clock settings
2. **LIS2DH Connection Failed**: Verify I2C address and wiring
3. **Sensor Reading Failed**: Check sensor configuration and power supply
4. **CAN Transmission Failed**: Verify CAN bus configuration

### Debug Steps
1. Enable sensor diagnostics task: `xTaskCreate(task_sensor_diagnostics, ...)`
2. Check debug variables in debugger
3. Use `sensor_test_print_diagnostics()` for real-time monitoring
4. Verify CAN bus with oscilloscope or CAN analyzer

## Performance Considerations

### Sampling Rates
- **Accelerometer**: 20Hz (separate task) or 10Hz (combined task)
- **Temperature**: 1Hz (separate task) or 10Hz (combined task)
- **Load Cell**: 10Hz (existing)

### Memory Usage
- **I2C Driver**: ~2KB flash, ~100 bytes RAM
- **LIS2DH Driver**: ~3KB flash, ~50 bytes RAM
- **Sensor Tasks**: ~1KB flash, ~200 bytes RAM each

### Power Consumption
- **LIS2DH Active**: ~10µA (100Hz ODR)
- **I2C Communication**: ~1mA during transfers
- **Overall Impact**: Minimal (<1% of total system power)

## Future Enhancements

### Potential Improvements
1. **Interrupt-driven sampling** instead of polling
2. **FIFO buffer** for burst data collection
3. **Motion detection** using built-in LIS2DH features
4. **Data filtering** and noise reduction
5. **Calibration routines** for improved accuracy

### Additional Sensors
The I2C infrastructure can easily support additional sensors:
- Additional LIS2DH sensors
- Other I2C sensors (pressure, humidity, etc.)
- I2C multiplexers for multiple sensor buses