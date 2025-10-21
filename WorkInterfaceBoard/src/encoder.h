#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Encoder configuration
#define ENCODER1_TC_CHANNEL    0  // TC0 channel 0 (TIOA0/TIOB0)
#define ENCODER2_TC_CHANNEL    1  // TC0 channel 1 (TIOA1/TIOB1)
#define ENCODER_MAX_COUNT      0xFFFF  // 16-bit counter maximum
#define ENCODER_CPR            1000    // Counts per revolution (adjust based on encoder)

// Encoder data structure
typedef struct {
    int32_t position;      // Current position in counts
    int32_t velocity;      // Velocity in counts per second (signed)
    uint32_t speed;        // Absolute speed in counts per second (unsigned)
    uint8_t direction;     // Direction: 0=stopped, 1=forward, 2=reverse
    uint32_t timestamp;    // Timestamp of last reading
    bool valid;           // Data validity flag
} encoder_data_t;

// Direction constants
#define ENCODER_DIR_STOPPED    0
#define ENCODER_DIR_FORWARD    1
#define ENCODER_DIR_REVERSE    2

// Function prototypes
bool encoder_init(void);
bool encoder_read_data(encoder_data_t* enc1_data, encoder_data_t* enc2_data);
void encoder_reset_counters(void);
int32_t encoder_get_position(uint8_t encoder_num);
int32_t encoder_get_velocity(uint8_t encoder_num);

// CAN message helper functions
void encoder_encode_can_message(const encoder_data_t* data, uint8_t* payload);
void encoder_decode_can_message(const uint8_t* payload, encoder_data_t* data);

#ifdef __cplusplus
}
#endif