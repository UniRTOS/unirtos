// Copyright 2025-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <common/bk_include.h>
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#include "sys_driver.h"
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <os/os.h>
#include <components/system.h>
#include <common/bk_kernel_err.h>
#include "aon_pmu_hal.h"
#include <components/log.h>
#include "common/bk_err.h"
#include "driver/pm_ap_core.h"
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_hal.h"
#include "gpio_driver_base.h"
#include "gpio_driver.h"
#include <os/mem.h>
/*=====================DEFINE  SECTION  START=====================*/

#define PM_AP_TAG "pm_ap"
#define LOGI(...) BK_LOGI(PM_AP_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(PM_AP_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(PM_AP_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(PM_AP_TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(PM_AP_TAG, ##__VA_ARGS__)

#define PM_AP_RTC_UNREGISTER_PERIOD_TICK   (1000)
/*=====================DEFINE  SECTION  END=====================*/
typedef enum
{
	PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE = 0,
	PM_SYSTEM_WAKEUP_MODE_DEEP_SLEEP ,
	PM_SYSTEM_WAKEUP_MODE_SUPER_DEEP_SLEEP ,
	PM_SYSTEM_WAKEUP_MODE_MAX
}pm_system_wakeup_mode_e;

typedef struct gpio_ldo_vote_node {
	gpio_id_t gpio_id;                    /* GPIO ID */
	uint32_t vote_state;                  /* Vote state bitmap (32 bits, supports 32 modules) */
	struct gpio_ldo_vote_node *next;      /* Next node */
} gpio_ldo_vote_node_t;

/*=====================VARIABLE  SECTION  START=================*/


static pm_ap_close_ap_callback_info_t s_close_ap_cb_arry[PM_AP_CLOSE_AP_MODULE_MAX];

static pm_ap_system_wakeup_cb_info_t s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_MAX][PM_AP_USING_SYS_WAKEUP_DEV_MAX];

static pm_ap_psram_power_state_callback_info_t s_psram_power_state_cb_arry[PM_POWER_PSRAM_MODULE_NAME_MAX];

static uint32_t s_pm_register_psram_callback_state = 0;
static uint32_t s_pm_handle_psram_callback_state   = 0;
static pm_rtc_wakeup_config_t s_pm_rtc_config      = {0};

static gpio_ldo_vote_node_t *s_gpio_ldo_vote_list  = NULL;
/*=====================VARIABLE  SECTION  END=================*/

/*================FUNCTION DECLARATION  SECTION  START========*/


/*================FUNCTION DECLARATION  SECTION  END========*/
bk_err_t bk_pm_ap_misc_rtc_enter_deepsleep(uint32_t time_interval , aon_rtc_isr_t callback)
{
	return BK_OK;
}

bk_err_t bk_pm_ap_misc_startup_rtc_tick_set(uint64_t time_tick)
{

	return BK_OK;
}
bk_err_t bk_pm_ap_rtc_unregsiter_wakeup(pm_sleep_mode_e sleep_mode)
{
    if(sleep_mode > PM_MODE_DEFAULT)
    {
        return BK_FAIL;
    }

    pm_ap_system_wakeup_cb_info_t rtc_wakeup_cb_info = {
		.dev_id = PM_AP_USING_SYS_WAKEUP_DEV_APP,
		.sleep_mode = sleep_mode,
		.wakeup_source = WAKEUP_SOURCE_INT_RTC,
		.sys_wakeup_fn = NULL,
		.param_p = NULL
	};
    bk_pm_ap_system_wakeup_unregister_callback(&rtc_wakeup_cb_info);

    s_pm_rtc_config.rtc_period = PM_AP_RTC_UNREGISTER_PERIOD_TICK;
    s_pm_rtc_config.rtc_cnt = 0;
    bk_pm_ap_rtc_wakeup_source_config(sleep_mode, WAKEUP_SOURCE_INT_RTC, &s_pm_rtc_config);

    return BK_OK;
}
bk_err_t bk_pm_ap_rtc_regsiter_wakeup(pm_sleep_mode_e sleep_mode,pm_ap_rtc_info_t *low_power_info)
{
    if(sleep_mode > PM_MODE_DEFAULT)
    {
        return BK_FAIL;
    }

    if(low_power_info == NULL)
    {
        return BK_FAIL;
    }

    pm_ap_system_wakeup_cb_info_t rtc_wakeup_cb_info = {
		.dev_id = PM_AP_USING_SYS_WAKEUP_DEV_APP, 
		.sleep_mode = sleep_mode,
		.wakeup_source = WAKEUP_SOURCE_INT_RTC,
		.sys_wakeup_fn = low_power_info->callback,
		.param_p = low_power_info->param_p
	};

    bk_pm_ap_system_wakeup_register_callback(&rtc_wakeup_cb_info);

    s_pm_rtc_config.rtc_period = low_power_info->period_tick;
    s_pm_rtc_config.rtc_cnt = low_power_info->period_cnt;
    bk_pm_ap_rtc_wakeup_source_config(sleep_mode, WAKEUP_SOURCE_INT_RTC, &s_pm_rtc_config);

    return BK_OK;
    
}

bk_err_t bk_pm_ap_close_ap_register_callback(pm_ap_close_ap_callback_info_t * p_close_ap_callback_info)
{
    if(p_close_ap_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_close_ap_callback_info->module >= PM_AP_CLOSE_AP_MODULE_MAX)
    {
        return BK_FAIL;
    }
    LOGD("Reg close ap_cb:0x%x,%d,0x%x,%d\r\n",p_close_ap_callback_info->close_ap_cb_fn,p_close_ap_callback_info->module,p_close_ap_callback_info->param1,p_close_ap_callback_info->param2);
	s_close_ap_cb_arry[p_close_ap_callback_info->module].close_ap_cb_fn = p_close_ap_callback_info->close_ap_cb_fn;
    s_close_ap_cb_arry[p_close_ap_callback_info->module].module= p_close_ap_callback_info->module;
    s_close_ap_cb_arry[p_close_ap_callback_info->module].param1 = p_close_ap_callback_info->param1;
    s_close_ap_cb_arry[p_close_ap_callback_info->module].param2 = p_close_ap_callback_info->param2;
    return BK_OK;
}

bk_err_t bk_pm_ap_close_ap_unregister_callback(pm_ap_close_ap_callback_info_t * p_close_ap_callback_info)
{
	if(p_close_ap_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_close_ap_callback_info->module >= PM_AP_CLOSE_AP_MODULE_MAX)
    {
        return BK_FAIL;
    }
    for(int i = 0; i < sizeof(s_close_ap_cb_arry)/sizeof(pm_ap_close_ap_callback_info_t);i++)
    {
        if(s_close_ap_cb_arry[i].module == p_close_ap_callback_info->module)
        {
            s_close_ap_cb_arry[i].close_ap_cb_fn = NULL;
            s_close_ap_cb_arry[i].module= PM_AP_CLOSE_AP_MODULE_MAX;
            s_close_ap_cb_arry[i].param1 = NULL;
            s_close_ap_cb_arry[i].param2 = 0;
        }
    }
    return BK_OK;
}

bk_err_t bk_pm_ap_close_ap_handle_callback()
{
    for(int i = 0; i < sizeof(s_close_ap_cb_arry)/sizeof(pm_ap_close_ap_callback_info_t);i++)
    {
        if(s_close_ap_cb_arry[i].close_ap_cb_fn != NULL)
        {
            s_close_ap_cb_arry[i].close_ap_cb_fn(s_close_ap_cb_arry[i].param1,s_close_ap_cb_arry[i].param2);
            //LOGD("Handle close ap cb:%d,0x%x\r\n",i,s_close_ap_cb_arry[i].close_ap_cb_fn);
        }
    }
    return BK_OK;
}

bk_err_t bk_pm_ap_system_wakeup_register_callback(pm_ap_system_wakeup_cb_info_t * p_sys_wakeup_callback_info)
{
    if(p_sys_wakeup_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_sys_wakeup_callback_info->dev_id >= PM_AP_USING_SYS_WAKEUP_DEV_MAX)
    {
        return BK_FAIL;
    }
    LOGD("Reg system_wakeup_cb:0x%x,%d,0x%x,%d\r\n",p_sys_wakeup_callback_info->sys_wakeup_fn,p_sys_wakeup_callback_info->dev_id,p_sys_wakeup_callback_info->sleep_mode,p_sys_wakeup_callback_info->wakeup_source);

    s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][p_sys_wakeup_callback_info->dev_id].sys_wakeup_fn = p_sys_wakeup_callback_info->sys_wakeup_fn;
    s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][p_sys_wakeup_callback_info->dev_id].dev_id= p_sys_wakeup_callback_info->dev_id;
    s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][p_sys_wakeup_callback_info->dev_id].sleep_mode = p_sys_wakeup_callback_info->sleep_mode;
    s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][p_sys_wakeup_callback_info->dev_id].wakeup_source = p_sys_wakeup_callback_info->wakeup_source;
    s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][p_sys_wakeup_callback_info->dev_id].param_p = p_sys_wakeup_callback_info->param_p;
    return BK_OK;
}

bk_err_t bk_pm_ap_system_wakeup_unregister_callback(pm_ap_system_wakeup_cb_info_t * p_sys_wakeup_callback_info)
{
	if(p_sys_wakeup_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_sys_wakeup_callback_info->dev_id >= PM_AP_USING_SYS_WAKEUP_DEV_MAX)
    {
        return BK_FAIL;
    }
    for(int i = 0; i < PM_AP_USING_SYS_WAKEUP_DEV_MAX;i++)
    {
        if(s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].dev_id == p_sys_wakeup_callback_info->dev_id)
        {
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].sys_wakeup_fn = NULL;
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].dev_id = PM_AP_USING_SYS_WAKEUP_DEV_MAX;
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].sleep_mode = PM_MODE_DEFAULT;
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].param_p = NULL;
        }
    }
    return BK_OK;
}


bk_err_t bk_pm_ap_system_wakeup_handle_callback(pm_ap_core_msg_t *msg)
{
    bk_err_t ret = BK_OK;
    if(msg == NULL)
    {
        ret = BK_FAIL;
        goto exit;
    }
    if(msg->param1 == 0)
    {
        ret = BK_ERR_PARAM;
        goto exit;
    }
    uint32_t wakeup_cb_index = 0;
    switch(msg->param1)
    {
        case PM_MODE_LOW_VOLTAGE:
            wakeup_cb_index = PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE;
            break;
        case PM_MODE_DEEP_SLEEP:
            wakeup_cb_index = PM_SYSTEM_WAKEUP_MODE_DEEP_SLEEP;
            break;
        case PM_MODE_SUPER_DEEP_SLEEP:
            wakeup_cb_index = PM_SYSTEM_WAKEUP_MODE_SUPER_DEEP_SLEEP;
            break;
        default:
        break;
    }
    for(int i = 0; i < PM_AP_USING_SYS_WAKEUP_DEV_MAX;i++)
    {
        if(s_system_wakeup_cb_arry[wakeup_cb_index][i].wakeup_source == msg->param2)
        {
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].sys_wakeup_fn(msg->param1,msg->param2,s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].param_p);
        }
    }
exit:
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LV_WAKEUP,0x1,0x0);
    return ret;
}

bk_err_t bk_pm_ap_psram_power_state_register_callback(pm_ap_psram_power_state_callback_info_t * p_psram_power_state_callback_info)
{
    if(p_psram_power_state_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_psram_power_state_callback_info->dev_id >= PM_POWER_PSRAM_MODULE_NAME_MAX)
    {
        return BK_FAIL;
    }
    //LOGD("Reg close ap_cb:0x%x,%d,0x%x,%d\r\n",p_psram_power_state_callback_info->psram_on_cb_fn,p_psram_power_state_callback_info->dev_id,p_psram_power_state_callback_info->param1,p_psram_power_state_callback_info->param2);
	s_psram_power_state_cb_arry[p_psram_power_state_callback_info->dev_id].psram_on_cb_fn = p_psram_power_state_callback_info->psram_on_cb_fn;
    s_psram_power_state_cb_arry[p_psram_power_state_callback_info->dev_id].psram_off_cb_fn = p_psram_power_state_callback_info->psram_off_cb_fn;
    s_psram_power_state_cb_arry[p_psram_power_state_callback_info->dev_id].dev_id= p_psram_power_state_callback_info->dev_id;
    s_psram_power_state_cb_arry[p_psram_power_state_callback_info->dev_id].param1 = p_psram_power_state_callback_info->param1;
    s_psram_power_state_cb_arry[p_psram_power_state_callback_info->dev_id].param2 = p_psram_power_state_callback_info->param2;
    s_pm_register_psram_callback_state |= 0x1 << p_psram_power_state_callback_info->dev_id;
    s_pm_handle_psram_callback_state   |= 0x1 << p_psram_power_state_callback_info->dev_id;
    return BK_OK;
}

bk_err_t bk_pm_ap_psram_power_state_unregister_callback(pm_ap_psram_power_state_callback_info_t * p_psram_power_state_callback_info)
{
	if(p_psram_power_state_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_psram_power_state_callback_info->dev_id >= PM_POWER_PSRAM_MODULE_NAME_MAX)
    {
        return BK_FAIL;
    }
    for(int i = 0; i < sizeof(s_psram_power_state_cb_arry)/sizeof(pm_ap_psram_power_state_callback_info_t);i++)
    {
        if(s_psram_power_state_cb_arry[i].dev_id == p_psram_power_state_callback_info->dev_id)
        {
            s_psram_power_state_cb_arry[i].psram_on_cb_fn = NULL;
            s_psram_power_state_cb_arry[i].psram_off_cb_fn = NULL;
            s_psram_power_state_cb_arry[i].dev_id= PM_AP_USING_PSRAM_POWER_STATE_DEV_MAX;
            s_psram_power_state_cb_arry[i].param1 = 0;
            s_psram_power_state_cb_arry[i].param2 = 0;
            s_pm_register_psram_callback_state &= ~(0x1 << p_psram_power_state_callback_info->dev_id);
        }
    }
    return BK_OK;
}

bk_err_t bk_pm_ap_psram_power_state_handle_callback(pm_power_psram_module_name_e dev_id,pm_ap_psram_power_state_e psram_power_state)
{
    if(psram_power_state == PM_AP_PSRAM_POWER_ON)
    {
        for(int i = 0; i < sizeof(s_psram_power_state_cb_arry)/sizeof(pm_ap_psram_power_state_callback_info_t);i++)
        {
            if(s_pm_handle_psram_callback_state & (0x1 << i))
            {
                if(s_psram_power_state_cb_arry[i].psram_on_cb_fn != NULL)
                {
                    s_psram_power_state_cb_arry[i].psram_on_cb_fn(0,0);
                    /*Clear the handled callback state:It have handled callback, it cannot process again*/
                    s_pm_handle_psram_callback_state &= ~(0x1 << i);
                }
            }
        }
    }
    else if(psram_power_state == PM_AP_PSRAM_POWER_OFF)
    {
        for(int i = 0; i < sizeof(s_psram_power_state_cb_arry)/sizeof(pm_ap_psram_power_state_callback_info_t);i++)
        {
            if(s_psram_power_state_cb_arry[i].psram_off_cb_fn != NULL)
            {
                s_psram_power_state_cb_arry[i].psram_off_cb_fn(0,0);
                s_pm_handle_psram_callback_state   |= 0x1 << i;
            }
        }
    }
    else
    {
        ;
    }

    return BK_OK;
}
static gpio_ldo_vote_node_t* gpio_ldo_find_or_create_node(gpio_id_t gpio_id, bool create)
{
	gpio_ldo_vote_node_t *current = s_gpio_ldo_vote_list;
	gpio_ldo_vote_node_t *prev = NULL;

	/* Traverse the linked list to check if GPIO node already exists */
	while (current != NULL) {
		if (current->gpio_id == gpio_id) {
			/* Found matching GPIO */
			return current;
		}
		prev = current;
		current = current->next;
	}

	/* Not found, allocate new node if creation is requested */
	if (create) {
		gpio_ldo_vote_node_t *new_node = (gpio_ldo_vote_node_t *)os_malloc(sizeof(gpio_ldo_vote_node_t));
		if (new_node == NULL) {
			LOGE("Failed to allocate GPIO LDO vote node for GPIO %d\r\n", gpio_id);
			return NULL;
		}

		/* Initialize new node */
		new_node->gpio_id = gpio_id;
		new_node->vote_state = 0;
		new_node->next = NULL;

		/* Add to the tail of the list */
		if (prev == NULL) {
			/* List is empty, set as head node */
			s_gpio_ldo_vote_list = new_node;
		} else {
			/* Add to the end of the list */
			prev->next = new_node;
		}

		LOGV("GPIO %d LDO vote node created (%d bytes)\r\n", gpio_id, sizeof(gpio_ldo_vote_node_t));
		return new_node;
	}

	/* Not creating and not found */
	return NULL;
}

static bk_err_t gpio_ldo_remove_node(gpio_id_t gpio_id)
{
	gpio_ldo_vote_node_t *current = s_gpio_ldo_vote_list;
	gpio_ldo_vote_node_t *prev = NULL;

	/* Traverse the linked list to find the node */
	while (current != NULL) {
		if (current->gpio_id == gpio_id) {
			/* Found the node, remove it from the list */
			if (prev == NULL) {
				/* It's the head node */
				s_gpio_ldo_vote_list = current->next;
			} else {
				/* It's a middle or tail node */
				prev->next = current->next;
			}

			/* Free node memory */
			os_free(current);
			LOGV("GPIO %d LDO vote node removed\r\n", gpio_id);
			return BK_OK;
		}
		prev = current;
		current = current->next;
	}

	return BK_ERR_GPIO_CHAN_ID;
}

static bk_err_t gpio_ldo_configure_output(gpio_id_t gpio_id, bool output_level)
{
	bk_err_t ret = BK_OK;

	if (gpio_id >= GPIO_NUM_MAX || gpio_id < 0) {
		LOGE("Invalid gpio_id: %d\r\n", gpio_id);
		return BK_ERR_GPIO_CHAN_ID;
	}
	/* GPIO function unmap and initialization */
	ret |= gpio_dev_unmap(gpio_id);

	/* Configure GPIO as output mode */
	ret |= bk_gpio_set_capacity(gpio_id, 0);
	ret |= bk_gpio_disable_input(gpio_id);
	ret |= bk_gpio_enable_output(gpio_id);

	/* Set output level */
	if (output_level) {
		ret |= bk_gpio_set_output_high(gpio_id);
	} else {
		ret |= bk_gpio_set_output_low(gpio_id);
	}

	return ret;
}

static bk_err_t bk_gpio_get_ldo_vote_state(gpio_id_t gpio_id, uint32_t *vote_state)
{
	gpio_ldo_vote_node_t *node = NULL;

	if (gpio_id >= GPIO_NUM_MAX || gpio_id < 0) {
		LOGE("Invalid gpio_id: %d\r\n", gpio_id);
		return BK_ERR_GPIO_CHAN_ID;
	}

	if (vote_state == NULL) {
		LOGE("vote_state pointer is NULL\r\n");
		return BK_ERR_GPIO_INVALID_MODE;
	}

	/* Find the vote node for this GPIO */
	node = gpio_ldo_find_or_create_node(gpio_id, false);
	if (node == NULL) {
		/* Node doesn't exist, meaning never used, vote state is 0 */
		*vote_state = 0;
	} else {
		*vote_state = node->vote_state;
	}

	return BK_OK;
}
bk_err_t bk_gpio_ctrl_external_ldo(uint32_t module, gpio_id_t gpio_id, gpio_output_state_e value)
{
	bk_err_t ret = BK_OK;
	uint32_t module_mask       = 0;
	uint32_t old_vote_state    = 0;
	gpio_ldo_vote_node_t *node = NULL;

	if (gpio_id >= GPIO_NUM_MAX || gpio_id < 0) {
		LOGE("Invalid gpio_id: %d\r\n", gpio_id);
		return BK_ERR_GPIO_CHAN_ID;
	}

	if (module >= 32) {
		LOGE("Invalid module: %d (valid range: 0~31)\r\n", module);
		return BK_ERR_GPIO_INVALID_MODE;
	}

	if (value != GPIO_OUTPUT_STATE_HIGH && value != GPIO_OUTPUT_STATE_LOW) {
		LOGE("Invalid output state: %d\r\n", value);
		return BK_ERR_GPIO_INVALID_MODE;
	}

	module_mask = (0x1 << module);

	/* Handle vote enable (output high level) */
	if (value == GPIO_OUTPUT_STATE_HIGH) {
		/* Find or create the vote node for this GPIO */
		node = gpio_ldo_find_or_create_node(gpio_id, true);
		if (node == NULL) {
			LOGE("Failed to create vote node for GPIO %d\r\n", gpio_id);
			return BK_ERR_NO_MEM;
		}

		old_vote_state = node->vote_state;

		/* Set the vote bit for this module */
		node->vote_state |= module_mask;

		/* If this is the first module voting, initialize GPIO and output high level */
		if (old_vote_state == 0) {
			ret = gpio_ldo_configure_output(gpio_id, true);
			if (ret != BK_OK) {
				LOGE("Failed to configure GPIO %d output HIGH (module %d)\r\n", gpio_id, module);
				/* Rollback vote state on failure */
				node->vote_state = old_vote_state;
				/* If it's a newly created node and configuration failed, delete the node */
				gpio_ldo_remove_node(gpio_id);

			} else {
				LOGV("GPIO %d output HIGH (module %d vote, state=0x%x)\r\n",gpio_id, module, node->vote_state);
			}
		} else {
			LOGV("GPIO %d already HIGH, module %d vote added (state=0x%x)\r\n",gpio_id, module, node->vote_state);
		}
	}
	/* Handle vote release (output low level) */
	else {
		/* Find the vote node for this GPIO (don't create) */
		node = gpio_ldo_find_or_create_node(gpio_id, true);
		if (node == NULL) {
			/* Node doesn't exist, meaning never voted, return success directly */
			LOGV("GPIO %d vote node not exist, already released\r\n", gpio_id);
			return BK_OK;
		}

		old_vote_state = node->vote_state;

		/* Clear the vote bit for this module */
		node->vote_state &= ~module_mask;

		/* Only output low level when all modules have released their votes */
		if (node->vote_state == 0) {
			ret = gpio_ldo_configure_output(gpio_id, false);
			if (ret != BK_OK) {
				LOGE("Failed to configure GPIO %d output LOW (module %d)\r\n", gpio_id, module);
				/* Rollback vote state on failure */
				node->vote_state = old_vote_state;
			} else {
				LOGV("GPIO %d output LOW (all modules released)\r\n", gpio_id);
				/* All modules released, delete the node to free memory */
				gpio_ldo_remove_node(gpio_id);
			}
		} else {
			LOGV("GPIO %d keep HIGH, module %d vote removed (remaining state=0x%x)\r\n",gpio_id, module, node->vote_state);
		}
	}

	return ret;
}