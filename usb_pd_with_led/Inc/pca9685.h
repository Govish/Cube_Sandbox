#ifndef PCA9685_H
#define PCA9685_H

#include "stm32f0xx_hal.h"
#include "stdbool.h"

typedef struct {
	uint16_t red;
	uint16_t green;
	uint16_t blue;
} LEDColorTypedef;

#include "led_sequence.h" //have to put this include here so LEDColorTypedef is defined

extern const LEDColorTypedef LED_OFF;
extern const LEDColorTypedef RED;
extern const LEDColorTypedef ORANGE;
extern const LEDColorTypedef YELLOW;
extern const LEDColorTypedef GREEN;
extern const LEDColorTypedef BLUE;
extern const LEDColorTypedef VIOLET;
extern const LEDColorTypedef WHITE;

//public functions
//return the result of the i2c transaction
HAL_StatusTypeDef pca_init();
HAL_StatusTypeDef pca_wake();
HAL_StatusTypeDef pca_sleep();
HAL_StatusTypeDef pca_reset();

HAL_StatusTypeDef pca_write_led(uint8_t led, LEDColorTypedef *color);
HAL_StatusTypeDef pca_all_off();

void pca_display_chargePower(float power); //from 0-100W
//pass state of charge as a float from 0->1
void pca_display_SOC(float soc);
#define PACK_CRITICAL_THRESHOLD 0.04 //0 to 0.1 -> soc that the pack is considered dead

HAL_StatusTypeDef pca_update(); //attach to a 1ms interrupt!!!!

#endif
