#include <os/os.h>
#include "cli.h"
#include <common/bk_include.h>
#include <components/bk_audio_asr_service.h>
#include <components/bk_asr_service_types.h>
#include <components/bk_asr_service.h>

#define TAG "aud_asr"

#define AUD_ASR_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "%s, %d, AUD_ASR_CHECK_NULL fail \n", __func__, __LINE__);\
            {act;};\
        }\
    } while(0)

#define AUD_ASR_RAW_READ_SIZE    (480)

typedef enum
{
    AUD_ASR_IDLE = 0,
    AUD_ASR_START,
    AUD_ASR_EXIT
} aud_asr_op_t;

typedef struct
{
    aud_asr_op_t op;
    void *param;
} aud_asr_msg_t;

struct aud_asr
{
	asr_handle_t asr_handle;                                                        /**< asr handle */
	void *args;                                                                     /**< the pravate parameter of callback */
	int task_stack;                                                                 /**< Task stack size */
	int task_core;                                                                  /**< Task running in core (0 or 1) */
	int task_prio;                                                                  /**< Task priority (based on freeRTOS priority) */
	audio_mem_type_t mem_type;                                                      /**< memory type used, sram, psram, audio_heap */
	beken_thread_t aud_asr_task_hdl;
	beken_queue_t aud_asr_msg_que;
	beken_semaphore_t sem;
	uint8_t *read_buff;
	bool running;
	uint32_t max_read_size;                                                         /**< the max size of data read from asr handle, used in asr_read_callback */
	void (*aud_asr_result_handle)(uint32_t param);
	int (*aud_asr_init)(void);
	int (*aud_asr_recog)(void *read_buf, uint32_t read_size, void *p1, void *p2);
	void (*aud_asr_deinit)(void);
};

extern int cli_asr_dump_init(void);
extern int cli_asr_dump_deinit(void);

#define ASR_INPUT_DEBUG (0)
#if ASR_INPUT_DEBUG
#define ASR_INPUT_START()    do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define ASR_INPUT_END()      do { GPIO_DOWN(34); } while (0)
#else
#define ASR_INPUT_START()
#define ASR_INPUT_END()
#endif


#include <components/bk_audio/audio_utils/uart_util.h>

#define ASR_DATA_DUMP_HEADER_MAGICWORD_PART1    (0xDEADBEEF)
#define ASR_DATA_DUMP_HEADER_MAGICWORD_PART2    (0x0F1001F0)

typedef struct
{
    uint32_t header_magicword_part1;
    uint32_t header_magicword_part2;
    uint32_t seq_no;
} asr_data_dump_header_t;

static asr_data_dump_header_t g_asr_data_dump_header = {
    .header_magicword_part1 = ASR_DATA_DUMP_HEADER_MAGICWORD_PART1,
    .header_magicword_part2 = ASR_DATA_DUMP_HEADER_MAGICWORD_PART2,
    .seq_no = 0,
};
static struct uart_util __maybe_unused gl_asr_util = {0};
#define ASR_DATA_DUMP_UART_ID            (1)
#define ASR_DATA_DUMP_UART_BAUD_RATE     (2000000)

#if CONFIG_ADK_UART_UTIL
#define ASR_DATA_DUMP_BY_UART_OPEN(id, baud_rate)       uart_util_create(&gl_asr_util, id, baud_rate)
#define ASR_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_asr_util)
#define ASR_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_asr_util, data_buf, len)
#else
#define ASR_DATA_DUMP_BY_UART_OPEN(id, baud_rate)
#define ASR_DATA_DUMP_BY_UART_CLOSE()
#define ASR_DATA_DUMP_BY_UART_DATA(data_buf, len)
#endif

static volatile uint8_t g_asr_dump_enable = 0;
static volatile uint8_t g_asr_time_debug_enable = 0;
static volatile uint32_t g_asr_time_threshold = 30;  // Default time threshold in milliseconds

// Runtime time statistics control
// Note: start_time and stop_time should be declared in the same scope where these macros are used
#define ASR_TIME_START()    uint64_t start_time = 0; \
                            if (g_asr_time_debug_enable) { start_time = rtos_get_time(); }

#define ASR_TIME_END()      uint64_t stop_time = 0; \
                            if (g_asr_time_debug_enable) { stop_time = rtos_get_time(); }

static inline void asr_time_check_internal(uint64_t start_time, uint64_t stop_time, int result)
{
    if (g_asr_time_debug_enable)
    {
        uint32_t time_diff = (uint32_t)(stop_time - start_time);
        if (time_diff >= g_asr_time_threshold)
        {
            BK_LOGI(TAG, "Recogn:%d---%d\n", time_diff, result);
        }
        else if ((int32_t)(stop_time - start_time) < 0)
        {
            BK_LOGI(TAG, "Error execute--%d\n", time_diff);
        }
    }
}

#define ASR_TIME_CHECK()    do { \
                                if (g_asr_time_debug_enable) { \
                                    asr_time_check_internal(start_time, stop_time, result); \
                                } \
                            } while(0)

const static char *text;
static float score;

static bk_err_t aud_asr_send_msg(beken_queue_t queue, aud_asr_op_t op, void *param)
{
	bk_err_t ret = BK_FAIL;
	aud_asr_msg_t msg;

	if (!queue)
	{
		BK_LOGE(TAG, "%s, %d, queue: %p \n", __func__, __LINE__, queue);
		return ret;
	}

	msg.op = op;
	msg.param = param;
	ret = rtos_push_to_queue(&queue, &msg, BEKEN_NO_WAIT);
	if (kNoErr != ret)
	{
		BK_LOGE(TAG, "%s, %d, send message: %d fail, ret: %d\n", __func__, __LINE__, op, ret);
		return ret;
	}

	return ret;
}

static void aud_asr_result_handle(uint32_t param)
{
	BK_LOGI(TAG, "asr_result : %s\r\n", (char *)param);
}

static void aud_asr_task_main(beken_thread_arg_t param_data)
{
	int result = 0;
	int read_size = 0;
	bk_err_t ret = BK_OK;

	aud_asr_handle_t aud_asr_handle = (aud_asr_handle_t)param_data;

	aud_asr_handle->running = false;
	long unsigned int wait_time = BEKEN_WAIT_FOREVER;

	if (aud_asr_handle->aud_asr_init) {
		if (aud_asr_handle->aud_asr_init() < 0)
		{
			os_printf("Wanson_ASR_Init Failed!\n");
			goto aud_asr_exit;
		}
	}
	rtos_set_semaphore(&aud_asr_handle->sem);

	while (1)
	{
		aud_asr_msg_t msg;
		ret = rtos_pop_from_queue(&aud_asr_handle->aud_asr_msg_que, &msg, wait_time);
		if (kNoErr == ret)
		{
			switch (msg.op)
			{
				case AUD_ASR_IDLE:
					aud_asr_handle->running = false;
					wait_time = BEKEN_WAIT_FOREVER;
					break;
				case AUD_ASR_EXIT:
					goto aud_asr_exit;
					break;
				case AUD_ASR_START:
					aud_asr_handle->running = true;
					wait_time = 0;
					break;
				default:
					break;
			}
		}

	/* read mic data and send */
	if (aud_asr_handle->running)
	{
			/* read mic data and send */
			extern int bk_aud_asr_get_size(asr_handle_t asr_handle);
			extern int bk_aud_asr_get_filled_size(asr_handle_t asr_handle);

		//	int __maybe_unused ss = bk_aud_asr_get_filled_size(aud_asr_handle->asr_handle);
		//	int __maybe_unused sss = bk_aud_asr_get_size(aud_asr_handle->asr_handle);

			{
				read_size = bk_aud_asr_read_mic_data(aud_asr_handle->asr_handle, (char *)aud_asr_handle->read_buff, aud_asr_handle->max_read_size);
				if (read_size == aud_asr_handle->max_read_size)
				{
					ASR_TIME_START();
					ASR_INPUT_START();
					if (aud_asr_handle->aud_asr_recog) {
						result = aud_asr_handle->aud_asr_recog((void*)aud_asr_handle->read_buff, aud_asr_handle->max_read_size, (void*)&text, (void*)&score);
					}
					ASR_INPUT_END();
					ASR_TIME_END();
					ASR_TIME_CHECK();

                    if (g_asr_dump_enable)
                    {
                        ASR_DATA_DUMP_BY_UART_DATA((uint8_t *)&g_asr_data_dump_header, sizeof(asr_data_dump_header_t));
                        ASR_DATA_DUMP_BY_UART_DATA(aud_asr_handle->read_buff, read_size);
                        g_asr_data_dump_header.seq_no++;
                    }

					if (result == 1) {
						if (aud_asr_handle->aud_asr_result_handle) {
							aud_asr_handle->aud_asr_result_handle((uint32_t)text);
						} else
						{
							BK_LOGE(TAG, "aud_asr_handle->aud_asr_result_handle is NULL\n");
							aud_asr_result_handle((uint32_t)text);
						}
					}
				}
				else {
					continue;
				}
			}
		}
	}

aud_asr_exit:

	aud_asr_handle->running = false;
	if (aud_asr_handle->aud_asr_deinit) {
		aud_asr_handle->aud_asr_deinit();
	}

	/* delete msg queue */
	ret = rtos_deinit_queue(&aud_asr_handle->aud_asr_msg_que);
	if (ret != kNoErr)
	{
		BK_LOGE(TAG, "%s, %d, delete message queue fail\n", __func__, __LINE__);
	}
	aud_asr_handle->aud_asr_msg_que = NULL;

	/* delete task */
	aud_asr_handle->aud_asr_task_hdl = NULL;
	rtos_set_semaphore(&aud_asr_handle->sem);
	rtos_delete_thread(NULL);
}

aud_asr_handle_t bk_aud_asr_init(aud_asr_cfg_t *cfg)
{
    bk_err_t ret = BK_OK;
    aud_asr_handle_t aud_asr_handle = NULL;

    AUD_ASR_CHECK_NULL(cfg, return NULL);

    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        aud_asr_handle = psram_malloc(sizeof(struct aud_asr));
    }
    else
    {
        aud_asr_handle = os_malloc(sizeof(struct aud_asr));
    }
    AUD_ASR_CHECK_NULL(aud_asr_handle, return NULL);

    os_memset(aud_asr_handle, 0x00, sizeof(struct aud_asr));

    /* copy config */
    aud_asr_handle->asr_handle    = cfg->asr_handle;
    aud_asr_handle->args          = cfg->args;
    aud_asr_handle->task_stack    = cfg->task_stack;
    aud_asr_handle->task_core     = cfg->task_core;
    aud_asr_handle->task_prio     = cfg->task_prio;
    aud_asr_handle->mem_type      = cfg->mem_type;

    aud_asr_handle->max_read_size         = cfg->max_read_size;
    aud_asr_handle->aud_asr_result_handle = cfg->aud_asr_result_handle;
    aud_asr_handle->aud_asr_init          = cfg->aud_asr_init;
    aud_asr_handle->aud_asr_recog         = cfg->aud_asr_recog;
    aud_asr_handle->aud_asr_deinit        = cfg->aud_asr_deinit;

    /* malloc read buffer */
    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        aud_asr_handle->read_buff = psram_malloc(aud_asr_handle->max_read_size);
    }
    else
    {
        aud_asr_handle->read_buff = os_malloc(aud_asr_handle->max_read_size);
    }
    AUD_ASR_CHECK_NULL(aud_asr_handle->read_buff, goto fail);

    os_memset(aud_asr_handle->read_buff, 0x00, aud_asr_handle->max_read_size);

    ret = rtos_init_semaphore(&aud_asr_handle->sem, 1);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create semaphore fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_init_queue(&aud_asr_handle->aud_asr_msg_que,
                          "aud_asr_que",
                          sizeof(aud_asr_msg_t),
                          32);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create aud asr message queue fail\n", __func__, __LINE__);
        goto fail;
    }

    #if CONFIG_ASR_SERVICE_THREAD_BIND_CPU
    #if CONFIG_ASR_SERVICE_THREAD_BIND_CPU_ID == 0
    ret = rtos_core0_create_thread(&aud_asr_handle->aud_asr_task_hdl,
                             aud_asr_handle->task_prio,
                             "aud_asr",
                             (beken_thread_function_t)aud_asr_task_main,
                             aud_asr_handle->task_stack,
                             (beken_thread_arg_t)aud_asr_handle);
    #elif CONFIG_ASR_SERVICE_THREAD_BIND_CPU_ID == 1
    ret = rtos_core1_create_thread(&aud_asr_handle->aud_asr_task_hdl,
                         aud_asr_handle->task_prio,
                         "aud_asr",
                         (beken_thread_function_t)aud_asr_task_main,
                         aud_asr_handle->task_stack,
                         (beken_thread_arg_t)aud_asr_handle);
    #endif
    #else
    ret = rtos_create_thread(&aud_asr_handle->aud_asr_task_hdl,
                         aud_asr_handle->task_prio,
                         "aud_asr",
                         (beken_thread_function_t)aud_asr_task_main,
                         aud_asr_handle->task_stack,
                         (beken_thread_arg_t)aud_asr_handle);
    #endif
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create aud asr task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&aud_asr_handle->sem, BEKEN_NEVER_TIMEOUT);

    cli_asr_dump_init();

    BK_LOGD(TAG, "init aud asr task complete\n");
    return aud_asr_handle;

fail:
    if (aud_asr_handle->sem)
    {
        rtos_deinit_semaphore(&aud_asr_handle->sem);
        aud_asr_handle->sem = NULL;
    }

    if (aud_asr_handle->aud_asr_msg_que)
    {
        rtos_deinit_queue(&aud_asr_handle->aud_asr_msg_que);
        aud_asr_handle->aud_asr_msg_que = NULL;
    }

    if (aud_asr_handle->read_buff)
    {
        if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
        {
            psram_free(aud_asr_handle->read_buff);
        }
        else
        {
            os_free(aud_asr_handle->read_buff);
        }
    }

    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(aud_asr_handle);
    }
    else
    {
        os_free(aud_asr_handle);
    }

    return NULL;
}

bk_err_t bk_aud_asr_deinit(aud_asr_handle_t aud_asr_handle)
{
    bk_err_t ret = BK_FAIL;

    AUD_ASR_CHECK_NULL(aud_asr_handle, return ret);

    BK_LOGD(TAG, "%s\n", __func__);

    ret = aud_asr_send_msg(aud_asr_handle->aud_asr_msg_que, AUD_ASR_EXIT, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: AUD_ASR_EXIT fail\n", __func__, __LINE__);
        return ret;
    }

    rtos_get_semaphore(&aud_asr_handle->sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&aud_asr_handle->sem);
    aud_asr_handle->sem = NULL;

    if (aud_asr_handle->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(aud_asr_handle->read_buff);
    }
    else
    {
        os_free(aud_asr_handle->read_buff);
    }

    if (aud_asr_handle->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(aud_asr_handle);
    }
    else
    {
        os_free(aud_asr_handle);
    }

    cli_asr_dump_deinit();
    BK_LOGD(TAG, "deinit aud asr complete\n");
    return ret;
}

bk_err_t bk_aud_asr_start(aud_asr_handle_t aud_asr_handle)
{
    bk_err_t ret = BK_FAIL;

#if (CONFIG_WANSON_NEW_LIB_SEG)
    extern void asr_func_null(void);
    asr_func_null();
#endif

    AUD_ASR_CHECK_NULL(aud_asr_handle, return ret);
    BK_LOGD(TAG, "%s, Line:%d\n", __func__, __LINE__);

    ret = aud_asr_send_msg(aud_asr_handle->aud_asr_msg_que, AUD_ASR_START, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: AUD_ASR_START fail\n", __func__, __LINE__);
        return ret;
    }
    return ret;
}

bk_err_t bk_aud_asr_stop(aud_asr_handle_t aud_asr_handle)
{
    bk_err_t ret = BK_FAIL;
    AUD_ASR_CHECK_NULL(aud_asr_handle, return ret);
    BK_LOGD(TAG, "%s, Line:%d\n", __func__, __LINE__);

    ret = aud_asr_send_msg(aud_asr_handle->aud_asr_msg_que, AUD_ASR_IDLE, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: AUD_ASR_IDLE fail\n", __func__, __LINE__);
        return ret;
    }
    return ret;
}

void cli_asr_dump_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2)
    {
        BK_LOGE(TAG, "Usage: asr_dump {start [uart_id] [baud_rate]|stop|time_debug [0|1]|time_debug threshold <value>}\n");
        return;
    }

    if (os_strcmp(argv[1], "start") == 0)
    {
        bk_err_t ret = BK_OK;
        if (argc == 4)
        {
            uint32_t __maybe_unused uart_id = 0;
            uint32_t __maybe_unused baud_rate = 0;
            uart_id = os_strtoul(argv[2], NULL, 10);
            baud_rate = os_strtoul(argv[3], NULL, 10);
            ASR_DATA_DUMP_BY_UART_OPEN(uart_id, baud_rate);
        }
        else if (argc == 2)
        {
            ASR_DATA_DUMP_BY_UART_OPEN(ASR_DATA_DUMP_UART_ID, ASR_DATA_DUMP_UART_BAUD_RATE);
        }
        else
        {
            BK_LOGE(TAG, "Usage: asr_dump start [uart_id] [baud_rate]\n");
            return;
        }

        if (ret == BK_OK)
        {
            g_asr_dump_enable = 1;
            BK_LOGI(TAG, "asr service start\n");
        }
        else
        {
            BK_LOGE(TAG, "Failed to open UART for ASR data dump\n");
        }
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        g_asr_dump_enable = 0;  // Disable first to avoid writing to UART while closing
        ASR_DATA_DUMP_BY_UART_CLOSE();
        g_asr_data_dump_header.seq_no = 0;
        BK_LOGI(TAG, "asr service stop complete\n");
    }
    else if (os_strcmp(argv[1], "time_debug") == 0)
    {
        if (argc == 3)
        {
            uint32_t enable = os_strtoul(argv[2], NULL, 10);
            g_asr_time_debug_enable = (enable != 0) ? 1 : 0;
            BK_LOGI(TAG, "asr time debug %s\n", g_asr_time_debug_enable ? "enabled" : "disabled");
        }
        else if (argc == 4 && os_strcmp(argv[2], "threshold") == 0)
        {
            uint32_t threshold = os_strtoul(argv[3], NULL, 10);
            if (threshold > 0)
            {
                g_asr_time_threshold = threshold;
                BK_LOGI(TAG, "asr time threshold set to %d ms\n", g_asr_time_threshold);
            }
            else
            {
                BK_LOGE(TAG, "Invalid threshold value, must be > 0\n");
            }
        }
        else
        {
            BK_LOGE(TAG, "Usage: asr_dump time_debug [0|1] or asr_dump time_debug threshold <value>\n");
        }
    }
    else
    {
        BK_LOGE(TAG, "Usage: asr_dump {start [uart_id] [baud_rate]|stop|time_debug [0|1]|time_debug threshold <value>}\n");
    }
}

static const struct cli_command s_asr_dump_commands[] =
{
    {"asr_dump", "asr_dump {start [uart_id] [baud_rate]|stop|time_debug [0|1]|time_debug threshold <value>}", cli_asr_dump_test_cmd},
};

#define ASR_DUMP_CMD_CNT   (sizeof(s_asr_dump_commands) / sizeof(struct cli_command))

int cli_asr_dump_init(void)
{
    return cli_register_commands(s_asr_dump_commands, ASR_DUMP_CMD_CNT);
}

int cli_asr_dump_deinit(void)
{
    return cli_unregister_commands(s_asr_dump_commands, ASR_DUMP_CMD_CNT);
}