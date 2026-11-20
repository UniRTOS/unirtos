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

#define SC7A20_TAG "sc7a20"
#define SC7A20_LOGI(...) BK_LOGI(SC7A20_TAG, ##__VA_ARGS__)
#define SC7A20_LOGW(...) BK_LOGW(SC7A20_TAG, ##__VA_ARGS__)
#define SC7A20_LOGE(...) BK_LOGE(SC7A20_TAG, ##__VA_ARGS__)
#define SC7A20_LOGD(...) BK_LOGD(SC7A20_TAG, ##__VA_ARGS__)



#define GSENSOR_I2C_ID   I2C_ID_0

#define I2C0_SDA_PIN            GPIO_21
#define I2C0_SCL_PIN            GPIO_20
#define GSENSOR_I2C_SDA_PIN     I2C0_SDA_PIN
#define GSENSOR_I2C_SCL_PIN     I2C0_SCL_PIN
#define GSENSOR_G_INT_PIN       GPIO_48

#define SC7A20_IIC_7BITS_ADDR        0x18
#define SC7A20_IIC_8BITS_ADDR        0x30

#define SC7A20_IIC_ADDRESS  SC7A20_IIC_7BITS_ADDR

#define SC7A20_CHIP_ID_ADDRESS    (unsigned char)0x0F
#define SC7A20_CHIP_ID_VALUE      (unsigned char)0x11

#define  SL_SC7A20_CTRL_REG1      (unsigned char)0x20
#define  SL_SC7A20_CTRL_REG2      (unsigned char)0x21
#define  SL_SC7A20_CTRL_REG3      (unsigned char)0x22
#define  SL_SC7A20_CTRL_REG4      (unsigned char)0x23
#define  SL_SC7A20_CTRL_REG5      (unsigned char)0x24
#define  SL_SC7A20_CTRL_REG6      (unsigned char)0x25

#define  SL_SC7A20_STATUS_REG     (unsigned char)0x27

#define  SL_SC7A20_OUT_X_L        (unsigned char)0x28
#define  SL_SC7A20_OUT_X_H        (unsigned char)0x29
#define  SL_SC7A20_OUT_Y_L        (unsigned char)0x2A
#define  SL_SC7A20_OUT_Y_H        (unsigned char)0x2B
#define  SL_SC7A20_OUT_Z_L        (unsigned char)0x2C
#define  SL_SC7A20_OUT_Z_H        (unsigned char)0x2D

#define  SL_SC7A20_FIFO_CTRL_REG  (unsigned char)0x2E
#define  SL_SC7A20_FIFO_SRC_REG   (unsigned char)0x2F

#define  SL_SC7A20_INT1_CFG    	  (unsigned char)0x30
#define  SL_SC7A20_INT1_SRC       (unsigned char)0x31
#define  SL_SC7A20_INT1_THS    	  (unsigned char)0x32
#define  SL_SC7A20_INT1_DURATION  (unsigned char)0x33

#define  SL_SC7A20_INT2_CFG    	  (unsigned char)0x34
#define  SL_SC7A20_INT2_SRC       (unsigned char)0x35
#define  SL_SC7A20_INT2_THS    	  (unsigned char)0x36
#define  SL_SC7A20_INT2_DURATION  (unsigned char)0x37
#define  SL_SC7A20_CLICK_CFG   	  (unsigned char)0x38
#define  SL_SC7A20_CLICK_SRC   	  (unsigned char)0x39
#define  SL_SC7A20_CLICK_THS   	  (unsigned char)0x3A
#define  SL_SC7A20_TIME_LIMIT     (unsigned char)0x3B
#define  SL_SC7A20_TIME_LATENCY   (unsigned char)0x3C
#define  SL_SC7A20_TIME_WINDOW    (unsigned char)0x3D
#define  SL_SC7A20_ACT_THS        (unsigned char)0x3E
#define  SL_SC7A20_ACT_DURATION   (unsigned char)0x3F

/*连续读取数据时的数据寄存器地址*/
#define  SL_SC7A20_DATA_OUT       (unsigned char)(SL_SC7A20_OUT_X_L|0X80)

/**********特殊功能寄存器**********/
/*非原厂技术人员请勿修改*/
#define  SL_SC7A20_MTP_ENABLE    	             0X00
#define  SL_SC7A20_MTP_CFG    	  (unsigned char)0x1E
#define  SL_SC7A20_MTP_VALUE   	  (unsigned char)0x05
#define  SL_SC7A20_SDOI2C_PU_CFG  (unsigned char)0x57
#define  SL_SC7A20_SDO_PU_MSK     (unsigned char)0x08
#define  SL_SC7A20_I2C_PU_MSK     (unsigned char)0x04
#define  SL_SC7A20_HR_ENABLE      (unsigned char)0X08
/*非原厂技术人员请勿修改*/

/***************数据更新速率**加速度计使能**********/
#define  SL_SC7A20_ODR_POWER_DOWN (unsigned char)0x00
#define  SL_SC7A20_ODR_1HZ        (unsigned char)0x17
#define  SL_SC7A20_ODR_10HZ       (unsigned char)0x27
#define  SL_SC7A20_ODR_25HZ       (unsigned char)0x37
#define  SL_SC7A20_ODR_50HZ       (unsigned char)0x47
#define  SL_SC7A20_ODR_100HZ      (unsigned char)0x57
#define  SL_SC7A20_ODR_200HZ      (unsigned char)0x67
#define  SL_SC7A20_ODR_400HZ      (unsigned char)0x77
#define  SL_SC7A20_ODR_1600HZ     (unsigned char)0x87
#define  SL_SC7A20_ODR_1250HZ     (unsigned char)0x97
#define  SL_SC7A20_ODR_5000HZ     (unsigned char)0x9F

#define  SL_SC7A20_LOWER_POWER_ODR_1HZ        (unsigned char)0x1F
#define  SL_SC7A20_LOWER_POWER_ODR_10HZ       (unsigned char)0x2F
#define  SL_SC7A20_LOWER_POWER_ODR_25HZ       (unsigned char)0x3F
#define  SL_SC7A20_LOWER_POWER_ODR_50HZ       (unsigned char)0x4F
#define  SL_SC7A20_LOWER_POWER_ODR_100HZ      (unsigned char)0x5F
#define  SL_SC7A20_LOWER_POWER_ODR_200HZ      (unsigned char)0x6F
#define  SL_SC7A20_LOWER_POWER_ODR_400HZ      (unsigned char)0x7F
/***************数据更新速率**加速度计使能**********/

/***************传感器量程设置**********************/
#define  SL_SC7A20_FS_2G          (unsigned char)0x00
#define  SL_SC7A20_FS_4G          (unsigned char)0x10
#define  SL_SC7A20_FS_8G          (unsigned char)0x20
#define  SL_SC7A20_FS_16G         (unsigned char)0x30
/***************传感器量程设置**********************/

/***取值在0-127之间，此处仅举例****/
#define SL_SC7A20_INT1_THS_2PERCENT   (unsigned char)0x02
#define SL_SC7A20_INT1_THS_5PERCENT   (unsigned char)0x06
#define SL_SC7A20_INT1_THS_10PERCENT  (unsigned char)0x0C
#define SL_SC7A20_INT1_THS_20PERCENT  (unsigned char)0x18
#define SL_SC7A20_INT1_THS_40PERCENT  (unsigned char)0x30
#define SL_SC7A20_INT1_THS_80PERCENT  (unsigned char)0x60


/***取值在0-127之间，此处仅举例 乘以ODR单位时间****/
#define SL_SC7A20_INT1_DURATION_2CLK  (unsigned char)0x02
#define SL_SC7A20_INT1_DURATION_5CLK  (unsigned char)0x05
#define SL_SC7A20_INT1_DURATION_10CLK (unsigned char)0x0A

/***中断有效时的电平设置，高电平相当于上升沿，低电平相当于下降沿****/
#define SL_SC7A20_INT_ACTIVE_LOWER_LEVEL 0x02 //0x02:中断时INT1脚输出 低电平
#define SL_SC7A20_INT_ACTIVE_HIGH_LEVEL  0x00 //0x00:中断时INT1脚输出 高电平

/***中断有效时的电平设置，高电平相当于上升沿，低电平相当于下降沿****/
#define SL_SC7A20_INT_AOI1_INT1          0x40 //AOI1 TO INT1
#define SL_SC7A20_INT_AOI2_INT1          0x20 //AOI2 TO INT1

typedef void    *gsensor_timer_handle;
typedef void (*gsensor_timer_callback_t)(gsensor_timer_handle timer,void *uarg);

typedef struct{
    beken2_timer_t timer;
    uint8_t type;
    gsensor_timer_callback_t callback;
}app_timer_bk7258_t;

typedef enum{
    GSENSOR_TIMER_TYPE_ONESHOT,
    GSENSOR_TIMER_TYPE_REPEAT,
}GSENSOR_TIMER_TYPE_T;

static beken_semaphore_t s_gsensor_sc7a20_event_wait = NULL;
static beken_thread_t s_gsensor_sc7a20_thread = NULL;
static gsensor_cb datacb = NULL;
static gsensor_mode_t runmode;
static gsensor_dr_t datarate;
static gsensor_range_t datarange;
static uint8_t is_running = 0;
static gsensor_timer_handle gsensor_check_timer;

static void gsensor_check_timer_handler(gsensor_timer_handle timer,void *uarg);
static uint8_t sc7a20_i2c_read(unsigned char reg, unsigned char len, unsigned char *buf);
static uint8_t sc7a20_i2c_write(unsigned char reg, unsigned char data);
static int sc7a20_init(void);
static void sc7a20_deinit(void);
static int sc7a20_open(void);
static void sc7a20_close(void);
static int sc7a20_setDatarate(gsensor_dr_t dr);
static int sc7a20_setMode(gsensor_mode_t mode);
static int sc7a20_setDataRange(gsensor_range_t rg);
static int sc7a20_registerCallback(gsensor_cb cb);

const gsensor_device_t gs_sc7a20 =
{
    .name = "sc7a20",
    .init = sc7a20_init,
    .deinit = sc7a20_deinit,
    .open = sc7a20_open,
    .close = sc7a20_close,
    .setDatarate = sc7a20_setDatarate,
    .setMode = sc7a20_setMode,
    .setDataRange = sc7a20_setDataRange,
    .registerCallback = sc7a20_registerCallback,
};

static void gsensor_sc7a20_event_handler(gsensor_mode_t t_runmode)
{
    if(t_runmode == GSENSOR_MODE_NOMAL)
    {
        unsigned char fifodepth;
        unsigned char sc7a20_data[6];

        sc7a20_i2c_read(SL_SC7A20_FIFO_SRC_REG, 1, &fifodepth);
        if((fifodepth & 0x40)==0x40)
            fifodepth = 32;
        else
            fifodepth = fifodepth&0x1f;
        //os_printf("gsensor fifo:%d\r\n",fifodepth);
        if(fifodepth == 0)
            return;

        gsensor_data_t *dat = os_malloc(sizeof(gsensor_data_t) + sizeof(gsensor_xyz_t)*fifodepth);
        if(dat == NULL)
        {
            SC7A20_LOGE("gsensor_check_timer_handler malloc fail!\n");
            return;
        }
        dat->count = fifodepth;
        for(int i=0;i<fifodepth;i++)
        {
            sc7a20_i2c_read(SL_SC7A20_DATA_OUT,6, &sc7a20_data[0]);
            dat->xyz[i].x = (short)((sc7a20_data[1]<<8)|sc7a20_data[0]);
            dat->xyz[i].y = (short)((sc7a20_data[3]<<8)|sc7a20_data[2]);
            dat->xyz[i].z = (short)((sc7a20_data[5]<<8)|sc7a20_data[4]);
            //os_printf("xyz:%d,%d,%d\r\n",dat->xyz[i].x,dat->xyz[i].y,dat->xyz[i].z);
        }
        if(datacb)
            datacb((void*)&gs_sc7a20,dat);
        os_free(dat);
    }
    else if(t_runmode == GSENSOR_MODE_WAKEUP)
    {
        gsensor_data_t nulld;
        nulld.count = 0;
        if(datacb)
            datacb((void*)&gs_sc7a20,&nulld);
    }
}

static void gsensor_sc7a20_thread(beken_thread_arg_t arg)
{
    bk_err_t ret = kNoErr;

    while(1) {
        ret = rtos_get_semaphore(&s_gsensor_sc7a20_event_wait, BEKEN_WAIT_FOREVER);
        if (kNoErr == ret) {
             gsensor_sc7a20_event_handler(runmode);
        }
    }
}

static void gsensor_task_init()
{
    uint32_t ret = 0;

    if((!s_gsensor_sc7a20_event_wait) && (!s_gsensor_sc7a20_thread)) {

        ret = rtos_init_semaphore(&s_gsensor_sc7a20_event_wait, 1);
        if(ret != kNoErr) {
            return;
        }

#if CONFIG_PSRAM_AS_SYS_MEMORY
        ret = rtos_create_psram_thread(&s_gsensor_sc7a20_thread,
                             5,
                             "gsensor_sc7a20",
                             (beken_thread_function_t)gsensor_sc7a20_thread,
                             1024,
                             NULL);
#else
        ret = rtos_create_thread(&s_gsensor_sc7a20_thread,
                             5,
                             "gsensor_sc7a20",
                             (beken_thread_function_t)gsensor_sc7a20_thread,
                             1024,
                             NULL);
#endif
        if(ret != kNoErr) {
            if(s_gsensor_sc7a20_event_wait) {
                rtos_deinit_semaphore(&s_gsensor_sc7a20_event_wait);
                s_gsensor_sc7a20_event_wait = NULL;
            }
        }
    }
}

static void gsensor_task_deinit()
{
    if(s_gsensor_sc7a20_event_wait) {
        rtos_deinit_semaphore(&s_gsensor_sc7a20_event_wait);
        s_gsensor_sc7a20_event_wait = NULL;
    }

    if (s_gsensor_sc7a20_thread) {
        rtos_delete_thread(&s_gsensor_sc7a20_thread);
        s_gsensor_sc7a20_thread = NULL;
    }
}

static void gsensor_check_timer_handler(gsensor_timer_handle timer,void *uarg)
{
	rtos_set_semaphore(&s_gsensor_sc7a20_event_wait);
}

static void gsensor_timer_handler(void *Larg, void *Rarg)
{
    app_timer_bk7258_t *app_timer = Larg;
    if(app_timer->type == GSENSOR_TIMER_TYPE_REPEAT)
    {
        rtos_oneshot_reload_timer(&app_timer->timer);
    }
    if(app_timer->callback)
    {
        app_timer->callback(app_timer,Rarg);
    }
}

static void gsensor_timer_create(gsensor_timer_handle* timer, GSENSOR_TIMER_TYPE_T type, unsigned int timeout,
                       gsensor_timer_callback_t callback, void* uarg)
{
    app_timer_bk7258_t *app_timer = os_malloc(sizeof(app_timer_bk7258_t));
    os_memset(app_timer,0,sizeof(app_timer_bk7258_t));
    app_timer->type = type;
    app_timer->callback = callback;
    int err = rtos_init_oneshot_timer(&app_timer->timer,
                                    timeout,
                                    gsensor_timer_handler,
                                    app_timer,
                                    uarg);

    SC7A20_LOGI("%s app_timer %p\r\n",__func__,app_timer);
    if(err != 0)
    {
        SC7A20_LOGE("%s err %d\r\n",__func__,err);
        return;
    }
    *timer = app_timer;

}

static void gsensor_timer_destory(gsensor_timer_handle* timer)
{
    app_timer_bk7258_t *p_app_timer = *timer;
    if(p_app_timer ==  NULL)
    {
        return;
    }

    if(rtos_is_oneshot_timer_running(&p_app_timer->timer))
    {
        rtos_stop_oneshot_timer(&p_app_timer->timer);
    }
    rtos_deinit_oneshot_timer(&p_app_timer->timer);
    os_free(p_app_timer);
    *timer = NULL;

}

static void gsensor_timer_stop(gsensor_timer_handle timer)
{
    app_timer_bk7258_t *p_app_timer = (app_timer_bk7258_t*)timer;
    if(p_app_timer ==  NULL)
    {
        return ;
    }

    rtos_stop_oneshot_timer(&p_app_timer->timer);
}

static void gsensor_timer_start(gsensor_timer_handle timer)
{
    app_timer_bk7258_t *p_app_timer = (app_timer_bk7258_t*)timer;
    if(p_app_timer ==  NULL)
    {
        return ;
    }

    if(!rtos_is_oneshot_timer_running(&p_app_timer->timer))
    {
        rtos_start_oneshot_timer(&p_app_timer->timer);
    }

}
static void gsensor_timer_restart(gsensor_timer_handle timer, GSENSOR_TIMER_TYPE_T type, unsigned int timeout,
                       gsensor_timer_callback_t callback, void* uarg)
{
    app_timer_bk7258_t *p_app_timer = (app_timer_bk7258_t*)timer;
    if(p_app_timer ==  NULL)
    {
        return ;
    }

    p_app_timer->type = type;
    p_app_timer->callback = callback;
    int err = rtos_oneshot_reload_timer_ex(&p_app_timer->timer,
                                            timeout,
                                            gsensor_timer_handler,
                                            p_app_timer,
                                            uarg);
    if(err != 0)
    {
        SC7A20_LOGE("%s err %d\r\n",__func__,err);
    }

}

static void gsensor_gpio_config(gpio_id_t index, gpio_io_mode_t dir, gpio_pull_mode_t pull, gpio_func_mode_t peir)
{
    if(index >= GPIO_NUM)
    {
        return;
    }
    gpio_dev_unmap(index);
    gpio_config_t cfg;
    cfg.io_mode = dir;
    cfg.pull_mode = pull;
    cfg.func_mode = peir;
    bk_gpio_set_config(index, &cfg);
}


typedef struct
{
    uint32_t i2c_id:2;
    uint32_t device_addr:10;
    uint32_t device_addr_mode:1; //0 7bit, 1 10 bits
    uint32_t reg_addr:16;
    uint32_t reg_addr_mode:1; //0 1byte, 1 2bytes
    uint32_t :2;
    uint8_t *   data;
    uint32_t data_len;
}i2c_transfer_t;

static bk_err_t gsensor_i2c_write(i2c_transfer_t *trx)
{
    if(trx == NULL)
    {
        SC7A20_LOGE("err,%s,para is NULL\r\n", __func__);
        return BK_FAIL;
    }
    bk_err_t ret = 0;
    i2c_mem_param_t mem_param={0};
    mem_param.dev_addr = trx->device_addr;
    mem_param.mem_addr = trx->reg_addr;
    mem_param.mem_addr_size = trx->reg_addr_mode;
    mem_param.data = trx->data;
    mem_param.data_size = trx->data_len;
    mem_param.timeout_ms = 2000;//2s
    ret = bk_i2c_memory_write(trx->i2c_id, &mem_param);
    return ret;
}

static bk_err_t gsensor_i2c_read(i2c_transfer_t *trx)
{
    if(trx == NULL)
    {
        SC7A20_LOGE("err,%s,para is NULL\r\n", __func__);
        return BK_FAIL;
    }
    bk_err_t ret = 0;

    i2c_mem_param_t mem_param={0};
    mem_param.dev_addr = trx->device_addr;
    mem_param.mem_addr = trx->reg_addr;
    mem_param.mem_addr_size = trx->reg_addr_mode;
    mem_param.data = trx->data;
    mem_param.data_size = trx->data_len;
    mem_param.timeout_ms = 2000;//2s

    ret = bk_i2c_memory_read(trx->i2c_id, &mem_param);
    return ret;

}

static uint8_t sc7a20_i2c_read(unsigned char reg, unsigned char len, unsigned char *buf)
{
        i2c_transfer_t tran = {0};
        tran.i2c_id = GSENSOR_I2C_ID;
        tran.device_addr_mode = 0;
        tran.device_addr = SC7A20_IIC_ADDRESS;
        tran.reg_addr_mode = 0;
        tran.reg_addr = reg;
        tran.data = buf;
        tran.data_len = len;
        bk_err_t ret = gsensor_i2c_read(&tran);
        if(ret != BK_OK)
            return -1;
        return 0;
}

static uint8_t sc7a20_i2c_write(unsigned char reg, unsigned char data)
{
        i2c_transfer_t tran = {0};
        tran.i2c_id = GSENSOR_I2C_ID;
        tran.device_addr_mode = 0;
        tran.device_addr = SC7A20_IIC_ADDRESS;
        tran.reg_addr_mode = 0;
        tran.reg_addr = reg;
        tran.data = &data;
        tran.data_len = sizeof(data);
        bk_err_t ret = gsensor_i2c_write(&tran);
        if(ret != BK_OK)
            return -1;
        return 0;
}

static void gsensor_isr(gpio_id_t gpio_id)
{
    gsensor_timer_restart(gsensor_check_timer,GSENSOR_TIMER_TYPE_ONESHOT,3,gsensor_check_timer_handler,NULL);
}

static int sc7a20_init(void)
{
    gsensor_task_init();
    gsensor_gpio_config(GSENSOR_G_INT_PIN,GPIO_INPUT_ENABLE,GPIO_PULL_UP_EN,GPIO_SECOND_FUNC_DISABLE);
    bk_gpio_register_isr(GSENSOR_G_INT_PIN , gsensor_isr);
    bk_gpio_set_interrupt_type(GSENSOR_G_INT_PIN, GPIO_INT_TYPE_FALLING_EDGE);

    gsensor_timer_create(&gsensor_check_timer,GSENSOR_TIMER_TYPE_REPEAT, 450,gsensor_check_timer_handler,NULL);

    i2c_config_t i2c_cfg = {0};
    i2c_cfg.baud_rate = 100000;
    i2c_cfg.addr_mode = I2C_ADDR_MODE_7BIT;
    i2c_cfg.slave_addr = SC7A20_IIC_ADDRESS;
    bk_i2c_init(GSENSOR_I2C_ID, &i2c_cfg);
    //bsp_i2c_init(GSENSOR_I2C_ID,GSENSOR_I2C_SCL_PIN,GSENSOR_I2C_SDA_PIN,&i2c_cfg);

    uint8_t ret;
    unsigned char SL_Read_Reg = 0xFF;

    ret = sc7a20_i2c_read(SC7A20_CHIP_ID_ADDRESS, 1, &SL_Read_Reg);
    if(ret != 0)
    {
        SC7A20_LOGE("read chip id fail\r\n");
        return -1;
    }
    os_printf("SL_Read_Reg = %d\r\n",SL_Read_Reg);
    if(SL_Read_Reg != SC7A20_CHIP_ID_VALUE)
    {
        SC7A20_LOGE("gsensor read chip id 0x%02x is wrong\r\n", SL_Read_Reg);
        return -1;
    }
    sc7a20_i2c_write(SL_SC7A20_CTRL_REG1,0x08);
    runmode = GSENSOR_MODE_NOMAL;
    datarate = GSENSOR_DR_50HZ;
    datarange = GSENSOR_RANGE_2G;
    is_running = 0;

    return 0;
}

static void sc7a20_deinit(void)
{
    if(is_running)
    {
        gsensor_timer_stop(gsensor_check_timer);
        bk_gpio_disable_interrupt(GSENSOR_G_INT_PIN);
        is_running = 0;
    }
    gsensor_timer_destory(&gsensor_check_timer);
    bk_i2c_deinit(GSENSOR_I2C_ID);
    //bsp_i2c_deinit(GSENSOR_I2C_ID);
    gsensor_gpio_config(GSENSOR_G_INT_PIN,GPIO_IO_DISABLE,GPIO_PULL_DISABLE,GPIO_SECOND_FUNC_DISABLE);

    gsensor_task_deinit();
}

static int sc7a20_open(void)
{
    if(is_running) return 0;
    rtos_delay_milliseconds(10);
    if(runmode == GSENSOR_MODE_NOMAL)
    {
        uint8_t drreg = 0;
        switch(datarate)
        {
            case GSENSOR_DR_1HZ:
                drreg = 0x10;
            break;
            case GSENSOR_DR_10HZ:
                drreg = 0x20;
            break;
            case GSENSOR_DR_25HZ:
                drreg = 0x30;
            break;
            case GSENSOR_DR_50HZ:
                drreg = 0x40;
            break;
            case GSENSOR_DR_100HZ:
                drreg = 0x50;
            break;
            case GSENSOR_DR_200HZ:
                drreg = 0x60;
            break;
            case GSENSOR_DR_400HZ:
                drreg = 0x70;
            break;
            default:
                drreg = 0x50;
            break;
        }
        switch(datarange)
        {
            case GSENSOR_RANGE_2G:
                sc7a20_i2c_write(SL_SC7A20_CTRL_REG4,0x00);
            break;
            case GSENSOR_RANGE_4G:
                sc7a20_i2c_write(SL_SC7A20_CTRL_REG4,0x10);
            break;
            case GSENSOR_RANGE_8G:
                sc7a20_i2c_write(SL_SC7A20_CTRL_REG4,0x20);
            break;
            case GSENSOR_RANGE_16G:
                sc7a20_i2c_write(SL_SC7A20_CTRL_REG4,0x30);
            break;
            default:
            break;
        }
        sc7a20_i2c_write(SL_SC7A20_FIFO_CTRL_REG,0x00);
        sc7a20_i2c_write(SL_SC7A20_CTRL_REG1,(drreg|0x07));
        sc7a20_i2c_write(SL_SC7A20_FIFO_CTRL_REG,(0x40|20));
        sc7a20_i2c_write(SL_SC7A20_CTRL_REG5,0x40);
        gsensor_timer_restart(gsensor_check_timer,GSENSOR_TIMER_TYPE_REPEAT,450,gsensor_check_timer_handler,NULL);
    }
    else if(runmode == GSENSOR_MODE_WAKEUP)
    {
        sc7a20_i2c_write(SL_SC7A20_CTRL_REG1,0x2f);
        sc7a20_i2c_write(SL_SC7A20_CTRL_REG2,0x81);
        sc7a20_i2c_write(SL_SC7A20_CTRL_REG3,0x40);
        sc7a20_i2c_write(SL_SC7A20_CTRL_REG4,0x98);
        sc7a20_i2c_write(SL_SC7A20_CTRL_REG5,0x00);
        sc7a20_i2c_write(SL_SC7A20_CTRL_REG6,0x02);
        sc7a20_i2c_write(SL_SC7A20_SDOI2C_PU_CFG,0xc);
        sc7a20_i2c_write(SL_SC7A20_INT1_CFG,0xaa);//Draw an similar S-shaped gesture to wake up
        sc7a20_i2c_write(SL_SC7A20_INT1_THS,0x02);//Threshold value
        sc7a20_i2c_write(SL_SC7A20_INT1_DURATION,0x02);

        bk_gpio_enable_interrupt(GSENSOR_G_INT_PIN);
    }
    is_running = 1;
    return 0;
}

static void sc7a20_close(void)
{
    if(!is_running) return;
    gsensor_timer_stop(gsensor_check_timer);
    bk_gpio_disable_interrupt(GSENSOR_G_INT_PIN);
    sc7a20_i2c_write(SL_SC7A20_CTRL_REG1,0x08);
    is_running = 0;
}

static int sc7a20_setDatarate(gsensor_dr_t dr)
{
    if(dr == datarate) return 0;
    datarate = dr;
    if(is_running && runmode == GSENSOR_MODE_NOMAL)
    {
        uint8_t drreg = 0;
        switch(datarate)
        {
            case GSENSOR_DR_1HZ:
                drreg = 0x10;
            break;
            case GSENSOR_DR_10HZ:
                drreg = 0x20;
            break;
            case GSENSOR_DR_25HZ:
                drreg = 0x30;
            break;
            case GSENSOR_DR_50HZ:
                drreg = 0x40;
            break;
            case GSENSOR_DR_100HZ:
                drreg = 0x50;
            break;
            case GSENSOR_DR_200HZ:
                drreg = 0x60;
            break;
            case GSENSOR_DR_400HZ:
                drreg = 0x70;
            break;
            default:
                drreg = 0x50;
            break;
        }
        unsigned char rReg = 0xFF;
        sc7a20_i2c_read(SL_SC7A20_CTRL_REG1, 1, &rReg);
        drreg |= rReg&0x0f;
        sc7a20_i2c_write(SL_SC7A20_CTRL_REG1,drreg);
    }
    return 0;
}

static int sc7a20_setMode(gsensor_mode_t mode)
{
    if(runmode == mode) return 0;
    runmode = mode;
    if(is_running)
    {
        sc7a20_close();
        rtos_delay_milliseconds(10);
        sc7a20_open();
    }
    return 0;
}

static int sc7a20_setDataRange(gsensor_range_t rg)
{
    if(datarange == rg) return 0;
    datarange = rg;
    if(is_running && runmode == GSENSOR_MODE_NOMAL)
    {
        switch(datarange)
        {
            case GSENSOR_RANGE_2G:
                sc7a20_i2c_write(SL_SC7A20_CTRL_REG4,0x00);
            break;
            case GSENSOR_RANGE_4G:
                sc7a20_i2c_write(SL_SC7A20_CTRL_REG4,0x10);
            break;
            case GSENSOR_RANGE_8G:
                sc7a20_i2c_write(SL_SC7A20_CTRL_REG4,0x20);
            break;
            case GSENSOR_RANGE_16G:
                sc7a20_i2c_write(SL_SC7A20_CTRL_REG4,0x30);
            break;
            default:
            break;
        }
    }
    return 0;
}

static int sc7a20_registerCallback(gsensor_cb cb)
{
    datacb = cb;
    return 0;
}

