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

    // DRDY as input with pull-up
    pio_set_input(PIOA, PIO_PA15, PIO_PULLUP);

    /***********************
     * TOOL SENSE (PD21)
     ***********************/
    pio_set_input(PIOD, PIO_PD21, PIO_PULLUP);
}
