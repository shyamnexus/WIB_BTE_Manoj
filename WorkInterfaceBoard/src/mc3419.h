/*
 * MC3419.h
 *
 * Created: 10/19/2025 7:40:02 PM
 *  Author: MKumar
 */ 
#ifndef MC3419_H
#define MC3419_H
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
	#endif

	// Default MC3419 I2C address: 0x4C (7-bit address)
	#ifndef MC3419_ADDR
	#define MC3419_ADDR 0x4Cu
	#endif

	// MC3419 Register definitions
	#define MC3419_REG_WHO_AM_I        0x0F
	#define MC3419_REG_STATUS          0x00
	#define MC3419_REG_XOUT_LSB        0x0D
	#define MC3419_REG_XOUT_MSB        0x0E
	#define MC3419_REG_YOUT_LSB        0x0B
	#define MC3419_REG_YOUT_MSB        0x0C
	#define MC3419_REG_ZOUT_LSB        0x09
	#define MC3419_REG_ZOUT_MSB        0x0A
	#define MC3419_REG_TEMP_LSB        0x07
	#define MC3419_REG_TEMP_MSB        0x08
	#define MC3419_REG_MODE            0x05
	#define MC3419_REG_SAMPLE_RATE     0x06
	#define MC3419_REG_RANGE           0x20

	// MC3419 Mode definitions
	#define MC3419_MODE_STANDBY        0x00
	#define MC3419_MODE_WAKE           0x01
	#define MC3419_MODE_SLEEP          0x02

	// MC3419 Range definitions
	#define MC3419_RANGE_2G            0x00
	#define MC3419_RANGE_4G            0x01
	#define MC3419_RANGE_8G            0x02
	#define MC3419_RANGE_16G           0x03

	// MC3419 Sample Rate definitions
	#define MC3419_SAMPLE_RATE_1HZ     0x00
	#define MC3419_SAMPLE_RATE_10HZ    0x01
	#define MC3419_SAMPLE_RATE_25HZ    0x02
	#define MC3419_SAMPLE_RATE_50HZ    0x03
	#define MC3419_SAMPLE_RATE_100HZ   0x04
	#define MC3419_SAMPLE_RATE_200HZ   0x05
	#define MC3419_SAMPLE_RATE_400HZ   0x06
	#define MC3419_SAMPLE_RATE_800HZ   0x07

	// Data structure for MC3419 sensor data
	typedef struct {
		int16_t x;          // X-axis acceleration (raw)
		int16_t y;          // Y-axis acceleration (raw)
		int16_t z;          // Z-axis acceleration (raw)
		int16_t temp;       // Temperature (raw)
		bool valid;         // Data validity flag
	} mc3419_data_t;

	// Initialize TWI0 on PA3(SDA)/PA4(SCL).
	// mck_hz: master clock in Hz (e.g., 120000000).
	// i2c_hz: desired I2C speed (e.g., 100000 or 400000).
	bool MC3419_i2c_init(uint32_t mck_hz, uint32_t i2c_hz); // Configure and enable TWI0 master

	// Check if TWI is properly initialized and ready
	bool MC3419_i2c_is_ready(void); // Check TWI initialization status

	// Get TWI status register for debugging
	uint32_t MC3419_i2c_get_status(void); // Get TWI status register value

	// Write 'len' bytes starting at register 'reg'.
	bool MC3419_i2c_write(uint8_t reg, const uint8_t *data, uint32_t len); // Register-write helper

	// Read 'len' bytes starting at register 'reg' into 'buf'.
	bool MC3419_i2c_read(uint8_t reg, uint8_t *buf, uint32_t len); // Register-read helper

	// Convenience: read WHO_AM_I (should be 0x33 on many MC3419 parts).
	bool MC3419_whoami(uint8_t *out_who); // Read device ID into out_who

	// Generic I2C functions that accept device address parameter
	bool i2c_write_bytes(uint8_t addr7, const uint8_t *data, uint32_t len); // Write bytes to any device
	bool i2c_write_then_read(uint8_t addr7, const uint8_t *w, uint32_t wn, uint8_t *r, uint32_t rn); // Write then read from any device

	// High-level MC3419 functions
	bool mc3419_init(void); // Initialize MC3419 sensor
	bool mc3419_read_data(mc3419_data_t *data); // Read accelerometer and temperature data
	bool mc3419_check_status(void); // Check if sensor is responding
	float mc3419_convert_accel_to_g(int16_t raw, uint8_t range); // Convert raw acceleration to g-force
	float mc3419_convert_temp_to_celsius(int16_t raw); // Convert raw temperature to Celsius

	#ifdef __cplusplus
}
#endif

#endif /* MC3419_H */