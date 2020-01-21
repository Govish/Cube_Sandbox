#ifndef BQ_AFE_H
#define BQ_AFE_H

#include "stm32f0xx_hal.h"
#include "bq_afe_regdefs.h"
#include "stdbool.h"

#define AFE_ADDR (0x18 << 1) //8-bit i2c address
#define NUM_CELLS 10 //10s lithium ion stack

// ================ Some enumerated types ==============
typedef enum {
	UV_DELAY_1s = 0, UV_DELAY_4s = 1, UV_DELAY_8s = 2, UV_DELAY_16s = 3
} UV_Delay_Settings;

typedef enum {
	OV_DELAY_1s = 0, OV_DELAY_2s = 1, OV_DELAY_4s = 2, OV_DELAY_8s = 3
} OV_Delay_Settings;

typedef enum {
	TEMP_DIE = 0, TEMP_THERM = 1
} Therm_sel;

#define OV_TRIP_THRESHOLD 4220 //in millivolts
#define UV_TRIP_THRESHOLD 3000 //in millivolts

// ================ Public Functions ================
HAL_StatusTypeDef afe_init();
HAL_StatusTypeDef afe_set_vThresholds(float ov, float uv);
HAL_StatusTypeDef afe_set_charge(bool enabled);
HAL_StatusTypeDef afe_set_discharge(bool enabled);
HAL_StatusTypeDef afe_measure_temps(float* ts1, float* ts2);
HAL_StatusTypeDef afe_measure_voltages(float* vBuffer, float* batBuffer);
HAL_StatusTypeDef afe_sleep();
HAL_StatusTypeDef afe_shutdown();

//read system status register?

//================= Functions in an Interrupt Context ==================
HAL_StatusTypeDef afe_balance();
HAL_StatusTypeDef afe_onAlert();

#endif
