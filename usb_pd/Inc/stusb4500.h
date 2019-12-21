#ifndef STUSB4500_H
#define STUSB4500_H

// =========== Public Functions ============

HAL_StatusTypeDef pd_init();
HAL_StatusTypeDef pd_soft_reset();	//call to soft-reset the STUSB4500
HAL_StatusTypeDef pd_send_soft_reset();	//call to send a soft-reset on the active CC line

HAL_StatusTypeDef pd_read_sink_pdos(uint8_t* num_pdos, uint32_t* pdos); //read how many pdo's we have set up and what they are
																		//pass this a pointer to a char and a pointer to an array of 3 longs
HAL_StatusTypeDef pd_read_rdo(uint32_t* rdo);	//read the currently active RDO /*TODO change the type of the pointer to a PDO struct*/

HAL_StatusTypeDef pd_update_sink_pdo(uint8_t pdo_index, uint32_t new_pdo); //update the PDO at a particular index

HAL_StatusTypeDef pd_update_num_pdos(uint8_t num_active_pdos);	//set the number of active PDOs (1-3, values will be constrained)

HAL_StatusTypeDef pd_typec_info(CC_Reg_Map* infobuf);	//call this to read the CC_STATUS_REGISTER; will have to decipher using struct encodings

HAL_StatusTypeDef pd_request_pdo_num(uint8_t pdo_index);	//call this to negotiate for a particular PDO in the received PDOs

HAL_StatusTypeDef pd_auto_nego(float* voltage, float* current, float* power);	//call this to automatically negotiate a contract
																				//type C device must be attached for nego to be successful

#endif