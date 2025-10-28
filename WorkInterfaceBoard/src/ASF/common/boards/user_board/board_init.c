#include "asf.h"

void board_init(void)
{
    // Enable peripheral clocks for PIO controllers
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOD);
}
