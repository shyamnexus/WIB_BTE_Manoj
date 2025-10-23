#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Encoder configuration based on pin config
#define ENCODER1_A_PIN        PIO_PA5_IDX    // PA5/TIOA0 - Encoder1 A channel
#define ENCODER1_B_PIN        PIO_PA1_IDX    // PA1/TIOB0 - Encoder1 B channel  
#define ENCODER1_ENABLE_PIN   PIO_PD17_IDX   // PD17 - Encoder1 enable line
#define ENCODER2_A_PIN        PIO_PA15_IDX   // PA15/TIOA1 - Encoder2 A channel
#define ENCODER2_B_PIN        PIO_PA16_IDX   // PA16/TIOB1 - Encoder2 B channel
#define ENCODER2_ENABLE_PIN   PIO_PD27_IDX   // PD27 - Encoder2 enable line

// Encoder data structure
typedef struct {
    int32_t position;        // Current position (incremental)
    int32_t velocity;        // Current velocity (pulses per sample period)
    bool direction;          // true = forward, false = reverse
    bool enabled;            // Encoder enable state
    uint32_t sample_count;   // Number of samples taken
} encoder_data_t;

// Encoder state structure for quadrature decoding
typedef struct {
    uint8_t last_state;      // Previous state of A and B pins (bit 1=A, bit 0=B)
    int32_t position;        // Current position
    int32_t velocity;        // Current velocity
    bool direction;          // Current direction
    uint32_t sample_count;   // Sample counter
} encoder_state_t;

// Function declarations
bool encoder_init(void);
void encoder_read_encoder1(encoder_data_t *data);
void encoder_read_encoder2(encoder_data_t *data);
void encoder_task(void *arg);
void encoder_enable_encoder1(bool enable);
void encoder_enable_encoder2(bool enable);
bool encoder_is_encoder1_enabled(void);
bool encoder_is_encoder2_enabled(void);

#ifdef __cplusplus
}
#endif