#ifndef PCA_REGS_H
#define PCA_REGS_H

#include "stm32f0xx_hal.h"

//defining some control register maps
//only to be included by the .c file for the PCA9685 library

//using this cool union style definition allowing you to twiddle individual bits
//in a meaningful way
#define MODE_1_REG 0x00
typedef union {
	uint8_t data;
	struct {
		uint8_t all_call_en : 1;
		uint8_t sub3_en : 1;
		uint8_t sub2_en : 1;
		uint8_t sub1_en : 1;
		uint8_t sleep_en : 1;
		uint8_t auto_inc_en : 1;
		uint8_t ext_clk_en : 1;
		uint8_t restart : 1;
	} map;
} Mode1RegTypedef;


#define MODE_2_REG 0x01
typedef union {
	uint8_t data;
	struct {
		uint8_t o_en : 2;
		uint8_t out_drv : 1;
		uint8_t out_change : 1;
		uint8_t invert : 1;
		uint8_t reserved : 3;
	} map;
} Mode2RegTypedef;

#define GENERAL_CALL 0x00
#define SW_RESET_COMMAND 0x06

#define ALL_ON_REG_BASE 0xFA //2 bytes
#define LED_REG_BASE 0x06 //2 bytes on, 2 bytes off

#endif
