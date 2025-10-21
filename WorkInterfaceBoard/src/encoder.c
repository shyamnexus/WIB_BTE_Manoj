#include "encoder.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"


#ifndef TickType_t
typedef portTickType TickType_t; // Backward-compatible alias if TickType_t isn't defined
#endif
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_PERIOD_MS)) // Convert milliseconds to OS ticks
#endif

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS portTICK_RATE_MS // Legacy macro mapping
#endif

// Static variables for encoder state
static volatile int32_t enc1_position = 0;
static volatile int32_t enc2_position = 0;
static volatile int32_t enc1_last_position = 0;
static volatile int32_t enc2_last_position = 0;
static volatile uint32_t enc1_last_timestamp = 0;
static volatile uint32_t enc2_last_timestamp = 0;
static volatile bool encoder_initialized = false;

// Encoder state tracking
static volatile uint8_t enc1_state = 0;
static volatile uint8_t enc2_state = 0;

// Encoder interrupt handlers
void PIOA_Handler_WIB(void)
{
    uint32_t status = pio_get_interrupt_status(PIOA);
    
    // Handle ENC1 interrupts
    if (status & PIO_PA5) { // ENC1_A
        uint8_t a_state = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA5);
        uint8_t b_state = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA1);
        uint8_t new_state = (a_state << 1) | b_state;
        
        // Quadrature decoding
        if (enc1_state == 0 && new_state == 1) enc1_position++;
        else if (enc1_state == 1 && new_state == 3) enc1_position++;
        else if (enc1_state == 3 && new_state == 2) enc1_position++;
        else if (enc1_state == 2 && new_state == 0) enc1_position++;
        else if (enc1_state == 0 && new_state == 2) enc1_position--;
        else if (enc1_state == 2 && new_state == 3) enc1_position--;
        else if (enc1_state == 3 && new_state == 1) enc1_position--;
        else if (enc1_state == 1 && new_state == 0) enc1_position--;
        
        enc1_state = new_state;
    }
    
    if (status & PIO_PA1) { // ENC1_B
        uint8_t a_state = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA5);
        uint8_t b_state = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA1);
        uint8_t new_state = (a_state << 1) | b_state;
        
        // Quadrature decoding
        if (enc1_state == 0 && new_state == 1) enc1_position++;
        else if (enc1_state == 1 && new_state == 3) enc1_position++;
        else if (enc1_state == 3 && new_state == 2) enc1_position++;
        else if (enc1_state == 2 && new_state == 0) enc1_position++;
        else if (enc1_state == 0 && new_state == 2) enc1_position--;
        else if (enc1_state == 2 && new_state == 3) enc1_position--;
        else if (enc1_state == 3 && new_state == 1) enc1_position--;
        else if (enc1_state == 1 && new_state == 0) enc1_position--;
        
        enc1_state = new_state;
    }
    
    // Handle ENC2 interrupts
    if (status & PIO_PA15) { // ENC2_A
        uint8_t a_state = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA15);
        uint8_t b_state = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA16);
        uint8_t new_state = (a_state << 1) | b_state;
        
        // Quadrature decoding
        if (enc2_state == 0 && new_state == 1) enc2_position++;
        else if (enc2_state == 1 && new_state == 3) enc2_position++;
        else if (enc2_state == 3 && new_state == 2) enc2_position++;
        else if (enc2_state == 2 && new_state == 0) enc2_position++;
        else if (enc2_state == 0 && new_state == 2) enc2_position--;
        else if (enc2_state == 2 && new_state == 3) enc2_position--;
        else if (enc2_state == 3 && new_state == 1) enc2_position--;
        else if (enc2_state == 1 && new_state == 0) enc2_position--;
        
        enc2_state = new_state;
    }
    
    if (status & PIO_PA16) { // ENC2_B
        uint8_t a_state = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA15);
        uint8_t b_state = pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA16);
        uint8_t new_state = (a_state << 1) | b_state;
        
        // Quadrature decoding
        if (enc2_state == 0 && new_state == 1) enc2_position++;
        else if (enc2_state == 1 && new_state == 3) enc2_position++;
        else if (enc2_state == 3 && new_state == 2) enc2_position++;
        else if (enc2_state == 2 && new_state == 0) enc2_position++;
        else if (enc2_state == 0 && new_state == 2) enc2_position--;
        else if (enc2_state == 2 && new_state == 3) enc2_position--;
        else if (enc2_state == 3 && new_state == 1) enc2_position--;
        else if (enc2_state == 1 && new_state == 0) enc2_position--;
        
        enc2_state = new_state;
    }
}

// Initialize encoder hardware
bool encoder_init(void)
{
    // Enable peripheral clocks
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);
    
    // Configure encoder pins as inputs with pull-ups
    // ENC1: PA5 (A) and PA1 (B)
    pio_configure(PIOA, PIO_INPUT, PIO_PA5 | PIO_PA1, PIO_PULLUP | PIO_DEBOUNCE);
    
    // ENC2: PA15 (A) and PA16 (B)
    pio_configure(PIOA, PIO_INPUT, PIO_PA15 | PIO_PA16, PIO_PULLUP | PIO_DEBOUNCE);
    
    // Configure encoder enable pins
    // ENC1_ENABLE: PD17
    pio_configure(PIOD, PIO_OUTPUT_0, PIO_PD17, 0);
    pio_set(PIOD, PIO_PD17); // Enable encoder 1
    
    // ENC2_ENABLE: PD27
    pio_configure(PIOD, PIO_OUTPUT_0, PIO_PD27, 0);
    pio_set(PIOD, PIO_PD27); // Enable encoder 2
    
    // Configure interrupts for encoder pins
    pio_set_input(PIOA, PIO_PA5 | PIO_PA1 | PIO_PA15 | PIO_PA16, PIO_PULLUP | PIO_DEBOUNCE);
    pio_handler_set(PIOA, ID_PIOA, PIO_PA5 | PIO_PA1 | PIO_PA15 | PIO_PA16, PIO_IT_EDGE, PIOA_Handler_WIB);
    pio_enable_interrupt(PIOA, PIO_PA5 | PIO_PA1 | PIO_PA15 | PIO_PA16);
    NVIC_EnableIRQ(PIOA_IRQn);
    
    // Initialize position tracking variables
    enc1_position = 0;
    enc2_position = 0;
    enc1_last_position = 0;
    enc2_last_position = 0;
    enc1_last_timestamp = 0;
    enc2_last_timestamp = 0;
    enc1_state = 0;
    enc2_state = 0;
    
    encoder_initialized = true;
    
    return true;
}

// Read encoder data
bool encoder_read_data(encoder_data_t* enc1_data, encoder_data_t* enc2_data)
{
    if (!encoder_initialized || !enc1_data || !enc2_data) {
        return false;
    }
    
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Read current positions from interrupt-updated variables
    int32_t enc1_current = enc1_position;
    int32_t enc2_current = enc2_position;
    
    // Calculate deltas
    int32_t enc1_delta = enc1_current - enc1_last_position;
    int32_t enc2_delta = enc2_current - enc2_last_position;
    
    // Update positions
    enc1_data->position = enc1_current;
    enc2_data->position = enc2_current;
    
    // Calculate velocity (counts per second)
    uint32_t time_delta = current_time - enc1_last_timestamp;
    if (time_delta > 0) {
        enc1_data->velocity = (enc1_delta * 1000) / time_delta;
        enc2_data->velocity = (enc2_delta * 1000) / time_delta;
    } else {
        enc1_data->velocity = 0;
        enc2_data->velocity = 0;
    }
    
    // Update timestamp
    enc1_data->timestamp = current_time;
    enc2_data->timestamp = current_time;
    
    // Update last values
    enc1_last_position = enc1_current;
    enc2_last_position = enc2_current;
    enc1_last_timestamp = current_time;
    enc2_last_timestamp = current_time;
    
    // Set validity flags
    enc1_data->valid = true;
    enc2_data->valid = true;
    
    return true;
}

// Reset encoder counters
void encoder_reset_counters(void)
{
    if (!encoder_initialized) {
        return;
    }
    
    // Reset position counters
    enc1_position = 0;
    enc2_position = 0;
    enc1_last_position = 0;
    enc2_last_position = 0;
    enc1_last_timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    enc2_last_timestamp = enc1_last_timestamp;
}

// Get current position for specific encoder
int32_t encoder_get_position(uint8_t encoder_num)
{
    if (!encoder_initialized) {
        return 0;
    }
    
    if (encoder_num == 1) {
        return enc1_position;
    } else if (encoder_num == 2) {
        return enc2_position;
    }
    
    return 0;
}

// Get current velocity for specific encoder
int32_t encoder_get_velocity(uint8_t encoder_num)
{
    // This is a simplified version - for accurate velocity calculation,
    // use the encoder_read_data function which tracks velocity over time
    return 0;
}