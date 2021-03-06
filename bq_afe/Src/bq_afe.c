#include "bq_afe.h"
#include "i2c.h"
#include "math.h" //need a natural logarithm for thermistor beta calculation

#define ADC_GAIN_CONSTANT 0.365 //mv per LSB gain when there's no gain tuning
#define ADC_GAIN_CAL 0.001 //mv per LSB gain tuning when we read from the ADCGAIN bits

#define CC_CONFIG_CONSTANT 0x19 //some arbitrary value that we have to put into the CC_CFG register

static float adc_mv_per_lsb;
static float adc_offset;

//================ Thermistor-related Constants ===================
#define THERM_VREF 3300 //thermistors are referenced to a 3.3v source
#define THERM_PULLUP 10000 //10k pullup resistor internal to the chip
#define THERM_NOM 10000 //ohms
#define THERM_NOM_TEMP 25 //degrees c
#define THERM_BETA 3950 //beta value from like 25-50 or 25-75 (get this from the datasheet or the product page)
#define KELVIN_OFFSET 273 //kelvin temperature at 0c (useful for beta calculation)
//also check out this webpage: https://www.thinksrs.com/downloads/programs/therm%20calc/ntccalibrator/ntccalculator.html

//================ Private Function Prototypes ============
//static HAL_StatusTypeDef afe_balance_upper(uint8_t channel_enable);
//static HAL_StatusTypeDef afe_balance_lower(uint8_t channel_enable);

// ================= PUBLIC FUNCTIONS ====================
HAL_StatusTypeDef afe_init() {
	HAL_StatusTypeDef status;

	//clear the DEVICE_XREADY bit
	SysStatTypedef stat;
	stat.data = 0xFF; //clear all the status flags
	status = i2c_write_reg(AFE_ADDR, SYS_STAT_REG, stat.data);
	if(status != HAL_OK) return status;

	//read the ADC gain and offset registers
	uint8_t gain_offset[10]; //only care about the 0th, 1st, and 9th registers
	status = i2c_read_regs(AFE_ADDR, ADC_CAL_REG_BASE, 10, gain_offset);
	if(status != HAL_OK) return status;
	//stitch together the ADC gain and offset bits as appropriate
	uint8_t adc_gain_bits = ((gain_offset[0] & 0x0C) << 1) | (gain_offset[9] >> 5);
	int8_t adc_offset_bits = (int8_t)(gain_offset[1]);

	adc_mv_per_lsb = ADC_GAIN_CONSTANT + (float)(adc_gain_bits)*ADC_GAIN_CAL;
	adc_offset = (float)(adc_offset_bits);

	//set the control and protect registers consecutively
	SysCtrl1Typedef ctrl1;
	ctrl1.map.ADC_en = 1; //enable the ADC and OVP
	ctrl1.map.Temp_sel = TEMP_THERM;

	SysCtrl2Typedef ctrl2;
	ctrl2.data = 0;
	//lol as of now, we don't need to assert any bits in the CTRL2 register

	Protect1Typedef pt1;
	pt1.data = 0;
	//where we set some delay parameters regarding short circuit detection
	//we're not sensing current through the AFE, so don't have to worry about any of the settings in here

	Protect2Typedef pt2;
	pt2.data = 0;
	//ditto as above (this register has to deal with over current protection though)

	Protect3Typedef pt3;
	pt3.map.OV_delay = OV_DELAY_2s;
	pt3.map.UV_delay = UV_DELAY_4s;

	//send the above 5 registers to the device
	uint8_t ctrl_prot_regs[6];
	ctrl_prot_regs[0] = SYS_CTRL_1_REG; //starting register that we'll be writing to
	ctrl_prot_regs[1] = ctrl1.data;
	ctrl_prot_regs[2] = ctrl2.data;
	ctrl_prot_regs[3] = pt1.data;
	ctrl_prot_regs[4] = pt2.data;
	ctrl_prot_regs[5] = pt3.data;
	status = i2c_write_regs(AFE_ADDR, ctrl_prot_regs, 6);
	if(status != HAL_OK) return status;

	//initialize the OV and UV voltages to something reasonable
	status = afe_set_vThresholds(OV_TRIP_THRESHOLD, UV_TRIP_THRESHOLD);
	if(status != HAL_OK) return status;

	//and finally, set the CC_CFG register to whatever we need to
	return i2c_write_reg(AFE_ADDR, CC_CFG_REG, CC_CONFIG_CONSTANT);
}

//ov and uv parameters are in MILLIVOLTS
HAL_StatusTypeDef afe_set_vThresholds(float ov, float uv) {
	//calculate the OV bit setting from the threshold we want
	// ov_voltage = adc*gain + offset, so reverse this equation
	uint16_t ADC_reading = (uint16_t)((ov-adc_offset)/adc_mv_per_lsb);
	if((ADC_reading & 0x3000) == 0x2000) return HAL_ERROR; //make sure the top two bits are `10`
	uint8_t ov_regval = (ADC_reading & 0xFF0) >> 4; //see BQ76930 datasheet page 37

	//calculate the UV bit setting from the threshold we want
	ADC_reading = (uint16_t)((uv-adc_offset)/adc_mv_per_lsb);
	if((ADC_reading & 0x3000) == 0x1000) return HAL_ERROR; //make sure the top two bits are `01`
	uint8_t uv_regval = (ADC_reading & 0xFF0) >> 4; //see BQ76930 datasheet page 37

	//send itttttt (sequentially in a single write)
	uint8_t thresh_vals[3];
	thresh_vals[0] = OV_TRIP_REG;
	thresh_vals[1] = ov_regval;
	thresh_vals[2] = uv_regval;
	return i2c_write_regs(AFE_ADDR, thresh_vals, 3);
}

HAL_StatusTypeDef afe_set_charge(bool enabled) {
	HAL_StatusTypeDef status;
	SysCtrl2Typedef ctrl2;

	//read the control2 register
	status = i2c_read_reg(AFE_ADDR, SYS_CTRL_2_REG, &ctrl2.data);
	if(status != HAL_OK) return status;

	//set the `charge` bit to the appropriate value
	ctrl2.map.Chg_on = enabled;

	//resend the control2 register to the chip
	return i2c_write_reg(AFE_ADDR, SYS_CTRL_2_REG, ctrl2.data);
}

HAL_StatusTypeDef afe_set_discharge(bool enabled) {
	HAL_StatusTypeDef status;
	SysCtrl2Typedef ctrl2;

	//read the control2 register
	status = i2c_read_reg(AFE_ADDR, SYS_CTRL_2_REG, &ctrl2.data);
	if(status != HAL_OK) return status;

	//set the `discharge` bit to the appropriate value
	ctrl2.map.Dsg_on = enabled;

	//resend the control2 register to the chip
	return i2c_write_reg(AFE_ADDR, SYS_CTRL_2_REG, ctrl2.data);
}

HAL_StatusTypeDef afe_measure_temps(float* ts1, float* ts2) {
	HAL_StatusTypeDef status;

	//read the TS1 and TS2 registers
	uint8_t therm[4];
	status = i2c_read_regs(AFE_ADDR, TS1_REG_BASE, 4, therm);
	if(status != HAL_OK) return status;

	//tack together the two thermistor readings and turn it into a voltage
	float v_th1, v_th2, r_th1, r_th2;
	v_th1 = (((therm[0] && 0x3F) << 8) | therm[1]) * adc_mv_per_lsb + adc_offset;
	v_th2 = (((therm[2] && 0x3F) << 8) | therm[3]) * adc_mv_per_lsb + adc_offset;
	r_th1 = (THERM_PULLUP * v_th1)/(THERM_VREF - v_th1);
	r_th2 = (THERM_PULLUP * v_th2)/(THERM_VREF - v_th2);

	//calculate the temperature values from the thermistor resistance using beta equation
	*ts1 = 1/(1/(THERM_NOM_TEMP + KELVIN_OFFSET) + (1/THERM_BETA) * (float)log((double)r_th1));
	*ts2 = 1/(1/(THERM_NOM_TEMP + KELVIN_OFFSET) + (1/THERM_BETA) * (float)log((double)r_th2));
	return HAL_OK;
}

//vbuffer should be a 10-element array and batBuffer should be a single floating-point value
//returns values in MILLIVOLTS
HAL_StatusTypeDef afe_measure_voltages(float* vBuffer, float* batBuffer) {
	HAL_StatusTypeDef status;

	//read ALLLL the voltage registers
	uint8_t voltages[22]; //includes 2 extras for the batt measurement
	status = i2c_read_regs(AFE_ADDR, CELL_VOLTAGE_REG_BASE, 12, voltages);
	if(status != HAL_OK) return status;

	//concatenate and trim all the voltages we read out
	for(int i = 0; i < 10; i++) {
		vBuffer[i] = (((voltages[i<<1] & 0x3F) << 8) | voltages[(i<<1)+1]) * adc_mv_per_lsb + adc_offset;
	}

	//do some special treatment for the batt register like we are supposed to
	//concatenate, multiply by 4, multiply by gain, add num_cells * offset
	*batBuffer = (float)((voltages[20] << 10) | (voltages[21] << 2)) * adc_mv_per_lsb + (NUM_CELLS * adc_offset);
	return HAL_OK;
}

//just shut down the ADC to save power
HAL_StatusTypeDef afe_sleep() {
	HAL_StatusTypeDef status;

	//read the system control 1 register
	SysCtrl1Typedef ctrl1;
	status = i2c_read_reg(AFE_ADDR, SYS_CTRL_1_REG, &ctrl1.data);
	if(status != HAL_OK) return status;

	//disable the ADC_EN bit
	ctrl1.map.ADC_en = 0;

	//rewrite the data to the control1 register
	return i2c_write_reg(AFE_ADDR, SYS_CTRL_1_REG, ctrl1.data);
}

//power the ADC back up for normal operation
HAL_StatusTypeDef afe_wake() {
	HAL_StatusTypeDef status;

	//read the system control 1 register
	SysCtrl1Typedef ctrl1;
	status = i2c_read_reg(AFE_ADDR, SYS_CTRL_1_REG, &ctrl1.data);
	if(status != HAL_OK) return status;

	//enable the ADC_EN bit
	ctrl1.map.ADC_en = 1;

	//rewrite the data to the control1 register
	return i2c_write_reg(AFE_ADDR, SYS_CTRL_1_REG, ctrl1.data);
}

//make the AFE enter ship mode, disabling the main regulator
HAL_StatusTypeDef afe_shutdown() {
	HAL_StatusTypeDef status;
	SysCtrl1Typedef ctrl1;

	//write 1 -> shutA = 0, shutB = 1
	ctrl1.map.Shut_A = 0;
	ctrl1.map.Shut_B = 1;
	status = i2c_write_reg(AFE_ADDR, SYS_CTRL_1_REG, ctrl1.data);
	if(status != HAL_OK) return status;

	//write 2 -> shutA = 1, shutB = 0
	ctrl1.map.Shut_A = 1;
	ctrl1.map.Shut_B = 0;
	return i2c_write_reg(AFE_ADDR, SYS_CTRL_1_REG, ctrl1.data);
}

//================= Functions in an Interrupt Context ==================
HAL_StatusTypeDef afe_balance(bool* egregious, bool* no_soh, bool* disable_chg) {
	HAL_StatusTypeDef status = HAL_OK;
	static uint32_t balance_time;
	if((HAL_GetTick() - balance_time) > BALANCE_PERIOD) {

		//get all the cell voltages
		float cell_voltages[10];
		float batt_voltage;
		status = afe_measure_voltages(cell_voltages, &batt_voltage);
		if(status != HAL_OK) return status;

		//hunt for the highest and lowest cell voltages
		float upper_vmax, lower_vmax, vmin = 5000; //initialize vmin to something ridiculous
		uint8_t upper_max_cell, lower_max_cell; //indices of the highest voltage cells in the respective banks
		for(int i = 0; i<5; i++) {
			//check the bottom cell bank for the lowest and highest cell voltage
			if(cell_voltages[i] < vmin) vmin = cell_voltages[i]; //compare the lower cell voltage to the lowest recorded
			if(cell_voltages[i] > lower_vmax) { //compare the lower cell voltage to the highest recorded (in the lower bank)
				lower_vmax = cell_voltages[i];
				lower_max_cell = i;
			}

			//check the top cell bank for the lowest and highest cell voltage
			if(cell_voltages[i+5] < vmin) vmin = cell_voltages[i+5]; //compare the upper cell voltage to the lowest recorded
			if(cell_voltages[i+5] > upper_vmax) { //compare the upper cecll voltage to the highest recorded (in the upper bank)
				upper_vmax = cell_voltages[i+5];
				upper_max_cell = i+5;
			}
		}

		//detect egregious imbalance and "invalid state of health" imbalance
		*egregious = *egregious || (upper_vmax - vmin > BALANCE_EGREGIOUS) || (lower_vmax - vmin > BALANCE_EGREGIOUS);
		*no_soh = *no_soh || (upper_vmax - vmin > BALANCE_SOH_THRESHOLD) || (lower_vmax - vmin > BALANCE_SOH_THRESHOLD);

		//and disable charging if cells are too out of balance near the end of the charging process
		*disable_chg = (upper_vmax > BALANCE_VMAX) || (lower_vmax > BALANCE_VMAX);

		//FINALLY, balance the banks if we're above the minimum balancing voltage
		//	and there's a big enough voltage difference between cells
		if(upper_vmax > BALANCE_VMIN && (upper_vmax - vmin) > BALANCE_TOLERANCE) {
			//upper_max_cell can range from 5-9 (in terms of index)
			//and the `afe_balance_upper()` function takes a `1` shifted in the appropriate position corresponding to a channel
			//where 5 corresponds to 1 << 0 and 9 corresponds to 1 << 4
			status = afe_balance_upper(1 << (upper_max_cell - 5));
			if(status != HAL_OK) return status;
		}
		if(lower_vmax > BALANCE_VMIN && (lower_vmax - vmin) > BALANCE_TOLERANCE) {
			//see above for description of the parameter
			//but this time lower_max_cell ranges from 0-4 so we don't need to subtract 5
			status = afe_balance_lower(1 << (lower_max_cell));
			if(status != HAL_OK) return status;
		}

		//update the balance time for the future
		balance_time = HAL_GetTick();
	}
	return status;
}

HAL_StatusTypeDef afe_onAlert(bool* xready, bool* uv, bool* ov) {
	HAL_StatusTypeDef status;

	//simply read the SYS_STAT register and decode it, clear the asserted bits after we read them
	SysStatTypedef dev_stat;
	status = i2c_read_reg(AFE_ADDR, SYS_STAT_REG, &dev_stat.data);
	if(status != HAL_OK) return status;

	*xready = dev_stat.map.X_ready;
	*uv = dev_stat.map.UV;
	*ov = dev_stat.map.OV;

	//clear all flags
	dev_stat.data = 0xFF;
	return i2c_write_reg(AFE_ADDR, SYS_STAT_REG, dev_stat.data);
}

// ================= PRIVATE FUNCTIONS ==================
HAL_StatusTypeDef afe_balance_upper(uint8_t c) {
	if(c != 16 && c != 8 && c != 4 && c != 2 && c != 1) return HAL_ERROR; //not balancing just a single cell in the bank
	return i2c_write_reg(AFE_ADDR, CELL_BAL_2_REG, c);
}

HAL_StatusTypeDef afe_balance_lower(uint8_t c) {
	if(c != 16 && c != 8 && c != 4 && c != 2 && c != 1) return HAL_ERROR; //not balancing just a single cell in the bank
	return i2c_write_reg(AFE_ADDR, CELL_BAL_1_REG, c);
}
