/*
 * encoder_gpio_test.c
 *
 * Created: 8/21/2025
 * Author: MKumar
 * 
 * Simple GPIO-based encoder pulse counting for verification
 * - PA0: Encoder A input (GPIO input with interrupt)
 * - PA1: Encoder B input (GPIO input with interrupt)  
 * - PD17: Enable pin (GPIO output)
 * 
 * This implementation uses simple GPIO edge detection to count pulses
 * and verify that encoder signals are reaching the MCU.
 */

#include "encoder_gpio_test.h"
#include "asf.h"
#include <stdint.h>
#include <stdbool.h>

// Global variables for pulse counting
static volatile uint32_t g_encoder_a_pulse_count = 0;
static volatile uint32_t g_encoder_b_pulse_count = 0;
static volatile uint32_t g_encoder_a_rising_edges = 0;
static volatile uint32_t g_encoder_a_falling_edges = 0;
static volatile uint32_t g_encoder_b_rising_edges = 0;
static volatile uint32_t g_encoder_b_falling_edges = 0;
static volatile bool g_encoder_enabled = false;
static volatile bool g_initialized = false;

// Previous state for edge detection
static volatile bool g_prev_a_state = false;
static volatile bool g_prev_b_state = false;

// Function prototypes
static void encoder_gpio_configure_pins(void);
static void encoder_gpio_configure_interrupts(void);
static void encoder_gpio_clear_interrupts(void);

// Interrupt handlers
void PIOA_Handler(void)
{
    // Check if interrupt is from our pins
    uint32_t pio_status = pio_get_interrupt_status(PIOA, PIO_INTERRUPT_ENABLE);
    
    if (pio_status & PIO_PA0) {
        // PA0 (Encoder A) interrupt
        bool current_a_state = pio_get(PIOA, PIO_PA0);
        
        if (current_a_state != g_prev_a_state) {
            if (current_a_state) {
                g_encoder_a_rising_edges++;
            } else {
                g_encoder_a_falling_edges++;
            }
            g_encoder_a_pulse_count++;
            g_prev_a_state = current_a_state;
        }
    }
    
    if (pio_status & PIO_PA1) {
        // PA1 (Encoder B) interrupt
        bool current_b_state = pio_get(PIOA, PIO_PA1);
        
        if (current_b_state != g_prev_b_state) {
            if (current_b_state) {
                g_encoder_b_rising_edges++;
            } else {
                g_encoder_b_falling_edges++;
            }
            g_encoder_b_pulse_count++;
            g_prev_b_state = current_b_state;
        }
    }
    
    // Clear interrupt flags
    pio_clear_interrupt(PIOA, pio_status);
}

bool encoder_gpio_test_init(void)
{
    if (g_initialized) {
        return true; // Already initialized
    }
    
    // Configure pins
    encoder_gpio_configure_pins();
    
    // Configure interrupts
    encoder_gpio_configure_interrupts();
    
    // Initialize enable pin (PD17) as output, initially disabled (high)
    pio_configure(PIOD, PIO_OUTPUT_0, PIO_PD17, 0);
    pio_set(PIOD, PIO_PD17); // Set high (disabled)
    
    // Initialize counters
    g_encoder_a_pulse_count = 0;
    g_encoder_b_pulse_count = 0;
    g_encoder_a_rising_edges = 0;
    g_encoder_a_falling_edges = 0;
    g_encoder_b_rising_edges = 0;
    g_encoder_b_falling_edges = 0;
    g_encoder_enabled = false;
    
    // Read initial pin states
    g_prev_a_state = pio_get(PIOA, PIO_PA0);
    g_prev_b_state = pio_get(PIOA, PIO_PA1);
    
    g_initialized = true;
    
    return true;
}

static void encoder_gpio_configure_pins(void)
{
    // Enable PIOA and PIOD clocks
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);
    
    // Configure PA0 (Encoder A) as GPIO input with pull-up
    pio_configure(PIOA, PIO_INPUT | PIO_PULLUP, PIO_PA0, 0);
    
    // Configure PA1 (Encoder B) as GPIO input with pull-up
    pio_configure(PIOA, PIO_INPUT | PIO_PULLUP, PIO_PA1, 0);
}

static void encoder_gpio_configure_interrupts(void)
{
    // Configure PA0 for both rising and falling edge interrupts
    pio_configure_interrupt(PIOA, PIO_PA0, PIO_IT_EDGE | PIO_IT_BOTH_EDGE);
    
    // Configure PA1 for both rising and falling edge interrupts
    pio_configure_interrupt(PIOA, PIO_PA1, PIO_IT_EDGE | PIO_IT_BOTH_EDGE);
    
    // Enable interrupts in PIO controller
    pio_enable_interrupt(PIOA, PIO_PA0 | PIO_PA1);
    
    // Enable PIOA interrupt in NVIC
    NVIC_EnableIRQ(PIOA_IRQn);
    
    // Clear any pending interrupts
    encoder_gpio_clear_interrupts();
}

static void encoder_gpio_clear_interrupts(void)
{
    // Clear any pending interrupts
    pio_clear_interrupt(PIOA, PIO_PA0 | PIO_PA1);
}

bool encoder_gpio_test_enable(bool enable)
{
    if (!g_initialized) {
        return false;
    }
    
    if (enable) {
        // Enable encoder (set PD17 low)
        pio_clear(PIOD, PIO_PD17);
        g_encoder_enabled = true;
        
        // Clear all counters
        encoder_gpio_test_reset_counters();
        
        // Clear any pending interrupts
        encoder_gpio_clear_interrupts();
        
        // Read current pin states
        g_prev_a_state = pio_get(PIOA, PIO_PA0);
        g_prev_b_state = pio_get(PIOA, PIO_PA1);
        
    } else {
        // Disable encoder (set PD17 high)
        pio_set(PIOD, PIO_PD17);
        g_encoder_enabled = false;
    }
    
    return true;
}

void encoder_gpio_test_reset_counters(void)
{
    g_encoder_a_pulse_count = 0;
    g_encoder_b_pulse_count = 0;
    g_encoder_a_rising_edges = 0;
    g_encoder_a_falling_edges = 0;
    g_encoder_b_rising_edges = 0;
    g_encoder_b_falling_edges = 0;
}

encoder_gpio_data_t encoder_gpio_test_get_data(void)
{
    encoder_gpio_data_t data;
    
    data.encoder_a_pulses = g_encoder_a_pulse_count;
    data.encoder_b_pulses = g_encoder_b_pulse_count;
    data.encoder_a_rising = g_encoder_a_rising_edges;
    data.encoder_a_falling = g_encoder_a_falling_edges;
    data.encoder_b_rising = g_encoder_b_rising_edges;
    data.encoder_b_falling = g_encoder_b_falling_edges;
    data.enabled = g_encoder_enabled;
    data.initialized = g_initialized;
    
    // Read current pin states
    data.current_a_state = pio_get(PIOA, PIO_PA0);
    data.current_b_state = pio_get(PIOA, PIO_PA1);
    data.enable_pin_state = pio_get(PIOD, PIO_PD17);
    
    return data;
}

void encoder_gpio_test_debug_status(void)
{
    // Store all debug values for analysis
    volatile encoder_gpio_data_t debug_data = encoder_gpio_test_get_data();
    
    // Additional debug information
    volatile uint32_t debug_pioa_pdsr = PIOA->PIO_PDSR;
    volatile uint32_t debug_piod_pdsr = PIOD->PIO_PDSR;
    volatile uint32_t debug_pioa_ier = PIOA->PIO_IER;
    volatile uint32_t debug_pioa_imr = PIOA->PIO_IMR;
    volatile uint32_t debug_pioa_isr = PIOA->PIO_ISR;
    
    (void)debug_data; (void)debug_pioa_pdsr; (void)debug_piod_pdsr;
    (void)debug_pioa_ier; (void)debug_pioa_imr; (void)debug_pioa_isr;
}

// Simple test function that runs for a specified duration
void encoder_gpio_test_run_duration(uint32_t duration_ms)
{
    if (!g_initialized) {
        encoder_gpio_test_init();
    }
    
    // Enable encoder
    encoder_gpio_test_enable(true);
    
    // Reset counters
    encoder_gpio_test_reset_counters();
    
    // Run test for specified duration
    volatile uint32_t start_time = 0;
    volatile uint32_t current_time = 0;
    
    // Simple time tracking (approximate)
    for (volatile uint32_t i = 0; i < duration_ms * 1000; i++) {
        // Read and store data periodically
        if (i % 10000 == 0) { // Every 10ms
            encoder_gpio_test_debug_status();
        }
    }
    
    // Disable encoder
    encoder_gpio_test_enable(false);
}

// Continuous test function for oscilloscope verification
void encoder_gpio_test_continuous(void)
{
    if (!g_initialized) {
        encoder_gpio_test_init();
    }
    
    // Enable encoder
    encoder_gpio_test_enable(true);
    
    // Reset counters
    encoder_gpio_test_reset_counters();
    
    // Run continuously
    while(1) {
        // Read and store data every 100ms
        encoder_gpio_test_debug_status();
        
        // Simple delay
        for (volatile int i = 0; i < 100000; i++);
    }
}

// Test function to verify pin configuration
void encoder_gpio_test_pin_verification(void)
{
    if (!g_initialized) {
        encoder_gpio_test_init();
    }
    
    // Test sequence to verify pin states
    // Phase 1: Read initial states
    volatile bool initial_a = pio_get(PIOA, PIO_PA0);
    volatile bool initial_b = pio_get(PIOA, PIO_PA1);
    volatile bool initial_enable = pio_get(PIOD, PIO_PD17);
    
    // Phase 2: Enable encoder and read states
    encoder_gpio_test_enable(true);
    volatile bool enabled_a = pio_get(PIOA, PIO_PA0);
    volatile bool enabled_b = pio_get(PIOA, PIO_PA1);
    volatile bool enabled_enable = pio_get(PIOD, PIO_PD17);
    
    // Phase 3: Disable encoder and read states
    encoder_gpio_test_enable(false);
    volatile bool disabled_a = pio_get(PIOA, PIO_PA0);
    volatile bool disabled_b = pio_get(PIOA, PIO_PA1);
    volatile bool disabled_enable = pio_get(PIOD, PIO_PD17);
    
    // Store all values for analysis
    (void)initial_a; (void)initial_b; (void)initial_enable;
    (void)enabled_a; (void)enabled_b; (void)enabled_enable;
    (void)disabled_a; (void)disabled_b; (void)disabled_enable;
}