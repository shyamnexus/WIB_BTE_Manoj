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
    // According to SAM4E datasheet section 38.6.16.1, for QDE mode:
    // - Use internal clock source (the QDE module handles encoder inputs separately)
    // - Enable waveform mode
    // - Set up for quadrature decoding
    // Note: When QDE is enabled, TIOA0 and TIOB0 are used as encoder inputs, not clock sources
    // The TC channel needs an internal clock to function properly
    TC0->TC_CHANNEL[0].TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 |  // Internal clock (QDE handles encoder inputs)
                                 TC_CMR_WAVE |                  // Enable waveform mode
                                 TC_CMR_WAVSEL_UP;              // UP mode for QDE
    
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
    // According to SAM4E datasheet section 38.6.16.1, QDE configuration:
    // Enable QDE in TC_BMR register
    TC0->TC_BMR = TC_BMR_QDEN |           // Enable QDE
                  TC_BMR_POSEN |          // Enable position counting
                  TC_BMR_SPEEDEN |        // Enable speed counting
                  TC_BMR_FILTER |         // Enable input filter
                  TC_BMR_MAXFILT(0x3F);   // Set maximum filter value (63)
    
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
    
    // Check QDE status according to datasheet section 38.6.16.1
    volatile uint32_t debug_qde_status = TC0->TC_QISR;  // QDE Interrupt Status Register
    volatile bool debug_qde_enabled = (TC0->TC_BMR & TC_BMR_QDEN) != 0;
    volatile bool debug_position_enabled = (TC0->TC_BMR & TC_BMR_POSEN) != 0;
    volatile bool debug_speed_enabled = (TC0->TC_BMR & TC_BMR_SPEEDEN) != 0;
    volatile bool debug_filter_enabled = (TC0->TC_BMR & TC_BMR_FILTER) != 0;
    volatile uint32_t debug_max_filter = (TC0->TC_BMR & TC_BMR_MAXFILT_Msk) >> TC_BMR_MAXFILT_Pos;
    
    // Check if TIOA0 and TIOB0 pins are configured correctly
    // For PIO_PERIPH_A, both PIO_ABCDSR[0] and PIO_ABCDSR[1] should have the bit cleared (0)
    volatile bool debug_tioa0_configured = ((PIOA->PIO_ABCDSR[0] & PIO_PA0) == 0) && ((PIOA->PIO_ABCDSR[1] & PIO_PA0) == 0);
    volatile bool debug_tiob0_configured = ((PIOA->PIO_ABCDSR[0] & PIO_PA1) == 0) && ((PIOA->PIO_ABCDSR[1] & PIO_PA1) == 0);
    
    // Check if enable pin is configured correctly
    volatile bool debug_enable_pin_configured = (PIOD->PIO_OSR & PIO_PD17) != 0; // Should be 1 for output
    
    // Check pin states
    volatile bool debug_tioa0_state = (PIOA->PIO_PDSR & PIO_PA0) != 0;
    volatile bool debug_tiob0_state = (PIOA->PIO_PDSR & PIO_PA1) != 0;
    volatile bool debug_enable_pin_state = (PIOD->PIO_PDSR & PIO_PD17) != 0;
    
    (void)debug_tc_cv; (void)debug_tc_sr; (void)debug_tc_cmr; (void)debug_tc_bmr; (void)debug_tc_qier;
    (void)debug_pioa_pdsr; (void)debug_piod_pdsr; (void)debug_encoder_enabled; (void)debug_encoder_initialized;
    (void)debug_position; (void)debug_tioa0_configured; (void)debug_tiob0_configured; 
    (void)debug_enable_pin_configured; (void)debug_qde_status; (void)debug_qde_enabled;
    (void)debug_position_enabled; (void)debug_speed_enabled; (void)debug_filter_enabled;
    (void)debug_max_filter; (void)debug_tioa0_state; (void)debug_tiob0_state; (void)debug_enable_pin_state;
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

// Check QDE status according to datasheet section 38.6.16.1
void encoder1_check_qde_status(void)
{
    // Check QDE interrupt status register
    volatile uint32_t qde_status = TC0->TC_QISR;
    volatile bool qde_error = (qde_status & TC_QISR_QERR) != 0;
    volatile bool direction_changed = (qde_status & TC_QISR_DIRCHG) != 0;
    volatile bool index_pulse = (qde_status & TC_QISR_IDX) != 0;
    
    // Check if QDE is properly enabled
    volatile bool qde_enabled = (TC0->TC_BMR & TC_BMR_QDEN) != 0;
    volatile bool position_enabled = (TC0->TC_BMR & TC_BMR_POSEN) != 0;
    volatile bool speed_enabled = (TC0->TC_BMR & TC_BMR_SPEEDEN) != 0;
    
    // Check Timer Counter status
    volatile uint32_t tc_status = TC0->TC_CHANNEL[0].TC_SR;
    volatile bool tc_clock_enabled = (tc_status & TC_SR_CLKSTA) != 0;
    volatile bool tc_overflow = (tc_status & TC_SR_LOVRS) != 0;
    
    // Check pin states
    volatile bool tioa0_state = (PIOA->PIO_PDSR & PIO_PA0) != 0;
    volatile bool tiob0_state = (PIOA->PIO_PDSR & PIO_PA1) != 0;
    
    // Store all debug values
    (void)qde_status; (void)qde_error; (void)direction_changed; (void)index_pulse;
    (void)qde_enabled; (void)position_enabled; (void)speed_enabled;
    (void)tc_status; (void)tc_clock_enabled; (void)tc_overflow;
    (void)tioa0_state; (void)tiob0_state;
}

// Configure encoder pins as GPIO outputs for testing
void encoder1_configure_pins_as_gpio(void)
{
    // Enable PIOA and PIOD clocks
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);
    
    // Configure PA0 (TIOA0) as GPIO output
    pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA0, 0);
    pio_clear(PIOA, PIO_PA0); // Start with low
    
    // Configure PA1 (TIOB0) as GPIO output  
    pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA1, 0);
    pio_clear(PIOA, PIO_PA1); // Start with low
    
    // Configure PD17 (Enable) as GPIO output
    pio_configure(PIOD, PIO_OUTPUT_0, PIO_PD17, 0);
    pio_set(PIOD, PIO_PD17); // Start with high (disabled state)
}

// Restore encoder pins to peripheral mode
void encoder1_restore_pins_as_peripheral(void)
{
    // Configure PA0 as TIOA0 (peripheral A)
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA0, 0);
    
    // Configure PA1 as TIOB0 (peripheral A)  
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA1, 0);
    
    // Keep PD17 as output (it's always GPIO)
    // pio_configure(PIOD, PIO_OUTPUT_0, PIO_PD17, 0);
}

// Simple pin toggle test - toggles each pin individually
void encoder1_pin_toggle_test(void)
{
    // Configure pins as GPIO outputs
    encoder1_configure_pins_as_gpio();
    
    // Test sequence: Each pin toggles for 1 second
    // This makes it easy to identify each pin on the oscilloscope
    
    // Test PA0 (TIOA0) - Encoder A
    for (volatile int i = 0; i < 1000000; i++) {

        pio_set(PIOA, PIO_PA0);

        for (volatile int j = 0; j < 1000; j++); // ~1ms delay
		pio_clear(PIOA, PIO_PA0);
		for (volatile int j = 0; j < 1000; j++); // ~1ms delay
    }
    
    // Small pause between tests
    for (volatile int i = 0; i < 100000; i++);
    
    // Test PA1 (TIOB0) - Encoder B  
    for (volatile int i = 0; i < 1000000; i++) {

        pio_set(PIOA, PIO_PA1);

        for (volatile int j = 0; j < 1000; j++); // ~1ms delay
		pio_clear(PIOA, PIO_PA1);
		for (volatile int j = 0; j < 1000; j++); // ~1ms delay
    }
    
    // Small pause between tests
    for (volatile int i = 0; i < 100000; i++);
    
    // Test PD17 (Enable) - Encoder Enable
    for (volatile int i = 0; i < 1000000; i++) {

        pio_set(PIOD, PIO_PD17);
        for (volatile int j = 0; j < 1000; j++); // ~1ms delay
		pio_clear(PIOD, PIO_PD17);
		for (volatile int j = 0; j < 1000; j++); // ~1ms delay
    }
}

// Comprehensive test sequence for all pins
void encoder1_test_all_pins_sequence(void)
{
    // Configure pins as GPIO outputs
    encoder1_configure_pins_as_gpio();
    
    // Test sequence with different patterns to make oscilloscope identification easier
    
    // Phase 1: All pins low for 2 seconds (baseline)
    pio_clear(PIOA, PIO_PA0);
    pio_clear(PIOA, PIO_PA1);
    pio_clear(PIOD, PIO_PD17);
    for (volatile int i = 0; i < 2000000; i++);
    
    // Phase 2: PA0 (TIOA0) square wave - 1Hz for 3 seconds
    for (volatile int cycle = 0; cycle < 3; cycle++) {
        pio_set(PIOA, PIO_PA0);
        for (volatile int i = 0; i < 500000; i++); // 0.5s high
        pio_clear(PIOA, PIO_PA0);
        for (volatile int i = 0; i < 500000; i++); // 0.5s low
    }
    
    // Phase 3: PA1 (TIOB0) square wave - 2Hz for 3 seconds  
    for (volatile int cycle = 0; cycle < 6; cycle++) {
        pio_set(PIOA, PIO_PA1);
        for (volatile int i = 0; i < 250000; i++); // 0.25s high
        pio_clear(PIOA, PIO_PA1);
        for (volatile int i = 0; i < 250000; i++); // 0.25s low
    }
    
    // Phase 4: PD17 (Enable) square wave - 0.5Hz for 4 seconds
    for (volatile int cycle = 0; cycle < 2; cycle++) {
        pio_set(PIOD, PIO_PD17);
        for (volatile int i = 0; i < 1000000; i++); // 1s high
        pio_clear(PIOD, PIO_PD17);
        for (volatile int i = 0; i < 1000000; i++); // 1s low
    }
    
    // Phase 5: All pins high for 2 seconds
    pio_set(PIOA, PIO_PA0);
    pio_set(PIOA, PIO_PA1);
    pio_set(PIOD, PIO_PD17);
    for (volatile int i = 0; i < 2000000; i++);
    
    // Phase 6: Alternating pattern - PA0 and PA1 toggle alternately
    for (volatile int cycle = 0; cycle < 10; cycle++) {
        pio_set(PIOA, PIO_PA0);
        pio_clear(PIOA, PIO_PA1);
        for (volatile int i = 0; i < 200000; i++); // 0.2s
        
        pio_clear(PIOA, PIO_PA0);
        pio_set(PIOA, PIO_PA1);
        for (volatile int i = 0; i < 200000; i++); // 0.2s
    }
    
    // Phase 7: All pins low for 2 seconds (end)
    pio_clear(PIOA, PIO_PA0);
    pio_clear(PIOA, PIO_PA1);
    pio_clear(PIOD, PIO_PD17);
    for (volatile int i = 0; i < 2000000; i++);
}

// Standalone pin test - runs continuously for oscilloscope testing
void encoder1_standalone_pin_test(void)
{
    // Configure pins as GPIO outputs
    encoder1_configure_pins_as_gpio();
    
    // Run the test sequence in a loop
    while(1) {
        encoder1_test_all_pins_sequence();
        
        // Add a longer pause between test cycles
        for (volatile int i = 0; i < 5000000; i++); // 5 second pause
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
            encoder1_check_qde_status();
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