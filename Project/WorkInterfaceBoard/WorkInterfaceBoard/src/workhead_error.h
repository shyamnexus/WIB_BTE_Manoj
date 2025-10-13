/**
 * \file workhead_error.h
 *
 * \brief Workhead Error Handling Module
 *
 * This module provides comprehensive error handling and recovery functionality.
 */

#ifndef WORKHEAD_ERROR_H
#define WORKHEAD_ERROR_H

#include <asf.h>
#include "workhead_config.h"

// Error severity levels
typedef enum {
    ERROR_SEVERITY_INFO = 0,
    ERROR_SEVERITY_WARNING,
    ERROR_SEVERITY_ERROR,
    ERROR_SEVERITY_CRITICAL
} error_severity_t;

// Error structure
typedef struct {
    uint8_t error_code;
    error_severity_t severity;
    uint32_t timestamp;
    uint8_t retry_count;
    bool active;
    char description[32];
} workhead_error_t;

// Error log structure
typedef struct {
    workhead_error_t errors[16];
    uint8_t error_count;
    uint8_t current_index;
} error_log_t;

// Function prototypes
void error_init(void);
void error_set(uint8_t error_code, error_severity_t severity);
void error_clear(uint8_t error_code);
void error_clear_all(void);
bool error_is_active(uint8_t error_code);
uint8_t error_get_count(void);
workhead_error_t* error_get_last(void);
void error_log_add(uint8_t error_code, error_severity_t severity, const char* description);
void error_process_recovery(void);
void error_send_status(void);
const char* error_get_description(uint8_t error_code);
error_severity_t error_get_severity(uint8_t error_code);

#endif // WORKHEAD_ERROR_H