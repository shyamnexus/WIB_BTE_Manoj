#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void task_loadcell(void *arg); // Task that reads load cell via ADS1120 and transmits over CAN
void task_temperature(void *arg); // Task that reads temperature from LIS2DH sensor and transmits
void task_accelerometer(void *arg); // Task that reads LIS2DH over I2C and transmits
void task_accelerometer_temperature(void *arg); // Combined task that reads both accelerometer and temperature from LIS2DH
void task_tooltype(void *arg); // Task that samples tool type GPIO and transmits
void task_test(void *arg); // Test task for load cell simulation
void task_sensor_diagnostics(void *arg); // Task for sensor health monitoring and diagnostics

void create_application_tasks(void); // Creates all application tasks and required primitives

#ifdef __cplusplus
}
#endif