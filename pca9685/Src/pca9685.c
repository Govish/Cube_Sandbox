#include "pca9685.h"
#include "pca_regs.h"
#include "i2c.h"

const LEDColorTypedef LED_OFF = {0,0,0};
const LEDColorTypedef RED = {4095,0,0};
const LEDColorTypedef ORANGE = {3840,1024,0};
const LEDColorTypedef YELLOW = {2048,2048,0};
const LEDColorTypedef GREEN = {0,4095,0};
const LEDColorTypedef BLUE = {0,0,4095};
const LEDColorTypedef VIOLET = {1024,0,3072};
const LEDColorTypedef WHITE = {3072,3072,3072};

static const uint8_t led_map[] = {4,3,0,1,2};

static volatile bool event_active; //can be deactivated in an interrupt context
static LEDEventTypedef* event_sequence;
static volatile uint8_t sequence_pointer; //points to the event in the event sequence
static volatile uint16_t event_tick_counter; //holds the ms tick for the event sequencing

#define PCA_ADDR 0x80
#define OFFSET_MULT 256

HAL_StatusTypeDef pca_init() {
	//configure the mode registers
		//ehhh prescaler is 200hz by default so should be fine
	//wake the device if it's in sleep
	//set all the LEDs off
	event_active = false;

	HAL_StatusTypeDef status;

	Mode1RegTypedef mode1;
	Mode2RegTypedef mode2;

	mode1.data = 0;
	mode1.map.auto_inc_en = 1;

	mode2.data = 0;
	mode2.map.invert = 1;
	mode2.map.o_en = 2;

	status = pca_reset();
	if(status != HAL_OK) return status;
	status = i2c_write_reg(PCA_ADDR, MODE_1_REG, mode1.data);

	if(status != HAL_OK) return status;
	status = i2c_write_reg(PCA_ADDR, MODE_2_REG, mode2.data);
	if(status != HAL_OK) return status;

	return pca_all_off();
}

HAL_StatusTypeDef pca_reset() {
	event_active = false;
	return i2c_write_command(GENERAL_CALL, SW_RESET_COMMAND);
}

HAL_StatusTypeDef pca_wake() {
	Mode1RegTypedef mode1;
	HAL_StatusTypeDef status;

	event_active = false;

	//read the mode1 register
	status = i2c_read_reg(PCA_ADDR, MODE_1_REG, &mode1.data);
	if(status != HAL_OK) return status;

	mode1.map.sleep_en = 0;
	status = i2c_write_reg(PCA_ADDR, MODE_1_REG, mode1.data);
	if(status != HAL_OK) return status;

	HAL_Delay(1); //let the oscillator stabilize
	mode1.map.restart = 1; //restart the PWM channels
	return i2c_write_reg(PCA_ADDR, MODE_1_REG, mode1.data);
}

HAL_StatusTypeDef pca_sleep() {
	Mode1RegTypedef mode1;
	HAL_StatusTypeDef status;

	event_active = false;

	//read the mode1 register
	status = i2c_read_reg(PCA_ADDR, MODE_1_REG, &mode1.data);

	mode1.map.sleep_en = 1; //set the sleep bit
	if(status != HAL_OK) return status;
	return i2c_write_reg(PCA_ADDR, MODE_1_REG, mode1.data);
}

HAL_StatusTypeDef pca_write_led(uint8_t led, LEDColorTypedef *color) {
	if(led > 4) return HAL_ERROR; //make sure the input is only between 0 and 4
	led = led_map[led];

	uint16_t red_start, green_start, blue_start, red_end, green_end, blue_end;
	uint8_t txdata[13];

	//stagger the LED turn-on so we don't get huge current spikes at the beginning of the cycle
	red_start = OFFSET_MULT * (led*3);
	green_start = OFFSET_MULT * (led*3 + 1);
	blue_start = OFFSET_MULT * (led*3 + 2);

	red_end = color->red == 0 ? 0x1FFF : (red_start + color->red) % 4096;
	green_end = color->green == 0 ? 0x1FFF : (green_start + color->green) % 4096;
	blue_end = color->blue == 0 ? 0x1FFF : (blue_start + color->blue) % 4096;

	txdata[0] = LED_REG_BASE + 3 * (led<<2);
	if(led <= 2) {
		txdata[1] = red_start & 0xFF;
		txdata[2] = (red_start >> 8) & 0x1F;
		txdata[3] = red_end & 0xFF;
		txdata[4] = (red_end >> 8) & 0x1F;
	}
	else {
		txdata[1] = blue_start & 0xFF;
		txdata[2] = (blue_start >> 8) & 0x1F;
		txdata[3] = blue_end & 0xFF;
		txdata[4] = (blue_end >> 8) & 0x1F;
	}

	txdata[5] = green_start & 0xFF;
	txdata[6] = (green_start >> 8) & 0x1F;
	txdata[7] = green_end & 0xFF;
	txdata[8] = (green_end >> 8) & 0x1F;

	if(led <= 2) {
		txdata[9] = blue_start & 0xFF;
		txdata[10] = (blue_start >> 8) & 0x1F;
		txdata[11] = blue_end & 0xFF;
		txdata[12] = (blue_end >> 8) & 0x1F;
	}
	else {
		txdata[9] = red_start & 0xFF;
		txdata[10] = (red_start >> 8) & 0x1F;
		txdata[11] = red_end & 0xFF;
		txdata[12] = (red_end >> 8) & 0x1F;
	}

	return i2c_write_regs(PCA_ADDR, txdata, 13);
}

HAL_StatusTypeDef pca_all_off() {
	uint8_t txdata[5];
	txdata[0] = ALL_ON_REG_BASE;
	txdata[1] = 0;
	txdata[2] = 0;
	txdata[3] = 0;
	txdata[4] = 0x10;
	return i2c_write_regs(PCA_ADDR, txdata, 5);
}

void pca_display_SOC(float soc) {
	//re-casting so compiler shuts up about discarding const qualifier
	if(soc > 0.9) event_sequence = (LEDEventTypedef*) PACK_100;
	else if(soc > 0.8) event_sequence = (LEDEventTypedef*) PACK_90;
	else if(soc > 0.7) event_sequence = (LEDEventTypedef*) PACK_80;
	else if(soc > 0.6) event_sequence = (LEDEventTypedef*) PACK_70;
	else if(soc > 0.5) event_sequence = (LEDEventTypedef*) PACK_60;
	else if(soc > 0.4) event_sequence = (LEDEventTypedef*) PACK_50;
	else if(soc > 0.3) event_sequence = (LEDEventTypedef*) PACK_40;
	else if(soc > 0.2) event_sequence = (LEDEventTypedef*) PACK_30;
	else if(soc > 0.1) event_sequence = (LEDEventTypedef*) PACK_20;
	else if(soc > PACK_CRITICAL_THRESHOLD) event_sequence = (LEDEventTypedef*) PACK_10;
	else event_sequence = (LEDEventTypedef*) PACK_CRITICAL;
	event_active = true;
	sequence_pointer = 0;
	event_tick_counter = 0;
}

//====================== PRIVATE FUNCTIONS ========================

//typically gets called from interrupt context
static HAL_StatusTypeDef pca_run_event() {
	HAL_StatusTypeDef status;
	status = HAL_OK;

	LEDEventTypedef* active_event; //create a pointer
	active_event = &event_sequence[sequence_pointer]; //and point it to the active event in our sequence

	if(active_event->LED_no == END_SEQUENCE.LED_no) { //deactivate event
		event_active = false;
		return pca_all_off(); //just shutting down all the LEDs after the event finishes
	}
	else if(event_tick_counter >= active_event->timestamp) { //time to execute the event
		uint8_t enabled_leds;
		enabled_leds = active_event->LED_no;
		if((enabled_leds == 0x1F) && (active_event->color == &LED_OFF)) status = pca_all_off();
		//recasting to remove the "no more const" warning
		else {
			if((enabled_leds & CHANNEL_0) && (status == HAL_OK)) status = pca_write_led(0, (LEDColorTypedef *) active_event->color);
			if((enabled_leds & CHANNEL_1) && (status == HAL_OK)) status = pca_write_led(1, (LEDColorTypedef *) active_event->color);
			if((enabled_leds & CHANNEL_2) && (status == HAL_OK)) status = pca_write_led(2, (LEDColorTypedef *) active_event->color);
			if((enabled_leds & CHANNEL_3) && (status == HAL_OK)) status = pca_write_led(3, (LEDColorTypedef *) active_event->color);
			if((enabled_leds & CHANNEL_4) && (status == HAL_OK)) status = pca_write_led(4, (LEDColorTypedef *) active_event->color);
		}
		sequence_pointer++; //look at the next event UNCONITIONALLY (i.e. even if there's a tx failure)
		event_tick_counter = 0; //reset the counter for the next event
	}
	else event_tick_counter++;
	return status;
}

//============================== 1ms ISR FUNCTION =============================
HAL_StatusTypeDef pca_update() {
	if(event_active) return pca_run_event();
	return HAL_OK;
}
