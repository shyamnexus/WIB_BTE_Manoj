// i2c0.h - I2C driver for SAM4E
#ifndef I2C0_H
#define I2C0_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// I2C configuration
#define I2C0_CLOCK_FREQ_HZ     100000  // 100 kHz standard mode
#define I2C0_TIMEOUT_MS        100     // 100ms timeout for operations

// I2C status codes
typedef enum {
    I2C_SUCCESS = 0,
    I2C_ERROR_TIMEOUT,
    I2C_ERROR_NACK,
    I2C_ERROR_BUSY,
    I2C_ERROR_INVALID_PARAM
} i2c_status_t;

// Function prototypes
i2c_status_t i2c0_init(void);
i2c_status_t i2c0_write_register(uint8_t device_addr, uint8_t reg_addr, uint8_t data);
i2c_status_t i2c0_read_register(uint8_t device_addr, uint8_t reg_addr, uint8_t *data);
i2c_status_t i2c0_read_multiple_registers(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t count);
i2c_status_t i2c0_write_multiple_registers(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, uint8_t count);

#ifdef __cplusplus
}
#endif

#endif // I2C0_H