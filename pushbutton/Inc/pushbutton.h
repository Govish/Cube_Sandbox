#ifndef PUSHBUTTON_H
#define PUSHBUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "stm32f0xx_hal.h"

typedef enum {
    RELEASED,
    SHORT_PRESSED,
    LONG_PRESSED
} b_states ;

extern TIM_HandleTypeDef htim17; //1ms timer to handle the blinking and debouncing code
								 //will create this in main since that's where cube dumps it

void pb_init(GPIO_TypeDef* _LED_PORT, uint16_t _LED_PIN, GPIO_TypeDef* _BUTTON_PORT, uint16_t _BUTTON_PIN);
void pb_shutdown(); //call before entering sleep mode
void pb_wake(); //call when the button pressed after pack deep sleep

void pb_toggle();  // LED function
void pb_set();     // turn LED on
void pb_clear();   // turn LED off

//pass millis to tell you how frequently to blink
void pb_start_blink(uint32_t millis); //blink the pushbutton LED at 1Hz
void pb_stop_blink();  //stop blinking the LED

b_states pb_state();   // get the state of the Pushbutton

void pb_update(); //PLACE IN TIMER_IRQ EVERY 1ms


#ifdef __cplusplus
}
#endif

#endif
