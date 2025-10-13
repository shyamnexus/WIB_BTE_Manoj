/**
 * \file workhead_can.c
 *
 * \brief Workhead Interface Board CAN Communication Implementation
 */

#include "workhead_can.h"

// Global variables
static workhead_status_t current_status = {0};
static workhead_command_t received_command = {0};
static bool command_received = false;
static bool can_initialized = false;

// CAN mailbox configuration
#define TX_MAILBOX     0
#define RX_MAILBOX     1

/**
 * \brief Initialize CAN controller
 */
void can_init(void)
{
    // Enable CAN0 peripheral clock
    pmc_enable_periph_clk(ID_CAN0);
    
    // Configure CAN pins (PA1 = CAN0_RX, PA0 = CAN0_TX)
    pio_configure_pin(PIO_PA0_IDX, PIO_PERIPH_A | PIO_DEFAULT);
    pio_configure_pin(PIO_PA1_IDX, PIO_PERIPH_A | PIO_DEFAULT);
    
    // Reset CAN controller
    can_reset(CAN0);
    
    // Configure CAN controller
    can_mailbox_init_t mailbox_init;
    mailbox_init.ul_mb_idx = 0;
    mailbox_init.ul_mb_priority = 0;
    mailbox_init.ul_mb_mode = CAN_MB_MODE_DISABLED;
    mailbox_init.ul_id_msk = 0;
    mailbox_init.ul_id = 0;
    mailbox_init.ul_fid = 0;
    mailbox_init.ul_fid_mask = 0;
    
    // Initialize all mailboxes as disabled
    for (uint8_t i = 0; i < 8; i++) {
        mailbox_init.ul_mb_idx = i;
        can_mailbox_init(CAN0, &mailbox_init);
    }
    
    // Configure TX mailbox (mailbox 0)
    mailbox_init.ul_mb_idx = TX_MAILBOX;
    mailbox_init.ul_mb_mode = CAN_MB_MODE_TX;
    mailbox_init.ul_id = CAN_MSG_STATUS;
    can_mailbox_init(CAN0, &mailbox_init);
    
    // Configure RX mailbox (mailbox 1)
    mailbox_init.ul_mb_idx = RX_MAILBOX;
    mailbox_init.ul_mb_mode = CAN_MB_MODE_RX;
    mailbox_init.ul_id = CAN_MSG_COMMAND;
    mailbox_init.ul_id_msk = 0x7FF; // Accept all 11-bit IDs
    can_mailbox_init(CAN0, &mailbox_init);
    
    // Set CAN baudrate to 500 kbps
    can_set_baudrate(CAN0, CAN_BAUDRATE, 16000000); // 16 MHz system clock
    
    // Enable CAN controller
    can_enable(CAN0);
    
    // Enable CAN interrupts
    can_enable_interrupt(CAN0, CAN_IER_MB1); // Enable mailbox 1 interrupt for RX
    NVIC_EnableIRQ(CAN0_IRQn);
    
    can_initialized = true;
}

/**
 * \brief Send workhead status message
 */
void can_send_status(const workhead_status_t *status)
{
    if (!can_initialized) return;
    
    // Check if mailbox is ready
    if (!can_mailbox_is_ready(CAN0, TX_MAILBOX)) {
        return;
    }
    
    // Prepare message data
    uint8_t data[8];
    data[0] = status->workhead_id;
    data[1] = status->status;
    data[2] = status->position;
    data[3] = status->speed;
    data[4] = status->temperature;
    data[5] = status->vibration;
    data[6] = status->error_code;
    data[7] = status->reserved;
    
    // Set message ID and data
    can_mailbox_set_id(CAN0, TX_MAILBOX, CAN_MSG_STATUS, 0, 0);
    can_mailbox_set_data(CAN0, TX_MAILBOX, data, 8);
    
    // Send message
    can_global_send_transfer_cmd(CAN0, CAN_TCR_MB0);
}

/**
 * \brief Send acknowledgment message
 */
void can_send_ack(uint8_t command_id, uint8_t result)
{
    if (!can_initialized) return;
    
    // Check if mailbox is ready
    if (!can_mailbox_is_ready(CAN0, TX_MAILBOX)) {
        return;
    }
    
    // Prepare acknowledgment data
    uint8_t data[8];
    data[0] = CAN_NODE_ID;
    data[1] = command_id;
    data[2] = result;
    data[3] = 0;
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    
    // Set message ID and data
    can_mailbox_set_id(CAN0, TX_MAILBOX, CAN_MSG_ACK, 0, 0);
    can_mailbox_set_data(CAN0, TX_MAILBOX, data, 8);
    
    // Send message
    can_global_send_transfer_cmd(CAN0, CAN_TCR_MB0);
}

/**
 * \brief Send error message
 */
void can_send_error(uint8_t error_code)
{
    if (!can_initialized) return;
    
    // Check if mailbox is ready
    if (!can_mailbox_is_ready(CAN0, TX_MAILBOX)) {
        return;
    }
    
    // Prepare error data
    uint8_t data[8];
    data[0] = CAN_NODE_ID;
    data[1] = error_code;
    data[2] = 0;
    data[3] = 0;
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    
    // Set message ID and data
    can_mailbox_set_id(CAN0, TX_MAILBOX, CAN_MSG_ERROR, 0, 0);
    can_mailbox_set_data(CAN0, TX_MAILBOX, data, 8);
    
    // Send message
    can_global_send_transfer_cmd(CAN0, CAN_TCR_MB0);
}

/**
 * \brief Check for received command
 */
bool can_receive_command(workhead_command_t *command)
{
    if (command_received) {
        *command = received_command;
        command_received = false;
        return true;
    }
    return false;
}

/**
 * \brief Process CAN messages
 */
void can_process_messages(void)
{
    // This function is called from main loop to process any pending messages
    // The actual message processing is done in the interrupt handler
}

/**
 * \brief Calculate checksum for command validation
 */
uint8_t calculate_checksum(const uint8_t *data, uint8_t length)
{
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < length - 1; i++) { // Exclude checksum byte
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * \brief CAN interrupt handler
 */
void CAN0_Handler(void)
{
    uint32_t status = can_get_status(CAN0);
    
    // Check for received message in mailbox 1
    if (status & CAN_SR_MB1) {
        // Read received data
        uint8_t data[8];
        can_mailbox_get_data(CAN0, RX_MAILBOX, data, 8);
        
        // Parse command
        received_command.command_id = data[0];
        received_command.workhead_id = data[1];
        received_command.parameter1 = data[2];
        received_command.parameter2 = data[3];
        received_command.parameter3 = data[4];
        received_command.parameter4 = data[5];
        received_command.checksum = data[6];
        received_command.reserved = data[7];
        
        // Validate checksum
        uint8_t calculated_checksum = calculate_checksum(data, 7);
        if (calculated_checksum == received_command.checksum) {
            command_received = true;
        } else {
            // Send error for invalid checksum
            can_send_error(0x01); // Invalid checksum error
        }
        
        // Clear mailbox status
        can_global_send_transfer_cmd(CAN0, CAN_TCR_MB1);
    }
}