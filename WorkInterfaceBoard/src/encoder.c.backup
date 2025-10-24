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

#ifndef TickType_t
typedef portTickType TickType_t; // Backward-compatible alias if TickType_t isn't defined
#endif
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_PERIOD_MS)) // Convert milliseconds to OS ticks
#endif

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS portTICK_RATE_MS // Legacy macro mapping
#endif

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

bool encoder_tc_channel_init(uint32_t channel)
{
    // Configure PIO for TC TIOA and TIOB pins
    if (channel == TC_QUADRATURE_CHANNEL_ENC1) {
        // Configure PA5 as TIOA0 and PA1 as TIOB0
        pio_configure(PIOA, PIO_PERIPH_A, PIO_PA5, PIO_DEFAULT);  // TIOA0
        pio_configure(PIOA, PIO_PERIPH_A, PIO_PA1, PIO_DEFAULT);  // TIOB0
    } else if (channel == TC_QUADRATURE_CHANNEL_ENC2) {
        // Configure PA15 as TIOA1 and PA16 as TIOB1
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
                      TC_BMR_MAXFILT(TC_QUADRATURE_FILTER); // Set filter value
    }
    
    // Configure channel mode register for quadrature decoder
    // For SAM4E TC quadrature decoder, we need to configure the channel properly
    // The channel should be in capture mode with quadrature decoder enabled
    TC0->TC_CHANNEL[channel].TC_CMR = TC_CMR_TCCLKS_XC0 |  // Use XC0 clock
                                      TC_CMR_LDRA_RISING |  // Load on rising edge of TIOA
                                      TC_CMR_LDRB_FALLING;  // Load on falling edge of TIOB
    
    // Enable the channel
    TC0->TC_CHANNEL[channel].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
    
    return true;
}

uint32_t encoder_tc_get_position(uint32_t channel)
{
    if (channel >= 3) return 0; // Invalid channel
    
    // Read the current counter value
    return TC0->TC_CHANNEL[channel].TC_CV;
}

void encoder_tc_reset_position(uint32_t channel)
{
    if (channel >= 3) return; // Invalid channel
    
    // Reset the counter by disabling and re-enabling
    TC0->TC_CHANNEL[channel].TC_CCR = TC_CCR_CLKDIS;
    TC0->TC_CHANNEL[channel].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

uint8_t encoder_tc_get_direction(uint32_t channel)
{
    if (channel >= 3) return 0; // Invalid channel
    
    // Read direction from QDEC status register
    uint32_t qisr = TC0->TC_QISR;
    
    // Check if direction change interrupt is pending
    if (qisr & TC_QISR_DIRCHG) {
        // Read direction bit
        return (qisr & TC_QISR_DIR) ? 2 : 1; // 1=forward, 2=reverse
    }
    
    return 0; // No direction change or stopped
}

// Debug function to check TC status
void encoder_debug_tc_status(void)
{
    volatile uint32_t tc_bmr = TC0->TC_BMR;
    volatile uint32_t tc_qisr = TC0->TC_QISR;
    volatile uint32_t tc_ch0_cv = TC0->TC_CHANNEL[0].TC_CV;
    volatile uint32_t tc_ch1_cv = TC0->TC_CHANNEL[1].TC_CV;
    volatile uint32_t tc_ch0_cmr = TC0->TC_CHANNEL[0].TC_CMR;
    volatile uint32_t tc_ch1_cmr = TC0->TC_CHANNEL[1].TC_CMR;
    volatile uint32_t tc_ch0_sr = TC0->TC_CHANNEL[0].TC_SR;
    volatile uint32_t tc_ch1_sr = TC0->TC_CHANNEL[1].TC_SR;
    
    // Store debug values (they will be visible in debugger)
    (void)tc_bmr; (void)tc_qisr; (void)tc_ch0_cv; (void)tc_ch1_cv;
    (void)tc_ch0_cmr; (void)tc_ch1_cmr; (void)tc_ch0_sr; (void)tc_ch1_sr;
}

// Debug function to read encoder pins directly as GPIO
void encoder_debug_gpio_read(void)
{
    // Read encoder pins as GPIO to verify they're receiving signals
    volatile uint32_t enc1_a_gpio = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA5);
    volatile uint32_t enc1_b_gpio = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA1);
    volatile uint32_t enc2_a_gpio = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA15);
    volatile uint32_t enc2_b_gpio = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA16);
    
    // Store debug values
    (void)enc1_a_gpio; (void)enc1_b_gpio; (void)enc2_a_gpio; (void)enc2_b_gpio;
}

// Simple GPIO-based encoder reading for debugging
uint32_t encoder_gpio_read_position(uint32_t channel)
{
    static uint32_t last_a_state[2] = {0, 0};
    static uint32_t last_b_state[2] = {0, 0};
    static uint32_t position[2] = {0, 0};
    
    uint32_t a_pin, b_pin;
    
    if (channel == 0) {
        a_pin = PIO_PA5;
        b_pin = PIO_PA1;
    } else if (channel == 1) {
        a_pin = PIO_PA15;
        b_pin = PIO_PA16;
    } else {
        return 0;
    }
    
    // Read current state
    uint32_t a_state = pio_get(PIOA, PIO_TYPE_PIO_INPUT, a_pin);
    uint32_t b_state = pio_get(PIOA, PIO_TYPE_PIO_INPUT, b_pin);
    
    // Simple quadrature decoding
    if (a_state != last_a_state[channel]) {
        if (a_state == b_state) {
            position[channel]++;
        } else {
            position[channel]--;
        }
        last_a_state[channel] = a_state;
    }
    
    if (b_state != last_b_state[channel]) {
        if (a_state != b_state) {
            position[channel]++;
        } else {
            position[channel]--;
        }
        last_b_state[channel] = b_state;
    }
    
    return position[channel];
}

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
    
    // Debug: Store raw TC counter value for debugging
    volatile uint32_t debug_tc_counter = current_position;
    volatile uint32_t debug_tc_channel = enc_data->tc_channel;
    
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
    
    // Calculate velocity periodically
    if (current_time - enc_data->velocity_window_start >= VELOCITY_CALC_WINDOW_MS) {
        enc_data->velocity = calculate_velocity(enc_data, current_time);
        apply_velocity_smoothing(enc_data);
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

void apply_velocity_smoothing(encoder_data_t* enc_data)
{
    // Apply exponential smoothing to reduce jerky velocity changes
    float smoothing_factor = VELOCITY_SMOOTHING_FACTOR;
    enc_data->smoothed_velocity = (int32_t)(smoothing_factor * enc_data->smoothed_velocity + 
                                           (1.0f - smoothing_factor) * enc_data->velocity);
}

bool is_direction_change_allowed(encoder_data_t* enc_data, uint32_t current_time, uint8_t new_direction)
{
    // Don't allow direction changes if we're already in that direction
    if (enc_data->direction == new_direction) {
        return false;
    }
    
    // Don't allow direction changes too frequently (debouncing)
    if (current_time - enc_data->last_direction_change < DIRECTION_DEBOUNCE_MS) {
        return false;
    }
    
    // Only allow direction change if we have some velocity (not just noise)
    if (abs(enc_data->velocity) < 2) { // Minimum velocity threshold
        return false;
    }
    
    return true;
}

void encoder_task(void *arg)
{
    (void)arg; // Unused parameter
    
    // Initialize encoders with TC quadrature decoder approach
    if (!encoder_init()) {
        // Encoder initialization failed
        while(1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // Wait a bit for encoders to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Debug: Test TC counter by manually incrementing it
    // This will help verify if the TC is working at all
    volatile uint32_t test_counter = 0;
    for (int i = 0; i < 1000; i++) {
        test_counter = encoder_tc_get_position(0);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    volatile uint32_t debug_test_counter = test_counter;
    
    // Debug: Test if we can manually increment the TC counter
    // This will help verify if the TC is working at all
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG; // Software trigger to increment
    volatile uint32_t debug_manual_trigger = encoder_tc_get_position(0);
    
    uint32_t last_enc1_position = 0;
    uint32_t last_enc2_position = 0;
    uint32_t last_transmission_time = 0;
    uint32_t debug_task_counter = 0;
    
    for (;;) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        debug_task_counter++;
        
        // Poll both encoders
        encoder_poll(&encoder1_data);
        if (ENCODER2_AVAILABLE) {
            encoder_poll(&encoder2_data);
        }
        
        // Debug: Test GPIO-based encoder reading as fallback
        volatile uint32_t debug_gpio_enc1 = encoder_gpio_read_position(0);
        volatile uint32_t debug_gpio_enc2 = encoder_gpio_read_position(1);
        
        // Send encoder data over CAN periodically
        if (current_time - last_transmission_time >= ENCODER_POLLING_RATE_MS) {
            // Debug: Check TC status and GPIO readings periodically
            encoder_debug_tc_status();
            encoder_debug_gpio_read();
            
            // Debug: Test if encoder pins are receiving signals
            volatile uint32_t debug_enc1_a = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA5);
            volatile uint32_t debug_enc1_b = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA1);
            volatile uint32_t debug_enc2_a = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA15);
            volatile uint32_t debug_enc2_b = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA16);
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
            enc1_data[4] = (uint8_t)(encoder1_data.position & 0xFF);
            enc1_data[5] = (uint8_t)((encoder1_data.position >> 8) & 0xFF);
            enc1_data[6] = (uint8_t)((encoder1_data.position >> 16) & 0xFF);
            enc1_data[7] = (uint8_t)((encoder1_data.position >> 24) & 0xFF);
            
            
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
        
        // Wait for next transmission cycle
        vTaskDelay(pdMS_TO_TICKS(ENCODER_POLLING_RATE_MS));
    }
}