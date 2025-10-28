/*
 * encoder.c
 *
 * Created: 8/21/2025
 * Author: MKumar
 * 
 * Quadrature Decoder (QDE) implementation for Encoder1
 * - PA0/TIOA0: Encoder A input
 * - PA1/TIOB0: Encoder B input  
 * - PD17: Enable pin (active low)
 */

#include "encoder.h"
#include "asf.h"
#include "can_app.h"
#include "FreeRTOS.h"
#include "task.h"

// Global encoder data
static encoder_data_t g_encoder1_data = {0};
static int32_t g_last_position = 0;
static bool g_encoder_initialized = false;

// FreeRTOS task handle
static TaskHandle_t encoder1_task_handle = NULL;

// Forward declarations
static void encoder1_configure_pins(void);
static void encoder1_configure_tc(void);
static void encoder1_configure_qde(void);

bool encoder1_init(void)
{
    if (g_encoder_initialized) {
        return true; // Already initialized
    }
    
    // Configure pins
    encoder1_configure_pins();
    
    // Configure Timer Counter
    encoder1_configure_tc();
    
    // Configure Quadrature Decoder
    encoder1_configure_qde();
    
    // Initialize enable pin (PD17) as output, initially disabled (high)
    pio_configure(PIOD, PIO_OUTPUT_0, PIO_PD17, 0);
    pio_set(PIOD, PIO_PD17); // Set high (disabled)
    
    // Initialize encoder data
    g_encoder1_data.position = 0;
    g_encoder1_data.velocity = 0;
    g_encoder1_data.enabled = false;
    g_encoder1_data.valid = true;
    g_last_position = 0;
    
    g_encoder_initialized = true;
    
    return true;
}

static void encoder1_configure_pins(void)
{
    // Enable PIOA clock for TIOA0 and TIOB0 pins
    pmc_enable_periph_clk(ID_PIOA);
    
    // Configure PA0 as TIOA0 (peripheral A)
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA0, 0);
    
    // Configure PA1 as TIOB0 (peripheral A)  
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA1, 0);
    
    // Enable PIOA peripheral clock for Timer Counter 0
    pmc_enable_periph_clk(ID_TC0);
}

static void encoder1_configure_tc(void)
{
    // Configure Timer Counter 0 for quadrature decoder mode using direct register access
    // Disable TC0 first
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKDIS;
    
    // Configure TC0 Channel 0 for quadrature decoder mode
    // Set clock source to MCK/2, enable clock input, external trigger on rising edge
    TC0->TC_CHANNEL[0].TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 |  // Clock source: MCK/2
                                TC_CMR_CLKI |                   // Clock invert
                                TC_CMR_ETRGEDG_RISING |         // External trigger edge
                                TC_CMR_ABETRG |                 // TIOA is used as external trigger
                                TC_CMR_LDRA_RISING |            // Load RA on rising edge
                                TC_CMR_LDRB_FALLING;            // Load RB on falling edge
}

static void encoder1_configure_qde(void)
{
    // Configure Quadrature Decoder mode using direct register access
    // Enable QDE in TC_BMR register
    TC0->TC_BMR = TC_BMR_QDEN |           // Enable QDE
                  TC_BMR_POSEN |          // Enable position counting
                  TC_BMR_SPEEDEN |        // Enable speed counting
                  TC_BMR_FILTER |         // Enable input filter
                  TC_BMR_MAXFILT(0x3F);   // Set maximum filter value
    
    // Configure QDE interrupt enable (optional)
    TC0->TC_QIER = TC_QIER_IDX |          // Enable index interrupt
                   TC_QIER_DIRCHG |       // Enable direction change interrupt
                   TC_QIER_QERR;          // Enable quadrature error interrupt
}

bool encoder1_enable(bool enable)
{
    if (!g_encoder_initialized) {
        return false;
    }
    
    if (enable) {
        // Enable encoder (set PD17 low)
        pio_clear(PIOD, PIO_PD17);
        g_encoder1_data.enabled = true;
        
        // Reset position counter
        tc_write_rc(TC0, 0, 0);
        g_encoder1_data.position = 0;
        g_last_position = 0;
    } else {
        // Disable encoder (set PD17 high)
        pio_set(PIOD, PIO_PD17);
        g_encoder1_data.enabled = false;
    }
    
    return true;
}

int32_t encoder1_read_position(void)
{
    if (!g_encoder_initialized || !g_encoder1_data.enabled) {
        return 0;
    }
    
    // Read current position from Timer Counter using direct register access
    uint32_t tc_value = TC0->TC_CHANNEL[0].TC_CV;
    
    // Convert to signed 32-bit value
    int32_t position = (int32_t)tc_value;
    
    // Update global data
    g_encoder1_data.position = position;
    
    return position;
}

int32_t encoder1_read_velocity(void)
{
    if (!g_encoder_initialized || !g_encoder1_data.enabled) {
        return 0;
    }
    
    // Read current position
    int32_t current_position = encoder1_read_position();
    
    // Calculate velocity (difference from last reading)
    int32_t velocity = current_position - g_last_position;
    
    // Update for next calculation
    g_last_position = current_position;
    
    // Update global data
    g_encoder1_data.velocity = velocity;
    
    return velocity;
}

encoder_data_t encoder1_get_data(void)
{
    // Update position and velocity
    encoder1_read_position();
    encoder1_read_velocity();
    
    return g_encoder1_data;
}

void encoder1_reset_position(void)
{
    if (!g_encoder_initialized) {
        return;
    }
    
    // Reset Timer Counter
    tc_write_rc(TC0, 0, 0);
    
    // Reset global data
    g_encoder1_data.position = 0;
    g_last_position = 0;
    g_encoder1_data.velocity = 0;
}

bool encoder1_is_enabled(void)
{
    return g_encoder1_data.enabled;
}

// FreeRTOS task for encoder reading and CAN transmission
void encoder1_task(void *arg)
{
    (void)arg; // Unused parameter
    
    // Initialize encoder
    if (!encoder1_init()) {
        // Encoder initialization failed
        while(1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // Enable encoder
    encoder1_enable(true);
    
    // Task variables
    uint32_t task_interval = 0;
    const uint32_t SAMPLE_RATE_MS = 10; // 100 Hz sampling rate
    const uint32_t CAN_TX_INTERVAL_MS = 50; // 20 Hz CAN transmission rate
    
    for (;;) {
        // Read encoder data
        encoder_data_t enc_data = encoder1_get_data();
        
        // Send encoder data over CAN every CAN_TX_INTERVAL_MS
        if (task_interval % CAN_TX_INTERVAL_MS == 0) {
            if (enc_data.enabled && enc_data.valid) {
                // Prepare CAN data: 8 bytes
                // Byte 0-3: Position (32-bit signed, little-endian)
                // Byte 4-7: Velocity (32-bit signed, little-endian)
                uint8_t can_data[8];
                
                // Pack position (little-endian)
                can_data[0] = (uint8_t)(enc_data.position & 0xFF);
                can_data[1] = (uint8_t)((enc_data.position >> 8) & 0xFF);
                can_data[2] = (uint8_t)((enc_data.position >> 16) & 0xFF);
                can_data[3] = (uint8_t)((enc_data.position >> 24) & 0xFF);
                
                // Pack velocity (little-endian)
                can_data[4] = (uint8_t)(enc_data.velocity & 0xFF);
                can_data[5] = (uint8_t)((enc_data.velocity >> 8) & 0xFF);
                can_data[6] = (uint8_t)((enc_data.velocity >> 16) & 0xFF);
                can_data[7] = (uint8_t)((enc_data.velocity >> 24) & 0xFF);
                
                // Send over CAN
                can_app_tx(CAN_ID_ENCODER1, can_data, 8);
            }
        }
        
        // Increment task interval
        task_interval += SAMPLE_RATE_MS;
        if (task_interval >= 1000) {
            task_interval = 0; // Reset every second
        }
        
        // Wait for next sample
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_RATE_MS));
    }
}