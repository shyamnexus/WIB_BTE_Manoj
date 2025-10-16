#include "mc3419.h"
#include "can_app.h"
#include "delay.h"
#include <stdio.h>

// Test function to verify MC3419 functionality
void mc3419_test(void)
{
    mc3419_data_t sensor_data;
    uint8_t accel_payload[6];
    uint8_t temp_payload[2];
    
    // Initialize MC3419
    if (!mc3419_init()) {
        printf("MC3419 initialization failed!\n");
        return;
    }
    
    printf("MC3419 initialized successfully\n");
    
    // Test data reading
    for (int i = 0; i < 10; i++) {
        if (mc3419_read_data(&sensor_data) && sensor_data.valid) {
            // Prepare accelerometer data
            accel_payload[0] = (uint8_t)(sensor_data.x & 0xFF);
            accel_payload[1] = (uint8_t)((sensor_data.x >> 8) & 0xFF);
            accel_payload[2] = (uint8_t)(sensor_data.y & 0xFF);
            accel_payload[3] = (uint8_t)((sensor_data.y >> 8) & 0xFF);
            accel_payload[4] = (uint8_t)(sensor_data.z & 0xFF);
            accel_payload[5] = (uint8_t)((sensor_data.z >> 8) & 0xFF);
            
            // Prepare temperature data
            temp_payload[0] = (uint8_t)(sensor_data.temp & 0xFF);
            temp_payload[1] = (uint8_t)((sensor_data.temp >> 8) & 0xFF);
            
            // Convert to engineering units for display
            float x_g = mc3419_convert_accel_to_g(sensor_data.x, MC3419_RANGE_8G);
            float y_g = mc3419_convert_accel_to_g(sensor_data.y, MC3419_RANGE_8G);
            float z_g = mc3419_convert_accel_to_g(sensor_data.z, MC3419_RANGE_8G);
            float temp_c = mc3419_convert_temp_to_celsius(sensor_data.temp);
            
            printf("Sample %d: X=%.3fg, Y=%.3fg, Z=%.3fg, Temp=%.1f°C\n", 
                   i+1, x_g, y_g, z_g, temp_c);
            
            // Send over CAN (if CAN is initialized)
            can_app_tx(CAN_ID_ACCELEROMETER, accel_payload, 6);
            can_app_tx(CAN_ID_TEMPERATURE, temp_payload, 2);
        } else {
            printf("Failed to read sensor data\n");
        }
        
        delay_ms(100); // 10 Hz sampling
    }
    
    printf("MC3419 test completed\n");
}