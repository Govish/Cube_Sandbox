/*
 * pushbutton.c
 *
 *  Created on: Nov 7, 2019
 *      Author: Ishaan Gov
 */

#include "pushbutton.h"

#define LONG_PRESS_DURATION 2000
#define B_SAMPLE_PERIOD 25 //sample the button every 25ms

//GPIOs for the LED and button
static GPIO_TypeDef* LED_PORT;
static uint16_t LED_PIN;
static GPIO_TypeDef* BUTTON_PORT;
static uint16_t BUTTON_PIN;

//a flag that will tell the interrupt to blink or not
static bool isBlinking;
static uint32_t blink_time; //a little counter to tell the period of the blink

static volatile b_states button_state; //changes in debounce handler, likely called in timer interrupt context

//tick counters used in the update function (called in ISR context)
static volatile uint32_t bounce_counter;
static volatile uint32_t blink_counter;

void pb_init(GPIO_TypeDef* _LED_PORT, uint16_t _LED_PIN, GPIO_TypeDef* _BUTTON_PORT, uint16_t _BUTTON_PIN) {
    LED_PORT = _LED_PORT;
    LED_PIN = _LED_PIN;
    BUTTON_PORT = _BUTTON_PORT;
    BUTTON_PIN = _BUTTON_PIN;

    isBlinking = false;
}

void pb_shutdown() {
	/*
	 * TODO:
	 * deinitialize the debounce interrupt
	 * maybe even deinitialize the LED output but prolly not necessary
	 * set up the button as a wake-up source
	 *
	 * might have to do more, but will need a proper state-machine diagram to determine that
	 */
}

void pb_wake() {
	/*
	 * TODO:
	 * reinitialize the debouncer interrupt
	 * and the LED output if it was deinitialized
	 *
	 * might have to do more, but will need a proper state-machine diagram to determine that
	 */
}

void pb_toggle() {
    isBlinking = false;
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
}

void pb_set() {
    isBlinking = false;
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

void pb_clear() {
    isBlinking = false;
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

//pass millis to tell you how frequently to blink
void pb_start_blink(uint32_t millis) {
    //blink every half a <millis> milliseconds
	__disable_irq();
	blink_counter = 0;
    blink_time = millis>>1;
    isBlinking = true;
    __enable_irq();
}

void pb_stop_blink() {
    isBlinking = false;
}

b_states pb_state() {
	//just keepin things atomic out here
	b_states temp;
	__disable_irq();
	temp = button_state;
	__enable_irq();
    return temp;
}

//=========== private functions ===========
static void sampleButton() {
	static uint32_t pressTimer;

    //a little memory element
    //if lastButton is the same as the current button, update the state
    static GPIO_PinState lastButton;
    GPIO_PinState button_pin;
    button_pin = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN); //read the button

    //if the button is done bouncing
    if(lastButton == button_pin) {
        //if the button is released
    	//pull up means inverted logic
    	if(button_pin == GPIO_PIN_SET) {
            button_state = RELEASED;
            pressTimer = HAL_GetTick();
        }

        //if the button is pressed
        else {
            if((HAL_GetTick() - pressTimer) >= LONG_PRESS_DURATION) button_state = LONG_PRESSED;
            else button_state = SHORT_PRESSED;
        }
    }
    lastButton = button_pin;
}
//================ END private functions ==============


//============== timer 17 ISR ================
void pb_update() {
	//####### call debouncer function every 25 ms #######
	bounce_counter = (bounce_counter + 1) % B_SAMPLE_PERIOD;
	if(bounce_counter == 0) sampleButton();

	//####### toggle the LED_PIN every however many ms #######
	blink_counter = (blink_counter + 1) % blink_time;
	if((blink_counter == 0) && (isBlinking)) HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

}
