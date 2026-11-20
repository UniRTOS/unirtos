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
#include <driver/aon_rtc.h>
#include <components/bk_gsensor_arithmetic_demo_public.h>

static int _arithmetic_module_msg(unsigned int type,module_comm_header_t *hdr,void *buffer,unsigned int size);
static void shake_set_onoff(unsigned char onoff);
static void shake_arithmetic_task_handler(void *data_buf);

static signed short xyz_buf[96] = {0};
static unsigned int xyz_len = 0;
static shake_struct shake_info = {0};

/*
    arithmetic step module opcode define
*/
typedef enum{
    /* Send */
    ARITHMETIC_OPCODE_INIT,
    ARITHMETIC_OPCODE_SET_SHAKE_ONOFF,
    /* Notify */
    ARITHMETIC_OPCODE_LIGHT_ON =  100,
    ARITHMETIC_OPCODE_SHAKE_DEVICE,
    /* sync */
}arithmetic_module_opcode_t;

typedef struct {
	arithmetic_status_processing_t op;
	void *param;
} gsensor_demo_msg_t;

typedef struct{
    void *arg;
    arithmetic_status_cb a_s_cb;
}arithmetic_status_cb_t;

static beken_thread_t  s_arithmetic_module_thread_hdl = NULL;
static beken_queue_t s_arithmetic_module_msg_que = NULL;
static arithmetic_status_cb_t arithmetic_s_callback[ARITHMETIC_STATUS_CHECK_MAX] = {0};

bk_err_t arithmetic_module_send_msg(uint32_t op, void *param)
{
	bk_err_t ret;
	gsensor_demo_msg_t msg;

	msg.op = op;
	if(param)
		msg.param = param;
	if (s_arithmetic_module_msg_que) {
		ret = rtos_push_to_queue(&s_arithmetic_module_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			return BK_FAIL;
		}
	}
	return BK_OK;
}

static void gsensor_data_arithmetic_handle(beken_thread_arg_t arg)
{
    bk_err_t ret = kNoErr;

    while(1) {
        gsensor_demo_msg_t msg;
        ret = rtos_pop_from_queue(&s_arithmetic_module_msg_que, &msg, BEKEN_WAIT_FOREVER);
        if (kNoErr == ret) {
            switch(msg.op)
            {
                case ARITHMETIC_SHAKE_CHECK:
                {
                    if(msg.param) {
                        shake_arithmetic_task_handler(msg.param);
                        os_free(msg.param);
                    }
                }break;
                case ARITHMETIC_TILT_CHECK:
                {
                }break;
                case ARITHMETIC_UNIDIRECTIONAL_CHECK:
                {
                }break;
                case ARITHMETIC_COMPOSITE_DIRECTION_CHECK:
                {
                }break;
                default:break;
            }
        }
    }
}

void arithmetic_module_init(void)
{
    uint32_t ret = 0;

    if((!s_arithmetic_module_msg_que) && (!s_arithmetic_module_thread_hdl)) {

        ret = rtos_init_queue(&s_arithmetic_module_msg_que,
                              "arithmetic_msg_que",
                              sizeof(gsensor_demo_msg_t),
                              10);
        if(ret != kNoErr) {
            return;
        }

        ret = rtos_create_thread(&s_arithmetic_module_thread_hdl,
                            3,
                             "arithmetic_module",
                             (beken_thread_function_t)gsensor_data_arithmetic_handle,
                             2048,
                             NULL);
        if(ret != kNoErr) {
            if(s_arithmetic_module_msg_que) {
                rtos_deinit_queue(&s_arithmetic_module_msg_que);
                s_arithmetic_module_msg_que = NULL;
            }
        }
    }

     /* The shake recognition algorithm is enabled by default */
     shake_set_onoff(1);

    return;
}

void arithmetic_module_deinit(void)
{
    if(s_arithmetic_module_thread_hdl) {
        rtos_delete_thread(&s_arithmetic_module_thread_hdl);
        s_arithmetic_module_thread_hdl = NULL;
    }

    if(s_arithmetic_module_msg_que) {
        rtos_deinit_queue(&s_arithmetic_module_msg_que);
        s_arithmetic_module_msg_que = NULL;
    }
}

void arithmetic_module_register_status_callback(arithmetic_status_processing_t status, arithmetic_status_cb cb, void *arg)
{
	if(status >= ARITHMETIC_STATUS_CHECK_MAX) return;

	arithmetic_s_callback[status].a_s_cb = cb;
	arithmetic_s_callback[status].arg = arg;
}

int arithmetic_module_copy_data_send_msg(gsensor_notify_data_ctx_t *gsensor_data)
{
    xyz_len = gsensor_data->count;
    if(xyz_len>0)
    {
        os_memcpy(xyz_buf,&gsensor_data->xyz[0],sizeof(gsensor_data->xyz[0])*xyz_len);
    }

    gsensor_notify_data_ctx_t *xyz_buf_p = NULL;
    unsigned int xyz_count = gsensor_data->count;

    xyz_buf_p = os_malloc(sizeof(gsensor_data_t) + sizeof(gsensor_xyz_t)*xyz_count);
    if(xyz_buf_p) {
        xyz_buf_p->count = xyz_count;
        os_memcpy(&xyz_buf_p->xyz[0],&gsensor_data->xyz[0],sizeof(gsensor_data->xyz[0])*xyz_count);
        arithmetic_module_send_msg(ARITHMETIC_TILT_CHECK, (void *)xyz_buf_p);
        arithmetic_module_send_msg(ARITHMETIC_COMPOSITE_DIRECTION_CHECK, (void *)xyz_buf_p);
        arithmetic_module_send_msg(ARITHMETIC_UNIDIRECTIONAL_CHECK, (void *)xyz_buf_p);
        arithmetic_module_send_msg(ARITHMETIC_SHAKE_CHECK, (void *)xyz_buf_p);
    }

    return BK_OK;
}

unsigned short sqrt_16(unsigned int value)
{
	unsigned short sqrtVal, i;
	unsigned int tmp, ttp;

	if (value == 0)
		return 0;

	sqrtVal = 0;

	tmp = (value >> 30);
	value <<= 2;
	if (tmp > 1)
	{
		sqrtVal ++;
		tmp -= sqrtVal;
	}

	for (i=15; i>0; i--)
	{
		sqrtVal <<= 1;

		tmp <<= 2;
		tmp += (value >> 30);

		ttp = sqrtVal;
		ttp = (ttp<<1)+1;

		value <<= 2;
		if (tmp >= ttp)
		{
			tmp -= ttp;
			sqrtVal++;
		}
	}

	return sqrtVal;
}


/*************************shake_module******************************/
shake_recognition_alg_param_t shake_alg_param;

static void shake_set_onoff(unsigned char onoff)
{
    memset(&shake_info, 0, sizeof(shake_info));
    shake_info.onoff = onoff;

    shake_alg_param.shake_threshold_v = 140;
    shake_alg_param.stability_threshold_v = 130;
    shake_alg_param.shake_check_number = 3;
    shake_alg_param.shake_time_limit_ms = 1200;
}

void shake_arithmetic_set_parameter(shake_recognition_alg_param_t *shake_alg_p)
{
    shake_alg_param.shake_threshold_v = shake_alg_p->shake_threshold_v;
    shake_alg_param.stability_threshold_v = shake_alg_p->stability_threshold_v;
    shake_alg_param.shake_check_number = shake_alg_p->shake_check_number;
    shake_alg_param.shake_time_limit_ms = shake_alg_p->shake_time_limit_ms;
}
static void shake_arithmetic_task_handler(void *param)
{
    signed short x,y,z;
    int sg;

    gsensor_notify_data_ctx_t *g_data = (gsensor_notify_data_ctx_t *)param;

	signed short *data_buf = (signed short *)(&g_data->xyz[0]);
	unsigned int count = g_data->count;

    /* if shake close, so return. */
    if(0 == shake_info.onoff)
        return ;

    uint64_t timestmp = 0;

    uint32_t shake_t_v = shake_alg_param.shake_threshold_v;
    uint32_t stability_t_v = shake_alg_param.stability_threshold_v;
    uint32_t shake_c_n = shake_alg_param.shake_check_number;
    uint64_t shake_t_l_ms = shake_alg_param.shake_time_limit_ms;

    for(unsigned int i = 0;i < count;i+=3)
    {
        /* get x,y,z data */
        x = data_buf[i+0];
        y = data_buf[i+1];
        z = data_buf[i+2];

        sg = x*x + y*y + z*z;
        sg = sqrt_16(sg);

        //os_printf("%s count:%d x:%d y:%d z:%d sg:%d \r\n",__func__,count, x, y, z, sg);
        if(sg > shake_t_v*shake_t_v)
        {
            shake_info.num++;
        }
        else if(sg < stability_t_v*stability_t_v)
        {
            shake_info.num = 0;
            shake_info.y_flag = 0;
        }

        if((shake_info.num > shake_c_n)&&(shake_info.num < 0xFFFF))
        {
            shake_info.num = 0xFFFF;
            //module_communicate_sync(ARITHMETIC_MODULE_ID,TIME_MODULE_ID,TIME_OPCODE_SYNC_GET_TIMESTAMP,&timestmp,&timesize);
            timestmp = bk_aon_rtc_get_ms();

            if((shake_info.last_time + shake_t_l_ms) < timestmp)
            {
                shake_info.last_time = timestmp;
                os_printf("========SHAKE========SHAKE========SHAKE========\r\n");
                if(arithmetic_s_callback[ARITHMETIC_SHAKE_CHECK].a_s_cb)
                {
                    arithmetic_s_callback[ARITHMETIC_SHAKE_CHECK].a_s_cb(arithmetic_s_callback[ARITHMETIC_SHAKE_CHECK].arg, xyz_buf);
                }

            }
            if(shake_info.last_time > timestmp)
            {
                shake_info.last_time = timestmp;
            }
            break;
        }
    }

}


#define ARITHMETIC_SUPPORT_OTHER_DISCRIMINATE_EN 0
#if ARITHMETIC_SUPPORT_OTHER_DISCRIMINATE_EN

static sport_direction_struct m_sport_dir = {0};
ARITHMETIC_MODE_TYPE arithmetic_sport_type = ARITHMETIC_TYPE_NULL;
static sport_info_struct g_sport_info = {0};
static sport_argument_info_struct sport_argument = {0};
static MIX_SPORT_TYPE sport_type = MIX_SPORT_NULL;

static calculate_struct run_cal_Sum = {0};
static smooth_data_struct run_smooth_data = {0};
static int run_temp = 0;
static unsigned char start_run_steps = 0;
static int all_run_steps = 0;

static calculate_struct cal_Sum = {0};
static smooth_data_struct steps_smooth_data = {0};
static int steps_temp = 0, last_steps_temp = 0;
static unsigned char start_steps = 0;
static int all_steps = 0;
static unsigned short dal_num = 0, y_over_num = 0;

static const steps_argument_struct steps_argument[3] =
{
    {
        .DIR_LINE_NUM = S_DIR_LINE_NUM,
        .DIR_CHANGE_N = S_DIR_CHANGE_N,
        .MAX_TIME_TWO_POINT = S_MAX_TIME_TWO_POINT,
        .DIR_DAL_VALUE = S_DIR_DAL_VALUE,
        .AVERAGE_VALUE = S_AVRAGE_VALUE_WATCH_PHONE,
        .MIN_VALUE = S_MIN_VALUE_WATCH_PHONE,
        .START_STEPS = S_START_STEPS_X,
    },
    {
        .DIR_LINE_NUM = S_DIR_LINE_NUM,
        .DIR_CHANGE_N = S_DIR_CHANGE_N,
        .MAX_TIME_TWO_POINT = S_MAX_TIME_TWO_POINT,
        .DIR_DAL_VALUE = S_DIR_DAL_VALUE,
        .AVERAGE_VALUE = S_AVRAGE_VALUE_NORMAL,
        .MIN_VALUE = S_MIN_VALUE_NORMAL,
        .START_STEPS = S_START_STEPS_Y,
    },
    {
        .DIR_LINE_NUM = S_DIR_LINE_NUM_Z,
        .DIR_CHANGE_N = S_DIR_CHANGE_N,
        .MAX_TIME_TWO_POINT = S_MAX_TIME_TWO_POINT,
        .DIR_DAL_VALUE = S_DIR_DAL_VALUE_Z,
        .AVERAGE_VALUE = S_AVRAGE_VALUE_Z,
        .MIN_VALUE = S_MIN_VALUE_Z,
        .START_STEPS = S_START_STEPS_Z,
    },
};

/**************************common******************************/
void clear_smooth_data(smooth_data_struct *smooth_data)
{
    memset(smooth_data, 0, sizeof(smooth_data_struct));
}

signed short smooth_filter_func(smooth_data_struct *smooth_data, signed short acc_data)
{
    signed short average = 0;

    if(smooth_data->first == 0)
    {
        clear_smooth_data(smooth_data);

        smooth_data->first = 1;
        for(unsigned char i = 0;i < SAVE_LEN;i++)
        {
            smooth_data->buf[i] = acc_data;
        }
        smooth_data->sum = acc_data*SAVE_LEN;
    }

    if(smooth_data->pos >= SAVE_LEN)
    smooth_data->pos = 0;

    smooth_data->sum -= smooth_data->buf[smooth_data->pos];

    smooth_data->buf[smooth_data->pos] = acc_data;

    smooth_data->sum += smooth_data->buf[smooth_data->pos];

    average = smooth_data->sum / SAVE_LEN;

    smooth_data->pos++;

    return average;
}

void calculate_run_steps(calculate_struct *cal_axis, signed short value)
{
    if(cal_axis->cur_pos == 0)
    {
        cal_axis->max_value = -32767;
        cal_axis->min_value = 32767;
        cal_axis->old_pos = 0;
        cal_axis->old_value = value;
    }
    cal_axis->cur_pos++;

    if(((cal_axis->cur_pos - cal_axis->old_pos) > cal_axis->MAX_TIME_TWO_POINT)
        ||(cal_axis->dir_num > cal_axis->DIR_LINE_NUM))
    {
        cal_axis->dal_steps = -1;
        cal_axis->dir_change_flag = 0;
        cal_axis->dir_num = 0;
        return ;
    }

    if(cal_axis->up_down == 0) //0: down, 1: up.
    {
        if(value < cal_axis->min_value)
        {
            cal_axis->min_pos = cal_axis->cur_pos;
            cal_axis->min_value = value;
            cal_axis->dir_change = 0;
            if(cal_axis->dir_change_flag != 1)
            {
                cal_axis->dir_change_flag = 1;
                cal_axis->dir_num++;
            }
        }
        else
        {
            if(cal_axis->dir_change_flag != 2)
            {
                cal_axis->dir_change_flag = 2;
                cal_axis->dir_num++;
            }

            cal_axis->max_pos = cal_axis->cur_pos;
            cal_axis->max_value = value;

            if(++cal_axis->dir_change > cal_axis->DIR_CHANGE_N)
            {
                cal_axis->last_min_value = cal_axis->min_value;

                int dal_value = cal_axis->max_value - cal_axis->min_value;
                int average_value = (cal_axis->last_max_value + cal_axis->last_min_value)/2;
                average_value = average_value > 0 ? average_value : (-average_value);

                if(dal_value > cal_axis->DIR_DAL_VALUE)
                {
                    if((cal_axis->dir_num < cal_axis->DIR_LINE_NUM)&&(average_value > cal_axis->AVERAGE_VALUE)
                        &&(ABS(cal_axis->last_max_value) > run_cal_Sum.MIN_VALUE)&&
                          (ABS(cal_axis->last_min_value) > run_cal_Sum.MIN_VALUE))
                    {
                        cal_axis->dal_steps++;
                    }

                    cal_axis->old_pos = cal_axis->min_pos;
                    cal_axis->old_value = cal_axis->min_value;
                    cal_axis->last_max_value = cal_axis->max_value;

                    cal_axis->dir_change = 0;
                    cal_axis->up_down = 1;

                    cal_axis->dir_change_flag = 0;
                    cal_axis->dir_num = 0;
                }
            }
        }
    }
    else
    {
        if(value > cal_axis->max_value)
        {
            cal_axis->max_pos = cal_axis->cur_pos;
            cal_axis->max_value = value;
            cal_axis->dir_change = 0;


            if(cal_axis->dir_change_flag != 3)
            {
                cal_axis->dir_change_flag = 3;
                cal_axis->dir_num++;
            }
        }
        else
        {
            if(cal_axis->dir_change_flag != 4)
            {
                cal_axis->dir_change_flag = 4;
                cal_axis->dir_num++;
            }
            cal_axis->min_pos = cal_axis->cur_pos;
            cal_axis->min_value = value;
            if(++cal_axis->dir_change > cal_axis->DIR_CHANGE_N)
            {
                cal_axis->last_max_value = cal_axis->max_value;

                int dal_value = cal_axis->max_value - cal_axis->min_value;
                int average_value = (cal_axis->last_max_value + cal_axis->last_min_value)/2;
                average_value = average_value > 0 ? average_value : (-average_value);
                if(dal_value > cal_axis->DIR_DAL_VALUE)
                {
                    if((cal_axis->dir_num < cal_axis->DIR_LINE_NUM)&&(average_value > cal_axis->AVERAGE_VALUE)
                        &&(ABS(cal_axis->last_max_value) > run_cal_Sum.MIN_VALUE)&&
                          (ABS(cal_axis->last_min_value) > run_cal_Sum.MIN_VALUE))
                    {
                        cal_axis->dal_steps++;
                    }

                    cal_axis->old_pos = cal_axis->max_pos;
                    cal_axis->old_value = cal_axis->max_value;
                    cal_axis->last_min_value = cal_axis->min_value;

                    cal_axis->dir_change = 0;
                    cal_axis->up_down = 0;

                    cal_axis->dir_change_flag = 0;
                    cal_axis->dir_num = 0;
                }
            }
        }
    }
}

void arithmetic_fifo_set(unsigned char onoff)
{
    memset(&m_sport_dir, 0, sizeof(m_sport_dir));
    if(onoff)
    {
#if 0
        module_communicate_send(ARITHMETIC_MODULE_ID,
                                GSENSOR_MODULE_ID,
                                GSENSOR_OPCODE_SET_NORMAL_MODE,
                                NULL,
                                0);
#endif
    }
    else
    {
#if 0
        module_communicate_send(ARITHMETIC_MODULE_ID,
                                GSENSOR_MODULE_ID,
                                GSENSOR_OPCODE_SET_WAKEUP_MODE,
                                NULL,
                                0);
#endif
    }
}

void save_sport_steps(unsigned int steps)
{
    time_t timestmp = 0;
    //unsigned int timesize = 0;
    /* maybe mix sport has start. */
    //mix_sport_module_send_context_t send_context = {0};
    //send_context.save_steps.step_mode = arithmetic_sport_type;
    //send_context.save_steps.steps = steps;
    //module_communicate_send(ARITHMETIC_MODULE_ID,MIX_SPORT_MODULE_ID,MIX_SPORT_OPCODE_SAVE_STEPS,&send_context,sizeof(send_context));
    /* save steps. */
    sport_argument.current_steps += steps;
    if(sport_argument.current_steps % 2)
    {
        return ;
    }
    //module_communicate_sync(ARITHMETIC_MODULE_ID,TIME_MODULE_ID,TIME_OPCODE_SYNC_GET_TIMESTAMP,&timestmp,&timesize);
    os_printf("%s get timestamp %d\r\n",__func__,timestmp);
    /* save start time. */
    g_sport_info.startSecs = timestmp;
    if(timestmp > sport_argument.start_sec)
    {
        sport_argument.duration += timestmp - sport_argument.start_sec;
        sport_argument.start_sec = timestmp;
    }
    /* save steps. */
    g_sport_info.walkSteps = sport_argument.current_steps/2 + sport_argument.base_steps;
    /* save distance. */
    //g_sport_info.distance = steps_2_distance_with_sport(arithmetic_sport_type, sport_argument.current_steps/2) + sport_argument.base_distance;
    /* save calories. */
    //g_sport_info.calories = steps_2_calorie_with_sport(arithmetic_sport_type, sport_argument.current_steps/2) + sport_argument.base_calories;
    /* save duration. */
    g_sport_info.duration = sport_argument.duration;
    os_printf("g_sport_info: walkSteps = %d, distance = %d, duration = %d, calories = %d\r\n",g_sport_info.walkSteps, g_sport_info.distance,g_sport_info.duration, g_sport_info.calories);
    /* save sport steps to flash. */
    //block_save_sensor_item(DATA_TYPE_REAL_SPORT, (unsigned char *)&g_sport_info);
    /* check step target. */
    //target_check();
}

void save_steps_start_time(void)
{
    time_t timestmp = 0;
    //unsigned int timesize = 0;
    //module_communicate_sync(ARITHMETIC_MODULE_ID,TIME_MODULE_ID,TIME_OPCODE_SYNC_GET_TIMESTAMP,&timestmp,&timesize);
    sport_argument.start_sec = timestmp - 5;
}

void calculate_steps(calculate_struct *cal_axis, signed short value)
{
    if(cal_axis->cur_pos == 0)
    {
        cal_axis->max_value = -32767;
        cal_axis->min_value = 32767;
        cal_axis->old_pos = 0;
        cal_axis->old_value = value;
    }
    cal_axis->cur_pos++;

    if(((cal_axis->cur_pos - cal_axis->old_pos) > cal_axis->MAX_TIME_TWO_POINT)
        ||(cal_axis->dir_num > cal_axis->DIR_LINE_NUM))
    {
        cal_axis->dal_steps = -1;
        cal_axis->dir_change_flag = 0;
        cal_axis->dir_num = 0;
        return ;
    }

    if(cal_axis->up_down == 0) //0: down, 1: up.
    {
        if(value < cal_axis->min_value)
        {
            cal_axis->min_pos = cal_axis->cur_pos;
            cal_axis->min_value = value;
            cal_axis->dir_change = 0;

            if(cal_axis->dir_change_flag != 1)
            {
                cal_axis->dir_change_flag = 1;
                cal_axis->dir_num++;
            }
        }
        else{
            if(cal_axis->dir_change_flag != 2)
            {
                cal_axis->dir_change_flag = 2;
                cal_axis->dir_num++;
            }

            cal_axis->max_pos = cal_axis->cur_pos;
            cal_axis->max_value = value;

            if(++cal_axis->dir_change > cal_axis->DIR_CHANGE_N)
            {
                cal_axis->last_min_value = cal_axis->min_value;

                int dal_value = cal_axis->max_value - cal_axis->min_value;
                int average_value = (cal_axis->last_max_value + cal_axis->last_min_value)/2;
                average_value = average_value > 0 ? average_value : (-average_value);
                if(dal_value > cal_axis->DIR_DAL_VALUE)
                {
                    if((cal_axis->dir_num < cal_axis->DIR_LINE_NUM)&&(average_value > cal_axis->AVERAGE_VALUE)
                        &&(ABS(cal_axis->last_max_value) > cal_Sum.MIN_VALUE)&&
                          (ABS(cal_axis->last_min_value) > cal_Sum.MIN_VALUE))
                    {
                        cal_axis->dal_steps++;
                    }

                    cal_axis->old_pos = cal_axis->min_pos;
                    cal_axis->old_value = cal_axis->min_value;
                    cal_axis->last_max_value = cal_axis->max_value;

                    cal_axis->dir_change = 0;
                    cal_axis->up_down = 1;

                    cal_axis->dir_change_flag = 0;
                    cal_axis->dir_num = 0;
                }
            }
        }
    }
    else
    {
        if(value > cal_axis->max_value)
        {

            cal_axis->max_pos = cal_axis->cur_pos;
            cal_axis->max_value = value;
            cal_axis->dir_change = 0;

            if(cal_axis->dir_change_flag != 3)
            {
                cal_axis->dir_change_flag = 3;
                cal_axis->dir_num++;
            }
        }
        else
        {
            if(cal_axis->dir_change_flag != 4)
            {
                cal_axis->dir_change_flag = 4;
                cal_axis->dir_num++;
            }

            cal_axis->min_pos = cal_axis->cur_pos;
            cal_axis->min_value = value;
            if(++cal_axis->dir_change > cal_axis->DIR_CHANGE_N)
            {
                cal_axis->last_max_value = cal_axis->max_value;

                int dal_value = cal_axis->max_value - cal_axis->min_value;
                int average_value = (cal_axis->last_max_value + cal_axis->last_min_value)/2;
                average_value = average_value > 0 ? average_value : (-average_value);
                if(dal_value > cal_axis->DIR_DAL_VALUE)
                {
                    if((cal_axis->dir_num < cal_axis->DIR_LINE_NUM)&&(average_value > cal_axis->AVERAGE_VALUE)
                        &&(ABS(cal_axis->last_max_value) > cal_Sum.MIN_VALUE)&&
                          (ABS(cal_axis->last_min_value) > cal_Sum.MIN_VALUE))
                    {
                        cal_axis->dal_steps++;
                    }


                    cal_axis->old_pos = cal_axis->max_pos;
                    cal_axis->old_value = cal_axis->max_value;
                    cal_axis->last_min_value = cal_axis->min_value;

                    cal_axis->dir_change = 0;
                    cal_axis->up_down = 0;

                    cal_axis->dir_change_flag = 0;
                    cal_axis->dir_num = 0;
                }
            }
        }
    }
}

/**************************run_module******************************/
void run_arithmetic_stop(void)
{
    os_printf("run_arithmetic_stop, all_run_steps = %d\r\n",all_run_steps/2);
    run_temp = 0;
    start_run_steps = 0;
    /* sport stop. */
    arithmetic_sport_type = ARITHMETIC_TYPE_NULL;
    memset(&run_cal_Sum, 0, sizeof(run_cal_Sum));
    clear_smooth_data(&run_smooth_data);
}


void run_arithmetic(void)
{
    os_printf("%s %d\r\n",__func__,__LINE__);
    signed short x = 0,y = 0,z = 0;
    int sg = 0;

    run_cal_Sum.DIR_LINE_NUM = RUN_S_DIR_LINE_NUM;
    run_cal_Sum.DIR_CHANGE_N = RUN_S_DIR_CHANGE_N;
    run_cal_Sum.MAX_TIME_TWO_POINT = RUN_S_MAX_TIME_TWO_POINT;
    run_cal_Sum.DIR_DAL_VALUE = RUN_S_DIR_DAL_VALUE;
    run_cal_Sum.AVERAGE_VALUE = RUN_S_AVRAGE_VALUE;
    run_cal_Sum.MIN_VALUE = RUN_S_MIN_VALUE_NORMAL;
    /* sport start. */
    arithmetic_sport_type = ARITHMETIC_TYPE_RUN;

    for(unsigned int i = 0;i < xyz_len; i+=3)
    {
        /* get x,y,z data */
        x = xyz_buf[i+0];
        y = xyz_buf[i+1];
        z = xyz_buf[i+2];

        sg = x*x + y*y + z*z;
        sg = sqrt_16(sg);
        //sg = xyz_buf[i+1]; //sg = y.

        /* smooth filter sg. */
        sg = smooth_filter_func(&run_smooth_data, sg);

        calculate_run_steps(&run_cal_Sum,sg);

        /* calculate all steps. */
        if(run_cal_Sum.dal_steps == -1)
        {
            os_printf("the steps stop , all_run_steps = %d\r\n",all_run_steps/2);
            run_temp = 0;
            start_run_steps = 0;

            /* sport stop. */
            arithmetic_sport_type = ARITHMETIC_TYPE_NULL;
            arithmetic_fifo_set(0); //close arithmetic fifo irq.

            memset(&run_cal_Sum, 0, sizeof(run_cal_Sum));

            clear_smooth_data(&run_smooth_data);
        }
        else if(run_cal_Sum.dal_steps > 0)
        {
            run_temp += run_cal_Sum.dal_steps;

            if(run_temp >= 20)
            {
                if(start_run_steps == 0)
                {
                    start_run_steps = 1;
                    if(sport_type != MIX_SPORT_NULL)
                    {
                        all_run_steps += 20;
                        save_steps_start_time();
                        save_sport_steps(20);
                    }
                    else
                    {

                        all_run_steps += 30;
                        save_steps_start_time();
                        save_sport_steps(30);
                    }
                }
                else
                {
                    all_run_steps+=run_cal_Sum.dal_steps;
                    save_sport_steps(run_cal_Sum.dal_steps);
                }
            }
            os_printf("the steps active , all_run_steps = %d\r\n",g_sport_info.walkSteps);
        }
        run_cal_Sum.dal_steps = 0;
    }
}

/*************************step_module******************************/

void steps_arithmetic_stop(void)
{
    os_printf("steps_arithmetic_stop, all_steps = %d\r\n",all_steps/2);
    last_steps_temp = steps_temp;
    dal_num = 0;
    y_over_num = 0;
    steps_temp = 0;
    start_steps = 0;
    /* sport stop. */
    arithmetic_sport_type = ARITHMETIC_TYPE_NULL;
    memset(&cal_Sum, 0, sizeof(cal_Sum));
    clear_smooth_data(&steps_smooth_data);
}

void steps_arithmetic(void)
{
    os_printf("%s %d\r\n",__func__,__LINE__);
    static signed short y_max = 0, y_min = 0;
    static unsigned char last_offset = 0;
    int x_sum = 0,y_sum = 0, z_sum = 0, value = 0, num = 0;
    unsigned char offset = 0, argu_off = 0;
    int sg = 0;

    if(xyz_len == 0)
        return;

    /* sport start. */
    arithmetic_sport_type = ARITHMETIC_TYPE_WALK;

    if(dal_num == 0)
    {
        y_max = y_min = xyz_buf[0];
    }

    for(unsigned int i = 0;i < xyz_len; i+=3)
    {
        //os_printf("xyz_buf1=%d,xyz_buf2=%d,xyz_buf3=%d \r\n",xyz_buf[i+0],xyz_buf[i+1],xyz_buf[i+2]);
        x_sum += xyz_buf[i+0];
        y_sum += xyz_buf[i+1];
        z_sum += xyz_buf[i+2];

        if(y_max < xyz_buf[i+1])
            y_max = xyz_buf[i+1];
        if(y_min > xyz_buf[i+1])
            y_min = xyz_buf[i+1];
    }
    dal_num += xyz_len/3;

    num = xyz_len/3;
    value = z_sum/num;

    os_printf("x_sum=%d,y_sum=%d,z_sum=%d \r\n",x_sum,y_sum,z_sum);
    os_printf("value = %d \r\n",value);
    os_printf("dal_num = %d \r\n",dal_num);

    if(value < -700)
    {
        if(dal_num > 35) //700ms.
        {
            dal_num = 0;
            int dal_y = y_max - y_min;
            if(dal_y > 450)
                y_over_num++;
            else if(dal_y < 250)
                y_over_num = 0;
            if(y_over_num > 3)
            {
                steps_arithmetic_stop();
                arithmetic_fifo_set(0); //close arithmetic fifo irq.
                return;
            }
        }
        offset = 2;
        argu_off = 2;
    }
    else
    {
        dal_num = 0;
        y_over_num = 0;
        if(ABS(x_sum-y_sum) < 50*num)
        {
            if(last_offset == 2)
                last_offset = 0;
            offset = last_offset;
            argu_off = offset;
        }
        else if(ABS(x_sum) > ABS(y_sum))
        {
            offset = 0;
            argu_off = 0;
        }
        else
        {
            offset = 1;
            argu_off = 1;
        }
        if(value > 300)
        {
            if(ABS(x_sum)/num > 500)
            {
                offset = 0;
                argu_off = 0;
            }
            else
            {
                offset = 2;
                argu_off = 0;
            }
        }
    }

    if((start_steps > 0)&&(last_offset != offset))
    {
        steps_temp += 2;
        all_steps+=2;
        save_sport_steps(2);
    }

    if((last_offset != offset)||(0 == cal_Sum.START_STEPS))
    {
        cal_Sum.DIR_LINE_NUM = steps_argument[argu_off].DIR_LINE_NUM;
        cal_Sum.DIR_CHANGE_N = steps_argument[argu_off].DIR_CHANGE_N;
        cal_Sum.MAX_TIME_TWO_POINT = steps_argument[argu_off].MAX_TIME_TWO_POINT;
        cal_Sum.DIR_DAL_VALUE = steps_argument[argu_off].DIR_DAL_VALUE;
        cal_Sum.AVERAGE_VALUE = steps_argument[argu_off].AVERAGE_VALUE;
        cal_Sum.MIN_VALUE = steps_argument[argu_off].MIN_VALUE;
        cal_Sum.START_STEPS = steps_argument[argu_off].START_STEPS;
        if((1 != argu_off)&&(last_steps_temp < 40))
        {
            if(cal_Sum.START_STEPS < 40)
                cal_Sum.START_STEPS = 40;
        }
    }

    last_offset = offset;


    for(unsigned int i = 0;i < xyz_len; i+=3)
    {
        /* get x,y,z data */
        //x = xyz_buf[i+0];
        //y = xyz_buf[i+1];
        //z = xyz_buf[i+2];

        //sg = x*x + y*y + z*z;
        //sg = sqrt_16(sg);
        sg = xyz_buf[i+offset]; //sg = X or Y.

        /* smooth filter sg. */
        sg = smooth_filter_func(&steps_smooth_data, sg);

        calculate_steps(&cal_Sum,sg);

        /* calculate all steps. */
        if(cal_Sum.dal_steps == -1)
        {
            os_printf("the steps stop , all_steps = %d\r\n",all_steps/2);
            steps_arithmetic_stop();
            arithmetic_fifo_set(0); //close arithmetic fifo irq.
            break;
        }
        else if(cal_Sum.dal_steps > 0)
        {
            steps_temp += cal_Sum.dal_steps;
            if(steps_temp >= cal_Sum.START_STEPS)
            {
                if(start_steps == 0)
                {
                    start_steps = 1;
                    all_steps += cal_Sum.START_STEPS+1;
                    save_steps_start_time();
                    save_sport_steps(cal_Sum.START_STEPS+1);
                }
                else
                {
                    all_steps+=cal_Sum.dal_steps;
                    save_sport_steps(cal_Sum.dal_steps);
                }
            }
            os_printf("the steps active , all_steps = %d\r\n",g_sport_info.walkSteps);
        }
        cal_Sum.dal_steps = 0;
    }
}


/*************************handler_gsensor_data******************************/

static void arithmetic_step_module_handler_data(void)
{
    os_printf("%s %d\r\n",__func__,__LINE__);
    for(int i=0;i<xyz_len;i+=3)
    {
        /* calculate the Y direction charge number. */
        if(xyz_buf[i+1] > 600)
        {
            m_sport_dir.y_up++;
        }
        else if(xyz_buf[i+1] < -600)
        {
            m_sport_dir.y_down++;
        }

        /* calculate the Z direction charge number. */
        if(xyz_buf[i+2] > -400)
        {
            m_sport_dir.z_up++;
        }
        else if(xyz_buf[i+2] < -800)
        {
            m_sport_dir.z_down++;
        }
    }

    if(m_sport_dir.num > 75) //50HZ, 1.5秒
    {
        if((m_sport_dir.y_up > 10)&&(m_sport_dir.y_down > 10))
        {
            m_sport_dir.dir = 1;
        }
        else if((m_sport_dir.z_up > 15)&&(m_sport_dir.z_down > 15))
        {
            m_sport_dir.dir = 2;
        }
        else
        {
            m_sport_dir.dir = 0;
        }
        m_sport_dir.num = 0;
        m_sport_dir.y_up = 0;
        m_sport_dir.y_down = 0;
        m_sport_dir.z_up = 0;
        m_sport_dir.z_down = 0;
    }
    os_printf("m_sport_dir.dir = %d\r\n",m_sport_dir.dir);
    if((sport_type == MIX_SPORT_NULL)||(sport_type == MIX_SPORT_WALK))
    {
        if(0 == m_sport_dir.dir) //once direction is walk.
        {
            if(arithmetic_sport_type == ARITHMETIC_TYPE_RUN)
            {
                run_arithmetic_stop();
            }

            /* calculate steps. */
            steps_arithmetic();
        }
        else if(1 == m_sport_dir.dir) //Y two direction is run.
        {
            if(arithmetic_sport_type == ARITHMETIC_TYPE_WALK)
            {
                steps_arithmetic_stop();
            }

            /* calculate run. */
            run_arithmetic();
        }
        else// if(2 == m_sport_dir.dir)
        {

        }
    }
    else
    {
        if(arithmetic_sport_type == ARITHMETIC_TYPE_WALK)
        {
            steps_arithmetic_stop();
        }
        /* calculate run. */
        run_arithmetic();
    }
    memset(&m_sport_dir, 0, sizeof(m_sport_dir));
    memset(&xyz_buf, 0, sizeof(xyz_buf));
    xyz_len = 0;
}
#endif

