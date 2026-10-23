/** @file         qurl_network.c
 *  @brief        Brief description: Network-related interfaces
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2024-02-21 13:45:21
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */
#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#include "qurl_network.h"
#include "qurl_log.h"
#include "qurl_platform_port.h"

struct qurl_port_nw_thread_data_s
{
    int                 nw_id;
    qurl_nw_activate_cb nw_activate_func;
    void               *cb_arg;
};
typedef struct qurl_port_nw_thread_data_s qurl_port_nw_thread_data_t;

/*Declaration*/
extern qurl_core_sec_t g_core_sec[QURL_CFG_CORE_SERVER_MAX];
int                    qurl_defaule_port_nw_activate(int nw_id);

/*Internal*/
static void qurl_nw_activate_thread_cb(void *arg)
{
    int                         ret = QURL_OK;
    qurl_port_nw_thread_data_t *data_ptr = (qurl_port_nw_thread_data_t *)arg;

    ret = qurl_port_nw_activate(data_ptr->nw_id);
    if (ret != QURL_OK)
    {
        qurl_loge("ret:%x", ret);
    }

    data_ptr->nw_activate_func(ret, data_ptr->cb_arg);

    qurl_free(data_ptr);

    qurl_thread_delete(QOSA_NULL);
}

/** Generic callback, with parameters specified as qurl_core_ctrl_t **/
static void nw_activate_cb(int nw_ret, void *arg)
{
    qurl_event_t      event = {0};
    qurl_event_t     *arg_ptr = (qurl_event_t *)arg;
    int               sec_index = (int)arg_ptr->id;
    qurl_core_ctrl_t *core_ptr = (qurl_core_ctrl_t *)arg_ptr->param1;

    qurl_free(arg_ptr);
    arg_ptr = QOSA_NULL;

    qurl_logd("enter\r\n");

    if (core_ptr != QOSA_NULL)                                      //core host
    {
        if (sec_index < 0 || sec_index >= QURL_CFG_CORE_SERVER_MAX) /*Parameter Validity Check*/
        {
            qurl_loge("delete [0x%x] failed\r\n", core_ptr);
            return;
        }
        qurl_mutex_lock(g_core_sec[sec_index].lock, QURL_WAIT_FOREVER);
        if (g_core_sec[sec_index].owner != core_ptr) /*Core Legitimacy Check*/
        {
            qurl_mutex_unlock(g_core_sec[sec_index].lock);
            return;
        }

        if (core_ptr->state == QURL_STATE_NETWORK) /*Can be unblocked*/
        {
            event.id = QURL_EVENT_CODE_NETWORK;
            event.param1 = nw_ret;
            qurl_event_send(core_ptr->ipc_event, &event, QURL_WAIT_FOREVER);
        }
        qurl_mutex_unlock(g_core_sec[sec_index].lock);
    }
}

/*Southbound Platform Default Functions*/
int qurl_defaule_port_nw_activate(int nw_id)
{
    (void)nw_id;

    return QURL_OK;
}

int qurl_defaule_port_nw_is_activated(int nw_id)
{
    (void)nw_id;

    return 1;
}

int qurl_defaule_port_nw_get_ip(int nw_id, char *ipv4_buf, int ipv4_buf_len, char *ipv6_buf, int ipv6_buf_len)
{
    (void)nw_id;
    qurl_memset(ipv4_buf, 0x00, ipv4_buf_len);
    qurl_memset(ipv6_buf, 0x00, ipv6_buf_len);

    return QURL_OK;
}

/*Northbound*/

int qurl_nw_activate_async_ex(int nw_id, qurl_nw_activate_cb nw_activate_func, void *cb_arg)
{
    qurl_thread_t               thread_ptr = QOSA_NULL;
    qurl_port_nw_thread_data_t *data_ptr = {0};

    data_ptr = (qurl_port_nw_thread_data_t *)qurl_malloc(sizeof(qurl_port_nw_thread_data_t));
    if (data_ptr == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }
    data_ptr->nw_id = nw_id;
    data_ptr->nw_activate_func = nw_activate_func;
    data_ptr->cb_arg = cb_arg;

    qurl_thread_create(&thread_ptr, qurl_nw_activate_thread_cb, QURL_NETWORK_TASK_PRIORITY, QURL_NETWORK_TASK_STACK_SIZE, (void *)data_ptr);

    return QURL_OK;
}

int qurl_nw_activate_async(qurl_core_ctrl_t *core_ptr)
{
    int           ret = 0;
    qurl_event_t *arg_ptr = qurl_malloc(sizeof(qurl_event_t));
    if (arg_ptr == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }
    arg_ptr->id = core_ptr->sec_index;
    arg_ptr->param1 = (unsigned long)core_ptr;
    arg_ptr->param2 = (unsigned long)0;
    arg_ptr->param3 = (unsigned long)0;
    ret = qurl_nw_activate_async_ex(core_ptr->usr_cfg.nw_id, nw_activate_cb, arg_ptr);
    if (ret != QURL_OK)
    {
        qurl_free(arg_ptr);
    }

    return ret;
}

int qurl_nw_is_activated(qurl_core_ctrl_t *core_ptr)
{
    if (core_ptr->usr_cfg.nw_id < 0)
    {
        return 1; /*< 0 default not specified*/
    }
    return qurl_port_nw_is_activated(core_ptr->usr_cfg.nw_id);
}

int qurl_nw_get_ip(qurl_core_ctrl_t *core_ptr)
{
    int ret = 0;

    if (core_ptr->usr_cfg.nw_id < 0)
    {
        return 1; /*< 0 default not specified*/
    }
    ret = qurl_port_nw_get_ip(core_ptr->usr_cfg.nw_id, core_ptr->cfg.nw_ipv4_str, QURL_INET4_ADDRSTRLEN, core_ptr->cfg.nw_ipv6_str, QURL_INET6_ADDRSTRLEN);

    return ret;
}
#endif