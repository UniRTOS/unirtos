#ifndef _PHY_IPC_H_
#define _PHY_IPC_H_

#include <common/bk_include.h>

enum
{
	PHY_CMD_GET_TEMP,
	PHY_CMD_GET_VOLT,
	PHY_CMD_GET_MAC_ADDR,
	PHY_CMD_GET_STA_MAC_ADDR,
	PHY_CMD_GET_AP_MAC_ADDR,
	PHY_CMD_SET_MAC_ADDR,
};

typedef struct
{
    float                        param;
    u32                          timeout;
    int16                        ret_status;
    u8                           mac[6];
    u32                          crc;
} phy_cmd_t;

#define PHY_IPC_READ_SIZE     0x400
#define PHY_IPC_WRITE_SIZE    0x400

#endif //_SARADC_IPC_H_
// eof