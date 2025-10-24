#include "asf.h"

void board_init(void)
{
    // Enable peripheral clocks for PIO controllers
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);

    /***********************
     * SPI0 - ADS1120
     ***********************/
  
    /***********************
     * ENCODER PINS
     ***********************/
    // Encoder 1 pins (PA0, PA1) - configured as TC peripheral pins
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA0, PIO_DEFAULT);  // TIOA0
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA1, PIO_DEFAULT);  // TIOB0
    
    // Encoder 2 disabled due to pin conflict with SPI DRDY (PA15)
    // PA15 is used for SPI DRDY, so encoder 2 is not available
    // Encoder enable pins (PD17, PD27) - configured as outputs, low = enabled
    pio_set_output(PIOD, PIO_PD17, 1, 0, 0);  // ENC1_ENABLE (initially high)
    pio_set_output(PIOD, PIO_PD27, 1, 0, 0);  // ENC2_ENABLE (initially high)
    pio_clear(PIOD, PIO_PD17);  // Enable encoder 1 (set low)
    pio_clear(PIOD, PIO_PD27);  // Enable encoder 2 (set low)


}
