// sensor_test.c - Sensor testing and validation functions implementation
#include "sensor_test.h"
#include "lis2dh.h"
#include "can_app.h"
#include "delay.h"

bool sensor_test_lis2dh_connection(void)
{
    return lis2dh_verify_connection();
}

bool sensor_test_accelerometer_reading(void)
{
    lis2dh_accel_data_t accel_data;
    
    if (!lis2dh_read_accelerometer(&accel_data)) {
        return false;
    }
    
    // Test if we get reasonable values (not all zeros)
    bool valid_data = (accel_data.x != 0) || (accel_data.y != 0) || (accel_data.z != 0);
    
    // Store test results in volatile variables for debugging
    volatile int16_t test_x = accel_data.x;
    volatile int16_t test_y = accel_data.y;
    volatile int16_t test_z = accel_data.z;
    volatile float test_x_g = accel_data.x_g;
    volatile float test_y_g = accel_data.y_g;
    volatile float test_z_g = accel_data.z_g;
    
    return valid_data;
}

bool sensor_test_temperature_reading(void)
{
    lis2dh_temp_data_t temp_data;
    
    if (!lis2dh_read_temperature(&temp_data)) {
        return false;
    }
    
    // Test if we get reasonable temperature values (between -40°C and +85°C)
    bool valid_temp = (temp_data.celsius >= -40.0f) && (temp_data.celsius <= 85.0f);
    
    // Store test results in volatile variables for debugging
    volatile int16_t test_raw = temp_data.raw;
    volatile float test_celsius = temp_data.celsius;
    
    return valid_temp;
}

void sensor_test_print_diagnostics(void)
{
    // This function can be called from debugger to inspect sensor values
    lis2dh_accel_data_t accel_data;
    lis2dh_temp_data_t temp_data;
    
    if (lis2dh_read_accelerometer_and_temperature(&accel_data, &temp_data)) {
        // Store values in volatile variables for debugging
        volatile int16_t accel_x = accel_data.x;
        volatile int16_t accel_y = accel_data.y;
        volatile int16_t accel_z = accel_data.z;
        volatile float accel_x_g = accel_data.x_g;
        volatile float accel_y_g = accel_data.y_g;
        volatile float accel_z_g = accel_data.z_g;
        volatile int16_t temp_raw = temp_data.raw;
        volatile float temp_celsius = temp_data.celsius;
        
        // These variables can be inspected in the debugger
        (void)accel_x; (void)accel_y; (void)accel_z;
        (void)accel_x_g; (void)accel_y_g; (void)accel_z_g;
        (void)temp_raw; (void)temp_celsius;
    }
}