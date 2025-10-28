/*
 * CFile1.c
 *
 * Created: 8/21/2025 10:12:19 AM
 *  Author: MKumar
 */ 
#include "WIB_Init.h"
int tool_type = 9;
unsigned char who_lis2 = 0;
int WIB_Init()
{
	SystemInit();
	board_init();
	/* Replace with your application code */
	// Debug: Print SystemCoreClock value
	// You can check this value in debugger or via UART
	volatile uint32_t debug_clock = SystemCoreClock;
	return 0;  // Success
}
