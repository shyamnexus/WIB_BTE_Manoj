#include "asf.h"

void board_init(void)
{
    // Enable peripheral clocks for PIO controllers
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOB);
    pmc_enable_periph_clk(ID_PIOC);
    pmc_enable_periph_clk(ID_PIOD);

    /***********************
     * I2C - MC3419 Accelerometer
     ***********************/
    // I2C pins -> Peripheral A
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA3A_TWD0, 0);  // SDA
    pio_configure(PIOA, PIO_PERIPH_A, PIO_PA4A_TWCK0, 0); // SCL

    /***********************
     * CAN - CAN Bus
     ***********************/
    // CAN pins -> Peripheral A
    pio_configure(PIOB, PIO_PERIPH_A, PIO_PB2A_CANTX0, 0); // CAN TX
    pio_configure(PIOB, PIO_PERIPH_A, PIO_PB3A_CANRX0, 0); // CAN RX


    /***********************
     * ENCODER 1 - PA0/TIOA0, PA1/TIOB0, PD17/ENABLE1
     ***********************/
    // Encoder 1 pins -> Peripheral B
    pio_configure(PIOA, PIO_PERIPH_B, PIO_PA0B_TIOA0, 0);  // ENC1_A
    pio_configure(PIOA, PIO_PERIPH_B, PIO_PA1B_TIOB0, 0);  // ENC1_B
    // Encoder 1 enable as GPIO output
    pio_set_output(PIOD, PIO_PD17, 0, 0, 0); // ENC1_ENABLE (low = enabled)

    /***********************
     * ENCODER 2 - PA15/TIOA1, PA16/TIOB1, PD27/ENABLE2
     ***********************/
    // Encoder 2 pins -> Peripheral B
    pio_configure(PIOA, PIO_PERIPH_B, PIO_PA15B_TIOA1, 0); // ENC2_A
    pio_configure(PIOA, PIO_PERIPH_B, PIO_PA16B_TIOB1, 0); // ENC2_B
    // Encoder 2 enable as GPIO output
    pio_set_output(PIOD, PIO_PD27, 0, 0, 0); // ENC2_ENABLE (low = enabled)

    /***********************
     * LED RING CONTROL - PD22/PWMH2
     ***********************/
    // LED ring control pin -> Peripheral A (PWM)
    pio_configure(PIOD, PIO_PERIPH_A, PIO_PD22A_PWMH2, 0); // LED_DATA/PWM

    /***********************
     * FAN CONTROL - PD25/FAULT, PD24/FULL_ON
     ***********************/
    // FAN fault input with pull-up
    pio_set_input(PIOD, PIO_PD25, PIO_PULLUP); // FAN_FAULT
    // FAN full on control as GPIO output
    pio_set_output(PIOD, PIO_PD24, 0, 0, 0); // FAN_FULL_ON (low = off)

    /***********************
     * ACCELEROMETER CONTROL - PA19/PGMD7, PA20/PGMD8
     ***********************/
    // Accelerometer control pins as GPIO outputs
    pio_set_output(PIOA, PIO_PA19, 0, 0, 0); // ACCEL_CTRL_1
    pio_set_output(PIOA, PIO_PA20, 0, 0, 0); // ACCEL_CTRL_2


    /***********************
     * CRYSTAL PINS - PB9/XIN, PB8/XOUT
     ***********************/
    // Crystal pins are automatically configured by the system
    // No explicit configuration needed for PB9/XIN and PB8/XOUT
}
