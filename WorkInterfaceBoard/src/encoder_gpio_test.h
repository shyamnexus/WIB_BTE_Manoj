/*
 * encoder_gpio_test.h
 *
 * Created: 8/21/2025
 * Author: MKumar
 * 
 * Simple GPIO-based encoder pulse counting for verification
 * - PA0: Encoder A input (GPIO input with interrupt)
 * - PA1: Encoder B input (GPIO input with interrupt)  
 * - PD17: Enable pin (GPIO output)
 */

#ifndef ENCODER_GPIO_TEST_H_
#define ENCODER_GPIO_TEST_H_

#include "sam4e.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Pin definitions
#define ENCODER_GPIO_A_PIN      PIO_PA0_IDX  // PA0 - Encoder A input
#define ENCODER_GPIO_B_PIN      PIO_PA1_IDX  // PA1 - Encoder B input
#define ENCODER_GPIO_ENABLE_PIN PIO_PD17_IDX // PD17 - Enable pin

// Encoder GPIO data structure
typedef struct {
    uint32_t encoder_a_pulses;      // Total pulses on Encoder A
    uint32_t encoder_b_pulses;      // Total pulses on Encoder B
    uint32_t encoder_a_rising;      // Rising edges on Encoder A
    uint32_t encoder_a_falling;     // Falling edges on Encoder A
    uint32_t encoder_b_rising;      // Rising edges on Encoder B
    uint32_t encoder_b_falling;     // Falling edges on Encoder B
    bool enabled;                   // Encoder enable status
    bool initialized;               // Initialization status
    bool current_a_state;           // Current state of Encoder A pin
    bool current_b_state;           // Current state of Encoder B pin
    bool enable_pin_state;          // Current state of enable pin
} encoder_gpio_data_t;

// Function prototypes
bool encoder_gpio_test_init(void);
bool encoder_gpio_test_enable(bool enable);
void encoder_gpio_test_reset_counters(void);
encoder_gpio_data_t encoder_gpio_test_get_data(void);
void encoder_gpio_test_debug_status(void);
void encoder_gpio_test_run_duration(uint32_t duration_ms);
void encoder_gpio_test_continuous(void);
void encoder_gpio_test_pin_verification(void);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_GPIO_TEST_H_ */