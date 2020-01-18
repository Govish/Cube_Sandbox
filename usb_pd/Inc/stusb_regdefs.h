/*
 * stusb_regdefs.h
 * Register addresses and mappings for STMicroelectronics STUSB4500 USB-PD standalone negotiator
 * Source code mostly ported from ST sample code, with some custom mods
 * I don't entirely know what I'm doing, so don't use this for critical applications without extensive testing
 */

#ifndef STUSB_REGDEFS_H
#define STUSB_REGDEFS_H

#define STUSB_ADDR 0x28<<1 //8-bit
#define DEVICE_ID_REG 0x2F

#define CC_STATUS_REG 0x11
#define ALERT_STATUS_1_MASK_REG 0x0C
#define ALERT_STATUS_1_REG 0x0B
#define PORT_STATUS_0_REG 0x0D
#define PORT_STATUS_1_REG 0x0E
#define PS1_CONNECTION_BITS 0xE0 //top 3 bits in the port status 1 register tell us the type of device the port is connected to
#define ATTACHED_SOURCE 0x40 //value of the top 3 bits of the PORT_STATUS_1 register if we're attached to a source

#define TYPEC_MON_STATUS_0_REG 0x0F
#define TYPEC_MON_STATUS_1_REG 0x10
/*
 * TODO: add more TCMON reg stuff in here
 */

#define CC_FAULT_STATUS_0_REG 0x12
#define CC_FAULT_STATUS_1_REG 0x13
/*
 * TODO add more CC_HW_FAULT reg stuff in here if necessary
 */

#define PROTOCOL_STATUS_REG 0x16
#define PRTL_MESSAGE_RECEIVED (1<<2)

#define RESET_CTRL_REG 0x23
#define TX_HEADER_REG 0x51
#define STUSB_GEN1S_CMD_CTRL_REG 0x1A

#define RX_HEADER_REG 0x31 //2 registers starting here

#define RX_DATA_OBJ_REG 0x33 //a bunch of registers starting here (28 regs in total)

#define DPM_PDO_NUMB_REG 0x70
#define DPM_SNK_PDO1 0x85
#define DPM_SNK_PDO2 0x89
#define DPM_SNK_PDO3 0x8D
#define DPM_REQ_RDO1 0x91
/*
=========================== CC_STATUS_REGISTER info =========================
    BIT 5       LOOKING_FOR_CONNECTION
                    1: The device is looking for a connection (either trying to act as a sink/source or DRP toggling)
                    0: The device has FOUND a connection (or simply not looking for a connection)
    BIT 4       CONNECTION RESULT
                    1: The STUSB4500 is acting as a power SINK
                    0: The STUSB4500 is acting as a power SOURCE
    BITS 3:2    CC2 STATE
                    11: SINK MODE - Source is presenting Rp indicating it can source 3.0A
                        SOURCE MODE - Reserved
                    10: SINK MODE - Source is advertising 1.5A
                        SOURCE MODE - This particular CC pin is connected to the SINK
                    01: SINK MODE - Source is advertising 900mA current capability
                        SOURCE MODE - This particular CC pin is connected to Ra (not used in CC communication)
                    00: SINK MODE - Being pulled down by Ra
                        SOURCE MODE - This particular CC pin is OPEN (not connected)
    BITS 1:0    CC1 STATE
                    Same encodings as above
*/

typedef union {
    uint8_t data;
    struct {
        //struct goes from LSB -> MSB
        uint8_t cc1 : 2; //cc1 state information
        uint8_t cc2 : 2; //cc2 state information
        uint8_t conn_res: 1; //connection result
        uint8_t looking : 1; //if it's looking for a connection
        uint8_t reserved : 2; //upper 2 bits
    } map;
} CC_Reg_Map;


typedef union {
    uint8_t data;
    struct {
        uint8_t phy_mask : 1;
        uint8_t prt_mask : 1;
        uint8_t reserved : 1;
        uint8_t tcstat_mask : 1;
        uint8_t ccfault_mask : 1;
        uint8_t tcmon_mask : 1;
        uint8_t ccdetect_mask : 1;
        uint8_t hreset_mask : 1;
    } map;
} Alt_S1Reg_Map;

#endif
