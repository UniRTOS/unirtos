#ifndef _ARITHMETIC_MODULE_PUBLIC_H_
#define _ARITHMETIC_MODULE_PUBLIC_H_

typedef struct
{
	unsigned char onoff;			// 0: close, 1: open.
	unsigned char start_hour;		//start time : hour.
	unsigned char start_min;		//start time : min.
	unsigned char end_hour;		//end time : hour.
	unsigned char end_min;			//end time : min.
}hand_rise_switch_struct;

typedef struct
{
    unsigned short num;
    unsigned short y_up;
    unsigned short y_down;
    unsigned short z_up;
    unsigned short z_down;
    unsigned char dir; //0: 单方向数据变化, 1: 双方向数据变化, 2: 杂乱数据变化.
}sport_direction_struct;

typedef enum
{
    MIX_SPORT_STOP = 0,
    MIX_SPORT_START,
    MIX_SPORT_PAUSE,
}MIX_SPORT_STATUS_TYPE;

typedef struct
{
    unsigned int start_sec; //the start time.
    unsigned int current_steps;    //the persion current steps.
    unsigned int base_steps;        //the base steps, maybe include other person all steps.
    unsigned int base_distance;    //the base distance, maybe include other person all distance.
    unsigned int base_calories;    //the base calories, maybe include other person all calories.
    unsigned int duration;    //the steps all time.
}sport_argument_info_struct;

#define SAVE_LEN    8

typedef struct
{
    unsigned char first;
    unsigned char pos;
    signed short buf[SAVE_LEN];
    int sum;
}smooth_data_struct;

#define    ABS(x)        (((x) > 0)?(x):(-(x))) //calculate absolute value.

#define RUN_G_50HZ        0
#define RUN_G_25HZ        1
#define RUN_G_10HZ        2

#define RUN_G_HZ            RUN_G_50HZ

#if (RUN_G_HZ == RUN_G_50HZ)
    #define RUN_S_DIR_CHANGE_N    5   //方向变化的最大计数值，超过这个数表示方向改变 , 10
#elif (RUN_G_HZ == RUN_G_25HZ)
    #define RUN_S_DIR_CHANGE_N      4   //方向变化的最大计数值，超过这个数表示方向改变 , 10
#endif
#define RUN_S_DIR_LINE_NUM    8   //方向更变的最大线条数，超过这个数表示抖动非常剧烈
#define RUN_S_DIR_DAL_VALUE   200  //方向改变的差值 , 4000
#define RUN_S_AVRAGE_VALUE    500
#define RUN_S_MAX_TIME_TWO_POINT  100 //连续两点之间的最大时间差值, 2秒
#define RUN_S_MIN_VALUE_NORMAL    400

#define G_50HZ        0
#define G_25HZ        1
#define G_10HZ        2

#define G_HZ            G_50HZ

#if (G_HZ == G_50HZ)
    #define S_DIR_CHANGE_N            6   //方向变化的最大计数值，超过这个数表示方向改变 , 10
#elif (G_HZ == G_25HZ)
    #define S_DIR_CHANGE_N            4   //方向变化的最大计数值，超过这个数表示方向改变 , 10
#endif
#define S_DIR_LINE_NUM                8   //方向更变的最大线条数，超过这个数表示抖动非常剧烈
#define S_DIR_LINE_NUM_Z                4   //方向更变的最大线条数，超过这个数表示抖动非常剧烈
#define S_DIR_DAL_VALUE                150  //方向改变的差值 , 4000
#define S_DIR_DAL_VALUE_Z            200  //方向改变的差值 , 4000
#define S_AVRAGE_VALUE_NORMAL        700
#define S_AVRAGE_VALUE_WATCH_PHONE    600
#define S_AVRAGE_VALUE_Z                700
#define S_MIN_VALUE_NORMAL            300
#define S_MIN_VALUE_WATCH_PHONE        300
#define S_MIN_VALUE_Z                600
#define S_START_STEPS_X                24
#define S_START_STEPS_Y                20
#define S_START_STEPS_Z                24
#define S_MAX_TIME_TWO_POINT  100 //连续两点之间的最大时间差值, 2秒

typedef enum{
    MODULE_PRIORITY_USER,       //Module Priority : For User,  Same as low priority level
    MODULE_PRIORITY_LOW,        //Module Priority : Low 
    MODULE_PRIORITY_MEDIUM,     //Module Priority : Medium 
    MODULE_PRIORITY_HIGH,       //Module Priority : Heigh 
    MODULE_PRIORITY_SYS,        //Module Priority : System， application don't use 
    MODULE_PRIORITY_NUM
}module_priority_t;

typedef enum{
    COMM_SUCCESS = 0,           //SUCCESS
    COMM_NOT_SUPPORT = -1,      //op_code not support
    COMM_BUSY = -2,             //Still wait req_op_code response  
    COMM_NO_MEMORY = -3,        //Malloc Failed
    COMM_NO_NEED_RSP = -4,      //No need to response
    COMM_PARAM_ERR  = -5,       //Param  Error
    COMM_SYS_ERR    =  -6,      //System Error
    COMM_MODULE_NOT_EXIST= -7,  //Module not exist
}communicate_err_code;

typedef enum {
    COMM_PACKET_REQ,
    COMM_PACKET_RSP,
    COMM_PACKET_SEND,
    COMM_PACKET_NOTIFY,
}module_comm_packet_type;

typedef enum
{
    MIX_SPORT_NULL = 0,
    MIX_SPORT_BREATHE,          //呼吸
    MIX_SPORT_CYCLING,          //骑车或室外骑车
    MIX_SPORT_CYCLING_INDOOR,   //室内骑车
    MIX_SPORT_RUNNING_MACHINE,  //跑步机或室内跑步
    MIX_SPORT_RUN,              //跑步或室外跑步
    MIX_SPORT_SWIM,             //游泳
    MIX_SPORT_WALK,             //走路
    MIX_SPORT_WEIGHT,           //举重
    MIX_SPORT_YOGA,             //瑜咖
    MIX_SPORT_BADMINTON,        //羽毛球
    MIX_SPORT_BASKETBALL,       //篮球
    MIX_SPORT_SKIP,             //跳绳
    MIX_SPORT_FREE_EXERCISE,    //自由锻炼
    MIX_SPORT_FOOTBALL,         //足球
    MIX_SPORT_CLIMBING,         //爬山
    MIX_SPORT_PINGPONG,         //乒乓球
    MIX_SPORT_MAX
}MIX_SPORT_TYPE;

/* mix sport  */
typedef struct
{
    unsigned char start_time[4];
    unsigned char end_time[4];
    unsigned char distance[4];
    unsigned char aver_heartrate;        //aver heartrate
    unsigned char max_heartrate;
    unsigned char step[4];
    unsigned char calories[4];
    unsigned char aver_speed[2];        //dispaly is current speed, result is aver speed
    unsigned char max_speed[2];
}mix_sport_info_struct;

typedef struct
{
    unsigned char mix_sport_type; //see MIX_SPORT_TYPE
    union
    {
        unsigned char data[36];
        mix_sport_info_struct info;
    }type;
}mix_sport_struct;

typedef union{
    struct{
        unsigned char mix_sport_type; //see MIX_SPORT_TYPE
    }set_mix_sport_type;
    struct{
        unsigned char step_mode;
        unsigned int steps;
    }save_steps;
    struct{
        unsigned int heart;
    }save_heart;
}mix_sport_module_send_context_t;

typedef union{
    struct{
        unsigned char mix_sport_type; //see MIX_SPORT_TYPE
    }get_mix_sport_type;
}mix_sport_module_sync_context_t;

typedef union{
    struct{
        unsigned short id:14;
        unsigned short cpu:2;
    };
    unsigned short full;
}module_id_t;

typedef struct{
    unsigned short src_module_id;
    unsigned short dst_module_id;
    unsigned short op_code;
    unsigned short rsp_op_code;
}module_comm_request_packet_header_t;

typedef struct{
    unsigned short src_module_id;
    unsigned short dst_module_id;
    unsigned short op_code;
}module_comm_response_packet_header_t;

typedef struct{
    unsigned short src_module_id;
    unsigned short dst_module_id;
    unsigned short op_code;
}module_comm_send_packet_header_t;

typedef struct{
    unsigned short module_id;
    unsigned short op_code;
}module_comm_notify_packet_header_t;

typedef struct{
    unsigned short module_id;
}module_comm_module_id_header_t;

typedef struct{
    unsigned short src_module_id;
    unsigned short dst_module_id;
    unsigned short op_code;
}module_comm_sync_packet_header_t;

typedef union{
    module_comm_module_id_header_t          id;
    module_comm_request_packet_header_t     request;
    module_comm_response_packet_header_t    response;
    module_comm_send_packet_header_t        send;
    module_comm_notify_packet_header_t      notify;
    module_comm_sync_packet_header_t        sync;
    module_comm_notify_packet_header_t      sync_notify;
}module_comm_header_t;

typedef struct
{
    signed char dal_steps; //分量计步值

    unsigned char up_down; //0: down, 1: up.
    unsigned char dir_change; //方向趋势改变计数
    unsigned char dir_change_flag; //方向改变标志
    unsigned short dir_num; //方向计数
    signed short min_value; //最小数值
    signed short max_value; //最大数值
    signed short last_min_value; //最后一次的最小数值
    signed short last_max_value; //最后一次的最大数值
    signed short old_value; //上一次值
    unsigned int cur_pos; //当前的计数点位置
    unsigned int min_pos; //最小值的计数点
    unsigned int max_pos; //最大值的计数点
    unsigned int old_pos; //上一次的计数点
    unsigned int DIR_LINE_NUM;    //方向更变的最大线条数，超过这个数表示抖动非常剧烈
    unsigned int DIR_CHANGE_N;   //方向变化的最大计数值，超过这个数表示方向改变 , 10
    int DIR_DAL_VALUE;  //方向改变的差值 , 4000
    int AVERAGE_VALUE;    //最大值与最小值之差, 500
    int MIN_VALUE; //最小绝对值
    unsigned int MAX_TIME_TWO_POINT; //连续两点之间的最大时间差值, 2秒
    unsigned char START_STEPS; //开始计步数值
}calculate_struct;

typedef struct
{
    unsigned int DIR_LINE_NUM;    //方向更变的最大线条数，超过这个数表示抖动非常剧烈
    unsigned int DIR_CHANGE_N;   //方向变化的最大计数值，超过这个数表示方向改变 , 10
    int DIR_DAL_VALUE;  //方向改变的差值 , 4000
    int AVERAGE_VALUE;    //最大值与最小值之差, 500
    int MIN_VALUE; //最小绝对值
    unsigned int MAX_TIME_TWO_POINT; //连续两点之间的最大时间差值, 2秒
    unsigned char START_STEPS; //开始计步数值
}steps_argument_struct;

typedef union{
    struct{
        unsigned char onoff;
    }set_shake_onoff;
}arithmetic_module_send_context_t;

typedef enum
{
	ARITHMETIC_TYPE_NULL = 0,
	ARITHMETIC_TYPE_WALK,
	ARITHMETIC_TYPE_RUN,
	ARITHMETIC_TYPE_MAX,
}ARITHMETIC_MODE_TYPE;

typedef struct
{
	unsigned int startSecs;
	unsigned int walkSteps;
	unsigned int distance;
	unsigned int calories;
	unsigned int duration;	//unit : s.
}sport_info_struct;

typedef union{
    struct{
        uint32_t steps;
        ARITHMETIC_MODE_TYPE type;
    }send_data;
}arithmetic_module_notify_context_t;

typedef union{
    struct{
        sport_info_struct sport_info;
    }get_sport_info;
}arithmetic_module_sync_context_t;

typedef struct
{
	unsigned char onoff; //0: close, 1: open.
	uint32_t y_flag;
	uint32_t num;
	uint64_t last_time;
}shake_struct;

typedef struct
{
	uint32_t shake_threshold_v;
	uint32_t stability_threshold_v;
	uint32_t shake_check_number;
	uint64_t shake_time_limit_ms;
}shake_recognition_alg_param_t;

/*
    arithmetic status module opcode define 
*/
typedef enum{
    /* shake */
    ARITHMETIC_SHAKE_CHECK,
    /* tilt */
    ARITHMETIC_TILT_CHECK,
    /* unidirectional */
    ARITHMETIC_UNIDIRECTIONAL_CHECK,
    /* composite direction */
    ARITHMETIC_COMPOSITE_DIRECTION_CHECK,
    ARITHMETIC_STATUS_CHECK_MAX,
}arithmetic_status_processing_t;

typedef gsensor_data_t gsensor_notify_data_ctx_t;
typedef void (*arithmetic_status_cb)(void *arg, void *latest_data);

bk_err_t gsensor_demo_init(void);
void gsensor_demo_deinit(void);
bk_err_t gsensor_demo_open();
bk_err_t gsensor_demo_close();
bk_err_t gsensor_demo_set_normal();
bk_err_t gsensor_demo_set_wakeup();
bk_err_t gsensor_demo_lowpower_wakeup();

void arithmetic_module_init(void);
void arithmetic_module_deinit(void);
int arithmetic_module_copy_data_send_msg(gsensor_notify_data_ctx_t *gsensor_data);
void arithmetic_module_register_status_callback(arithmetic_status_processing_t status, arithmetic_status_cb cb, void *arg);

void shake_arithmetic_set_parameter(shake_recognition_alg_param_t *shake_alg_p);


#endif//_ARITHMETIC_MODULE_PUBLIC_H_
