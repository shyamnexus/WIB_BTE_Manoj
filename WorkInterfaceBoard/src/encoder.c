#include "encoder.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "can_app.h"
#include "pio_handler.h"
#include "interrupt.h"

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
    volatile bool state_changed; // Flag to indicate state change from interrupt
} simple_encoder_t;

static simple_encoder_t enc1_simple = {0};
static simple_encoder_t enc2_simple = {0};

// Interrupt handler for encoder 1 pins
// Not used in TC QDEC mode (kept for compatibility, no-op)
void encoder1_interrupt_handler(uint32_t id, uint32_t mask)
{
    (void)id;
    (void)mask;
}

// Interrupt handler for encoder 2 pins
// Not used (encoder 2 disabled)
void encoder2_interrupt_handler(uint32_t id, uint32_t mask)
{
    (void)id;
    (void)mask;
}

// Quadrature encoder state table for direction detection
// State transitions: 00->01->11->10->00 (forward) or 00->10->11->01->00 (reverse)
static const int8_t state_table[16] = { 0 };

// Encoder initialization
// TC0 QDEC state
static volatile uint16_t tc0_last_cv = 0;
static volatile int32_t tc0_position_accumulated = 0;

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
    encoder1_data.tc_channel = 0;

    // Route PA0/PA1 to TC0 peripheral (TIOA0/TIOB0 on Peripheral B)
    pmc_enable_periph_clk(ID_PIOA);
    pio_configure(PIOA, PIO_PERIPH_B, PIO_PA0B_TIOA0, PIO_DEFAULT);
    pio_configure(PIOA, PIO_PERIPH_B, PIO_PA1B_TIOB0, PIO_DEFAULT);

    // Optional: configure enable pin low if wired
    pmc_enable_periph_clk(ID_PIOD);
    pio_configure(PIOD, PIO_OUTPUT_0, ENC1_ENABLE_PIN, PIO_DEFAULT);
    pio_clear(PIOD, ENC1_ENABLE_PIN);

    // Enable TC0 peripheral clock
    pmc_enable_periph_clk(ID_TC0);

    // Configure TC0 Block Mode Register for Quadrature Decoder
    // - QDEN: enable QDEC
    // - POSEN: enable position measurement
    // - EDGPHA: count on both edges phase (typical for QDEC)
    // - FILTER + MAXFILT: enable simple glitch filter
    TC0->TC_BMR = TC_BMR_QDEN | TC_BMR_POSEN | TC_BMR_EDGPHA | TC_BMR_FILTER | (10 << TC_BMR_MAXFILT_Pos);

    // Disable QDEC interrupts (we will poll)
    TC0->TC_QIDR = TC_QIDR_IDX | TC_QIDR_DIRCHG | TC_QIDR_QERR;

    // Enable channel 0 clock so CV0 runs
    TC0->TC_CHANNEL[0].TC_CMR = 0;
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;

    // Initialize software position baseline
    tc0_last_cv = (uint16_t)TC0->TC_CHANNEL[0].TC_CV;
    tc0_position_accumulated = 0;

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

// Process encoder using simple state machine (now handled in interrupt)
// This function is kept for compatibility but is no longer used
void process_encoder(simple_encoder_t* enc, uint8_t encoder_num)
{
    // This function is now handled by interrupt handlers
    // Kept for compatibility but not used
}

// Update encoder data from interrupt-driven state
void encoder_poll(encoder_data_t* enc_data)
{
    // Read current 16-bit counter value from TC0 Channel 0
    uint16_t current_cv = (uint16_t)TC0->TC_CHANNEL[0].TC_CV;

    // Compute signed delta with wrap handling
    int16_t delta = (int16_t)(current_cv - tc0_last_cv);
    tc0_last_cv = current_cv;

    // Accumulate software position (extend to 32-bit)
    tc0_position_accumulated += (int32_t)delta;

    // Update velocity every ENCODER_POLLING_RATE_MS (called at that cadence)
    // Convert to pulses per second: delta * (1000 / period_ms)
    const int32_t pulses_per_second = (int32_t)delta * (1000 / ENCODER_POLLING_RATE_MS);

    enc_data->position = (uint32_t)tc0_position_accumulated;
    enc_data->velocity = pulses_per_second;
    enc_data->smoothed_velocity = pulses_per_second;

    if (pulses_per_second > 0) {
        enc_data->direction = 1; // forward
    } else if (pulses_per_second < 0) {
        enc_data->direction = 2; // reverse
    } else {
        enc_data->direction = 0; // stopped
    }

    enc_data->last_update_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
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