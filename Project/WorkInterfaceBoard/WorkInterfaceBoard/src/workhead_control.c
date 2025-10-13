/**
 * \file workhead_control.c
 *
 * \brief Workhead Control Implementation
 */

#include "workhead_control.h"

// Global workhead configuration
static workhead_config_t workhead = {0};
static bool workhead_initialized = false;

// Timing variables for stepper control
static uint32_t step_timer = 0;
static uint32_t step_interval = 1000; // microseconds between steps
static bool step_active = false;

/**
 * \brief Initialize workhead control
 */
void workhead_init(void)
{
    // Configure GPIO pins
    pio_configure_pin(PIN_WORKHEAD_ENABLE, PIO_OUTPUT_0 | PIO_DEFAULT);
    pio_configure_pin(PIN_WORKHEAD_DIR, PIO_OUTPUT_0 | PIO_DEFAULT);
    pio_configure_pin(PIN_WORKHEAD_STEP, PIO_OUTPUT_0 | PIO_DEFAULT);
    pio_configure_pin(PIN_WORKHEAD_LIMIT_MIN, PIO_INPUT | PIO_PULLUP);
    pio_configure_pin(PIN_WORKHEAD_LIMIT_MAX, PIO_INPUT | PIO_PULLUP);
    pio_configure_pin(PIN_WORKHEAD_SENSOR1, PIO_INPUT | PIO_PULLUP);
    pio_configure_pin(PIN_WORKHEAD_SENSOR2, PIO_INPUT | PIO_PULLUP);
    pio_configure_pin(PIN_STATUS_LED, PIO_OUTPUT_0 | PIO_DEFAULT);
    pio_configure_pin(PIN_ERROR_LED, PIO_OUTPUT_0 | PIO_DEFAULT);
    
    // Initialize workhead configuration
    workhead.workhead_id = CAN_NODE_ID;
    workhead.max_position = 100;
    workhead.min_position = 0;
    workhead.max_speed = 100;
    workhead.current_position = 0;
    workhead.target_position = 0;
    workhead.current_speed = 0;
    workhead.state = WORKHEAD_STATE_IDLE;
    workhead.error_code = 0;
    workhead.step_count = 0;
    workhead.total_steps = 2000; // Total steps for full range
    
    // Disable workhead initially
    pio_set_pin_low(PIN_WORKHEAD_ENABLE);
    
    workhead_initialized = true;
}

/**
 * \brief Set workhead state
 */
void workhead_set_state(workhead_state_t state)
{
    workhead.state = state;
    
    // Update LEDs based on state
    switch (state) {
        case WORKHEAD_STATE_IDLE:
            workhead_set_leds(true, false);
            break;
        case WORKHEAD_STATE_WORKING:
            workhead_set_leds(true, false);
            break;
        case WORKHEAD_STATE_ERROR:
            workhead_set_leds(false, true);
            break;
        case WORKHEAD_STATE_MAINTENANCE:
            workhead_set_leds(false, false);
            break;
        case WORKHEAD_STATE_CALIBRATING:
            workhead_set_leds(true, true);
            break;
    }
}

/**
 * \brief Get workhead state
 */
workhead_state_t workhead_get_state(void)
{
    return workhead.state;
}

/**
 * \brief Set workhead position
 */
void workhead_set_position(uint8_t position)
{
    if (position > workhead.max_position) {
        position = workhead.max_position;
    }
    if (position < workhead.min_position) {
        position = workhead.min_position;
    }
    
    workhead.target_position = position;
    
    // Calculate direction
    if (workhead.target_position > workhead.current_position) {
        pio_set_pin_high(PIN_WORKHEAD_DIR); // Forward
    } else {
        pio_set_pin_low(PIN_WORKHEAD_DIR);  // Reverse
    }
}

/**
 * \brief Get current workhead position
 */
uint8_t workhead_get_position(void)
{
    return workhead.current_position;
}

/**
 * \brief Set workhead speed
 */
void workhead_set_speed(uint8_t speed)
{
    if (speed > workhead.max_speed) {
        speed = workhead.max_speed;
    }
    
    workhead.current_speed = speed;
    
    // Calculate step interval based on speed (0-100%)
    // Higher speed = shorter interval
    if (speed > 0) {
        step_interval = 1000 - (speed * 8); // 200-1000 microseconds
        if (step_interval < 200) step_interval = 200;
    } else {
        step_interval = 1000;
    }
}

/**
 * \brief Get current workhead speed
 */
uint8_t workhead_get_speed(void)
{
    return workhead.current_speed;
}

/**
 * \brief Start workhead
 */
void workhead_start(void)
{
    if (workhead.state == WORKHEAD_STATE_ERROR) {
        return; // Cannot start in error state
    }
    
    workhead_set_state(WORKHEAD_STATE_WORKING);
    pio_set_pin_high(PIN_WORKHEAD_ENABLE);
    step_active = true;
}

/**
 * \brief Stop workhead
 */
void workhead_stop(void)
{
    workhead_set_state(WORKHEAD_STATE_IDLE);
    pio_set_pin_low(PIN_WORKHEAD_ENABLE);
    step_active = false;
    workhead.current_speed = 0;
}

/**
 * \brief Emergency stop workhead
 */
void workhead_emergency_stop(void)
{
    workhead_set_state(WORKHEAD_STATE_ERROR);
    pio_set_pin_low(PIN_WORKHEAD_ENABLE);
    step_active = false;
    workhead.current_speed = 0;
    workhead.error_code = 0xFF; // Emergency stop error
}

/**
 * \brief Reset workhead
 */
void workhead_reset(void)
{
    workhead_stop();
    workhead.current_position = 0;
    workhead.target_position = 0;
    workhead.error_code = 0;
    workhead.step_count = 0;
    workhead_set_state(WORKHEAD_STATE_IDLE);
}

/**
 * \brief Calibrate workhead
 */
void workhead_calibrate(void)
{
    workhead_set_state(WORKHEAD_STATE_CALIBRATING);
    
    // Move to minimum position
    pio_set_pin_low(PIN_WORKHEAD_DIR);
    workhead.current_position = 0;
    workhead.step_count = 0;
    
    // Move to maximum position to count steps
    pio_set_pin_high(PIN_WORKHEAD_DIR);
    workhead.target_position = workhead.max_position;
    
    // Calibration will complete when limit switch is hit
}

/**
 * \brief Update workhead status structure
 */
void workhead_update_status(workhead_status_t *status)
{
    status->workhead_id = workhead.workhead_id;
    status->status = (uint8_t)workhead.state;
    status->position = workhead.current_position;
    status->speed = workhead.current_speed;
    status->temperature = workhead_read_temperature();
    status->vibration = workhead_read_vibration();
    status->error_code = workhead.error_code;
    status->reserved = 0;
}

/**
 * \brief Process workhead command
 */
void workhead_process_command(const workhead_command_t *command)
{
    uint8_t result = 0; // 0 = success, 1 = error
    
    switch (command->command_id) {
        case CMD_START:
            workhead_start();
            break;
            
        case CMD_STOP:
            workhead_stop();
            break;
            
        case CMD_SET_POSITION:
            workhead_set_position(command->parameter1);
            break;
            
        case CMD_SET_SPEED:
            workhead_set_speed(command->parameter1);
            break;
            
        case CMD_EMERGENCY_STOP:
            workhead_emergency_stop();
            break;
            
        case CMD_RESET:
            workhead_reset();
            break;
            
        case CMD_GET_STATUS:
            // Status will be sent automatically
            break;
            
        case CMD_CALIBRATE:
            workhead_calibrate();
            break;
            
        default:
            result = 1; // Unknown command
            break;
    }
    
    // Send acknowledgment
    can_send_ack(command->command_id, result);
}

/**
 * \brief Update workhead (call from main loop)
 */
void workhead_update(void)
{
    if (!workhead_initialized) return;
    
    static uint32_t last_update = 0;
    uint32_t current_time = rtt_read_timer_value(RTT);
    
    // Check for step timing
    if (step_active && workhead.current_position != workhead.target_position) {
        if ((current_time - last_update) >= step_interval) {
            // Generate step pulse
            pio_set_pin_high(PIN_WORKHEAD_STEP);
            delay_us(10); // Short pulse
            pio_set_pin_low(PIN_WORKHEAD_STEP);
            
            // Update position
            if (workhead.target_position > workhead.current_position) {
                workhead.current_position++;
            } else if (workhead.target_position < workhead.current_position) {
                workhead.current_position--;
            }
            
            workhead.step_count++;
            last_update = current_time;
        }
    }
    
    // Check limit switches
    if (workhead_read_limit_min() && workhead.current_position > 0) {
        workhead.current_position = 0;
        workhead.target_position = 0;
    }
    
    if (workhead_read_limit_max() && workhead.current_position < workhead.max_position) {
        workhead.current_position = workhead.max_position;
        workhead.target_position = workhead.max_position;
    }
    
    // Check for calibration completion
    if (workhead.state == WORKHEAD_STATE_CALIBRATING) {
        if (workhead_read_limit_max()) {
            workhead.total_steps = workhead.step_count;
            workhead_set_state(WORKHEAD_STATE_IDLE);
        }
    }
}

/**
 * \brief Set LED states
 */
void workhead_set_leds(bool status_led, bool error_led)
{
    if (status_led) {
        pio_set_pin_high(PIN_STATUS_LED);
    } else {
        pio_set_pin_low(PIN_STATUS_LED);
    }
    
    if (error_led) {
        pio_set_pin_high(PIN_ERROR_LED);
    } else {
        pio_set_pin_low(PIN_ERROR_LED);
    }
}

/**
 * \brief Read minimum limit switch
 */
bool workhead_read_limit_min(void)
{
    return !pio_get_pin_value(PIN_WORKHEAD_LIMIT_MIN);
}

/**
 * \brief Read maximum limit switch
 */
bool workhead_read_limit_max(void)
{
    return !pio_get_pin_value(PIN_WORKHEAD_LIMIT_MAX);
}

/**
 * \brief Read sensor 1
 */
bool workhead_read_sensor1(void)
{
    return pio_get_pin_value(PIN_WORKHEAD_SENSOR1);
}

/**
 * \brief Read sensor 2
 */
bool workhead_read_sensor2(void)
{
    return pio_get_pin_value(PIN_WORKHEAD_SENSOR2);
}

/**
 * \brief Read temperature (simulated)
 */
uint8_t workhead_read_temperature(void)
{
    // In a real implementation, this would read from a temperature sensor
    // For now, return a simulated value based on workhead activity
    uint8_t base_temp = 25; // Room temperature
    uint8_t activity_temp = workhead.current_speed / 4; // Heat from activity
    return base_temp + activity_temp;
}

/**
 * \brief Read vibration level (simulated)
 */
uint8_t workhead_read_vibration(void)
{
    // In a real implementation, this would read from a vibration sensor
    // For now, return a simulated value based on workhead activity
    return workhead.current_speed / 2;
}