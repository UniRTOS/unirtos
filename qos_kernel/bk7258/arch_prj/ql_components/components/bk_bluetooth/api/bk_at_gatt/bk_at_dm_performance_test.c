#include <common/sys_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"
#include "components/bluetooth/bk_dm_gap_ble.h"

#include "dm_gattc.h"
#include "dm_gatts.h"
#include "dm_gatt_connection.h"
#include "dm_gap_utils.h"

#include "bk_at_dm_performance_test.h"

#define MIN_VALUE(x, y) (((x) < (y)) ? (x): (y))

enum
{
    PROFILE_DEBUG_LEVEL_ERROR,
    PROFILE_DEBUG_LEVEL_WARNING,
    PROFILE_DEBUG_LEVEL_INFO,
    PROFILE_DEBUG_LEVEL_DEBUG,
    PROFILE_DEBUG_LEVEL_VERBOSE,
};

#ifdef LOGE
    #undef LOGE
#endif

#ifdef LOGW
    #undef LOGW
#endif

#ifdef LOGI
    #undef LOGI
#endif

#ifdef LOGD
    #undef LOGD
#endif

#ifdef LOGV
    #undef LOGV
#endif

#ifdef LOG_TAG
    #undef LOG_TAG
#endif

#define PROFILE_DEBUG_LEVEL PROFILE_DEBUG_LEVEL_INFO

#define LOG_TAG "ble_ptest"
#define LOGE(format, ...) do{if(PROFILE_DEBUG_LEVEL_INFO >= PROFILE_DEBUG_LEVEL_ERROR)   BK_LOGE(LOG_TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(PROFILE_DEBUG_LEVEL_INFO >= PROFILE_DEBUG_LEVEL_WARNING) BK_LOGW(LOG_TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(PROFILE_DEBUG_LEVEL_INFO >= PROFILE_DEBUG_LEVEL_INFO)    BK_LOGI(LOG_TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(PROFILE_DEBUG_LEVEL_INFO >= PROFILE_DEBUG_LEVEL_DEBUG)   BK_LOGI(LOG_TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(PROFILE_DEBUG_LEVEL_INFO >= PROFILE_DEBUG_LEVEL_VERBOSE) BK_LOGI(LOG_TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

#define GATT_ATTR_COUNT (sizeof(s_gatts_performance_service_default) / sizeof(s_gatts_performance_service_default[0]))


typedef struct
{
    uint8_t status; //0 idle 1 connected
    beken_semaphore_t server_sem;
    uint16_t send_notify_read_rsp_status;
} profile_app_env_t;

static uint16_t s_char_notify_desc_buff = 0;
static uint16_t s_send_data_len = 23;
static uint16_t s_send_interval = 1000;

static const bk_gatts_attr_db_t s_gatts_performance_service_default[] =
{
    //service
    {
        BK_GATT_PRIMARY_SERVICE_DECL(0),
    },

    //char 1
    {
        BK_GATT_CHAR_DECL(0,
                          0, NULL,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_RSP_BY_APP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(s_char_notify_desc_buff), (uint8_t *)&s_char_notify_desc_buff,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_RSP_BY_APP),
    },

    //char 2 data len
    {
        BK_GATT_CHAR_DECL(0x9abc,
                          sizeof(s_send_data_len), (uint8_t *)&s_send_data_len,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_RSP_BY_APP),
    },

    //char 3 send interval
    {
        BK_GATT_CHAR_DECL(0xf0de,
                          sizeof(s_send_interval), (uint8_t *)&s_send_interval,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_RSP_BY_APP),
    },

    //char 4 write test
    {
        BK_GATT_CHAR_DECL(0xf012,
                          0, NULL,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_RSP_BY_APP),
    },
};

enum
{
    GATT_DB_BK_PERFORMANCE_IDX_SVC,
    GATT_DB_BK_PERFORMANCE_IDX_CHAR1,
    GATT_DB_BK_PERFORMANCE_IDX_CHAR1_DESC,
    GATT_DB_BK_PERFORMANCE_IDX_CHAR2,
    GATT_DB_BK_PERFORMANCE_IDX_CHAR3,
    GATT_DB_BK_PERFORMANCE_IDX_CHAR4,
    GATT_DB_BK_PERFORMANCE_IDX_NB,
};

enum
{
    STATISTICS_TYPE_TX = (1 << 0),
    STATISTICS_TYPE_RX = (1 << 1),
};

static uint16_t s_attr_handle_list[GATT_ATTR_COUNT];

static bk_gatts_attr_db_t *s_gatts_performance_service_p;

static uint8_t s_profile_is_init;
static uint8_t s_db_init;
static int32_t (*s_char_recv_cb)(uint16_t gatt_conn_handle, uint8_t *data, uint32_t len);
static beken_timer_t s_performance_statistics_timer;
static beken_timer_t s_performance_send_test_timer;
static uint8_t s_performance_statistics_type;

static uint32_t s_performance_tx_bytes = 0;
static uint32_t s_performance_rx_bytes = 0;
static uint8_t s_performance_tx_method; // 0 when time out, 1 when notify tx completed

struct tmp_timer_param_t
{
    uint8_t *found;
    uint8_t *data;
    uint32_t data_len;
};

static void ble_tx_test_timer_callback(void *param)
{
    int32_t retval = 0;
    uint8_t *tmp_buff = NULL;
    uint8_t tmp_found = 0;
    int16_t conn_id = (typeof(conn_id))(size_t)param;

    static uint8_t send_data = 0;

    tmp_buff = os_malloc(s_send_data_len);

    if (!tmp_buff)
    {
        LOGE("%s alloc send failed\n", __func__);
        return;
    }

    os_memset(tmp_buff, 0, s_send_data_len);
    tmp_buff[0] = tmp_buff[s_send_data_len - 1] = send_data++;

    if (conn_id >= 0)
    {
        retval = bk_ble_gatts_send_indicate(bk_at_dm_gatts_get_current_if(), conn_id,
                                            s_attr_handle_list[GATT_DB_BK_PERFORMANCE_IDX_CHAR1],
                                            s_send_data_len,
                                            tmp_buff,
                                            0);
    }
    else
    {
        struct tmp_timer_param_t tmp_param = {0};

        tmp_param.data = tmp_buff;
        tmp_param.data_len = s_send_data_len;
        tmp_param.found = &tmp_found;

        int32_t nest_notify_all (dm_gatt_app_env_t *env, void *arg)
        {
            int32_t retval = 0;
            uint8_t *found = (typeof(found))arg;
            struct tmp_timer_param_t *tmp_param = (typeof(tmp_param))arg;

            if (env && env->status == GAP_CONNECT_STATUS_CONNECTED && tmp_param && !*(tmp_param->found)) // !tmp_found
            {
                *(tmp_param->found) = 1;
                //tmp_found = 1; //can't access tmp_found in timer callback, maybe timer task stack is too small !!!

                retval = bk_ble_gatts_send_indicate(bk_at_dm_gatts_get_current_if(), env->conn_id,
                                                    s_attr_handle_list[GATT_DB_BK_PERFORMANCE_IDX_CHAR1],
                                                    tmp_param->data_len,
                                                    tmp_param->data, 0);

                if (retval)
                {
                    LOGE("send indicate err %d conn_id %d", retval, env->conn_id);
                }
            }

            return 0;
        }

        bk_at_dm_ble_app_env_foreach(nest_notify_all, (void *)&tmp_param);
    }

    os_free(tmp_buff);

    if (retval != 0)
    {
        LOGE("notify err %d", retval);
    }
    else
    {
        s_performance_tx_bytes += s_send_data_len;
    }
}

static int32_t profile_gatts_cb(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *comm_param)
{
    ble_err_t ret = 0;
    dm_gatt_app_env_t *common_env_tmp = NULL;
    profile_app_env_t *app_env_tmp = NULL;

    switch (event)
    {
    case BK_GATTS_CONNECT_EVT:
    {
        struct gatts_connect_evt_param *param = (typeof(param))comm_param;

        LOGI("BK_GATTS_CONNECT_EVT %d role %d %02X:%02X:%02X:%02X:%02X:%02X", param->conn_id, param->link_role,
             param->remote_bda[5],
             param->remote_bda[4],
             param->remote_bda[3],
             param->remote_bda[2],
             param->remote_bda[1],
             param->remote_bda[0]);

        common_env_tmp = bk_at_dm_ble_alloc_profile_data_by_addr(s_gatts_performance_service_p[0].att_desc.attr_content.uuid.uuid16, param->remote_bda, sizeof(*app_env_tmp), (uint8_t **)&app_env_tmp);

        if (!common_env_tmp)
        {
            LOGE("alloc profile data err !!!!");
            break;
        }

        app_env_tmp->status = 1;
    }
    break;

    case BK_GATTS_DISCONNECT_EVT:
    {
        struct gatts_disconnect_evt_param *param = (typeof(param))comm_param;

        LOGI("BK_GATTS_DISCONNECT_EVT %02X:%02X:%02X:%02X:%02X:%02X",
             param->remote_bda[5],
             param->remote_bda[4],
             param->remote_bda[3],
             param->remote_bda[2],
             param->remote_bda[1],
             param->remote_bda[0]);

        common_env_tmp = bk_at_dm_ble_find_app_env_by_addr(param->remote_bda);

        if (!common_env_tmp)
        {
            LOGE("cant find app env");
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))bk_at_dm_ble_find_profile_data_by_profile_id(common_env_tmp, s_gatts_performance_service_p[0].att_desc.attr_content.uuid.uuid16);

        if (app_env_tmp)
        {
            if (app_env_tmp->server_sem)
            {
                rtos_deinit_semaphore(&app_env_tmp->server_sem);
                app_env_tmp->server_sem = NULL;
            }
        }
    }
    break;

    case BK_GATTS_CONF_EVT:
    {
        struct gatts_conf_evt_param *param = (typeof(param))comm_param;

        LOGV("BK_GATTS_CONF_EVT %d %d %d", param->status, param->conn_id, param->handle);

        common_env_tmp = bk_at_dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp)
        {
            LOGE("cant find app env");
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))bk_at_dm_ble_find_profile_data_by_profile_id(common_env_tmp, s_gatts_performance_service_p[0].att_desc.attr_content.uuid.uuid16);

        if (app_env_tmp)
        {
            app_env_tmp->send_notify_read_rsp_status = param->status;

            if (app_env_tmp->server_sem)
            {
                rtos_set_semaphore(&app_env_tmp->server_sem);
            }
        }

        if (param->handle == s_attr_handle_list[GATT_DB_BK_PERFORMANCE_IDX_CHAR1]
                && s_performance_tx_method == 1
                && s_char_notify_desc_buff)
        {
            ble_tx_test_timer_callback((void *)(size_t)param->conn_id);
        }
    }
    break;

    case BK_GATTS_RESPONSE_EVT:
    {
        struct gatts_rsp_evt_param *param = (typeof(param))comm_param;
        LOGI("BK_GATTS_RESPONSE_EVT 0x%x %d", param->status, param->handle);

        common_env_tmp = bk_at_dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp)
        {
            LOGE("cant find app env");
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))bk_at_dm_ble_find_profile_data_by_profile_id(common_env_tmp, s_gatts_performance_service_p[0].att_desc.attr_content.uuid.uuid16);

        if (app_env_tmp)
        {
            app_env_tmp->send_notify_read_rsp_status = param->status;

            if (app_env_tmp->server_sem)
            {
                rtos_set_semaphore(&app_env_tmp->server_sem);
            }
        }
    }
    break;

    case BK_GATTS_READ_EVT:
    {
        struct gatts_read_evt_param *param = (typeof(param))comm_param;
        bk_gatt_rsp_t rsp;
        uint16_t final_len = 0;

        os_memset(&rsp, 0, sizeof(rsp));
        LOGV("read attr handle %d need rsp %d", param->handle, param->need_rsp);

        uint8_t *tmp_buff = NULL;
        uint16_t buff_size = 0;

        uint8_t valid = 0;
        uint32_t i = 0;

        if (bk_ble_gatts_get_attr_value(param->handle, &buff_size, &tmp_buff))
        {
            LOGE("can't get attr %d buff !!!", param->handle);
            valid = 0;
        }

        for (i = GATT_DB_BK_PERFORMANCE_IDX_SVC + 1; i < GATT_DB_BK_PERFORMANCE_IDX_NB; ++i)
        {
            if (s_attr_handle_list[i] == param->handle)
            {
                break;
            }
        }

        if (i >= GATT_DB_BK_PERFORMANCE_IDX_NB)
        {
            LOGE("attr hande %d app invalid !!!", param->handle);
            valid = 0;
        }
        else
        {
            LOGV("index %d attr hande %d size %d buff %p", i, param->handle, buff_size, tmp_buff);
        }

        if (param->need_rsp)
        {
            final_len = buff_size - param->offset;

            rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
            rsp.attr_value.handle = param->handle;
            rsp.attr_value.offset = param->offset;

            if (tmp_buff && valid)
            {
                rsp.attr_value.len = final_len;
                rsp.attr_value.value = tmp_buff + param->offset;
            }
            else
            {
                rsp.attr_value.len = 0;
                rsp.attr_value.value = NULL;
            }

            ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, (tmp_buff && valid ? BK_GATT_OK : BK_GATT_INSUF_RESOURCE), &rsp);
        }
    }
    break;

    case BK_GATTS_WRITE_EVT:
    {
        struct gatts_write_evt_param *param = (typeof(param))comm_param;
        bk_gatt_rsp_t rsp;
        uint16_t final_len = 0;

        os_memset(&rsp, 0, sizeof(rsp));

        LOGV("write attr handle %d need rsp %d", param->handle, param->need_rsp);

        uint8_t *tmp_buff = NULL;
        uint16_t buff_size = 0;

        uint8_t valid = 0;
        uint32_t i = 0;

        if (bk_ble_gatts_get_attr_value(param->handle, &buff_size, &tmp_buff))
        {
            LOGE("can't get attr %d buff !!!", param->handle);
            valid = 0;
        }

        for (i = GATT_DB_BK_PERFORMANCE_IDX_SVC + 1; i < GATT_DB_BK_PERFORMANCE_IDX_NB; ++i)
        {
            if (s_attr_handle_list[i] == param->handle)
            {
                break;
            }
        }

        if (i >= GATT_DB_BK_PERFORMANCE_IDX_NB)
        {
            LOGE("attr hande %d app invalid !!!", param->handle);
            valid = 0;
        }
        else
        {
            LOGV("index %d attr hande %d size %d buff %p", i, param->handle, buff_size, tmp_buff);

            if (param->handle == s_attr_handle_list[GATT_DB_BK_PERFORMANCE_IDX_CHAR1_DESC])
            {
                valid = 1;

                os_memcpy(&s_char_notify_desc_buff, param->value, sizeof(s_char_notify_desc_buff));

                bk_performance_test_profile_enable_tx(s_char_notify_desc_buff);
            }
        }

        if (param->need_rsp)
        {
            final_len = MIN_VALUE(param->len, buff_size - param->offset);

            if (tmp_buff)
            {
                os_memcpy(tmp_buff + param->offset, param->value, final_len);
            }

            rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
            rsp.attr_value.handle = param->handle;
            rsp.attr_value.offset = param->offset;

            if (tmp_buff && valid)
            {
                rsp.attr_value.len = final_len;
                rsp.attr_value.value = tmp_buff + param->offset;
            }

            ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, valid ? BK_GATT_OK : BK_GATT_INSUF_RESOURCE, &rsp);
        }
    }
    break;

    case BK_GATTS_EXEC_WRITE_EVT:
    {
        struct gatts_exec_write_evt_param *param = (typeof(param))comm_param;
        LOGI("exec write");
    }
    break;

    default:
        break;
    }

    return ret;
}

static int32_t bk_gattc_cb (bk_gattc_cb_event_t event, bk_gatt_if_t gattc_if, bk_ble_gattc_cb_param_t *comm_param)
{
    ble_err_t ret = 0;
    //dm_gattc_app_env_t *app_env_tmp = NULL;
    //dm_gatt_app_env_t *common_env_tmp = NULL;

    switch (event)
    {
    case BK_GATTC_NOTIFY_EVT:
    {
        struct gattc_notify_evt_param *param = (typeof(param))comm_param;

        LOGV("BK_GATTC_NOTIFY_EVT %d %d handle %d vallen %d %02X:%02X:%02X:%02X:%02X:%02X", param->conn_id,
             param->is_notify,
             param->handle,
             param->value_len,
             param->remote_bda[5],
             param->remote_bda[4],
             param->remote_bda[3],
             param->remote_bda[2],
             param->remote_bda[1],
             param->remote_bda[0]);

        //        common_env_tmp = bk_at_dm_ble_find_app_env_by_conn_id(param->conn_id);
        //
        //        if (!common_env_tmp || !common_env_tmp->data)
        //        {
        //            LOGE("conn_id %d not found %d %p", param->conn_id, common_env_tmp->data);
        //            break;
        //        }

        s_performance_rx_bytes += param->value_len;
    }
    break;

    default:
        break;
    }

    return ret;
}

static int32_t profile_reg_db(void)
{
    int32_t ret = bk_at_dm_gatts_reg_db((bk_gatts_attr_db_t *)s_gatts_performance_service_p,
                                        GATT_ATTR_COUNT,
                                        s_attr_handle_list,
                                        profile_gatts_cb,
                                        s_db_init ? 0 : 1);

    if (ret)
    {
        LOGE("reg db err");
        return ret;
    }

    s_db_init = 1;

    return ret;
}


int32_t bk_performance_test_profile_init(
    uint16_t service_uuid,
    uint16_t char_uuid,
    uint8_t tx_method,
    int32_t (*recv_cb)(uint16_t gatt_conn_handle, uint8_t *data, uint32_t len))
{
#if BK_PERFORMANCE_PROFILE_ENABLE

    if (!bk_at_dm_gatts_is_init())
    {
        LOGE("gatts is not init");
        return -1;
    }

    if (s_profile_is_init)
    {
        LOGE("already init");
        return -1;
    }

    if (s_gatts_performance_service_p)
    {
        os_free(s_gatts_performance_service_p);
        s_gatts_performance_service_p = NULL;
    }

    s_gatts_performance_service_p = os_malloc(sizeof(s_gatts_performance_service_default));

    if (!s_gatts_performance_service_p)
    {
        LOGE("alloc db err");
        return -1;
    }

    os_memcpy(s_gatts_performance_service_p, s_gatts_performance_service_default, sizeof(s_gatts_performance_service_default));

    s_gatts_performance_service_p[0].att_desc.attr_content.uuid.uuid16 = service_uuid;
    s_gatts_performance_service_p[1].att_desc.attr_content.uuid.uuid16 = char_uuid;

    s_performance_tx_method = (tx_method ? 1 : 0);
    s_profile_is_init = 1;
    s_char_recv_cb = recv_cb;

    profile_reg_db();

    bk_at_dm_gattc_add_gattc_callback(bk_gattc_cb);

    LOGI("done, tx method %d", s_performance_tx_method);
#else
    LOGE("niu spp not enable");
#endif
    return 0;
}

int32_t bk_performance_test_profile_deinit(uint8_t deinit_bluetooth_future)
{
#if BK_PERFORMANCE_PROFILE_ENABLE

    if (!s_profile_is_init)
    {
        LOGE("already deinit");
        return -1;
    }

    s_char_recv_cb = NULL;

    LOGW("sdk can't del db service now !!!");

    bk_at_dm_gatts_unreg_db((bk_gatts_attr_db_t *)s_gatts_performance_service_p);

    if (deinit_bluetooth_future)
    {
        s_db_init = 0;
        os_free(s_gatts_performance_service_p);
        s_gatts_performance_service_p = NULL;
    }

    s_profile_is_init = 0;
#endif
    return 0;
}


void bk_performance_test_profile_set_data_len(uint16_t len)
{
    if (!len)
    {
        LOGE("param err");
    }

    s_send_data_len = len;
}

void bk_performance_test_profile_set_interval(uint16_t interval)
{
    if (!interval)
    {
        LOGE("param err");
    }

    s_send_interval = interval;
}

static void statistics_timer_hdl(void *param)
{
    uint32_t tmp = 0;
    uint8_t type = s_performance_statistics_type;

    if (type & STATISTICS_TYPE_TX)
    {
        tmp = s_performance_tx_bytes;
        s_performance_tx_bytes = 0;
        LOGI("current tx %d bytes/sec", tmp);
    }

    if (type & STATISTICS_TYPE_RX)
    {
        tmp = s_performance_rx_bytes;
        s_performance_rx_bytes = 0;
        LOGI("current rx %d bytes/sec", tmp);
    }
}

void bk_performance_test_profile_enable_tx(uint8_t enable)
{
    int32_t ret = 0;

    if (!s_profile_is_init)
    {
        LOGE("already deinit");
        return;
    }

    s_char_notify_desc_buff = enable;

    if (enable)
    {
        LOGI("enable performance test");

        if (s_performance_tx_method == 1)
        {
            ble_tx_test_timer_callback((void *) -1);
        }
        else if (s_performance_tx_method == 0)
        {
            if (!rtos_is_timer_init(&s_performance_send_test_timer))
            {
                ret = rtos_init_timer(&s_performance_send_test_timer, s_send_interval, ble_tx_test_timer_callback, (void *) -1);

                if (ret)
                {
                    LOGE("init timer err %d", ret);
                    goto end;
                }
            }

            if (rtos_is_timer_running(&s_performance_send_test_timer))
            {
                ret = rtos_stop_timer(&s_performance_send_test_timer);

                if (ret)
                {
                    LOGE("stop timer err %d", ret);
                    goto end;
                }
            }

            ret = rtos_start_timer(&s_performance_send_test_timer);

            if (ret)
            {
                LOGE("start timer err %d", ret);
                goto end;
            }
        }
    }
    else
    {
        LOGI("disable performance test");

        if (rtos_is_timer_init(&s_performance_send_test_timer))
        {
            if (rtos_is_timer_running(&s_performance_send_test_timer))
            {
                ret = rtos_stop_timer(&s_performance_send_test_timer);

                if (ret)
                {
                    LOGE("stop timer err %d", ret);
                    goto end;
                }
            }

            ret = rtos_deinit_timer(&s_performance_send_test_timer);

            if (ret)
            {
                LOGE("deinit timer err %d", ret);
                goto end;
            }

            os_memset(&s_performance_send_test_timer, 0, sizeof(s_performance_send_test_timer));
        }
    }

end:;
}

void bk_performance_test_profile_enable_statistics(uint8_t enable, uint8_t type) //type 1 tx, 2 rx
{
    int32_t ret = 0;

    if (!s_profile_is_init)
    {
        LOGE("already deinit");
        return;
    }

    if (type == 1)
    {
        if (enable)
        {
            s_performance_statistics_type |= STATISTICS_TYPE_TX;
        }
        else
        {
            s_performance_statistics_type &= ~STATISTICS_TYPE_TX;
        }
    }
    else if (type == 2) //rx
    {
        if (enable)
        {
            s_performance_statistics_type |= STATISTICS_TYPE_RX;
        }
        else
        {
            s_performance_statistics_type &= ~STATISTICS_TYPE_RX;
        }
    }

    s_performance_tx_bytes = s_performance_rx_bytes = 0;

    if (s_performance_statistics_type)
    {
        if (!rtos_is_timer_init(&s_performance_statistics_timer))
        {
            ret = rtos_init_timer(&s_performance_statistics_timer, 1000, statistics_timer_hdl, (void *)0);

            if (ret)
            {
                LOGE("init timer err %d", ret);
                goto end;
            }
        }

        if (rtos_is_timer_running(&s_performance_statistics_timer))
        {
            ret = rtos_stop_timer(&s_performance_statistics_timer);

            if (ret)
            {
                LOGE("stop timer err %d", ret);
                goto end;
            }
        }

        ret = rtos_start_timer(&s_performance_statistics_timer);

        if (ret)
        {
            LOGE("start timer err %d", ret);
            goto end;
        }
    }
    else
    {

        if (rtos_is_timer_init(&s_performance_statistics_timer))
        {
            if (rtos_is_timer_running(&s_performance_statistics_timer))
            {
                ret = rtos_stop_timer(&s_performance_statistics_timer);

                if (ret)
                {
                    LOGE("stop timer err %d", ret);
                    goto end;
                }
            }

            ret = rtos_deinit_timer(&s_performance_statistics_timer);

            if (ret)
            {
                LOGE("deinit timer err %d", ret);
                goto end;
            }

            os_memset(&s_performance_statistics_timer, 0, sizeof(s_performance_statistics_timer));
        }
    }

end:;
}
