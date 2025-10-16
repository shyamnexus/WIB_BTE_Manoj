#include "i2c0.h"
#include "conf_board.h"
#include "pmc.h"
#include "pio.h"
#include "twi.h"
#include "delay.h"

// I2C0 TWI instance
#define I2C0_TWI         TWI0
#define I2C0_TWI_ID      ID_TWI0

// I2C Pin definitions
#define I2C0_SCL_PIN     PIN_I2C_SCL
#define I2C0_SDA_PIN     PIN_I2C_SDA

// Private function prototypes
static void i2c0_configure_pins(void);
static i2c0_status_t i2c0_wait_for_transfer_complete(void);
static i2c0_status_t i2c0_wait_for_tx_ready(void);
static i2c0_status_t i2c0_wait_for_rx_ready(void);

i2c0_status_t i2c0_init(void)
{
    // Enable TWI0 peripheral clock
    pmc_enable_periph_clk(I2C0_TWI_ID);
    
    // Configure I2C pins
    i2c0_configure_pins();
    
    // Initialize TWI in master mode
    twi_options_t twi_options;
    twi_options.master_clk = sysclk_get_cpu_hz();
    twi_options.speed = I2C0_CLOCK_FREQ_HZ;
    twi_options.smbus = false;
    
    if (twi_master_init(I2C0_TWI, &twi_options) != TWI_SUCCESS) {
        return I2C0_ERROR_BUSY;
    }
    
    return I2C0_SUCCESS;
}

static void i2c0_configure_pins(void)
{
    // Configure SCL pin (PA4)
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA4A_TWD0, 0);
    
    // Configure SDA pin (PA3)  
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA3A_TWCK0, 0);
}

i2c0_status_t i2c0_write_byte(uint8_t device_addr, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_data[2] = {reg_addr, data};
    
    if (twi_master_write(I2C0_TWI, device_addr, write_data, 2) != TWI_SUCCESS) {
        return I2C0_ERROR_NACK;
    }
    
    return I2C0_SUCCESS;
}

i2c0_status_t i2c0_read_byte(uint8_t device_addr, uint8_t reg_addr, uint8_t *data)
{
    // First write the register address
    if (twi_master_write(I2C0_TWI, device_addr, &reg_addr, 1) != TWI_SUCCESS) {
        return I2C0_ERROR_NACK;
    }
    
    // Then read the data
    if (twi_master_read(I2C0_TWI, device_addr, data, 1) != TWI_SUCCESS) {
        return I2C0_ERROR_NACK;
    }
    
    return I2C0_SUCCESS;
}

i2c0_status_t i2c0_read_bytes(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t count)
{
    // First write the register address
    if (twi_master_write(I2C0_TWI, device_addr, &reg_addr, 1) != TWI_SUCCESS) {
        return I2C0_ERROR_NACK;
    }
    
    // Then read the data
    if (twi_master_read(I2C0_TWI, device_addr, data, count) != TWI_SUCCESS) {
        return I2C0_ERROR_NACK;
    }
    
    return I2C0_SUCCESS;
}

i2c0_status_t i2c0_write_bytes(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, uint8_t count)
{
    // Prepare write buffer with register address + data
    uint8_t write_data[count + 1];
    write_data[0] = reg_addr;
    for (uint8_t i = 0; i < count; i++) {
        write_data[i + 1] = data[i];
    }
    
    if (twi_master_write(I2C0_TWI, device_addr, write_data, count + 1) != TWI_SUCCESS) {
        return I2C0_ERROR_NACK;
    }
    
    return I2C0_SUCCESS;
}

bool i2c0_is_device_present(uint8_t device_addr)
{
    uint8_t dummy_data;
    return (i2c0_read_byte(device_addr, 0x00, &dummy_data) == I2C0_SUCCESS);
}