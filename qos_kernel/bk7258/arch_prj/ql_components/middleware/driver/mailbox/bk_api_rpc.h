#pragma once
#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include <common/bk_err.h>
#include <stdint.h>

#define BK_API_RPC_EVENT_GET_CHIP_UID            (0x0001)

#define BK_UID_SIZE            (32)
typedef struct
{
    uint16_t event;
    uint16_t out_para_size;    // must be BK_UID_SIZE
    unsigned char out_para[BK_UID_SIZE]; // inline buffer for UID data
} bk_api_rpc_get_uid_msg_t;


bk_err_t bk_api_rpc_get_uid(unsigned char data[32]);

bk_err_t bk_api_rpc_init(void);

#ifdef __cplusplus
}
#endif//__cplusplus


