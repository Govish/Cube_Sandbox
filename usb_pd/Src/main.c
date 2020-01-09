/* USER CODE BEGIN Header */

/*
 * The general idea of this program is to test/negotiate different USB PD power levels
 * This is how we'll do it:
 * IN AN INTERRUPT CONTEXT
 * 		- when the attach line goes low, we'll call the pd_onAttach() function
 * 			- this initializes the STUSB4500 and asserts an internal flag *
 * 		- when the alert line goes low, we'll call the pd_onAlert() function
 * 			- follow the instructions taken from the sample code
 * 			- update the flag variables as appropriate
 *
 *	IN THE MAIN LOOP:
 *		- when we're not attached, we'll sit in an idle loop (checked by reading pd_attached())
 *			- simple conditional
 *		- on the rising edge of attached
 *			- call pd_auto_nego()
 *			- THIS IS A BLOCKING CALL (which should be fine depending on how we structure our code)
 *				- may need to set a timeout in here
 * 		- once we're properly attached and operating on a PDO
 * 			- check every now and then that the RDO that we have is valid
 * 			- read the attach line to check if we're still attached (if we somehow missed the rising edge)
 * 			- check the attached
 * 				- if any one of these are false, call the pd_onDetach() function
 * 		- on the falling edge of attached
 * 			- don't think we need to do anything here
 *
 *	THE pd_auto_nego WORKS BY:
 *		- clearing the pdo_received flag
 *		- send a soft_reset command
 *		- waiting (WITH A TIMEOUT) until the pdo_received flag goes high
 *			- might retry the soft reset a couple times if we don't receive PDOs, we'll see
 *		- check all the PDOs, see which one matches the best
 *		- reset the accept_received flag
 *		- select that particular PDO by calling the pd_request_pdo_index() function
 *		- waiting (WITH A TIMEOUT) until the accept_received or reject_received flag goes high
 *			- will retry the negotiation a couple times if we timeout or get a reject message
 *		- update the pointer variables with the voltage, current, and power negotiated
 *
 * 	IN THE EVENT THAT WE GET ANY SORT OF I2C FAILURE
 * 		- uhhhh honestly not sure, need to think about this more
 */

/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stusb_regdefs.h"
#include "stusb4500.h"
#include "printf_override.h"
#include "stdbool.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
bool last_attach;
bool int_enable = true;
UART_HandleTypeDef huart2;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("\r\nStarting...\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  bool current_attach = HAL_GPIO_ReadPin(ATTACH_GPIO_Port, ATTACH_Pin);
	  if(current_attach && !last_attach) {
		  printf("Attach Line High!\n");
	  } else if (!current_attach && last_attach) {
		  printf("Attach Line Low!\n");

		  HAL_Delay(1000);
		  pd_send_soft_reset(); //have source resend PDOs

		  uint8_t num_pdos;
		  PDOTypedef pdos[8];
		  pd_get_source_pdos(&num_pdos, pdos);
		  printf("Number of PDOs Available: %d\n", num_pdos);
		  for(int i = 0; i < num_pdos; i++) {
			  printf("\tPDO number %d: %lx\n", i+1, pdos[i].data);
		  }

		  pd_read_sink_pdos(&num_pdos, pdos);
		  printf("Number of sink PDOs: %d\n", num_pdos);
		  for(int i = 0; i < num_pdos; i++) {
			  printf("\tPDO number %d: %lx\n", i+1, pdos[i].data);
		  }

		  HAL_Delay(5000);
		  //pd_update_num_pdos(2);
		  //pd_send_soft_reset();
		  pd_request_pdo_num(2);
		  pd_read_sink_pdos(&num_pdos, pdos);
		  printf("Number of sink PDOs: %d\n", num_pdos);
		  for(int i = 0; i < num_pdos; i++) {
			  printf("\tPDO number %d: %lx\n", i+1, pdos[i].data);
		  }
		  pd_send_soft_reset();
	  }
	  last_attach = current_attach;
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == ATTACH_Pin) {
    	if(!HAL_GPIO_ReadPin(ATTACH_GPIO_Port, GPIO_Pin)) { //x:0-15
    		pd_onAttach(); //will automatically time out if this is unsuccessful
    	} else {
    		pd_onDetach(); //only runs when a source has been attached
    	}
    }

    if (GPIO_Pin == USB_ALT_Pin) {
    	pd_onAlert();
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
