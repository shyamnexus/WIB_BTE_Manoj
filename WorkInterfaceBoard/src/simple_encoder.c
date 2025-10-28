#include "simple_encoder.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "can_app.h"
#include "pio_handler.h"
#include "interrupt.h"

// Simple encoder data structure
static simple_encoder_data_t encoder_data = {0};

// Configuration constants
#define ENCODER_POLLING_RATE_MS    50      // Polling rate in milliseconds (20Hz)
#define VELOCITY_WINDOW_MS         200     // Velocity calculation window in milliseconds

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

static simple_encoder_t simple_enc = {0};

// Quadrature encoder state table for direction detection
// State transitions: 00->01->11->10->00 (forward) or 00->10->11->01->00 (reverse)
static const int8_t state_table[16] = {
    // prev_state=0: 00, 01, 10, 11
    0, 1, -1, 0,  // current_state=0 (00)
    1, 0, 0, 1,   // current_state=1 (01) 
    -1, 0, 0, -1, // current_state=2 (10)
    0, -1, 1, 0   // current_state=3 (11)
};

// Interrupt handler for encoder pins
void encoder_interrupt_handler(uint32_t id, uint32_t mask)
{
    // Read current state of both pins
    uint8_t current_state = 0;
    if (pio_get(PIOA, PIO_INPUT, PIO_PA0)) current_state |= 0x01; // A bit (PA0)
    if (pio_get(PIOA, PIO_INPUT, PIO_PA1)) current_state |= 0x02; // B bit (PA1)
    
    // Only process if state changed
    if (current_state != simple_enc.current_state) {
        // Calculate direction using state table
        int8_t direction = state_table[(simple_enc.last_state << 2) | current_state];
        
        if (direction != 0) {
            // Update position
            simple_enc.position += direction;
            
            // Update velocity calculation
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            simple_enc.pulse_count++;
            
            // Calculate velocity every VELOCITY_WINDOW_MS
            if (current_time - simple_enc.velocity_window_start >= VELOCITY_WINDOW_MS) {
                uint32_t time_delta = current_time - simple_enc.velocity_window_start;
                if (time_delta > 0) {
                    simple_enc.velocity = (simple_enc.pulse_count * 1000) / time_delta;
                    if (direction < 0) {
                        simple_enc.velocity = -simple_enc.velocity; // Negative for reverse
                    }
                }
                simple_enc.velocity_window_start = current_time;
                simple_enc.pulse_count = 0;
            }
            
            simple_enc.last_pulse_time = current_time;
        }
        
        simple_enc.last_state = simple_enc.current_state;
        simple_enc.current_state = current_state;
        simple_enc.state_changed = true;
        
        // Send immediate CAN message for pin state changes
        uint8_t pin_a_state = (current_state & 0x01) ? 1 : 0;
        uint8_t pin_b_state = (current_state & 0x02) ? 1 : 0;
        uint8_t payload[2] = { pin_a_state, pin_b_state };
        can_app_tx(CAN_ID_ENCODER_PINS, payload, 2);
    }
}

// Initialize the simple encoder
bool simple_encoder_init(void)
{
    // Initialize encoder data structure
    encoder_data.position = 0;
    encoder_data.velocity = 0;
    encoder_data.direction = 0;
    encoder_data.last_update_time = 0;
    
    // Initialize simple encoder structure
    simple_enc.current_state = 0;
    simple_enc.last_state = 0;
    simple_enc.position = 0;
    simple_enc.velocity = 0;
    simple_enc.last_pulse_time = 0;
    simple_enc.pulse_count = 0;
    simple_enc.velocity_window_start = 0;
    simple_enc.state_changed = false;
    
    // Enable PIO clock
    pmc_enable_periph_clk(ID_PIOA);
    
    // Configure encoder pins as inputs with pull-ups and interrupt capability
    // Encoder: PA0 (A), PA1 (B)
    pio_configure(PIOA, PIO_INPUT, PIO_PA0, PIO_PULLUP | PIO_IT_EDGE);
    pio_configure(PIOA, PIO_INPUT, PIO_PA1, PIO_PULLUP | PIO_IT_EDGE);
    
    // Set up interrupt handler for encoder pins
    pio_handler_set(PIOA, ID_PIOA, PIO_PA0 | PIO_PA1, PIO_IT_EDGE, encoder_interrupt_handler);
    pio_enable_interrupt(PIOA, PIO_PA0 | PIO_PA1);
    
    // Set interrupt priority (higher priority for better responsiveness)
    pio_handler_set_priority(PIOA, PIOA_IRQn, 5);
    
    // Enable interrupts globally
    NVIC_EnableIRQ(PIOA_IRQn);
    
    return true;
}

// Read encoder state (A and B pins)
uint8_t simple_encoder_read_state(void)
{
    uint8_t state = 0;
    if (pio_get(PIOA, PIO_INPUT, PIO_PA0)) state |= 0x01; // A bit
    if (pio_get(PIOA, PIO_INPUT, PIO_PA1)) state |= 0x02; // B bit
    return state;
}

// Update encoder data from interrupt-driven state
void simple_encoder_poll(void)
{
    // Get current time
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Update encoder data structure from interrupt-driven state
    encoder_data.position = (uint32_t)simple_enc.position;
    encoder_data.velocity = simple_enc.velocity;
    
    // Determine direction
    if (simple_enc.velocity > 0) {
        encoder_data.direction = 1; // Forward
    } else if (simple_enc.velocity < 0) {
        encoder_data.direction = 2; // Reverse
    } else {
        // Check if we've had recent movement
        if (current_time - simple_enc.last_pulse_time > 100) { // 100ms timeout
            encoder_data.direction = 0; // Stopped
        }
    }
    
    encoder_data.last_update_time = current_time;
}

// Main encoder task
void simple_encoder_task(void *arg)
{
    (void)arg; // Unused parameter
    
    // Initialize encoder
    if (!simple_encoder_init()) {
        // Encoder initialization failed
        while(1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // Wait a bit for encoder to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    uint32_t last_transmission_time = 0;
    
    for (;;) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Poll encoder
        simple_encoder_poll();
        
        // Send encoder data over CAN periodically
        if (current_time - last_transmission_time >= ENCODER_POLLING_RATE_MS) {
            
            // Prepare CAN message for encoder
            // Message format: [Direction(1)] [Velocity(3)] [Position(4)]
            // Direction: 0=stopped, 1=forward, 2=reverse
            // Velocity: signed 24-bit value (pulses per second)
            // Position: signed 32-bit value (pulse count)
            uint8_t enc_data[8];
            
            // Pack direction (1 byte)
            enc_data[0] = encoder_data.direction;
            
            // Pack velocity (3 bytes) - signed 24-bit value
            int32_t velocity = encoder_data.velocity;
            enc_data[1] = (uint8_t)(velocity & 0xFF);
            enc_data[2] = (uint8_t)((velocity >> 8) & 0xFF);
            enc_data[3] = (uint8_t)((velocity >> 16) & 0xFF);
            
            // Pack position (4 bytes) - signed 32-bit value
            int32_t position_value = (int32_t)encoder_data.position;
            enc_data[4] = (uint8_t)(position_value & 0xFF);
            enc_data[5] = (uint8_t)((position_value >> 8) & 0xFF);
            enc_data[6] = (uint8_t)((position_value >> 16) & 0xFF);
            enc_data[7] = (uint8_t)((position_value >> 24) & 0xFF);
            
            can_app_tx(CAN_ID_ENCODER_DIR_VEL, enc_data, 8);
            
            last_transmission_time = current_time;
        }
        
        // Small delay to prevent excessive CPU usage
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Get current encoder data
simple_encoder_data_t* simple_encoder_get_data(void)
{
    return &encoder_data;
}
