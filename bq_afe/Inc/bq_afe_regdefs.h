#ifndef BQ_AFE_REGDEFS_H
#define BQ_AFE_REGDEFS_H

// =========== System Status Register ============
typedef union {
	struct {
		uint8_t OCD : 1;
		uint8_t SCD : 1;
		uint8_t OV : 1;
		uint8_t Override_alt : 1;
		uint8_t X_ready : 1;
		uint8_t reserved : 1;
		uint8_t CC_ready : 1;
	} map;
	uint8_t data;
} SysStatTypedef;
#define SYS_STAT_REG 0x00

// ============= Cell Balance Registers =============
#define CELL_BAL_REG_BASE 0x01
#define CELL_BAL_1_REG 0x01 //cells 5-1
#define CELL_BAL_2_REG 0x02 //cells 10-6

//============ System Control Registers =============
#define SYS_CTRL_BASE_REG 0x04

typedef union {
	struct {
		uint8_t Shut_B : 1;
		uint8_t Shut_A : 1;
		uint8_t reserved1 : 1;
		uint8_t Temp_sel : 1;
		uint8_t ADC_en : 1;
		uint8_t reserved2 : 2;
		uint8_t Load_pres : 1;
	} map;
	uint8_t data;
} SysCtrl1Typedef;
#define SYS_CTRL_1_REG 0x04

typedef union {
	struct {
		uint8_t Chg_on : 1;
		uint8_t Dsg_on : 1;
		uint8_t reserved : 3;
		uint8_t CC_oneshot : 1;
		uint8_t CC_en : 1;
		uint8_t Delay_dis : 1;
	} map;
	uint8_t data;
} SysCtrl2Typedef;
#define SYS_CTRL_2_REG 0x05

// ============ Protect Registers ===========
#define PROTECT_REG_BASE 0x06

typedef union {
	struct {
		uint8_t SCD_thresh : 3;
		uint8_t SCD_delay : 2;
		uint8_t reserved : 2;
		uint8_t Rsns : 1;
	} map;
	uint8_t data;
} Protect1Typedef;
#define PROTECT1_REG 0x06

typedef union {
	struct {
		uint8_t OCD_thresh : 4;
		uint8_t OCD_delay : 3;
		uint8_t reserved : 1;
	} map;
	uint8_t data;
} Protect2Typedef;
#define PROTECT2_REG 0x07

typedef union {
	struct {
		uint8_t reserved : 4;
		uint8_t OV_delay : 2;
		uint8_t UV_delay : 2;
	} map;
	uint8_t data;
} Protect3Typedef;
#define PROTECT3_REG 0x08

// ================ OV/UV Threshold Registers ============
#define OV_TRIP_REG 0x09
#define UV_TRIP_REG 0x0A

// ============== CC Config Register ==============
#define CC_CFG_REG 0x0B

// ============== ADC Relevant Registers ==============
#define CELL_VOLTAGE_REG_BASE 0x0C 	//each reading is 2-wide, read HI (lower address) and LOW (higher address), 14-bits
									//VC10 is 0x1F
#define BAT_REG_BASE 0x2A //2 regs, low byte -> low address; high byte -> high address, 16-bits
#define TS1_REG_BASE 0x2C // same thing as above, but 14 bits
#define TS2_REG_BASE 0x2E // same thing as above
#define CC_REG_BASE 0x32 // same thing as above, but 16-bits

// ============== ADC Calibration Registers ===========
#define ADC_CAL_REG_BASE 0x50 //read from here to 0x59 (10 regs)
#define ADC_GAIN_1_REG 0x50 //gain bits 4-3 are in the 3rd and 4th bits
#define ADC_GAIN_2_REG 0x59 //gain bits 2-0 are in the 6th, 7th, and 8th bits
#define ADC_OFFSET_REG 0x51 //two's complement offset value in mV

#endif
