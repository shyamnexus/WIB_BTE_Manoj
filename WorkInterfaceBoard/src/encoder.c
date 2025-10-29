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
    // Configure Timer Counter 0 for quadrature decoder mode using direct register access
    // Disable TC0 first
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKDIS;
    
    // Wait for disable to take effect
    while (TC0->TC_CHANNEL[0].TC_SR & TC_SR_CLKSTA);
    
    // Configure TC0 Channel 0 for quadrature decoder mode
    // For QDE mode, we need to configure it as a simple counter mode
    // Set clock source to MCK/2, enable waveform mode for QDE
    TC0->TC_CHANNEL[0].TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 |  // Clock source: MCK/2
                                 TC_CMR_WAVE |                   // Enable waveform mode
                                 TC_CMR_WAVSEL_UP;               // UP mode for QDE
    
    // Enable the timer counter
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN;
    
    // Wait for enable to take effect
    while (!(TC0->TC_CHANNEL[0].TC_SR & TC_SR_CLKSTA));
    
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
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG;
    
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
        
        // Reset position counter using direct register access
        TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG;
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
    
    // Read current position from Timer Counter using direct register access
    // For quadrature decoder mode, we need to read the position from the QDE register
    // The QDE position is available in TC_CV when QDE is enabled
    uint32_t tc_value = TC0->TC_CHANNEL[0].TC_CV;
    
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
    
    // Reset Timer Counter using direct register access
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG;
    
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
    volatile uint32_t debug_tc_cv = TC0->TC_CHANNEL[0].TC_CV;
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
   // volatile bool debug_tioa0_configured = (PIOA->PIO_ABSR & PIO_PA0) == 0; // Should be 0 for peripheral A
 //   volatile bool debug_tiob0_configured = (PIOA->PIO_ABSR & PIO_PA1) == 0; // Should be 0 for peripheral A
    
    // Check if enable pin is configured correctly
    volatile bool debug_enable_pin_configured = (PIOD->PIO_OSR & PIO_PD17) != 0; // Should be 1 for output
    
    (void)debug_tc_cv; (void)debug_tc_sr; (void)debug_tc_cmr; (void)debug_tc_bmr; (void)debug_tc_qier;
    (void)debug_pioa_pdsr; (void)debug_piod_pdsr; (void)debug_encoder_enabled; (void)debug_encoder_initialized;
    (void)debug_position; (void)
	//debug_tioa0_configured; (void)debug_tiob0_configured; 
	(void)debug_enable_pin_configured;
}

// Test function to manually check encoder operation
void encoder1_test_operation(void)
{
    // Initialize encoder if not already done
    if (!g_encoder_initialized) {
        encoder1_init();
    }
    
    // Enable encoder
    encoder1_enable(true);
    
    // Read initial position
    int32_t initial_pos = encoder1_read_position();
    
    // Wait a bit and read again
    for (volatile int i = 0; i < 1000000; i++); // Simple delay
    
    int32_t current_pos = encoder1_read_position();
    
    // Store results in debug variables
    volatile int32_t debug_initial_pos = initial_pos;
    volatile int32_t debug_current_pos = current_pos;
    volatile int32_t debug_position_change = current_pos - initial_pos;
    volatile uint32_t debug_tc_cv = TC0->TC_CHANNEL[0].TC_CV;
    volatile uint32_t debug_tc_sr = TC0->TC_CHANNEL[0].TC_SR;
    volatile uint32_t debug_tc_cmr = TC0->TC_CHANNEL[0].TC_CMR;
    volatile uint32_t debug_tc_bmr = TC0->TC_BMR;
    
    (void)debug_initial_pos; (void)debug_current_pos; (void)debug_position_change;
    (void)debug_tc_cv; (void)debug_tc_sr; (void)debug_tc_cmr; (void)debug_tc_bmr;
}

// Simple encoder test without CAN - just read position and store in debug variables
void encoder1_simple_test(void)
{
    // Initialize encoder if not already done
    if (!g_encoder_initialized) {
        encoder1_init();
    }
    
    // Enable encoder
    encoder1_enable(true);
    
    // Read position multiple times
    for (int i = 0; i < 10; i++) {
        int32_t pos = encoder1_read_position();
        volatile int32_t debug_pos_reading = pos;
        (void)debug_pos_reading;
        
        // Small delay between readings
        for (volatile int j = 0; j < 100000; j++);
    }
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
    
    // Run simple test once at startup
    encoder1_simple_test();
    
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
        }
        
        // Send encoder data over CAN every CAN_TX_INTERVAL_MS
        if (task_interval % CAN_TX_INTERVAL_MS == 0) {
            // Always send encoder data for debugging, even if not enabled
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
            
            // Debug: Store encoder status for analysis
            volatile bool debug_encoder_enabled = enc_data.enabled;
            volatile bool debug_encoder_valid = enc_data.valid;
            volatile int32_t debug_position = enc_data.position;
            volatile int32_t debug_velocity = enc_data.velocity;
            (void)debug_encoder_enabled; (void)debug_encoder_valid;
            (void)debug_position; (void)debug_velocity;
            
            // Send over CAN
            can_app_tx(CAN_ID_ENCODER1, can_data, 8);
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