#include "can_app.h"
#include "asf.h"
#include "can.h"

#include "FreeRTOS.h"
#include "task.h"

// Define TickType_t if not already defined
#ifndef TickType_t
typedef portTickType TickType_t;
#endif

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS portTICK_RATE_MS // Backward compatibility macro
#endif
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_RATE_MS)) // Convert ms to OS ticks
#endif

static void can0_configure_pins_local(void)
{
	pmc_enable_periph_clk(ID_PIOB); // Enable PIOB clock for CAN0 pins
	pio_configure(PIOB, PIO_PERIPH_A, PIO_PB2A_CANTX0, 0); // PB2 as CANTX0
	pio_configure(PIOB, PIO_PERIPH_A, PIO_PB3A_CANRX0, 0); // PB3 as CANRX0
}

/* Force robust 500 kbps timing for PCLK = 96 MHz using 16 TQ and ~81.25% sample point.
 * Segments: Prop=5, Phase1=7, Phase2=3, SJW=2, BRP = 12 (encoded as N-1 in CAN_BR fields).
 * This yields bitrate = 96e6 / (12 * 16) = 500000 bps, SP = (1+5+7)/16 = 81.25%.
 */
static void can_force_500k_16tq_timing(Can *p_can, uint32_t mck)
{
(void)mck; // mck expected 96 MHz per board clocking
/* Disable before modifying CAN_BR (per driver convention). */
can_disable(p_can);
const uint32_t brp_enc= 12 - 1; // prescaler 12
const uint32_t sjw_enc= 2  - 1; // SJW = 2 tq
const uint32_t propag_enc= 5  - 1; // PropSeg = 5 tq
const uint32_t phase1_enc= 7  - 1; // PhaseSeg1 = 7 tq
const uint32_t phase2_enc= 3  - 1; // PhaseSeg2 = 3 tq
p_can->CAN_BR =
CAN_BR_PHASE2(phase2_enc) |
CAN_BR_PHASE1(phase1_enc) |
CAN_BR_PROPAG(propag_enc) |
CAN_BR_SJW(sjw_enc)       |
CAN_BR_BRP(brp_enc)       |
CAN_BR_SMP_ONCE; // single sampling
can_enable(p_can);
}

bool can_app_init(void)
{
	uint32_t mck = sysclk_get_peripheral_hz(); // Peripheral clock frequency
	if (mck == 0) return false; // Invalid clock frequency
	
	// Expected: mck = 96,000,000 Hz for proper 500kbps CAN timing
	
	pmc_enable_periph_clk(ID_CAN0); // Enable CAN0 peripheral clock
	can0_configure_pins_local(); // Route pins to CAN peripheral
	
	// Add delay after pin configuration to ensure stability
	delay_ms(10);
	
	// Initialize CAN controller with proper baudrate constant
	if (!can_init(CAN0, mck, CAN_BPS_500K)) {
		return false; // CAN baudrate configuration failed
	}
	
	
	can_reset_all_mailbox(CAN0); // Reset all mailboxes to known state
	
	/* Configure a generic RX mailbox 0 - ACCEPT ALL MESSAGES for testing */
	can_mb_conf_t mb;
	mb.ul_mb_idx = 0; // Mailbox index 0 for RX
	mb.uc_obj_type = CAN_MB_RX_MODE; // Receive mode
	mb.uc_id_ver = 0; // Standard ID (11-bit)
	mb.ul_id_msk = 0; // NO MASK - Accept all messages (for loopback test compatibility)
	mb.ul_id = 0; // Don't filter on specific ID
	mb.uc_length = 8; // Set MDLC before arming to avoid undefined length
	
	
	can_mailbox_init(CAN0, &mb); // Apply configuration
	// Arm RX mailbox 0 to start receiving
	can_mailbox_send_transfer_cmd(CAN0, &mb);
	
	
	// Initialize TX mailbox 1 for future use
	can_mb_conf_t tx_init;
	tx_init.ul_mb_idx = 1; // Use mailbox 1 for TX
	tx_init.uc_obj_type = CAN_MB_TX_MODE; // Transmit mode
	tx_init.uc_tx_prio = 15; // High priority
	tx_init.uc_id_ver = 0; // Standard ID
	tx_init.ul_id_msk = 0; // Not used for TX
	can_mailbox_init(CAN0, &tx_init); // Configure TX mailbox
	
	
	
	return true;
}

// Function to verify CAN bit rate configuration
bool can_verify_bitrate(uint32_t expected_kbps)
{
	uint32_t mck = sysclk_get_peripheral_hz();
	uint32_t can_br = CAN0->CAN_BR;
	
	// Decode CAN_BR register
	uint32_t phase2 = (can_br & CAN_BR_PHASE2_Msk) >> CAN_BR_PHASE2_Pos;
	uint32_t phase1 = (can_br & CAN_BR_PHASE1_Msk) >> CAN_BR_PHASE1_Pos;
	uint32_t propag = (can_br & CAN_BR_PROPAG_Msk) >> CAN_BR_PROPAG_Pos;
	uint32_t brp = (can_br & CAN_BR_BRP_Msk) >> CAN_BR_BRP_Pos;
	
	// Calculate actual bit rate
	uint32_t total_tq = 1 + (propag + 1) + (phase1 + 1) + (phase2 + 1);
	uint32_t actual_bitrate = mck / ((brp + 1) * total_tq);
	uint32_t expected_bitrate = expected_kbps * 1000;
	
	// Allow 1% tolerance
	uint32_t tolerance = expected_bitrate / 100;
	return (actual_bitrate >= (expected_bitrate - tolerance)) && 
	       (actual_bitrate <= (expected_bitrate + tolerance));
}

bool can_app_tx(uint32_t id, const uint8_t *data, uint8_t len)
{
	// First, reset the TX mailbox to ensure it's in a clean state
	can_mb_conf_t reset_mb;
	reset_mb.ul_mb_idx = 1;
	reset_mb.uc_obj_type = CAN_MB_DISABLE_MODE; // Disable first
	can_mailbox_init(CAN0, &reset_mb);
	
	// Small delay to ensure reset takes effect
	delay_ms(1);
	
	// Now configure the TX mailbox properly. IMPORTANT: set ul_id before mailbox init
	can_mb_conf_t tx;
	tx.ul_mb_idx = 1; // Use mailbox 1 for TX
	tx.uc_obj_type = CAN_MB_TX_MODE; // Transmit mode
	tx.uc_tx_prio = 15; // High priority
	tx.uc_id_ver = 0; // Standard ID
	tx.ul_id_msk = 0; // Not used for TX
	tx.ul_id = CAN_MID_MIDvA(id); // Set CAN identifier BEFORE init (driver may snapshot)
	can_mailbox_init(CAN0, &tx); // Configure mailbox
	uint32_t dl = 0, dh = 0; // Data low/high 32-bit words
	for (uint8_t i=0;i<4 && i<len;i++) dl |= ((uint32_t)data[i]) << (i*8); // Pack first 4 bytes
	for (uint8_t i=4;i<8 && i<len;i++) dh |= ((uint32_t)data[i]) << ((i-4)*8); // Pack next 4 bytes
	tx.ul_datal = dl;
	tx.ul_datah = dh;
	tx.uc_length = len; // DLC
	
	if (can_mailbox_write(CAN0, &tx) != CAN_MAILBOX_TRANSFER_OK) {
		return false; // Load MB failed
	}
	
	can_global_send_transfer_cmd(CAN0, CAN_TCR_MB1); // Trigger transmission
	
	// Wait for transmission to complete
	delay_ms(10);
	
	return true;
}


bool can_app_reset(void)
{
	
	can_disable(CAN0);
	delay_ms(10);
	can_enable(CAN0);
	delay_ms(10);
		can_reset_all_mailbox(CAN0);
	can_mb_conf_t mb;
	mb.ul_mb_idx = 0; // Mailbox index 0 for RX
	mb.uc_obj_type = CAN_MB_RX_MODE; // Receive mode
	mb.uc_id_ver = 0; // Standard ID (11-bit)
	mb.ul_id_msk = CAN_MAM_MIDvA_Msk; // Mask for standard ID only
	mb.ul_id = CAN_MID_MIDvA(CAN_ID_POT_COMMAND); // Filter on command base ID
	can_mailbox_init(CAN0, &mb); // Apply configuration
	// Arm RX mailbox 0 to start receiving after reset
	can_mailbox_send_transfer_cmd(CAN0, &mb);
	return true;	
}
void can_rx_task(void *arg)
{
	(void)arg; // Unused
	uint32_t error_count = 0;
	
	for (;;) {
		// Check CAN controller status first
		if (!can_app_get_status()) {
			error_count++;
			if (error_count > 100) { // After 100 consecutive errors, try to reinitialize
				can_app_reset();// Note: In a real system, you might want to reset the CAN controller here
				error_count = 0;
			}
			vTaskDelay(pdMS_TO_TICKS(100)); // Wait longer on errors
			continue;
		}
		
		error_count = 0; // Reset error count on successful status check
		
		// Check if RX mailbox has data
		uint32_t mb_status = can_mailbox_get_status(CAN0, 0);
		if (mb_status & CAN_MSR_MRDY) { // If RX mailbox has data
			can_mb_conf_t rx;
			rx.ul_mb_idx = 0;
			
			// Read mailbox with error checking
			if (can_mailbox_read(CAN0, &rx) == CAN_MAILBOX_TRANSFER_OK) {
				uint8_t len = (uint8_t)(rx.ul_status >> CAN_MSR_MDLC_Pos) & 0xF; // Extract DLC
				if (len > 8) len = 8; // Sanity check on length
				
				uint8_t data[8] = {0}; // Initialize to zero
				// Unpack data words
				for (uint8_t i = 0; i < 4 && i < len; i++) {
					data[i] = (rx.ul_datal >> (i * 8)) & 0xFF;
				}
				for (uint8_t i = 0; i < 4 && (i + 4) < len; i++) {
					data[4 + i] = (rx.ul_datah >> (i * 8)) & 0xFF;
				}
				
				// Extract CAN ID and process based on message type
				uint32_t can_id = rx.ul_fid;//(rx.ul_id >> CAN_MID_MIDvA_Pos) & 0x7FFu;
// 				if (can_id == CAN_ID_POT_COMMAND) {
// 					handle_pot_command(data, len); // Execute command
// 				}
				// Note: Other message IDs are received but not processed in this task
				// This allows loopback test (ID 0x123) to be received successfully
			}
		}
		
		vTaskDelay(pdMS_TO_TICKS(5)); // Small delay to yield CPU time
	}
}
bool can_app_get_status(void){
	// Check if CAN controller is properly initialized and running
	if (CAN0->CAN_SR & CAN_SR_BOFF) {
		return false; // CAN controller is in bus-off state (bad)
	}
	
	// Check if there are any error conditions
	uint32_t status = CAN0->CAN_SR;
	if (status & (CAN_SR_ERRA | CAN_SR_WARN | CAN_SR_BOFF)) {
		return false; // Error conditions present
	}
	
	// Check if CAN controller is enabled
	if (!(CAN0->CAN_MR & CAN_MR_CANEN)) {
		return false; // CAN controller not enabled
	}
	
	return true; // CAN is working properly
}

bool can_app_test_loopback(void)
{
	
	// For SAM4E, implement loopback by configuring two mailboxes:
	// - MB1 for TX
	// - MB2 for RX (configured to receive the test message)
	
	// First, configure a dedicated RX mailbox for loopback test
	can_mb_conf_t rx_mb;
	rx_mb.ul_mb_idx = 2; // Use mailbox 2 for loopback RX
	rx_mb.uc_obj_type = CAN_MB_RX_MODE; // Receive mode
	rx_mb.uc_id_ver = 0; // Standard ID (11-bit)
	rx_mb.ul_id_msk = CAN_MAM_MIDvA_Msk; // Match exact ID
	rx_mb.ul_id = CAN_MID_MIDvA(0x100); // Accept only test ID 0x100
	can_mailbox_init(CAN0, &rx_mb); // Configure mailbox
	// Arm RX mailbox 2 to start receiving
	can_mailbox_send_transfer_cmd(CAN0, &rx_mb);
	
	// Reset and configure TX mailbox
	can_mb_conf_t reset_tx;
	reset_tx.ul_mb_idx = 1;
	reset_tx.uc_obj_type = CAN_MB_DISABLE_MODE; // Disable first
	can_mailbox_init(CAN0, &reset_tx);
	delay_ms(1);
	
	// Configure TX mailbox
	can_mb_conf_t tx_mb;
	tx_mb.ul_mb_idx = 1; // Use mailbox 1 for TX
	tx_mb.uc_obj_type = CAN_MB_TX_MODE; // Transmit mode
	tx_mb.uc_tx_prio = 15; // High priority
	tx_mb.uc_id_ver = 0; // Standard ID
	tx_mb.ul_id_msk = 0; // Not used for TX
	// IMPORTANT: set ID before initializing mailbox so driver snapshots it correctly
	tx_mb.ul_id = CAN_MID_MIDvA(0x100);
	can_mailbox_init(CAN0, &tx_mb); // Configure mailbox
	
	// Wait for configuration to take effect
	delay_ms(10);
	
	// Check if TX mailbox is ready
	uint32_t tx_mb_status = can_mailbox_get_status(CAN0, 1);
	if (!(tx_mb_status & CAN_MSR_MRDY)) {
		return false;
	}
	
	// Test transmission
	uint8_t test_data[4] = {0xAA, 0x55, 0x12, 0x34};
	uint32_t test_id = 0x100; // Test ID that matches RX filter
	
	// Send test message using the TX mailbox
	// Ensure mailbox's ID is set (already set above) and data prepared
	uint32_t dl = 0, dh = 0; // Data low/high 32-bit words
	for (uint8_t i=0;i<4;i++) dl |= ((uint32_t)test_data[i]) << (i*8); // Pack data
	tx_mb.ul_datal = dl;
	tx_mb.ul_datah = dh;
	tx_mb.uc_length = 4; // DLC
	
	if (can_mailbox_write(CAN0, &tx_mb) != CAN_MAILBOX_TRANSFER_OK) {
		return false; // Transmission failed
	}
	
	// Trigger transmission
	can_global_send_transfer_cmd(CAN0, CAN_TCR_MB1); // Send from MB1
	
	
	// Wait for message to be transmitted and looped back
	// In a real CAN network, the TX message should be seen by all nodes including self
	delay_ms(50); // Increased delay for message propagation
	
	// Check if message was received in mailbox 2
	uint32_t mb_status = can_mailbox_get_status(CAN0, 2); // Check MB2
	
	if (mb_status & CAN_MSR_MRDY) {
		can_mb_conf_t rx;
		rx.ul_mb_idx = 2; // Read from MB2
		if (can_mailbox_read(CAN0, &rx) == CAN_MAILBOX_TRANSFER_OK) {
			// Verify the received data matches what we sent
			uint32_t received_id = (rx.ul_id >> CAN_MID_MIDvA_Pos) & 0x7FFu;
			
			// Verify ID and data match
			if (received_id == test_id && rx.ul_datal == dl) {
				// Restore original mailbox configuration
				// Reconfigure MB2 back to unused state
				rx_mb.ul_mb_idx = 2;
				rx_mb.uc_obj_type = CAN_MB_RX_MODE;
				rx_mb.uc_id_ver = 0;
				rx_mb.ul_id_msk = 0; // Accept all
				rx_mb.ul_id = 0;
				can_mailbox_init(CAN0, &rx_mb);
				// Re-arm RX mailbox 2
				can_mailbox_send_transfer_cmd(CAN0, &rx_mb);
				
				return true; // Test passed
			}
		}
	}
	
	// Restore mailbox configuration
	rx_mb.ul_mb_idx = 2;
	rx_mb.uc_obj_type = CAN_MB_RX_MODE;
	rx_mb.uc_id_ver = 0;
	rx_mb.ul_id_msk = 0; // Accept all
	rx_mb.ul_id = 0;
	can_mailbox_init(CAN0, &rx_mb);
	// Re-arm RX mailbox 2 after restore
	can_mailbox_send_transfer_cmd(CAN0, &rx_mb);
	
	return false; // Test failed
}


void can_status_task(void *arg)
{
	(void)arg; // Unused
	uint32_t status_report_interval = 0;
	
	for (;;) {
		// Check CAN status periodically
		bool can_ok = can_app_get_status();
		
		
		// Report status every 10 seconds (10000ms / 1000ms = 10 iterations)
		status_report_interval++;
		if (status_report_interval >= 10) {
			status_report_interval = 0;
			
			// Send status message
			uint8_t status_data[2] = {0};
			status_data[0] = can_ok ? 0x01 : 0x00; // Status byte
			status_data[1] = 0x00; // Reserved
			
			// Use the dedicated status ID
			can_app_tx(CAN_ID_STATUS, status_data, 2);
		}
		
		vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
	}
}

// Simple CAN test function to help diagnose Protocol Violation issues
bool can_app_simple_test(void)
{
	// Check if CAN controller is in a good state
	uint32_t can_sr = CAN0->CAN_SR;
	
	// Check for bus-off condition
	if (can_sr & CAN_SR_BOFF) {
		return false;
	}
	
	// Check for error active state
	if (can_sr & CAN_SR_ERRA) {
		return false;
	}
	
	// Check if CAN controller is enabled
	if (!(CAN0->CAN_MR & CAN_MR_CANEN)) {
		return false;
	}
	
	return true;
}