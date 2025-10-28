#ifndef SIMPLE_ENCODER_H
#define SIMPLE_ENCODER_H

#include <stdint.h>
#include <stdbool.h>

// Simple encoder data structure
typedef struct {
    uint32_t position;              // Current position (pulse count)
    int32_t velocity;              // Current velocity (pulses per second)
    uint8_t direction;             // 0=stopped, 1=forward, 2=reverse
    uint32_t last_update_time;     // Last time position was updated (ms)
} simple_encoder_data_t;

// Encoder pin definitions for PA0 (TIOA0) and PA1 (TIOB0)
#define ENC_A_PIN                    PIO_PA0  // TIOA0 (pin PA0)
#define ENC_B_PIN                    PIO_PA1  // TIOB0 (pin PA1)

// Use existing CAN message IDs from can_app.h
// CAN_ID_ENCODER1_DIR_VEL is already defined as 0x130
// CAN_ID_ENCODER1_PINS is already defined as 0x188

// Function prototypes
bool simple_encoder_init(void);
void simple_encoder_poll(void);
void simple_encoder_task(void *arg);
uint8_t simple_encoder_read_state(void);
simple_encoder_data_t* simple_encoder_get_data(void);

// Interrupt handler prototype
void encoder_interrupt_handler(uint32_t id, uint32_t mask);

#endif // SIMPLE_ENCODER_H
