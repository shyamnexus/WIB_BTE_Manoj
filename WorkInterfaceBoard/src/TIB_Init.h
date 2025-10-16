/*
 * TIB_Init.h
 *
 * Created: 8/21/2025 10:14:32 AM
 *  Author: MKumar
 */ 


#ifndef TIB_INIT_H_
#define TIB_INIT_H_
#include "sam.h"
#include <board.h>

#include "spi0.h"
#include "delay.h"

int TIB_Init();
int  TIB_test();
enum tooltype{
	TOOL_TYPE_HI_TORQUE=0,
	TOOL_TYPE_LOW_TORQUE
	
	
};

#endif /* TIB_INIT_H_ */