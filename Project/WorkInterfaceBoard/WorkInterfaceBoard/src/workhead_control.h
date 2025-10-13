/**
 * \file workhead_control.h
 *
 * \brief Workhead Control Module
 *
 * This module provides hardware control functionality for the workhead interface board.
 * It handles GPIO control, sensor reading, and workhead operation.
 */

#ifndef WORKHEAD_CONTROL_H
#define WORKHEAD_CONTROL_H

#include <asf.h>
#include "workhead_can.h"

// GPIO Pin Definitions (based on typical workhead interface board)
#define PIN_WORKHEAD_ENABLE    PIO_PA2
#define PIN_WORKHEAD_DIR       PIO_PA3
#define PIN_WORKHEAD_STEP      PIO_PA4
#define PIN_WORKHEAD_LIMIT_MIN PIO_PA5
#define PIN_WORKHEAD_LIMIT_MAX PIO_PA6
#define PIN_WORKHEAD_SENSOR1   PIO_PA7
#define PIN_WORKHEAD_SENSOR2   PIO_PA8
#define PIN_STATUS_LED         PIO_PA9
#define PIN_ERROR_LED          PIO_PA10

// Workhead States
typedef enum {
    WORKHEAD_STATE_IDLE = 0,
    WORKHEAD_STATE_WORKING,
    WORKHEAD_STATE_ERROR,
    WORKHEAD_STATE_MAINTENANCE,
    WORKHEAD_STATE_CALIBRATING
} workhead_state_t;

// Workhead Configuration
typedef struct {
    uint8_t workhead_id;
    uint8_t max_position;
    uint8_t min_position;
    uint8_t max_speed;
    uint8_t current_position;
    uint8_t target_position;
    uint8_t current_speed;
    workhead_state_t state;
    uint8_t error_code;
    uint32_t step_count;
    uint32_t total_steps;
} workhead_config_t;

// Function prototypes
void workhead_init(void);
void workhead_set_state(workhead_state_t state);
workhead_state_t workhead_get_state(void);
void workhead_set_position(uint8_t position);
uint8_t workhead_get_position(void);
void workhead_set_speed(uint8_t speed);
uint8_t workhead_get_speed(void);
void workhead_start(void);
void workhead_stop(void);
void workhead_emergency_stop(void);
void workhead_reset(void);
void workhead_calibrate(void);
void workhead_update_status(workhead_status_t *status);
void workhead_process_command(const workhead_command_t *command);
void workhead_update(void); // Call from main loop
void workhead_set_leds(bool status_led, bool error_led);

// Sensor reading functions
bool workhead_read_limit_min(void);
bool workhead_read_limit_max(void);
bool workhead_read_sensor1(void);
bool workhead_read_sensor2(void);
uint8_t workhead_read_temperature(void);
uint8_t workhead_read_vibration(void);

#endif // WORKHEAD_CONTROL_H