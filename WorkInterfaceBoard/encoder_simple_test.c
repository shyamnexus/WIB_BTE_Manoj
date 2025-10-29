/**
 * \file
 *
 * \brief Simple Encoder GPIO Test Application
 *
 * This is a minimal application that only tests encoder inputs using GPIO.
 * Use this to verify that encoder signals are reaching the MCU.
 * 
 * Pin Configuration:
 * - PA0: Encoder A input (GPIO input with interrupt)
 * - PA1: Encoder B input (GPIO input with interrupt)  
 * - PD17: Enable pin (GPIO output)
 * 
 * This program will:
 * 1. Initialize the MCU and GPIO pins
 * 2. Enable encoder monitoring
 * 3. Count pulses on both encoder lines
 * 4. Store results in debug variables for analysis
 */

#include <asf.h>
#include "encoder_gpio_test.h"

int main (void)
{
    /* Initialize system clock */
    sysclk_init();
    
    /* Initialize board */
    board_init();
    
    /* Initialize GPIO-based encoder test */
    if (!encoder_gpio_test_init()) {
        // Initialization failed - handle error
        while(1); // Stop execution if initialization fails
    }
    
    /* Run pin verification test first */
    encoder_gpio_test_pin_verification();
    
    /* Enable encoder and start monitoring */
    encoder_gpio_test_enable(true);
    
    /* Reset counters to start fresh */
    encoder_gpio_test_reset_counters();
    
    /* Main test loop - run for a specific duration */
    volatile uint32_t test_duration = 0;
    const uint32_t MAX_TEST_DURATION = 10000000; // Adjust as needed
    
    while (test_duration < MAX_TEST_DURATION) {
        // Read encoder data and store in debug variables
        encoder_gpio_data_t data = encoder_gpio_test_get_data();
        
        // Store data in volatile variables for debugging
        volatile uint32_t debug_a_pulses = data.encoder_a_pulses;
        volatile uint32_t debug_b_pulses = data.encoder_b_pulses;
        volatile uint32_t debug_a_rising = data.encoder_a_rising;
        volatile uint32_t debug_a_falling = data.encoder_a_falling;
        volatile uint32_t debug_b_rising = data.encoder_b_rising;
        volatile uint32_t debug_b_falling = data.encoder_b_falling;
        volatile bool debug_enabled = data.enabled;
        volatile bool debug_a_state = data.current_a_state;
        volatile bool debug_b_state = data.current_b_state;
        volatile bool debug_enable_state = data.enable_pin_state;
        
        // Prevent optimization
        (void)debug_a_pulses; (void)debug_b_pulses;
        (void)debug_a_rising; (void)debug_a_falling;
        (void)debug_b_rising; (void)debug_b_falling;
        (void)debug_enabled; (void)debug_a_state;
        (void)debug_b_state; (void)debug_enable_state;
        
        // Call debug status function periodically
        if (test_duration % 100000 == 0) {
            encoder_gpio_test_debug_status();
        }
        
        // Simple delay
        for (volatile int i = 0; i < 1000; i++);
        
        test_duration++;
    }
    
    /* Disable encoder */
    encoder_gpio_test_enable(false);
    
    /* Final data read */
    encoder_gpio_data_t final_data = encoder_gpio_test_get_data();
    volatile uint32_t final_a_pulses = final_data.encoder_a_pulses;
    volatile uint32_t final_b_pulses = final_data.encoder_b_pulses;
    (void)final_a_pulses; (void)final_b_pulses;
    
    /* Infinite loop - program complete */
    while(1);
}