#include "simple_encoder.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "can_app.h"

// Simple test program for the encoder
int main(void)
{
    // Initialize system
    WIB_Init();
    
    // Initialize CAN controller
    if (!can_app_init()) {
        // CAN initialization failed
        while(1);
    }
    
    // Initialize encoder
    if (!simple_encoder_init()) {
        // Encoder initialization failed
        while(1);
    }
    
    // Wait for encoder to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    uint32_t last_transmission_time = 0;
    
    while(1) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Poll encoder
        simple_encoder_poll();
        
        // Send encoder data over CAN every 50ms
        if (current_time - last_transmission_time >= 50) {
            
            // Get encoder data
            simple_encoder_data_t* enc_data = simple_encoder_get_data();
            
            // Prepare CAN message
            // Message format: [Direction(1)] [Velocity(3)] [Position(4)]
            uint8_t can_data[8];
            
            // Pack direction (1 byte)
            can_data[0] = enc_data->direction;
            
            // Pack velocity (3 bytes) - signed 24-bit value
            int32_t velocity = enc_data->velocity;
            can_data[1] = (uint8_t)(velocity & 0xFF);
            can_data[2] = (uint8_t)((velocity >> 8) & 0xFF);
            can_data[3] = (uint8_t)((velocity >> 16) & 0xFF);
            
            // Pack position (4 bytes) - signed 32-bit value
            int32_t position_value = (int32_t)enc_data->position;
            can_data[4] = (uint8_t)(position_value & 0xFF);
            can_data[5] = (uint8_t)((position_value >> 8) & 0xFF);
            can_data[6] = (uint8_t)((position_value >> 16) & 0xFF);
            can_data[7] = (uint8_t)((position_value >> 24) & 0xFF);
            
            // Send over CAN
            can_app_tx(CAN_ID_ENCODER_DIR_VEL, can_data, 8);
            
            last_transmission_time = current_time;
        }
        
        // Small delay
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
