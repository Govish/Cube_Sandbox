#include "stusb4500.h"
#include "stusb_regdefs.h"

//TODO: update uint32_t's representing PDOs to the PDOTypedef

HAL_StatusTypeDef pd_init() {
	HAL_StatusTypeDef status;

	//read the deviceID, make sure it's 0x21 or 0x25
	uint8_t buf;
	status = i2c_read_reg(STUSB_ADDR, DEVICE_ID_REG, &buf);
	if(buf != 0x21 || buf != 0x25) return HAL_ERROR;	//return error if we don't get an expected deviceID
	if(status != HAL_OK) return status;

	/*
	 * unmask the:
	 * 	PRT_STATUS_AL_MASK (bit 1)
	 * 	PD_TYPEC_STATUS_AL_MASK (bit 3)
	 * 	CC_DETECTION_STATUS_AL_MASK (bit 6) typo in datasheet
	 * 	HARD_RESET_AL_MASK (bit 7)
	 * 	in the ALERT_STATUS_1_MASK register
	 */
	Alt_S1Reg_Map alt_sreg_mask;
	alt_sreg_mask.data = 0xFF; //mask everything at the start
	alt_sreg_mask.map.prt_mask = 0;
	alt_sreg_mask.map.tcstat_mask = 0;
	alt_sreg_mask.map.ccdetect_mask = 0;
	alt_sreg_mask.map.hreset_mask = 0;
	status = i2c_write_reg(STUSB_ADDR, ALERT_STATUS_1_MASK_REG, alt_sreg_mask.data);
	if(status != HAL_OK) return status;

	//read 12 bytes starting at 0x0B (the ALERT_STATUS_1_REG buffer)
	//not 100% sure why we have to do this but the sample code does this?
	uint8_t throwaway[12];
	return i2c_read_regs(STUSB_ADDR, ALERT_STATUS_1_REG, 12, &throwaway);
}

HAL_StatusTypeDef pd_soft_reset() {
	HAL_StatusTypeDef status;

	//write '1' to the LSB of the RESET_CTRL_REGISTER
	status = i2c_write_reg(STUSB_ADDR, RESET_CTRL_REG, 0x01);
	if(status != HAL_OK) return status;

	//read the 12 bytes starting at the ALERT_STATUS_1 register again (not sure why but¯\_(ツ)_/¯)
	uint8_t throwaway[12];
	status = i2c_read_regs(STUSB_ADDR, ALERT_STATUS_1_REG, 12, &throwaway);
	if(status != HAL_OK) return status;

	//wait like 25ish milliseconds according to the sample code
	HAL_Delay(25);

	//write '0' to the LSB of the RESET_CTRL_REGISTER
	return i2c_write_reg(STUSB_ADDR, RESET_CTRL_REG, 0x00);
}

HAL_StatusTypeDef pd_send_soft_reset() {
	HAL_StatusTypeDef status;

	//dude like this entire "methodology" isn't in the datasheet and uses "ghost" registers lol
	//like none of these registers are in the datasheet
	//but just following what the sample code is doingggggg

	//send 0x0D to the TX_HEADER register (x51, not in the datasheet)
	status = i2c_write_reg(STUSB_ADDR, TX_HEADER_REG, 0x0D);
	if(status != HAL_OK) return status;

	//send 0x26 to the STUSB_GEN1S_CMD_CTRL register (x1A, not in the datasheet)
	return i2c_write_regs(STUSB_ADDR, STUSB_GEN1S_CMD_CTRL_REG, 0x26);
}

HAL_StatusTypeDef pd_read_sink_pdos(uint8_t* num_pdos, uint32_t* pdos) {
	//TODO: change the uint32_t to a PDOTypedef union

	HAL_StatusTypeDef status;

	//read from DPM_PDO_NUMB register (0x70) to find the number of PDO's available
	//just remember to dereference the pointer to num_pdos correctly!
	status = i2c_read_reg(STUSB_ADDR, DPM_PDO_NUMB_REG, num_pdos);
	if(status != HAL_OK) return status;
	*num_pdos &= 0x07; //mask only the bottom three bits

	//had some sort of read error if this number of PDOs we read is out of the expected range
	if(*num_pdos > 3 || *num_pdos < 1) return HAL_ERROR;

	//depending on the number of pdo's we have, read that many bytes x4 from the PDO registers
	//we're gonna have to do some weird concatenation scheme from the buffers though before returning them
	uint8_t pdo_data[*num_pdos << 2];
	status = i2c_read_regs(STUSB_ADDR, DPM_SNK_PDO1, *num_pdos << 2, &pdo_data);
	if(status != HAL_OK) return status;

	//time to reformat all the data we read from the registers
	for(int i = 0; i < *num_pdos; i++) {
		//the earlier indices correspond to the earlier registers
		//and each PDO is 32 bits, so we'll concatenate 4 registers together
		pdos[i] = pdo_data[i<<2] | pdo_data[1+i<<2] << 8 | pdo_data[2+i<<2] << 16 | pdo_data[3+i<<2] << 24;
	}
	return status;
}

HAL_StatusTypeDef pd_read_rdo(uint32_t* rdo) {
	//TODO change the UINT32_T to a PDOTypedef union
	HAL_StatusTypeDef status;

	uint8_t rdo_data[4];
	status = i2c_read_regs(STUSB_ADDR, DPM_REQ_RDO1, 4, &rdo_data);

	//concatenate the four bytes into one 32 bit long which we passed
	*rdo = rdo_data[0] | rdo_data[1] << 8 | rdo_data[2] << 16 | rdo_data << 24;

	return status;
}

//TODO: change the uint32_t to a PDOTypedef message
HAL_StatusTypeDef pd_update_sink_pdo(uint8_t pdo_index, uint32_t new_pdo) {
	HAL_StatusTypeDef status;

	//break down the new PDO into 4 byte messages
	//we'll save the first index for the particular pdo register that we want to update
	uint8_t pdo_buf[5];
	pdo_buf[1] = new_pdo & 0xFF;
	pdo_buf[2] = (new_pdo >> 8) & 0xFF;
	pdo_buf[3] = (new_pdo >> 16) & 0xFF;
	pdo_buf[4] = (new_pdo >> 24) & 0xFF;

	//only allow updates to PDO2 and PDO3 (PDO1 MUST be 5V so we get some sort of nego
	if(pdo_index == 2) {
		pdo_buf[0] = DPM_SNK_PDO2;
		return i2c_write_regs(STUSB_ADDR, &pdo_buf, 5);
	}
	else if(pdo_index == 3) {
		pdo_buf[0] = DPM_SNK_PDO3;
		return i2c_write_regs(STUSB_ADDR, &pdo_buf, 5);
	}

	//this means that the PDO index wasn't 2 or 3, so the PDO wasn't updated
	return HAL_ERROR;
}

HAL_StatusTypeDef pd_update_num_pdos(uint8_t num_active_pdos) {
	HAL_StatusTypeDef status;

	//constrain the number of active pdos from 1-3
	num_active_pdos = num_active_pdos < 1 ? 1 : (num_active_pdos > 3 ? 3 : num_active_pdos);

	//write that value to the DPM_PDO_NUMB register
	return i2c_write_reg(STUSB_ADDR, DPM_PDO_NUMB_REG, num_active_pdos);
}

HAL_StatusTypeDef pd_typec_info(CC_Reg_Map* infobuf) {
	//basically just reading from the CC_STATUS register and returning the value
	return i2c_read_reg(STUSB_ADDR, CC_STATUS_REG, infobuf->data);
}

HAL_StatusTypeDef pd_request_pdo_num(uint8_t pdo_index) {
	HAL_StatusTypeDef status;

	/*
	void request_pdo_number(uint8_t pdo_index) {
	    the way this function works is it looks at the PDO object received by the STUSB4500 at the particular index
	    and just copies it into the PDO2 register

	    feels kinda dumb but apparently it works
	}
	 */
}

HAL_StatusTypeDef pd_auto_nego(float* voltage, float* current, float* power);	//call this to automatically negotiate a contract
																				//type C device must be attached for nego to be successful

