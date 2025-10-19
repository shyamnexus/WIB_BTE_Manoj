/*
 * can_command_handler.c
 *
 * Created: 10/19/2025 12:31:19 AM
 *  Author: MKumar
 */ 
// can_command_handler.c
#include "can_command_handler.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <string.h>

/* Configuration */
#ifndef CAN_CMD_TABLE_SIZE
#define CAN_CMD_TABLE_SIZE 16
#endif

#ifndef TickType_t
typedef portTickType TickType_t; // Backward-compatible alias if TickType_t isn't defined
#endif
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_PERIOD_MS)) // Convert milliseconds to OS ticks
#endif

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS portTICK_RATE_MS // Legacy macro mapping
#endif
typedef struct {
    uint8_t cmd_id;
    can_cmd_handler_t handler;
    bool used;
} can_cmd_entry_t;

/* Static table + mutex for safe registration from different tasks */
static can_cmd_entry_t cmd_table[CAN_CMD_TABLE_SIZE];
static xSemaphoreHandle cmd_table_mutex;

/* Forward declarations for example handlers (you can move these to your app) */
static can_cmd_status_t cmd_ping(const uint8_t *payload, uint8_t payload_len, uint32_t src_can_id);
static can_cmd_status_t cmd_echo(const uint8_t *payload, uint8_t payload_len, uint32_t src_can_id);
static can_cmd_status_t cmd_set_led(const uint8_t *payload, uint8_t payload_len, uint32_t src_can_id);

/* Initialize table (call once) */
void CAN_CommandInit(void)
{
    memset(cmd_table, 0, sizeof(cmd_table));
    if (cmd_table_mutex == NULL) {
        cmd_table_mutex = xSemaphoreCreateMutex();
    }

    /* Register built-in/example handlers */
    CAN_RegisterCommand(0x01, cmd_ping);    // ping
    CAN_RegisterCommand(0x02, cmd_echo);    // echo payload back
    CAN_RegisterCommand(0x10, cmd_set_led); // hypothetical set-led
}

/* Register handler: thread-safe */
bool CAN_RegisterCommand(uint8_t cmd_id, can_cmd_handler_t handler)
{
    if (handler == NULL) return false;
    if (cmd_table_mutex == NULL) return false;

    if (xSemaphoreTake(cmd_table_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return false; // couldn't take mutex
    }

    /* If cmd exists, replace; otherwise find free slot */
    int free_idx = -1;
    for (int i = 0; i < CAN_CMD_TABLE_SIZE; ++i) {
        if (cmd_table[i].used && cmd_table[i].cmd_id == cmd_id) {
            cmd_table[i].handler = handler;
            xSemaphoreGive(cmd_table_mutex);
            return true;
        }
        if (!cmd_table[i].used && free_idx == -1) {
            free_idx = i;
        }
    }

    if (free_idx >= 0) {
        cmd_table[free_idx].used = true;
        cmd_table[free_idx].cmd_id = cmd_id;
        cmd_table[free_idx].handler = handler;
        xSemaphoreGive(cmd_table_mutex);
        return true;
    }

    xSemaphoreGive(cmd_table_mutex);
    return false; // no space
}

bool CAN_UnregisterCommand(uint8_t cmd_id)
{
    if (cmd_table_mutex == NULL) return false;
    if (xSemaphoreTake(cmd_table_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;

    for (int i = 0; i < CAN_CMD_TABLE_SIZE; ++i) {
        if (cmd_table[i].used && cmd_table[i].cmd_id == cmd_id) {
            cmd_table[i].used = false;
            cmd_table[i].handler = NULL;
            cmd_table[i].cmd_id = 0;
            xSemaphoreGive(cmd_table_mutex);
            return true;
        }
    }

    xSemaphoreGive(cmd_table_mutex);
    return false;
}

/* Dispatch function */
can_cmd_status_t CAN_CommandHandler(const uint8_t *data, uint8_t len, uint32_t src_can_id)
{
    if (data == NULL || len == 0) return CAN_CMD_ERR_INVALID_PARAM;

    uint8_t cmd = data[0];
    const uint8_t *payload = (len > 1) ? &data[1] : NULL;
    uint8_t payload_len = (len > 0) ? (len - 1) : 0;

    /* Look up handler */
    if (cmd_table_mutex == NULL) return CAN_CMD_ERR_INTERNAL;
    if (xSemaphoreTake(cmd_table_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return CAN_CMD_ERR_INTERNAL;
    }

    can_cmd_handler_t handler = NULL;
    for (int i = 0; i < CAN_CMD_TABLE_SIZE; ++i) {
        if (cmd_table[i].used && cmd_table[i].cmd_id == cmd) {
            handler = cmd_table[i].handler;
            break;
        }
    }
    xSemaphoreGive(cmd_table_mutex);

    if (handler == NULL) {
        /* Unknown command */
        return CAN_CMD_ERR_NOT_FOUND;
    }

    /* Call handler synchronously. Handlers should be quick; if not, they can queue work to another task. */
    return handler(payload, payload_len, src_can_id);
}

/* -----------------------
   Example handlers below
   ----------------------- */

/* cmd 0x01: ping
 * payload: optional (ignored)
 * returns OK
 * (In a real system you'd send a response CAN frame)
 */
static can_cmd_status_t cmd_ping(const uint8_t *payload, uint8_t payload_len, uint32_t src_can_id)
{
    (void)payload; (void)payload_len; (void)src_can_id;
    // TODO: send a reply frame using your CAN TX API indicating alive
    // send_can_response(src_can_id, 0x01, NULL, 0);
    return CAN_CMD_OK;
}

/* cmd 0x02: echo
 * payload: any bytes
 * behaviour: echo payload back to source
 */
static can_cmd_status_t cmd_echo(const uint8_t *payload, uint8_t payload_len, uint32_t src_can_id)
{
    // Example: call your send function: send_can_response(src_can_id, 0x02, payload, payload_len);
    (void)src_can_id;
    if (payload == NULL && payload_len > 0) return CAN_CMD_ERR_INVALID_PARAM;
    // send back...
    return CAN_CMD_OK;
}

/* cmd 0x10: set-led (example)
 * payload: [0] = led_index, [1] = value (0 or 1)
 */
static can_cmd_status_t cmd_set_led(const uint8_t *payload, uint8_t payload_len, uint32_t src_can_id)
{
    (void)src_can_id;
    if (payload_len < 2) return CAN_CMD_ERR_INVALID_PARAM;
    uint8_t led_idx = payload[0];
    uint8_t value = payload[1];
    // call your board-specific API e.g. board_set_led(led_idx, value);
    return CAN_CMD_OK;
}
