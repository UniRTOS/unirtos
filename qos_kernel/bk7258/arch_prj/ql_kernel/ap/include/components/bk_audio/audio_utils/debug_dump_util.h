#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "sdkconfig.h"
#include "uart_util.h"
#include "compiler.h"

#define HEADER_MAGICWORD_PART1    (0xDEADBEEF)
#define HEADER_MAGICWORD_PART2    (0x0F1001F0)
#define DEBUG_DUMP_MAX_DATA_FLOW_NO        (6)

typedef enum {
    DUMP_TYPE_DEC_OUT_DATA = 0,
    DUMP_TYPE_DEC_IN_DATA,
    DUMP_TYPE_ENC_OUT_DATA,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_AEC_REF_DATA,
    DUMP_TYPE_AEC_OUT_DATA,
    DUMP_TYPE_ENC_IN_DATA,
    DUMP_TYPE_EQ_IN_DATA,
    DUMP_TYPE_EQ_OUT_DATA,
    DUMP_TYPE_MAX
} debug_dump_type_t;

#define HEADER_ARRAY_CNT (DUMP_TYPE_MAX-2)//AEC MIC/REF/OUT DATA use same HEADER

typedef enum {
    DUMP_FILE_TYPE_PCM = 0,
    DUMP_FILE_TYPE_G722,
    DUMP_FILE_TYPE_OPUS,
    DUMP_FILE_TYPE_G711A,
    DUMP_FILE_TYPE_G711U,
    DUMP_FILE_TYPE_MP3,
    DUMP_FILE_TYPE_AAC,
    DUMP_FILE_TYPE_SBC,
    DUMP_FILE_TYPE_WAV,
    DUMP_FILE_TYPE_MAX,
} debug_dump_file_type_t;

typedef struct
{
    uint8_t  dump_type;
    uint8_t  dump_file_type;
    uint16_t len;
#if CONFIG_ADK_DEBUG_DUMP_DATA_TYPE_EXTENSION
    uint16_t sample_rate;
    uint8_t  frame_in_ms;
    uint8_t  ch_num;
#endif
}data_flow_t;

typedef struct
{
    uint32_t header_magicword_part1;
    uint32_t header_magicword_part2;
    uint32_t  data_flow_num;
    data_flow_t data_flow[DEBUG_DUMP_MAX_DATA_FLOW_NO];
    uint32_t seq_no;
    uint32_t timestamp;
} debug_dump_data_header_t;

#if CONFIG_ADK_DEBUG_DUMP_UTIL
extern struct uart_util g_debug_data_uart_util;
extern volatile debug_dump_data_header_t dump_header[HEADER_ARRAY_CNT];
extern const uint8_t g_dump_type2header_array_idx[DUMP_TYPE_MAX];
extern uint16_t g_aud_data_dump_bitmap;

#define DEBUG_DATA_DUMP_UART_ID            (1)
#define DEBUG_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define DEBUG_DATA_DUMP_BY_UART_OPEN()                        uart_util_create(&g_debug_data_uart_util, DEBUG_DATA_DUMP_UART_ID, DEBUG_DATA_DUMP_UART_BAUD_RATE)
#define DEBUG_DATA_DUMP_BY_UART_CLOSE()                       uart_util_destroy(&g_debug_data_uart_util)
#define DEBUG_DATA_DUMP_BY_UART_DATA(data_buf, len)           uart_util_tx_data(&g_debug_data_uart_util, data_buf, len)


#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_NUM(dump_type,data_flow_num)              dump_header[g_dump_type2header_array_idx[dump_type]].data_flow_num = data_flow_num
#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW(type,data_flow_idx,file_type,length)      do\
                                                                                          {\
                                                                                              dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].dump_type = type;\
                                                                                              dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].dump_file_type = file_type;\
                                                                                              dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].len = length;\
                                                                                          }while(0)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_DUMP_FILE_TYPE(dump_type,data_flow_idx,file_type) dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].dump_file_type = file_type
#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(dump_type,data_flow_idx,length)     dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].len = length
#define DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(dump_type)            dump_header[g_dump_type2header_array_idx[dump_type]].timestamp = rtos_get_time()
#define DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(dump_type)              dump_header[g_dump_type2header_array_idx[dump_type]].seq_no++
#if CONFIG_ADK_DEBUG_DUMP_DATA_TYPE_EXTENSION
#define DEBUG_DATA_DUMP_UPDATE_HEADER_SAMPLE_RATE(dump_type,data_flow_idx,sr)          dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].sample_rate = sr
#define DEBUG_DATA_DUMP_UPDATE_HEADER_FRAME_IN_MS(dump_type,data_flow_idx,frame_ms)    dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].frame_in_ms = frame_ms
#define DEBUG_DATA_DUMP_UPDATE_HEADER_CHANNEL_NUM(dump_type,data_flow_idx,ch_n)        dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].ch_num = ch_n
#endif
#define DEBUG_DATA_DUMP_BY_UART_HEADER(dump_type)                     DEBUG_DATA_DUMP_BY_UART_DATA((void *)&dump_header[g_dump_type2header_array_idx[dump_type]], sizeof(debug_dump_data_header_t))

#define DEBUG_DATA_DUMP_SUSPEND_ALL uint32_t irq_level = rtos_enter_critical();
#define DEBUG_DATA_DUMP_RESUME_ALL  rtos_exit_critical(irq_level);

__INLINE void set_aud_dump_bitmap_bit(uint8_t dump_type)
{
    if(DUMP_TYPE_MAX <= dump_type)
    {
        BK_LOGE("aud_dump","%s,%d,dump_type:%d is invalid!\n",__func__, __LINE__,dump_type);
        return ;
    }

    g_aud_data_dump_bitmap |= (1<<dump_type);
}

__INLINE void clr_aud_dump_bitmap_bit(uint8_t dump_type)
{
    if(DUMP_TYPE_MAX <= dump_type)
    {
        BK_LOGE("aud_dump","%s,%d,dump_type:%d is invalid!\n", __func__, __LINE__,dump_type);
        return;
    }

    g_aud_data_dump_bitmap &= (~(1<<dump_type));
}

__INLINE bool is_aud_dump_valid(uint8_t dump_type)
{
    if(DUMP_TYPE_MAX <= dump_type)
    {
        BK_LOGE("aud_dump","%s,%d,dump_type:%d is invalid!\n", __func__, __LINE__,dump_type);
        return false;
    }
    return (bool)(((g_aud_data_dump_bitmap & (1<<dump_type))>>dump_type)&0x1);
}

__INLINE uint16_t get_aud_dump_bitmap(void)
{
    return g_aud_data_dump_bitmap;
}

__INLINE void clr_aud_dump_bitmap(void)
{
    g_aud_data_dump_bitmap = 0;
}

int aud_dump_cli_init(void);

#else

#define DEBUG_DATA_DUMP_BY_UART_OPEN()
#define DEBUG_DATA_DUMP_BY_UART_CLOSE()
#define DEBUG_DATA_DUMP_BY_UART_DATA(data_buf, len)

#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_NUM(dump_type,data_flow_num)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW(dump_type,data_flow_idx,dump_file_type,len)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_DUMP_FILE_TYPE(dump_type,data_flow_idx,file_type)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(dump_type,data_flow_idx,len)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(dump_type)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(dump_type)
#define DEBUG_DATA_DUMP_BY_UART_HEADER(dump_type)

#define DEBUG_DATA_DUMP_SUSPEND_ALL
#define DEBUG_DATA_DUMP_RESUME_ALL

__INLINE void set_aud_dump_bitmap_bit(uint8_t dump_type)
{
    return;
}

__INLINE void clr_aud_dump_bitmap_bit(uint8_t dump_type)
{
    return;
}

__INLINE bool is_aud_dump_valid(uint8_t dump_type)
{
    return false;
}

__INLINE uint16_t get_aud_dump_bitmap(void)
{
    return 0;
}

__INLINE void clr_aud_dump_bitmap(void)
{
    return;
}

#endif

#ifdef __cplusplus
}
#endif

