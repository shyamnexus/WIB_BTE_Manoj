#include "asf.h"

void board_init(void)
{
    // Enable peripheral clocks for PIO controllers
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);

    /***********************
     * SPI0 - ADS1120
     ***********************/
    // SPI0 pins -> Peripheral A
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA14A_SPCK, 0); // SPCK
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA12A_MISO, 0); // MISO
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA13A_MOSI, 0); // MOSI

    // CS for ADS1120 (GPIO output, high = inactive)
    pio_set_output(PIOA, PIO_PA11, 1, 0, 0);

    // DRDY as input with pull-up (Note: PA15 conflicts with ENC2_A - using PA15 for DRDY)
    pio_set_input(PIOA, PIO_PA15, PIO_PULLUP);

    /***********************
     * ENCODER PINS
     ***********************/
    // Encoder 1 pins (PA5, PA1) - configured as inputs with pull-up
    pio_set_input(PIOA, PIO_PA5, PIO_PULLUP);  // ENC1_A
    pio_set_input(PIOA, PIO_PA1, PIO_PULLUP);  // ENC1_B
    
    // Encoder 2 pins (PA15 conflicts with DRDY, using PA16 for ENC2_B)
    // Note: PA15 is used for SPI DRDY, so ENC2_A functionality is disabled
    pio_set_input(PIOA, PIO_PA16, PIO_PULLUP); // ENC2_B
    
    // Encoder enable pins (PD17, PD27) - configured as outputs, high = enabled
    pio_set_output(PIOD, PIO_PD17, 1, 0, 0);  // ENC1_ENABLE
    pio_set_output(PIOD, PIO_PD27, 1, 0, 0);  // ENC2_ENABLE

    /***********************
     * TOOL SENSE (PD21)
     ***********************/
    pio_set_input(PIOD, PIO_PD21, PIO_PULLUP);
}
