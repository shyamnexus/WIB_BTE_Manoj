#include "encoder.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "can_app.h"

// Global encoder data structures
static encoder_data_t encoder1_data = {0};
static encoder_data_t encoder2_data = {0};

// Volatile counter for interrupt-based pulse counting
volatile uint32_t encoder1_pulse_count = 0;
volatile uint32_t encoder1_last_interrupt_time = 0;

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
    pio_clear(PIOD, ENC1_ENABLE_PIN);  // Enable encoder 1
    
    if (ENCODER2_AVAILABLE) {
        pio_configure(PIOD, PIO_OUTPUT_0, ENC2_ENABLE_PIN, PIO_DEFAULT);
        pio_clear(PIOD, ENC2_ENABLE_PIN);  // Enable encoder 2
    }
    
    // Initialize encoder data structures
    encoder1_data.position = 0;
    encoder1_data.velocity = 0;
    encoder1_data.smoothed_velocity = 0;
    encoder1_data.direction = 0;
    encoder1_data.state_a = 0;
    encoder1_data.state_b = 0;
    encoder1_data.prev_state_a = 0;
    encoder1_data.prev_state_b = 0;
    encoder1_data.last_update_time = 0;
    encoder1_data.last_direction_change = 0;
    encoder1_data.pulse_count = 0;
    encoder1_data.velocity_window_start = 0;
    
    encoder2_data.position = 0;
    encoder2_data.velocity = 0;
    encoder2_data.smoothed_velocity = 0;
    encoder2_data.direction = 0;
    encoder2_data.state_a = 0;
    encoder2_data.state_b = 0;
    encoder2_data.prev_state_a = 0;
    encoder2_data.prev_state_b = 0;
    encoder2_data.last_update_time = 0;
    encoder2_data.last_direction_change = 0;
    encoder2_data.pulse_count = 0;
    encoder2_data.velocity_window_start = 0;
    
    return true;
}

bool encoder_interrupt_init(void)
{
    // Enable PIO clocks for encoder pins
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);
    
    // Configure encoder pins as inputs with pull-up
    pio_configure(PIOA, PIO_INPUT, ENC1_A_PIN, PIO_PULLUP | PIO_DEBOUNCE);
    pio_configure(PIOA, PIO_INPUT, ENC1_B_PIN, PIO_PULLUP | PIO_DEBOUNCE);
    
    // Configure enable pins as outputs and set them high (enable encoders)
    pio_configure(PIOD, PIO_OUTPUT_0, ENC1_ENABLE_PIN, PIO_DEFAULT);
    pio_clear(PIOD, ENC1_ENABLE_PIN);  // Enable encoder 1
    
    // Configure interrupt for both encoder pins
    // Enable interrupt on both rising and falling edges for quadrature decoding
    if (pio_handler_set(PIOA, ID_PIOA, ENC1_A_PIN | ENC1_B_PIN, PIO_IT_EDGE, encoder_interrupt_handler) != 0) {
        // Failed to set interrupt handler
        return false;
    }
    
    // Set interrupt priority
    NVIC_SetPriority(PIOA_IRQn, ENCODER_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(PIOA_IRQn);
    
    // Enable PIO interrupts
    pio_enable_interrupt(PIOA, ENC1_A_PIN | ENC1_B_PIN);
    
    // Initialize pulse counter
    encoder1_pulse_count = 0;
    encoder1_last_interrupt_time = 0;
    
    // Debug: Store initialization status
    volatile uint32_t debug_encoder_interrupt_init_success = 1;
    
    return true;
}

void encoder_interrupt_handler(const uint32_t id, const uint32_t index)
{
    (void)id;   // Unused parameter
    (void)index; // Unused parameter
    
    // Get interrupt status
    uint32_t status = pio_get_interrupt_status(PIOA);
    
    // Debug: Store interrupt status for debugging
    volatile uint32_t debug_interrupt_status = status;
    
    // Check if our pins triggered the interrupt
    if (status & (ENC1_A_PIN | ENC1_B_PIN)) {
        // Simple debouncing - check if enough time has passed since last interrupt
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (current_time - encoder1_last_interrupt_time > ENCODER_DEBOUNCE_US / 1000) {
            // Increment pulse counter
            encoder1_pulse_count++;
            encoder1_last_interrupt_time = current_time;
            
            // Debug: Store pulse count for debugging
            volatile uint32_t debug_pulse_count = encoder1_pulse_count;
        }
    }
    
    // Clear interrupt status
    pio_disable_interrupt(PIOA, ENC1_A_PIN | ENC1_B_PIN);
    pio_enable_interrupt(PIOA, ENC1_A_PIN | ENC1_B_PIN);
}

uint32_t encoder_get_pulse_count(void)
{
    return encoder1_pulse_count;
}

void encoder_reset_pulse_count(void)
{
    encoder1_pulse_count = 0;
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
        uint8_t new_direction = 0;
        bool position_changed = false;
        
        if (enc_data->prev_state_a == 0 && enc_data->prev_state_b == 0) {
            if (enc_data->state_a == 1 && enc_data->state_b == 0) {
                // Forward: A leads
                enc_data->position++;
                new_direction = 1;
                enc_data->pulse_count++;
                position_changed = true;
            } else if (enc_data->state_a == 0 && enc_data->state_b == 1) {
                // Reverse: B leads
                enc_data->position--;
                new_direction = 2;
                enc_data->pulse_count++;
                position_changed = true;
            }
        } else if (enc_data->prev_state_a == 1 && enc_data->prev_state_b == 0) {
            if (enc_data->state_a == 1 && enc_data->state_b == 1) {
                // Forward: A leads
                enc_data->position++;
                new_direction = 1;
                enc_data->pulse_count++;
                position_changed = true;
            } else if (enc_data->state_a == 0 && enc_data->state_b == 0) {
                // Reverse: B leads
                enc_data->position--;
                new_direction = 2;
                enc_data->pulse_count++;
                position_changed = true;
            }
        } else if (enc_data->prev_state_a == 1 && enc_data->prev_state_b == 1) {
            if (enc_data->state_a == 0 && enc_data->state_b == 1) {
                // Forward: A leads
                enc_data->position++;
                new_direction = 1;
                enc_data->pulse_count++;
                position_changed = true;
            } else if (enc_data->state_a == 1 && enc_data->state_b == 0) {
                // Reverse: B leads
                enc_data->position--;
                new_direction = 2;
                enc_data->pulse_count++;
                position_changed = true;
            }
        } else if (enc_data->prev_state_a == 0 && enc_data->prev_state_b == 1) {
            if (enc_data->state_a == 0 && enc_data->state_b == 0) {
                // Forward: A leads
                enc_data->position++;
                new_direction = 1;
                enc_data->pulse_count++;
                position_changed = true;
            } else if (enc_data->state_a == 1 && enc_data->state_b == 1) {
                // Reverse: B leads
                enc_data->position--;
                new_direction = 2;
                enc_data->pulse_count++;
                position_changed = true;
            }
        }
        
        // Update direction only if change is allowed (debouncing)
        if (position_changed && is_direction_change_allowed(enc_data, current_time, new_direction)) {
            enc_data->direction = new_direction;
            enc_data->last_direction_change = current_time;
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
    
    // Initialize encoders with interrupt-based approach
    if (!encoder_interrupt_init()) {
        // Encoder initialization failed
        while(1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // Wait a bit for encoders to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    uint32_t last_pulse_count = 0;
    uint32_t last_transmission_time = 0;
    
    for (;;) {
        // Get current pulse count from interrupt handler
        uint32_t current_pulse_count = encoder_get_pulse_count();
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Calculate velocity based on pulse count difference
        uint32_t pulse_delta = current_pulse_count - last_pulse_count;
        uint32_t time_delta = current_time - last_transmission_time;
        
        // Send encoder 1 data over CAN periodically
        if (time_delta >= ENCODER_POLLING_RATE_MS) {
            // Prepare CAN message with pulse count and calculated velocity
            uint8_t enc1_data[8];
            
            // Pack pulse count (4 bytes)
            enc1_data[0] = (uint8_t)(current_pulse_count & 0xFF);
            enc1_data[1] = (uint8_t)((current_pulse_count >> 8) & 0xFF);
            enc1_data[2] = (uint8_t)((current_pulse_count >> 16) & 0xFF);
            enc1_data[3] = (uint8_t)((current_pulse_count >> 24) & 0xFF);
            
            // Calculate and pack velocity (pulses per second)
            uint32_t velocity = 0;
            if (time_delta > 0) {
                velocity = (pulse_delta * 1000) / time_delta;
            }
            enc1_data[4] = (uint8_t)(velocity & 0xFF);
            enc1_data[5] = (uint8_t)((velocity >> 8) & 0xFF);
            enc1_data[6] = (uint8_t)((velocity >> 16) & 0xFF);
            enc1_data[7] = (uint8_t)((velocity >> 24) & 0xFF);
            
            can_app_tx(CAN_ID_ENCODER1_DIR_VEL, enc1_data, 8);
            
            // Debug: Store encoder data for debugging
            volatile uint32_t debug_enc1_pulse_count = current_pulse_count;
            volatile uint32_t debug_enc1_velocity = velocity;
            volatile uint32_t debug_enc1_pulse_delta = pulse_delta;
            
            // Update for next calculation
            last_pulse_count = current_pulse_count;
            last_transmission_time = current_time;
        }
        
        // Wait for next transmission cycle
        vTaskDelay(pdMS_TO_TICKS(ENCODER_POLLING_RATE_MS));
    }
}