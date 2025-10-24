#include "encoder.h"
#include "asf.h"
#include <tc.h>
#include "FreeRTOS.h"
#include "task.h"
#include "can_app.h"

// Encoder data structures
static encoder_data_t encoder1_data = {0};
static encoder_data_t encoder2_data = {0};

// Define TickType_t if not already defined
#ifndef TickType_t
typedef portTickType TickType_t;
#endif

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS portTICK_RATE_MS // Backward compatibility macro
#endif
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_RATE_MS)) // Convert ms to OS ticks
#endif

// Configuration constants
#define ENCODER_POLLING_RATE_MS    50      // Polling rate in milliseconds
#define VELOCITY_WINDOW_MS         100     // Velocity calculation window in milliseconds
#define DIRECTION_CHANGE_DEBOUNCE_MS 20    // Direction change debounce time in milliseconds

// Encoder initialization
bool encoder_init(void)
{
    // Initialize encoder data structures
    encoder1_data.position = 0;
    encoder1_data.velocity = 0;
    encoder1_data.smoothed_velocity = 0;
    encoder1_data.direction = 0;
    encoder1_data.last_position = 0;
    encoder1_data.last_update_time = 0;
    encoder1_data.last_direction_change = 0;
    encoder1_data.pulse_count = 0;
    encoder1_data.velocity_window_start = 0;
    encoder1_data.tc_channel = TC_QUADRATURE_CHANNEL_ENC1;
    
    encoder2_data.position = 0;
    encoder2_data.velocity = 0;
    encoder2_data.smoothed_velocity = 0;
    encoder2_data.direction = 0;
    encoder2_data.last_position = 0;
    encoder2_data.last_update_time = 0;
    encoder2_data.last_direction_change = 0;
    encoder2_data.pulse_count = 0;
    encoder2_data.velocity_window_start = 0;
    encoder2_data.tc_channel = TC_QUADRATURE_CHANNEL_ENC2;
    
    // Initialize TC quadrature decoder
    if (!encoder_tc_init()) {
        return false;
    }
    
    // Enable PIO clocks for enable pins
    pmc_enable_periph_clk(ID_PIOD);
    
    // Configure enable pins as outputs and set them low (enable encoders)
    pio_configure(PIOD, PIO_OUTPUT_0, ENC1_ENABLE_PIN, PIO_DEFAULT);
    pio_clear(PIOD, ENC1_ENABLE_PIN);  // Enable encoder 1 (set low)
    
    if (ENCODER2_AVAILABLE) {
        pio_configure(PIOD, PIO_OUTPUT_0, ENC2_ENABLE_PIN, PIO_DEFAULT);
        pio_clear(PIOD, ENC2_ENABLE_PIN);  // Enable encoder 2 (set low)
    }
    
    return true;
}

// TC initialization
bool encoder_tc_init(void)
{
    // Enable TC0 peripheral clock
    pmc_enable_periph_clk(ID_TC0);
    
    // Configure PIO for TC TIOA and TIOB pins (matching pinconfig_workhead_interface_pinconfig.csv)
    // Configure PA5 as TIOA0 and PA1 as TIOB0 for encoder 1
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA5, PIO_DEFAULT);  // TIOA0 (pin 52)
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA1, PIO_DEFAULT);  // TIOB0 (pin 70)
    
    if (ENCODER2_AVAILABLE) {
        // Configure PA15 as TIOA1 and PA16 as TIOB1 for encoder 2
        pio_configure(PIOA, PIO_PERIPH_A, PIO_PA15, PIO_DEFAULT); // TIOA1 (pin 33)
        pio_configure(PIOA, PIO_PERIPH_A, PIO_PA16, PIO_DEFAULT); // TIOB1 (pin 30)
    }
    
    // Add small delay after pin configuration
    delay_ms(10);
    
    // Set up Block Mode Register for quadrature decoding
    uint32_t block_mode = TC_BMR_QDEN |                    // Enable quadrature decoder
                         TC_BMR_POSEN |                   // Enable position counting
                         TC_BMR_SPEEDEN |                 // Enable speed counting
                         TC_BMR_FILTER |                  // Enable glitch filter
                         TC_BMR_MAXFILT(TC_QUADRATURE_FILTER); // Set filter value
    
    tc_set_block_mode(TC0, block_mode);
    
    // Add delay after setting block mode
    delay_ms(10);
    
    // For debugging: Try to use timer mode instead of quadrature mode
    // This will help us determine if the issue is with quadrature decoding or basic TC functionality
    // Comment out the quadrature-specific configuration for now
    /*
    // Initialize TC0 channel 0 for ENC1 (external clock mode for quadrature)
    // For quadrature decoding, we need capture mode with external clock
    tc_init(TC0, TC_QUADRATURE_CHANNEL_ENC1, TC_CMR_TCCLKS_XC0 | TC_CMR_BURST_NONE | 
            TC_CMR_LDRA_RISING | TC_CMR_LDRB_FALLING);
    
    // Initialize TC0 channel 1 for ENC2 if available
    if (ENCODER2_AVAILABLE) {
        tc_init(TC0, TC_QUADRATURE_CHANNEL_ENC2, TC_CMR_TCCLKS_XC1 | TC_CMR_BURST_NONE |
                TC_CMR_LDRA_RISING | TC_CMR_LDRB_FALLING);
    }
    
    // Additional configuration for quadrature decoding
    // Set up the TC channels for quadrature decoding mode
    // This might be needed in addition to the tc_init call
    TC0->TC_CHANNEL[TC_QUADRATURE_CHANNEL_ENC1].TC_CMR |= TC_CMR_CPCTRG; // Enable RC compare trigger
    if (ENCODER2_AVAILABLE) {
        TC0->TC_CHANNEL[TC_QUADRATURE_CHANNEL_ENC2].TC_CMR |= TC_CMR_CPCTRG; // Enable RC compare trigger
    }
    */
    
    // For debugging: Use timer mode instead of quadrature mode
    // This will help us determine if the issue is with quadrature decoding or basic TC functionality
    tc_init(TC0, TC_QUADRATURE_CHANNEL_ENC1, TC_CMR_TCCLKS_TIMER_CLOCK1 | TC_CMR_BURST_NONE);
    if (ENCODER2_AVAILABLE) {
        tc_init(TC0, TC_QUADRATURE_CHANNEL_ENC2, TC_CMR_TCCLKS_TIMER_CLOCK1 | TC_CMR_BURST_NONE);
    }
    
    // Initialize TC0 channel 0 for ENC1 (external clock mode for quadrature)
    // For quadrature decoding, we need capture mode with external clock
    tc_init(TC0, TC_QUADRATURE_CHANNEL_ENC1, TC_CMR_TCCLKS_XC0 | TC_CMR_BURST_NONE | 
            TC_CMR_LDRA_RISING | TC_CMR_LDRB_FALLING);
    
    // Initialize TC0 channel 1 for ENC2 if available
    if (ENCODER2_AVAILABLE) {
        tc_init(TC0, TC_QUADRATURE_CHANNEL_ENC2, TC_CMR_TCCLKS_XC1 | TC_CMR_BURST_NONE |
                TC_CMR_LDRA_RISING | TC_CMR_LDRB_FALLING);
    }
    
    // Additional configuration for quadrature decoding
    // Set up the TC channels for quadrature decoding mode
    // This might be needed in addition to the tc_init call
    TC0->TC_CHANNEL[TC_QUADRATURE_CHANNEL_ENC1].TC_CMR |= TC_CMR_CPCTRG; // Enable RC compare trigger
    if (ENCODER2_AVAILABLE) {
        TC0->TC_CHANNEL[TC_QUADRATURE_CHANNEL_ENC2].TC_CMR |= TC_CMR_CPCTRG; // Enable RC compare trigger
    }
    
    // Add delay after channel initialization
    delay_ms(10);
    
    // Enable the channels with proper sequencing
    // Enable channel 0 first
    TC0->TC_CHANNEL[TC_QUADRATURE_CHANNEL_ENC1].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
    delay_ms(10); // Small delay between channel enables
    
    if (ENCODER2_AVAILABLE) {
        TC0->TC_CHANNEL[TC_QUADRATURE_CHANNEL_ENC2].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
        delay_ms(10); // Small delay after enabling
    }
    
    return true;
}


// Get current position from TC counter
uint32_t encoder_tc_get_position(uint32_t channel)
{
    if (channel >= 3) return 0; // Invalid channel
    
    // Read the current counter value
    return TC0->TC_CHANNEL[channel].TC_CV;
}

// Reset position counter
void encoder_tc_reset_position(uint32_t channel)
{
    if (channel >= 3) return; // Invalid channel
    
    // Reset the counter by disabling and re-enabling
    TC0->TC_CHANNEL[channel].TC_CCR = TC_CCR_CLKDIS;
    TC0->TC_CHANNEL[channel].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

// Get direction from TC
uint8_t encoder_tc_get_direction(uint32_t channel)
{
    if (channel >= 3) return 0; // Invalid channel
    
    // Read direction from TC QISR register
    // For quadrature decoder, check the direction bit for the specific channel
    return (TC0->TC_QISR & (1 << (channel + 1))) ? 1 : 0;
}

// Poll encoder for position changes
void encoder_poll(encoder_data_t* enc_data)
{
    // Skip polling if this is encoder2 and it's not available
    if (enc_data == &encoder2_data && !ENCODER2_AVAILABLE) {
        return;
    }
    
    // Get current time (using FreeRTOS tick count)
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Read current position from TC counter
    uint32_t current_position = encoder_tc_get_position(enc_data->tc_channel);
    
    // Debug: Use a simple counter instead of TC position for testing
    // This will help us determine if the issue is with TC counting or CAN transmission
    static uint32_t test_counter = 0;
    test_counter++;
    current_position = test_counter; // Use simple counter for testing
    
    // Check for position change
    if (current_position != enc_data->last_position) {
        // Calculate direction based on position change
        uint8_t new_direction = 0;
        if (current_position > enc_data->last_position) {
            new_direction = 1; // Forward
        } else if (current_position < enc_data->last_position) {
            new_direction = 2; // Reverse
        }
        
        // Update position
        enc_data->position = current_position;
        
        // Update direction only if change is allowed (debouncing)
        if (is_direction_change_allowed(enc_data, current_time, new_direction)) {
            enc_data->direction = new_direction;
            enc_data->last_direction_change = current_time;
        }
        
        // Calculate pulse count for velocity calculation
        uint32_t position_delta = (current_position > enc_data->last_position) ? 
                                 (current_position - enc_data->last_position) : 
                                 (enc_data->last_position - current_position);
        enc_data->pulse_count += position_delta;
        
        enc_data->last_position = current_position;
        enc_data->last_update_time = current_time;
    } else {
        // No position change - check if we should reset direction to stopped
        if (current_time - enc_data->last_update_time > 50) { // 50ms timeout
            enc_data->direction = 0; // Stopped
        }
    }
    
    // Calculate velocity
    enc_data->velocity = calculate_velocity(enc_data, current_time);
    
    // Apply velocity smoothing
    apply_velocity_smoothing(enc_data);
}

// Calculate velocity based on position changes
int32_t calculate_velocity(encoder_data_t* enc_data, uint32_t current_time)
{
    // Reset velocity window if needed
    if (current_time - enc_data->velocity_window_start >= VELOCITY_WINDOW_MS) {
        enc_data->velocity_window_start = current_time;
        enc_data->pulse_count = 0;
    }
    
    // Calculate velocity in pulses per second
    uint32_t time_delta = current_time - enc_data->velocity_window_start;
    if (time_delta == 0) return 0;
    
    int32_t velocity = (enc_data->pulse_count * 1000) / time_delta;
    
    // Apply direction
    if (enc_data->direction == 2) { // Reverse
        velocity = -velocity;
    }
    
    return velocity;
}

// Apply velocity smoothing
void apply_velocity_smoothing(encoder_data_t* enc_data)
{
    // Simple moving average smoothing
    static int32_t velocity_history[4] = {0};
    static uint8_t history_index = 0;
    
    velocity_history[history_index] = enc_data->velocity;
    history_index = (history_index + 1) % 4;
    
    // Calculate average
    int32_t sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += velocity_history[i];
    }
    enc_data->smoothed_velocity = sum / 4;
}

// Check if direction change is allowed (debouncing)
bool is_direction_change_allowed(encoder_data_t* enc_data, uint32_t current_time, uint8_t new_direction)
{
    // Don't change direction if it's the same
    if (enc_data->direction == new_direction) {
        return false;
    }
    
    // Don't change direction if too soon after last change
    if (current_time - enc_data->last_direction_change < DIRECTION_CHANGE_DEBOUNCE_MS) {
        return false;
    }
    
    return true;
}

// Main encoder task
void encoder_task(void *arg)
{
    // Initialize encoders
    if (!encoder_init()) {
        // Encoder initialization failed
        while(1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // Wait a bit for encoders to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    uint32_t last_transmission_time = 0;
    
    for (;;) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Poll both encoders
        encoder_poll(&encoder1_data);
        if (ENCODER2_AVAILABLE) {
            encoder_poll(&encoder2_data);
        }
        
        // Send encoder data over CAN periodically
        if (current_time - last_transmission_time >= ENCODER_POLLING_RATE_MS) {
            
            // Prepare CAN message for encoder 1
            // Message format: [Direction(1)] [Velocity(3)] [Position(4)]
            // Direction: 0=stopped, 1=forward, 2=reverse
            // Velocity: signed 24-bit value (pulses per second)
            // Position: unsigned 32-bit value (pulse count from TC counter)
            uint8_t enc1_data[8];
            
            // Pack direction (1 byte) - 0=stopped, 1=forward, 2=reverse
            // Use actual encoder direction, or 0 if no movement detected
            enc1_data[0] = encoder1_data.direction;
            
            // Pack velocity (3 bytes) - signed 24-bit value
            // Use actual encoder velocity
            int32_t velocity = encoder1_data.velocity;
            enc1_data[1] = (uint8_t)(velocity & 0xFF);
            enc1_data[2] = (uint8_t)((velocity >> 8) & 0xFF);
            enc1_data[3] = (uint8_t)((velocity >> 16) & 0xFF);
            
            // Pack position (4 bytes) - unsigned 32-bit value
            // Use actual encoder position
            uint32_t position_value = encoder1_data.position;
            enc1_data[4] = (uint8_t)(position_value & 0xFF);
            enc1_data[5] = (uint8_t)((position_value >> 8) & 0xFF);
            enc1_data[6] = (uint8_t)((position_value >> 16) & 0xFF);
            enc1_data[7] = (uint8_t)((position_value >> 24) & 0xFF);
            
            // Debug: Add diagnostic information
            static uint8_t debug_counter = 0;
            enc1_data[0] = debug_counter++; // Incrementing counter to verify transmission
            
            can_app_tx(CAN_ID_ENCODER1_DIR_VEL, enc1_data, 8);
            
            // Send encoder 2 data if available
            if (ENCODER2_AVAILABLE) {
                // Message format: [Direction(1)] [Velocity(3)] [Position(4)]
                // Direction: 0=stopped, 1=forward, 2=reverse
                // Velocity: signed 24-bit value (pulses per second)
                // Position: unsigned 32-bit value (pulse count from TC counter)
                uint8_t enc2_data[8];
                
                // Pack direction (1 byte) - 0=stopped, 1=forward, 2=reverse
                // Use actual encoder direction, or 0 if no movement detected
                enc2_data[0] = encoder2_data.direction;
                
                // Pack velocity (3 bytes) - signed 24-bit value
                // Use actual encoder velocity
                int32_t velocity2 = encoder2_data.velocity;
                enc2_data[1] = (uint8_t)(velocity2 & 0xFF);
                enc2_data[2] = (uint8_t)((velocity2 >> 8) & 0xFF);
                enc2_data[3] = (uint8_t)((velocity2 >> 16) & 0xFF);
                
                // Pack position (4 bytes) - unsigned 32-bit value
                // Use actual encoder position
                uint32_t position_value2 = encoder2_data.position;
                enc2_data[4] = (uint8_t)(position_value2 & 0xFF);
                enc2_data[5] = (uint8_t)((position_value2 >> 8) & 0xFF);
                enc2_data[6] = (uint8_t)((position_value2 >> 16) & 0xFF);
                enc2_data[7] = (uint8_t)((position_value2 >> 24) & 0xFF);
                
                can_app_tx(CAN_ID_ENCODER2_DIR_VEL, enc2_data, 8);
            }
            
            last_transmission_time = current_time;
        }
        
        // Small delay to prevent excessive CPU usage
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}