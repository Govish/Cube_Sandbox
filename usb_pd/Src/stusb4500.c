#include "stusb4500.h"
#include "i2c.h"

//================== PRIVATE VARIABLES ==================
volatile static uint8_t num_source_pdos = 0; //variable to store how many source PDOs we've read, updated from interrupt context!
volatile static PDOTypedef source_pdos[8]; //a little buffer to store all those source PDOs, updated from interrupt context!

volatile static bool pdo_received; //flag that says if we're received any fresh PDOs
volatile static bool accept_received; //flag that says if we've received an accept message
volatile static bool reject_received; //flag that says if we've received a reject message
volatile static bool psready_received; //flag that says if we've received a PS_RDY message

volatile static bool attached; //just a little flag variable that makes things convenient, updated from interrupt context!

//================== PUBLIC FUNCTIONS ===================
HAL_StatusTypeDef pd_init() {
	HAL_StatusTypeDef status;

	//read the deviceID, make sure it's 0x21 or 0x25
	uint8_t buf;
	status = i2c_read_reg(STUSB_ADDR, DEVICE_ID_REG, &buf);
	if(buf != 0x21 && buf != 0x25) return HAL_ERROR;	//return error if we don't get an expected deviceID
	if(status != HAL_OK) return status;

	//enable the interrupts that we care about
	//don't need to worry about the attach detection, phy interrupts, and type-c status
	Alt_S1Reg_Map alt_sreg_mask;
	alt_sreg_mask.data = 0xFF; //mask everything at the start
	alt_sreg_mask.map.prt_mask = 0; //interrupt when we receive a new protocol message
	alt_sreg_mask.map.tcmon_mask = 0; //interrupt when vbus crosses an invalid threshold
	alt_sreg_mask.map.ccfault_mask = 0; //interrupt when we detect an error on the CC lines
	alt_sreg_mask.map.hreset_mask = 0; //interrupt when we receive a hard reset
	status = i2c_write_reg(STUSB_ADDR, ALERT_STATUS_1_MASK_REG, alt_sreg_mask.data);
	if(status != HAL_OK) return status;

	//read 12 bytes starting at 0x0B (the ALERT_STATUS_1_REG buffer)
	//not 100% sure why we have to do this but the sample code does this?
	uint8_t throwaway[12];
	return i2c_read_regs(STUSB_ADDR, ALERT_STATUS_1_REG, 12, throwaway);
}

HAL_StatusTypeDef pd_soft_reset() {
	HAL_StatusTypeDef status;

	//write '1' to the LSB of the RESET_CTRL_REGISTER
	status = i2c_write_reg(STUSB_ADDR, RESET_CTRL_REG, 0x01);
	if(status != HAL_OK) return status;

	//read the 12 bytes starting at the ALERT_STATUS_1 register again (not sure why but¯\_(ツ)_/¯)
	uint8_t throwaway[12];
	status = i2c_read_regs(STUSB_ADDR, ALERT_STATUS_1_REG, 12, throwaway);
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
	return i2c_write_reg(STUSB_ADDR, STUSB_GEN1S_CMD_CTRL_REG, 0x26);
}

HAL_StatusTypeDef pd_read_sink_pdos(uint8_t* num_pdos, PDOTypedef* pdos) {
	HAL_StatusTypeDef status;

	//read from DPM_PDO_NUMB register (0x70) to find the number of PDO's available
	//just remember to dereference the pointer to num_pdos correctly!
	status = i2c_read_reg(STUSB_ADDR, DPM_PDO_NUMB_REG, num_pdos);
	if(status != HAL_OK) return status;
	*num_pdos &= 0x03; //mask only the bottom two bits

	//had some sort of read error if this number of PDOs we read is out of the expected range
	if(*num_pdos < 1) return HAL_ERROR;

	//depending on the number of pdo's we have, read that many bytes x4 from the PDO registers
	//we're gonna have to do some weird concatenation scheme from the buffers though before returning them
	uint8_t pdo_data[*num_pdos << 2];
	status = i2c_read_regs(STUSB_ADDR, DPM_SNK_PDO1, *num_pdos << 2, pdo_data);
	if(status != HAL_OK) return status;

	//time to reformat all the data we read from the registers
	for(int i = 0; i < *num_pdos; i++) {
		//the earlier indices correspond to the earlier registers
		//and each PDO is 32 bits, so we'll concatenate 4 registers together
		pdos[i].data = pdo_data[i<<2] | (pdo_data[1+(i<<2)] << 8) | (pdo_data[2+(i<<2)] << 16) | (pdo_data[3+(i<<2)] << 24);
	}
	return status;
}

HAL_StatusTypeDef pd_get_source_pdos(uint8_t* num_pdos, PDOTypedef* pdos) {
	//copy the values from the buffers instead of associating the pointers
	//will prevent any weird stuff from happening if we modify the values of the pointers we passed
	*num_pdos = num_source_pdos;
	for(int i = 0; i < num_source_pdos; i++) pdos[i] = source_pdos[i];

	return HAL_OK;
}

HAL_StatusTypeDef pd_read_rdo(PDOTypedef* rdo) {
	HAL_StatusTypeDef status;

	uint8_t rdo_data[4];
	status = i2c_read_regs(STUSB_ADDR, DPM_REQ_RDO1, 4, rdo_data);

	//concatenate the four bytes into one PDO structure which we passed
	rdo->data = (rdo_data[0]) | (rdo_data[1] << 8) | (rdo_data[2] << 16) | (rdo_data[3] << 24);

	return status;
}

HAL_StatusTypeDef pd_update_sink_pdo(uint8_t pdo_index, PDOTypedef* new_pdo) {
	//break down the new PDO into 4 byte messages
	//we'll save the first index for the particular pdo register that we want to update
	uint8_t pdo_buf[5];
	pdo_buf[1] = new_pdo->data & 0xFF;
	pdo_buf[2] = (new_pdo->data >> 8) & 0xFF;
	pdo_buf[3] = (new_pdo->data >> 16) & 0xFF;
	pdo_buf[4] = (new_pdo->data >> 24) & 0xFF;

	//only allow updates to PDO2 and PDO3 (PDO1 MUST be 5V so we get some sort of nego
	if(pdo_index == 2) {
		pdo_buf[0] = DPM_SNK_PDO2;
		return i2c_write_regs(STUSB_ADDR, pdo_buf, 5);
	}
	else if(pdo_index == 3) {
		pdo_buf[0] = DPM_SNK_PDO3;
		return i2c_write_regs(STUSB_ADDR, pdo_buf, 5);
	}

	//this means that the PDO index wasn't 2 or 3, so the PDO wasn't updated
	return HAL_ERROR;
}

HAL_StatusTypeDef pd_update_num_pdos(uint8_t num_active_pdos) {
	//constrain the number of active pdos from 1-3
	num_active_pdos = num_active_pdos < 1 ? 1 : (num_active_pdos > 3 ? 3 : num_active_pdos);

	//write that value to the DPM_PDO_NUMB register
	return i2c_write_reg(STUSB_ADDR, DPM_PDO_NUMB_REG, num_active_pdos);
}

HAL_StatusTypeDef pd_typec_info(CC_Reg_Map* infobuf) {
	//basically just reading from the CC_STATUS register and returning the value
	return i2c_read_reg(STUSB_ADDR, CC_STATUS_REG, &infobuf->data);
}

HAL_StatusTypeDef pd_request_pdo_num(uint8_t pdo_index) {
	HAL_StatusTypeDef status;

	//sanity check our index
	if(pdo_index < 1 || pdo_index > num_source_pdos) return HAL_ERROR;

	//read the PDO at our index
	PDOTypedef PDO_to_request;
	PDO_to_request = source_pdos[pdo_index-1];

	printf("Requesting this PDO: 0x%lx\n", PDO_to_request.data);

	//write that PDO to index 2 and set the number of PDOs to 2
	status = pd_update_sink_pdo(2, &PDO_to_request);
	if(status != HAL_OK) return status;
	status = pd_update_num_pdos(2);
	if(status != HAL_OK) return status;

	//send a soft reset command to force renegotiation
	return pd_send_soft_reset();
}

/* TODO: write pd_auto_nego function! (namely how retries work) */
//call this to automatically negotiate a contract
//type C device must be attached for nego to be successful
HAL_StatusTypeDef pd_auto_nego(float* voltage, float* current, float* power) {
	HAL_Delay(NEGO_WAIT_TIME); //wait for the source to chill out for a little

	/* ========= prepare to negotiate ========= */
	HAL_StatusTypeDef status;
	pdo_received = false; //clear the PDO received flag
	uint8_t retry_counter = NEGO_NUM_RETRIES;
	uint32_t timeout_counter = HAL_GetTick();

	/* ========== Actually do the negotiation, with finite retries and timeouts =========== */
	while(true) {
		//send a soft reset
		status = pd_send_soft_reset();
		if(status != HAL_OK) return status;

		//do nothing until we either receive a PDO or timeout
		while(!pdo_received && (HAL_GetTick() - timeout_counter) < NEGO_WAIT_TIME);

		//if we received a PDO, break from this loop
		if(pdo_received) break;

		//decrement the retry counter, return a timeout if we retry too many times
		retry_counter--;
		if(retry_counter == 0) return HAL_TIMEOUT;
	}

	/* ======= if we've made it this far, we have PDOs to parse through ======= */
	//the first PDO is always 5V and has some weird readouts
	//so we'll just never charge if we only have a single PDO
	if(num_source_pdos <= 1) return HAL_TIMEOUT;

	//iterate over all the PDOs except the first one
	int8_t select_pdo_index = -1; //our "invalid" signal
	float max_power = -1; //holds the peak power that we can draw from the source
	//defining a bunch floating point numbers again
	float pdo_voltage, pdo_vmin, pdo_vmax, pdo_current, pdo_power, duty_cycle, i_ripple, chi, chi_min, chi_max;
	for(int i = 1; i < num_source_pdos; i++) {
		PDOTypedef pdo = source_pdos[i]; //just kinda convenient to manipulate the pdo this way

		//A valid PDO will have a minimum power (so that we have a high enough charge current)
		//and the inductor current and the ripple current have to be within a certain ratio
		//this ratio is called the Chi Factor, and should be between 0.2 and 0.4
		//	(see page 14 of this datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/3783fb.pdf)
		//in our case, inductor average current is our PDO current
		//and our ripple current is a function of some converter constants and PDO voltage

 		//treat the different pdo types differently
		switch(pdo.type.identifier) {
		case SUPPLY_FIXED :
			pdo_voltage = pdo.fixed.Voltage * 0.05; //1 LSB -> 50mV
			pdo_current = pdo.fixed.Current * 0.01; //1 LSB -> 10mA

			//calculate chi factor
			duty_cycle = (BATTERY_FLOAT_V + DIODE_VF - pdo_voltage)/(BATTERY_FLOAT_V + DIODE_VF);
			i_ripple = (pdo_voltage*duty_cycle/CHARGER_FSW)/L_INDUCTOR;
			chi = i_ripple/pdo_current;

			//calculate PDO power
			pdo_power = pdo_voltage * pdo_current;

			//check if chi is within range, power is above the minimum acceptable, and greater than our peak recorded power
			if(	chi >= CHI_MIN && chi <= CHI_MAX &&
				pdo_power >= MIN_CHARGE_POWER && pdo_power >= max_power) {
				//if it is, remember this index and the power level
				select_pdo_index = i;
				max_power = pdo_power;
			}
			break;

		case SUPPLY_BATT :
			pdo_vmin = pdo.battery.Min_Voltage * 0.05; //1 LSB -> 50mV
			pdo_vmax = pdo.battery.Max_Voltage * 0.05; //1 LSB -> 50mV
			pdo_power = pdo.battery.Operating_Power * 0.25; //1 LSB -> 250mW

			//calculate chi factors for the minimum and maximum voltages respectively
			duty_cycle = (BATTERY_FLOAT_V + DIODE_VF - pdo_vmin)/(BATTERY_FLOAT_V + DIODE_VF);
			i_ripple = (pdo_vmin*duty_cycle/CHARGER_FSW)/L_INDUCTOR;
			chi_min = i_ripple/(pdo_power/pdo_vmin); //conservative estimate
			duty_cycle = (BATTERY_FLOAT_V + DIODE_VF - pdo_vmax)/(BATTERY_FLOAT_V + DIODE_VF);
			i_ripple = (pdo_vmax*duty_cycle/CHARGER_FSW)/L_INDUCTOR;
			chi_max = i_ripple/(pdo_power/pdo_vmax); //conservative estimate

			//check if chi is within range, power is above the minimum acceptable, and greater than our peak recorded power
			if(	chi_min >= CHI_MIN && chi_max <= CHI_MAX &&
				pdo_power >= MIN_CHARGE_POWER && pdo_power >= max_power) {
				//if it is, remember this index and the power level
				select_pdo_index = i;
				max_power = pdo_power;
			}
			break;

		case SUPPLY_VAR :
			pdo_vmin = pdo.variable.Min_Voltage * 0.05; //1 LSB -> 50mV
			pdo_vmax = pdo.variable.Max_Voltage * 0.05; //1 LSB -> 50mV
			pdo_current = pdo.variable.Operating_Current * 0.01; //1 LSB -> 10mA

			//calculate chi factors for the minimum and maximum voltages respectively
			duty_cycle = (BATTERY_FLOAT_V + DIODE_VF - pdo_vmin)/(BATTERY_FLOAT_V + DIODE_VF);
			i_ripple = (pdo_vmin*duty_cycle/CHARGER_FSW)/L_INDUCTOR;
			chi_min = i_ripple/pdo_current;
			duty_cycle = (BATTERY_FLOAT_V + DIODE_VF - pdo_vmax)/(BATTERY_FLOAT_V + DIODE_VF);
			i_ripple = (pdo_vmax*duty_cycle/CHARGER_FSW)/L_INDUCTOR;
			chi_max = i_ripple/pdo_current;

			//calculate PDO power conservatively
			pdo_power = pdo_vmin * pdo_current;

			//check if chi is within range, power is above the minimum acceptable, and greater than our peak recorded power
			if(	chi_min >= CHI_MIN && chi_max <= CHI_MAX &&
				pdo_power >= MIN_CHARGE_POWER && pdo_power >= max_power) {
				//if it is, remember this index and the power level
				select_pdo_index = i;
				max_power = pdo_power;
			}
			break;

		default : break;
		}
	}
	if(select_pdo_index < 0) return HAL_TIMEOUT; //none of the PDOs worked, so return this

	/* ======= Alright, so we've picked a PDO by now ==== */
	//reset the accept and reject flags
	accept_received = false;
	reject_received = false;
	status = pd_request_pdo_num(select_pdo_index); //ask for the PDO we picked
	if(status != HAL_OK) return status;

	//wait for an accept, reject, or a timeout
	timeout_counter = HAL_GetTick();
	while((HAL_GetTick() - timeout_counter) < NEGO_WAIT_TIME && !accept_received && !reject_received);

	if(accept_received) {
		PDOTypedef pdo = source_pdos[select_pdo_index];
		switch(pdo.type.identifier) {
		case SUPPLY_FIXED :
			*voltage = pdo.fixed.Voltage;
			*current = pdo.fixed.Current;
			*power = (*voltage) * (*current);
			return HAL_OK;
		case SUPPLY_BATT:
			*voltage = pdo.battery.Min_Voltage; //conservative
			*power = pdo.battery.Operating_Power;
			*current = *voltage / pdo.battery.Max_Voltage; //also conservative
			return HAL_OK;
		case SUPPLY_VAR:
			*voltage = pdo.variable.Min_Voltage; //conservative
			*current = pdo.variable.Operating_Current;
			*power = (*voltage) * (*current);
			return HAL_OK;
		}
	}

	return HAL_TIMEOUT;
}

bool pd_attached() {
	return attached;
}

HAL_StatusTypeDef pd_onAttach() {
	HAL_StatusTypeDef status;

	//perform a register soft reset of the chip
	//status = pd_soft_reset();
	//if(status != HAL_OK) return status;

	//read from the PORT_STATUS_1 register to make sure we're connected to a source
	uint8_t ps1_reg;
	status = i2c_read_reg(STUSB_ADDR, PORT_STATUS_1_REG, &ps1_reg);
	if(status != HAL_OK) return status;

	//if we're attached and attached to a source
	if((ps1_reg & PS1_CONNECTION_BITS) == ATTACHED_SOURCE) {
		status = pd_init(); //initialize the STUSB4500
		if(status != HAL_OK) return status;

		attached = true; //set the attached flag
		printf("Attached!\n");
	} else {
		attached = false;
		printf("Attached, but not to a source! PORT_STATUS_1 reg contents: 0x%x\n", ps1_reg);
	}
	return status;
}

HAL_StatusTypeDef pd_onDetach() {
	//just reset the flags and stuff on the detach event
	if(attached == true) {
		attached = false;
		num_source_pdos = 0;
		for(int i = 0; i < 8; i++) source_pdos[i].data = 0;
		printf("Detached!\n");
	}
	return HAL_OK;
}

HAL_StatusTypeDef pd_onAlert() {
	if(!attached) return HAL_OK; //just return early if the function call isn't relevant

	HAL_StatusTypeDef status;

	//read ALERT_STATUS_1 and ALERT_STATUS_1_MASK (invert this) and bitwise AND them together
	//left with the interrupt sources that we care about
	uint8_t alert_regs[2]; //goes STATUS, then MASK
	status = i2c_read_regs(STUSB_ADDR, ALERT_STATUS_1_REG, 2, alert_regs);
	if(status != HAL_OK) return status;
	Alt_S1Reg_Map alert_sources;
	alert_sources.data = alert_regs[0] & ~alert_regs[1]; //invert since 0 UNmasks the interrupt

	//if there's an alert due to a hard reset
	if(alert_sources.map.hreset_mask) {
		printf("Received Hard Reset!\n");
		//just let the chip deassert attach and reassert it
		//should take care of things itself
		// I THINK??? (i hope lol)
	}

	//if there's some event involving VBUS
	if(alert_sources.map.tcmon_mask) {
		printf("VBus Event!\n");
		uint8_t tcmon_regs[2];
		status = i2c_read_regs(STUSB_ADDR, TYPEC_MON_STATUS_0_REG, 2, tcmon_regs);
		if(status != HAL_OK) return status;

		//TODO: figure out how to handle these vbus events
		//will most likely update a flag variable when the VBUS goes valid
		//just like how we're doing when we receive protocol events
	}

	//if there's some hardware fault with the CC lines
	if(alert_sources.map.ccfault_mask) {
		printf("CC Fault Event!\n");
		uint8_t ccfault_regs[2];
		status = i2c_read_regs(STUSB_ADDR, CC_FAULT_STATUS_0_REG, 2, ccfault_regs);
		if(status != HAL_OK) return status;

		//TODO: figure out how to handle these cc line hardware faults
	}

	//if there's a protocol event
	if(alert_sources.map.prt_mask) {

		//read from the protocol status register
		uint8_t prt_reg;
		status = i2c_read_reg(STUSB_ADDR, PROTOCOL_STATUS_REG, &prt_reg);
		if(status != HAL_OK) return status;

		//if we've received a new message from the source
		if(prt_reg & PRTL_MESSAGE_RECEIVED) {
			//printf("Message Received!\n");

			//read the RX_HEADER registers
			uint8_t header_regs[2];
			PDHeaderTypedef header;
			status = i2c_read_regs(STUSB_ADDR, RX_HEADER_REG, 2, header_regs);
			if(status != HAL_OK) return status;
			header.data = (header_regs[1] << 8) | header_regs[0];

			//printf("\tHeader: 0x%x\n", header.data);

			//determine if the header is a data or control message
			if(header.map.num_objects == 0) {
				//control message
				//simply update the flag variables here
				if(header.map.message_type == CONTROL_ACCEPT) {
					accept_received = true;
					//printf("Received an Accept!\n");
				} else if(header.map.message_type == CONTROL_REJECT) {
					reject_received = true;
					//printf("Received a Reject\n");
				} else if(header.map.message_type == CONTROL_PSRDY) {
					psready_received = true;
					//printf("Supply ready\n");
				}
			} else {
				//data message
				//if it's a source capabiliites message
				if(header.map.message_type == DATA_SOURCECAP) {
					//read the number of bytes received
					uint8_t num_objects;
					num_objects = header.map.num_objects;

					//read the received data objects
					// 4 bytes per object
					uint8_t rx_data_objects[28];
					status = i2c_read_regs(STUSB_ADDR, RX_DATA_OBJ_REG, 28/*num_objects << 2*/, rx_data_objects);
					if(status != HAL_OK) return status;

					//stitch the PDOs together and store them into the static pdo array
					for(int i = 0; i < num_objects; i++) {
						source_pdos[i].data = 	(rx_data_objects[(i<<2)]) |
												(rx_data_objects[(i<<2) + 1] << 8)  |
												(rx_data_objects[(i<<2) + 2] << 16) |
												(rx_data_objects[(i<<2) + 3] << 24);
					}

					num_source_pdos = num_objects; //and update the `num_source_pdos` as well
					pdo_received = true; //update the flag

					printf("Received PDOs!\n");

				} //source capabilities
			} //data message
		} //protocol event message received
	} //protocol event

	return status;
}
