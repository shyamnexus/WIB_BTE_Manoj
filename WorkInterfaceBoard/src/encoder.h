#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include <stdbool.h>

// Encoder data structure
typedef struct {
    uint32_t position;              // Current position (pulse count)
    int32_t velocity;              // Current velocity (pulses per second)
    int32_t smoothed_velocity;     // Smoothed velocity (moving average)
    uint8_t direction;             // 0=stopped, 1=forward, 2=reverse
    uint32_t last_position;        // Previous position for delta calculation
    uint32_t last_update_time;     // Last time position was updated (ms)
    uint32_t last_direction_change; // Last time direction changed (ms)
    uint32_t pulse_count;          // Pulse count for velocity calculation
    uint32_t velocity_window_start; // Start time of velocity calculation window
    uint32_t tc_channel;           // TC channel number for this encoder
} encoder_data_t;

// Encoder channel definitions
#define TC_QUADRATURE_CHANNEL_ENC1    0
#define TC_QUADRATURE_CHANNEL_ENC2    1

// Encoder pin definitions
#define ENC1_A_PIN                    PIO_PA0  // TIOA0
#define ENC1_B_PIN                    PIO_PA1  // TIOB0
#define ENC1_ENABLE_PIN               PIO_PD17
#define ENC2_A_PIN                    PIO_PA15  // TIOA1
#define ENC2_B_PIN                    PIO_PA16  // TIOB1
#define ENC2_ENABLE_PIN               PIO_PD27

// Encoder availability
#define ENCODER2_AVAILABLE            1  // Both encoders available

// Quadrature filter value (0-15, higher = more filtering)
#define TC_QUADRATURE_FILTER          3

// CAN message IDs for encoder data
#define CAN_ID_ENCODER1_DIR_VEL       0x100
#define CAN_ID_ENCODER2_DIR_VEL       0x101

// Function prototypes
bool encoder_init(void);
void encoder_poll(encoder_data_t* enc_data);
void encoder_task(void *arg);
int32_t calculate_velocity(encoder_data_t* enc_data, uint32_t current_time);
void apply_velocity_smoothing(encoder_data_t* enc_data);
bool is_direction_change_allowed(encoder_data_t* enc_data, uint32_t current_time, uint8_t new_direction);

// TC-specific functions
bool encoder_tc_init(void);
bool encoder_tc_channel_init(uint32_t channel);
uint32_t encoder_tc_get_position(uint32_t channel);
void encoder_tc_reset_position(uint32_t channel);
uint8_t encoder_tc_get_direction(uint32_t channel);

#endif // ENCODER_H
