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

	// Default MC3419 I2C address: 0x19 (SA0=1). Set to 0x18 if SA0=0.
	#ifndef MC3419_ADDR
	#define MC3419_ADDR 0x19u
	#endif

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

	#ifdef __cplusplus
}
#endif

#endif /* MC3419_H */