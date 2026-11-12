/*================================================================
  Copyright (c) 2021, Quectel Wireless Solutions Co., Ltd. All rights reserved.
  Quectel Wireless Solutions Proprietary and Confidential.
=================================================================*/


/*=================================================================

						EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN			  WHO		  WHAT, WHERE, WHY
------------	 -------	 -------------------------------------------------------------------------------

=================================================================*/


#ifndef QL_WAKE_H
#define QL_WAKE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef QL_WAKEPAD_DEBOUNCE_SUPPORT

#include "commontypedef.h"
#include "gpio.h"
#include "pad.h"

#include "ql_api_common.h"
#include "qosa_sys.h"
#include "slpman.h"

/*===========================================================================
 * Macro Definition
 ===========================================================================*/
//#define QL_WAKE_QUEUE_MAX   	20
//#define QL_WAKE_STACK_SIZE  	4096
//#define QL_WAKE_TASK_PRIORITY 	APP_PRIORITY_REALTIME
#define ql_get_wakeup_pin_level(pin) ((slpManGetWakeupPinValue()&0x1<<pin) ? 1 : 0)


#ifdef QL_SUPPORT_USIM_HOTPLUG
#define QL_HOTPLUG_DEBOUNCE_INTERVAL_MS   		20
#define QL_HOTPLUG_DEBOUNCE_INTERVAL_COUNT   	20
#endif




#define QL_DTR_DEBOUNCE_INTERVAL_MS   			10
#define QL_DTR_DEBOUNCE_INTERVAL_COUNT   		2

/*===========================================================================
 * function gpio Definition
 ===========================================================================*/


/*===========================================================================
 * Enum
 ===========================================================================*/

typedef struct
{
	uint8 wake_id;
	uint8 level;
	uint8 resved[2];
}QL_Wakeup_Msg;

typedef void (*ql_wakeup_callback)(QL_Wakeup_Msg *);


typedef struct
{
	ql_wakeup_callback cb;
}QL_Wakeup_Handle;

/**
 * GPIO interrupt debounce
 */
typedef enum
{
    QL_WAKEUP_0 = 0,
    QL_WAKEUP_1,
    QL_WAKEUP_2,
    QL_WAKEUP_3,
    QL_WAKEUP_4,
    QL_WAKEUP_5,
    QL_WAKEUP_6,
    QL_WAKEUP_7,
    QL_WAKEUP_8,
    QL_WAKEUP_9,
    QL_WAKEUP_10,
    QL_WAKEUP_11,
    QL_WAKEUP_12,
    QL_WAKEUP_MAX,
}QL_Wakeup_ID;


typedef struct
{
	IRQn_Type IRQn_Num;
	QL_Wakeup_ID wakeup_id;
    qosa_timer_t timerRef;
    uint16 interval_time;   //ms
    uint8 interval_count;
    uint8 last_level;  //last level
    uint8 interrupt_level;
    uint8 interrupt_happen_count;
    void  (*callBackTimerHandler)(void *);
}ql_wake_debounce_s;


#ifdef QL_SUPPORT_USIM_HOTPLUG
extern ql_wake_debounce_s hotplug_debounce;
void ql_hotplug_config_init(void);
#endif





extern ql_wake_debounce_s dtr_debounce;
void ql_dtr_config_init(void);

void ql_wakeup_set_irq(bool isEnable,UINT8 irq_num);
void ql_wakeup_pad_manage(ql_wake_debounce_s *pregister_debounce);

#endif

#ifdef __cplusplus
} /*"C" */
#endif

#endif /* QL_WAKE_H */

