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
#define ENCODER_POLLING_RATE_MS    20      // Polling rate in milliseconds (50Hz)
#define VELOCITY_WINDOW_MS         100     // Velocity calculation window in milliseconds

// Simple encoder state machine
typedef struct {
    uint8_t current_state;     // Current state (0-3)
    uint8_t last_state;        // Previous state
    int32_t position;          // Current position
    int32_t velocity;          // Current velocity (pulses per second)
    uint32_t last_pulse_time;  // Time of last pulse
    uint32_t pulse_count;      // Pulse count for velocity calculation
    uint32_t velocity_window_start; // Start time of velocity calculation window
} simple_encoder_t;

static simple_encoder_t enc1_simple = {0};
static simple_encoder_t enc2_simple = {0};

// Quadrature encoder state table for direction detection
// State transitions: 00->01->11->10->00 (forward) or 00->10->11->01->00 (reverse)
static const int8_t state_table[16] = {
    // prev_state=0: 00, 01, 10, 11
    0, 1, -1, 0,  // current_state=0 (00)
    1, 0, 0, 1,   // current_state=1 (01) 
    -1, 0, 0, -1, // current_state=2 (10)
    0, -1, 1, 0   // current_state=3 (11)
};

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
    encoder1_data.tc_channel = 0; // Not used in simple mode
    
    encoder2_data.position = 0;
    encoder2_data.velocity = 0;
    encoder2_data.smoothed_velocity = 0;
    encoder2_data.direction = 0;
    encoder2_data.last_position = 0;
    encoder2_data.last_update_time = 0;
    encoder2_data.last_direction_change = 0;
    encoder2_data.pulse_count = 0;
    encoder2_data.velocity_window_start = 0;
    encoder2_data.tc_channel = 1; // Not used in simple mode
    
    // Initialize simple encoder structures
    enc1_simple.current_state = 0;
    enc1_simple.last_state = 0;
    enc1_simple.position = 0;
    enc1_simple.velocity = 0;
    enc1_simple.last_pulse_time = 0;
    enc1_simple.pulse_count = 0;
    enc1_simple.velocity_window_start = 0;
    
    enc2_simple.current_state = 0;
    enc2_simple.last_state = 0;
    enc2_simple.position = 0;
    enc2_simple.velocity = 0;
    enc2_simple.last_pulse_time = 0;
    enc2_simple.pulse_count = 0;
    enc2_simple.velocity_window_start = 0;
    
    // Enable PIO clocks
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);
    
    // Configure encoder pins as inputs with pull-ups
    // Encoder 1: PA5 (A), PA1 (B)
    pio_configure(PIOA, PIO_INPUT, PIO_PA5, PIO_PULLUP);
    pio_configure(PIOA, PIO_INPUT, PIO_PA1, PIO_PULLUP);
    
    // Encoder 2: PA15 (A), PA16 (B)
    if (ENCODER2_AVAILABLE) {
        pio_configure(PIOA, PIO_INPUT, PIO_PA15, PIO_PULLUP);
        pio_configure(PIOA, PIO_INPUT, PIO_PA16, PIO_PULLUP);
    }
    
    // Configure enable pins as outputs and set them low (enable encoders)
    pio_configure(PIOD, PIO_OUTPUT_0, ENC1_ENABLE_PIN, PIO_DEFAULT);
    pio_clear(PIOD, ENC1_ENABLE_PIN);  // Enable encoder 1 (set low)
    
    if (ENCODER2_AVAILABLE) {
        pio_configure(PIOD, PIO_OUTPUT_0, ENC2_ENABLE_PIN, PIO_DEFAULT);
        pio_clear(PIOD, ENC2_ENABLE_PIN);  // Enable encoder 2 (set low)
    }
    
    return true;
}

// Read encoder state (A and B pins)
uint8_t read_encoder_state(uint8_t encoder_num)
{
    if (encoder_num == 1) {
        // Read PA5 (A) and PA1 (B) for encoder 1
        uint8_t state = 0;
        if (pio_get(PIOA, PIO_INPUT, PIO_PA5)) state |= 0x01; // A bit
        if (pio_get(PIOA, PIO_INPUT, PIO_PA1)) state |= 0x02; // B bit
        return state;
    } else if (encoder_num == 2 && ENCODER2_AVAILABLE) {
        // Read PA15 (A) and PA16 (B) for encoder 2
        uint8_t state = 0;
        if (pio_get(PIOA, PIO_INPUT, PIO_PA15)) state |= 0x01; // A bit
        if (pio_get(PIOA, PIO_INPUT, PIO_PA16)) state |= 0x02; // B bit
        return state;
    }
    return 0;
}

// Process encoder using simple state machine
void process_encoder(simple_encoder_t* enc, uint8_t encoder_num)
{
    uint8_t current_state = read_encoder_state(encoder_num);
    
    // Only process if state changed
    if (current_state != enc->current_state) {
        // Calculate direction using state table
        int8_t direction = state_table[(enc->last_state << 2) | current_state];
        
        if (direction != 0) {
            // Update position
            enc->position += direction;
            
            // Update velocity calculation
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            enc->pulse_count++;
            
            // Calculate velocity every VELOCITY_WINDOW_MS
            if (current_time - enc->velocity_window_start >= VELOCITY_WINDOW_MS) {
                uint32_t time_delta = current_time - enc->velocity_window_start;
                if (time_delta > 0) {
                    enc->velocity = (enc->pulse_count * 1000) / time_delta;
                    if (direction < 0) {
                        enc->velocity = -enc->velocity; // Negative for reverse
                    }
                }
                enc->velocity_window_start = current_time;
                enc->pulse_count = 0;
            }
            
            enc->last_pulse_time = current_time;
            
            // Debug output (can be removed in production)
            // printf("Encoder %d: State %d->%d, Dir %d, Pos %d\n", 
            //        encoder_num, enc->last_state, current_state, direction, enc->position);
        }
        
        enc->last_state = enc->current_state;
        enc->current_state = current_state;
    }
}

// Poll encoder for position changes
void encoder_poll(encoder_data_t* enc_data)
{
    // Skip polling if this is encoder2 and it's not available
    if (enc_data == &encoder2_data && !ENCODER2_AVAILABLE) {
        return;
    }
    
    // Get current time
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Process the appropriate simple encoder
    simple_encoder_t* simple_enc = (enc_data == &encoder1_data) ? &enc1_simple : &enc2_simple;
    uint8_t encoder_num = (enc_data == &encoder1_data) ? 1 : 2;
    
    // Process encoder state machine
    process_encoder(simple_enc, encoder_num);
    
    // Update encoder data structure
    enc_data->position = (uint32_t)simple_enc->position;
    enc_data->velocity = simple_enc->velocity;
    enc_data->smoothed_velocity = simple_enc->velocity; // No smoothing for now
    
    // Determine direction
    if (simple_enc->velocity > 0) {
        enc_data->direction = 1; // Forward
    } else if (simple_enc->velocity < 0) {
        enc_data->direction = 2; // Reverse
    } else {
        // Check if we've had recent movement
        if (current_time - simple_enc->last_pulse_time > 50) { // 50ms timeout
            enc_data->direction = 0; // Stopped
        }
    }
    
    enc_data->last_update_time = current_time;
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
            // Position: signed 32-bit value (pulse count)
            uint8_t enc1_data[8];
            
            // Pack direction (1 byte)
            enc1_data[0] = encoder1_data.direction;
            
            // Pack velocity (3 bytes) - signed 24-bit value
            int32_t velocity = encoder1_data.velocity;
            enc1_data[1] = (uint8_t)(velocity & 0xFF);
            enc1_data[2] = (uint8_t)((velocity >> 8) & 0xFF);
            enc1_data[3] = (uint8_t)((velocity >> 16) & 0xFF);
            
            // Pack position (4 bytes) - signed 32-bit value
            int32_t position_value = (int32_t)encoder1_data.position;
            enc1_data[4] = (uint8_t)(position_value & 0xFF);
            enc1_data[5] = (uint8_t)((position_value >> 8) & 0xFF);
            enc1_data[6] = (uint8_t)((position_value >> 16) & 0xFF);
            enc1_data[7] = (uint8_t)((position_value >> 24) & 0xFF);
            
            can_app_tx(CAN_ID_ENCODER1_DIR_VEL, enc1_data, 8);
            
            // Send encoder 2 data if available
            if (ENCODER2_AVAILABLE) {
                uint8_t enc2_data[8];
                
                // Pack direction (1 byte)
                enc2_data[0] = encoder2_data.direction;
                
                // Pack velocity (3 bytes) - signed 24-bit value
                int32_t velocity2 = encoder2_data.velocity;
                enc2_data[1] = (uint8_t)(velocity2 & 0xFF);
                enc2_data[2] = (uint8_t)((velocity2 >> 8) & 0xFF);
                enc2_data[3] = (uint8_t)((velocity2 >> 16) & 0xFF);
                
                // Pack position (4 bytes) - signed 32-bit value
                int32_t position_value2 = (int32_t)encoder2_data.position;
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

// Legacy functions (kept for compatibility but not used in simple mode)
bool encoder_tc_init(void) { return true; }
bool encoder_tc_channel_init(uint32_t channel) { return true; }
uint32_t encoder_tc_get_position(uint32_t channel) { return 0; }
void encoder_tc_reset_position(uint32_t channel) { }
uint8_t encoder_tc_get_direction(uint32_t channel) { return 0; }
int32_t calculate_velocity(encoder_data_t* enc_data, uint32_t current_time) { return 0; }
void apply_velocity_smoothing(encoder_data_t* enc_data) { }
bool is_direction_change_allowed(encoder_data_t* enc_data, uint32_t current_time, uint8_t new_direction) { return true; }