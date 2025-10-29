/*
 * encoder.h
 *
 * Created: 8/21/2025
 * Author: MKumar
 * 
 * Quadrature Decoder (QDE) implementation for Encoder1
 * - PA0/TIOA0: Encoder A input
 * - PA1/TIOB0: Encoder B input  
 * - PD17: Enable pin (active low)
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#include "sam4e.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Encoder configuration
#define ENCODER1_TC_CHANNEL    0  // Timer Counter 0
#define ENCODER1_TIOA_PIN      PIO_PA0_IDX  // PA0/TIOA0
#define ENCODER1_TIOB_PIN      PIO_PA1_IDX  // PA1/TIOB0
#define ENCODER1_ENABLE_PIN    PIO_PD17_IDX // PD17 (active low)

// CAN ID for encoder data
#define CAN_ID_ENCODER1        0x130u

// Encoder data structure
typedef struct {
    int32_t position;        // Current encoder position (counts)
    int32_t velocity;        // Encoder velocity (counts per sample)
    bool enabled;           // Encoder enable status
    bool valid;             // Data validity flag
} encoder_data_t;

// Function prototypes
bool encoder1_init(void);
bool encoder1_enable(bool enable);
int32_t encoder1_read_position(void);
int32_t encoder1_read_velocity(void);
encoder_data_t encoder1_get_data(void);
void encoder1_reset_position(void);
bool encoder1_is_enabled(void);
void encoder1_debug_status(void);
void encoder1_test_operation(void);
void encoder1_simple_test(void);
void encoder1_check_qde_status(void);

// FreeRTOS task for encoder reading and CAN transmission
void encoder1_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_H_ */