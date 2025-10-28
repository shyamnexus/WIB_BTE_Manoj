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


#ifndef TickType_t
typedef portTickType TickType_t; // Backward-compatible alias if TickType_t isn't defined
#endif
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_PERIOD_MS)) // Convert milliseconds to OS ticks
#endif

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS portTICK_RATE_MS // Legacy macro mapping
#endif

// Global encoder data
static encoder_data_t g_encoder1_data = {0};
static int32_t g_last_position = 0;
static bool g_encoder_initialized = false;

// FreeRTOS task handle
//static TaskHandle_t encoder1_task_handle = NULL;

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
    
    // Debug: Check enable pin configuration
    volatile bool debug_enable_pin_configured = (PIOD->PIO_OSR & PIO_PD17) != 0;
    volatile bool debug_enable_pin_state = (PIOD->PIO_PDSR & PIO_PD17) != 0;
    (void)debug_enable_pin_configured; (void)debug_enable_pin_state;
    
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
    
    // Enable PIOD clock for enable pin
    pmc_enable_periph_clk(ID_PIOD);
}

static void encoder1_configure_tc(void)
{
    // Configure Timer Counter 0 for quadrature decoder mode
    // Use ASF functions for proper configuration
    
    // Initialize TC0 channel 0
    tc_init(TC0, 0, TC_CMR_TCCLKS_TIMER_CLOCK1 |  // Clock source: MCK/2
                     TC_CMR_CLKI |                   // Clock invert
                     TC_CMR_ETRGEDG_RISING |         // External trigger edge
                     TC_CMR_ABETRG |                 // TIOA is used as external trigger
                     TC_CMR_LDRA_RISING |            // Load RA on rising edge
                     TC_CMR_LDRB_FALLING);           // Load RB on falling edge
    
    // Start the timer counter
    tc_start(TC0, 0);
    
    // Debug: Store configuration values
    volatile uint32_t debug_tc_cmr = TC0->TC_CHANNEL[0].TC_CMR;
    volatile uint32_t debug_tc_sr = TC0->TC_CHANNEL[0].TC_SR;
    (void)debug_tc_cmr; (void)debug_tc_sr;
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
    
    // Reset the position counter to zero
    tc_write_rc(TC0, 0, 0);
    
    // Debug: Store QDE configuration values
    volatile uint32_t debug_tc_bmr = TC0->TC_BMR;
    volatile uint32_t debug_tc_qier = TC0->TC_QIER;
    volatile uint32_t debug_tc_sr_after_reset = TC0->TC_CHANNEL[0].TC_SR;
    (void)debug_tc_bmr; (void)debug_tc_qier; (void)debug_tc_sr_after_reset;
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
        
        // Debug: Check enable pin state after enabling
        volatile bool debug_enable_pin_state_after = (PIOD->PIO_PDSR & PIO_PD17) != 0;
        (void)debug_enable_pin_state_after;
    } else {
        // Disable encoder (set PD17 high)
        pio_set(PIOD, PIO_PD17);
        g_encoder1_data.enabled = false;
        
        // Debug: Check enable pin state after disabling
        volatile bool debug_enable_pin_state_after = (PIOD->PIO_PDSR & PIO_PD17) != 0;
        (void)debug_enable_pin_state_after;
    }
    
    return true;
}

int32_t encoder1_read_position(void)
{
    if (!g_encoder_initialized || !g_encoder1_data.enabled) {
        return 0;
    }
    
    // Read current position from Timer Counter using ASF function
    uint32_t tc_value = tc_read_cv(TC0, 0);
    
    // Debug: Store register values for analysis
    volatile uint32_t debug_tc_cv = tc_value;
    volatile uint32_t debug_tc_sr = TC0->TC_CHANNEL[0].TC_SR;
    volatile uint32_t debug_tc_cmr = TC0->TC_CHANNEL[0].TC_CMR;
    volatile uint32_t debug_tc_bmr = TC0->TC_BMR;
    volatile uint32_t debug_tc_qier = TC0->TC_QIER;
    
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

// Debug function to check encoder status
void encoder1_debug_status(void)
{
    volatile uint32_t debug_tc_cv = tc_read_cv(TC0, 0);
    volatile uint32_t debug_tc_sr = TC0->TC_CHANNEL[0].TC_SR;
    volatile uint32_t debug_tc_cmr = TC0->TC_CHANNEL[0].TC_CMR;
    volatile uint32_t debug_tc_bmr = TC0->TC_BMR;
    volatile uint32_t debug_tc_qier = TC0->TC_QIER;
    volatile uint32_t debug_pioa_pdsr = PIOA->PIO_PDSR;
    volatile uint32_t debug_piod_pdsr = PIOD->PIO_PDSR;
    volatile bool debug_encoder_enabled = g_encoder1_data.enabled;
    volatile bool debug_encoder_initialized = g_encoder_initialized;
    volatile int32_t debug_position = g_encoder1_data.position;
    
    // Check if TIOA0 and TIOB0 pins are configured correctly
    volatile bool debug_tioa0_configured = (PIOA->PIO_ABSR & PIO_PA0) == 0; // Should be 0 for peripheral A
    volatile bool debug_tiob0_configured = (PIOA->PIO_ABSR & PIO_PA1) == 0; // Should be 0 for peripheral A
    
    // Check if enable pin is configured correctly
    volatile bool debug_enable_pin_configured = (PIOD->PIO_OSR & PIO_PD17) != 0; // Should be 1 for output
    
    // Check QDE status
    volatile uint32_t debug_qde_status = TC0->TC_QISR;
    volatile bool debug_qde_enabled = (TC0->TC_BMR & TC_BMR_QDEN) != 0;
    volatile bool debug_position_enabled = (TC0->TC_BMR & TC_BMR_POSEN) != 0;
    
    (void)debug_tc_cv; (void)debug_tc_sr; (void)debug_tc_cmr; (void)debug_tc_bmr; (void)debug_tc_qier;
    (void)debug_pioa_pdsr; (void)debug_piod_pdsr; (void)debug_encoder_enabled; (void)debug_encoder_initialized;
    (void)debug_position; (void)debug_tioa0_configured; (void)debug_tiob0_configured; (void)debug_enable_pin_configured;
    (void)debug_qde_status; (void)debug_qde_enabled; (void)debug_position_enabled;
}

// Test function to verify encoder is working
bool encoder1_test(void)
{
    if (!g_encoder_initialized) {
        return false;
    }
    
    // Enable encoder
    encoder1_enable(true);
    
    // Read position multiple times to see if it changes
    int32_t pos1 = encoder1_read_position();
    vTaskDelay(pdMS_TO_TICKS(10));
    int32_t pos2 = encoder1_read_position();
    vTaskDelay(pdMS_TO_TICKS(10));
    int32_t pos3 = encoder1_read_position();
    
    // Check if position is changing (indicating encoder is working)
    volatile bool test_passed = (pos1 != pos2) || (pos2 != pos3) || (pos1 != pos3);
    volatile int32_t test_pos1 = pos1;
    volatile int32_t test_pos2 = pos2;
    volatile int32_t test_pos3 = pos3;
    
    (void)test_passed; (void)test_pos1; (void)test_pos2; (void)test_pos3;
    
    return test_passed;
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
    const uint32_t DEBUG_INTERVAL_MS = 1000; // 1 Hz debug rate
    
    for (;;) {
        // Read encoder data
        encoder_data_t enc_data = encoder1_get_data();
        
        // Call debug function periodically
        if (task_interval % DEBUG_INTERVAL_MS == 0) {
            encoder1_debug_status();
            // Also run encoder test
            encoder1_test();
        }
        
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