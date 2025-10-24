#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Encoder configuration
#define ENCODER_POLLING_RATE_MS    10    // Polling rate in milliseconds
#define VELOCITY_CALC_WINDOW_MS    100   // Time window for velocity calculation in ms
#define ENCODER_PULSES_PER_REV     2000  // Adjust based on your encoder specification
#define VELOCITY_SMOOTHING_FACTOR  0 //0.7f  // Smoothing factor for velocity (0.0 = no smoothing, 1.0 = no change)
#define DIRECTION_DEBOUNCE_MS      100    // Minimum time between direction changes in ms

// TC Quadrature Decoder Configuration
// ENC1 uses TC0 channel 0: PA5/TIOA0, PA1/TIOB0
// ENC2 uses TC0 channel 1: PA15/TIOA1, PA16/TIOB1
#define TC_QUADRATURE_TC           TC0
#define TC_QUADRATURE_CHANNEL_ENC1 0
#define TC_QUADRATURE_CHANNEL_ENC2 1

// Encoder pin definitions based on pinconfig_workhead_interface_pinconfig.csv
// TC quadrature decoder pins (TIOA/TIOB are automatically configured by TC)
#define ENC1_A_PIN                 PIO_PA5   // TIOA0
#define ENC1_B_PIN                 PIO_PA1   // TIOB0
#define ENC1_ENABLE_PIN            PIO_PD17
#define ENC2_A_PIN                 PIO_PA15  // TIOA1
#define ENC2_B_PIN                 PIO_PA16  // TIOB1
#define ENC2_ENABLE_PIN            PIO_PD27

// Encoder availability flags
#define ENCODER1_AVAILABLE         1
#define ENCODER2_AVAILABLE         1

// TC Quadrature Decoder configuration
#define TC_QUADRATURE_FILTER       3       // Glitch filter value (0-63)
#define TC_QUADRATURE_MAX_FILTER   63      // Maximum filter value  

// Encoder data structure
typedef struct {
    uint32_t position;        // Current position (pulses from TC counter)
    int32_t velocity;         // Current velocity (pulses per second)
    int32_t smoothed_velocity; // Smoothed velocity for transmission
    uint8_t direction;        // Direction: 0=stopped, 1=forward, 2=reverse
    uint32_t last_position;   // Previous position for velocity calculation
    uint32_t last_update_time; // Last update time in ms
    uint32_t last_direction_change; // Time of last direction change
    uint32_t pulse_count;     // Total pulse count for velocity calculation
    uint32_t velocity_window_start; // Start time of velocity calculation window
    uint32_t tc_channel;      // TC channel number for this encoder
} encoder_data_t;

// Function prototypes
bool encoder_init(void);
void encoder_poll(encoder_data_t* enc_data);
void encoder_task(void *arg);
int32_t calculate_velocity(encoder_data_t* enc_data, uint32_t current_time);
void apply_velocity_smoothing(encoder_data_t* enc_data);
bool is_direction_change_allowed(encoder_data_t* enc_data, uint32_t current_time, uint8_t new_direction);

// TC Quadrature Decoder functions
bool encoder_tc_init(void);
bool encoder_tc_channel_init(uint32_t channel);
uint32_t encoder_tc_get_position(uint32_t channel);
void encoder_tc_reset_position(uint32_t channel);
uint8_t encoder_tc_get_direction(uint32_t channel);
void encoder_debug_tc_status(void);
void encoder_debug_gpio_read(void);
uint32_t encoder_gpio_read_position(uint32_t channel);

#ifdef __cplusplus
}
#endif