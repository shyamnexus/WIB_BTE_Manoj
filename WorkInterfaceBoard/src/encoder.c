#include "encoder.h"
#include "asf.h"
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
    
    // Initialize TC0 channel 0 for ENC1
    if (!encoder_tc_channel_init(TC_QUADRATURE_CHANNEL_ENC1)) {
        return false;
    }
    
    // Initialize TC0 channel 1 for ENC2 if available
    if (ENCODER2_AVAILABLE) {
        if (!encoder_tc_channel_init(TC_QUADRATURE_CHANNEL_ENC2)) {
            return false;
        }
    }
    
    return true;
}

// TC channel initialization
bool encoder_tc_channel_init(uint32_t channel)
{
    // Configure PIO for TC TIOA and TIOB pins
    if (channel == TC_QUADRATURE_CHANNEL_ENC1) {
        // Configure PA0 as TIOA0 and PA1 as TIOB0
        pio_configure(PIOA, PIO_PERIPH_A, PIO_PA0, PIO_DEFAULT);  // TIOA0
        pio_configure(PIOA, PIO_PERIPH_A, PIO_PA1, PIO_DEFAULT);  // TIOB0
    } else if (channel == TC_QUADRATURE_CHANNEL_ENC2) {
        // Configure PA15 as TIOA1 and PA16 as TIOB1 (disabled due to SPI conflict)
        pio_configure(PIOA, PIO_PERIPH_A, PIO_PA15, PIO_DEFAULT); // TIOA1
        pio_configure(PIOA, PIO_PERIPH_A, PIO_PA16, PIO_DEFAULT); // TIOB1
    } else {
        return false;
    }
    
    // Configure TC for quadrature decoder mode
    // Set up Block Mode Register for quadrature decoding (only once, not per channel)
    if (channel == TC_QUADRATURE_CHANNEL_ENC1) {
        TC0->TC_BMR = TC_BMR_QDEN |                    // Enable quadrature decoder
                      TC_BMR_POSEN |                   // Enable position counting
                      TC_BMR_SPEEDEN |                 // Enable speed counting
                      TC_BMR_FILTER |                  // Enable glitch filter
                      TC_BMR_MAXFILT(TC_QUADRATURE_FILTER) | // Set filter value
                      TC_BMR_TC0XC0S_TIOA0 |           // Connect TIOA0 to XC0 for encoder 1
                      TC_BMR_TC1XC1S_TIOA1;            // Connect TIOA1 to XC1 for encoder 2
    }
    
    // Configure channel mode register for quadrature decoder
    // For SAM4E TC quadrature decoder, use external clock from encoder signals
    if (channel == TC_QUADRATURE_CHANNEL_ENC1) {
        TC0->TC_CHANNEL[channel].TC_CMR = TC_CMR_TCCLKS_XC0 |  // Use XC0 clock (TIOA0)
                                      TC_CMR_BURST_NONE;        // No external gating
    } else if (channel == TC_QUADRATURE_CHANNEL_ENC2) {
        TC0->TC_CHANNEL[channel].TC_CMR = TC_CMR_TCCLKS_XC1 |  // Use XC1 clock (TIOA1)
                                      TC_CMR_BURST_NONE;        // No external gating
    }
    
    // Enable the channel
    TC0->TC_CHANNEL[channel].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
    
    // Debug: Store register values for verification
    volatile uint32_t tc_bmr = TC0->TC_BMR;
    volatile uint32_t tc_cmr = TC0->TC_CHANNEL[channel].TC_CMR;
    volatile uint32_t tc_cv = TC0->TC_CHANNEL[channel].TC_CV;
    
    // Additional debug: Check if TC is actually running
    volatile uint32_t tc_sr = TC0->TC_CHANNEL[channel].TC_SR;
    volatile uint32_t tc_qisr = TC0->TC_QISR;
    
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
    return (TC0->TC_QISR & TC_QISR_DIRCHG) ? 1 : 0;
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
    
    // Debug: Store current TC register values for debugging
    volatile uint32_t tc_cv = TC0->TC_CHANNEL[enc_data->tc_channel].TC_CV;
    volatile uint32_t tc_cmr = TC0->TC_CHANNEL[enc_data->tc_channel].TC_CMR;
    volatile uint32_t tc_bmr = TC0->TC_BMR;
    volatile uint32_t tc_qisr = TC0->TC_QISR;
    
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
    
    // Debug: Test TC configuration
    volatile uint32_t debug_tc_bmr = TC0->TC_BMR;
    volatile uint32_t debug_tc_ch0_cmr = TC0->TC_CHANNEL[0].TC_CMR;
    volatile uint32_t debug_tc_ch0_cv = TC0->TC_CHANNEL[0].TC_CV;
    volatile uint32_t debug_tc_ch0_sr = TC0->TC_CHANNEL[0].TC_SR;
    
    // Test: Try to manually increment the counter to verify TC is working
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG; // Software trigger
    volatile uint32_t debug_tc_ch0_cv_after_trigger = TC0->TC_CHANNEL[0].TC_CV;
    
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
            // Debug: Add some test data to verify CAN is working
            static uint32_t debug_counter = 0;
            debug_counter++;
            
            // Prepare CAN message for encoder 1
            // Message format: [Direction(1)] [Velocity(3)] [Position(4)]
            // Direction: 0=stopped, 1=forward, 2=reverse
            // Velocity: signed 24-bit value (pulses per second)
            // Position: unsigned 32-bit value (pulse count from TC counter)
            uint8_t enc1_data[8];
            
            // Pack direction (1 byte) - 0=stopped, 1=forward, 2=reverse
            enc1_data[0] = encoder1_data.direction;
            
            // Pack velocity (3 bytes) - signed 24-bit value
            enc1_data[1] = (uint8_t)(encoder1_data.velocity & 0xFF);
            enc1_data[2] = (uint8_t)((encoder1_data.velocity >> 8) & 0xFF);
            enc1_data[3] = (uint8_t)((encoder1_data.velocity >> 16) & 0xFF);
            
            // Pack position (4 bytes) - unsigned 32-bit value
            // For debugging, include some test data
            uint32_t debug_position = encoder1_data.position;
            if (debug_position == 0) {
                debug_position = debug_counter; // Use debug counter if position is 0
            }
            enc1_data[4] = (uint8_t)(debug_position & 0xFF);
            enc1_data[5] = (uint8_t)((debug_position >> 8) & 0xFF);
            enc1_data[6] = (uint8_t)((debug_position >> 16) & 0xFF);
            enc1_data[7] = (uint8_t)((debug_position >> 24) & 0xFF);
            
            can_app_tx(CAN_ID_ENCODER1_DIR_VEL, enc1_data, 8);
            
            // Send encoder 2 data if available
            if (ENCODER2_AVAILABLE) {
                // Message format: [Direction(1)] [Velocity(3)] [Position(4)]
                // Direction: 0=stopped, 1=forward, 2=reverse
                // Velocity: signed 24-bit value (pulses per second)
                // Position: unsigned 32-bit value (pulse count from TC counter)
                uint8_t enc2_data[8];
                
                // Pack direction (1 byte) - 0=stopped, 1=forward, 2=reverse
                enc2_data[0] = encoder2_data.direction;
                
                // Pack velocity (3 bytes) - signed 24-bit value
                enc2_data[1] = (uint8_t)(encoder2_data.velocity & 0xFF);
                enc2_data[2] = (uint8_t)((encoder2_data.velocity >> 8) & 0xFF);
                enc2_data[3] = (uint8_t)((encoder2_data.velocity >> 16) & 0xFF);
                
                // Pack position (4 bytes) - unsigned 32-bit value
                enc2_data[4] = (uint8_t)(encoder2_data.position & 0xFF);
                enc2_data[5] = (uint8_t)((encoder2_data.position >> 8) & 0xFF);
                enc2_data[6] = (uint8_t)((encoder2_data.position >> 16) & 0xFF);
                enc2_data[7] = (uint8_t)((encoder2_data.position >> 24) & 0xFF);
                
                can_app_tx(CAN_ID_ENCODER2_DIR_VEL, enc2_data, 8);
            }
            
            last_transmission_time = current_time;
        }
        
        // Small delay to prevent excessive CPU usage
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
