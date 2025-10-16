// lis2dh.c - LIS2DH accelerometer and temperature sensor driver implementation
#include "lis2dh.h"
#include "i2c0.h"
#include "delay.h"

// Global variables for sensor configuration
static lis2dh_fs_t current_fs = LIS2DH_FS_2G;
static bool temperature_enabled = false;

// Sensitivity values for different full scale ranges (mg/LSB)
static const float sensitivity_values[] = {
    1.0f,   // ±2g: 1 mg/LSB
    2.0f,   // ±4g: 2 mg/LSB
    4.0f,   // ±8g: 4 mg/LSB
    12.0f   // ±16g: 12 mg/LSB
};

bool lis2dh_init(void)
{
    // Initialize I2C first
    if (i2c0_init() != I2C_SUCCESS) {
        return false;
    }
    
    // Verify connection
    if (!lis2dh_verify_connection()) {
        return false;
    }
    
    // Configure accelerometer
    // CTRL_REG1: Enable X, Y, Z axes, set ODR to 100Hz
    uint8_t ctrl_reg1 = 0x57; // 0101 0111: X, Y, Z enabled, 100Hz ODR
    if (i2c0_write_register(LIS2DH_I2C_ADDR, LIS2DH_REG_CTRL_REG1, ctrl_reg1) != I2C_SUCCESS) {
        return false;
    }
    
    // CTRL_REG4: Set full scale to ±2g, high resolution mode
    uint8_t ctrl_reg4 = 0x00; // 0000 0000: ±2g, high resolution
    if (i2c0_write_register(LIS2DH_I2C_ADDR, LIS2DH_REG_CTRL_REG4, ctrl_reg4) != I2C_SUCCESS) {
        return false;
    }
    
    // Enable temperature sensor
    if (!lis2dh_enable_temperature_sensor(true)) {
        return false;
    }
    
    // Small delay for configuration to take effect
    delay_ms(10);
    
    return true;
}

bool lis2dh_verify_connection(void)
{
    uint8_t who_am_i;
    if (i2c0_read_register(LIS2DH_I2C_ADDR, LIS2DH_REG_WHO_AM_I, &who_am_i) != I2C_SUCCESS) {
        return false;
    }
    
    return (who_am_i == LIS2DH_WHO_AM_I_VALUE);
}

bool lis2dh_set_full_scale(lis2dh_fs_t fs)
{
    if (fs > LIS2DH_FS_16G) {
        return false;
    }
    
    uint8_t ctrl_reg4;
    if (i2c0_read_register(LIS2DH_I2C_ADDR, LIS2DH_REG_CTRL_REG4, &ctrl_reg4) != I2C_SUCCESS) {
        return false;
    }
    
    // Clear FS bits (bits 4-5) and set new value
    ctrl_reg4 &= ~0x30;
    ctrl_reg4 |= (fs << 4);
    
    if (i2c0_write_register(LIS2DH_I2C_ADDR, LIS2DH_REG_CTRL_REG4, ctrl_reg4) != I2C_SUCCESS) {
        return false;
    }
    
    current_fs = fs;
    return true;
}

bool lis2dh_set_output_data_rate(lis2dh_odr_t odr)
{
    if (odr > LIS2DH_ODR_5376HZ) {
        return false;
    }
    
    uint8_t ctrl_reg1;
    if (i2c0_read_register(LIS2DH_I2C_ADDR, LIS2DH_REG_CTRL_REG1, &ctrl_reg1) != I2C_SUCCESS) {
        return false;
    }
    
    // Clear ODR bits (bits 4-7) and set new value
    ctrl_reg1 &= ~0xF0;
    ctrl_reg1 |= (odr << 4);
    
    if (i2c0_write_register(LIS2DH_I2C_ADDR, LIS2DH_REG_CTRL_REG1, ctrl_reg1) != I2C_SUCCESS) {
        return false;
    }
    
    return true;
}

bool lis2dh_enable_temperature_sensor(bool enable)
{
    uint8_t temp_cfg;
    if (i2c0_read_register(LIS2DH_I2C_ADDR, LIS2DH_REG_TEMP_CFG_REG, &temp_cfg) != I2C_SUCCESS) {
        return false;
    }
    
    if (enable) {
        temp_cfg |= 0xC0; // Enable temperature sensor (bits 7-6)
    } else {
        temp_cfg &= ~0xC0; // Disable temperature sensor
    }
    
    if (i2c0_write_register(LIS2DH_I2C_ADDR, LIS2DH_REG_TEMP_CFG_REG, temp_cfg) != I2C_SUCCESS) {
        return false;
    }
    
    temperature_enabled = enable;
    return true;
}

bool lis2dh_read_accelerometer(lis2dh_accel_data_t *data)
{
    if (data == NULL) {
        return false;
    }
    
    // Read 6 bytes starting from OUT_X_L register
    uint8_t raw_data[6];
    if (i2c0_read_multiple_registers(LIS2DH_I2C_ADDR, LIS2DH_REG_OUT_X_L, raw_data, 6) != I2C_SUCCESS) {
        return false;
    }
    
    // Combine low and high bytes for each axis
    data->x = (int16_t)((raw_data[1] << 8) | raw_data[0]);
    data->y = (int16_t)((raw_data[3] << 8) | raw_data[2]);
    data->z = (int16_t)((raw_data[5] << 8) | raw_data[4]);
    
    // Convert to g values
    float sensitivity = sensitivity_values[current_fs];
    data->x_g = (float)data->x * sensitivity / 1000.0f; // Convert mg to g
    data->y_g = (float)data->y * sensitivity / 1000.0f;
    data->z_g = (float)data->z * sensitivity / 1000.0f;
    
    return true;
}

bool lis2dh_read_temperature(lis2dh_temp_data_t *data)
{
    if (data == NULL || !temperature_enabled) {
        return false;
    }
    
    // Read 2 bytes starting from OUT_ADC3_L register (temperature data)
    uint8_t raw_data[2];
    if (i2c0_read_multiple_registers(LIS2DH_I2C_ADDR, LIS2DH_REG_OUT_ADC3_L, raw_data, 2) != I2C_SUCCESS) {
        return false;
    }
    
    // Combine low and high bytes
    data->raw = (int16_t)((raw_data[1] << 8) | raw_data[0]);
    
    // Convert to Celsius
    // LIS2DH temperature sensitivity is typically 1 LSB/°C
    // Temperature = (raw_value / 8) + 25°C (typical formula)
    data->celsius = (float)data->raw / 8.0f + 25.0f;
    
    return true;
}

bool lis2dh_read_accelerometer_and_temperature(lis2dh_accel_data_t *accel_data, lis2dh_temp_data_t *temp_data)
{
    bool accel_success = true;
    bool temp_success = true;
    
    if (accel_data != NULL) {
        accel_success = lis2dh_read_accelerometer(accel_data);
    }
    
    if (temp_data != NULL) {
        temp_success = lis2dh_read_temperature(temp_data);
    }
    
    return accel_success && temp_success;
}