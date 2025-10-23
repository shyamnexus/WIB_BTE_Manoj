#include "encoder.h"
#include "asf.h"
#include "pio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "can_app.h"

// Static encoder state variables
static encoder_state_t encoder1_state = {0};
static encoder_state_t encoder2_state = {0};

// Quadrature decoding lookup table for 2-bit state transitions
// Index: (last_state << 2) | current_state
// Value: position change (-1, 0, +1)
static const int8_t quadrature_table[16] = {
    0,  // 00->00: no change
    -1, // 00->01: reverse
    +1, // 00->10: forward
    0,  // 00->11: invalid transition
    +1, // 01->00: forward
    0,  // 01->01: no change
    0,  // 01->10: invalid transition
    -1, // 01->11: reverse
    -1, // 10->00: reverse
    0,  // 10->01: invalid transition
    0,  // 10->10: no change
    +1, // 10->11: forward
    0,  // 11->00: invalid transition
    +1, // 11->01: forward
    -1, // 11->10: reverse
    0   // 11->11: no change
};

bool encoder_init(void)
{
    // Enable PIO clocks for all encoder pins
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);
    
    // Configure encoder pins as inputs with pull-up resistors
    // Encoder1 pins
    pio_set_input(PIOA, PIO_PA5, PIO_PULLUP);   // ENC1_A
    pio_set_input(PIOA, PIO_PA1, PIO_PULLUP);   // ENC1_B
    pio_set_output(PIOD, PIO_PD17, LOW, DISABLE, DISABLE); // ENC1_ENABLE (output, initially low)
    
    // Encoder2 pins
    pio_set_input(PIOA, PIO_PA15, PIO_PULLUP);  // ENC2_A
    pio_set_input(PIOA, PIO_PA16, PIO_PULLUP);  // ENC2_B
    pio_set_output(PIOD, PIO_PD27, LOW, DISABLE, DISABLE); // ENC2_ENABLE (output, initially low)
    
    // Initialize encoder states
    encoder1_state.last_state = 0;
    encoder1_state.position = 0;
    encoder1_state.velocity = 0;
    encoder1_state.direction = true;
    encoder1_state.sample_count = 0;
    
    encoder2_state.last_state = 0;
    encoder2_state.position = 0;
    encoder2_state.velocity = 0;
    encoder2_state.direction = true;
    encoder2_state.sample_count = 0;
    
    // Read initial state
    uint8_t enc1_a = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA5) ? 1 : 0;
    uint8_t enc1_b = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA1) ? 1 : 0;
    uint8_t enc2_a = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA15) ? 1 : 0;
    uint8_t enc2_b = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA16) ? 1 : 0;
    
    encoder1_state.last_state = (enc1_a << 1) | enc1_b;
    encoder2_state.last_state = (enc2_a << 1) | enc2_b;
    
    return true;
}

void encoder_enable_encoder1(bool enable)
{
    if (enable) {
        pio_set(PIOD, PIO_PD17);  // Set enable pin high
    } else {
        pio_clear(PIOD, PIO_PD17); // Set enable pin low
    }
}

void encoder_enable_encoder2(bool enable)
{
    if (enable) {
        pio_set(PIOD, PIO_PD27);  // Set enable pin high
    } else {
        pio_clear(PIOD, PIO_PD27); // Set enable pin low
    }
}

bool encoder_is_encoder1_enabled(void)
{
    return pio_get(PIOD, PIO_TYPE_PIO_INPUT, PIO_PD17) ? true : false;
}

bool encoder_is_encoder2_enabled(void)
{
    return pio_get(PIOD, PIO_TYPE_PIO_INPUT, PIO_PD27) ? true : false;
}

void encoder_read_encoder1(encoder_data_t *data)
{
    if (data == NULL) return;
    
    // Read current state of encoder pins
    uint8_t enc_a = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA5) ? 1 : 0;
    uint8_t enc_b = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA1) ? 1 : 0;
    uint8_t current_state = (enc_a << 1) | enc_b;
    
    // Calculate position change using quadrature table
    uint8_t state_index = (encoder1_state.last_state << 2) | current_state;
    int8_t position_change = quadrature_table[state_index];
    
    // Update position
    encoder1_state.position += position_change;
    
    // Update velocity (position change per sample)
    encoder1_state.velocity = position_change;
    
    // Update direction based on velocity
    if (position_change > 0) {
        encoder1_state.direction = true;  // Forward
    } else if (position_change < 0) {
        encoder1_state.direction = false; // Reverse
    }
    // If position_change == 0, keep previous direction
    
    // Update sample count
    encoder1_state.sample_count++;
    
    // Store current state for next iteration
    encoder1_state.last_state = current_state;
    
    // Fill output data structure
    data->position = encoder1_state.position;
    data->velocity = encoder1_state.velocity;
    data->direction = encoder1_state.direction;
    data->enabled = encoder_is_encoder1_enabled();
    data->sample_count = encoder1_state.sample_count;
}

void encoder_read_encoder2(encoder_data_t *data)
{
    if (data == NULL) return;
    
    // Read current state of encoder pins
    uint8_t enc_a = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA15) ? 1 : 0;
    uint8_t enc_b = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA16) ? 1 : 0;
    uint8_t current_state = (enc_a << 1) | enc_b;
    
    // Calculate position change using quadrature table
    uint8_t state_index = (encoder2_state.last_state << 2) | current_state;
    int8_t position_change = quadrature_table[state_index];
    
    // Update position
    encoder2_state.position += position_change;
    
    // Update velocity (position change per sample)
    encoder2_state.velocity = position_change;
    
    // Update direction based on velocity
    if (position_change > 0) {
        encoder2_state.direction = true;  // Forward
    } else if (position_change < 0) {
        encoder2_state.direction = false; // Reverse
    }
    // If position_change == 0, keep previous direction
    
    // Update sample count
    encoder2_state.sample_count++;
    
    // Store current state for next iteration
    encoder2_state.last_state = current_state;
    
    // Fill output data structure
    data->position = encoder2_state.position;
    data->velocity = encoder2_state.velocity;
    data->direction = encoder2_state.direction;
    data->enabled = encoder_is_encoder2_enabled();
    data->sample_count = encoder2_state.sample_count;
}

void encoder_task(void *arg)
{
    (void)arg; // Unused parameter
    
    encoder_data_t enc1_data, enc2_data;
    uint8_t can_data[8];
    uint32_t task_count = 0;
    
    // Enable both encoders
    encoder_enable_encoder1(true);
    encoder_enable_encoder2(true);
    
    for (;;) {
        task_count++;
        // Read encoder 1
        encoder_read_encoder1(&enc1_data);
        
        // Prepare CAN data for encoder 1
        // Byte 0-3: Position (32-bit signed integer, little-endian)
        can_data[0] = (uint8_t)(enc1_data.position & 0xFF);
        can_data[1] = (uint8_t)((enc1_data.position >> 8) & 0xFF);
        can_data[2] = (uint8_t)((enc1_data.position >> 16) & 0xFF);
        can_data[3] = (uint8_t)((enc1_data.position >> 24) & 0xFF);
        
        // Byte 4-5: Velocity (16-bit signed integer, little-endian)
        can_data[4] = (uint8_t)(enc1_data.velocity & 0xFF);
        can_data[5] = (uint8_t)((enc1_data.velocity >> 8) & 0xFF);
        
        // Byte 6: Direction and enable status
        can_data[6] = (enc1_data.direction ? 0x01 : 0x00) | (enc1_data.enabled ? 0x02 : 0x00);
        
        // Byte 7: Sample count (8-bit, wraps at 255)
        can_data[7] = (uint8_t)(enc1_data.sample_count & 0xFF);
        
        // Send encoder 1 data over CAN
        can_app_tx(CAN_ID_ENCODER1, can_data, 8);
        
        // Read encoder 2
        encoder_read_encoder2(&enc2_data);
        
        // Prepare CAN data for encoder 2
        // Byte 0-3: Position (32-bit signed integer, little-endian)
        can_data[0] = (uint8_t)(enc2_data.position & 0xFF);
        can_data[1] = (uint8_t)((enc2_data.position >> 8) & 0xFF);
        can_data[2] = (uint8_t)((enc2_data.position >> 16) & 0xFF);
        can_data[3] = (uint8_t)((enc2_data.position >> 24) & 0xFF);
        
        // Byte 4-5: Velocity (16-bit signed integer, little-endian)
        can_data[4] = (uint8_t)(enc2_data.velocity & 0xFF);
        can_data[5] = (uint8_t)((enc2_data.velocity >> 8) & 0xFF);
        
        // Byte 6: Direction and enable status
        can_data[6] = (enc2_data.direction ? 0x01 : 0x00) | (enc2_data.enabled ? 0x02 : 0x00);
        
        // Byte 7: Sample count (8-bit, wraps at 255)
        can_data[7] = (uint8_t)(enc2_data.sample_count & 0xFF);
        
        // Send encoder 2 data over CAN
        can_app_tx(CAN_ID_ENCODER2, can_data, 8);
        
        // Debug: Send status every 1000 iterations (about every second at 1ms delay)
        if (task_count % 1000 == 0) {
            // Send debug status message
            uint8_t debug_data[4] = {
                (uint8_t)(task_count & 0xFF),
                (uint8_t)((task_count >> 8) & 0xFF),
                (uint8_t)((task_count >> 16) & 0xFF),
                (uint8_t)((task_count >> 24) & 0xFF)
            };
            can_app_tx(CAN_ID_STATUS, debug_data, 4);
        }
        
        // Task delay - adjust frequency as needed
        // 1ms delay = 1000 Hz sampling rate
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}