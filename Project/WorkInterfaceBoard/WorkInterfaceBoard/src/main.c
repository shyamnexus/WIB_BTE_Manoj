/**
 * \file main.c
 *
 * \brief Workhead Interface Board Main Application
 *
 * This is the main application for the workhead interface board.
 * It provides CAN communication for sending workhead status and receiving commands.
 */

#include <asf.h>
#include "workhead_can.h"
#include "workhead_control.h"

// Application configuration
#define STATUS_UPDATE_INTERVAL_MS    100     // Update status every 100ms
#define COMMAND_TIMEOUT_MS           1000    // Command timeout

// Global variables
static uint32_t last_status_update = 0;
static uint32_t last_command_time = 0;
static workhead_command_t received_command;

/**
 * \brief Main application function
 */
int main(void)
{
    /* System clock is automatically initialized in SystemInit() during startup */
    /* Clock configuration is handled in system_sam4e.c for 16MHz crystal */
    
    // Initialize board
    board_init();
    
    // Initialize RTT for timing
    rtt_init(RTT, 32768); // 32.768 kHz RTT clock
    
    // Initialize workhead control
    workhead_init();
    
    // Initialize CAN communication
    workhead_can_init();
    
    // Send startup message
    workhead_status_t status;
    workhead_update_status(&status);
    can_send_status(&status);
    
    // Main application loop
    while (1) {
        uint32_t current_time = rtt_read_timer_value(RTT);
        
        // Process CAN messages
        can_process_messages();
        
        // Check for received commands
        if (can_receive_command(&received_command)) {
            last_command_time = current_time;
            workhead_process_command(&received_command);
        }
        
        // Update workhead control
        workhead_update();
        
        // Send status update periodically
        if ((current_time - last_status_update) >= STATUS_UPDATE_INTERVAL_MS) {
            workhead_update_status(&status);
            can_send_status(&status);
            last_status_update = current_time;
        }
        
        // Check for command timeout
        if (workhead_get_state() == WORKHEAD_STATE_WORKING) {
            if ((current_time - last_command_time) >= COMMAND_TIMEOUT_MS) {
                // No command received for timeout period, stop workhead
                workhead_stop();
                can_send_error(0x02); // Timeout error
            }
        }
        
        // Small delay to prevent excessive CPU usage
        delay_ms(1);
    }
}
