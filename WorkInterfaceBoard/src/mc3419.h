#ifndef MC3419_H
#define MC3419_H

#include <stdint.h>
#include <stdbool.h>
#include "i2c0.h"

#ifdef __cplusplus
extern "C" {
#endif

// MC3419 I2C Address (7-bit)
#define MC3419_I2C_ADDR         0x4C    // Default I2C address for MC3419

// MC3419 Register Addresses
#define MC3419_REG_PRODUCT_ID   0x00    // Product ID register
#define MC3419_REG_XOUT_LSB     0x0D    // X-axis data LSB
#define MC3419_REG_XOUT_MSB     0x0E    // X-axis data MSB
#define MC3419_REG_YOUT_LSB     0x0F    // Y-axis data LSB
#define MC3419_REG_YOUT_MSB     0x10    // Y-axis data MSB
#define MC3419_REG_ZOUT_LSB     0x11    // Z-axis data LSB
#define MC3419_REG_ZOUT_MSB     0x12    // Z-axis data MSB
#define MC3419_REG_TEMP_LSB     0x13    // Temperature data LSB
#define MC3419_REG_TEMP_MSB     0x14    // Temperature data MSB
#define MC3419_REG_STATUS       0x02    // Status register
#define MC3419_REG_MODE         0x07    // Mode register
#define MC3419_REG_SAMPLE_RATE  0x08    // Sample rate register
#define MC3419_REG_RANGE        0x20    // Range register

// MC3419 Configuration Values
#define MC3419_PRODUCT_ID       0x19    // Expected product ID for MC3419

// Mode register values
#define MC3419_MODE_STANDBY     0x00    // Standby mode
#define MC3419_MODE_WAKE        0x01    // Wake mode
#define MC3419_MODE_SLEEP       0x02    // Sleep mode

// Sample rate values (Hz)
#define MC3419_SAMPLE_RATE_1    0x00    // 1 Hz
#define MC3419_SAMPLE_RATE_10   0x01    // 10 Hz
#define MC3419_SAMPLE_RATE_20   0x02    // 20 Hz
#define MC3419_SAMPLE_RATE_50   0x03    // 50 Hz
#define MC3419_SAMPLE_RATE_100  0x04    // 100 Hz
#define MC3419_SAMPLE_RATE_200  0x05    // 200 Hz
#define MC3419_SAMPLE_RATE_500  0x06    // 500 Hz
#define MC3419_SAMPLE_RATE_1000 0x07    // 1000 Hz

// Range values (g)
#define MC3419_RANGE_2G         0x00    // ±2g
#define MC3419_RANGE_4G         0x01    // ±4g
#define MC3419_RANGE_8G         0x02    // ±8g
#define MC3419_RANGE_16G        0x03    // ±16g

// Data structure for accelerometer readings
typedef struct {
    int16_t x;      // X-axis acceleration (raw value)
    int16_t y;      // Y-axis acceleration (raw value)
    int16_t z;      // Z-axis acceleration (raw value)
    int16_t temp;   // Temperature (raw value)
    bool valid;     // Data validity flag
} mc3419_data_t;

// Function prototypes
bool mc3419_init(void);
bool mc3419_read_data(mc3419_data_t *data);
bool mc3419_set_mode(uint8_t mode);
bool mc3419_set_sample_rate(uint8_t rate);
bool mc3419_set_range(uint8_t range);
bool mc3419_is_data_ready(void);
float mc3419_convert_accel_to_g(int16_t raw_value, uint8_t range);
float mc3419_convert_temp_to_celsius(int16_t raw_value);

#ifdef __cplusplus
}
#endif

#endif // MC3419_H