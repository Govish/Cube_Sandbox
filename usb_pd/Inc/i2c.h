/**
  ******************************************************************************
  * File Name          : I2C.h
  * Description        : This file provides code for the configuration
  *                      of the I2C instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __i2c_H
#define __i2c_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_I2C2_Init(void);

/* USER CODE BEGIN Prototypes */

//some wrappers that make reading and writing on I2C a tad bit easier
HAL_StatusTypeDef i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t* buf);
HAL_StatusTypeDef i2c_write_command(uint8_t addr, uint8_t command);
HAL_StatusTypeDef i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t data);
HAL_StatusTypeDef i2c_print_reg(uint8_t addr, uint8_t reg);

HAL_StatusTypeDef i2c_read_regs(uint8_t addr, uint8_t reg_start, uint8_t num_regs, uint8_t* read_data);
HAL_StatusTypeDef i2c_write_regs(uint8_t addr, uint8_t* data, uint8_t len);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ i2c_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
