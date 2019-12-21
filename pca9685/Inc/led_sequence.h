#ifndef LED_SEQUENCE_H
#define LED_SEQUENCE_H

#include "pca9685.h"
#include "stm32f0xx_hal.h"

typedef enum {
	CHANNEL_0 = 0b00001,
	CHANNEL_1 = 0b00010,
	CHANNEL_2 = 0b00100,
	CHANNEL_3 = 0b01000,
	CHANNEL_4 = 0b10000
} LEDChannelTypedef;

//this is one action item -> keep it in program memory
typedef struct {
	const uint8_t LED_no;				//which LEDs to illuminate (implemented as an LED channel mask)
	const LEDColorTypedef *color;		//what color to light said LED
	const uint16_t timestamp;			//at how many ms after the event call should this event happen
} LEDEventTypedef;

#define END_SEQUENCE (LEDEventTypedef){0, NULL, 0}

extern const LEDEventTypedef PACK_100[];
extern const LEDEventTypedef PACK_90[];
extern const LEDEventTypedef PACK_80[];
extern const LEDEventTypedef PACK_70[];
extern const LEDEventTypedef PACK_60[];
extern const LEDEventTypedef PACK_50[];
extern const LEDEventTypedef PACK_40[];
extern const LEDEventTypedef PACK_30[];
extern const LEDEventTypedef PACK_20[];
extern const LEDEventTypedef PACK_10[];
extern const LEDEventTypedef PACK_CRITICAL[];

#endif
