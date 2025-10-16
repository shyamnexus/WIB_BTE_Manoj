// sensor_test.h - Sensor testing and validation functions
#ifndef SENSOR_TEST_H
#define SENSOR_TEST_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Test function prototypes
bool sensor_test_lis2dh_connection(void);
bool sensor_test_accelerometer_reading(void);
bool sensor_test_temperature_reading(void);
void sensor_test_print_diagnostics(void);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_TEST_H