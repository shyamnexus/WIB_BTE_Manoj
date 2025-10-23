#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Encoder configuration
#define ENCODER_POLLING_RATE_MS    10    // Polling rate in milliseconds
#define VELOCITY_CALC_WINDOW_MS    100   // Time window for velocity calculation in ms
#define ENCODER_PULSES_PER_REV     1000  // Adjust based on your encoder specification

// Encoder pin definitions based on pinconfig_workhead_interface_pinconfig.csv
#define ENC1_A_PIN                 PIO_PA5
#define ENC1_B_PIN                 PIO_PA1
#define ENC1_ENABLE_PIN            PIO_PD17
#define ENC2_A_PIN                 PIO_PA15
#define ENC2_B_PIN                 PIO_PA16
#define ENC2_ENABLE_PIN            PIO_PD27

// Encoder data structure
typedef struct {
    uint32_t position;        // Current position (pulses)
    int32_t velocity;         // Current velocity (pulses per second)
    uint8_t direction;        // Direction: 0=stopped, 1=forward, 2=reverse
    uint8_t state_a;          // Current state of A channel
    uint8_t state_b;          // Current state of B channel
    uint8_t prev_state_a;     // Previous state of A channel
    uint8_t prev_state_b;     // Previous state of B channel
    uint32_t last_update_time; // Last update time in ms
    uint32_t pulse_count;     // Total pulse count for velocity calculation
    uint32_t velocity_window_start; // Start time of velocity calculation window
} encoder_data_t;

// Function prototypes
bool encoder_init(void);
void encoder_poll(encoder_data_t* enc_data);
void encoder_task(void *arg);
int32_t calculate_velocity(encoder_data_t* enc_data, uint32_t current_time);

#ifdef __cplusplus
}
#endif