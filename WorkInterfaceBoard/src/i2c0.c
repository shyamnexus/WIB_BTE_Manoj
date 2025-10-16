// i2c0.c - I2C driver implementation for SAM4E
#include "i2c0.h"
#include "asf.h"
#include "sam4e.h"
#include "pmc.h"
#include "pio.h"
#include "twi.h"
#include "sysclk.h"
#include "delay.h"

// I2C pin configuration based on PinMUX file
// PA3 = I2C_SDA, PA4 = I2C_SCL
#define I2C0_SDA_PIN    (1u << 3)   // PA3
#define I2C0_SCL_PIN    (1u << 4)   // PA4

static void i2c0_configure_pins(void)
{
    // Enable PIOA clock
    pmc_enable_periph_clk(ID_PIOA);
    
    // Configure PA3 (SDA) and PA4 (SCL) for TWI0 peripheral
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA3A_TWD0 | PIO_PA4A_TWCK0, 0);
}

i2c_status_t i2c0_init(void)
{
    // Configure I2C pins
    i2c0_configure_pins();
    
    // Enable TWI0 peripheral clock
    pmc_enable_periph_clk(ID_TWI0);
    
    // Get master clock frequency
    uint32_t mck = sysclk_get_peripheral_hz();
    
    // Configure TWI0 for 100kHz
    twi_options_t twi_options = {
        .master_clk = mck,
        .speed = I2C0_CLOCK_FREQ_HZ,
        .smbus = false,
        .chip = 0  // Not used in master mode
    };
    
    // Initialize TWI0
    if (twi_master_init(TWI0, &twi_options) != TWI_SUCCESS) {
        return I2C_ERROR_INVALID_PARAM;
    }
    
    return I2C_SUCCESS;
}

i2c_status_t i2c0_write_register(uint8_t device_addr, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_data[2] = {reg_addr, data};
    
    if (twi_master_write(TWI0, device_addr, write_data, 2) != TWI_SUCCESS) {
        return I2C_ERROR_NACK;
    }
    
    return I2C_SUCCESS;
}

i2c_status_t i2c0_read_register(uint8_t device_addr, uint8_t reg_addr, uint8_t *data)
{
    if (data == NULL) {
        return I2C_ERROR_INVALID_PARAM;
    }
    
    // First write the register address
    if (twi_master_write(TWI0, device_addr, &reg_addr, 1) != TWI_SUCCESS) {
        return I2C_ERROR_NACK;
    }
    
    // Then read the data
    if (twi_master_read(TWI0, device_addr, data, 1) != TWI_SUCCESS) {
        return I2C_ERROR_NACK;
    }
    
    return I2C_SUCCESS;
}

i2c_status_t i2c0_read_multiple_registers(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t count)
{
    if (data == NULL || count == 0) {
        return I2C_ERROR_INVALID_PARAM;
    }
    
    // First write the register address
    if (twi_master_write(TWI0, device_addr, &reg_addr, 1) != TWI_SUCCESS) {
        return I2C_ERROR_NACK;
    }
    
    // Then read multiple bytes
    if (twi_master_read(TWI0, device_addr, data, count) != TWI_SUCCESS) {
        return I2C_ERROR_NACK;
    }
    
    return I2C_SUCCESS;
}

i2c_status_t i2c0_write_multiple_registers(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, uint8_t count)
{
    if (data == NULL || count == 0) {
        return I2C_ERROR_INVALID_PARAM;
    }
    
    // Prepare write buffer with register address + data
    uint8_t write_data[count + 1];
    write_data[0] = reg_addr;
    for (uint8_t i = 0; i < count; i++) {
        write_data[i + 1] = data[i];
    }
    
    if (twi_master_write(TWI0, device_addr, write_data, count + 1) != TWI_SUCCESS) {
        return I2C_ERROR_NACK;
    }
    
    return I2C_SUCCESS;
}