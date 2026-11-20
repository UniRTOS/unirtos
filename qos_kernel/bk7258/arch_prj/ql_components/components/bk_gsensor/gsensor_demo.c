#include <string.h>
#include "os/os.h"
#include "os/mem.h"
#include <string.h>
#include "i2c_hal.h"
#include <driver/i2c.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/bk_gsensor.h>
#include <components/log.h>
#include <modules/pm.h>
#include <components/bk_gsensor_arithmetic_demo_public.h>

#define GSENSOR_D_TAG "gsensor_demo"
#define GSENSOR_D_LOGI(...) BK_LOGI(GSENSOR_D_TAG, ##__VA_ARGS__)
#define GSENSOR_D_LOGW(...) BK_LOGW(GSENSOR_D_TAG, ##__VA_ARGS__)
#define GSENSOR_D_LOGE(...) BK_LOGE(GSENSOR_D_TAG, ##__VA_ARGS__)
#define GSENSOR_D_LOGD(...) BK_LOGD(GSENSOR_D_TAG, ##__VA_ARGS__)

static void *gsensor_handle;

static const uint16_t _sp_opcode[] = {
    GSENSOR_OPCODE_INIT,
    GSENSOR_OPCODE_SET_NORMAL_MODE,
    GSENSOR_OPCODE_SET_WAKEUP_MODE,
    GSENSOR_OPCODE_CLOSE,
    GSENSOR_OPCODE_NTF_DATA,
    GSENSOR_OPCODE_WAKEUP,
};

typedef struct {
	gsensor_module_opcode_t op;
	void *param;
} gsensor_demo_msg_t;

#define GSENSOR_G_INT1_PIN       GPIO_48
static beken_thread_t  s_gsensor_demo_thread_hdl = NULL;
static beken_queue_t s_gsensor_demo_msg_que = NULL;

static int gsensor_demo_msg(gsensor_module_opcode_t op_code);
bk_err_t gsensor_demo_init(void);
void gsensor_demo_deinit(void);
bk_err_t gsensor_demo_open();
bk_err_t gsensor_demo_close();
bk_err_t gsensor_demo_set_normal();
bk_err_t gsensor_demo_set_wakeup();
bk_err_t gsensor_demo_lowpower_wakeup();

bk_err_t gsensor_demo_send_msg(int op, void *param)
{
	bk_err_t ret;
	gsensor_demo_msg_t msg;

	msg.op = op;
	if(param)
		msg.param = param;
	if (s_gsensor_demo_msg_que) {
		ret = rtos_push_to_queue(&s_gsensor_demo_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			return BK_FAIL;
		}
	}
	return BK_OK;
}

static int gsensor_demo_check_op_code_support(unsigned short op_code)
{
    for(uint8_t i=0;i<sizeof(_sp_opcode)/sizeof(_sp_opcode[0]);i++)
    {
        if(op_code == _sp_opcode[i])
        {
            return BK_OK;
        }
    }
    return BK_FAIL;
}

static void gsensor_demo_thread(beken_thread_arg_t arg)
{
    bk_err_t ret = kNoErr;

    gsensor_demo_open();

    while(1) {
        gsensor_demo_msg_t msg;
        ret = rtos_pop_from_queue(&s_gsensor_demo_msg_que, &msg, BEKEN_WAIT_FOREVER);
        if (kNoErr == ret) {
             gsensor_demo_msg(msg.op);
        }
    }
}

static void gsensor_data_send_to_arithemtic_module(gsensor_notify_data_ctx_t *ctx)
{
#if CONFIG_GSENSOR_ARITHEMTIC_DEMO_EN
    arithmetic_module_copy_data_send_msg(ctx);
#endif
}

static void gsensor_callback(void *handle,gsensor_data_t *data)
{
    if(data->count != 0)
    {
        gsensor_notify_data_ctx_t *ctx = data;
        gsensor_data_send_to_arithemtic_module(ctx);
    }
    else
    {
        GSENSOR_D_LOGI("gsensor_callback GSENSOR_OPCODE_WAKEUP:%d\r\n", GSENSOR_OPCODE_WAKEUP);
    }
}

void gsensor_lowpower_gpio_wakeup_callback(gpio_id_t gpio_id)
{
    GSENSOR_D_LOGI("%s[%d]\r\n", __func__, gpio_id);
    gsensor_demo_set_normal();
}
bk_err_t gsensor_enter_sleep_config()
{
	bk_gsensor_setMode(gsensor_handle,GSENSOR_MODE_WAKEUP);
	bk_gsensor_open(gsensor_handle);
#if CONFIG_GPIO_WAKEUP_SUPPORT
	gpio_dev_unmap(GSENSOR_G_INT1_PIN);
	GSENSOR_D_LOGI("gsensor set WAKEUP SUCCESS!\r\n");
	bk_gpio_register_isr(GSENSOR_G_INT1_PIN, gsensor_lowpower_gpio_wakeup_callback);
	bk_gpio_register_wakeup_source(GSENSOR_G_INT1_PIN,GPIO_INT_TYPE_FALLING_EDGE);
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
#endif //CONFIG_GPIO_WAKEUP_SUPPORT
	return 0;
}
static int gsensor_demo_msg(gsensor_module_opcode_t op_code)
{
    GSENSOR_D_LOGI("%s ok op_code:%d\r\n", __func__, op_code);

    switch(op_code)
    {
        case GSENSOR_OPCODE_INIT:
        {
            gsensor_handle = bk_gsensor_init("sc7a20");
            if(gsensor_handle != NULL)
            {
                bk_gsensor_setMode(gsensor_handle,GSENSOR_MODE_NOMAL);
                bk_gsensor_setDatarate(gsensor_handle,GSENSOR_DR_50HZ);
                bk_gsensor_setDateRange(gsensor_handle,GSENSOR_RANGE_2G);
                bk_gsensor_registerCallback(gsensor_handle,gsensor_callback);
            }
            else
            {
                GSENSOR_D_LOGI("init gsensor fail\r\n");
            }
        }break;
        case GSENSOR_OPCODE_SET_NORMAL_MODE:
        {
            bk_gsensor_setMode(gsensor_handle,GSENSOR_MODE_NOMAL);
            bk_gsensor_open(gsensor_handle);
        }break;
        case GSENSOR_OPCODE_SET_WAKEUP_MODE:
        {
            bk_gsensor_setMode(gsensor_handle,GSENSOR_MODE_WAKEUP);
            bk_gsensor_open(gsensor_handle);
        }break;
        case GSENSOR_OPCODE_CLOSE:
        {
            bk_gsensor_close(gsensor_handle);
        }break;
        case GSENSOR_OPCODE_LOWPOWER_WAKEUP:
        {
			gsensor_enter_sleep_config();
        }break;
        default:break;
    }

    return BK_OK;
}

bk_err_t gsensor_demo_init(void)
{
    uint32_t ret = 0;

    if((!s_gsensor_demo_msg_que) && (!s_gsensor_demo_thread_hdl)) {

        ret = rtos_init_queue(&s_gsensor_demo_msg_que,
                              "gs_demo_msg_que",
                              sizeof(gsensor_demo_msg_t),
                              10);
        if(ret != kNoErr) {
            return ret;
        }

#if CONFIG_PSRAM_AS_SYS_MEMORY
        ret = rtos_create_psram_thread(&s_gsensor_demo_thread_hdl,
                            3,
                             "gsensor_demo",
                             (beken_thread_function_t)gsensor_demo_thread,
                             1536,
                             NULL);
#else
        ret = rtos_create_thread(&s_gsensor_demo_thread_hdl,
                            3,
                             "gsensor_demo",
                             (beken_thread_function_t)gsensor_demo_thread,
                             1536,
                             NULL);
#endif
        if(ret != kNoErr) {
            if(s_gsensor_demo_msg_que) {
                rtos_deinit_queue(&s_gsensor_demo_msg_que);
                s_gsensor_demo_msg_que = NULL;
            }
        }
    }

#if CONFIG_GSENSOR_ARITHEMTIC_DEMO_EN
	arithmetic_module_init();
#endif
    return ret;
}

void gsensor_demo_deinit(void)
{
    if(s_gsensor_demo_thread_hdl) {
        rtos_delete_thread(&s_gsensor_demo_thread_hdl);
        s_gsensor_demo_thread_hdl = NULL;
    }

    if(s_gsensor_demo_msg_que) {
        rtos_deinit_queue(&s_gsensor_demo_msg_que);
        s_gsensor_demo_msg_que = NULL;
    }

#if CONFIG_GSENSOR_ARITHEMTIC_DEMO_EN
	arithmetic_module_deinit();
#endif
}
bk_err_t gsensor_demo_open()
{
    bk_err_t ret = kNoErr;
    gsensor_demo_msg_t msg;

    msg.op = GSENSOR_OPCODE_INIT;

    if (s_gsensor_demo_msg_que) {
        ret = rtos_push_to_queue(&s_gsensor_demo_msg_que, &msg, BEKEN_NO_WAIT);
        if (kNoErr != ret) {
            GSENSOR_D_LOGE("gsensor_demo_open push_msg fail \r\n");
            return BK_FAIL;
        }
    }
    return ret;
}

bk_err_t gsensor_demo_close()
{
    bk_err_t ret = kNoErr;
    gsensor_demo_msg_t msg;

    msg.op = GSENSOR_OPCODE_CLOSE;

    if (s_gsensor_demo_msg_que) {
        ret = rtos_push_to_queue(&s_gsensor_demo_msg_que, &msg, BEKEN_NO_WAIT);
        if (kNoErr != ret) {
            GSENSOR_D_LOGE("gsensor_demo_close push_msg fail \r\n");
            return BK_FAIL;
        }
    }
    return ret;
}

bk_err_t gsensor_demo_set_normal()
{
    bk_err_t ret = kNoErr;
    gsensor_demo_msg_t msg;

    msg.op = GSENSOR_OPCODE_SET_NORMAL_MODE;

    if (s_gsensor_demo_msg_que) {
        ret = rtos_push_to_queue(&s_gsensor_demo_msg_que, &msg, BEKEN_NO_WAIT);
        if (kNoErr != ret) {
            GSENSOR_D_LOGE("gsensor_demo_set_normal push_msg fail \r\n");
            return BK_FAIL;
        }
    }
    return ret;
}

bk_err_t gsensor_demo_set_wakeup()
{
    bk_err_t ret = kNoErr;
    gsensor_demo_msg_t msg;

    msg.op = GSENSOR_OPCODE_SET_WAKEUP_MODE;

    if (s_gsensor_demo_msg_que) {
        ret = rtos_push_to_queue(&s_gsensor_demo_msg_que, &msg, BEKEN_NO_WAIT);
        if (kNoErr != ret) {
            GSENSOR_D_LOGE("gsensor_demo_set_wakeup push_msg fail \r\n");
            return BK_FAIL;
        }
    }
    return ret;
}

bk_err_t gsensor_demo_lowpower_wakeup()
{
    bk_err_t ret = kNoErr;
    gsensor_demo_msg_t msg;

    msg.op = GSENSOR_OPCODE_LOWPOWER_WAKEUP;

    if (s_gsensor_demo_msg_que) {
        ret = rtos_push_to_queue(&s_gsensor_demo_msg_que, &msg, BEKEN_NO_WAIT);
        if (kNoErr != ret) {
            GSENSOR_D_LOGE("gsensor_demo_lowpower_wakeup push_msg fail \r\n");
            return BK_FAIL;
        }
    }
    return ret;
}

