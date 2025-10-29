/**
 * Pin Test Main - Standalone encoder pin toggle test
 * 
 * This is a simplified main function that only runs the encoder pin toggle test
 * for oscilloscope verification. Use this to test soldering without running
 * the full application.
 * 
 * To use:
 * 1. Replace main.c with this file temporarily
 * 2. Build and flash the project
 * 3. Connect oscilloscope probes to:
 *    - PA0 (pin 52) - Encoder A
 *    - PA1 (pin 70) - Encoder B  
 *    - PD17 (pin 53) - Encoder Enable
 * 4. Run the test and observe the pin patterns on oscilloscope
 */

#include <asf.h>
#include "WIB_Init.h"
#include "encoder.h"

int main (void)
{
    /* Initialize TIB hardware */
    WIB_Init();
    
    /* Run encoder pin toggle test for oscilloscope verification */
    /* This will toggle PA0, PA1, and PD17 in a specific pattern */
    /* Connect oscilloscope probes to these pins to verify soldering */
    encoder1_standalone_pin_test();
    
    /* Should never reach here */
    while(1);
}