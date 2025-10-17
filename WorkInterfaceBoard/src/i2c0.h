#ifndef I2C0_H
#define I2C0_H

#include <stdint.h>
#include <stdbool.h>
#include "sam4e.h"

#ifdef __cplusplus
extern "C" {
#endif

// I2C Configuration
#define I2C0_CLOCK_FREQ_HZ    100000  // 100 kHz I2C clock
#define I2C0_TIMEOUT_MS       100     // 100ms timeout for operations

// I2C Status Codes
typedef enum {
    I2C0_SUCCESS = 0,
    I2C0_ERROR_TIMEOUT,
    I2C0_ERROR_NACK,
    I2C0_ERROR_BUSY,
    I2C0_ERROR_INVALID_PARAM
} i2c0_status_t;

// Function prototypes
i2c0_status_t i2c0_init(void);
i2c0_status_t i2c0_write_byte(uint8_t device_addr, uint8_t reg_addr, uint8_t data);
i2c0_status_t i2c0_read_byte(uint8_t device_addr, uint8_t reg_addr, uint8_t *data);
i2c0_status_t i2c0_read_bytes(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t count);
i2c0_status_t i2c0_write_bytes(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, uint8_t count);
bool i2c0_is_device_present(uint8_t device_addr);

#ifdef __cplusplus
}
#endif

#endif // I2C0_H