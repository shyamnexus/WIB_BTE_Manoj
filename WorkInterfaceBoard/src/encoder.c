#include "encoder.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "can_app.h"

// Global encoder data structures
static encoder_data_t encoder1_data = {0};
static encoder_data_t encoder2_data = {0};

// CAN message IDs for encoder data
#define CAN_ID_ENCODER1_DIR_VEL    0x130u  // Encoder 1 direction and velocity
#define CAN_ID_ENCODER2_DIR_VEL    0x131u  // Encoder 2 direction and velocity

bool encoder_init(void)
{
    // Enable PIO clocks for encoder pins
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);
    
    // Configure encoder pins as inputs with pull-up
    pio_configure(PIOA, PIO_INPUT, ENC1_A_PIN, PIO_PULLUP | PIO_DEBOUNCE);
    pio_configure(PIOA, PIO_INPUT, ENC1_B_PIN, PIO_PULLUP | PIO_DEBOUNCE);
    pio_configure(PIOA, PIO_INPUT, ENC2_A_PIN, PIO_PULLUP | PIO_DEBOUNCE);
    pio_configure(PIOA, PIO_INPUT, ENC2_B_PIN, PIO_PULLUP | PIO_DEBOUNCE);
    
    // Configure enable pins as outputs and set them high (enable encoders)
    pio_configure(PIOD, PIO_OUTPUT_0, ENC1_ENABLE_PIN, PIO_DEFAULT);
    pio_set(PIOD, ENC1_ENABLE_PIN);  // Enable encoder 1
    
    if (ENCODER2_AVAILABLE) {
        pio_configure(PIOD, PIO_OUTPUT_0, ENC2_ENABLE_PIN, PIO_DEFAULT);
        pio_set(PIOD, ENC2_ENABLE_PIN);  // Enable encoder 2
    }
    
    // Initialize encoder data structures
    encoder1_data.position = 0;
    encoder1_data.velocity = 0;
    encoder1_data.direction = 0;
    encoder1_data.state_a = 0;
    encoder1_data.state_b = 0;
    encoder1_data.prev_state_a = 0;
    encoder1_data.prev_state_b = 0;
    encoder1_data.last_update_time = 0;
    encoder1_data.pulse_count = 0;
    encoder1_data.velocity_window_start = 0;
    
    encoder2_data.position = 0;
    encoder2_data.velocity = 0;
    encoder2_data.direction = 0;
    encoder2_data.state_a = 0;
    encoder2_data.state_b = 0;
    encoder2_data.prev_state_a = 0;
    encoder2_data.prev_state_b = 0;
    encoder2_data.last_update_time = 0;
    encoder2_data.pulse_count = 0;
    encoder2_data.velocity_window_start = 0;
    
    return true;
}

void encoder_poll(encoder_data_t* enc_data)
{
    // Skip polling if this is encoder2 and it's not available
    if (enc_data == &encoder2_data && !ENCODER2_AVAILABLE) {
        return;
    }
    
    // Read current encoder states
    uint8_t current_a = (pio_get(PIOA, PIO_TYPE_PIO_INPUT, enc_data == &encoder1_data ? ENC1_A_PIN : ENC2_A_PIN)) ? 1 : 0;
    uint8_t current_b = (pio_get(PIOA, PIO_TYPE_PIO_INPUT, enc_data == &encoder1_data ? ENC1_B_PIN : ENC2_B_PIN)) ? 1 : 0;
    
    // Get current time (using FreeRTOS tick count)
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Check for state change (quadrature decoding)
    if (current_a != enc_data->state_a || current_b != enc_data->state_b) {
        // Store previous states
        enc_data->prev_state_a = enc_data->state_a;
        enc_data->prev_state_b = enc_data->state_b;
        
        // Update current states
        enc_data->state_a = current_a;
        enc_data->state_b = current_b;
        
        // Quadrature decoding logic
        // Forward: A leads B (A changes first)
        // Reverse: B leads A (B changes first)
        if (enc_data->prev_state_a == 0 && enc_data->prev_state_b == 0) {
            if (enc_data->state_a == 1 && enc_data->state_b == 0) {
                // Forward: A leads
                enc_data->position++;
                enc_data->direction = 1;
                enc_data->pulse_count++;
            } else if (enc_data->state_a == 0 && enc_data->state_b == 1) {
                // Reverse: B leads
                enc_data->position--;
                enc_data->direction = 2;
                enc_data->pulse_count++;
            }
        } else if (enc_data->prev_state_a == 1 && enc_data->prev_state_b == 0) {
            if (enc_data->state_a == 1 && enc_data->state_b == 1) {
                // Forward: A leads
                enc_data->position++;
                enc_data->direction = 1;
                enc_data->pulse_count++;
            } else if (enc_data->state_a == 0 && enc_data->state_b == 0) {
                // Reverse: B leads
                enc_data->position--;
                enc_data->direction = 2;
                enc_data->pulse_count++;
            }
        } else if (enc_data->prev_state_a == 1 && enc_data->prev_state_b == 1) {
            if (enc_data->state_a == 0 && enc_data->state_b == 1) {
                // Forward: A leads
                enc_data->position++;
                enc_data->direction = 1;
                enc_data->pulse_count++;
            } else if (enc_data->state_a == 1 && enc_data->state_b == 0) {
                // Reverse: B leads
                enc_data->position--;
                enc_data->direction = 2;
                enc_data->pulse_count++;
            }
        } else if (enc_data->prev_state_a == 0 && enc_data->prev_state_b == 1) {
            if (enc_data->state_a == 0 && enc_data->state_b == 0) {
                // Forward: A leads
                enc_data->position++;
                enc_data->direction = 1;
                enc_data->pulse_count++;
            } else if (enc_data->state_a == 1 && enc_data->state_b == 1) {
                // Reverse: B leads
                enc_data->position--;
                enc_data->direction = 2;
                enc_data->pulse_count++;
            }
        }
        
        enc_data->last_update_time = current_time;
    } else {
        // No state change - check if we should reset direction to stopped
        if (current_time - enc_data->last_update_time > 50) { // 50ms timeout
            enc_data->direction = 0; // Stopped
        }
    }
    
    // Calculate velocity periodically
    if (current_time - enc_data->velocity_window_start >= VELOCITY_CALC_WINDOW_MS) {
        enc_data->velocity = calculate_velocity(enc_data, current_time);
        enc_data->velocity_window_start = current_time;
        enc_data->pulse_count = 0;
    }
}

int32_t calculate_velocity(encoder_data_t* enc_data, uint32_t current_time)
{
    if (enc_data->velocity_window_start == 0) {
        enc_data->velocity_window_start = current_time;
        return 0;
    }
    
    uint32_t time_diff = current_time - enc_data->velocity_window_start;
    if (time_diff == 0) return 0;
    
    // Calculate velocity in pulses per second
    int32_t velocity = (enc_data->pulse_count * 1000) / time_diff;
    
    // Apply direction sign
    if (enc_data->direction == 2) { // Reverse
        velocity = -velocity;
    }
    
    return velocity;
}

void encoder_task(void *arg)
{
    (void)arg; // Unused parameter
    
    // Initialize encoders
    if (!encoder_init()) {
        // Encoder initialization failed
        while(1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // Wait a bit for encoders to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    for (;;) {
        // Poll both encoders
        encoder_poll(&encoder1_data);
        if (ENCODER2_AVAILABLE) {
            encoder_poll(&encoder2_data);
        }
        
        // Send encoder 1 data over CAN
        uint8_t enc1_data[6];
        enc1_data[0] = (uint8_t)(encoder1_data.direction & 0xFF);
        enc1_data[1] = (uint8_t)(encoder1_data.velocity & 0xFF);
        enc1_data[2] = (uint8_t)((encoder1_data.velocity >> 8) & 0xFF);
        enc1_data[3] = (uint8_t)((encoder1_data.velocity >> 16) & 0xFF);
        enc1_data[4] = (uint8_t)((encoder1_data.velocity >> 24) & 0xFF);
        enc1_data[5] = (uint8_t)(encoder1_data.position & 0xFF);
        
        can_app_tx(CAN_ID_ENCODER1_DIR_VEL, enc1_data, 6);
        
        // Debug: Store encoder data for debugging
        volatile uint32_t debug_enc1_direction = encoder1_data.direction;
        volatile uint32_t debug_enc1_velocity = encoder1_data.velocity;
        volatile uint32_t debug_enc1_position = encoder1_data.position;
        
        // Send encoder 2 data over CAN (only if available)
        if (ENCODER2_AVAILABLE) {
            uint8_t enc2_data[6];
            enc2_data[0] = (uint8_t)(encoder2_data.direction & 0xFF);
            enc2_data[1] = (uint8_t)(encoder2_data.velocity & 0xFF);
            enc2_data[2] = (uint8_t)((encoder2_data.velocity >> 8) & 0xFF);
            enc2_data[3] = (uint8_t)((encoder2_data.velocity >> 16) & 0xFF);
            enc2_data[4] = (uint8_t)((encoder2_data.velocity >> 24) & 0xFF);
            enc2_data[5] = (uint8_t)(encoder2_data.position & 0xFF);
            
            can_app_tx(CAN_ID_ENCODER2_DIR_VEL, enc2_data, 6);
        }
        
        // Wait for next polling cycle
        vTaskDelay(pdMS_TO_TICKS(ENCODER_POLLING_RATE_MS));
    }
}