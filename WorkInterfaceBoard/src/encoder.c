#include "encoder.h"
#include "asf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "can_app.h"

// Simple encoder data structure
typedef struct {
    int32_t position;          // Current position (pulse count)
    int32_t velocity;          // Current velocity (pulses per second)
    uint8_t direction;         // 0=stopped, 1=forward, 2=reverse
    uint32_t last_update_time; // Last time position was updated (ms)
    uint32_t last_position;    // Previous position for velocity calculation
} simple_encoder_t;

static simple_encoder_t encoder_data = {0};

// QDE configuration
#define QDE_CHANNEL 0  // Use TC0 for QDE
#define VELOCITY_CALC_PERIOD_MS 50  // Calculate velocity every 50ms

// Initialize QDE (Quadrature Decoder)
bool encoder_qde_init(void)
{
    // Enable peripheral clocks
    pmc_enable_periph_clk(ID_TC0);
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);
    
    // Configure PA0 (TIOA0) and PA1 (TIOB0) as peripheral pins
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA0, 0);  // TIOA0
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA1, 0);  // TIOB0
    
    // Configure enable pin as output and enable encoder
    pio_configure(PIOD, PIO_OUTPUT_0, ENC_ENABLE_PIN, PIO_DEFAULT);
    pio_clear(PIOD, ENC_ENABLE_PIN);  // Enable encoder (active low)
    
    // Configure TC0 for quadrature decoding
    tc_init(TC0, QDE_CHANNEL, TC_CMR_TCCLKS_TIMER_CLOCK1 |  // MCK/2
                     TC_CMR_ABETRG |                          // External trigger on TIOA
                     TC_CMR_ETRGEDG_RISING |                  // Rising edge trigger
                     TC_CMR_LDRA_RISING |                     // Load on rising edge of TIOA
                     TC_CMR_LDRB_FALLING |                    // Load on falling edge of TIOA
                     TC_CMR_CPCTRG |                          // RC compare trigger
                     TC_CMR_WAVE |                            // Waveform mode
                     TC_CMR_WAVSEL_UP_RC |                    // Up counter with RC compare
                     TC_CMR_ACPA_CLEAR |                      // Clear on TIOA rising
                     TC_CMR_ACPC_SET |                        // Set on TIOA falling
                     TC_CMR_BCPB_CLEAR |                      // Clear on TIOB rising
                     TC_CMR_BCPC_SET);                        // Set on TIOB falling
    
    // Set RC value for maximum count range
    tc_write_rc(TC0, QDE_CHANNEL, 0xFFFFFFFF);
    
    // Enable TC0
    tc_start(TC0, QDE_CHANNEL);
    
    return true;
}

// Read encoder position from QDE
int32_t encoder_read_position(void)
{
    return (int32_t)tc_read_cv(TC0, QDE_CHANNEL);
}

// Calculate velocity based on position change
int32_t encoder_calculate_velocity(void)
{
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    int32_t current_position = encoder_read_position();
    
    if (current_time - encoder_data.last_update_time >= VELOCITY_CALC_PERIOD_MS) {
        int32_t position_delta = current_position - encoder_data.last_position;
        uint32_t time_delta = current_time - encoder_data.last_update_time;
        
        if (time_delta > 0) {
            encoder_data.velocity = (position_delta * 1000) / time_delta;
        }
        
        encoder_data.last_position = current_position;
        encoder_data.last_update_time = current_time;
    }
    
    return encoder_data.velocity;
}

// Update encoder data
void encoder_update_data(void)
{
    encoder_data.position = encoder_read_position();
    encoder_data.velocity = encoder_calculate_velocity();
    
    // Determine direction
    if (encoder_data.velocity > 0) {
        encoder_data.direction = 1;  // Forward
    } else if (encoder_data.velocity < 0) {
        encoder_data.direction = 2;  // Reverse
    } else {
        encoder_data.direction = 0;  // Stopped
    }
}

// Send encoder data via CAN
void encoder_send_can_data(void)
{
    uint8_t can_data[8];
    
    // Pack direction (1 byte)
    can_data[0] = encoder_data.direction;
    
    // Pack velocity (3 bytes) - signed 24-bit value
    int32_t velocity = encoder_data.velocity;
    can_data[1] = (uint8_t)(velocity & 0xFF);
    can_data[2] = (uint8_t)((velocity >> 8) & 0xFF);
    can_data[3] = (uint8_t)((velocity >> 16) & 0xFF);
    
    // Pack position (4 bytes) - signed 32-bit value
    int32_t position = encoder_data.position;
    can_data[4] = (uint8_t)(position & 0xFF);
    can_data[5] = (uint8_t)((position >> 8) & 0xFF);
    can_data[6] = (uint8_t)((position >> 16) & 0xFF);
    can_data[7] = (uint8_t)((position >> 24) & 0xFF);
    
    can_app_tx(CAN_ID_ENCODER_DIR_VEL, can_data, 8);
}

// Main encoder task
void encoder_task(void *arg)
{
    (void)arg;
    
    // Initialize QDE
    if (!encoder_qde_init()) {
        while(1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // Initialize encoder data
    encoder_data.position = 0;
    encoder_data.velocity = 0;
    encoder_data.direction = 0;
    encoder_data.last_update_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    encoder_data.last_position = encoder_read_position();
    
    // Wait for encoder to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    uint32_t last_can_time = 0;
    const uint32_t CAN_PERIOD_MS = 20;  // Send CAN data every 20ms (50Hz)
    
    for (;;) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Update encoder data
        encoder_update_data();
        
        // Send CAN data periodically
        if (current_time - last_can_time >= CAN_PERIOD_MS) {
            encoder_send_can_data();
            last_can_time = current_time;
        }
        
        // Small delay to prevent excessive CPU usage
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Legacy functions for compatibility
bool encoder_init(void) {
    return encoder_qde_init();
}

void encoder_poll(encoder_data_t* enc_data) {
    (void)enc_data;  // Not used in simple implementation
}

// Dummy functions for compatibility
void encoder1_interrupt_handler(uint32_t id, uint32_t mask) { (void)id; (void)mask; }
void encoder2_interrupt_handler(uint32_t id, uint32_t mask) { (void)id; (void)mask; }
bool encoder_tc_init(void) { return true; }
bool encoder_tc_channel_init(uint32_t channel) { (void)channel; return true; }
uint32_t encoder_tc_get_position(uint32_t channel) { (void)channel; return 0; }
void encoder_tc_reset_position(uint32_t channel) { (void)channel; }
uint8_t encoder_tc_get_direction(uint32_t channel) { (void)channel; return 0; }
int32_t calculate_velocity(encoder_data_t* enc_data, uint32_t current_time) { (void)enc_data; (void)current_time; return 0; }
void apply_velocity_smoothing(encoder_data_t* enc_data) { (void)enc_data; }
bool is_direction_change_allowed(encoder_data_t* enc_data, uint32_t current_time, uint8_t new_direction) { (void)enc_data; (void)current_time; (void)new_direction; return true; }