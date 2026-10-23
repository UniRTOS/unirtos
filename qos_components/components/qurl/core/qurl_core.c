/** @file         qurl_core.c
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-08-31 21:17:53
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qurl_code_in.h"
#include "qurl_def_in.h"
#include "qurl_log.h"
#include "qurl_list.h"
#include "qurl_dbuf.h"
#include "qurl_core.h"
#include "qurl_port.h"
#include "qurl_network.h"
#include "qurl_setopt.h"
#include "qurl_getinfo.h"
#include "qurl_transfer.h"
#include "qurl_conn.h"
#include "qurl_select.h"
#include "qurl_url.h"

#if (QURL_CFG_ENABLE_FS == 0)
#include "qurl_default_fs.h"
#endif

#if QURL_CFG_ENABLE_HTTP
#include "qurl_http_multiform.h"
#endif

#if QURL_CFG_ENABLE_SMTP
#include "qurl_smtp.h"
#endif

qurl_core_sec_t g_core_sec[QURL_CFG_CORE_SERVER_MAX] = {0};

/*Global operation of g_core_sec*/
qurl_ecode_t qurl_core_sec_init(void)
{
    qurl_ecode_t ret = QURL_OK;
    int          i = 0;

    for (i = 0; i < QURL_CFG_CORE_SERVER_MAX; i++)
    {
        ret = qurl_mutex_create(&g_core_sec[i].lock);
        if (ret != QURL_OK)
        {
            break;
        }
        g_core_sec[i].owner = QOSA_NULL;
        g_core_sec[i].state = QURL_SEC_STATE_IDLE;
    }

    /*[jamie][note][tab][20230908]Force deletion.*/
    if (i < QURL_CFG_CORE_SERVER_MAX)
    {
        for (--i; i >= 0; i--)
        {
            qurl_mutex_delete(g_core_sec[i].lock);
        }
    }

    return ret;
}

qurl_ecode_t qurl_core_sec_deinit(void)
{
    qurl_ecode_t ret = QURL_OK;
    int          i = 0;

    //Force release. [lzm][log][tab][20230908]Of course, a safe release option can also be chosen; it can be added later when customers have frequent use cases for global init and deinit.
    for (i = 0; i < QURL_CFG_CORE_SERVER_MAX; i++)
    {
        qurl_mutex_delete(g_core_sec[i].lock);
        g_core_sec[i].owner = QOSA_NULL;
        g_core_sec[i].state = QURL_SEC_STATE_IDLE;
    }

    return ret;
}

qurl_ecode_t qurl_core_sec_malloc(int *index_ptr)
{
    qurl_ecode_t ret = QURL_OK;
    int          i = 0;

    for (i = 0; i < QURL_CFG_CORE_SERVER_MAX; i++)
    {
        ret = qurl_mutex_lock(g_core_sec[i].lock, 0); /*[lzm][note]I don't want to wait.*/
        if (ret != QURL_OK)
        {
            continue;
        }

        if (g_core_sec[i].state == QURL_SEC_STATE_IDLE)
        {
            *index_ptr = i;
            g_core_sec[i].owner = QOSA_NULL;
            g_core_sec[i].state = QURL_SEC_STATE_BUSY;
            qurl_mutex_unlock(g_core_sec[i].lock);
            break;
        }
        qurl_mutex_unlock(g_core_sec[i].lock);
    }

    if (i >= QURL_CFG_CORE_SERVER_MAX)
    {
        *index_ptr = -1;
        ret = QURL_ECODE_NO_MEMORY;
    }

    return ret;
}

qurl_ecode_t qurl_core_sec_free(int index)
{
    qurl_ecode_t ret = QURL_OK;
    int          i = 0;

    ret = qurl_mutex_lock(g_core_sec[index].lock, QURL_WAIT_FOREVER);
    if (ret != QURL_OK)
    {
        return ret;
    }

    g_core_sec[index].owner = QOSA_NULL;
    g_core_sec[index].state = QURL_SEC_STATE_IDLE;

    qurl_mutex_unlock(g_core_sec[i].lock);

    return ret;
}

static void core_default_usr_cfg(qurl_core_ctrl_t *core_ptr)
{
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    core_ptr->usr_cfg.nw_id = -1; /*Default not specified*/
#endif

    core_ptr->usr_cfg.port = 0;   /*Default not specified*/

#if QURL_CFG_ENABLE_HTTP
    core_ptr->usr_cfg.http.http_version = QURL_HTTP_VERSION_NONE;
#endif                                                                                      /* QURL_CFG_ENABLE_HTTP */

    core_ptr->usr_cfg.timeout_ms = QURL_CFG_DEFAULT_PERFORM_TIMEOUT_MS;                     /*Default 30s*/
    core_ptr->usr_cfg.idle_timeout_ms = QURL_CFG_DEFAULT_PERFORM_IDLE_TIMEOUT_MS;           /*Default not enabled*/
    core_ptr->usr_cfg.conn_idle_timeout_ms = QURL_CFG_DEFAULT_PERFORM_CONN_IDLE_TIMEOUT_MS; /*Default 120s*/
    core_ptr->usr_cfg.conn_maxlifetime_ms = QURL_CFG_DEFAULT_PERFORM_CONN_MAXLIFETIME_MS;   /*Default not enabled*/
    core_ptr->usr_cfg.rouse_check_time_ms = QURL_CFG_DEFAULT_ROUSE_CHECK_TIME_MS;

    core_ptr->usr_cfg.resume_from = 0;                  /*Default recovery transmission disabled*/

    core_ptr->usr_cfg.upload_head_data_ptr = QOSA_NULL; /*First clear*/
    core_ptr->usr_cfg.upload_head_size = -1;            /*Not used*/
    core_ptr->usr_cfg.upload_data_ptr = QOSA_NULL;      /*First clear*/
    core_ptr->usr_cfg.upload_size = -1;                 /*Not used*/

    core_ptr->usr_cfg.redirs_cnt_max = 1;               /*Default maximum number of redirections.*/

    /*Some markers*/
    core_ptr->usr_cfg.bits.bound_thread = QOSA_TRUE; /*Default binding*/
    core_ptr->usr_cfg.bits.reuse_fresh = QOSA_TRUE;  /*[lzm][todo]To be modified to false after developing the connection cache pool*/
    core_ptr->usr_cfg.bits.reuse_forbid = QOSA_TRUE; /*[lzm][todo]To be modified to false after developing the connection cache pool*/
    core_ptr->usr_cfg.bits.reuse_hold = QOSA_TRUE;
    core_ptr->usr_cfg.bits.tcp_fastopen = 0;

    /* http */

    /* ftp */
    core_ptr->usr_cfg.bits.ftp_skip_ip = QOSA_TRUE;   /*In PASV mode, the IP address issued by the server is ignored by default, and the IP address of the control connection is used instead.*/
    core_ptr->usr_cfg.bits.ftp_use_port = QOSA_FALSE; /*Prefer passive mode*/
    core_ptr->usr_cfg.bits.ftp_use_epsv = QOSA_TRUE;  /*Default to using extended commands*/
    core_ptr->usr_cfg.bits.ftp_use_eprt = QOSA_TRUE;  /*Default to using extended commands*/
    core_ptr->usr_cfg.bits.ftp_use_pret = QOSA_FALSE; /*Primarily used for DRFTPD servers. Therefore, it is disabled by default.*/
}

static qurl_ecode_t core_default_config(qurl_core_ctrl_t *core_ptr)
{
    /*Internal interface, some parameter validation is performed by the exposed interface layer.*/
    qurl_logd("core default config\r\n");

    /*Default Configuration*/
    core_ptr->state = QURL_STATE_INIT;
    core_ptr->bits.abort_flag = QOSA_FALSE;

    core_default_usr_cfg(core_ptr);

    return QURL_OK;
}

/**
 * @brief State transition function. Core state changes can only be achieved through this function.
 * @return void
 */
static inline void core_state_change(qurl_core_ctrl_t *core_ptr, qurl_state_e new_state)
{
    qurl_state_e old_state = core_ptr->state;

    core_ptr->state = new_state;

    QURL_UNUSED(old_state);
    qurl_logd("state change:[%d]->[%d]\r\n", old_state, new_state);
}

/**
 * @brief abort setting
 * @return void
 */
static inline void core_abort_set(qurl_core_ctrl_t *core_ptr, qurl_state_e new_abort_flag)
{
    qurl_bit_t old_abort_flag = core_ptr->bits.abort_flag;

    core_ptr->bits.abort_flag = new_abort_flag;

    QURL_UNUSED(old_abort_flag);
    qurl_logd("state change:[%d]->[%d]\r\n", old_abort_flag, new_abort_flag);
}

/**
 * @brief abort acquisition
 * @return qurl_bit_t
 */
qurl_bit_t core_abort_get(qurl_core_ctrl_t *core_ptr)
{
    return core_ptr->bits.abort_flag;
}

/**
 * @brief timeout check
 * @return qurl_bit_t
 */
qurl_bit_t core_timeout_check(qurl_core_ctrl_t *core_ptr)
{
    qurl_timeval_t now_timeval = {0};
    qurl_time_t    diff_time_ms = 0;

    if (core_ptr->bits.timeout != QOSA_FALSE)
    {
        return QOSA_TRUE;
    }
    if (core_ptr->usr_cfg.timeout_ms > 0)
    {
        now_timeval = qurl_time_now();
        diff_time_ms = qurl_time_diff_ms(core_ptr->start_timeval, now_timeval);
        diff_time_ms = core_ptr->usr_cfg.timeout_ms - diff_time_ms;
        if (diff_time_ms < 0)
        {
            core_ptr->bits.timeout = QOSA_TRUE;
            return QOSA_TRUE;
        }
    }
    return QOSA_FALSE;
}

/**
 * @brief Idle timeout watchdog feeding
 * @return qurl_bit_t
 */
void core_idle_timeout_feed_dog(qurl_core_ctrl_t *core_ptr)
{
    if (core_ptr->usr_cfg.idle_timeout_ms > 0)
    {
        core_ptr->idle_start_timeval = qurl_time_now();
    }
}

/**
 * @brief Idle timeout watchdog feeding
 * @return qurl_bit_t
 */
qurl_bit_t core_idle_timeout_check_dog(qurl_core_ctrl_t *core_ptr)
{
    qurl_timeval_t now_timeval = {0};
    qurl_time_t    diff_time_ms = 0;

    if (core_ptr->bits.timeout != QOSA_FALSE)
    {
        return QOSA_TRUE;
    }
    if (core_ptr->usr_cfg.idle_timeout_ms > 0)
    {
        now_timeval = qurl_time_now();
        diff_time_ms = qurl_time_diff_ms(core_ptr->idle_start_timeval, now_timeval);
        diff_time_ms = core_ptr->usr_cfg.idle_timeout_ms - diff_time_ms;
        if (diff_time_ms <= 0)
        {
            core_ptr->bits.timeout = QOSA_TRUE;
            return QOSA_TRUE;
        }
    }
    return QOSA_FALSE;
}

static int core_get_socket_connect(qurl_core_ctrl_t *core_ptr, qurl_socket_t *socks_ptr)
{
    int i = 0;
    int s = 0;
    int rc = 0;

    /*[lzm][todo]Connection status judgment, used for developing socket proxy*/

    for (i = 0; i < 2; i++)
    {
        if (core_ptr->conn_ptr->temp_sock[i] != QURL_SOCKET_BAD)
        {
            socks_ptr[s] = core_ptr->conn_ptr->temp_sock[i];
            rc |= QURL_GETSOCK_WRITESOCK_FUNC(s);
            s++;
        }
    }

    return rc;
}

static qurl_ecode_t core_get_socket(qurl_core_ctrl_t *core_ptr, qurl_socket_t *socks, int *bitmap_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    *bitmap_ptr = QURL_GETSOCK_BLANK;
    if (!core_ptr->conn_ptr)
    {
        /*Not yet bound connection*/
        return ret;
    }

    if (core_ptr->state > QURL_STATE_CONNECTION && core_ptr->state < QURL_STATE_COMPLETED)
    {
        /*Mutual binding*/
        core_ptr->conn_ptr->core_ptr = core_ptr;
    }

    switch (core_ptr->state)
    {
        case QURL_STATE_RESOLVE: {
            /*[lzm][socket][todo]Consider what method to use for blocking here.*/
        }
        break;
        case QURL_STATE_CONNECT: {
            *bitmap_ptr = core_get_socket_connect(core_ptr, socks);
        }
        break;
        case QURL_STATE_WAIT_PROXY_CONNECT: {
            /*[lzm][socket][todo]HTTP-related: when sending CONNECT to a proxy, need to wait for the socket to become readable.*/
        }
        break;
        case QURL_STATE_SEND_PROT_CONNECT:
        case QURL_STATE_PROT_CONNECT: {
            if (core_ptr->conn_ptr->port_handler->prot_getsock_func)
            {
                *bitmap_ptr = core_ptr->conn_ptr->port_handler->prot_getsock_func(core_ptr->conn_ptr, socks);
            }
            else
            {
                socks[0] = core_ptr->conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd;
                *bitmap_ptr = QURL_GETSOCK_READSOCK_FUNC(0) | QURL_GETSOCK_WRITESOCK_FUNC(0);
            }
        }
        break;
        case QURL_STATE_REQ:
        case QURL_STATE_REQING: {
            if (core_ptr->conn_ptr->port_handler->req_getsock_func)
            {
                *bitmap_ptr = core_ptr->conn_ptr->port_handler->req_getsock_func(core_ptr->conn_ptr, socks);
            }
            else
            {
                *bitmap_ptr = QURL_GETSOCK_BLANK;
            }
        }
        break;
        case QURL_STATE_REQ_MORE: {
            if (core_ptr->conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].bits.trans_connecting == QOSA_TRUE
                && core_ptr->conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].bits.trans_connected == QOSA_FALSE)
            {
                *bitmap_ptr = core_get_socket_connect(core_ptr, socks);
            }
            else if (core_ptr->conn_ptr->port_handler->req_more_getsock_func)
            {
                *bitmap_ptr = core_ptr->conn_ptr->port_handler->req_more_getsock_func(core_ptr->conn_ptr, socks);
            }
            else
            {
                *bitmap_ptr = QURL_GETSOCK_BLANK;
            }
        }
        break;
        case QURL_STATE_REQ_DONE:
        case QURL_STATE_TRANS_DATA: {
            if (core_ptr->conn_ptr->port_handler->trans_data_getsock_func)
            {
                *bitmap_ptr = core_ptr->conn_ptr->port_handler->trans_data_getsock_func(core_ptr->conn_ptr, socks);
            }
            else
            {
                /*[lzm][socket][todo]Need to consider more.*/
                *bitmap_ptr = QURL_GETSOCK_BLANK;
            }
        }
        break;
        default:
            break;
    }

    return ret;
}

/**
 * @brief Blocking mechanism of qurl core
 * @return void
 */
static qurl_ecode_t core_wait(qurl_core_ctrl_t *core_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    switch (core_ptr->state)
    {
        case QURL_STATE_INIT: {
            /*Start performing timing. [lzm][note]Check if time processing is executed in core wait.*/
            core_ptr->start_timeval = qurl_time_now();
            qurl_logd("start_timeval:[%d:%d]\r\n", core_ptr->start_timeval.s, core_ptr->start_timeval.ms);

            core_ptr->idle_start_timeval = qurl_time_now();
            qurl_logd("idle_start_timeval:[%d:%d]\r\n", core_ptr->idle_start_timeval.s, core_ptr->idle_start_timeval.ms);

            break;
        }
        case QURL_STATE_NETWORK: {
            unsigned char  continue_flag = 1;
            qurl_timeval_t now_timeval = {0};
            qurl_time_t    diff_time_ms = 0;
            qurl_time_t    idle_diff_time_ms = 0;
            qurl_time_t    wait_time_ms = core_ptr->usr_cfg.rouse_check_time_ms; /*It wakes up by default every second to check for abort and other statuses.*/

            do {
                QURL_CORE_CHECK_ABORT_FUNC(core_ptr);
                continue_flag = 0;

                qurl_memset(&now_timeval, 0x00, sizeof(qurl_timeval_t));
                diff_time_ms = QURL_TIME_T_MAX;
                idle_diff_time_ms = QURL_TIME_T_MAX;
                /*1. Calculate blocking time*/
                now_timeval = qurl_time_now();
                /** Total timeout*/
                if (core_ptr->usr_cfg.timeout_ms > 0)
                {
                    diff_time_ms = qurl_time_diff_ms(core_ptr->start_timeval, now_timeval);
                    diff_time_ms = core_ptr->usr_cfg.timeout_ms - diff_time_ms;
                }
                /** Idle timeout*/
                if (core_ptr->usr_cfg.idle_timeout_ms > 0)
                {
                    idle_diff_time_ms = qurl_time_diff_ms(core_ptr->idle_start_timeval, now_timeval);
                    idle_diff_time_ms = core_ptr->usr_cfg.idle_timeout_ms - idle_diff_time_ms;
                }
                /** Select the smallest*/
                diff_time_ms = QURL_MIN(diff_time_ms, idle_diff_time_ms);
                if (diff_time_ms < 0)
                {
                    continue_flag = 0; /*Timeout occurred*/
                    diff_time_ms = 0;
                    wait_time_ms = 0;
                }
                else if (wait_time_ms > diff_time_ms)
                {
                    wait_time_ms = diff_time_ms;
                }
                diff_time_ms -= wait_time_ms;

                /*2. Enter Event Blocking*/
                qurl_memset(&core_ptr->recv_event, 0x00, sizeof(core_ptr->recv_event));
                qurl_mutex_unlock(g_core_sec[core_ptr->sec_index].lock);
                ret = qurl_event_wait(core_ptr->ipc_event, &core_ptr->recv_event, wait_time_ms);
                qurl_mutex_lock(g_core_sec[core_ptr->sec_index].lock, QURL_WAIT_FOREVER);
                if (ret != QURL_OK)
                {
                    if (ret == (qurl_ecode_t)QURL_ECODE_EVENT_TIMEOUT && diff_time_ms > 0)
                    {
                        continue_flag = 1;
                        continue;
                    }
                    else
                    {
                        qurl_logd("ret:[%d:%d]\r\n", ret, wait_time_ms);
                        continue_flag = 0;
                        break;
                    }
                }
                else
                {
                    /*3. Determine Event*/
                    if (core_ptr->recv_event.id == QURL_EVENT_CODE_NETWORK)
                    {
                        ret = (qurl_ecode_t)core_ptr->recv_event.param1;
                        continue_flag = 0; /*Received a valid event, proceed to the next step.*/
                    }
                    /*Received an illegal event, continue receiving until timeout or exception.*/
                }
            } while (continue_flag);
            break;
        }
        case QURL_STATE_RESOLVE: {
            unsigned char  continue_flag = 1;
            qurl_timeval_t now_timeval = {0};
            qurl_time_t    diff_time_ms = 0;
            qurl_time_t    idle_diff_time_ms = 0;
            qurl_time_t    wait_time_ms = core_ptr->usr_cfg.rouse_check_time_ms; /*It wakes up by default every second to check for abort and other statuses.*/

            do {
                QURL_CORE_CHECK_ABORT_FUNC(core_ptr);
                continue_flag = 0;

                qurl_memset(&now_timeval, 0x00, sizeof(qurl_timeval_t));
                diff_time_ms = QURL_TIME_T_MAX;
                idle_diff_time_ms = QURL_TIME_T_MAX;
                /*1. Calculate blocking time*/
                now_timeval = qurl_time_now();
                /** Total timeout*/
                if (core_ptr->usr_cfg.timeout_ms > 0)
                {
                    diff_time_ms = qurl_time_diff_ms(core_ptr->start_timeval, now_timeval);
                    diff_time_ms = core_ptr->usr_cfg.timeout_ms - diff_time_ms;
                }
                /** Idle timeout*/
                if (core_ptr->usr_cfg.idle_timeout_ms > 0)
                {
                    idle_diff_time_ms = qurl_time_diff_ms(core_ptr->idle_start_timeval, now_timeval);
                    idle_diff_time_ms = core_ptr->usr_cfg.idle_timeout_ms - idle_diff_time_ms;
                }
                /** Select the smallest*/
                diff_time_ms = QURL_MIN(diff_time_ms, idle_diff_time_ms);
                if (diff_time_ms < 0)
                {
                    continue_flag = 0; /*Timeout occurred*/
                    diff_time_ms = 0;
                    wait_time_ms = 0;
                }
                else if (wait_time_ms > diff_time_ms)
                {
                    wait_time_ms = diff_time_ms;
                }
                diff_time_ms -= wait_time_ms;

                /*2. Enter event blocking*/
                qurl_memset(&core_ptr->recv_event, 0x00, sizeof(core_ptr->recv_event));
                qurl_mutex_unlock(g_core_sec[core_ptr->sec_index].lock);
                ret = qurl_event_wait(core_ptr->ipc_event, &core_ptr->recv_event, wait_time_ms);
                qurl_mutex_lock(g_core_sec[core_ptr->sec_index].lock, QURL_WAIT_FOREVER);
                if (ret != QURL_OK)
                {
                    if (ret == (qurl_ecode_t)QURL_ECODE_EVENT_TIMEOUT && diff_time_ms > 0)
                    {
                        continue_flag = 1;
                        continue;
                    }
                    else
                    {
                        continue_flag = 0;
                        break;
                    }
                }
                else
                {
                    /*3. Determine Event*/
                    if (core_ptr->recv_event.id == QURL_EVENT_CODE_RESOLVE)
                    {
                        ret = (qurl_ecode_t)core_ptr->recv_event.param1;
                        continue_flag = 0; /*Received a valid event, proceed to the next step.*/
                    }
                    /*Received an illegal event, continue receiving until timeout or exception.*/
                }
            } while (continue_flag);
            break;
        }
        case QURL_STATE_CONNECT:
        case QURL_STATE_SEND_PROT_CONNECT:
        case QURL_STATE_PROT_CONNECT:
        case QURL_STATE_REQ:
        case QURL_STATE_REQING:
        case QURL_STATE_REQ_MORE:
        case QURL_STATE_TRANS_DATA: {
            unsigned char  continue_flag = 1;
            int            i = 0;
            qurl_time_t    wait_time_ms = core_ptr->usr_cfg.rouse_check_time_ms; /*It wakes up by default every second to check for abort and other statuses.*/
            qurl_time_t    diff_time_ms = 0;
            qurl_time_t    idle_diff_time_ms = 0;
            qurl_timeval_t now_timeval = {0};
            fd_set         fds_read = {0};
            fd_set         fds_write = {0};
            fd_set         fds_err = {0};
            qurl_socket_t  maxfd = -1;
            qurl_socket_t  temp_sock = 0;
            int            bitmap = 0; /*Corresponding to the read and write of the monitored sockfd*/

            do {
                QURL_CORE_CHECK_ABORT_FUNC(core_ptr);
                continue_flag = 0;

                qurl_socket_t sockbunch[QURL_CORE_PROTOCOL_SOCKET_MAX] = {QURL_SOCKET_BAD}; /*The socket fd that needs to be monitored*/

                /*1. First calculate the nearest unblocking time*/
                /*2. Enter select blocking*/

                diff_time_ms = QURL_TIME_T_MAX;
                idle_diff_time_ms = QURL_TIME_T_MAX;
                /*1. Calculate the nearest unblocking time*/
                now_timeval = qurl_time_now();
                /** Total timeout*/
                if (core_ptr->usr_cfg.timeout_ms > 0)
                {
                    diff_time_ms = qurl_time_diff_ms(core_ptr->start_timeval, now_timeval);
                    diff_time_ms = core_ptr->usr_cfg.timeout_ms - diff_time_ms;
                }
                /** Idle timeout*/
                if (core_ptr->usr_cfg.idle_timeout_ms > 0)
                {
                    idle_diff_time_ms = qurl_time_diff_ms(core_ptr->idle_start_timeval, now_timeval);
                    idle_diff_time_ms = core_ptr->usr_cfg.idle_timeout_ms - idle_diff_time_ms;
                }
                /** Select the smallest*/
                diff_time_ms = QURL_MIN(diff_time_ms, idle_diff_time_ms);
                if (diff_time_ms < 0)
                {
                    continue_flag = 0; /*Timeout occurred*/
                    diff_time_ms = 0;
                    wait_time_ms = 0;
                    core_ptr->bits.timeout = QOSA_TRUE;
                    ret = QURL_ECODE_PERFORM_TIMEOUT;
                    break; /*Change to forced exit*/
                }
                else if (wait_time_ms > diff_time_ms)
                {
                    wait_time_ms = diff_time_ms;
                }

                /*2. Enter select blocking*/
                /** Select the fd to listen on*/
                FD_ZERO(&fds_read);
                FD_ZERO(&fds_write);
                FD_ZERO(&fds_err);
                maxfd = (qurl_socket_t)-1;
                for (i = 0; i < QURL_CORE_PROTOCOL_SOCKET_MAX; i++)
                {
                    sockbunch[i] = QURL_SOCKET_BAD;
                }

                ret = core_get_socket(core_ptr, sockbunch, &bitmap);
                if (ret != QURL_OK)
                {
                    return ret;
                }
                for (i = 0; i < QURL_CORE_PROTOCOL_SOCKET_MAX; i++)
                {
                    temp_sock = sockbunch[i];
                    if (temp_sock == QURL_SOCKET_BAD)
                    {
                        break;
                    }
                    if (bitmap & QURL_GETSOCK_READSOCK_FUNC(i))
                    {
                        FD_SET(temp_sock, &fds_read);
                    }
                    if (bitmap & QURL_GETSOCK_WRITESOCK_FUNC(i))
                    {
                        FD_SET(temp_sock, &fds_write);
                    }
                    FD_SET(temp_sock, &fds_err);
                    if (temp_sock > maxfd)
                    {
                        maxfd = temp_sock;
                    }
                }

                qurl_mutex_unlock(g_core_sec[core_ptr->sec_index].lock);
                ret = qurl_select(maxfd, &fds_read, &fds_write, &fds_err, wait_time_ms);
                qurl_mutex_lock(g_core_sec[core_ptr->sec_index].lock, QURL_WAIT_FOREVER);
                if (ret > 0)
                {
                    continue_flag = 0;
                    qurl_logd("select ok:%x\r\n", ret);
                    ret = QURL_OK;
                }
                else if (ret == 0)
                {
                    continue_flag = 1;
                    continue;
                }
                else
                {
                    continue_flag = 0;
                    break;
                }
            } while (continue_flag);
            break;
        }
        default:
            /*No need to wait*/
            break;
    }

    QURL_CORE_CHECK_ABORT_FUNC(core_ptr);

    /*Correct error codes*/
    if (ret == (int)QURL_ECODE_EVENT_TIMEOUT)
    {
        ret = QURL_ECODE_PERFORM_TIMEOUT;
    }

    return ret;
}

static qurl_ecode_t core_perform(qurl_core_ctrl_t *core_ptr)
{
    qurl_ecode_t   ret = QURL_OK;
    unsigned char  continue_flag = 0;
    qurl_timeval_t now_timeval = {0};
    qurl_time_t    diff_time_ms = 0;
    qurl_time_t    idle_diff_time_ms = 0;
    int            control = 0;
    qurl_bit_t     connected = QOSA_FALSE;
    qurl_bit_t     prot_connected = QOSA_FALSE;
    qurl_bit_t     req_phase_done = QOSA_FALSE;
    qurl_bit_t     trans_data_done = QOSA_FALSE;

    qurl_logd("state:[%d]\r\n", core_ptr->state);

    do {
        continue_flag = 0;

        if (core_ptr->bits.timeout == QOSA_FALSE)
        {
            qurl_memset(&now_timeval, 0x00, sizeof(qurl_timeval_t));
            diff_time_ms = QURL_TIME_T_MAX;
            idle_diff_time_ms = QURL_TIME_T_MAX;
            now_timeval = qurl_time_now(); /*[lzm][note]Consider whether to update the time inside the loop or outside the loop*/
            /** Total timeout*/
            if (core_ptr->usr_cfg.timeout_ms > 0)
            {
                diff_time_ms = qurl_time_diff_ms(core_ptr->start_timeval, now_timeval);
                diff_time_ms = core_ptr->usr_cfg.timeout_ms - diff_time_ms;
            }
            /** Idle timeout*/
            if (core_ptr->usr_cfg.idle_timeout_ms > 0)
            {
                idle_diff_time_ms = qurl_time_diff_ms(core_ptr->idle_start_timeval, now_timeval);
                idle_diff_time_ms = core_ptr->usr_cfg.idle_timeout_ms - idle_diff_time_ms;
            }
            /** Select the smallest*/
            diff_time_ms = QURL_MIN(diff_time_ms, idle_diff_time_ms);
            if (diff_time_ms < 0)
            {
                core_ptr->bits.timeout = QOSA_TRUE;
                qurl_logd("start_timeval:[%d:%d]\r\n", core_ptr->start_timeval.s, core_ptr->start_timeval.ms);
                qurl_logd("idle_start_timeval:[%d:%d]\r\n", core_ptr->idle_start_timeval.s, core_ptr->idle_start_timeval.ms);
                qurl_logd("now_timeval:[%d:%lld]\r\n", now_timeval.s, now_timeval.ms);
            }
        }

        /*State Machine Execution*/
        switch (core_ptr->state)
        {
            case QURL_STATE_INIT: {
                //Initial state
                qurl_logd("state:[INIT]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK)
                {
                    ret = core_ptr->err_code;
                    break;
                }
                else if (core_ptr->bits.abort_flag != QOSA_FALSE)
                {
                    ret = QURL_ECODE_ABORT;
                    break;
                }
                if (core_ptr->bits.timeout != QOSA_FALSE)
                {
                    ret = QURL_ECODE_PERFORM_TIMEOUT;
                    break;
                }

                ret = qurl_trans_init(core_ptr);
                if (ret == QURL_OK)
                {
                    /*Activate network*/
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
                    core_state_change(core_ptr, QURL_STATE_NETWORK);
#else
                    core_state_change(core_ptr, QURL_STATE_CONNECTION);
#endif
                    continue_flag = 1;
                }

                break;
            }
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
            case QURL_STATE_NETWORK: {
                int activated = 0;
                qurl_logd("state:[NETWORK]\r\n");
                //Ensure the network is functioning properly

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_COMPLETED);
                    continue_flag = 1;
                    break;
                }

                activated = qurl_nw_is_activated(core_ptr);
                if (!activated)
                {
                    ret = qurl_nw_activate_async(core_ptr);
                    if (ret != QURL_OK)
                    {
                        qurl_logd("state:[CONNECTION][ret:%x]\r\n", ret);
                        core_state_change(core_ptr, QURL_STATE_COMPLETED);
                        continue_flag = 1;
                        break;
                    }
                }

                if (activated)
                {
                    qurl_nw_get_ip(core_ptr);
                    core_state_change(core_ptr, QURL_STATE_CONNECTION);
                    continue_flag = 1;
                }
                /** Check already activated: switch to QURL_STATE_CONNECT, continue*/
                /** Not activated: exit, enter wait event*/

                break;
            }
#endif
            // case QURL_STATE_CONNECT_PEND:
            // {
            //// No connection, waiting for a connection
            //     /* We will stay here until there is a connection available.
            //      * Then we try again in the QURL_STATE_CONNECTION state. */
            //     qurl_logd( "state:[CONNECT_PEND]\r\n");
            //
            //     break;
            // }
            case QURL_STATE_CONNECTION: {
                //Initiated parsing and connection operations
                qurl_logd("state:[CONNECTION]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_COMPLETED);
                    continue_flag = 1;
                    break;
                }

                /*[qurl][todo]Create/find connection and bind*/
                ret = qurl_conn_create(core_ptr);
                if (ret != QURL_OK)
                {
                    qurl_loge("state:[CONNECTION][ret:%x]\r\n", ret);
                    continue_flag = 1;
                    core_state_change(core_ptr, QURL_STATE_COMPLETED);
                    break;
                }

                /*Jump to resolve*/
                core_state_change(core_ptr, QURL_STATE_RESOLVE);
                continue_flag = 1;

                break;
            }
            case QURL_STATE_RESOLVE: {
                //Wait for the parsing operation to complete
                qurl_logd("state:[RESOLVE]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }

                /*[qurl][todo]Check parsing status*/
                ret = qurl_conn_core_resolve_check(core_ptr->conn_ptr);
                if (ret == QURL_OK)
                {
                    continue_flag = 1;
                    core_state_change(core_ptr, QURL_STATE_CONNECT);
                    break;
                }

                ret = qurl_conn_core_resolve(core_ptr->conn_ptr);
                if (ret != QURL_OK)
                {
                    qurl_loge("faild[ret:%d]\r\n", ret);
                    continue_flag = 1;
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    break;
                }

                break;
            }
            case QURL_STATE_CONNECT: {
                //Wait for TCP connection to complete
                qurl_logd("state:[CONNECT]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }

                ret = qurl_conn_core_connected_check(core_ptr->conn_ptr, QURL_SOCKET_INDEX_0, &connected);
                if (ret != QURL_OK)
                {
                    qurl_loge("faild[ret:%x]\r\n", ret);
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }
                if (connected == QOSA_TRUE)
                {
                    core_state_change(core_ptr, QURL_STATE_SEND_PROT_CONNECT);
                    continue_flag = 1;
                    break;
                }

                /*[qurl][todo]Execute TCP connection*/
                ret = qurl_conn_core_connect(core_ptr->conn_ptr);
                if (ret != QURL_OK)
                {
                    qurl_loge("faild[ret:%x]\r\n", ret);
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }

                break;
            }
            case QURL_STATE_WAIT_PROXY_CONNECT: {
                //Wait for SSL initialization of the HTTPS proxy or completion of the proxy connection
                break;
            }
            case QURL_STATE_SEND_PROT_CONNECT: {
                //Start protocol connection process: the current state will only be executed once.
                qurl_logd("state:[SEND_PROT_CONNECT]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }

                ret = qurl_conn_core_prot_connected_check(core_ptr->conn_ptr, QURL_SOCKET_INDEX_0, &connected);
                if (ret != QURL_OK)
                {
                    qurl_loge("faild[ret:%x]\r\n", ret);
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }
                if (connected == QOSA_TRUE)
                {
                    core_state_change(core_ptr, QURL_STATE_REQ);
                    continue_flag = 1;
                    break;
                }
                ret = qurl_conn_core_send_prot_connect(core_ptr->conn_ptr, &prot_connected);
                if (ret == QURL_OK && prot_connected == QOSA_TRUE)
                {
                    core_state_change(core_ptr, QURL_STATE_REQ);
                    continue_flag = 1;
                }
                else if (ret == QURL_OK)
                {
                    /*The protocol connection has been initiated but not yet completed. At this point, simply wait for the data to become readable.*/
                    core_state_change(core_ptr, QURL_STATE_PROT_CONNECT);
                }
                else
                {
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }

                break;
            }
            case QURL_STATE_PROT_CONNECT: {
                //Complete the protocol-specific connection phase
                qurl_logd("state:[PROT_CONNECT]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }

                ret = qurl_conn_core_prot_connected_check(core_ptr->conn_ptr, QURL_SOCKET_INDEX_0, &connected);
                if (ret != QURL_OK)
                {
                    qurl_loge("faild[ret:%d]\r\n", ret);
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }
                if (connected == QOSA_TRUE)
                {
                    core_state_change(core_ptr, QURL_STATE_REQ);
                    continue_flag = 1;
                    break;
                }

                ret = qurl_conn_core_prot_connect(core_ptr->conn_ptr, &prot_connected);
                if (ret == QURL_OK && prot_connected == QOSA_TRUE)
                {
                    core_state_change(core_ptr, QURL_STATE_REQ);
                    continue_flag = 1;
                }
                else if (ret == QURL_OK)
                {
                    /*Protocol connecting.*/
                }
                else
                {
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }
                break;
            }
            case QURL_STATE_REQ: {
                qurl_logd("state:[REQ]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_REQ_DONE);
                    continue_flag = 1;
                    break;
                }

                /*Start sending request (first part): execute only once*/
                ret = qurl_conn_core_req(core_ptr->conn_ptr, &req_phase_done);
                if (ret == QURL_OK && req_phase_done == QOSA_TRUE)
                {
                    /*[lzm][todo]Here it is necessary to distinguish whether to enter REQ_MORE or REQ_DONE*/
                    core_state_change(core_ptr, QURL_STATE_REQ_DONE);
                    continue_flag = 1;
                }
                else if (ret == QURL_OK)
                {
                    core_state_change(core_ptr, QURL_STATE_REQING);
                    continue_flag = 1;
                }
                else
                {
                    core_state_change(core_ptr, QURL_STATE_REQ_DONE);
                    continue_flag = 1;
                    break;
                }

                break;
            }
            case QURL_STATE_REQING: {
                qurl_logd("state:[REQING]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_REQ_DONE);
                    continue_flag = 1;
                    break;
                }

                //Sending request (part 1)
                ret = qurl_conn_core_reqing(core_ptr->conn_ptr, &req_phase_done);
                if (ret == QURL_OK && req_phase_done == QOSA_TRUE)
                {
                    /*[lzm][todo]Here we need to distinguish whether to enter REQ_MORE or REQ_DONE*/
                    continue_flag = 1;
                    if (core_ptr->conn_ptr->bits.req_more)
                    {
                        core_state_change(core_ptr, QURL_STATE_REQ_MORE);
                    }
                    else
                    {
                        core_state_change(core_ptr, QURL_STATE_REQ_DONE);
                    }
                }
                else if (ret == QURL_OK)
                {
                    ; /*Continue waiting for sendable*/
                }
                else
                {
                    core_state_change(core_ptr, QURL_STATE_REQ_DONE);
                    continue_flag = 1;
                    break;
                }
                break;
            }
            case QURL_STATE_REQ_MORE: {
                qurl_logd("state:[REQ_MORE]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_REQ_DONE);
                    continue_flag = 1;
                    break;
                }

                //Continue sending request (Part 2)
                ret = qurl_conn_core_req_more(core_ptr->conn_ptr, &control);
                if (ret == QURL_OK && control)
                {
                    /*>0: Enter req done*/
                    /*<0:Return to requesting*/
                    if (control > 0)
                    {
                        core_state_change(core_ptr, QURL_STATE_REQ_DONE);
                    }
                    else
                    {
                        core_state_change(core_ptr, QURL_STATE_REQING);
                    }
                    continue_flag = 1;
                }
                else if (ret == QURL_OK)
                {
                    ; /*Continue waiting for sendable*/
                }
                else
                {
                    core_state_change(core_ptr, QURL_STATE_REQ_DONE);
                    continue_flag = 1;
                    break;
                }
                break;
            }
            case QURL_STATE_REQ_DONE: {
                /*[lzm][note]The significance of the current state. In curl, it is a buffer, this state is designed for multi,
				 * After the current easy request is completed, proceed to the next easy request first. Enter the data transmission state only after all easy requests have been sent.
				 * Therefore, there is no protocol stack callback function for the req_done state in curl either.
				 * In our qurl, this state is still maintained, but its meaning is different. Currently, it is only used to check whether normal entry into the data transmission state is possible.
				 */

                qurl_logd("state:[REQ_DONE]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || ret != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }

                /*After the protocol initiates the request and executes the transmission preparation, the following x_sock_fd will be assigned.*/
                // if (core_ptr->conn_ptr->r_sock_fd != QURL_SOCKET_BAD || core_ptr->conn_ptr->w_sock_fd != QURL_SOCKET_BAD)
                {
                    core_state_change(core_ptr, QURL_STATE_TRANS_DATA);
                    continue_flag
                        = 1; /*[202403151500] Changed to first enter the trans state to see the internal situation, this is to avoid getting stuck in select when data transmission is not needed. Of course, subsequent optimization of the state machine can prioritize modifying this part. [2023] Blocking continues, because it is necessary to check if the socket is readable.*/
                }

                break;
            }
            case QURL_STATE_TRANS_DATA: {
                char *new_url = QOSA_NULL;

                //Data transmission phase
                qurl_logd("state:[TRANS_DATA]\r\n");

                /*Exception Monitoring*/
                if (core_ptr->err_code != QURL_OK || core_ptr->bits.abort_flag != QOSA_FALSE || core_ptr->bits.timeout != QOSA_FALSE)
                {
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                    break;
                }

                ret = qurl_conn_core_trans_data(core_ptr->conn_ptr, &trans_data_done);
                if (ret == QURL_OK && trans_data_done == QOSA_TRUE)
                {
                    new_url = core_ptr->cfg.new_url;
                    core_ptr->cfg.new_url = QOSA_NULL;
                    /*When we follow a redirect or are set to retry the connection, we must return to the CONNECT state.*/
                    if (new_url != QOSA_NULL)
                    {
#if QURL_CFG_ENABLE_HTTP
                        /*[lzm][todo]The logic for determining retries and redirects needs to be modified; this is only a temporary fix.*/
                        qurl_logd("state:[%d]\r\n", core_ptr->cfg.http.http_auth);
                        if (core_ptr->cfg.http.http_auth)
                        {
                            ret = qurl_conn_core_trans_done(core_ptr->conn_ptr, ret, QOSA_FALSE);
                            if (ret != QURL_OK)
                            {
                                break;
                            }

                            ret = qurl_trans_follow(core_ptr, new_url, QURL_TRANS_FOLLOWTYPE_RETRY);
                            qurl_free(new_url);
                            new_url = QOSA_NULL;
                            if (ret == QURL_OK)
                            {
                                core_state_change(core_ptr, QURL_STATE_CONNECTION);
                                break;
                            }
                        }
                        else
#endif /* QURL_CFG_ENABLE_HTTP */
                            if (core_ptr->cfg.bits.is_follow)
                            {
                                ret = qurl_conn_core_trans_done(core_ptr->conn_ptr, ret, QOSA_FALSE);
                                if (ret != QURL_OK)
                                {
                                    break;
                                }

                                ret = qurl_trans_follow(core_ptr, new_url, QURL_TRANS_FOLLOWTYPE_REDIR);
                                qurl_free(new_url);
                                new_url = QOSA_NULL;
                                if (ret == QURL_OK)
                                {
                                    core_state_change(core_ptr, QURL_STATE_CONNECTION);
                                    break;
                                }
                            }
                            else
                            {
                                /*Get information without following*/
                                ret = qurl_trans_follow(core_ptr, new_url, QURL_TRANS_FOLLOWTYPE_FAKE);
                            }
                        qurl_free(new_url);
                        new_url = QOSA_NULL;
                    }

                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                }
                else if (ret != QURL_OK)
                {
                    qurl_loge("faild ret:[%x]\r\n", ret);
                    core_state_change(core_ptr, QURL_STATE_TRANS_DONE);
                    continue_flag = 1;
                }
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
                //Platform differences, cfun0, lwip socket does not necessarily return an error, GET/PUT may block until timeout, used here to determine if the network is normal, advantage: data is fully output before exiting
                if (!qurl_nw_is_activated(core_ptr))
                {
                    ret = QURL_ECODE_NETWORK_ERR;
                    continue_flag = 0;
                }
#endif
                break;
            }
            case QURL_STATE_TRANS_TOOFAST: {
                //Waiting due to speed limit
                break;
            }
            case QURL_STATE_TRANS_DONE: {
                //Data transfer operation completed
                qurl_logd("state:[TRANS_DONE]\r\n");

                /*Exception Monitoring*/
                if (ret != QURL_OK)
                {
                    qurl_conn_core_trans_done(core_ptr->conn_ptr, ret, QOSA_TRUE);
                }
                else if (core_ptr->err_code != QURL_OK)
                {
                    ret = core_ptr->err_code;
                    qurl_conn_core_trans_done(core_ptr->conn_ptr, ret, QOSA_TRUE);
                }
                /**
				 * [lzm][todo] Consider this scenario:
				 * If the service is for reporting variable-length data, and the user-provided callback function intentionally delays time, causing a timeout upon return,
				 * However, due to the variable-length behavior, the user did not report the end of the upload in the callback, but the qurl has already returned to the user side due to a timeout.
				 * We need to consider whether this return value should indicate a timeout or normal completion? Here, we will choose timeout.
				 *
				 * If the service is for reporting data, the user side has already provided all the data, and it has been fully delivered to the server, at which point a timeout occurs (though it is somewhat borderline).
				 * We need to consider whether this return value should indicate a timeout or a normal status. From a business optimization perspective, it might lean toward normal.
				 *
				 * We return according to the actual situation here.
				 */
                // else if(trans_data_done != QOSA_FALSE)
                // {
                ///* Transmission completed, other exceptions and timeouts are not worth mentioning */
                // 	qurl_conn_core_trans_done(core_ptr->conn_ptr, ret, QOSA_TRUE);
                // }
                else if (core_ptr->bits.abort_flag != QOSA_FALSE)
                {
                    ret = QURL_ECODE_ABORT;
                    qurl_conn_core_trans_done(core_ptr->conn_ptr, ret, QOSA_TRUE);
                }
                else if (core_ptr->bits.timeout != QOSA_FALSE)
                {
                    ret = QURL_ECODE_PERFORM_TIMEOUT;
                    qurl_conn_core_trans_done(core_ptr->conn_ptr, ret, QOSA_TRUE);
                }
                else
                {
                    ret = qurl_conn_core_trans_done(core_ptr->conn_ptr, ret, QOSA_FALSE);
                }

                core_state_change(core_ptr, QURL_STATE_COMPLETED);
                continue_flag = 1;
                break;
            }
            case QURL_STATE_COMPLETED: {
                //Operation completed
                /*[lzm][note]Do not modify ret, as it belongs to the ret from the previous state. Unless it is err_code*/
                qurl_logd("state:[COMPLETED]\r\n");

                if (ret == QURL_OK)
                {
                    if (core_ptr->err_code != QURL_OK)
                    {
                        ret = core_ptr->err_code;
                    }
                }

                qurl_trans_completed(core_ptr);
                break;
            }
            case QURL_STATE_MSGSENT: {
                //Operation completed message has been sent
                continue_flag = 0;
                break;
            }
            default: {
                if (core_ptr->err_code != QURL_OK)
                {
                    ret = core_ptr->err_code;
                }
                else if (core_ptr->bits.abort_flag != QOSA_FALSE)
                {
                    ret = QURL_ECODE_ABORT;
                }
                else if (core_ptr->bits.timeout != QOSA_FALSE)
                {
                    ret = QURL_ECODE_PERFORM_TIMEOUT;
                }
                else
                {
                    ret = QURL_ECODE_PARAM_INVALID;
                }
                break;
            }
        }

        /*Watchdog*/
        core_idle_timeout_feed_dog(core_ptr);
    } while (continue_flag);

    return ret;
}

/* API ================================================================= */
qurl_ecode_t qurl_core_create(qurl_core_t *core_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ctrl_ptr = QOSA_NULL;
    int               sec_index = -1;

    if (QOSA_NULL == core_ptr)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    ret = qurl_core_sec_malloc(&sec_index);
    if (ret != QURL_OK)
    {
        return ret;
    }

    core_ctrl_ptr = (qurl_core_ctrl_t *)qurl_malloc(sizeof(qurl_core_ctrl_t));
    if (QOSA_NULL == core_ctrl_ptr)
    {
        qurl_core_sec_free(sec_index);
        return QURL_ECODE_NO_MEMORY;
    }
    qurl_memset(core_ctrl_ptr, 0x00, sizeof(qurl_core_ctrl_t));

    /*Default Configuration*/
    ret = core_default_config(core_ctrl_ptr);
    if (ret != QURL_OK)
    {
        qurl_core_sec_free(sec_index);
        qurl_free(core_ctrl_ptr);
        qurl_logd("create failed:%x\r\n", ret);
        return ret;
    }

    ret = qurl_dbuf_new(&core_ctrl_ptr->send_cache, QURL_DBUF_NO_LIMIT);
    if (ret != QURL_OK)
    {
        goto exit;
    }
    ret = qurl_dbuf_new(&core_ctrl_ptr->recv_cache, QURL_DBUF_NO_LIMIT);
    if (ret != QURL_OK)
    {
        goto exit;
    }

    ret = qurl_event_create(&core_ctrl_ptr->ipc_event, 5);
    if (ret != QURL_OK)
    {
        goto exit;
    }

    /* http */
#if QURL_CFG_ENABLE_HTTP
    qurl_http_multiform_init(&core_ctrl_ptr->usr_cfg.http.multiform_ctrl);
    qurl_http_multiform_set_id_limit(&core_ctrl_ptr->usr_cfg.http.multiform_ctrl, QURL_CFG_MULTIFORM_MAX_ID_DEFAULT); /*Update default form count*/
#endif                                                                                                                /* QURL_CFG_ENABLE_HTTP */
#if QURL_CFG_ENABLE_SMTP
    qurl_smtp_cfg_init(&core_ctrl_ptr->usr_cfg.smtp);
#endif /* QURL_CFG_ENABLE_SMTP */

    /*Created successfully: Legalization flag*/
    core_ctrl_ptr->create_thread_id = qurl_get_thread_id();
    core_ctrl_ptr->owner_thread_id = core_ctrl_ptr->create_thread_id;
    core_ctrl_ptr->sec_index = sec_index;

    core_ctrl_ptr->usr_cfg.thread_id = core_ctrl_ptr->create_thread_id; /*Update some initial configurations*/

    qurl_mutex_lock(g_core_sec[sec_index].lock, QURL_WAIT_FOREVER);
    g_core_sec[sec_index].owner
        = core_ctrl_ptr; /*[lzm][note]This approach is not actually the safest, because two different qurl services on the timeline may use the same actual memory block carrier. Therefore, if time permits later, using a global id++ for identification can resolve both temporal and spatial conflict issues.*/
    qurl_mutex_unlock(g_core_sec[sec_index].lock);

    *core_ptr = (qurl_core_t)core_ctrl_ptr;

    return QURL_OK;

exit:

    if (core_ctrl_ptr->recv_cache)
    {
        qurl_dbuf_delete(core_ctrl_ptr->recv_cache);
    }
    if (core_ctrl_ptr->send_cache)
    {
        qurl_dbuf_delete(core_ctrl_ptr->send_cache);
    }

    qurl_core_sec_free(sec_index);
    qurl_free(core_ctrl_ptr);
    qurl_loge("create failed:%x\r\n", ret);
    return ret;
}

qurl_ecode_t qurl_core_delete(qurl_core_t core)
{
    int               index = 0;
    qurl_core_ctrl_t *core_ctrl_ptr = (qurl_core_ctrl_t *)core;

    if (core_ctrl_ptr == QOSA_NULL || core_ctrl_ptr->sec_index < 0 || core_ctrl_ptr->sec_index >= QURL_CFG_CORE_SERVER_MAX) /*Parameter Validity Check*/
    {
        qurl_loge("delete [0x%p] failed\r\n", core_ctrl_ptr);
        return QURL_ECODE_PARAM_INVALID;
    }
    if (core_ctrl_ptr->usr_cfg.bits.bound_thread && core_ctrl_ptr->owner_thread_id != qurl_get_thread_id()) /*Thread Permission Detection*/
    {
        qurl_loge("perform [0x%p] failed.[%p]!=[%p]\r\n", core_ctrl_ptr, core_ctrl_ptr->owner_thread_id, qurl_get_thread_id());
        return QURL_ECODE_NO_PERMISSION;
    }
    qurl_mutex_lock(g_core_sec[core_ctrl_ptr->sec_index].lock, QURL_WAIT_FOREVER);
    if (g_core_sec[core_ctrl_ptr->sec_index].owner != core_ctrl_ptr) /*Core Legitimacy Check*/
    {
        qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);
        qurl_loge("delete [0x%p] failed\r\n", core_ctrl_ptr);
        return QURL_ECODE_NO_PERMISSION;
    }

    qurl_logd("enter\r\n");

    /*[lzm][note]Unbind or delete*/
    if (core_ctrl_ptr->usr_cfg.bits.reuse_forbid)
    {
        qurl_conn_delete(core_ctrl_ptr->conn_ptr); /*When using long connections or persistent connections, deletion is not required.*/
    }
    else
    {
        // qurl_conn_detach(core_ctrl_ptr->conn_ptr);
    }

    if (core_ctrl_ptr->recv_cache)
    {
        qurl_dbuf_delete(core_ctrl_ptr->recv_cache);
    }
    if (core_ctrl_ptr->send_cache)
    {
        qurl_dbuf_delete(core_ctrl_ptr->send_cache);
    }
    qurl_event_delete(core_ctrl_ptr->ipc_event);

    if (core_ctrl_ptr->cfg.referer_ptr != QOSA_NULL)
    {
        if (core_ctrl_ptr->cfg.bits.referer_alloc_flag)
        {
            qurl_free(core_ctrl_ptr->cfg.referer_ptr);
        }
        core_ctrl_ptr->cfg.referer_ptr = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.new_url != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.new_url);
        core_ctrl_ptr->cfg.new_url = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.first_host != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.first_host);
        core_ctrl_ptr->cfg.first_host = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.url_ptr != QOSA_NULL)
    {
        if (core_ctrl_ptr->cfg.bits.url_alloc_flag)
        {
            qurl_free(core_ctrl_ptr->cfg.url_ptr);
        }
        core_ctrl_ptr->cfg.url_ptr = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.uurl_ptr != QOSA_NULL)
    {
        qurl_url_delete(core_ctrl_ptr->cfg.uurl_ptr);
        core_ctrl_ptr->cfg.uurl_ptr = QOSA_NULL;
    }

    if (core_ctrl_ptr->cfg.scheme != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.scheme);
        core_ctrl_ptr->cfg.scheme = QOSA_NULL;
    }

    if (core_ctrl_ptr->cfg.proxyuserpwd != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.proxyuserpwd);
        core_ctrl_ptr->cfg.proxyuserpwd = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.uagent != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.uagent);
        core_ctrl_ptr->cfg.uagent = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.accept_encoding != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.accept_encoding);
        core_ctrl_ptr->cfg.accept_encoding = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.userpwd != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.userpwd);
        core_ctrl_ptr->cfg.userpwd = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.rangeline != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.rangeline);
        core_ctrl_ptr->cfg.rangeline = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.ref != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.ref);
        core_ctrl_ptr->cfg.ref = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.host != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.host);
        core_ctrl_ptr->cfg.host = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.cookiehost != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.cookiehost);
        core_ctrl_ptr->cfg.cookiehost = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.rtsp_transport != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.rtsp_transport);
        core_ctrl_ptr->cfg.rtsp_transport = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.te != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.te);
        core_ctrl_ptr->cfg.te = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.user != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.user);
        core_ctrl_ptr->cfg.user = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.passwd != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.passwd);
        core_ctrl_ptr->cfg.passwd = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.proxyuser != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.proxyuser);
        core_ctrl_ptr->cfg.proxyuser = QOSA_NULL;
    }
    if (core_ctrl_ptr->cfg.proxypasswd != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->cfg.proxypasswd);
        core_ctrl_ptr->cfg.proxypasswd = QOSA_NULL;
    }
    if (core_ctrl_ptr->info.resp_date != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->info.resp_date);
        core_ctrl_ptr->info.resp_date = QOSA_NULL;
    }

    /*Some files need to be closed after opening.*/
    if (core_ctrl_ptr->cfg.upload_file_fd >= 0)
    {
        qurl_fclose(core_ctrl_ptr->cfg.upload_file_fd);
        core_ctrl_ptr->cfg.upload_file_fd = -1;
    }

    /* http */
#if QURL_CFG_ENABLE_HTTP
    qurl_list_delete(&core_ctrl_ptr->usr_cfg.http.multiform_ctrl.form_list); /*Clear the form linked list*/
    /** Remove some residual response header data*/
    qurl_slist_del_all(core_ctrl_ptr->cfg.http.http_resp_headers_slist);
    core_ctrl_ptr->cfg.http.http_resp_headers_slist = QOSA_NULL;
#endif /* QURL_CFG_ENABLE_HTTP */
#if QURL_CFG_ENABLE_SMTP
#endif /* QURL_CFG_ENABLE_SMTP */

       /* ftp */
#if QURL_CFG_ENABLE_FTP
#endif /* QURL_CFG_ENABLE_FTP */
    qurl_setopt_reset_for_core(core_ctrl_ptr);

    if (core_ctrl_ptr->bits.prot_data_alloc_flag && core_ctrl_ptr->prot_data != QOSA_NULL)
    {
        qurl_free(core_ctrl_ptr->prot_data);
        core_ctrl_ptr->prot_data = QOSA_NULL;
        core_ctrl_ptr->bits.prot_data_alloc_flag = QOSA_FALSE;
    }

    /*Unbind*/
    index = core_ctrl_ptr->sec_index;
    core_ctrl_ptr->owner_thread_id = 0; /*Clear valid tags*/
    qurl_free(core_ctrl_ptr);
    core_ctrl_ptr = QOSA_NULL;

    qurl_mutex_unlock(g_core_sec[index].lock);

    qurl_core_sec_free(index);

    qurl_logd("delete success\r\n");

    return QURL_OK;
}

qurl_ecode_t qurl_core_reset(qurl_core_t core)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ctrl_ptr = (qurl_core_ctrl_t *)core;
    unsigned long     thread_id = 0;
    qurl_bit_t        bound_thread = false;

    if (core_ctrl_ptr == QOSA_NULL || core_ctrl_ptr->sec_index < 0 || core_ctrl_ptr->sec_index >= QURL_CFG_CORE_SERVER_MAX) /*Parameter Validity Check*/
    {
        qurl_loge("setopt [0x%p] failed\r\n", core_ctrl_ptr);
        return QURL_ECODE_PARAM_INVALID;
    }
    if (core_ctrl_ptr->usr_cfg.bits.bound_thread && core_ctrl_ptr->owner_thread_id != qurl_get_thread_id()) /*Thread Permission Detection*/
    {
        qurl_loge("perform [0x%p] failed.[%p]!=[%p]\r\n", core_ctrl_ptr, core_ctrl_ptr->owner_thread_id, qurl_get_thread_id());
        return QURL_ECODE_NO_PERMISSION;
    }
    qurl_mutex_lock(g_core_sec[core_ctrl_ptr->sec_index].lock, QURL_WAIT_FOREVER);
    if (g_core_sec[core_ctrl_ptr->sec_index].owner != core_ctrl_ptr) /*Core Legitimacy Check*/
    {
        qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);
        qurl_loge("setopt [0x%p] failed\r\n", core_ctrl_ptr);
        return QURL_ECODE_NO_PERMISSION;
    }

    /*[lzm][note]Do not reset permissions*/
    bound_thread = core_ctrl_ptr->usr_cfg.bits.bound_thread;
    thread_id = core_ctrl_ptr->usr_cfg.thread_id;

    ret = qurl_setopt_reset_for_core(core_ctrl_ptr);
    if (ret == QURL_OK)
    {
        /*Default Configuration*/
        core_default_usr_cfg(core_ctrl_ptr);
    }
    core_ctrl_ptr->usr_cfg.bits.bound_thread = bound_thread;
    core_ctrl_ptr->usr_cfg.thread_id = thread_id;

    qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);

    return ret;
}

qurl_ecode_t qurl_core_setopt(qurl_core_t core, int opt, ...)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ctrl_ptr = (qurl_core_ctrl_t *)core;
    va_list           arg;

    if (core_ctrl_ptr == QOSA_NULL || core_ctrl_ptr->sec_index < 0 || core_ctrl_ptr->sec_index >= QURL_CFG_CORE_SERVER_MAX) /*Parameter Validity Check*/
    {
        qurl_loge("setopt [0x%p] failed\r\n", core_ctrl_ptr);
        return QURL_ECODE_PARAM_INVALID;
    }
    if (core_ctrl_ptr->usr_cfg.bits.bound_thread && core_ctrl_ptr->owner_thread_id != qurl_get_thread_id()) /*Thread Permission Detection*/
    {
        qurl_loge("perform [0x%p] failed.[%p]!=[%p]\r\n", core_ctrl_ptr, core_ctrl_ptr->owner_thread_id, qurl_get_thread_id());
        return QURL_ECODE_NO_PERMISSION;
    }
    qurl_mutex_lock(g_core_sec[core_ctrl_ptr->sec_index].lock, QURL_WAIT_FOREVER);
    if (g_core_sec[core_ctrl_ptr->sec_index].owner != core_ctrl_ptr) /*Core Legitimacy Check*/
    {
        qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);
        qurl_loge("setopt [0x%p] failed\r\n", core_ctrl_ptr);
        return QURL_ECODE_NO_PERMISSION;
    }

    va_start(arg, opt);
    ret = qurl_setopt_for_core(core_ctrl_ptr, opt, arg);
    va_end(arg);

    qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);

    return ret;
}

qurl_ecode_t qurl_core_getinfo(qurl_core_t core, int opt, ...)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ctrl_ptr = (qurl_core_ctrl_t *)core;
    va_list           arg;

    if (core_ctrl_ptr == QOSA_NULL || core_ctrl_ptr->sec_index < 0 || core_ctrl_ptr->sec_index >= QURL_CFG_CORE_SERVER_MAX) /*Parameter Validity Check*/
    {
        qurl_loge("getinfo [0x%p] failed\r\n", core_ctrl_ptr);
        return QURL_ECODE_PARAM_INVALID;
    }
    if (core_ctrl_ptr->usr_cfg.bits.bound_thread && core_ctrl_ptr->owner_thread_id != qurl_get_thread_id()) /*Thread Permission Detection*/
    {
        qurl_loge("perform [0x%p] failed.[%p]!=[%p]\r\n", core_ctrl_ptr, core_ctrl_ptr->owner_thread_id, qurl_get_thread_id());
        return QURL_ECODE_NO_PERMISSION;
    }
    qurl_mutex_lock(g_core_sec[core_ctrl_ptr->sec_index].lock, QURL_WAIT_FOREVER);
    if (g_core_sec[core_ctrl_ptr->sec_index].owner != core_ctrl_ptr) /*Core Legitimacy Check*/
    {
        qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);
        qurl_loge("getinfo [0x%p] failed\r\n", core_ctrl_ptr);
        return QURL_ECODE_NO_PERMISSION;
    }

    /*[lzm][todo]Skip compilation warnings here for now*/
    (void)opt;
    va_start(arg, opt);
    ret = qurl_getinfo_for_core(core_ctrl_ptr, opt, arg);
    va_end(arg);

    qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);

    return ret;
}

qurl_ecode_t qurl_core_perform(qurl_core_t *core)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ctrl_ptr = (qurl_core_ctrl_t *)core;
    qurl_bit_t        done = QOSA_FALSE;

    if (core_ctrl_ptr == QOSA_NULL || core_ctrl_ptr->sec_index < 0 || core_ctrl_ptr->sec_index >= QURL_CFG_CORE_SERVER_MAX) /*Parameter Validity Check*/
    {
        qurl_loge("perform [0x%p][%d] failed\r\n", core_ctrl_ptr, core_ctrl_ptr ? core_ctrl_ptr->sec_index : -1);
        return QURL_ECODE_PARAM_INVALID;
    }
    if (core_ctrl_ptr->usr_cfg.bits.bound_thread && core_ctrl_ptr->owner_thread_id != qurl_get_thread_id()) /*Thread Permission Detection*/
    {
        qurl_loge("perform [0x%p] failed.[%p]!=[%p]\r\n", core_ctrl_ptr, core_ctrl_ptr->owner_thread_id, qurl_get_thread_id());
        return QURL_ECODE_NO_PERMISSION;
    }
    qurl_mutex_lock(g_core_sec[core_ctrl_ptr->sec_index].lock, QURL_WAIT_FOREVER);
    if (g_core_sec[core_ctrl_ptr->sec_index].owner != core_ctrl_ptr) /*Core Legitimacy Check*/
    {
        qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);
        qurl_loge("perform [0x%p] failed\r\n", core_ctrl_ptr);
        return QURL_ECODE_NO_PERMISSION;
    }

    /*Reset some states*/
    core_state_change(core_ctrl_ptr, QURL_STATE_INIT);
    core_abort_set(core_ctrl_ptr, QOSA_FALSE);
    core_ctrl_ptr->bits.timeout = QOSA_FALSE;

    while (ret == QURL_OK && !done)
    {
        /* 1. poll wait */
        /* [lzm][qurl][todo] */
        ret = core_wait(core_ctrl_ptr);
        if (ret != QURL_OK && core_ctrl_ptr->state <= QURL_STATE_INIT)
        {
            break;
        } /*Other states must be exited through the state machine.*/
        else
        {
            core_ctrl_ptr->err_code = ret; /*Error Code Propagation*/
        }

        /* 2. perform single core */
        ret = core_perform(core_ctrl_ptr);
        if (ret != QURL_OK)
        {
            break;
        }
        if (core_ctrl_ptr->state >= QURL_STATE_COMPLETED)
        {
            done = QOSA_TRUE;
        }
    }
    core_state_change(core_ctrl_ptr, QURL_STATE_INIT); /*In any situation, restore to the initial state [this mechanism can be modified], facilitating the qurl_core_setopt API to handle configurations based on the state.*/
    qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);

    return ret;
}

qurl_ecode_t qurl_core_abort(qurl_core_t *core)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ctrl_ptr = (qurl_core_ctrl_t *)core;

    if (core_ctrl_ptr == QOSA_NULL || core_ctrl_ptr->sec_index < 0 || core_ctrl_ptr->sec_index >= QURL_CFG_CORE_SERVER_MAX) /*Parameter Validity Check*/
    {
        qurl_loge("perform [0x%p][%d] failed\r\n", core_ctrl_ptr, core_ctrl_ptr ? core_ctrl_ptr->sec_index : -1);
        return QURL_ECODE_PARAM_INVALID;
    }
    qurl_mutex_lock(g_core_sec[core_ctrl_ptr->sec_index].lock, QURL_WAIT_FOREVER);
    if (g_core_sec[core_ctrl_ptr->sec_index].owner != core_ctrl_ptr) /*Core Legitimacy Check*/
    {
        qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);
        qurl_loge("perform [0x%p] failed\r\n", core_ctrl_ptr);
        return QURL_ECODE_NO_PERMISSION;
    }

    /*Check status*/
    if (core_ctrl_ptr->state == QURL_STATE_INIT)
    {
        qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);
        return QURL_OK; /*Initially, it is not executing. [lzm][note] Note a limit case: right after entering perform, before the state is updated, at this limit moment it is also not executing, but it will execute next. This is a logical choice, not a technical issue.*/
    }

    core_ctrl_ptr->bits.abort_flag = QOSA_TRUE;

    qurl_mutex_unlock(g_core_sec[core_ctrl_ptr->sec_index].lock);

    return ret;
}

int qurl_core_get_last_close_event(qurl_core_t core)
{
    //socket creation: core_perform => qurl_conn_core_connect => conn_single_connect => conn_socket => qurl_socket_socket => socket/lwip_socket
    //FTP Data Connection Socket Creation:
    //PORT(Active Mode): qurl_conn_listen, qurl_conn_accept, Main listening socket: core_ctrl_ptr->conn_ptr->conn_conn_server.sock_fd
    //PASV (Passive Mode): ftp_state_pasv_resp => qurl_conn_connect => conn_single_connect => conn_socket => qurl_socket_socket => socket/lwip_socket
    //When verifying PASV, you can modify the IP address passed to qurl_socket_resolve in the ftp_state_pasv_resp call.

    //Active mode: The client sends the IP address + port number, and the server connects to the client via the IP address and port number.
    //Passive mode: The server sends an IP address + port number, and the client connects to the server via the IP and port.

    //There are two types of close:

    //(1) TCP connection not yet established, closed during SYN handshake
    //2 CLOSE_EVENT_RST: Server actively sends RST during SYN handshake
    //3 CLOSE_EVENT_SYN_TIMEOUT: TCP connection closed due to internal SYN retransmission timeout

    //(2) TCP connection is already established, closed during DATA transmission process
    //1 CLOSE_EVENT_FIN: TCP connection disconnection event caused by the server actively sending a FIN packet
    //2 CLOSE_EVENT_RST: Server actively sends RST after connection is established.
    //4 CLOSE_EVENT_ACK_TIMEOUT: TCP connection close event caused by internal ACK retransmission timeout

    qurl_core_ctrl_t *core_ctrl_ptr = (qurl_core_ctrl_t *)core;
    int               last_close_event = 0;
    if (QOSA_NULL == core_ctrl_ptr || QOSA_NULL == core_ctrl_ptr->conn_ptr)  //Note activation failure, conn_ptr null pointer dump issue
    {
        return qurl_get_errno();
    }

    //Data connection abnormally closed, causing the CURL control connection to proceed with the shutdown process (the CURL control connection shutdown process is normal). The reason for the abnormal closure of the data connection should be reported.
    if (core_ctrl_ptr->conn_ptr->index1_last_close_event != 0)
    {
        //The data connection has a recorded close event, indicating that the close event was caused by the FTP data connection, returning the reason for the data connection closure.
        //Data connection creation: ftp_state_pasv_resp => qurl_conn_connect => conn_single_connect => conn_socket => qurl_socket_socket => socket/lwip_socket

        //Function for destroying data connection socket:
        //1. SYN failure, connection not formally established: core_perform => qurl_conn_core_connected_check => conn_connected_check => (qurl_socket_weable QURL_SELECT_BIT_ERROR) qurl_socket_close/lwip_close
        //2. TCP connection is established, and closed during data transmission:
        //PORT: ftp_trans_done => qurl_conn_conn_free(conn_ptr, QURL_SOCKET_SERVER_INDEX_0)/Close the main listening socket => qurl_conn_close => conn_close => qurl_socket_close
        //=> qurl_conn_conn_free(conn_ptr, QURL_SOCKET_INDEX_1)/Close sub-socket            => qurl_conn_close => conn_close => qurl_socket_close
        //       PASV: ftp_trans_done => qurl_conn_conn_free(conn_ptr, QURL_SOCKET_INDEX_1) => qurl_conn_close => conn_close => qurl_socket_close

        qurl_logd("index1_last_close_event");
        last_close_event = core_ctrl_ptr->conn_ptr->index1_last_close_event;
    }
    else
    {
        //Closure event of HTTP or FTP control connection
        if (core_ctrl_ptr->conn_ptr->index0_last_close_event != 0)
        {
            //(1): core_perform => qurl_conn_core_connected_check => conn_connected_check => (qurl_socket_weable QURL_SELECT_BIT_ERROR) qurl_socket_close/lwip_close
            //The socket is destroyed in conn_connected_check, and the close event is recorded in conn_ptr->last_close_event.

            //The last_close_event of conn_ptr is not 0, indicating an abnormality has already occurred in conn_connected_check, an exception during the SYN handshake process.
            qurl_logd("index0_last_close_event");
            last_close_event = core_ctrl_ptr->conn_ptr->index0_last_close_event;
        }
        else
        {
            //(2): The TCP connection has been established, and during DATA transmission, the socket will be destroyed in qurl_core_delete (for example, after each HTTP request is executed (whether successful or not), delete will be actively called).
            //qurl_core_delete => qurl_conn_delete => conn_delete => qurl_conn_conn_free => qurl_conn_close => conn_close => qurl_socket_close

            qurl_socket_t sock_fd = core_ctrl_ptr->conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd;
            qurl_logd("sock_fd: %d", sock_fd);
            last_close_event = qurl_socket_get_last_close_event(sock_fd);
        }
    }
    qurl_logd("last_close_event: %d", last_close_event);

    //Before deleting the core (before executing qurl_core_delete), record the saved shutdown events.

    //1 CLOSE_EVENT_FIN: TCP connection disconnection event caused by the server actively sending a FIN packet.
    //2 CLOSE_EVENT_RST: During the SYN handshake, the server actively sends an RST / After the connection is established, the server actively sends an RST.
    //3 CLOSE_EVENT_SYN_TIMEOUT: TCP connection closed due to internal SYN retransmission timeout event
    //4 CLOSE_EVENT_ACK_TIMEOUT: TCP connection disconnection event caused by internal ACK retransmission timeout
    if (last_close_event > 0 && last_close_event < 3)
    {
        //1 2 The close event is caused by the server sending FIN or RST, which is not necessarily a problem and should not be reported.
        //Only report shutdown events caused by the module (3, 4)
        last_close_event = 0;
    }

    return last_close_event;
}
