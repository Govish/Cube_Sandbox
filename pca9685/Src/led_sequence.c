/*
 * led_sequence.c
 *
 *  Created on: Nov 10, 2019
 *      Author: Ishaan
 */

#include "led_sequence.h"

const LEDEventTypedef PACK_100[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &GREEN, 0},
		{CHANNEL_1, &GREEN, 75},
		{CHANNEL_2, &GREEN, 75},
		{CHANNEL_3, &GREEN, 75},
		{CHANNEL_4, &GREEN, 75},
		{CHANNEL_4, &LED_OFF, 500},
		{CHANNEL_4, &GREEN, 500},
		{CHANNEL_4, &LED_OFF, 500},
		{CHANNEL_4, &GREEN, 500},
		{CHANNEL_4, &LED_OFF, 500},
		{CHANNEL_4, &GREEN, 500},
		{CHANNEL_4, &LED_OFF, 500},
		{CHANNEL_4, &GREEN, 500},
		{CHANNEL_4, &LED_OFF, 500},
		END_SEQUENCE //this is the "sequence end" code
};

const LEDEventTypedef PACK_90[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &GREEN, 0},
		{CHANNEL_1, &GREEN, 75},
		{CHANNEL_2, &GREEN, 75},
		{CHANNEL_3, &GREEN, 75},
		{CHANNEL_3, &LED_OFF, 500},
		{CHANNEL_3, &GREEN, 500},
		{CHANNEL_3, &LED_OFF, 500},
		{CHANNEL_3, &GREEN, 500},
		{CHANNEL_3, &LED_OFF, 500},
		{CHANNEL_3, &GREEN, 500},
		{CHANNEL_3, &LED_OFF, 500},
		{CHANNEL_3, &GREEN, 500},
		{CHANNEL_3, &LED_OFF, 500},
		END_SEQUENCE //this is the "sequence end" code
};

const LEDEventTypedef PACK_80[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &GREEN, 0},
		{CHANNEL_1, &GREEN, 75},
		{CHANNEL_2, &GREEN, 75},
		{CHANNEL_2, &LED_OFF, 500},
		{CHANNEL_2, &GREEN, 500},
		{CHANNEL_2, &LED_OFF, 500},
		{CHANNEL_2, &GREEN, 500},
		{CHANNEL_2, &LED_OFF, 500},
		{CHANNEL_2, &GREEN, 500},
		{CHANNEL_2, &LED_OFF, 500},
		{CHANNEL_2, &GREEN, 500},
		{CHANNEL_2, &LED_OFF, 500},
		END_SEQUENCE //this is the "sequence end" code
};

const LEDEventTypedef PACK_70[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &GREEN, 0},
		{CHANNEL_1, &GREEN, 75},
		{CHANNEL_1, &LED_OFF, 500},
		{CHANNEL_1, &GREEN, 500},
		{CHANNEL_1, &LED_OFF, 500},
		{CHANNEL_1, &GREEN, 500},
		{CHANNEL_1, &LED_OFF, 500},
		{CHANNEL_1, &GREEN, 500},
		{CHANNEL_1, &LED_OFF, 500},
		{CHANNEL_1, &GREEN, 500},
		{CHANNEL_1, &LED_OFF, 500},
		END_SEQUENCE //this is the "sequence end" code
};

const LEDEventTypedef PACK_60[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &GREEN, 0},
		{CHANNEL_0, &LED_OFF, 500},
		{CHANNEL_0, &GREEN, 500},
		{CHANNEL_0, &LED_OFF, 500},
		{CHANNEL_0, &GREEN, 500},
		{CHANNEL_0, &LED_OFF, 500},
		{CHANNEL_0, &GREEN, 500},
		{CHANNEL_0, &LED_OFF, 500},
		{CHANNEL_0, &GREEN, 500},
		{CHANNEL_0, &LED_OFF, 500},
		END_SEQUENCE //this is the "sequence end" code
};

const LEDEventTypedef PACK_50[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &YELLOW, 0},
		{CHANNEL_1, &YELLOW, 75},
		{CHANNEL_2, &YELLOW, 75},
		{CHANNEL_3, &YELLOW, 75},
		{CHANNEL_4, &YELLOW, 75},
		{CHANNEL_4, &LED_OFF, 500},
		{CHANNEL_4, &YELLOW, 500},
		{CHANNEL_4, &LED_OFF, 500},
		{CHANNEL_4, &YELLOW, 500},
		{CHANNEL_4, &LED_OFF, 500},
		{CHANNEL_4, &YELLOW, 500},
		{CHANNEL_4, &LED_OFF, 500},
		{CHANNEL_4, &YELLOW, 500},
		{CHANNEL_4, &LED_OFF, 500},
		END_SEQUENCE //this is the "sequence end" code
};

const LEDEventTypedef PACK_40[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &YELLOW, 0},
		{CHANNEL_1, &YELLOW, 75},
		{CHANNEL_2, &YELLOW, 75},
		{CHANNEL_3, &YELLOW, 75},
		{CHANNEL_3, &LED_OFF, 500},
		{CHANNEL_3, &YELLOW, 500},
		{CHANNEL_3, &LED_OFF, 500},
		{CHANNEL_3, &YELLOW, 500},
		{CHANNEL_3, &LED_OFF, 500},
		{CHANNEL_3, &YELLOW, 500},
		{CHANNEL_3, &LED_OFF, 500},
		{CHANNEL_3, &YELLOW, 500},
		{CHANNEL_3, &LED_OFF, 500},
		END_SEQUENCE //this is the "sequence end" code
};

const LEDEventTypedef PACK_30[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &YELLOW, 0},
		{CHANNEL_1, &YELLOW, 75},
		{CHANNEL_2, &YELLOW, 75},
		{CHANNEL_2, &LED_OFF, 500},
		{CHANNEL_2, &YELLOW, 500},
		{CHANNEL_2, &LED_OFF, 500},
		{CHANNEL_2, &YELLOW, 500},
		{CHANNEL_2, &LED_OFF, 500},
		{CHANNEL_2, &YELLOW, 500},
		{CHANNEL_2, &LED_OFF, 500},
		{CHANNEL_2, &YELLOW, 500},
		{CHANNEL_2, &LED_OFF, 500},
		END_SEQUENCE //this is the "sequence end" code
};

const LEDEventTypedef PACK_20[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &RED, 0},
		{CHANNEL_1, &RED, 75},
		{CHANNEL_1, &LED_OFF, 500},
		{CHANNEL_1, &RED, 500},
		{CHANNEL_1, &LED_OFF, 500},
		{CHANNEL_1, &RED, 500},
		{CHANNEL_1, &LED_OFF, 500},
		{CHANNEL_1, &RED, 500},
		{CHANNEL_1, &LED_OFF, 500},
		{CHANNEL_1, &RED, 500},
		{CHANNEL_1, &LED_OFF, 500},
		END_SEQUENCE //this is the "sequence end" code
};

const LEDEventTypedef PACK_10[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &RED, 0},
		{CHANNEL_0, &LED_OFF, 500},
		{CHANNEL_0, &RED, 500},
		{CHANNEL_0, &LED_OFF, 500},
		{CHANNEL_0, &RED, 500},
		{CHANNEL_0, &LED_OFF, 500},
		{CHANNEL_0, &RED, 500},
		{CHANNEL_0, &LED_OFF, 500},
		{CHANNEL_0, &RED, 500},
		{CHANNEL_0, &LED_OFF, 500},
		END_SEQUENCE //this is the "sequence end" code
};

const LEDEventTypedef PACK_CRITICAL[] = {
	/* {LED_NOs, {r,g,b}, timestamp} */
		{CHANNEL_0|CHANNEL_1|CHANNEL_2|CHANNEL_3|CHANNEL_4, &LED_OFF, 0},
		{CHANNEL_0, &RED, 0},
		{CHANNEL_0, &LED_OFF, 100},
		{CHANNEL_0, &RED, 100},
		{CHANNEL_0, &LED_OFF, 100},
		{CHANNEL_0, &RED, 100},
		{CHANNEL_0, &LED_OFF, 100},
		{CHANNEL_0, &RED, 100},
		{CHANNEL_0, &LED_OFF, 100},
		{CHANNEL_0, &RED, 100},
		{CHANNEL_0, &LED_OFF, 100},
		{CHANNEL_0, &RED, 100},
		{CHANNEL_0, &LED_OFF, 100},
		{CHANNEL_0, &RED, 100},
		{CHANNEL_0, &LED_OFF, 100},
		{CHANNEL_0, &RED, 100},
		{CHANNEL_0, &LED_OFF, 100},
		{CHANNEL_0, &RED, 100},
		{CHANNEL_0, &LED_OFF, 100},
		{CHANNEL_0, &RED, 100},
		{CHANNEL_0, &LED_OFF, 100},
		END_SEQUENCE //this is the "sequence end" code
};
