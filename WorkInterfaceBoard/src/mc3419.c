#include "mc3419.h"
#include "delay.h"

// Private function prototypes
static bool mc3419_verify_product_id(void);
static int16_t mc3419_read_16bit_register(uint8_t reg_lsb, uint8_t reg_msb);

bool mc3419_init(void)
{
    // Initialize I2C if not already done
    if (i2c0_init() != I2C0_SUCCESS) {
        return false;
    }
    
    // Verify product ID
    if (!mc3419_verify_product_id()) {
        return false;
    }
    
    // Configure accelerometer
    // Set to wake mode
    if (i2c0_write_byte(MC3419_I2C_ADDR, MC3419_REG_MODE, MC3419_MODE_WAKE) != I2C0_SUCCESS) {
        return false;
    }
    
    // Set sample rate to 100 Hz
    if (i2c0_write_byte(MC3419_I2C_ADDR, MC3419_REG_SAMPLE_RATE, MC3419_SAMPLE_RATE_100) != I2C0_SUCCESS) {
        return false;
    }
    
    // Set range to ±8g
    if (i2c0_write_byte(MC3419_I2C_ADDR, MC3419_REG_RANGE, MC3419_RANGE_8G) != I2C0_SUCCESS) {
        return false;
    }
    
    // Small delay for configuration to take effect
    delay_ms(10);
    
    return true;
}

static bool mc3419_verify_product_id(void)
{
    uint8_t product_id;
    if (i2c0_read_byte(MC3419_I2C_ADDR, MC3419_REG_PRODUCT_ID, &product_id) != I2C0_SUCCESS) {
        return false;
    }
    
    return (product_id == MC3419_PRODUCT_ID);
}

bool mc3419_read_data(mc3419_data_t *data)
{
    if (data == NULL) {
        return false;
    }
    
    // Check if data is ready
    if (!mc3419_is_data_ready()) {
        data->valid = false;
        return false;
    }
    
    // Read accelerometer data
    data->x = mc3419_read_16bit_register(MC3419_REG_XOUT_LSB, MC3419_REG_XOUT_MSB);
    data->y = mc3419_read_16bit_register(MC3419_REG_YOUT_LSB, MC3419_REG_YOUT_MSB);
    data->z = mc3419_read_16bit_register(MC3419_REG_ZOUT_LSB, MC3419_REG_ZOUT_MSB);
    
    // Read temperature data
    data->temp = mc3419_read_16bit_register(MC3419_REG_TEMP_LSB, MC3419_REG_TEMP_MSB);
    
    data->valid = true;
    return true;
}

static int16_t mc3419_read_16bit_register(uint8_t reg_lsb, uint8_t reg_msb)
{
    uint8_t lsb, msb;
    int16_t result = 0;
    
    // Read LSB first
    if (i2c0_read_byte(MC3419_I2C_ADDR, reg_lsb, &lsb) != I2C0_SUCCESS) {
        return 0;
    }
    
    // Read MSB
    if (i2c0_read_byte(MC3419_I2C_ADDR, reg_msb, &msb) != I2C0_SUCCESS) {
        return 0;
    }
    
    // Combine MSB and LSB (16-bit signed value)
    result = (int16_t)((msb << 8) | lsb);
    
    return result;
}

bool mc3419_set_mode(uint8_t mode)
{
    return (i2c0_write_byte(MC3419_I2C_ADDR, MC3419_REG_MODE, mode) == I2C0_SUCCESS);
}

bool mc3419_set_sample_rate(uint8_t rate)
{
    return (i2c0_write_byte(MC3419_I2C_ADDR, MC3419_REG_SAMPLE_RATE, rate) == I2C0_SUCCESS);
}

bool mc3419_set_range(uint8_t range)
{
    return (i2c0_write_byte(MC3419_I2C_ADDR, MC3419_REG_RANGE, range) == I2C0_SUCCESS);
}

bool mc3419_is_data_ready(void)
{
    uint8_t status;
    if (i2c0_read_byte(MC3419_I2C_ADDR, MC3419_REG_STATUS, &status) != I2C0_SUCCESS) {
        return false;
    }
    
    // Check if new data is available (bit 0 of status register)
    return (status & 0x01) != 0;
}

float mc3419_convert_accel_to_g(int16_t raw_value, uint8_t range)
{
    float scale_factor;
    
    // Determine scale factor based on range
    switch (range) {
        case MC3419_RANGE_2G:
            scale_factor = 2.0f / 32768.0f;  // ±2g range
            break;
        case MC3419_RANGE_4G:
            scale_factor = 4.0f / 32768.0f;  // ±4g range
            break;
        case MC3419_RANGE_8G:
            scale_factor = 8.0f / 32768.0f;  // ±8g range
            break;
        case MC3419_RANGE_16G:
            scale_factor = 16.0f / 32768.0f; // ±16g range
            break;
        default:
            scale_factor = 8.0f / 32768.0f;  // Default to ±8g
            break;
    }
    
    return (float)raw_value * scale_factor;
}

float mc3419_convert_temp_to_celsius(int16_t raw_value)
{
    // MC3419 temperature conversion (typical formula)
    // Temperature = (raw_value / 256.0) + 25.0
    return ((float)raw_value / 256.0f) + 25.0f;
}