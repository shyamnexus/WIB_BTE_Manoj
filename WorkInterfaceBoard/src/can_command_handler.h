/*
 * can_command_handler.h
 *
 * Created: 10/19/2025 12:30:30 AM
 *  Author: MKumar
 */ 

#ifndef CAN_COMMAND_HANDLER_H
#define CAN_COMMAND_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CAN_CMD_OK = 0,
    CAN_CMD_ERR_NOT_FOUND,
    CAN_CMD_ERR_INVALID_PARAM,
    CAN_CMD_ERR_INTERNAL,
} can_cmd_status_t;

/* Handler signature:
 * payload -> pointer to payload (data[1] .. data[len-1])
 * payload_len -> number of payload bytes (len - 1)
 * src_can_id -> source CAN ID (for replies or context)
 * return -> status code (optional)
 */
typedef can_cmd_status_t (*can_cmd_handler_t)(const uint8_t *payload, uint8_t payload_len, uint32_t src_can_id);

/* Initialize command subsystem (call once at startup) */
void CAN_CommandInit(void);

/* Main dispatch function to call from your RX task */
can_cmd_status_t CAN_CommandHandler(const uint8_t *data, uint8_t len, uint32_t src_can_id);

/* Register a handler at runtime. If same cmd exists, it will be replaced.
 * Returns true on success, false on failure (e.g., no slot).
 */
bool CAN_RegisterCommand(uint8_t cmd_id, can_cmd_handler_t handler);

/* Unregister a command. Returns true if found and removed. */
bool CAN_UnregisterCommand(uint8_t cmd_id);

#endif // CAN_COMMAND_HANDLER_H
