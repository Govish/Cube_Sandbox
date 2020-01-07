#ifndef STUSB4500_H
#define STUSB4500_H

#include "stm32f0xx_hal.h"
#include "stusb_regdefs.h"
#include "stdbool.h"

typedef enum {
	SUPPLY_FIXED = 0b00, SUPPLY_BATT = 0b01, SUPPLY_VAR = 0b10, SUPPLY_APDO = 0b11
} PDOSupplyType;

typedef union {
	uint32_t data;
	struct {
		uint16_t Current : 10;
		uint16_t Voltage : 10;
		uint8_t PeakCurrent : 2;
		uint8_t Reserved : 2;
		uint8_t Unchuncked_Extended : 1;
		uint8_t Dual_RoleData : 1;
		uint8_t Communication : 1;
		uint8_t Extern_Power : 1;
		uint8_t SuspendSupported : 1;
		uint8_t DualRolePower : 1;
		uint8_t FixedSupply : 2;
	} fixed;
	struct {
		uint16_t Operating_Current : 10;
		uint16_t Min_Voltage : 10;
		uint16_t Max_Voltage : 10;
		uint8_t VariableSupply : 2;
	} variable;
	struct {
		uint32_t Operating_Power : 10;
		uint32_t Min_Voltage : 10;
		uint32_t Max_Voltage : 10;
		uint8_t Battery : 2;
	} battery;
} PDOTypedef;

/*
 ======================= PDO MESSAGE STRUCTURES ====================
 https://composter.com.ua/documents/Power-Delivery-Specification.pdf

 ###### FOR FIXED SUPPLIES (SOURCES) #######
    BITS 31:30      Fixed Supply Tag (00)
    BIT 29          Dual role power
    BIT 28          USB Suspend supported   //has something to do with suspending USB device operation after a timeout (7ms?)
    BIT 27          Externally Powered
    BIT 26          USB communications capable
    BIT 25          Dual role data
    BIIT 24:22      Reserved
    BITS 21:20      Peak Current (100%, 130%, 150%, 200%) for values 00 -> 11 respectively
    BITS 19:10      Voltage (1-bit = 50mV)
    BITS 9:0        Current (1-bit = 10mA)

###### FOR VARIABLE SUPPLIES (SOURCES) ######
    BITS 31:30      Variable Supply Tag (10)
    BITS 29:20      Max voltage (1-bit = 50mV)
    BITS 19:10      Min voltage (1-bit = 50mV)
    BITS 9:0        Max current (1-bit = 10mA)

    NOTE: this doesn't actually indicate the voltage that will be supplied, but rather just a range!!!!

###### FOR BATTERY SUPPLIES (SOURCES) ######
    BITS 31:30      Battery Supply Tag (01)
    BITS 29:20      Max voltage (1-bit = 50mV)
    BITS 19:10      Min voltage (1-bit = 50mV)
    BITS 9:0        Max power (1-bit = 250mW)

    batteries don't broadcast a fixed voltage, but rather a voltage range that they'll output
    if powered by a battery source, be careful about not exceeding the power limit
        can do this actively by varying load current
        or just be safe and set the load current to the minimum voltage

//pretty rare PDOs for sink devices, but regardless:
###### FOR FIXED SOURCES (SINK) ######
    BITS 31:30      Fixed Supply Tag (00)
    BIT 29          Dual role power
    BIT 28          Higher Capability
    BIT 27          Externally Powered
    BIT 26          USB communications capable
    BIT 25          Dual role data
    BIIT 24:20      Reserved
    BITS 19:10      Required voltage (1-bit = 50mV)
    BITS 9:0        Operational current (1-bit = 10mA)

###### FOR VARIABLE SUPPLIES (SOURCES) ######
    BITS 31:30      Variable Supply Tag (10)
    BITS 29:20      Max voltage (1-bit = 50mV)
    BITS 19:10      Min voltage (1-bit = 50mV)
    BITS 9:0        Operational current (1-bit = 10mA)

###### FOR BATTERY SUPPLIES (SOURCES) ######
    BITS 31:30      Battery Supply Tag (01)
    BITS 29:20      Max voltage (1-bit = 50mV)
    BITS 19:10      Min voltage (1-bit = 50mV)
    BITS 9:0        Operational power (1-bit = 250mW)

========================= RDO MESSAGE STRUCTURES (SINK ONLY) =========================

##### FIXED AND VARIABLE RDO's #######
    BIT 31          Reserved (set to 0)
    BITS 30:28      PDO object position (001 - 111)
    BIT 27          Giveback flag (set to 0, has something to deal with load limiting)
    BIT 26          Capability Mismatch
    BIT 25          USB Communications Capable
    BIT 24          No USB Suspend (set to 1)
    BIT 23          Unchunked Messages Supported (just set to 0 lol)
    BITS 22:20      Reserved (set to 0)
    BITS 19:10      Operating current (1-bit = 10mA)
    BITS 9:0        Max operating current (1-bit = 10mA)

###### BATTERY RDO's ######
    BIT 31          Reserved (set to 0)
    BITS 30:28      PDO object position (001-111)
    BIT 27          Giveback flag (set to 0)
    BIT 26          Capability mismatch
    BIT 25          USB comm capable
    BIT 24          No USB Suspend (set to 1)
    BIT 23          Unchunked Extended Messages Supported (set to 0)
    BITS 22:20      Reserved (set to 0)
    BITS 19:10      Operating power (1-bit = 250mW)
    BITS 9:0        Operating power (1-bit = 250mW)
*/

// =========== Public Functions ============

HAL_StatusTypeDef pd_init();
HAL_StatusTypeDef pd_soft_reset();	//call to soft-reset the STUSB4500
HAL_StatusTypeDef pd_send_soft_reset();	//call to send a soft-reset on the active CC line
HAL_StatusTypeDef pd_read_sink_pdos(uint8_t* num_pdos, PDOTypedef* pdos);	//read how many pdo's we have set up and what they are
																			//pass this a pointer to a char and a pointer to an array of 3 PDOTypedefs
HAL_StatusTypeDef pd_get_source_pdos(uint8_t* num_pdos, PDOTypedef* pdos);	//read how many pdo's we have received and what they are
																			//pass this a pointer to a char and a pointer to an array of 8 PDOTypedefs
HAL_StatusTypeDef pd_read_rdo(PDOTypedef* rdo);	//read the currently active RDO
HAL_StatusTypeDef pd_update_sink_pdo(uint8_t pdo_index, PDOTypedef* new_pdo); //update the PDO at a particular index
HAL_StatusTypeDef pd_update_num_pdos(uint8_t num_active_pdos);	//set the number of active PDOs (1-3, values will be constrained)
HAL_StatusTypeDef pd_typec_info(CC_Reg_Map* infobuf);	//call this to read the CC_STATUS_REGISTER; will have to decipher using struct encodings
HAL_StatusTypeDef pd_request_pdo_num(uint8_t pdo_index);	//call this to negotiate for a particular PDO in the received PDOs
HAL_StatusTypeDef pd_auto_nego(float* voltage, float* current, float* power);	//call this to automatically negotiate a contract
																				//type C device must be attached for nego to be successful
bool pd_attached(); //to read the internal flag
//=============== Functions called from INTERRUPT CONTEXT ===================
HAL_StatusTypeDef pd_onAttach();
HAL_StatusTypeDef pd_onDetach();
HAL_StatusTypeDef pd_onAlert();

#endif //include protection
