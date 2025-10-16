// lis2dh.h - LIS2DH accelerometer and temperature sensor driver
#ifndef LIS2DH_H
#define LIS2DH_H

#include <stdint.h>
#include <stdbool.h>
#include "i2c0.h"

#ifdef __cplusplus
extern "C" {
#endif

// LIS2DH I2C address (7-bit address)
#define LIS2DH_I2C_ADDR         0x19    // LIS2DH default I2C address

// LIS2DH register addresses
#define LIS2DH_REG_STATUS_REG_AUX   0x07
#define LIS2DH_REG_OUT_ADC1_L       0x08
#define LIS2DH_REG_OUT_ADC1_H       0x09
#define LIS2DH_REG_OUT_ADC2_L       0x0A
#define LIS2DH_REG_OUT_ADC2_H       0x0B
#define LIS2DH_REG_OUT_ADC3_L       0x0C
#define LIS2DH_REG_OUT_ADC3_H       0x0D
#define LIS2DH_REG_INT_COUNTER_REG  0x0E
#define LIS2DH_REG_WHO_AM_I         0x0F
#define LIS2DH_REG_TEMP_CFG_REG     0x1F
#define LIS2DH_REG_CTRL_REG1        0x20
#define LIS2DH_REG_CTRL_REG2        0x21
#define LIS2DH_REG_CTRL_REG3        0x22
#define LIS2DH_REG_CTRL_REG4        0x23
#define LIS2DH_REG_CTRL_REG5        0x24
#define LIS2DH_REG_CTRL_REG6        0x25
#define LIS2DH_REG_REFERENCE        0x26
#define LIS2DH_REG_STATUS_REG       0x27
#define LIS2DH_REG_OUT_X_L          0x28
#define LIS2DH_REG_OUT_X_H          0x29
#define LIS2DH_REG_OUT_Y_L          0x2A
#define LIS2DH_REG_OUT_Y_H          0x2B
#define LIS2DH_REG_OUT_Z_L          0x2C
#define LIS2DH_REG_OUT_Z_H          0x2D
#define LIS2DH_REG_FIFO_CTRL_REG    0x2E
#define LIS2DH_REG_FIFO_SRC_REG     0x2F
#define LIS2DH_REG_INT1_CFG         0x30
#define LIS2DH_REG_INT1_SRC         0x31
#define LIS2DH_REG_INT1_THS         0x32
#define LIS2DH_REG_INT1_DURATION    0x33
#define LIS2DH_REG_INT2_CFG         0x34
#define LIS2DH_REG_INT2_SRC         0x35
#define LIS2DH_REG_INT2_THS         0x36
#define LIS2DH_REG_INT2_DURATION    0x37
#define LIS2DH_REG_CLICK_CFG        0x38
#define LIS2DH_REG_CLICK_SRC        0x39
#define LIS2DH_REG_CLICK_THS        0x3A
#define LIS2DH_REG_TIME_LIMIT       0x3B
#define LIS2DH_REG_TIME_LATENCY     0x3C
#define LIS2DH_REG_TIME_WINDOW      0x3D
#define LIS2DH_REG_ACT_THS          0x3E
#define LIS2DH_REG_ACT_DUR          0x3F

// WHO_AM_I register value
#define LIS2DH_WHO_AM_I_VALUE       0x33

// Accelerometer data structure
typedef struct {
    int16_t x;      // X-axis acceleration (raw value)
    int16_t y;      // Y-axis acceleration (raw value)
    int16_t z;      // Z-axis acceleration (raw value)
    float x_g;      // X-axis acceleration in g
    float y_g;      // Y-axis acceleration in g
    float z_g;      // Z-axis acceleration in g
} lis2dh_accel_data_t;

// Temperature data structure
typedef struct {
    int16_t raw;    // Raw temperature value
    float celsius;  // Temperature in Celsius
} lis2dh_temp_data_t;

// Full scale range options
typedef enum {
    LIS2DH_FS_2G = 0,   // ±2g
    LIS2DH_FS_4G = 1,   // ±4g
    LIS2DH_FS_8G = 2,   // ±8g
    LIS2DH_FS_16G = 3   // ±16g
} lis2dh_fs_t;

// Output data rate options
typedef enum {
    LIS2DH_ODR_POWER_DOWN = 0,
    LIS2DH_ODR_1HZ = 1,
    LIS2DH_ODR_10HZ = 2,
    LIS2DH_ODR_25HZ = 3,
    LIS2DH_ODR_50HZ = 4,
    LIS2DH_ODR_100HZ = 5,
    LIS2DH_ODR_200HZ = 6,
    LIS2DH_ODR_400HZ = 7,
    LIS2DH_ODR_1620HZ = 8,
    LIS2DH_ODR_5376HZ = 9
} lis2dh_odr_t;

// Function prototypes
bool lis2dh_init(void);
bool lis2dh_verify_connection(void);
bool lis2dh_set_full_scale(lis2dh_fs_t fs);
bool lis2dh_set_output_data_rate(lis2dh_odr_t odr);
bool lis2dh_enable_temperature_sensor(bool enable);
bool lis2dh_read_accelerometer(lis2dh_accel_data_t *data);
bool lis2dh_read_temperature(lis2dh_temp_data_t *data);
bool lis2dh_read_accelerometer_and_temperature(lis2dh_accel_data_t *accel_data, lis2dh_temp_data_t *temp_data);

#ifdef __cplusplus
}
#endif

#endif // LIS2DH_H