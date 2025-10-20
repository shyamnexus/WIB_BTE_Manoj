/*
 * MC3419.c
 *
 * Created: 10/19/2025 7:39:38 PM
 *  Author: MKumar
 */ 
#include "MC3419.h"
#include "sam4e.h"

// --- Pin mapping for SAM4E: TWI0 on PA3 (TWD0/SDA) and PA4 (TWCK0/SCL), Peripheral A
#define TWI0_SDA_PIN   (1u << 3)  // PA3
#define TWI0_SCL_PIN   (1u << 4)  // PA4
#define TWI0_PINS      (TWI0_SDA_PIN | TWI0_SCL_PIN)

static inline void twi0_configure_pins_periphA(void)
{
	// Enable PIOA clock (for pin configuration)
	PMC->PMC_PCER0 = (1u << ID_PIOA);

	// Enable pull-ups (if you also have external pull-ups that's fine)
	PIOA->PIO_PUER = TWI0_PINS;

	// Enable multi-drive (open-drain) on SDA/SCL
	PIOA->PIO_MDER = TWI0_PINS;

	// Select Peripheral A for PA3/PA4
	// Some header sets expose A/B via PIO_ABSR (0=A, 1=B).
	// Others expose A/B/C/D via PIO_ABCDSR[2] (00=A, 01=B, 10=C, 11=D).
	// Some minimal headers expose neither; in that case, assume Peripheral A default.
	#if defined(PIOA) && defined(PIO_ABSR)
	// Clear bits => Peripheral A
	PIOA->PIO_ABSR &= ~TWI0_PINS;
	#elif defined(PIOA) && defined(PIO_ABCDSR) && defined(PIO_ABCDSR__NUM)
	// Clear both selects to 0 (Peripheral A)
	// Some device headers define PIO_ABCDSR as array; others as two registers PIO_ABCDSR[0], [1].
	PIOA->PIO_ABCDSR[0] &= ~TWI0_PINS;
	PIOA->PIO_ABCDSR[1] &= ~TWI0_PINS;
	#elif defined(PIOA) && defined(PIO_ABCDSR)
	// Treat as array by default
	((volatile uint32_t*)PIOA->PIO_ABCDSR)[0] &= ~TWI0_PINS;
	((volatile uint32_t*)PIOA->PIO_ABCDSR)[1] &= ~TWI0_PINS;
	#else
	// No peripheral select registers visible; most SAM parts default to Peripheral A
	// Nothing to do here.
	#endif

	// Disable PIO control so peripheral takes over
	PIOA->PIO_PDR = TWI0_PINS;

	// Optional: disable internal glitch/debounce on these pins
	PIOA->PIO_IFDR = TWI0_PINS;
}

static inline void twi0_reset(void)
{
	TWI0->TWI_CR = TWI_CR_SWRST; // Software reset of TWI0
	(void)TWI0->TWI_RHR; // dummy read
}

static bool twi0_set_speed(uint32_t mck_hz, uint32_t i2c_hz)
{
	if (i2c_hz == 0 || mck_hz == 0) return false; // Validate parameters

	// Debug variables
	volatile uint32_t debug_cldiv_calc;
	volatile uint32_t debug_ckdiv_final;
	volatile uint32_t debug_cldiv_final;

	// Use standard formula: CLDIV=CHDIV=((mck/(2*I2C*2)) - 3) / (2^CKDIV)
	// We'll solve for CKDIV so that CLDIV fits in 8 bits.
	uint32_t ckdiv = 0;
	uint32_t cldiv;

	// We use a simpler common formula from ASF examples:
	// CLDIV = CHDIV = (mck / (2 * i2c_hz)) - 3 ; then scale by CKDIV until <=255
	cldiv = (mck_hz / (2u * i2c_hz)) - 3u;
	debug_cldiv_calc = cldiv;
	
	while ((cldiv > 255u) && (ckdiv < 7u)) {
		ckdiv++;
		cldiv >>= 1;
	}

	debug_ckdiv_final = ckdiv;
	debug_cldiv_final = cldiv;

	if (cldiv > 255u || ckdiv > 7u) {
		return false; // Requested speed out of range
	}

	TWI0->TWI_CWGR = TWI_CWGR_CLDIV(cldiv) | TWI_CWGR_CHDIV(cldiv) | TWI_CWGR_CKDIV(ckdiv); // Apply clock config
	return true;
}

bool MC3419_i2c_init(uint32_t mck_hz, uint32_t i2c_hz)
{
	if (mck_hz == 0 || i2c_hz == 0) return false; // Validate parameters
	
	// Debug: Store parameters for debugging
	volatile uint32_t debug_mck = mck_hz;
	volatile uint32_t debug_i2c = i2c_hz;
	volatile uint32_t debug_step = 1;
	
	// Enable peripheral clock for TWI0
	PMC->PMC_PCER0 = (1u << ID_TWI0);
	debug_step = 2;

	// Configure pins
	twi0_configure_pins_periphA();
	debug_step = 3;

	// Reset and enable master
	twi0_reset();
	debug_step = 4;
	
	TWI0->TWI_CR = TWI_CR_MSEN | TWI_CR_SVDIS; // Enable master, disable slave
	debug_step = 5;

	// Set master mode: 7-bit addressing (default), internal addr size configured per transfer
	TWI0->TWI_MMR = 0;
	debug_step = 6;

	// Set speed
	if (!twi0_set_speed(mck_hz, i2c_hz)) {
		// Debug: Speed setting failed
		volatile uint32_t debug_speed_fail = 1;
		debug_step = 99; // Speed calculation failed
		return false; // Invalid clock settings
	}
	
	debug_step = 7; // Success

	return true;
}

// Get TWI status for debugging
uint32_t MC3419_i2c_get_status(void)
{
	return TWI0->TWI_SR;
}

static inline bool twi0_write_bytes(uint8_t addr, const uint8_t* buf, uint32_t len)
{
	if (len == 0) return true; // Nothing to write

	// Set write mode and slave address
	TWI0->TWI_MMR = TWI_MMR_DADR(addr);

	// Send bytes
	for (uint32_t i = 0; i < len; i++) {
		// Wait TXRDY with timeout
		uint32_t timeout = 10000; // Adjust timeout as needed
		while (!(TWI0->TWI_SR & TWI_SR_TXRDY) && timeout > 0) {
			timeout--;
		}
		if (timeout == 0) return false; // Timeout occurred
		TWI0->TWI_THR = buf[i];
	}

	// Send STOP
	TWI0->TWI_CR = TWI_CR_STOP;
	// Wait TXCOMP with timeout
	uint32_t timeout = 10000; // Adjust timeout as needed
	while (!(TWI0->TWI_SR & TWI_SR_TXCOMP) && timeout > 0) {
		timeout--;
	}
	if (timeout == 0) return false; // Timeout occurred

	return true;
}

static inline bool twi0_write_then_read(uint8_t addr, const uint8_t* w, uint32_t wn, uint8_t* r, uint32_t rn)
{
	if (wn == 0 || rn == 0) return false; // Need both phases

	// Program internal address (use IADR) when wn<=3, otherwise do a separate write phase.
	if (wn <= 3) {
		uint32_t iadrsz = (wn == 1 ? TWI_MMR_IADRSZ_1_BYTE :
		wn == 2 ? TWI_MMR_IADRSZ_2_BYTE :
		TWI_MMR_IADRSZ_3_BYTE);
		uint32_t iadr = 0;
		for (uint32_t i=0;i<wn;i++) {
			iadr = (iadr << 8) | w[i]; // Pack internal address bytes
		}
		TWI0->TWI_MMR = TWI_MMR_MREAD | TWI_MMR_DADR(addr) | iadrsz; // Read mode + address size
		TWI0->TWI_IADR = iadr; // Internal address
		} else {
		// Long register sequence: write the bytes first (no STOP), then issue read
		TWI0->TWI_MMR = TWI_MMR_DADR(addr); // Write mode
		for (uint32_t i = 0; i < wn; i++) {
			uint32_t timeout = 10000;
			while (!(TWI0->TWI_SR & TWI_SR_TXRDY) && timeout > 0) {
				timeout--;
			}
			if (timeout == 0) return false; // Timeout occurred
			TWI0->TWI_THR = w[i];
		}
		uint32_t timeout = 10000;
		while (!(TWI0->TWI_SR & TWI_SR_TXRDY) && timeout > 0) {
			timeout--;
		}
		if (timeout == 0) return false; // Timeout occurred
		// fall through to read with no internal address
		TWI0->TWI_MMR = TWI_MMR_MREAD | TWI_MMR_DADR(addr); // Switch to read mode
	}

	// Start read
	if (rn == 1) {
		TWI0->TWI_CR = TWI_CR_START | TWI_CR_STOP; // Single-byte read
		} else {
		TWI0->TWI_CR = TWI_CR_START; // Multi-byte read
	}

	for (uint32_t i = 0; i < rn; i++) {
		if (i == rn - 1) {
			// Last byte: ensure STOP was set
			TWI0->TWI_CR = TWI_CR_STOP;
		}
		uint32_t timeout = 10000;
		while (!(TWI0->TWI_SR & TWI_SR_RXRDY) && timeout > 0) {
			timeout--;
		}
		if (timeout == 0) return false; // Timeout occurred
		r[i] = (uint8_t)TWI0->TWI_RHR; // Read byte
	}

	uint32_t timeout = 10000;
	while (!(TWI0->TWI_SR & TWI_SR_TXCOMP) && timeout > 0) {
		timeout--;
	}
	if (timeout == 0) return false; // Timeout occurred
	return true;
}


bool MC3419_i2c_read(uint8_t reg, uint8_t *buf, uint32_t len)
{
	uint8_t regb[1] = { reg };
	return twi0_write_then_read(MC3419_ADDR, regb, 1, buf, len); // Register pointer then read
}

bool MC3419_whoami(uint8_t *out_who)
{
	if (!out_who) return false; // Validate pointer
	return MC3419_i2c_read(0x0F, out_who, 1); // Read WHO_AM_I register
}

// Generic I2C functions that accept device address parameter
bool i2c_write_bytes(uint8_t addr7, const uint8_t *data, uint32_t len)
{
	if (!data || len == 0) return true; // Nothing to write
	return twi0_write_bytes(addr7, data, len);
}

bool i2c_write_then_read(uint8_t addr7, const uint8_t *w, uint32_t wn, uint8_t *r, uint32_t rn)
{
	if (!w || !r || wn == 0 || rn == 0) return false; // Validate parameters
	return twi0_write_then_read(addr7, w, wn, r, rn);
}

// Check if TWI is properly initialized and ready
bool MC3419_i2c_is_ready(void)
{
	// Check if TWI0 is enabled and in master mode
	return (PMC->PMC_PCSR0 & (1u << ID_TWI0)) && // TWI0 clock enabled
	       (TWI0->TWI_CR & TWI_CR_MSEN) && // Master mode enabled
	       !(TWI0->TWI_CR & TWI_CR_SVDIS); // Slave disabled
}

// Write 'len' bytes starting at register 'reg'.
bool MC3419_i2c_write(uint8_t reg, const uint8_t *data, uint32_t len)
{
	if (!data || len == 0) return true; // Nothing to write
	
	// Prepare write buffer with register address followed by data
	uint8_t write_buf[16]; // Max 15 bytes of data + 1 register address
	if (len > 15) return false; // Too much data
	
	write_buf[0] = reg;
	for (uint32_t i = 0; i < len; i++) {
		write_buf[i + 1] = data[i];
	}
	
	return twi0_write_bytes(MC3419_ADDR, write_buf, len + 1);
}

// High-level MC3419 functions
bool mc3419_init(void)
{
	uint8_t who_am_i;
	uint8_t config_data;
	
	// Initialize I2C with 100kHz speed
	if (!MC3419_i2c_init(SystemCoreClock, 100000)) {
		return false;
	}
	
	// Wait for I2C to be ready
	delay_ms(10);
	
	// Check WHO_AM_I register
	if (!MC3419_whoami(&who_am_i)) {
		return false;
	}
	
	// Expected WHO_AM_I value for MC3419 (adjust if different)
	if (who_am_i != 0x33) {
		// Store for debugging
		volatile uint8_t debug_who_am_i = who_am_i;
		// Continue anyway - might be different variant
	}
	
	// Configure sensor for wake mode, 100Hz, ±8g range
	config_data = MC3419_MODE_WAKE;
	if (!MC3419_i2c_write(MC3419_REG_MODE, &config_data, 1)) {
		return false;
	}
	
	delay_ms(10);
	
	// Set sample rate to 100Hz
	config_data = MC3419_SAMPLE_RATE_100HZ;
	if (!MC3419_i2c_write(MC3419_REG_SAMPLE_RATE, &config_data, 1)) {
		return false;
	}
	
	delay_ms(10);
	
	// Set range to ±8g
	config_data = MC3419_RANGE_8G;
	if (!MC3419_i2c_write(MC3419_REG_RANGE, &config_data, 1)) {
		return false;
	}
	
	delay_ms(10);
	
	return true;
}

bool mc3419_read_data(mc3419_data_t *data)
{
	if (!data) return false;
	
	uint8_t temp_lsb, temp_msb;
	uint8_t z_lsb, z_msb;
	uint8_t y_lsb, y_msb;
	uint8_t x_lsb, x_msb;
	
	// Debug: Check I2C status before reading
	volatile uint32_t debug_i2c_status = MC3419_i2c_get_status();
	
	// Read temperature data (2 bytes)
	if (!MC3419_i2c_read(MC3419_REG_TEMP_LSB, &temp_lsb, 1) ||
	    !MC3419_i2c_read(MC3419_REG_TEMP_MSB, &temp_msb, 1)) {
		volatile uint32_t debug_temp_read_failed = 1;
		data->valid = false;
		return false;
	}
	
	// Read Z-axis data (2 bytes)
	if (!MC3419_i2c_read(MC3419_REG_ZOUT_LSB, &z_lsb, 1) ||
	    !MC3419_i2c_read(MC3419_REG_ZOUT_MSB, &z_msb, 1)) {
		volatile uint32_t debug_z_read_failed = 1;
		data->valid = false;
		return false;
	}
	
	// Read Y-axis data (2 bytes)
	if (!MC3419_i2c_read(MC3419_REG_YOUT_LSB, &y_lsb, 1) ||
	    !MC3419_i2c_read(MC3419_REG_YOUT_MSB, &y_msb, 1)) {
		volatile uint32_t debug_y_read_failed = 1;
		data->valid = false;
		return false;
	}
	
	// Read X-axis data (2 bytes)
	if (!MC3419_i2c_read(MC3419_REG_XOUT_LSB, &x_lsb, 1) ||
	    !MC3419_i2c_read(MC3419_REG_XOUT_MSB, &x_msb, 1)) {
		volatile uint32_t debug_x_read_failed = 1;
		data->valid = false;
		return false;
	}
	
	// Debug: Store raw byte values for analysis
	volatile uint8_t debug_temp_lsb = temp_lsb;
	volatile uint8_t debug_temp_msb = temp_msb;
	volatile uint8_t debug_x_lsb = x_lsb;
	volatile uint8_t debug_x_msb = x_msb;
	volatile uint8_t debug_y_lsb = y_lsb;
	volatile uint8_t debug_y_msb = y_msb;
	volatile uint8_t debug_z_lsb = z_lsb;
	volatile uint8_t debug_z_msb = z_msb;
	
	// Combine bytes into 16-bit values
	data->temp = (int16_t)((temp_msb << 8) | temp_lsb);
	data->z = (int16_t)((z_msb << 8) | z_lsb);
	data->y = (int16_t)((y_msb << 8) | y_lsb);
	data->x = (int16_t)((x_msb << 8) | x_lsb);
	
	data->valid = true;
	return true;
}

float mc3419_convert_accel_to_g(int16_t raw, uint8_t range)
{
	float scale_factor;
	
	// Determine scale factor based on range
	switch (range) {
		case MC3419_RANGE_2G:
			scale_factor = 2.0f / 32768.0f; // ±2g range
			break;
		case MC3419_RANGE_4G:
			scale_factor = 4.0f / 32768.0f; // ±4g range
			break;
		case MC3419_RANGE_8G:
			scale_factor = 8.0f / 32768.0f; // ±8g range
			break;
		case MC3419_RANGE_16G:
			scale_factor = 16.0f / 32768.0f; // ±16g range
			break;
		default:
			scale_factor = 8.0f / 32768.0f; // Default to ±8g
			break;
	}
	
	return (float)raw * scale_factor;
}

float mc3419_convert_temp_to_celsius(int16_t raw)
{
	// MC3419 temperature conversion formula
	// Temperature in Celsius = (raw_value / 256.0) + 25.0
	return ((float)raw / 256.0f) + 25.0f;
}


