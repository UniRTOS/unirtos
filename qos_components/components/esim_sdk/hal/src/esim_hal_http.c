/*****************************************************************/ /**
* @file esim_hal_http.h
* @brief
* @author neo.jiang@quectel.com
* @date 2024-10-21
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-10-21 <td>1.0 <td>neo.jiang <td> Init
* </table>
**********************************************************************/
#include "esim_hal_http.h"
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "esim_error.h"
#include "esim_hal_fs.h"

#include "qcm_proj_config.h"
#include "qurl.h"

#ifdef CONFIG_QCM_VTLS_FUNC
#include "qcm_vtls_cfg.h"
#include "qcm_vtls.h"
#endif /* CONFIG_QCM_VTLS_FUNC */

#define ESIM_HTTP_RECV_DATA_LEN_MAX 655360

/** HTTP service processing thread stack size*/
#define ESIM_HTTP_TASK_STACK_SIZE   12 * 1024

/** HTTP Service Thread Priority*/
#define ESIM_HTTP_TASK_PRIORITY     QOSA_PRIORITY_ABOVE_NORMAL

/** Maximum cache count for HTTP service message queue*/
#define ESIM_HTTP_MSGQ_MAX_CNT      10

#define QOS_LOG_TAG                 LOG_TAG_ESIM

//http task handles the business logic of the task thread
static qosa_task_t g_http_stask;

//HTTP Service Thread Message Queue
static qosa_msgq_t g_http_msgq;

/**
 * @brief For managing eSIM HTTP requests
 */
typedef struct
{
    qosa_sem_t          sem;
    qosa_bool_t         http_requst_running; /*!< HTTP request is in progress*/
    int                 upload_len;          /*!< Size of data already uploaded*/
    char               *data_ptr;            /*!< Current HTTP body content; if successful, the memory space is released within the ESIM SDK to save space.*/
    int                 cur_len;             /*!< Current length of the saved HTTP body content*/
    esim_http_request_t config_info;
} esim_http_param_t;

static esim_http_param_t *g_http_client = QOSA_NULL;

static void ql_esim_http_task_process(void *argv);


char esim_http_running_status_get(void)
{
    if (g_http_client != QOSA_NULL)
    {
        return g_http_client->http_requst_running;
    }
    else
    {
        return QOSA_FALSE;
    }
}

int esim_ipa_save_ca_cert_to_file(char *ca_data, qosa_uint32_t ca_len, char *file_path)
{
    qosa_uint32_t ret = 0;
    qosa_uint8_t *read_data = QOSA_NULL;
    qosa_int32_t read_len = 0;
    qosa_int32_t  file_size = 0;
    int           fd = -1;
    int           save_ret = -1;

    QLOGI("ca_len = %d", ca_len);

    if (ca_data == QOSA_NULL || ca_len == 0)
    {
        QLOGE("ca cert is QOSA_NULL");
    }
    else
    {
        ret = ipa_ap_fs_exist(file_path);
        if (ret == 0)
        {
            fd = ipa_ap_fs_open(file_path, ESIM_AP_FS_O_RDWR);
            if (fd == -1)
            {
                QLOGE("open file error\n");
                return -1;
            }
            ret = ipa_ap_fs_lseek(fd, 0, ESIM_AP_FS_SEEK_END);
            if (ret == -1)
            {
                QLOGE("lseek end error\n");
                ipa_ap_fs_close(fd);
                return -1;
            }

            file_size = ipa_ap_fs_ftell(fd);
            if (file_size < 0)
            {
                QLOGE("get file size error");
                ipa_ap_fs_close(fd);
                return -1;
            }

            read_data = qosa_malloc(file_size + 1);
            if (read_data == QOSA_NULL)
            {
                QLOGE("qosa_malloc error");
                ipa_ap_fs_close(fd);
                return -1;
            }
            qosa_memset(read_data, 0, file_size + 1);

            ret = ipa_ap_fs_lseek(fd, 0, ESIM_AP_FS_SEEK_SET);
            if (ret == -1)
            {
                QLOGE("lseek set error\n");
            }
            else
            {
                read_len = ipa_ap_fs_read(fd, read_data, file_size);
                QLOGI("file_size = %d, read_len = %d", file_size, read_len);

                if (read_len > 0 && read_len == ca_len && (!qosa_memcmp(ca_data, read_data, ca_len)))
                {
                    QLOGI("data from tls ca path is same with tls data,no need write again");
                    save_ret = 0;
                }
                else
                {
                    ret = ipa_ap_fs_lseek(fd, 0, ESIM_AP_FS_SEEK_SET);
                    ret = ipa_ap_fs_write(fd, (void *)ca_data, ca_len);
                    if (ret == ca_len)
                    {
                        save_ret = 0;
                    }
                    else
                    {
                        QLOGE("write ca cert error, ca_len = %d, ret = %d", ca_len, ret);
                    }
                }
            }
            qosa_free(read_data);
            ipa_ap_fs_close(fd);
        }
        else
        {
            fd = ipa_ap_fs_open(file_path, ESIM_AP_FS_O_RDWR);
            if (fd == -1)
            {
                QLOGE("open file error\n");
                return -1;
            }
            ret = ipa_ap_fs_write(fd, (void *)ca_data, ca_len);
            if (ret == ca_len)
            {
                save_ret = 0;
            }
            else
            {
                QLOGE("write ca cert error, ca_len = %d, ret = %d", ca_len, ret);
            }
            ipa_ap_fs_close(fd);
        }
    }

    return save_ret;
}

/**
 * @brief Event notification function
 */
static void ql_esim_msgq_report(esim_http_msg_type_e cmd, void *argv)
{
    esim_http_msg_info_t msg_info = {0, QOSA_NULL};

    msg_info.cmd = cmd;
    msg_info.argv = argv;

    qosa_msgq_release(g_http_msgq, sizeof(esim_http_msg_info_t), (qosa_uint8_t *)&msg_info, QOSA_NO_WAIT);
}

void esim_http_process_init(void)
{
    int ret = 0;

    g_http_client = (esim_http_param_t *)qosa_malloc(sizeof(esim_http_param_t));
    if (g_http_client == QOSA_NULL)
    {
        return;
    }

    ret = qosa_sem_create(&g_http_client->sem, 0);
    if (ret != QOSA_OK)
    {
        QLOGE("sem create error!");
        return;
    }

    ret = qosa_msgq_create(&g_http_msgq, sizeof(esim_http_msg_info_t), ESIM_HTTP_MSGQ_MAX_CNT);
    if (ret != QOSA_OK)
    {
        QLOGE("msgq create error!");
        return;
    }

    ret = qosa_task_create(&g_http_stask, ESIM_HTTP_TASK_STACK_SIZE, ESIM_HTTP_TASK_PRIORITY, "esim_http", ql_esim_http_task_process, QOSA_NULL);
    if (ret != QOSA_OK)
    {
        qosa_msgq_delete(g_http_msgq);
        QLOGE("task create error!");
        return;
    }
    QLOGI("esim http init success");
}

void esim_http_process_deinit(void)
{
    if (g_http_client != QOSA_NULL)
    {
        qosa_sem_delete(g_http_client->sem);
        qosa_free(g_http_client);
    }

    g_http_client = QOSA_NULL;

    QLOGI("esim http deinit success");
}

static int ql_esim_http_write_response_cb(unsigned char *buf, long size, void *arg)
{
    esim_http_param_t *http_ptr = QOSA_NULL;

    http_ptr = (esim_http_param_t *)arg;
    if (QOSA_NULL == buf || 0 == size || QOSA_NULL == http_ptr)
    {
        return 0;
    }

    QLOGI("cur_len: %d", http_ptr->cur_len);

    if (0 == http_ptr->cur_len)
    {
        if (http_ptr->data_ptr != QOSA_NULL)
        {
            qosa_free(http_ptr->data_ptr);
            http_ptr->data_ptr = QOSA_NULL;
        }

        http_ptr->data_ptr = (char *)qosa_malloc(size + 1);
        if (QOSA_NULL == http_ptr->data_ptr)
        {
            QLOGE("qosa_malloc error");
            return -1;
        }
        qosa_memset(http_ptr->data_ptr, 0x00, size + 1);
    }
    else
    {
        // cur_len > 0

        char *old_data_ptr = QOSA_NULL;

        if (QOSA_NULL == http_ptr->data_ptr)
        {
            QLOGE("Unknown error");
            return -1;
        }

        old_data_ptr = http_ptr->data_ptr;
        http_ptr->data_ptr = (char *)qosa_realloc(old_data_ptr, http_ptr->cur_len + size + 1);
        if (QOSA_NULL == http_ptr->data_ptr)
        {
            QLOGE("realloc error");
            qosa_free(old_data_ptr);
            old_data_ptr = QOSA_NULL;
            http_ptr->cur_len = 0;

            return -1;
        }
        qosa_memset(http_ptr->data_ptr + http_ptr->cur_len, 0x00, size + 1);
    }

    qosa_memcpy(http_ptr->data_ptr + http_ptr->cur_len, buf, size);
    http_ptr->cur_len += size;

    QLOGI("size: %d", size);

    return size;
}

//HTTP response headers, output line by line (for printing only)
static qosa_int32_t ql_esim_http_write_head_cb(char *buf, long size, void *arg)
{
    char *field = QOSA_NULL;
    field = (char *)qosa_malloc(size + 1);
    if (field != QOSA_NULL)
    {
        qosa_memset(field, 0x00, size + 1);
        qosa_memcpy(field, buf, size);

        QLOGI("size:%d, field:%s", size, field);

        qosa_free(field);
        field = QOSA_NULL;
    }

    return size;
}

static int esim_http_read_request_data(unsigned char *buf, long size, void *arg)
{
    esim_http_param_t *http_ptr = (esim_http_param_t *)arg;
    int                remain_len = 0;  //In the user's complete data, how much data remains to be uploaded
    int                ul_len = 0;      //The actual amount of data uploaded in this callback

    if (QOSA_NULL == buf || 0 == size || QOSA_NULL == http_ptr)
    {
        return 0;
    }

    if (QOSA_NULL == http_ptr->config_info.playload || 0 == http_ptr->config_info.playload_len)
    {
        return 0;
    }

    QLOGI("http_ptr: %p, size: %d, upload_len: %d, playload_len: %d", http_ptr, size, http_ptr->upload_len, http_ptr->config_info.playload_len);

    remain_len = http_ptr->config_info.playload_len - http_ptr->upload_len;

    //Take the minimum value between size and remain_len as the data size uploaded for this callback execution.
    ul_len = remain_len > size ? size : remain_len;

    qosa_memcpy(buf, http_ptr->config_info.playload + http_ptr->upload_len, ul_len);
    http_ptr->upload_len += ul_len;

    if (http_ptr->config_info.playload_len <= http_ptr->upload_len)
    {
        //All user data has been uploaded, the cache will be released here.
        qosa_free(http_ptr->config_info.playload);
        http_ptr->config_info.playload = QOSA_NULL;

        QLOGI("free playload");
    }

    QLOGI("ul_len: %d", ul_len);

    return ul_len;
}

/**
 * @brief HTTP Request Business Layer
 */
static void ql_esim_http_send_start(esim_http_msg_type_e cmd, esim_http_param_t *http_ptr)
{
    int  ret = 0;
    char url_buff[522] = {0};

#ifdef CONFIG_QCM_VTLS_FUNC
    qurl_tls_cfg_t tls_cfg = {0};  //SSL configuration
#endif /* CONFIG_QCM_VTLS_FUNC */

    qurl_core_t qurl = NULL;
    qurl_slist_t qurl_headers = QOSA_NULL;  //User adds header field linked list
    long         resp_code = 0;             //HTTP Response Codes

    http_ptr->data_ptr = QOSA_NULL;
    http_ptr->cur_len = 0;

    QLOGI("*********** ql_esim_http_send_start start ***********");
    qurl_global_init();
    ret = qurl_core_create(&qurl);
    if (ret != QURL_OK)
    {
        QLOGE("create http hd failed %d!", ret);
        goto exit;
    }

    //qurl_core_setopt(qurl, QURL_OPT_SIM_ID, 0);

    qurl_core_setopt(qurl, QURL_OPT_NETWORK_ID, 1);
    QLOGI("https timeout set:%d seconds", http_ptr->config_info.http_max_req_time);
    qurl_core_setopt(qurl, QURL_OPT_TIMEOUT_MS, http_ptr->config_info.http_max_req_time * 1000);

    //Set URL
    qosa_memset(url_buff, 0x00, 522);
    qosa_snprintf(url_buff, 522, "https://%s%s", http_ptr->config_info.domain, http_ptr->config_info.path);
    qurl_core_setopt(qurl, QURL_OPT_URL, (char *)url_buff);
    QLOGI("url_buff:%s", url_buff);

    //Redirection
    qurl_core_setopt(qurl, QURL_OPT_FOLLOWLOCATION, 1L);

    QLOGI("type: %d", http_ptr->config_info.type);
    //esim http only supports get/post, set request type
    if (ESIM_HTTPS_METHOD_GET == http_ptr->config_info.type)
    {
        qurl_core_setopt(qurl, QURL_OPT_HTTP_GET, 1L);
    }
    else
    {
        qurl_core_setopt(qurl, QURL_OPT_HTTP_POST, 1L);
    }

#ifdef CONFIG_QCM_VTLS_FUNC
        qurl_tls_cfg_init(&tls_cfg);
        tls_cfg.bits.sni_enable = QOSA_TRUE;
        tls_cfg.verify = QURL_TLS_VERIFY_NONE;
        qurl_core_setopt(qurl, QURL_OPT_TLS_CFG, &tls_cfg);
#endif /* CONFIG_QCM_VTLS_FUNC */

    qurl_headers = qurl_slist_add_strdup(qurl_headers, http_ptr->config_info.header);
    qurl_core_setopt(qurl, QURL_OPT_HTTP_HEADER, qurl_headers);

    if (ESIM_HTTPS_METHOD_POST == http_ptr->config_info.type)
    {
        //Set the message body to be uploaded for ESIM HTTP POST (HTTP request)
        qurl_core_setopt(qurl, QURL_OPT_UPLOAD_SIZE, http_ptr->config_info.playload_len);  //Set upload data
        qurl_core_setopt(qurl, QURL_OPT_READ_CB, esim_http_read_request_data);
        qurl_core_setopt(qurl, QURL_OPT_READ_CB_ARG, (void *)http_ptr);
    }

    //Set the callback for obtaining response header information (each field of the HTTP response header)
    qurl_core_setopt(qurl, QURL_OPT_WRITE_HEAD_CB, ql_esim_http_write_head_cb);
    qurl_core_setopt(qurl, QURL_OPT_WRITE_HEAD_CB_ARG, (void *)http_ptr);

    //Set the callback for obtaining the response message body (HTTP response message body)
    qurl_core_setopt(qurl, QURL_OPT_WRITE_CB, ql_esim_http_write_response_cb);
    qurl_core_setopt(qurl, QURL_OPT_WRITE_CB_ARG, (void *)http_ptr);

    //Formally initiate the HTTP request
    ret = qurl_core_perform(qurl);

    QLOGI(" http ret %x", ret);

    qurl_core_getinfo(qurl, QURL_INFO_RESP_CODE, &resp_code);
    QLOGI(" http resp_code %d", resp_code);

exit:
#ifdef CONFIG_QAPP_UNIRTOS_SSL_AT_FUNC
    unir_qurl_tls_cfg_free(&tls_cfg);
#endif /* CONFIG_QAPP_UNIRTOS_SSL_AT_FUNC */
    if (qurl_headers != QOSA_NULL)
    {
        qurl_slist_del_all(qurl_headers);
        qurl_headers = QOSA_NULL;
    }

    qurl_core_delete(qurl);
    qurl_global_deinit();

    if (QURL_OK == ret && (200 <= resp_code || resp_code < 300))
    {
        esim_https_resp_t resp = {0};
        resp.body_info.body = http_ptr->data_ptr;
        resp.body_info.body_len = http_ptr->cur_len;
        QLOGI(" http resp_code body_len %d", http_ptr->cur_len);

        resp.result = ESIM_HTTPS_REQUEST_SUCCESS;
        http_ptr->data_ptr = QOSA_NULL;
        http_ptr->cur_len = 0;
        http_ptr->config_info.cb(&resp);
    }
    else
    {
        esim_https_resp_t resp = {0};
        resp.body_info.body = QOSA_NULL;
        resp.body_info.body_len = 0;
        resp.result = ESIM_HTTPS_REQUEST_FAIL;
        http_ptr->config_info.cb(&resp);

        if (http_ptr->data_ptr != QOSA_NULL)
        {
            qosa_free(http_ptr->data_ptr);
        }
    }
    //HTTP request ended, all http_ptr set to null
    http_ptr->http_requst_running = QOSA_FALSE;
    http_ptr->upload_len = 0;
    http_ptr->data_ptr = QOSA_NULL;
    http_ptr->cur_len = 0;


    QLOGI("########## ql_esim_http_send_start end ##########");

    QLOGI("header = %p", http_ptr->config_info.header);
    if (http_ptr->config_info.header != QOSA_NULL)
    {
        qosa_free(http_ptr->config_info.header);
    }

    QLOGI("trustedPK = %p", http_ptr->config_info.trustedPK);
    if (http_ptr->config_info.trustedPK != QOSA_NULL)
    {
        qosa_free(http_ptr->config_info.trustedPK);
    }

    QLOGI("CA_cert = %p", http_ptr->config_info.CA_cert);
    if (http_ptr->config_info.CA_cert != QOSA_NULL)
    {
        qosa_free(http_ptr->config_info.CA_cert);
    }

    QLOGI("playload = %p", http_ptr->config_info.playload);
    if(http_ptr->config_info.playload != QOSA_NULL)
    {
        qosa_free(http_ptr->config_info.playload);
        http_ptr->config_info.playload = QOSA_NULL;
    }

    qosa_memset(&http_ptr->config_info, 0x00, sizeof(esim_http_request_t));

    QLOGI("https end");
}

esim_result_errno_e esim_https_request_to_server(esim_http_request_t *config)
{
    esim_http_param_t *http_ptr = g_http_client;
    if (http_ptr == QOSA_NULL)
    {
        QLOGE("http_ptr QOSA_NULL");
        return ESIM_OPERATION_ERR;
    }
    if (http_ptr->http_requst_running == QOSA_TRUE)
    {
        QLOGE("http_ptr running");
        return ESIM_OPERATION_ERR;
    }
    http_ptr->http_requst_running = QOSA_TRUE;

    qosa_memcpy(&http_ptr->config_info, config, sizeof(esim_http_request_t));

    if (http_ptr->config_info.playload_len != 0 && config->playload != QOSA_NULL)
    {
        http_ptr->config_info.playload = qosa_malloc(http_ptr->config_info.playload_len);
        if (http_ptr->config_info.playload == QOSA_NULL)
        {
            return ESIM_OPERATION_ERR;
        }
        qosa_memcpy(http_ptr->config_info.playload, config->playload, config->playload_len);
    }

    if (http_ptr->config_info.header_len != 0)
    {
        QLOGI("config header:%s", config->header);
        QLOGI("config header_len:%d, http_ptr->config_info.header_len:%d", config->header_len, http_ptr->config_info.header_len);
        http_ptr->config_info.header = qosa_malloc(http_ptr->config_info.header_len + 1);
        if (http_ptr->config_info.header != QOSA_NULL)
        {
            qosa_memcpy(http_ptr->config_info.header, config->header, config->header_len);
        }
        else
        {
            QLOGE("header qosa_malloc fail");
        }
    }

    // This data is currently not in use.
    http_ptr->config_info.trustedPK_len = 0;
    http_ptr->config_info.trustedPK = QOSA_NULL;

    if (http_ptr->config_info.CA_cert_len != 0)
    {
        http_ptr->config_info.CA_cert = qosa_malloc(http_ptr->config_info.CA_cert_len + 1);
        if (http_ptr->config_info.CA_cert != QOSA_NULL)
        {
            qosa_memcpy(http_ptr->config_info.CA_cert, config->CA_cert, config->CA_cert_len);
        }
        else
        {
            QLOGE("CA_cert qosa_malloc fail");
        }
    }

    ql_esim_msgq_report(ESIM_DPP_HTTP_SEND_CMD, http_ptr);
    return ESIM_OK;
}

/**
 * @brief HTTP request internal business task
 */
static void ql_esim_http_task_process(void *argv)
{
    int                  ret = 0;
    esim_http_msg_info_t msg_info = {0, QOSA_NULL};

    QOSA_UNUSED(argv);

    while (1)
    {
        ret = qosa_msgq_wait(g_http_msgq, (qosa_uint8_t *)&msg_info, sizeof(esim_http_msg_info_t), QOSA_WAIT_FOREVER);
        if (ret != QOSA_OK)
        {
            QLOGI("ret = %d", ret);
            qosa_task_sleep_sec(1);
            continue;
        }

        QLOGI("cmd=%d", msg_info.cmd);
        switch (msg_info.cmd)
        {
            case ESIM_DPP_HTTP_SEND_CMD:
            case ESIM_EIM_HTTP_SEND_CMD: {
                ql_esim_http_send_start(msg_info.cmd, msg_info.argv);
            }
            break;
            default:
                break;
        }
    }
}
