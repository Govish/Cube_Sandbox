/**
  ******************************************************************************
  * File Name          : I2C.c
  * Description        : This file provides code for the configuration
  *                      of the I2C instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */
static uint8_t BUS_IN_USE; //a little semaphore since HAL_LOCK is a little sus
/* USER CODE END 0 */

I2C_HandleTypeDef hi2c2;

/* I2C2 init function */
void MX_I2C2_Init(void)
{

  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00506682;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter 
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter 
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C2)
  {
  /* USER CODE BEGIN I2C2_MspInit 0 */

  /* USER CODE END I2C2_MspInit 0 */
  
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C2 GPIO Configuration    
    PB13     ------> I2C2_SCL
    PB14     ------> I2C2_SDA 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C2 clock enable */
    __HAL_RCC_I2C2_CLK_ENABLE();
  /* USER CODE BEGIN I2C2_MspInit 1 */

  /* USER CODE END I2C2_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C2)
  {
  /* USER CODE BEGIN I2C2_MspDeInit 0 */

  /* USER CODE END I2C2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C2_CLK_DISABLE();
  
    /**I2C2 GPIO Configuration    
    PB13     ------> I2C2_SCL
    PB14     ------> I2C2_SDA 
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13|GPIO_PIN_14);

  /* USER CODE BEGIN I2C2_MspDeInit 1 */

  /* USER CODE END I2C2_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */

//a couple wrapper functions to make I2C transactions a little easier
HAL_StatusTypeDef i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t* buf) {
	HAL_StatusTypeDef status;
	if(BUS_IN_USE) return HAL_BUSY;
	BUS_IN_USE = 1; //lock the bus
	status = HAL_I2C_Master_Transmit(&hi2c2, addr, &reg, 1, 1); //1ms timeout
	if(status == HAL_OK) status = HAL_I2C_Master_Receive(&hi2c2, addr, buf, 1, 1); //1ms timeout
	BUS_IN_USE = 0;//unlock the bus
	return status;
}

HAL_StatusTypeDef i2c_write_command(uint8_t addr, uint8_t command) {
	HAL_StatusTypeDef status;
	if(BUS_IN_USE) return HAL_BUSY;
	BUS_IN_USE = 1; //lock the bus
	status = HAL_I2C_Master_Transmit(&hi2c2, addr, &command, 1, 1); //1ms timeout
	BUS_IN_USE = 0; //unlock the bus
	return status;
}

HAL_StatusTypeDef i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t data) {
	uint8_t txdata[2];
	HAL_StatusTypeDef status;
	txdata[0] = reg;
	txdata[1] = data;
	if(BUS_IN_USE) return HAL_BUSY;
	BUS_IN_USE = 1; //lock the bus
	status = HAL_I2C_Master_Transmit(&hi2c2, addr, txdata, 2, 1); //1ms timeout
	BUS_IN_USE = 0; //unlock the bus
	return status;
}

HAL_StatusTypeDef i2c_print_reg(uint8_t addr, uint8_t reg) {
	HAL_StatusTypeDef status;
	if(BUS_IN_USE) return HAL_BUSY;
	BUS_IN_USE = 1; //lock the bus
	status = HAL_I2C_Master_Transmit(&hi2c2, addr, &reg, 1, 1); //1ms timeout
	if(status == HAL_OK) {
		uint8_t buf;
		status = HAL_I2C_Master_Receive(&hi2c2, addr, &buf, 1, 1); //1ms timeout
		BUS_IN_USE = 0; //unlock the bus
		printf(" 0x%x\r\n", buf);
	}
	else {
		printf("\n");
		BUS_IN_USE = 0; //unlock the bus
	}
	return status;
}

HAL_StatusTypeDef i2c_read_regs(uint8_t addr, uint8_t reg_start, uint8_t num_regs, uint8_t* read_data) {
	HAL_StatusTypeDef status;
	if(BUS_IN_USE) return HAL_BUSY;
	BUS_IN_USE = 1; //lock the bus
	status = HAL_I2C_Master_Transmit(&hi2c2, addr, &reg_start, 1, 1); //1ms timeout
	if(status == HAL_OK) status = HAL_I2C_Master_Receive(&hi2c2, addr, read_data, num_regs, num_regs); //1ms per byte, should be plenty even at 100khz
	BUS_IN_USE = 0; //unlock the bus
	return status;
}

HAL_StatusTypeDef i2c_write_regs(uint8_t addr, uint8_t* data_to_write, uint8_t len) {
	HAL_StatusTypeDef status;
	if(BUS_IN_USE) return HAL_BUSY;
	BUS_IN_USE = 1; //lock the bus
	status = HAL_I2C_Master_Transmit(&hi2c2, addr, data_to_write, len, len); //1ms per byte, should be plenty even at 100khz
	BUS_IN_USE = 0; //unlock the bus
	return status;
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
