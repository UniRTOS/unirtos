/*****************************************************************/ /**
* @file qcm_websocket.c
* @brief
* @author harry.li@quectel.com
* @date 2024-04-25
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-04-25 <td>1.0 <td>harry.Li <td> Init
* </table>
**********************************************************************/

#include "qosa_def.h"
#include "qosa_log.h"
#include "qosa_queue_list.h"
#include "qosa_virtual_file.h"
#include "qosa_asyn_dns.h"
#include "qosa_sys.h"
#include "qcm_socket_adp.h"
#include "qcm_web_ws.h"
#include "qcm_web_http.h"
#include "qcm_web_internal.h"
#include "qcm_web_tcp.h"
#include "qcm_proj_config.h"

/************************************************************************/
#define QOS_LOG_TAG LOG_TAG_WEB_API

#define WEB_FREE_PTR(p)                                                                                                                                        \
    {                                                                                                                                                          \
        if ((p) != QOSA_NULL)                                                                                                                                  \
        {                                                                                                                                                      \
            qosa_free(p);                                                                                                                                      \
            (p) = QOSA_NULL;                                                                                                                                   \
        }                                                                                                                                                      \
    }

static qcm_web_service_t          g_ws_service = {0};
static qcm_web_client_t          *g_ws_client[QCM_WEB_CLIENT_ID_NUM_MAX + 1] = {0}; /*!< All client information*/
static qcm_web_config_internal_t *g_ws_cfg[QCM_WEB_CFG_CONFIG_ID_MAX + 1] = {0};    /*!< Configuration item collection*/

static qcm_web_err_e qcm_ws_header_del(qosa_q_type_t *header_list, qcm_web_header_data_t *data);

/**
 * Initialize "conn/url-ex" configuration
 *
 * Save the URL configuration from the "SFS:conn_url_ex/cfg_id" file to "conn/url-ex"
 * thus achieving the "conn/url-ex" power-off save function
 *
 */
static void qcm_ws_conn_url_ex_init(void)
{
    QOSA_VFS_DIR             *dir = QOSA_NULL;
    struct qosa_vfs_dirent_t *ent = QOSA_NULL;
    struct qosa_vfs_stat_t    st = {0};

    dir = qosa_vfs_opendir(QCM_WEB_CFG_CONN_URL_EX_DIR);
    if (QOSA_NULL == dir)
    {
        return;
    }

    while ((ent = qosa_vfs_readdir(dir)) != QOSA_NULL)
    {
        char file_path[512] = {
            0};  //512 To prevent compilation warnings: qosa_snprintf may truncate when concatenating the two strings QCM_WEB_CFG_CONN_URL_EX_DIR and ent->d_name into a single buffer, as the combined length could exceed the buffer size (256 bytes).
        qosa_int32_t fd = 0;
        int          ret = 0;

        char *url = QOSA_NULL;
        int   url_len = 0;

        QLOGD("d_name: %s", ent->d_name);
        qosa_snprintf(file_path, 512, "%s/%s", QCM_WEB_CFG_CONN_URL_EX_DIR, ent->d_name);
        QLOGD("file_path: %s", file_path);

        fd = qosa_vfs_open(file_path, QOSA_VFS_O_RDONLY);
        if (fd < 0)
        {
            QLOGE("fd error");
            continue;
        }

        if ((int)qosa_vfs_fstat(fd, &st) == 0)
        {
            /*File size*/
            url_len = st.st_size;
        }
        else
        {
            url_len = -1;
        }
        if (url_len < 0)
        {
            QLOGE("fsize error");
            qosa_vfs_close(fd);
            continue;
        }
        QLOGD("url_len: %d", url_len);

        url = (char *)qosa_malloc(url_len + 1);
        if (QOSA_NULL == url)
        {
            QLOGE("out of mem");
            qosa_vfs_close(fd);
            continue;
        }
        qosa_memset(url, 0x00, url_len + 1);

        ret = qosa_vfs_read(fd, (void *)url, url_len);
        if (ret > 0)
        {
            int cfg_id = 0;
            cfg_id = qosa_atoi(ent->d_name);

            QLOGD("cfg_id: %d", cfg_id);
            QLOGD("url: %s", url);

            //Configure the read URL into the cfg
            qcm_ws_cfg_set(cfg_id, QCM_WEB_CFG_CONN_URL, (void *)url, qosa_strlen(url));
            //Configured URL, set to conn/url-ex ENABLE
            qcm_ws_cfg_set(cfg_id, QCM_WEB_CFG_CONN_URL_EX, QOSA_NULL, QCM_WEB_URL_EX_ENABLE);
        }
        else
        {
            QLOGE("fread error");
        }

        qosa_vfs_close(fd);
        WEB_FREE_PTR(url);
    }
}

/************************************************************************/
//Non-public API

/**
 * Initialize WebSocket configuration.
 *
 * This function is used to initialize the WebSocket configuration structure for a specified ID. If the configuration ID exceeds the predefined maximum value, or if the configuration has already been initialized,
 * Otherwise, the function will return directly. Otherwise, the function will allocate memory for this configuration ID and initialize the relevant configuration parameters.
 *
 * @param config_id Configuration ID, used to identify the specific WebSocket configuration.
 * @return If the function executes successfully, return QCM_WEB_OK; if the configuration ID is out of range, return QCM_WEB_ERR_INVAL_PARM;
 *         If memory allocation fails, return QCM_WEB_ERR_NO_MEM.
 */
static qcm_web_err_e qcm_ws_config_init(int config_id)
{
    int               ret = 0;
    qcm_web_config_t *cfg_ptr = QOSA_NULL;

    QLOGD("enter");
    //Check if the configuration ID exceeds the maximum value
    if (config_id < 0 || config_id > QCM_WEB_CFG_CONFIG_ID_MAX)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    //Check if the configuration has been initialized.
    if (g_ws_cfg[config_id] != QOSA_NULL)
    {
        return QCM_WEB_ERR_OK;
    }

    //Allocate memory for the specified configuration ID
    g_ws_cfg[config_id] = qosa_malloc(sizeof(qcm_web_config_internal_t));
    //Check if memory allocation is successful
    if (g_ws_cfg[config_id] == QOSA_NULL)
    {
        return QCM_WEB_ERR_NOMEM;
    }
    qosa_memset(g_ws_cfg[config_id], 0, sizeof(qcm_web_config_internal_t));
    if (g_ws_cfg[config_id]->lock == QOSA_NULL)
    {
        ret = qosa_mutex_create(&g_ws_cfg[config_id]->lock);
        if (ret != QOSA_OK)
        {
            return QCM_WEB_ERROR_UNKNOW;
        }
    }

    qosa_mutex_lock(g_ws_cfg[config_id]->lock, QOSA_WAIT_FOREVER);
    cfg_ptr = &g_ws_cfg[config_id]->config;
    //Initialize the allocated memory and set default parameters.
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    cfg_ptr->conn.pdp_cid = 1;
    cfg_ptr->conn.sim_cid = 0;
#endif
    cfg_ptr->conn.url = QOSA_NULL;
    cfg_ptr->conn.url_len = 0;
    cfg_ptr->conn.url_ex = QCM_WEB_URL_EX_DISABLE;
    cfg_ptr->conn.subprot_en = QCM_WEB_SUBPROT_DISABLE;
    cfg_ptr->conn.subprot = QOSA_NULL;
    cfg_ptr->conn.subprot_len = 0;
    cfg_ptr->conn.extension_en = QCM_WEB_EXTENSION_DISABLE;
    cfg_ptr->conn.extension = QOSA_NULL;
    cfg_ptr->conn.extension_len = 0;
    cfg_ptr->conn.method = QOSA_NULL;
    cfg_ptr->conn.method_len = 0;
    cfg_ptr->conn.ssl_ctx_id = 0;
    cfg_ptr->conn.ssl_config = QOSA_NULL;
    cfg_ptr->conn.timeout = QCM_WEB_CFG_CONN_TIMEOUT_DEFAULT;
    cfg_ptr->ping.interval = QCM_WEB_CFG_PING_INTERVAL_DEFAULT;
    cfg_ptr->read.buffer_sz = QCM_WEB_CFG_READ_BUF_LEN_DEFAULT;
    cfg_ptr->read.timeout = QCM_WEB_CFG_READ_TIMEOUT_DEFAULT;
    cfg_ptr->read.mode = QCM_WEB_MODE_BUFFER;
    cfg_ptr->read.recv_cb = QOSA_NULL;
    cfg_ptr->write.buffer_sz = QCM_WEB_CFG_WRITE_BUF_LEN_DEFAULT;
    cfg_ptr->write.timeout = QCM_WEB_CFG_WRITE_TIMEOUT_DEFAULT;
    cfg_ptr->write.echo = QCM_WEB_ECHO_OFF;
    cfg_ptr->close.wait_time = QCM_WEB_CFG_CLOSE_WAIT_TIMEOUT_DEFAULT;

    qosa_q_init(&cfg_ptr->header_list);

    qcm_ws_conn_url_ex_init();

    qosa_mutex_unlock(g_ws_cfg[config_id]->lock);
    return QCM_WEB_ERR_OK;
}

/**
 * OSA Web Socket Receive Callback Function
 * When a WebSocket message is received, this function is called to notify the corresponding client.
 *
 * @param client_id Client ID, used to identify the client calling the WebSocket service.
 * @param type The message type, indicating the type of the received message.
 * @param result Operation result, indicating the success or failure of the receive operation.
 * @param data The received data, the actual message content.
 * @param len Data length, indicates the length of the received data.
 */
void qcm_ws_recv_cb(int client_id, qcm_web_cb_type_e type, int result, char *data, int len)
{
    int          config_id = 0;
    qcm_web_cb_t cbt = {0};

    //Get the configuration ID corresponding to the client ID
    config_id = g_ws_client[client_id]->config_id;
    //Check if configuration exists
    if (g_ws_cfg[config_id] != QOSA_NULL)
    {
        //Check if a receive callback function exists
        if (g_ws_cfg[config_id]->config.read.recv_cb)
        {
            //Prepare callback information
            cbt.result = result;
            cbt.type = type;
            cbt.data = data;
            cbt.size = len;
            //Call the receive callback function at_web_service_cb/ql_ws_service_cb
            g_ws_cfg[config_id]->config.read.recv_cb(client_id, &cbt);
        }
    }
}

/**
 * @brief Get the WebSocket client pointer for the specified client ID
 *
 * This function is used to find the corresponding WebSocket client pointer from a global client array based on the provided client ID.
 * If the provided client ID is outside the valid range, NULL is returned.
 *
 * @param client_id Client ID, used to identify a specific WebSocket client
 * @return qcm_web_client_t* Pointer to the specified WebSocket client. Returns NULL if the client ID is invalid.
 */
qcm_web_client_t *qcm_ws_client_get_ptr(int client_id)
{
    //Check if the client ID is within the valid range
    if (client_id < 0 || client_id > QCM_WEB_CLIENT_ID_NUM_MAX)
    {
        return QOSA_NULL;           //Client ID is invalid, returns NULL
    }
    return g_ws_client[client_id];  //Returns the WebSocket client pointer corresponding to the client ID.
}

/**
 * @brief Get the network configuration pointer for the specified configuration ID
 *
 * This function is used to retrieve the corresponding network configuration pointer from the global configuration array based on the configuration ID.
 * If the configuration ID is invalid, NULL is returned.
 *
 * @param config_id Configuration ID, used to identify specific network configuration
 * @return qcm_web_config_t* Pointer to the specific network configuration. Returns NULL if the config_id is invalid.
 */
qcm_web_config_internal_t *qcm_ws_config_get_ptr(int config_id)
{
    //Check if the client ID is within the valid range
    if (config_id < 0 || config_id > QCM_WEB_CFG_CONFIG_ID_MAX)
    {
        return QOSA_NULL;
    }
    return g_ws_cfg[config_id];
}

/**
 * Get the WebSocket client status for the specified client ID
 *
 * @param client_id Client ID, used to identify a specific WebSocket client
 * @return Returns the client's status. If the ID is out of range, returns an error code;
 *         If the client is not initialized, return the initialization status;
 *         Otherwise, return the current status of the client.
 */
static int qcm_ws_client_get_status(int client_id)
{
    //Check if the client ID is within the valid range
    if (client_id < 0 || client_id > QCM_WEB_CLIENT_ID_NUM_MAX)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    //Check if the client is initialized.
    if (g_ws_client[client_id] == QOSA_NULL)
    {
        return QCM_WEB_STATUS_INIT;
    }
    //Returns the current status of the client.
    return g_ws_client[client_id]->status;
}

/**
 * Delete all frame information nodes in the s_frame_list linked list
 *
 * @param s_frame_list points to the linked list pointer containing frame information nodes.
 *
 */
static void qcm_ws_client_frame_list_del_all(qosa_q_type_t *s_frame_list)
{
    web_socket_s_frame_t *s_frame_node = QOSA_NULL;

    //Check if the queue has been initialized
    if (qosa_q_is_initialized(s_frame_list) == QOSA_FALSE)
    {
        QLOGE("Header list has not been initialized!");
        return;
    }

    s_frame_node = (web_socket_s_frame_t *)qosa_q_check(s_frame_list);

    while (s_frame_node != QOSA_NULL)
    {
        web_socket_s_frame_t *next_node = QOSA_NULL;

        //First, get the node of the next frame.
        next_node = (web_socket_s_frame_t *)qosa_q_next(s_frame_list, &s_frame_node->list);

        //Delete the current frame node
        WEB_FREE_PTR(s_frame_node);

        //Update the node pointer to the next node
        s_frame_node = next_node;
    }
}

/**
 * Release WebSocket client resources
 * @param client_id Client ID, used to identify the WebSocket client whose resources need to be released.
 * This function iterates to find the client instance corresponding to the given ID, then releases all resources it occupies.
 */
void qcm_ws_client_free(int client_id)
{
    qcm_web_client_t *client_ptr = QOSA_NULL;

    QLOGD("enter");
    //Check if the client ID is within the valid range
    if (client_id < 0 || client_id > QCM_WEB_CLIENT_ID_NUM_MAX)
    {
        return;
    }
    //Attempt to obtain the client pointer based on the client ID
    client_ptr = g_ws_client[client_id];
    //If no client with the corresponding ID is found, return directly.
    if (client_ptr == QOSA_NULL)
    {
        return;
    }
    //Release the initialization state of TX and RX
    qcm_ws_tx_rx_deinit(client_id);
    //Release the client's data buffer
    WEB_FREE_PTR(client_ptr->data);
    //Release the hostname and path of the client URL
    WEB_FREE_PTR(client_ptr->url.host);
    WEB_FREE_PTR(client_ptr->url.path);
    //If the client has set a timer, stop and delete that timer.
    if (client_ptr->timer)
    {
        qosa_timer_stop(client_ptr->timer);
        qosa_timer_delete(client_ptr->timer);
        client_ptr->timer = QOSA_NULL;
    }
    if (client_ptr->ping_timer)
    {
        qosa_timer_stop(client_ptr->ping_timer);
        qosa_timer_delete(client_ptr->ping_timer);
        client_ptr->ping_timer = QOSA_NULL;
    }
    //Release the client's PING data frame resources
    WEB_FREE_PTR(client_ptr->r_frame.ping_data);
    client_ptr->r_frame.ping = QOSA_FALSE;
    client_ptr->r_frame.ping_len = 0;
    WEB_FREE_PTR(client_ptr->s_frame.ping_data);
    client_ptr->s_frame.ping = QOSA_FALSE;
    client_ptr->s_frame.ping_len = 0;

    client_ptr->s_frame.close = QOSA_FALSE;
    client_ptr->r_frame.close = QOSA_FALSE;

    //Delete all unprocessed nodes in the s_frame_list linked list
    qcm_ws_client_frame_list_del_all(&client_ptr->s_frame_list);

    //Release the response header information received by the web client
    WEB_FREE_PTR(client_ptr->web_response_hd);
    client_ptr->web_response_hd_len = 0;

    //Finally release the client instance resources
    //First delete the lock to avoid memory leaks
    qosa_mutex_delete(g_ws_client[client_id]->lock);
    g_ws_client[client_id]->lock = QOSA_NULL;
    //Release the client instance itself
    WEB_FREE_PTR(g_ws_client[client_id]);
}

/**
 * Set WebSocket option to string type
 *
 * This function is used to copy the source string into newly allocated memory and update the destination pointer to point to the newly allocated memory.
 * Note: This function will free the memory previously allocated to dst. Use with caution if dst is statically allocated.
 *
 * @param dst points to the address of the destination string pointer; the function will allocate new memory at this address and store the source string.
 * @param src Pointer to the source string.
 * @param src_len The length of the source string.
 * @param max_len The maximum allowed length of the target string.
 * @return Returns the operation result status code, QCM_WEB_OK for success, and corresponding error codes for various error conditions.
 */
static qcm_web_err_e qcm_ws_setopt_string(char **dst, const char *src, int src_len, int max_len)
{
    //Check if the pointer to the target string pointer is NULL
    if (dst == QOSA_NULL)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }

    //Check if the source string pointer and source string length are valid
    if (src == QOSA_NULL || src_len == 0)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    //Check if the source string length exceeds the maximum allowed length of the target string
    if (src_len > max_len && max_len != 0)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    //Free the memory previously allocated to the target string
    if (*dst != QOSA_NULL)
    {
        WEB_FREE_PTR(*dst);
        *dst = QOSA_NULL;
    }
    //Allocate sufficient memory to store the source string
    *dst = (char *)qosa_malloc(src_len + 1);  //Note to reserve space for a null character in the string.
    if (*dst == QOSA_NULL)
    {
        return QCM_WEB_ERR_NOMEM;
    }
    //Clear newly allocated memory and copy the source string to the destination string
    qosa_memset(*dst, 0x00, src_len + 1);
    qosa_memcpy(*dst, src, src_len);
    return QCM_WEB_ERR_OK;
}

/**
 * Set the integer value of the option
 * @param dst pointer to the integer value to be set
 * @param src The new integer value to set
 * @param min Minimum allowed value
 * @param max allowed maximum value
 * @return If the operation is successful, return QCM_WEB_ERR_OK; if the parameter is invalid or the value is out of range, return QCM_WEB_ERR_INVAL_PARM
 */
static qcm_web_err_e qcm_ws_setopt_int(int *dst, int src, int min, int max)
{
    if (dst == QOSA_NULL)  //Check if the target pointer is NULL
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    if (src < min || src > max)  //Check if the new integer value is within the allowed range
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }

    *dst = src;  //Set the integer value pointed to by the target pointer to the new value.

    return QCM_WEB_ERR_OK;
}

/**
 * @brief Assign the integer value to the variable pointed to by the target pointer.
 *
 * This function is used to assign the source integer value to the variable pointed to by the specified target pointer. If the target pointer is NULL, an error code is returned.
 *
 * @param dst pointer to the target variable to be assigned
 * @param src The source integer value to be assigned to the target variable
 * @return qcm_web_err_e returns QCM_WEB_ERR_OK on success, and returns QCM_WEB_ERR_INVAL_PARM if the target pointer is NULL.
 */
static qcm_web_err_e qcm_ws_getopt_int(int *dst, int src)
{
    if (dst == QOSA_NULL)  //Check if the target pointer is NULL
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }

    *dst = src;  //Assign the source value to the target variable

    return QCM_WEB_ERR_OK;
}

/**
 * @brief WebSocket send low watermark callback function
 * This function is used as a callback handler when the send memory watermark reaches a low level, allowing data writing to continue.
 *
 * @param wm_ptr points to a pointer of the memory watermark type.
 * @param callback_data points to a pointer of the qcm_web_client_t structure, which contains the transmission session information of the Web client.
 */
static void qcm_ws_tx_wm_low_level_cb(qosa_buffer_watermark_t *wm_ptr, void *callback_data)
{
    QOSA_UNUSED(wm_ptr);
    QLOGD("...");
    qosa_bool_t       send_wm_high = QOSA_FALSE;
    qcm_web_client_t *client_ptr = (qcm_web_client_t *)callback_data;  //Convert callback_data to a pointer of type qcm_web_client_t

    send_wm_high = client_ptr->transfer_session.send_wm_high;
    client_ptr->transfer_session.send_wm_high = QOSA_FALSE;  //Mark the high water mark send flag as false, indicating that the upper layer can continue sending data.
    if (send_wm_high == QOSA_TRUE)
    {
        //Send continue write event
        qcm_ws_recv_cb(client_ptr->client_id, QCM_WEB_TYPE_WRITE, 0, QOSA_NULL, 0);
    }
}

/**
 * @brief Callback function when WebSocket sends watermark reaching the high threshold
 *
 * When this function is called, it indicates that the watermark of the send buffer has reached the preset high threshold. The application should use this callback to control data transmission,
 * Avoid sending excessive data that could cause buffer overflow.
 *
 * @param wm_ptr points to a pointer of the memory watermark type.
 * @param callback_data User-provided callback data, typically a pointer to the client instance.
 */
static void qcm_ws_tx_wm_high_level_cb(qosa_buffer_watermark_t *wm_ptr, void *callback_data)
{
    QOSA_UNUSED(wm_ptr);
    QLOGD("...");

    //Get the Web client instance pointer from the callback data
    qcm_web_client_t *client_ptr = (qcm_web_client_t *)callback_data;

    //Set the sending session watermark to high state; when the upper-layer app detects that the buffer is full, it is not allowed to send data further.
    client_ptr->transfer_session.send_wm_high = QOSA_TRUE;
}

/**
 * Callback function when the Websocket send buffer is not empty
 *
 * This function is called to handle the trigger when the WebSocket send buffer transitions from empty to non-empty.
 * It sends a message to the Web client indicating that a TCP write operation can be performed.
 *
 * @param wm_ptr points to a pointer of the memory watermark type. This parameter is not used in this function but may be used in other contexts or extensions.
 * @param callback_data User-defined callback data, typically a pointer to the client instance.
 */
static void qcm_ws_tx_wm_non_empty_cb(qosa_buffer_watermark_t *wm_ptr, void *callback_data)
{
    QOSA_UNUSED(wm_ptr);

    QLOGD("...");
    qcm_web_client_t *client_ptr = (qcm_web_client_t *)callback_data;

    //Check if the client pointer is null; if it is null, return directly.
    if (client_ptr == QOSA_NULL)
    {
        return;
    }

    //When the send buffer is not empty, send a TCP write message.
    qcm_ws_msg_send(QCM_WEB_MSG_TCP_WRITE, client_ptr->client_id, QOSA_NULL);
}

/**
 * @brief WebSocket receive watermark low-level callback function
 *
 * This function is used to handle the event when the WebSocket receive watermark is lowered. It resets the high watermark state of the receive watermark,
 *
 * @param wm_ptr points to a pointer of the memory watermark type. This parameter is not used in this function.
 * @param callback_data Callback data, should point to an instance of type qcm_web_client_t.
 */
static void qcm_ws_rx_wm_low_level_cb(qosa_buffer_watermark_t *wm_ptr, void *callback_data)
{
    QOSA_UNUSED(wm_ptr);
    QLOGD("...");
    //Get the Web client instance pointer from the callback data
    qcm_web_client_t *client_ptr = (qcm_web_client_t *)callback_data;

    //Set the high water mark to FALSE, indicating that data can continue to be read from TCP.
    client_ptr->transfer_session.recv_wm_high = QOSA_FALSE;

    //recv_wm_high is QOSA_TRUE, during this period qcm_ws_socket_read cannot read data, there may be network data that has not been read out
    //Now WM is low, attempting to retrieve data not read by qcm_ws_socket_read when WM is high
    qcm_ws_socket_read(client_ptr->client_id);
    // qcm_ws_socket_read => qcm_socket_read => recv
    //After qcm_socket_read successfully reads data, it will automatically add a write event qcm_socket_add_monitor(sock, OSA_SOCK_MONITOR_RD);
}

/**
 * @brief WebSocket receiver high water mark callback function
 * When this function is called, it indicates that the water level of the receive buffer has reached a high level, and it is necessary to stop receiving data from the network to avoid overflow.
 *
 * @param wm_ptr points to a pointer of the memory watermark marker type. This parameter is not used in this function but may be used in future extensions.
 * @param callback_data points to a pointer of the qcm_web_client_t structure, which contains the context information of the Web client.
 */
static void qcm_ws_rx_wm_high_level_cb(qosa_buffer_watermark_t *wm_ptr, void *callback_data)
{
    QOSA_UNUSED(wm_ptr);
    QLOGD("...");

    qcm_web_client_t *client_ptr = (qcm_web_client_t *)callback_data;
    //Indicates that the receive buffer's water level has reached a high level, and data reception from the network needs to be stopped.
    client_ptr->transfer_session.recv_wm_high = QOSA_TRUE;
}

/**
 * Callback function when WebSocket receives non-empty data
 *
 * @param wm_ptr points to a pointer of memory watermark type, this parameter is not used in this function
 * @param callback_data User-defined callback data, typically a pointer to the web client
 */
static void qcm_ws_rx_wm_non_empty_cb(qosa_buffer_watermark_t *wm_ptr, void *callback_data)
{
    QOSA_UNUSED(wm_ptr);
    QOSA_UNUSED(callback_data);

    QLOGD("...");

    qcm_web_client_t *client_ptr = (qcm_web_client_t *)callback_data;

    //If the client pointer is null, return directly.
    if (client_ptr == QOSA_NULL || client_ptr->status != QCM_WEB_STATUS_WEBSOCKET)
    {
        return;
    }

    //Get the configuration pointer based on the client's configuration ID
    qcm_web_config_t *cfg_ptr = qcm_ws_cfg_get_all(client_ptr->config_id);

    //If the configuration pointer is null, return directly.
    if (cfg_ptr == QOSA_NULL)
    {
        return;
    }

    //In cache mode, report URC after receiving data
    if (cfg_ptr->read.mode == QCM_WEB_MODE_BUFFER)
    {
        qcm_ws_msg_send_2(QCM_WEB_MSG_TCP_RECV_DATA_REPORT, client_ptr->client_id, client_ptr->transfer_session.current_recv_cnt, QOSA_NULL);
    }
}

/**
 * Initialize watermark for managing the cache of WebSocket client send and receive data
 *
 * @param client_id Client ID, used to identify a specific WebSocket client.
 *
 */
void qcm_ws_tx_rx_watermark_init(int client_id)
{
    web_transfer_session *transfer_session = QOSA_NULL;
    qcm_web_client_t     *client_ptr = QOSA_NULL;

    //Get client pointer based on client ID
    client_ptr = qcm_ws_client_get_ptr(client_id);
    //Obtain the transmission session pointer via the client pointer
    transfer_session = &client_ptr->transfer_session;
    qosa_memset(transfer_session, 0, sizeof(web_transfer_session));

    //Initialize the send watermark buffer size
    qosa_buffer_queue_init(&transfer_session->send_wm_ptr, QCM_WEB_WATERMARK_LIMIT, &transfer_session->send_q);
    if (QOSA_FALSE == qosa_buffer_is_wm_empty(&transfer_session->send_wm_ptr))
    {
        qosa_buffer_empty_queue(&transfer_session->send_wm_ptr);
    }

    //Set the various parameters for sending the watermark flag
    transfer_session->send_wm_ptr.current_bytes = 0;
    transfer_session->send_wm_ptr.low_watermark = QCM_WEB_WATERMARK_LOW_LIMIT;
    transfer_session->send_wm_ptr.high_watermark = QCM_WEB_WATERMARK_HIGH_LIMIT;
    transfer_session->send_wm_ptr.wm_low_water_cb = qcm_ws_tx_wm_low_level_cb;
    transfer_session->send_wm_ptr.wm_low_water_argv = (void *)client_ptr;
    transfer_session->send_wm_ptr.wm_high_water_cb = qcm_ws_tx_wm_high_level_cb;
    transfer_session->send_wm_ptr.wm_high_water_argv = (void *)client_ptr;
    transfer_session->send_wm_ptr.wm_in_queue_cb = QOSA_NULL;
    transfer_session->send_wm_ptr.wm_become_empty_cb = QOSA_NULL;
    transfer_session->send_wm_ptr.wm_become_nonempty_cb = qcm_ws_tx_wm_non_empty_cb;
    transfer_session->send_wm_ptr.wm_become_nonempty_argv = (void *)client_ptr;

    //Initialize receive watermark buffer size
    qosa_buffer_queue_init(&transfer_session->recv_wm_ptr, QCM_WEB_WATERMARK_LIMIT, &transfer_session->recv_q);
    if (QOSA_FALSE == qosa_buffer_is_wm_empty(&transfer_session->recv_wm_ptr))
    {
        qosa_buffer_empty_queue(&transfer_session->recv_wm_ptr);
    }

    //Set the various parameters for the receive watermark
    transfer_session->recv_wm_ptr.current_bytes = 0;
    transfer_session->recv_wm_ptr.low_watermark = QCM_WEB_WATERMARK_LOW_LIMIT;
    transfer_session->recv_wm_ptr.high_watermark = QCM_WEB_WATERMARK_HIGH_LIMIT;
    transfer_session->recv_wm_ptr.wm_low_water_cb = qcm_ws_rx_wm_low_level_cb;
    transfer_session->recv_wm_ptr.wm_low_water_argv = (void *)client_ptr;
    transfer_session->recv_wm_ptr.wm_high_water_cb = qcm_ws_rx_wm_high_level_cb;
    transfer_session->recv_wm_ptr.wm_high_water_argv = (void *)client_ptr;
    transfer_session->recv_wm_ptr.wm_in_queue_cb = QOSA_NULL;
    transfer_session->recv_wm_ptr.wm_become_empty_cb = QOSA_NULL;
    transfer_session->recv_wm_ptr.wm_become_nonempty_cb = qcm_ws_rx_wm_non_empty_cb;
    transfer_session->recv_wm_ptr.wm_become_nonempty_argv = (void *)client_ptr;

    //Mark sending and receiving watermark initialization completed
    transfer_session->rw_wm_ready = QOSA_TRUE;
}

/**
 * @brief Initialize WebSocket send and receive resources
 *
 * This function is used to terminate the sending and receiving operations of the WebSocket client and release related resources.
 *
 * @param client_id Client ID, used to identify the WebSocket client whose operation needs to be terminated.
 */
void qcm_ws_tx_rx_deinit(int client_id)
{
    web_transfer_session *transfer_session = QOSA_NULL;
    qcm_web_client_t     *client_ptr = QOSA_NULL;

    //Get client pointer based on client ID
    client_ptr = qcm_ws_client_get_ptr(client_id);
    //Obtain the transmission session pointer via the client pointer
    transfer_session = &client_ptr->transfer_session;

    QLOGD("send_wm_ptr.q_ptr:%p", transfer_session->send_wm_ptr.q_ptr);
    QLOGD("rw_wm_ready:%d", transfer_session->rw_wm_ready);

    //If the send window is not ready, return directly.
    if (transfer_session->rw_wm_ready == QOSA_FALSE)
    {
        return;
    }

    //If there is a send window and the current count is not 0, clear the queue and destroy the send window.
    if (transfer_session->send_wm_ptr.q_ptr != QOSA_NULL)
    {
        if (transfer_session->send_wm_ptr.current_bytes != 0)
        {
            qosa_buffer_empty_queue(&transfer_session->send_wm_ptr);
        }
        qosa_buffer_queue_destroy(&transfer_session->send_wm_ptr);
    }

    //If there is a current send data pointer, release that data pointer.
    if (transfer_session->current_send_data_ptr != QOSA_NULL)
    {
        qosa_buffer_free_packet(&transfer_session->current_send_data_ptr);
        transfer_session->current_send_data_ptr = QOSA_NULL;
    }

    QLOGD("recv_wm_ptr.q_ptr:%p", transfer_session->recv_wm_ptr.q_ptr);

    //If a receive window exists and the current count is not 0, clear the queue and destroy the receive window.
    if (transfer_session->recv_wm_ptr.q_ptr != QOSA_NULL)
    {
        if (transfer_session->recv_wm_ptr.current_bytes != 0)
        {
            qosa_buffer_empty_queue(&transfer_session->recv_wm_ptr);
        }
        qosa_buffer_queue_destroy(&transfer_session->recv_wm_ptr);
    }

    //If there is a current receive data pointer, release that data pointer.
    if (transfer_session->current_recv_data_ptr != QOSA_NULL)
    {
        qosa_buffer_free_packet(&transfer_session->current_recv_data_ptr);
        transfer_session->current_recv_data_ptr = QOSA_NULL;
    }

    //Mark the send and receive windows as not ready.
    transfer_session->rw_wm_ready = QOSA_FALSE;
}

/**
 * Write data to the transmit watermark
 *
 * @param client_id Client ID, used to identify a specific client connection
 * @param data pointer to be written to the watermark queue
 * @param data_len The length of data to be written
 * @return Function execution status code, returns QCM_WEB_ERR_OK if execution is successful, otherwise returns the corresponding error code.
 */
static int qcm_ws_write_data_to_watermark(int client_id, char *data, int data_len)
{
    qcm_web_client_t     *client_ptr = QOSA_NULL;        //Define a pointer to the client structure
    web_transfer_session *transfer_session = QOSA_NULL;  //Define a pointer to the transmission session structure
    qosa_buffer_block_t  *item_ptr = QOSA_NULL;          //Define a pointer to the memory item structure

    //Get client pointer based on client ID
    client_ptr = qcm_ws_client_get_ptr(client_id);
    //Check if the client pointer is null
    if (client_ptr == QOSA_NULL)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    qosa_mutex_lock(client_ptr->lock, QOSA_WAIT_FOREVER);
    //Obtain the transmission session pointer via the client pointer
    transfer_session = &client_ptr->transfer_session;
    //Create a new memory buffer for the data
    item_ptr = qosa_buffer_new_block();
    //Check if the memory buffer pointer is null
    if (item_ptr == QOSA_NULL)
    {
        //If the memory buffer pointer is NULL, print an error message and return the out-of-memory error code.
        QLOGE("nomem");
        client_ptr->result = QCM_WEB_ERR_NOMEM;
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_CLOSING, client_id, QOSA_NULL);
        qosa_mutex_unlock(client_ptr->lock);
        return QCM_WEB_ERR_NOMEM;
    }

    //Push data to the tail of the memory buffer
    qosa_buffer_pushdown_tail(&item_ptr, data, data_len);
    //Add memory buffer items to the send watermark
    qosa_buffer_enqueue(&transfer_session->send_wm_ptr, &item_ptr);
    qosa_mutex_unlock(client_ptr->lock);
    return QCM_WEB_ERR_OK;
}

/**
 * Read data from the watermark
 *
 * This function is used to read data of a specified length from the receive watermark of a designated client.
 *
 * @param client_id Client ID, used to identify the client from which to read data.
 * @param data points to the buffer used to store the read data.
 * @param want_len The expected length of data to read (the maximum length that data is allowed to receive).
 * @param data_len points to an integer pointer used to receive the actual length of the data read.
 * @return Function execution status code. If the execution is successful, returns QCM_WEB_ERR_OK; if the parameters are invalid or an error occurs, returns the corresponding error code.
 */
static int qcm_ws_read_data_from_watermark(int client_id, char *data, int want_len, int *data_len)
{
    qcm_web_client_t     *client_ptr = QOSA_NULL;
    web_transfer_session *transfer_session = QOSA_NULL;

    //Define read length, total length, and pull length
    int read_len = 0;
    int total_len = 0;
    int pullup_len = 0;

    qcm_web_cfg_t cfg = {0};
    int           read_buf_size = QCM_WEB_CFG_READ_BUF_LEN_DEFAULT;
    int           ret = 0;

    QLOGD("enter");
    //Get client pointer
    client_ptr = qcm_ws_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    qosa_mutex_lock(client_ptr->lock, QOSA_WAIT_FOREVER);
    //Obtain the transmission session pointer
    transfer_session = &client_ptr->transfer_session;
    //Check if the client status is WebSocket status
    if (client_ptr->status != QCM_WEB_STATUS_WEBSOCKET)
    {
        qosa_mutex_unlock(client_ptr->lock);
        return QCM_WEB_ERR_INVAL_PARM;
    }

    //Get the read_buf_size set by AT+QWSCFG="read/mode"
    ret = qcm_ws_cfg_get(client_ptr->config_id, QCM_WEB_CFG_READ_BUFFERSZ, &cfg);
    if (ret != QCM_WEB_ERR_OK)
    {
        qosa_mutex_unlock(client_ptr->lock);
        return QCM_WEB_ERR_INVAL_PARM;
    }
    read_buf_size = cfg.int_value;
    QLOGD("read_buf_len: %d", read_buf_size);

    //Adjust the expected read length, which must not exceed the predefined read buffer size at one time.
    if (want_len > read_buf_size)
    {
        want_len = read_buf_size;
    }

    //Loop through the receive queue until enough data is read or no more data is available.
    while (transfer_session->current_recv_data_ptr != QOSA_NULL
           || (transfer_session->current_recv_data_ptr = (qosa_buffer_block_t *)qosa_buffer_dequeue(&transfer_session->recv_wm_ptr)) != QOSA_NULL)
    {
        //Calculate the length to be read this time
        read_len = MIN((want_len - total_len), read_buf_size);
        //Fetch the data into the buffer
        pullup_len = qosa_buffer_pullup(&(transfer_session->current_recv_data_ptr), data + total_len, read_len);
        total_len += pullup_len;

        QLOGD("read_len=%d,pullup_len=%d,total_len=%d", read_len, pullup_len, total_len);
        //If the read length is greater than or equal to the expected length, exit the loop.
        if (total_len >= want_len)
        {
            break;
        }
    }

    //Copy the read data to the user-provided data buffer

    QLOGD("total_len=%d", total_len);
    QLOGD("pullup_len=%d", pullup_len);
    //Set the actual length of data read
    *data_len = total_len;
    qosa_mutex_unlock(client_ptr->lock);
    return QCM_WEB_ERR_OK;
}

/**
 * Add a new custom header to the WebSocket header list.
 *
 * @param header_list points to the header list.
 * @param src Pointer to the header data to be added.
 * @param len The length of the header data.
 * @return If the header is successfully added, return QCM_WEB_ERR_OK; if the parameter is invalid, return QCM_WEB_ERR_INVAL_PARM; if memory allocation fails, return QCM_WEB_ERR_NOMEM.
 */
static qcm_web_err_e qcm_ws_header_add(qosa_q_type_t *header_list, qcm_web_header_data_t *data)
{
    //Check if the input parameters are valid
    if (qosa_q_is_initialized(header_list) == QOSA_FALSE)
    {
        QLOGE("Header list has not been initialized!");
        return QCM_WEB_ERR_INVAL_PARM;
    }

    qcm_web_header_list_t *header_ptr = QOSA_NULL;
    //Dynamically allocate memory for the new header structure
    header_ptr = (qcm_web_header_list_t *)qosa_malloc(sizeof(qcm_web_header_list_t));
    if (QOSA_NULL == header_ptr)
    {
        return QCM_WEB_ERR_NOMEM;
    }
    qosa_memset(header_ptr, 0, sizeof(qcm_web_header_list_t));
    header_ptr->data.key = qosa_malloc(data->key_len + 1);
    if (QOSA_NULL == header_ptr->data.key)
    {
        WEB_FREE_PTR(header_ptr);
        return QCM_WEB_ERR_NOMEM;
    }
    header_ptr->data.value = qosa_malloc(data->value_len + 1);
    if (QOSA_NULL == header_ptr->data.value)
    {
        WEB_FREE_PTR(header_ptr->data.key);
        WEB_FREE_PTR(header_ptr);
        return QCM_WEB_ERR_NOMEM;
    }
    qosa_memset(header_ptr->data.key, 0, data->key_len + 1);
    qosa_memset(header_ptr->data.value, 0, data->value_len + 1);
    qosa_memcpy(header_ptr->data.key, data->key, data->key_len);
    qosa_memcpy(header_ptr->data.value, data->value, data->value_len);
    //Set header length
    header_ptr->data.key_len = data->key_len;
    header_ptr->data.value_len = data->value_len;

    //First, remove identical fields from the linked list (to avoid duplicate addition of fields with the same name)
    qcm_ws_header_del(header_list, data);
    //Delete and then add new fields

    //Add the new header to the linked list
    qosa_q_link((void *)header_ptr, &header_ptr->list);
    qosa_q_put(header_list, &header_ptr->list);

    return QCM_WEB_ERR_OK;
}

/**
 * Remove WebSocket header information
 *
 * Remove the header item that matches the given source string from the specified header list.
 *
 * @param header_list points to the header list.
 * @param src points to the pointer of the header string to be deleted.
 * @param len The length of the header string to be deleted.
 * @return If executed successfully, returns QCM_WEB_ERR_OK; if the parameter is invalid, returns QCM_WEB_ERR_INVAL_PARM.
 */
static qcm_web_err_e qcm_ws_header_del(qosa_q_type_t *header_list, qcm_web_header_data_t *data)
{
    qcm_web_header_list_t *header_ptr = QOSA_NULL;

    //Check if the input parameters are valid
    if (qosa_q_is_initialized(header_list) == QOSA_FALSE)
    {
        QLOGE("Header list has not been initialized!");
        return QCM_WEB_ERR_INVAL_PARM;
    }

    //Traverse the header list to find the header item that matches the given source string.
    header_ptr = (qcm_web_header_list_t *)qosa_q_check(header_list);
    qcm_web_header_list_t *temp_ptr = QOSA_NULL;
    while (header_ptr != QOSA_NULL)
    {
        QLOGD("file_name=%s,%d", header_ptr->data.key, header_ptr->data.key_len);
        temp_ptr = qosa_q_next(header_list, &header_ptr->list);
        //If a matching header entry is found, delete it
        if (data->key_len == header_ptr->data.key_len && 0 == qosa_memcmp(header_ptr->data.key, data->key, data->key_len))
        {
            qosa_q_delete(header_list, &header_ptr->list);
            WEB_FREE_PTR(header_ptr->data.key);
            WEB_FREE_PTR(header_ptr->data.value);
            WEB_FREE_PTR(header_ptr);
            break;
        }
        header_ptr = temp_ptr;
    }
    return QCM_WEB_ERR_OK;
}

/**
 * Delete all elements in the HTTP header list
 *
 * @param header_list points to a pointer of queue type containing HTTP header information.
 * @return Returns the operation result status code. If successful, returns QCM_WEB_ERR_OK; if the parameter is invalid, returns QCM_WEB_ERR_INVAL_PARM.
 */
static qcm_web_err_e qcm_ws_header_del_all(qosa_q_type_t *header_list)
{
    qcm_web_header_list_t *header_ptr = QOSA_NULL;

    //Check if the queue is empty, i.e., whether the head pointer's next_ptr or prev_ptr is NULL
    if (qosa_q_is_initialized(header_list) == QOSA_FALSE)
    {
        QLOGE("Header list has not been initialized!");
        return QCM_WEB_ERR_INVAL_PARM;
    }

    //Get the first element of the queue
    header_ptr = (qcm_web_header_list_t *)qosa_q_check(header_list);
    qcm_web_header_list_t *temp_ptr = QOSA_NULL;
    //Traverse the queue, deleting elements one by one
    while (header_ptr != QOSA_NULL)
    {
        QLOGD("file_name=%s,%d", header_ptr->data.key, header_ptr->data.key_len);

        //Prepare to delete the current element and retrieve the next element in the queue.
        temp_ptr = qosa_q_next(header_list, &header_ptr->list);

        //Remove the current element from the queue
        qosa_q_delete(header_list, &header_ptr->list);

        //Release the data pointer and the pointer to the current element itself.
        WEB_FREE_PTR(header_ptr->data.key);
        WEB_FREE_PTR(header_ptr->data.value);
        WEB_FREE_PTR(header_ptr);

        //Update the pointer to the next element to be processed
        header_ptr = temp_ptr;
    }

    return QCM_WEB_ERR_OK;
}

/**
 * Get the total length of the OSA WebSocket header list
 *
 * @param header_list points to a pointer of the OSA queue type structure, which represents the WebSocket header list
 * @return Returns the total length of the header list; if the input parameters are invalid, returns the QCM_WEB_ERR_INVAL_PARM error code.
 */
int qcm_ws_header_length_get(qosa_q_type_t *header_list)
{
    int                    len = 0;                 //Initialized with length 0
    qcm_web_header_list_t *header_ptr = QOSA_NULL;  //Initialize the head pointer to null

    //Check if the header list is empty, i.e., whether the next_ptr or prev_ptr of the doubly linked list is NULL
    if (qosa_q_is_initialized(header_list) == QOSA_FALSE)
    {
        QLOGE("Header list has not been initialized!");
        return QCM_WEB_ERR_INVAL_PARM;
    }

    //Verify and obtain the starting pointer of the header list
    header_ptr = (qcm_web_header_list_t *)qosa_q_check(header_list);
    qcm_web_header_list_t *temp_ptr = QOSA_NULL;

    //Header field format:
    //Field Name: Field Content\r\n

    //Traverse the header list and calculate the total length
    while (header_ptr != QOSA_NULL)
    {
        QLOGD("file_name=%s,%d", header_ptr->data.key, header_ptr->data.key_len);
        //Update the temporary pointer and calculate the total length
        temp_ptr = qosa_q_next(header_list, &header_ptr->list);
        len += header_ptr->data.key_len;
        len += header_ptr->data.value_len;
        header_ptr = temp_ptr;  //Move to the next header
    }

    return len;  //Returns the total length of the header list
}

/**
 * Send Web Message
 *
 * This function is used to send a Web message with a specified ID and parameters via a message queue.
 *
 * @param msg_id Message ID, used to identify different types of messages.
 * @param param1 The first parameter passed to the message, its specific meaning depends on the message type.
 * @param argv The parameter of the message, pointing to the memory address containing the message content.
 * @return Returns OSA_OK on success, returns QCM_WEB_ERROR_UNKNOW on failure.
 */
int qcm_ws_msg_send(int msg_id, int param1, void *argv)
{
    qcm_web_msg_t msg = {0};  //Initialize the message structure
    int           ret = QOSA_OK;

    //Fill the message structure
    msg.msgid = msg_id;
    msg.param1 = param1;
    msg.argv = argv;

    //Attempt to post a message to the message queue without waiting.
    ret = qosa_msgq_release(g_ws_service.msgq, sizeof(qcm_web_msg_t), (qosa_uint8_t *)&msg, QOSA_NO_WAIT);
    if (ret != QOSA_OK)  //Check the message sending result
    {
        QLOGE("err msg_id=%d", msg_id);
        return QCM_WEB_ERROR_UNKNOW;  //Sending failed, returned unknown error
    }

    return ret;  //Send successful, return result
}

int qcm_ws_msg_send_2(int msg_id, int param1, int param2, void *argv)
{
    qcm_web_msg_t msg = {0};  //Initialize the message structure
    int           ret = QOSA_OK;

    //Fill the message structure
    msg.msgid = msg_id;
    msg.param1 = param1;
    msg.param2 = param2;
    msg.argv = argv;

    //Attempt to post a message to the message queue without waiting.
    ret = qosa_msgq_release(g_ws_service.msgq, sizeof(qcm_web_msg_t), (qosa_uint8_t *)&msg, QOSA_NO_WAIT);
    if (ret != QOSA_OK)  //Check the message sending result
    {
        QLOGE("err msg_id=%d", msg_id);
        return QCM_WEB_ERROR_UNKNOW;  //Sending failed, returned unknown error
    }

    return ret;  //Send successful, return result
}

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
/**
 * @brief Network activation callback function
 *
 * This callback function is invoked when the network activation status changes.
 *
 * @param user_param points to a structure of type qcm_web_param_t, which contains information such as the client ID.
 * @param argv points to a pointer of the qcm_datacall_act_status_t structure, containing network status information.
 */
static void qcm_ws_datacall_cb(void *user_param, qosa_datacall_act_cnf_t *argv)
{
    //Convert user_param to a pointer of type qcm_web_param_t
    qcm_web_param_t *param_ptr = (qcm_web_param_t *)user_param;

    QLOGD("enter=%d,%d,%d", param_ptr->client_id, argv->pdpid, argv->status);

    //Allocate memory to store the status information of data calls.
    param_ptr->data = qosa_malloc(sizeof(qosa_datacall_act_cnf_t));
    if (param_ptr->data == QOSA_NULL)
    {
        QLOGE("err");
        qcm_ws_msg_send(QCM_WEB_MSG_NET_RESPONSE, param_ptr->client_id, param_ptr);//user_param needs to be released
        return;
    }

    qosa_memcpy(param_ptr->data, argv, sizeof(qosa_datacall_act_cnf_t));

    //Send message to notify the client of the data call response
    qcm_ws_msg_send(QCM_WEB_MSG_NET_RESPONSE, param_ptr->client_id, param_ptr);
}

/**
 * @brief Respond to network requests
 *
 * This function handles network requests, activates the PDP context based on the request type, and performs DNS resolution and TCP connection after successful activation.
 *
 * @param msg points to a pointer to the structure containing the request information.
 * @return qcm_web_err_e Function execution status code, indicating whether the operation succeeded or failed.
 */
static qcm_web_err_e qcm_ws_net_response(qcm_web_msg_t *msg)
{
    qcm_web_param_t         *user_ptr = QOSA_NULL;     //User parameter pointer
    qosa_datacall_act_cnf_t *act_ptr = QOSA_NULL;      //Network activation status pointer
    qcm_web_client_t        *client_ptr = QOSA_NULL;   //WebSocket client pointer
    int                      client_id = msg->param1;  //Client ID
    int                      ret = 0;                  //Return Value

    QLOGD("enter");
    if (msg->argv == QOSA_NULL)  //Check if the input parameter is NULL
    {
        QLOGE("1");
        ret = QCM_WEB_ERR_INVAL_PARM;
        goto err;
    }
    user_ptr = (qcm_web_param_t *)msg->argv;
    if (user_ptr->data == QOSA_NULL)  //Check if user data is NULL
    {
        QLOGE("2");
        ret = QCM_WEB_ERR_INVAL_PARM;
        goto err;
    }
    client_ptr = qcm_ws_client_get_ptr(client_id);        //Get client pointer
    QLOGD("client_id=%d", client_id);
    if (client_ptr == QOSA_NULL)  //Check if the client pointer is NULL
    {
        WEB_FREE_PTR(user_ptr->data);  //Release the memory of the activation status pointer
        return QCM_WEB_ERR_INVAL_PARM;
    }

    act_ptr = (qosa_datacall_act_cnf_t *)user_ptr->data;  //Get activation status pointer
    QLOGD("opt=%d,%d", act_ptr->opt, act_ptr->status);
    if (act_ptr->opt == QOSA_PDP_OPT_ACTIVE)              //Check if PDP context is activated
    {
        if (act_ptr->status == QOSA_TRUE)                 //Check if PDP activation is successful
        {
            //PDP activation successful, processing DNS resolution and TCP connection logic
            ret = QCM_WEB_ERR_OK;
            if (act_ptr->ip_info.ip_type == QOSA_PDP_IPV4 || act_ptr->ip_info.ip_type == QOSA_PDP_IPV4V6)
            {
                client_ptr->local_addr.addr.ipv4_addr = act_ptr->ip_info.ipv4_ip.addr.ipv4_addr;
                client_ptr->local_addr.ip_vsn = QOSA_PDP_IPV4;
            }
            else if (act_ptr->ip_info.ip_type == QOSA_PDP_IPV4 || act_ptr->ip_info.ip_type == QOSA_PDP_IPV4V6)
            {
                client_ptr->local_addr.addr.ipv6_addr = act_ptr->ip_info.ipv6_ip.addr.ipv6_addr;
                client_ptr->local_addr.ip_vsn = QOSA_PDP_IPV6;
            }
            else
            {
                ret = QCM_WEB_ERR_INVAL_PARM;
                goto err;
            }

            if (ret == QCM_WEB_ERR_OK)
            {
                int ip_info = 0;
                ip_info = qcm_socket_checkip_is_ip46(client_ptr->url.host);  //Check URL host address type
                QLOGD("ip_info=%d", ip_info);
                if (ip_info == 0)
                {
                    ret = qcm_ws_msg_send(QCM_WEB_MSG_DNS_REQUEST, client_id, QOSA_NULL);  //Send DNS request message
                }
                else if (ip_info == -1)
                {
                    QLOGE("url host check fail");
                    ret = QCM_WEB_ERR_INVAL_PARM;
                    goto err;
                }
                else
                {
                    //Known v4v6 type
                    if (ip_info == QCM_AF_INET6)
                    {
                        client_ptr->ai_family = QCM_AF_INET6;
                        inet_pton(AF_INET6, client_ptr->url.host, &client_ptr->remote_ip.addr.ipv6_addr);
                        client_ptr->remote_ip.ip_vsn = QOSA_PDP_IPV6;
                    }
                    else
                    {
                        client_ptr->ai_family = QCM_AF_INET;
                        inet_pton(AF_INET, client_ptr->url.host, &client_ptr->remote_ip.addr.ipv4_addr);
                        client_ptr->remote_ip.ip_vsn = QOSA_PDP_IPV4;
                    }

                    ret = qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN, client_id, QOSA_NULL);  //Send TCP open request message
                }
            }
        }
        else
        {
            client_ptr->result = QCM_WEB_ERR_NET_FAIL;
            ret = qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, client_id, QOSA_NULL);  //Send TCP open request message
        }
        WEB_FREE_PTR(act_ptr);                                                      //Release the memory of the activation status pointer

        if (ret != QCM_WEB_ERR_OK)                                                  //If an error occurs, jump to the error handling logic.
        {
            goto err;
        }
    }

    return ret;
err:
    QLOGE("err");
    WEB_FREE_PTR(act_ptr);  //Release the memory of the activation status pointer

    client_ptr->result = ret;
    qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, client_id, QOSA_NULL);
    return ret;
}

/**
 * @brief The function that initiates a request to the network.
 *
 * @param msg points to a pointer to a message structure containing client information and request parameters.
 *
 * @return Returns the execution result status code, OK indicates success, others indicate an error.
 */
static qcm_web_err_e qcm_ws_net_request(qcm_web_msg_t *msg)
{
    //Define required parameters and variables
    qcm_web_param_t        *user_ptr = QOSA_NULL;
    qcm_web_client_t       *client_ptr = QOSA_NULL;
    qcm_web_config_t       *cfg_ptr = QOSA_NULL;
    qosa_datacall_ip_info_t info = {0};
    qosa_datacall_conn_t    conn = 0;
    qosa_datacall_act_cnf_t act_ptr = {0};
    qosa_uint32_t           sim_id = 0;
    qosa_uint32_t           pdp_id = 0;
    int                     client_id = 0;
    int                     ret = 0;

    QLOGD("enter");
    //Get client ID
    client_id = msg->param1;
    client_ptr = qcm_ws_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return QCM_WEB_ERROR_UNKNOW;//Client information is invalid, exiting directly.
    }
    QLOGD("client_id=%d,config_id=%d", client_id, client_ptr->config_id);
    //Retrieve SIM card ID, PDP context ID, and timeout from the configuration
    cfg_ptr = qcm_ws_cfg_get_all(client_ptr->config_id);
    if (cfg_ptr == QOSA_NULL)
    {
        ret = QCM_WEB_ERROR_UNKNOW;
        goto err;
    }

    sim_id = cfg_ptr->conn.sim_cid;
    pdp_id = cfg_ptr->conn.pdp_cid;

    conn = qosa_datacall_conn_new(sim_id, pdp_id, QOSA_DATACALL_CONN_TCPIP);
    if (QOSA_DATACALL_ERR_NO_ACTIVE == qosa_datacall_get_ip_info(conn, &info))
    {
        user_ptr = qosa_malloc(sizeof(qcm_web_param_t));
        if (user_ptr == QOSA_NULL)
        {
            ret = QCM_WEB_ERR_NOMEM;
            goto err;  //Memory allocation failed, jump to error handling
        }

        user_ptr->client_id = client_id;
        user_ptr->config_id = client_ptr->config_id;

        QLOGD("sim_id=%d,pdp_id=%d", sim_id, pdp_id);
        //If the PDP is not activated, start the asynchronous activation process.
        ret = qosa_datacall_start_async(conn, QCM_WEB_PDP_ACTIVE_TIMEOUT, (datacall_callback_cb_ptr)qcm_ws_datacall_cb, user_ptr);
        if (QOSA_DATACALL_OK != ret)
        {
            QLOGE("datacall ret=%x", ret);
            ret = QCM_WEB_ERR_NET_FAIL;
            goto err;  //Network activation failed, jump to error handling
        }
    }
    else
    {
        user_ptr = qosa_malloc(sizeof(qcm_web_param_t));
        if (user_ptr == QOSA_NULL)
        {
            ret = QCM_WEB_ERR_NOMEM;
            goto err;  //Memory allocation failed, jump to error handling
        }
        user_ptr->client_id = client_id;
        user_ptr->config_id = client_ptr->config_id;
        user_ptr->data = qosa_malloc(sizeof(qosa_datacall_act_cnf_t));
        if (user_ptr->data == QOSA_NULL)
        {
            ret = QCM_WEB_ERR_NOMEM;
            goto err;  //Memory allocation failed, jump to error handling
        }
        act_ptr.opt = QOSA_PDP_OPT_ACTIVE;
        //act_ptr.simid = sim_id;
        act_ptr.pdpid = pdp_id;
        act_ptr.status = QOSA_TRUE;
        qosa_memset(&act_ptr.ip_info, 0, sizeof(qosa_datacall_ip_info_t));
        qosa_memcpy(&act_ptr.ip_info, &info, sizeof(qosa_datacall_ip_info_t));
        qosa_memcpy(user_ptr->data, &act_ptr, sizeof(qosa_datacall_act_cnf_t));
        qcm_ws_msg_send(QCM_WEB_MSG_NET_RESPONSE, client_id, user_ptr);
    }

    QLOGD("datacall ret=%x", ret);
    return QCM_WEB_ERR_OK;  //Network request completed successfully

err:
    //Release the memory of the user parameter structure
    WEB_FREE_PTR(user_ptr);
    client_ptr->result = ret;
    qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, client_id, QOSA_NULL);
    return ret;
}
#endif
/**
 * @brief OSA Web Client DNS Query Result Callback Function
 *
 * This function is used to process the results of DNS query requests initiated by the web client. It will act according to the status of the DNS query,
 * Correspondingly process the result and send it back to the Web client.
 *
 * @param argv is a user-defined parameter used to pass to the callback function. In this example, it is a pointer to a qcm_web_param_t structure.
 * @param info A double pointer to a structure of type osoa_addrinfo_s. Upon success, it points to the resolved address information.
 * @param status DNS query result status, indicating whether the query was successful.
 */
static void qcm_ws_client_dns_result_cb(void *argv, struct qosa_addrinfo_s **info, qosa_dns_error_e status)
{
    qcm_web_param_t  *param_ptr = QOSA_NULL;
    qcm_web_client_t *client_ptr = QOSA_NULL;

    QLOGD("enter");  //Print upon entering the function, used for debugging.
    param_ptr = (qcm_web_param_t *)argv;

    //Check if the DNS query is successful.
    if (status != QOSA_DNS_RESULT_OK)
    {
        QLOGE("DNS request fail: %d", status);
        goto err;
    }

    //!!!
    //Check the DNS status again to determine if param_ptr->data needs to be set.
    if (status != QOSA_DNS_RESULT_OK)
    {
        param_ptr->data = QOSA_NULL;
    }
    else
    {
        param_ptr->data = *info;
    }

    //Send the DNS response message back to the Web client.
    qcm_ws_msg_send(QCM_WEB_MSG_DNS_RESPONSE, param_ptr->client_id, param_ptr);
    return;

err:  //Error handling label.
    client_ptr = qcm_ws_client_get_ptr(param_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        return;
    }
    client_ptr->result = QCM_WEB_ERR_DNS_FAIL;
    qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, param_ptr->client_id, QOSA_NULL);
    return;
}

/**
 * @brief Send a request to the DNS server to resolve the hostname of the given URL.
 *
 * @param msg points to a message structure containing the client ID and other possible parameters.
 * @return qcm_web_err_e returns the operation result status code for the DNS request.
 *         Returns QCM_WEB_ERR_SUCCESS on success, and QCM_WEB_ERR_DNS_FAIL on failure.
 */
static qcm_web_err_e qcm_ws_dns_request(qcm_web_msg_t *msg)
{
    qcm_web_param_t       *param_ptr = QOSA_NULL;   //Pointer to the parameter structure used to store DNS request results
    qcm_web_client_t      *client_ptr = QOSA_NULL;  //Pointer to the Web client
    int                    ret = 0;                 //Variable for storing function call return values
    struct qosa_addrinfo_s addrinfo_list = {0};     //Structure for storing parsed address information
    int                    client_id = 0;           //Client ID

    QLOGD("enter");
    client_id = msg->param1;                        //Extract client ID from the message
    client_ptr = qcm_ws_client_get_ptr(client_id);  //Get client pointer based on client ID

    if (client_ptr == QOSA_NULL)
    {
        return ret;
    }
    //Allocate memory to store parameters for DNS requests
    param_ptr = qosa_malloc(sizeof(qcm_web_param_t));
    if (param_ptr == QOSA_NULL)
    {
        ret = QCM_WEB_ERR_NOMEM;
        //Memory allocation failed
        client_ptr->result = ret;
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, client_id, QOSA_NULL);
        return ret;
    }

    param_ptr->client_id = client_id;  //Set client ID
    //Asynchronously obtain address information
    ret = qosa_dns_asyn_getaddrinfo(
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
        g_ws_cfg[client_id]->config.conn.sim_cid,
        g_ws_cfg[client_id]->config.conn.pdp_cid,
#endif
        client_ptr->url.host,
        &addrinfo_list,
        qcm_ws_client_dns_result_cb,
        (void *)param_ptr
    );

    if (ret != QOSA_DNS_RESULT_OK)
    {
        //If the DNS request fails, set the return value to QCM_WEB_ERR_DNS_FAIL
        ret = QCM_WEB_ERR_DNS_FAIL;
        client_ptr->result = ret;
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, client_id, QOSA_NULL);
        return ret;
    }
    QLOGD("dns ret=%d", ret);  //Print the return value of the DNS request
    return ret;
}

/**
 * Process DNS responses for WebSocket clients.
 *
 * @param msg points to a pointer to a qcm_web_msg_t structure containing the client request information.
 * @return Returns QCM_WEB_ERR_OK on success, error codes indicate failure.
 */
static qcm_web_err_e qcm_ws_dns_response(qcm_web_msg_t *msg)
{
    qcm_web_param_t        *param_ptr = QOSA_NULL;   //Pointer for storing web parameters
    qcm_web_client_t       *client_ptr = QOSA_NULL;  //Pointer to the WebSocket client
    struct qosa_addrinfo_s *info_ptr = QOSA_NULL;    //Pointer to store DNS resolution results
    int                     client_id = 0;           //Client ID
    int                     ret = 0;

    QLOGD("enter");
    client_id = msg->param1;                        //Get client ID
    client_ptr = qcm_ws_client_get_ptr(client_id);  //Get client pointer based on client ID

    if (client_ptr == QOSA_NULL)
    {
        return ret;
    }
    if (msg->argv == QOSA_NULL)
    {
        ret = QCM_WEB_ERR_DNS_FAIL;
        client_ptr->result = ret;
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, client_id, QOSA_NULL);
        return ret;
    }
    param_ptr = (qcm_web_param_t *)msg->argv;  //Get parameter pointer
    if (param_ptr->data == QOSA_NULL)
    {
        //Handling of DNS Resolution Failures
        ret = QCM_WEB_ERR_DNS_FAIL;
        client_ptr->result = ret;
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, client_id, QOSA_NULL);
        return ret;
    }
    else
    {
        //DNS resolution successful, processing the resolution result
        info_ptr = (struct qosa_addrinfo_s *)param_ptr->data;
        QLOGD("ip_addr:%s", info_ptr->ip_addr);
        if (info_ptr->ai_family == AF_INET)
        {
            //Processing IPv4 Addresses
            inet_pton(AF_INET, info_ptr->ip_addr, &client_ptr->remote_ip.addr.ipv4_addr);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
            client_ptr->remote_ip.ip_vsn = QOSA_PDP_IPV4;
#else
            client_ptr->remote_ip.ip_vsn = QOSA_IP_V4;
#endif
            client_ptr->ai_family = AF_INET;
        }
        else
        {
            //Processing IPv6 Addresses
            inet_pton(AF_INET6, info_ptr->ip_addr, &client_ptr->remote_ip.addr.ipv6_addr);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
            client_ptr->remote_ip.ip_vsn = QOSA_PDP_IPV6;
#else
            client_ptr->remote_ip.ip_vsn = QOSA_IP_V6;
#endif
            client_ptr->ai_family = AF_INET6;
        }
        //Release DNS address memory
        qosa_dns_result_free(info_ptr);
        //Send a message to open a TCP connection
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN, client_id, QOSA_NULL);
    }
    return QCM_WEB_ERR_OK;
}

/**
 * Read WebSocket data and report it
 * This function reads data from the specified WebSocket client, where the data has already been decoded according to the WebSocket protocol before being stored in the watermark.
 *
 * @param msg points to a pointer to a structure containing WebSocket message parameters.
 *
 * Parameter Description:
 * msg->param1: The ID of the WebSocket client.
 *
 * This function does not return any value.
 */
static int qcm_ws_read(int client_id, char *data, int *data_len)
{
    int ret = 0;
    int want_len = *data_len;

    qosa_memset(data, 0, want_len);

    //Read data from watermark into the data buffer
    ret = qcm_ws_read_data_from_watermark(client_id, data, want_len, data_len);
    QLOGD("want_len=%d,data_len=%d", want_len, *data_len);
    return ret;
}

/**
 * @brief Send WebSocket data
 * This function is used to encode WebSocket messages and write them into the watermark.
 *
 * @param msg points to a pointer to a structure containing the message content and parameters.
 */
static void qcm_ws_write(qcm_web_msg_t *msg)
{
    int               client_id = 0;           //Client ID
    qcm_web_client_t *client_ptr = QOSA_NULL;  //Client pointer
    qcm_web_data_t   *data_ptr = QOSA_NULL;    //Data pointer
    char             *dst = QOSA_NULL;         //Temporary storage area for encoded data
    int               dst_len = 0;             //Length of encoded data
    int               ret = 0;

    //Get the client ID and attempt to retrieve the corresponding client pointer.
    client_id = msg->param1;
    client_ptr = qcm_ws_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL || QOSA_NULL == msg->argv)
    {
        return;  //If the client cannot be found, return directly.
    }

    //Verify the validity of the incoming data and allocate memory to store the encoded data.
    data_ptr = (qcm_web_data_t *)msg->argv;
    if (data_ptr != QOSA_NULL && data_ptr->data_len != 0 && data_ptr->data != QOSA_NULL)
    {
        dst = qosa_malloc(data_ptr->data_len + 32);  //Allocate memory for storing encoded data, the WebSocket protocol header is up to 14 bytes long.
        if (dst == QOSA_NULL)
        {
            WEB_FREE_PTR(data_ptr->data);  //Memory allocation failed, release the memory of the passed data and return
            client_ptr->result = QCM_WEB_ERR_NOMEM;
            qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, client_id, QOSA_NULL);
            return;
        }
    }

    QLOGD("data_len=%d,%s", data_ptr->data_len, data_ptr->data);

    //Construct WebSocket frames
    qcm_ws_construct_frame(client_id, data_ptr->opcode, data_ptr->data, data_ptr->data_len, dst, &dst_len);
    QLOGD("dst_len=%d", dst_len);

    //Write the constructed WebSocket frame to the watermark
    ret = qcm_ws_write_data_to_watermark(client_id, dst, dst_len);
    QLOGD("ret=%d", ret);
    WEB_FREE_PTR(data_ptr->data);
    WEB_FREE_PTR(dst);  //WebSocket frame has been written to watermark, release the data frame to avoid memory leaks
}

/**
 * Send a message to the network.
 *
 * @param msg points to a structure of type qcm_web_msg_t containing the parameters of the message to be sent.
 *        The param1 field is used to specify the client ID for receiving messages.
 * @note The function internally calls the qcm_ws_socket_send function to actually send data.
 * @return This function has no return value.
 */
static void qcm_ws_socket_write(qcm_web_msg_t *msg)
{
    int client_id = 0;

    //Get the client ID from the message parameters
    client_id = msg->param1;
    qcm_ws_socket_send(client_id);
}

/**
 * @brief WebSocket PING Heartbeat Timer Callback Function
 *
 * This function is used as the heartbeat maintenance for the WebSocket client and is called when the timer expires.
 * It will send a PING request to the server to keep the connection alive.
 *
 * @param The arg parameter points to a structure of type qcm_web_client_t, containing information about the WebSocket client.
 */
static void qcm_ws_ping_timer_cb(void *arg)
{
    qcm_web_client_t *client_ptr = QOSA_NULL;

    //Attempt to convert and obtain the WebSocket client pointer from the arg parameter
    client_ptr = (qcm_web_client_t *)arg;
    //If the conversion fails (i.e., the parameter is NULL), return directly without performing subsequent operations.
    if (client_ptr == QOSA_NULL)
    {
        return;
    }
    //Send a PING request message to the server, using the client's ID and empty parameters.
    qcm_ws_msg_send(QCM_WEB_MSG_PING_REQUEST, client_ptr->client_id, QOSA_NULL);
}

/**
 * Send a ping request to the specified WebSocket client.
 *
 * @param msg is a pointer to the message structure containing information such as the client ID.
 *
 * This function first obtains the client pointer through the client ID, then retrieves the ping interval based on the configuration,
 * If the time setting is reasonable, create or reset the ping timer and send a ping frame to the client.
 */
static void qcm_ws_ping_request(qcm_web_msg_t *msg)
{
    int               client_id = 0;
    qcm_web_client_t *client_ptr = QOSA_NULL;
    qcm_web_config_t *cfg_ptr = QOSA_NULL;
    char              data[32] = {0};
    int               data_len = 0;
    int               timeout = 0;
    int               first_req = 0;
    int               ret = 0;

    QLOGD("enter");
    client_id = msg->param1;
    //Get client pointer based on client ID
    client_ptr = qcm_ws_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        //If the client cannot be found, return directly.
        return;
    }
    //Get ping interval configuration
    cfg_ptr = qcm_ws_cfg_get_all(client_ptr->config_id);
    if (cfg_ptr == QOSA_NULL)
    {
        return;
    }
    timeout = cfg_ptr->ping.interval;
    //Check if the ping interval is within a reasonable range
    if (timeout < QCM_WEB_CFG_PING_INTERVAL_MIN || timeout > QCM_WEB_CFG_PING_INTERVAL_MAX || timeout == 0)
    {
        QLOGE("timeout=%d", timeout);
        return;
    }
    //If the client's ping timer is not created, then create it.
    if (client_ptr->ping_timer == QOSA_NULL)
    {
        ret = qosa_timer_create(&client_ptr->ping_timer, qcm_ws_ping_timer_cb, client_ptr);
        if (ret != 0)
        {
            return;
        }
        first_req = 1;
    }

    //Stop and restart the ping timer
    qosa_timer_stop(client_ptr->ping_timer);
    qosa_timer_start(client_ptr->ping_timer, timeout * 1000, 0);
    //When the WebSocket connection is successful, it will check the timer and start it, so do not send during the first creation.
    if (!first_req)
    {
        //Construct ping frame and send
        qcm_ws_construct_frame(client_id, OPCODE_PING, QOSA_NULL, 0, data, &data_len);
        qcm_ws_write_data_to_watermark(client_id, data, data_len);
    }
}

/**
 * Send PONG response request
 *
 * This function is used to handle the PONG response request from a WebSocket client. When a PING frame is received from the client, the server will respond with a PONG frame.
 * This function creates a PONG frame and sends it back to the client.
 *
 * @param msg points to a message structure containing the client ID and other relevant information.
 */
static void qcm_ws_pong_request(qcm_web_msg_t *msg)
{
    int               client_id = 0;           //Client ID
    qcm_web_client_t *client_ptr = QOSA_NULL;  //Client pointer
    char             *data = QOSA_NULL;        //Data buffer used for constructing PONG frames
    int               data_len = 0;            //Data Length

    QLOGD("enter");

    //Get client ID based on the message
    client_id = msg->param1;

    //Attempt to obtain the pointer to the corresponding client.
    client_ptr = qcm_ws_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        //If the client does not exist, return directly.
        return;
    }

    //Allocate memory to construct a PONG frame
    data = qosa_malloc(client_ptr->r_frame.ping_len + 32);
    if (data == QOSA_NULL)
    {
        //If memory allocation fails, return directly.
        return;
    }

    //Construct PONG frame and write data
    qcm_ws_construct_frame(client_id, OPCODE_PONG, client_ptr->r_frame.ping_data, client_ptr->r_frame.ping_len, data, &data_len);
    qcm_ws_write_data_to_watermark(client_id, data, data_len);

    //The content of the PING frame has been copied to the PONG frame, and the content of the PING frame can be deleted.
    WEB_FREE_PTR(client_ptr->r_frame.ping_data);
    client_ptr->r_frame.ping = QOSA_FALSE;  //PONG message request has been sent. The ping frame flag is set to QOSA_FALSE, waiting to see if the next set of data contains a ping frame.
    client_ptr->r_frame.ping_len = 0;

    //Release the memory used for constructing PONG frames.
    WEB_FREE_PTR(data);
}

static void qcm_ws_tcp_recv_report(qcm_web_msg_t *msg)
{
    int               client_id = 0;           //Client ID
    int               size = 0;
    qcm_web_client_t *client_ptr = QOSA_NULL;  //Client pointer

    //Get client ID based on the message
    client_id = msg->param1;
    size = msg->param2;

    //Attempt to obtain the pointer to the corresponding client
    client_ptr = qcm_ws_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        //If the client does not exist, return directly.
        return;
    }

    qcm_ws_recv_cb(client_id, QCM_WEB_TYPE_RECV, client_ptr->result, QOSA_NULL, size);
}

/**
 * @brief OSA WebSocket Client Thread Function
 *
 * This function is an infinite loop used to handle various messages from the WebSocket client.
 * It retrieves messages from the message queue and then calls the corresponding handler function based on the message type.
 *
 * @param argv parameter passing, not used in this function
 */
static void qcm_ws_client_thread(void *argv)
{
    qcm_web_msg_t msg = {0};  //Initialize the message structure
    int           err = QOSA_OK;

    QOSA_UNUSED(argv);
    while (1)                                            //Infinite loop to handle client messages
    {
        qosa_memset(&msg, 0x00, sizeof(qcm_web_msg_t));  //Clear the message structure
        err = qosa_msgq_wait(g_ws_service.msgq, (qosa_uint8_t *)&msg, sizeof(qcm_web_msg_t), QOSA_WAIT_FOREVER);
        if (err != QOSA_OK)                              //If message retrieval fails, continue the loop.
        {
            continue;
        }

        QLOGD("enter=%d", msg.msgid);                           //Print message ID, used for debugging
        qosa_mutex_lock(g_ws_service.lock, QOSA_WAIT_FOREVER);  //Acquire mutex
        switch (msg.msgid)                                      //Call the corresponding handler function based on the message ID
        {
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
            case QCM_WEB_MSG_NET_REQUEST:
                qcm_ws_net_request(&msg);
                break;
            case QCM_WEB_MSG_NET_RESPONSE:
                qcm_ws_net_response(&msg);
                break;
#endif
                case QCM_WEB_MSG_DNS_REQUEST:
                qcm_ws_dns_request(&msg);
                break;
            case QCM_WEB_MSG_DNS_RESPONSE:
                qcm_ws_dns_response(&msg);
                break;
            case QCM_WEB_MSG_HTTP_REQUEST:
                qcm_ws_http_request(&msg);
                break;
            case QCM_WEB_MSG_HTTP_RESPONSE:
                qcm_ws_http_response(&msg);
                break;
            case QCM_WEB_MSG_TCP_OPEN:
                qcm_ws_tcp_open(&msg);
                break;
            case QCM_WEB_MSG_TLS_HANDSHAKE:
#ifdef CONFIG_QCM_VTLS_FUNC
                qcm_ws_ssl_session_setup(&msg);
#endif
                break;
            case QCM_WEB_MSG_TCP_OPEN_TIMEOUT:
            case QCM_WEB_MSG_TCP_OPEN_CNF:
                qcm_ws_tcp_open_cnf(&msg);
                break;
            case QCM_WEB_MSG_TCP_CLOSING:
                qcm_ws_tcp_close(&msg);
                break;
            case QCM_WEB_MSG_TCP_CLOSE_CNF:
                qcm_ws_tcp_close_cnf(&msg);
                break;
            case QCM_WEB_MSG_TCP_WRITE:
                qcm_ws_socket_write(&msg);
                break;
            case QCM_WEB_MSG_TCP_RECV_DATA_REPORT:
                qcm_ws_tcp_recv_report(&msg);  //This feature may not be implemented or is commented out.
                break;
            case QCM_WEB_MSG_SOCKET_EVENT:
                qcm_ws_socket_evt(&msg);
                break;
            case QCM_WEB_MSG_WS_WRITE:
                qcm_ws_write(&msg);
                break;
            case QCM_WEB_MSG_PING_REQUEST:
                qcm_ws_ping_request(&msg);
                break;
            case QCM_WEB_MSG_PONG_REQUEST:
                qcm_ws_pong_request(&msg);
                break;
            default:  //Do not process unknown message types
                break;
        }
        WEB_FREE_PTR(msg.argv);                //Release message parameter memory
        qosa_mutex_unlock(g_ws_service.lock);  //Release mutex
    }
}

/**
 * Initialize the WebSocket service.
 * This function is responsible for creating the mutex, message queue, and tasks required for the WebSocket service.
 * If these components have already been created, they will not be created again.
 *
 * @return Returns QCM_WEB_ERR_OK on successful initialization, otherwise returns QCM_WEB_ERROR_UNKNOW.
 */
static int qcm_ws_service_init(void)
{
    int ret = 0;

    QLOGD("enter");
    //Check if the mutex has been created; if not, create it.
    if (g_ws_service.lock == QOSA_NULL)
    {
        ret = qosa_mutex_create(&g_ws_service.lock);
        if (ret != QOSA_OK)
        {
            return QCM_WEB_ERROR_UNKNOW;
        }
    }

    //Print logs to check if the message queue is created.
    QLOGD("lock");
    if (g_ws_service.msgq == QOSA_NULL)
    {
        //If the message queue is not created, then create it.
        ret = qosa_msgq_create(&g_ws_service.msgq, sizeof(qcm_web_msg_t), QCM_WEB_MSGQ_SIZE);
        if (ret != QOSA_OK)
        {
            return QCM_WEB_ERROR_UNKNOW;
        }
    }

    //Print logs to check if the task is created.
    QLOGD("msgq");
    if (g_ws_service.taskid == QOSA_NULL)
    {
        //If the task is not created, then create it.
        ret = qosa_task_create(&g_ws_service.taskid, QCM_WEB_CLIENT_TASK_SIZE, QOSA_PRIORITY_NORMAL, "web_client", qcm_ws_client_thread, QOSA_NULL);
        if (ret != QOSA_OK)
        {
            return QCM_WEB_ERROR_UNKNOW;
        }
    }
    QLOGD("end");

    return QCM_WEB_ERR_OK;
}

/************************************************************************/
//Open API

/**
 * Create a new WebSocket configuration
 *
 * This function is used to create a new WebSocket configuration based on the configuration ID. If the configuration ID exceeds the maximum number or the configuration already exists, no creation will be performed.
 *
 * @param config_id Configuration ID, used to identify the specific WebSocket configuration.
 * @return Returns the operation result status code. Returns QCM_WEB_ERR_OK on success, returns QCM_WEB_ERR_INVAL_PARM if the configuration ID is invalid or the configuration already exists.
 */
int qcm_ws_cfg_new(int config_id)
{
    int ret = QCM_WEB_ERR_OK;

    //Check if the configuration ID exceeds the maximum quantity
    if (config_id < 0 || config_id > QCM_WEB_CLIENT_ID_NUM_MAX)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    //Check if the configuration already exists
    if (g_ws_cfg[config_id] != QOSA_NULL)
    {
        return QCM_WEB_ERR_OK;
    }
    //Initialize WebSocket configuration
    ret = qcm_ws_config_init(config_id);
    QLOGD("cfg init ret=%d", ret);  //Print configuration initialization result
    return ret;
}

/**
 * Release the WebSocket configuration resources for the specified configuration ID.
 * @param config_id The ID of the WebSocket configuration, used to identify the configuration to be released
 *
 * First, check if the ID is within the valid range, then check if the configuration item has been initialized.
 * If all are within the valid range and have been initialized, release the memory for their URL, subprotocol, and extension fields.
 * Finally, release the memory occupied by the configuration item itself.
 */
void qcm_ws_cfg_free(int config_id)
{
    //Check if config_id is within the valid range
    if (config_id < 0 || config_id > QCM_WEB_CLIENT_ID_NUM_MAX)
    {
        return;
    }
    //Check if the corresponding configuration item is empty
    if (g_ws_cfg[config_id] == QOSA_NULL)
    {
        return;
    }

    //Release the memory occupied by the URL, subprotocol, and extension fields in the configuration item.
    WEB_FREE_PTR(g_ws_cfg[config_id]->config.conn.url);
    WEB_FREE_PTR(g_ws_cfg[config_id]->config.conn.subprot);
    WEB_FREE_PTR(g_ws_cfg[config_id]->config.conn.extension);
    WEB_FREE_PTR(g_ws_cfg[config_id]->config.conn.method);
    if (g_ws_cfg[config_id]->lock)
    {
        qosa_mutex_delete(g_ws_cfg[config_id]->lock);
    }

    qcm_ws_header_del_all(&g_ws_cfg[config_id]->config.header_list);
    qosa_q_destroy(&g_ws_cfg[config_id]->config.header_list);
    //Release the memory occupied by the configuration item itself.
    WEB_FREE_PTR(g_ws_cfg[config_id]);
}

qcm_web_err_e qcm_ws_cfg_set(int config_id, qcm_web_cfg_e opt_tag, void *ptr, int value)
{
    int               err = QCM_WEB_ERR_OK;
    qcm_web_config_t *cfg_ptr = QOSA_NULL;

    QLOGD("enter");
    //Check if the configuration ID is valid
    if (config_id < 0 || config_id > QCM_WEB_CFG_CONFIG_ID_MAX)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    //If the configuration has not been initialized, initialize it.
    if (g_ws_cfg[config_id] == QOSA_NULL)
    {
        err = qcm_ws_config_init(config_id);
        if (err != QCM_WEB_ERR_OK)
        {
            return err;
        }
    }

    cfg_ptr = &g_ws_cfg[config_id]->config;
    qosa_mutex_lock(g_ws_cfg[config_id]->lock, QOSA_WAIT_FOREVER);
    //Check the configuration status; if the current status is not the initial state, return a busy error.
    if (qcm_ws_client_get_status(config_id) != QCM_WEB_STATUS_INIT)
    {
        qosa_mutex_unlock(g_ws_cfg[config_id]->lock);
        return QCM_WEB_ERR_BUSY;
    }

    QLOGD("opt_tag=%d,%d", opt_tag, config_id);
    switch (opt_tag)
    {
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
        //Set different configuration parameters based on option labels
        case QCM_WEB_CFG_CONN_PDPCID:
            err = qcm_ws_setopt_int(&cfg_ptr->conn.pdp_cid, value, 0, QCM_WEB_CFG_CONN_PDP_CID_MAX);
            break;
        case QCM_WEB_CFG_CONN_SIMCID:
            err = qcm_ws_setopt_int(&cfg_ptr->conn.sim_cid, value, 0, QCM_WEB_CFG_CONN_SIM_CID_MAX);
            break;
#endif
        case QCM_WEB_CFG_CONN_SSL_CTX_ID:
            err = qcm_ws_setopt_int(&cfg_ptr->conn.ssl_ctx_id, value, QCM_WEB_CFG_CONN_SSL_ID_MIN, QCM_WEB_CFG_CONN_SSL_ID_MAX);
            break;
        case QCM_WEB_CFG_CONN_SSLCONFIG:
            cfg_ptr->conn.ssl_config = ptr;
            break;
        case QCM_WEB_CFG_CONN_URL:
            qcm_ws_setopt_string(&cfg_ptr->conn.url, (char *)ptr, value, QCM_WEB_CFG_CONN_URL_LEN_MAX);
            cfg_ptr->conn.url_len = value;
            QLOGD("str=%s", cfg_ptr->conn.url);
            break;
        case QCM_WEB_CFG_CONN_URL_EX:
            err = qcm_ws_setopt_int(&cfg_ptr->conn.url_ex, value, QCM_WEB_URL_EX_DISABLE, QCM_WEB_URL_EX_ENABLE);
            break;
        case QCM_WEB_CFG_CONN_SUBPROT_EN:
            err = qcm_ws_setopt_int(&cfg_ptr->conn.subprot_en, value, QCM_WEB_SUBPROT_DISABLE, QCM_WEB_SUBPROT_ENABLE);
            break;
        case QCM_WEB_CFG_CONN_SUBPROT:
            qcm_ws_setopt_string(&cfg_ptr->conn.subprot, (char *)ptr, value, QCM_WEB_CFG_CONN_SUBPROT_LEN_MAX);
            cfg_ptr->conn.subprot_len = value;
            break;
        case QCM_WEB_CFG_CONN_EXTENSION_EN:
            err = qcm_ws_setopt_int(&cfg_ptr->conn.extension_en, value, QCM_WEB_SUBPROT_DISABLE, QCM_WEB_SUBPROT_ENABLE);
            break;
        case QCM_WEB_CFG_CONN_EXTENSION:
            qcm_ws_setopt_string(&cfg_ptr->conn.extension, (char *)ptr, value, QCM_WEB_CFG_CONN_EXTENSION_LEN_MAX);
            cfg_ptr->conn.extension_len = value;
            break;
        case QCM_WEB_CFG_CONN_METHOD:
            qcm_ws_setopt_string(&cfg_ptr->conn.method, (char *)ptr, value, QCM_WEB_CFG_CONN_METHOD_LEN_MAX);
            cfg_ptr->conn.method_len = value;
            break;
        case QCM_WEB_CFG_CONN_TIMEOUT:
            err = qcm_ws_setopt_int(&cfg_ptr->conn.timeout, value, QCM_WEB_CFG_CONN_TIMEOUT_MIN, QCM_WEB_CFG_CONN_TIMEOUT_MAX);
            break;
        case QCM_WEB_CFG_PING_INTERVAL:
            err = qcm_ws_setopt_int(&cfg_ptr->ping.interval, value, QCM_WEB_CFG_PING_INTERVAL_MIN, QCM_WEB_CFG_PING_INTERVAL_MAX);
            break;
        case QCM_WEB_CFG_WRITE_BUFFERSZ:
            err = qcm_ws_setopt_int(&cfg_ptr->write.buffer_sz, value, QCM_WEB_CFG_WRITE_BUF_LEN_MIN, QCM_WEB_CFG_WRITE_BUF_LEN_MAX);
            break;
        case QCM_WEB_CFG_WRITE_TIMEOUT:
            err = qcm_ws_setopt_int(&cfg_ptr->write.timeout, value, QCM_WEB_CFG_WRITE_TIMEOUT_MIN, QCM_WEB_CFG_WRITE_TIMEOUT_MAX);
            break;
        case QCM_WEB_CFG_WRITE_ECHO:
            err = qcm_ws_setopt_int(&cfg_ptr->write.echo, value, QCM_WEB_ECHO_OFF, QCM_WEB_ECHO_ON);
            break;
        case QCM_WEB_CFG_READ_BUFFERSZ:
            err = qcm_ws_setopt_int(&cfg_ptr->read.buffer_sz, value, QCM_WEB_CFG_READ_BUF_LEN_MIN, QCM_WEB_CFG_READ_BUF_LEN_MAX);
            break;
        case QCM_WEB_CFG_READ_TIMEOUT:
            err = qcm_ws_setopt_int(&cfg_ptr->read.timeout, value, QCM_WEB_CFG_READ_TIMEOUT_MIN, QCM_WEB_CFG_READ_TIMEOUT_MAX);
            break;
        case QCM_WEB_CFG_READ_MODE:
            err = qcm_ws_setopt_int(&cfg_ptr->read.mode, value, QCM_WEB_MODE_BUFFER, QCM_WEB_MODE_PUSH);
            break;
        case QCM_WEB_CFG_CLOSE_WAITTIME:
            err = qcm_ws_setopt_int(&cfg_ptr->close.wait_time, value, QCM_WEB_CFG_CLOSE_WAIT_TIMEOUT_MIN, QCM_WEB_CFG_CLOSE_WAIT_TIMEOUT_MAX);
            break;
        case QCM_WEB_CFG_RECV_CB:
            cfg_ptr->read.recv_cb = (qcm_web_recv_cb_t)ptr;
            break;
        case QCM_WEB_CFG_REQHEAD:
            qcm_ws_header_add(&g_ws_cfg[config_id]->config.header_list, (qcm_web_header_data_t *)ptr);
            break;
        case QCM_WEB_CFG_REQHEAD_DEL:
            qcm_ws_header_del(&g_ws_cfg[config_id]->config.header_list, (qcm_web_header_data_t *)ptr);
            break;
        case QCM_WEB_CFG_REQHEAD_DEL_ALL:
            qcm_ws_header_del_all(&g_ws_cfg[config_id]->config.header_list);
            break;
        default:
            err = QCM_WEB_ERR_INVAL_PARM;
            break;
    }

    qosa_mutex_unlock(g_ws_cfg[config_id]->lock);
    return err;  //Returns the function execution status code
}

/**
 * @brief Obtain network configuration information
 *
 * This function is used to retrieve the corresponding network configuration information based on the configuration ID and option label.
 *
 * @param config_id Configuration ID, used to identify specific configuration information.
 * @param opt_tag Option tag, used to specify the configuration item to be retrieved.
 * @param cfg Pointer to the structure storing configuration values.
 * @return qcm_web_err_e Function execution status, returns QCM_WEB_ERR_OK to indicate success, other values indicate an error.
 */
qcm_web_err_e qcm_ws_cfg_get(int config_id, qcm_web_cfg_e opt_tag, qcm_web_cfg_t *cfg)
{
    int               err = 0;
    qcm_web_config_t *cfg_ptr = QOSA_NULL;

    //Check if the configuration ID is valid
    if (config_id < 0 || config_id > QCM_WEB_CFG_CONFIG_ID_MAX)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    //If the configuration has not been initialized, initialize it.
    if (g_ws_cfg[config_id] == QOSA_NULL)
    {
        err = qcm_ws_config_init(config_id);
        if (err != QCM_WEB_ERR_OK)
        {
            return err;
        }
    }

    QLOGD("opt_tag=%d,%d", opt_tag, config_id);

    //Lock to ensure concurrent access safety
    qosa_mutex_lock(g_ws_cfg[config_id]->lock, QOSA_WAIT_FOREVER);
    cfg_ptr = &g_ws_cfg[config_id]->config;
    switch (opt_tag)  //Process different configuration items based on option labels
    {
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
        //Based on the different option tags, obtain the corresponding configuration values and store them in the respective parameter pointers.
        case QCM_WEB_CFG_CONN_PDPCID:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->conn.pdp_cid);
            break;
        case QCM_WEB_CFG_CONN_SIMCID:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->conn.sim_cid);
            break;
#endif
        case QCM_WEB_CFG_CONN_SSL_CTX_ID:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->conn.ssl_ctx_id);
            break;
        case QCM_WEB_CFG_CONN_SSLCONFIG:
            cfg->ptr_value = cfg_ptr->conn.ssl_config;
            break;
        case QCM_WEB_CFG_CONN_URL:
            cfg->str_value = cfg_ptr->conn.url;
            cfg->str_len = cfg_ptr->conn.url_len;
            QLOGD("url=%s", cfg->str_value);
            break;
        case QCM_WEB_CFG_CONN_URL_EX:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->conn.url_ex);
            break;
        case QCM_WEB_CFG_CONN_SUBPROT_EN:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->conn.subprot_en);
            break;
        case QCM_WEB_CFG_CONN_SUBPROT:
            cfg->str_value = cfg_ptr->conn.subprot;
            cfg->str_len = cfg_ptr->conn.subprot_len;
            break;
        case QCM_WEB_CFG_CONN_EXTENSION_EN:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->conn.extension_en);
            break;
        case QCM_WEB_CFG_CONN_EXTENSION:
            cfg->str_value = cfg_ptr->conn.extension;
            cfg->str_len = cfg_ptr->conn.extension_len;
            break;
        case QCM_WEB_CFG_CONN_METHOD:
            cfg->str_value = cfg_ptr->conn.method;
            cfg->str_len = cfg_ptr->conn.method_len;
            break;
        case QCM_WEB_CFG_CONN_TIMEOUT:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->conn.timeout);
            break;
        case QCM_WEB_CFG_PING_INTERVAL:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->ping.interval);
            break;
        case QCM_WEB_CFG_WRITE_BUFFERSZ:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->write.buffer_sz);
            break;
        case QCM_WEB_CFG_WRITE_TIMEOUT:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->write.timeout);
            break;
        case QCM_WEB_CFG_WRITE_ECHO:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->write.echo);
            break;
        case QCM_WEB_CFG_READ_BUFFERSZ:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->read.buffer_sz);
            break;
        case QCM_WEB_CFG_READ_TIMEOUT:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->read.timeout);
            break;
        case QCM_WEB_CFG_READ_MODE:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->read.mode);
            break;
        case QCM_WEB_CFG_CLOSE_WAITTIME:
            err = qcm_ws_getopt_int(&cfg->int_value, cfg_ptr->close.wait_time);
            break;
        case QCM_WEB_CFG_RECV_CB:
            cfg->ptr_value = (void *)cfg_ptr->read.recv_cb;
            break;
        case QCM_WEB_CFG_REQHEAD:
            cfg->ptr_value = (void *)&cfg_ptr->header_list;
            break;
        default:
            err = QCM_WEB_ERR_INVAL_PARM;  //For unknown option tags, return an invalid parameter error.
            break;
    }
    //Unlock
    qosa_mutex_unlock(g_ws_cfg[config_id]->lock);
    return err;
}

/**
 * @brief Set WebSocket configuration information.
 *
 * This function is used to set all configuration parameters for the WebSocket with the specified configuration ID. If the configuration already exists, it will overwrite the original configuration.
 * Configuration parameters include the connection URL, subscription protocol, extensions, etc.
 *
 * @param config_id Configuration ID, used to identify different WebSocket configurations.
 * @param new_config points to a pointer to a qcm_web_config_t structure containing the new configuration information.
 *
 * @return The function returns an error code of type qcm_web_err_e, indicating the result of the operation.
 *         If the operation is successful, return QCM_WEB_ERR_OK;
 *         If the config_id is invalid or new_config is empty, return QCM_WEB_ERR_INVAL_PARM.
 */
qcm_web_err_e qcm_ws_cfg_set_all(int config_id, const qcm_web_config_t *new_config)
{
    int               err = 0;
    qcm_web_config_t *cfg_ptr = QOSA_NULL;

    //Check if the configuration ID is valid and if new_config is not empty
    if (config_id < 0 || config_id > QCM_WEB_CFG_CONFIG_ID_MAX || !new_config)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }

    //If the configuration has not been initialized, initialize it.
    if (g_ws_cfg[config_id] == QOSA_NULL)
    {
        err = qcm_ws_config_init(config_id);
        if (err != QCM_WEB_ERR_OK)
        {
            return err;
        }
    }

    cfg_ptr = &g_ws_cfg[config_id]->config;

    //Release the memory of the old string member
    WEB_FREE_PTR(cfg_ptr->conn.url);
    WEB_FREE_PTR(cfg_ptr->conn.subprot);
    WEB_FREE_PTR(cfg_ptr->conn.extension);
    WEB_FREE_PTR(cfg_ptr->conn.method);

    //Copy new configuration information
    qosa_memcpy(cfg_ptr, new_config, sizeof(qcm_web_config_t));

    //Allocate memory and copy the new string content
    if (new_config->conn.url && (new_config->conn.url_len > 0 && new_config->conn.url_len < QCM_WEB_CFG_CONN_URL_LEN_MAX))
    {
        qcm_ws_setopt_string(&cfg_ptr->conn.url, new_config->conn.url, new_config->conn.url_len, QCM_WEB_CFG_CONN_URL_LEN_MAX);
        cfg_ptr->conn.url_len = new_config->conn.url_len;
    }
    if (new_config->conn.subprot && (new_config->conn.subprot_len > 0 && new_config->conn.subprot_len < QCM_WEB_CFG_CONN_SUBPROT_LEN_MAX))
    {
        qcm_ws_setopt_string(&cfg_ptr->conn.subprot, new_config->conn.subprot, new_config->conn.subprot_len, QCM_WEB_CFG_CONN_SUBPROT_LEN_MAX);
        cfg_ptr->conn.subprot_len = new_config->conn.subprot_len;
    }
    if (new_config->conn.extension && (new_config->conn.extension_len > 0 && new_config->conn.extension_len < QCM_WEB_CFG_CONN_EXTENSION_LEN_MAX))
    {
        qcm_ws_setopt_string(&cfg_ptr->conn.extension, new_config->conn.extension, new_config->conn.extension_len, QCM_WEB_CFG_CONN_EXTENSION_LEN_MAX);
        cfg_ptr->conn.extension_len = new_config->conn.extension_len;
    }
    if (new_config->conn.method && (new_config->conn.method_len > 0 && new_config->conn.method_len < QCM_WEB_CFG_CONN_METHOD_LEN_MAX))
    {
        qcm_ws_setopt_string(&cfg_ptr->conn.method, new_config->conn.method, new_config->conn.method_len, QCM_WEB_CFG_CONN_METHOD_LEN_MAX);
        cfg_ptr->conn.method_len = new_config->conn.method_len;
    }

    return QCM_WEB_ERR_OK;
}

/**
 * @brief Retrieve all web configuration information for the specified configuration ID
 *
 * This function is used to retrieve the corresponding websocket configuration information from the global configuration array based on the provided configuration ID. If the configuration information has not been initialized,
 * then the initialization function will be called to perform initialization.
 *
 * @param config_id Configuration ID, used to identify specific web configuration information
 * @return qcm_web_config_t* Pointer to the requested web configuration information. Returns OSA_NULL if the configuration ID is invalid.
 */
qcm_web_config_t *qcm_ws_cfg_get_all(int config_id)
{
    int               err = 0;

    //Check if the configuration ID exceeds the valid range
    if (config_id < 0 || config_id > QCM_WEB_CFG_CONFIG_ID_MAX)
    {
        return QOSA_NULL;
    }
    //If the configuration has not been initialized, initialize it.
    if (g_ws_cfg[config_id] == QOSA_NULL)
    {
        err = qcm_ws_config_init(config_id);
        if (err != QCM_WEB_ERR_OK)
        {
            return QOSA_NULL;
        }
    }

    return &g_ws_cfg[config_id]->config;
}

/**
 * Handler function for opening a WebSocket connection.
 *
 * @param open_t points to a structure containing information such as client ID and configuration ID.
 * @return Returns the operation result status code, OK indicates success, others indicate an error.
 */
qcm_web_err_e qcm_ws_open_proc(qcm_web_open_t *open_t)
{
    int               ret = QCM_WEB_ERR_OK;
    qcm_web_config_t *cfg_ptr = QOSA_NULL;
    char             *url_ptr = QOSA_NULL;  //Pointer for storing URL

    //Check if the passed parameter is NULL
    if (open_t == QOSA_NULL)
    {
        return QCM_WEB_ERR_INVAL_PARM;  //Invalid parameter, returns error code
    }

    //Check if client_id and config_id are out of range
    if (open_t->client_id <0 || open_t->client_id > QCM_WEB_CLIENT_ID_NUM_MAX || open_t->config_id < 0 || open_t->config_id > QCM_WEB_CFG_CONFIG_ID_MAX)
    {
        return QCM_WEB_ERR_INVAL_PARM;  //Parameter out of range, returns error code
    }

    QLOGD("client_id=%d,%d", open_t->client_id, open_t->config_id);

    //Check if the configuration is valid
    if (g_ws_cfg[open_t->config_id] == QOSA_NULL)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }

    //Check if the client is initialized or in use
    if (qcm_ws_client_get_status(open_t->client_id) != QCM_WEB_STATUS_INIT || g_ws_client[open_t->client_id] != QOSA_NULL)
    {
        return QCM_WEB_ERR_BUSY;  //Client is busy
    }

    //Allocate memory for the client
    g_ws_client[open_t->client_id] = qosa_malloc(sizeof(qcm_web_client_t));
    if (g_ws_client[open_t->client_id] == QOSA_NULL)
    {
        return QCM_WEB_ERR_NOMEM;  //Memory allocation failed
    }
    qosa_memset(g_ws_client[open_t->client_id], 0, sizeof(qcm_web_client_t));

    if (g_ws_client[open_t->client_id]->lock != QOSA_NULL)
    {
        //If lock is not NULL, first delete the previously created lock, then recreate it.
        qosa_mutex_delete(g_ws_client[open_t->client_id]->lock);
        g_ws_client[open_t->client_id]->lock = QOSA_NULL;
    }
    ret = qosa_mutex_create(&g_ws_client[open_t->client_id]->lock);
    if (ret != 0)
    {
        WEB_FREE_PTR(g_ws_client[open_t->client_id]);
        return QCM_WEB_ERR_NOMEM;  //Memory allocation failed
    }
    //Initialize the allocated client structure
    g_ws_client[open_t->client_id]->client_id = open_t->client_id;
    g_ws_client[open_t->client_id]->config_id = open_t->config_id;
    //Initialize WebSocket service
    ret = qcm_ws_service_init();
    if (ret != QCM_WEB_ERR_OK)
    {
        WEB_FREE_PTR(g_ws_client[open_t->client_id]);
        return QCM_WEB_ERR_INVAL_PARM;  //Service initialization failed
    }

    //Obtain and parse URL
    cfg_ptr = qcm_ws_cfg_get_all(open_t->config_id);
    if (cfg_ptr == QOSA_NULL)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    url_ptr = cfg_ptr->conn.url;
    if (QOSA_NULL == url_ptr)
    {
        QLOGE("url NULL");
        return QCM_WEB_ERR_INVAL_PARM;
    }

    QLOGD("url_ptr=%s", url_ptr);
    ret = qcm_ws_parse_url(url_ptr, &g_ws_client[open_t->client_id]->url);
    if (ret != QCM_WEB_ERR_OK)
    {
        WEB_FREE_PTR(g_ws_client[open_t->client_id]->url.host);
        WEB_FREE_PTR(g_ws_client[open_t->client_id]->url.path);
        WEB_FREE_PTR(g_ws_client[open_t->client_id]);
        return QCM_WEB_ERR_INVAL_PARM;  //URL parsing failed
    }

    //Set the client status to network connection status
    g_ws_client[open_t->client_id]->status = QCM_WEB_STATUS_NET;

#ifdef CONFIG_QCM_VTLS_FUNC
    //Set SSL configuration
    g_ws_client[open_t->client_id]->ssl_config = g_ws_cfg[open_t->config_id]->config.conn.ssl_config;
#endif
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    ret = qcm_ws_msg_send(QCM_WEB_MSG_NET_REQUEST, open_t->client_id, QOSA_NULL);
#else
    //Send network request message
    ret = qcm_ws_msg_send(QCM_WEB_MSG_DNS_REQUEST, open_t->client_id, QOSA_NULL);
#endif
    return ret;  //Returns the final operation result
}

/**
 * @brief Handling WebSocket Client Close Process
 *
 * @param client_id Client ID, used to identify the WebSocket client that needs to be closed
 * @return qcm_web_err_e returns the operation result status code, OK indicates success, BUSY indicates the client is currently busy with network operations
 */
qcm_web_err_e qcm_ws_close_proc(int client_id)
{
    int status = 0;

    QLOGD("enter");
    //Attempt to obtain the client pointer for the specified ID; if the client does not exist, directly return OK.
    if (qcm_ws_client_get_ptr(client_id) == QOSA_NULL)
    {
        return QCM_WEB_ERR_OK;
    }

    //Get the current status of the client
    status = qcm_ws_client_get_status(client_id);
    //If the client status indicates that a network operation is in progress or that it is closing, return BUSY to indicate that the operation cannot proceed.
    if ((status >= QCM_WEB_STATUS_NET && status <= QCM_WEB_STATUS_TCP_CONNECTING) || status == QCM_WEB_STATUS_TCP_CLOSING)
    {
        return QCM_WEB_ERR_BUSY;
    }

    //Send TCP close message to the client
    qcm_ws_msg_send(QCM_WEB_MSG_TCP_CLOSING, client_id, QOSA_NULL);
    return QCM_WEB_ERR_OK;
}

/**
 * Read WebSocket data
 *
 * This function is used to read data from a WebSocket client. It attempts to read data from the specified client ID and stores the read data into the provided buffer.
 *
 * @param client_id Client ID, used to identify the WebSocket client that needs to read data.
 * @param data points to the pointer of the buffer used to store the read data. The read data will be stored in this buffer.
 * @param size Pointer to an integer, used to provide the buffer size during function call and return the actual read data size upon function return.
 * @return Returns the operation result status code, returns OSA_WS_SUCCESS on success, and returns other error codes on failure.
 */
qcm_web_err_e qcm_ws_read_proc(int client_id, char *data, int *size)
{
    qcm_web_client_t *client_ptr = QOSA_NULL;
    int               ret = 0;

    QLOGD("enter=%d,%d", client_id, *size);
    //Get client pointer based on client ID
    client_ptr = qcm_ws_client_get_ptr(client_id);

    if (client_ptr == QOSA_NULL)
    {
        return QCM_WEB_ERR_NOT_INIT;
    }
    if (client_ptr->status != QCM_WEB_STATUS_WEBSOCKET)
    {
        return QCM_WEB_ERR_BUSY;
    }
    ret = qcm_ws_read(client_id, data, size);
    return ret;
}

/**
 * @brief Write data to the specified WebSocket client.
 *
 * @param client_id Client ID, used to identify the target WebSocket client.
 * @param data Pointer to the buffer where data needs to be written.
 * @param len The length of data to be written.
 * @return qcm_web_err_e returns the result status code of the operation.
 *         - QCM_WEB_ERR_OK indicates that the operation was successful.
 *         - QCM_WEB_ERR_NOT_INIT indicates that the client is not initialized.
 *         - QCM_WEB_ERR_BUSY indicates that the client is currently busy.
 *         - QCM_WEB_ERR_NOMEM indicates a memory allocation failure.
 */
qcm_web_err_e qcm_ws_write_proc(int client_id, char *data, int len, qcm_web_opcode_e opcode)
{
    qcm_web_data_t   *data_ptr = QOSA_NULL;
    qcm_web_client_t *client_ptr = QOSA_NULL;

    QLOGD("enter");
    //Get client pointer based on client ID
    client_ptr = qcm_ws_client_get_ptr(client_id);
    //Check if the client pointer is null, i.e., whether the client is uninitialized.
    if (client_ptr == QOSA_NULL)
    {
        return QCM_WEB_ERR_NOT_INIT;
    }
    //Check if the client's current state is WebSocket state.
    if (client_ptr->status != QCM_WEB_STATUS_WEBSOCKET)
    {
        return QCM_WEB_ERR_BUSY;
    }
    //Check if the sending data limit has been reached
    if (client_ptr->transfer_session.send_wm_high == QOSA_TRUE)
    {
        return QCM_WEB_ERR_NOMEM;
    }

    //Allocate memory for the data to be sent
    data_ptr = qosa_malloc(sizeof(qcm_web_data_t));
    //Check if memory allocation is successful
    if (data_ptr == QOSA_NULL)
    {
        return QCM_WEB_ERR_NOMEM;
    }

    //Release in qcm_ws_write
    data_ptr->data = qosa_malloc(len + 1);
    if (data_ptr->data == QOSA_NULL)
    {
        WEB_FREE_PTR(data_ptr);
        return QCM_WEB_ERR_NOMEM;
    }
    qosa_memcpy(data_ptr->data, data, len);
    data_ptr->data_len = len;
    //data_ptr->data will be released in qcm_ws_write

    client_ptr->tx_total_size += data_ptr->data_len;  //Accumulate and record the data volume transmitted from TX
    QLOGD("data_len: %d, tx_total_size: %d", data_ptr->data_len, client_ptr->tx_total_size);

    if (opcode == QCM_WEB_OPCODE_TEXT)
    {
        data_ptr->opcode = OPCODE_TEXT;
    }
    else
    {
        data_ptr->opcode = OPCODE_BINARY;
    }
    //Send data
    qcm_ws_msg_send(QCM_WEB_MSG_WS_WRITE, client_id, data_ptr);
    return QCM_WEB_ERR_OK;
}

/**
 * Get the current socket connection status and client status
 *
 * @param client_id Client ID, used to identify a specific WebSocket client
 * @param socket_state returns the connection status of the socket.
 * @param client_state returns the status of the client.
 * @return If successful, returns QCM_WEB_OK. If failed, returns an error code;
 */
int qcm_ws_client_get_curr_conn_status(int client_id, int *socket_state, int *client_state)
{
    int ret = 0;

    ret = qcm_ws_client_get_status(client_id);
    QLOGD("ret: %d", ret);

    if (ret < 0)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }

    *client_state = ret;

    if (QCM_WEB_STATUS_INIT == *client_state || QCM_WEB_STATUS_TCP_CLOSED == *client_state)
    {
        //WebSocket is in the initialization state or closed state
        *socket_state = QCM_WEB_CONN_STA_IDLE;
    }
    else if (QCM_WEB_STATUS_NET <= *client_state && *client_state <= QCM_WEB_STATUS_HTTP_HANDSHAKE_FAIL)
    {
        //During PDP activation, DNS query, TCP connection establishment, and SSL handshake, all are within the WebSocket connection establishment process.
        *socket_state = QCM_WEB_CONN_STA_CONNECTING;
    }
    else if (QCM_WEB_STATUS_WEBSOCKET == *client_state)
    {
        //WebSocket connection has been established
        *socket_state = QCM_WEB_CONN_STA_CONNECTED;
    }
    else if (QCM_WEB_STATUS_TCP_CLOSING == *client_state)
    {
        //WebSocket connection is closing
        *socket_state = QCM_WEB_CONN_STA_CLOSING;
    }

    return QCM_WEB_ERR_OK;
}

/**
 * Get the amount of data currently sent from TX to the module, and how much of this data has been successfully sent out via send.
 *
 * @param client_id Client ID, used to identify a specific WebSocket client
 * @param tx_total_size Get data: How much data has been sent from TX to the module in total
 * @param send_size Get data: how much data was successfully sent via send
 * @return If successful, returns QCM_WEB_OK. If failed, returns an error code;
 */
qcm_web_err_e qcm_ws_client_get_send_size(int client_id, int *tx_total_size, int *send_size)
{
    qcm_web_client_t *client_ptr = QOSA_NULL;

    if (QOSA_NULL == tx_total_size || QOSA_NULL == send_size)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }

    client_ptr = qcm_ws_client_get_ptr(client_id);
    if (QOSA_NULL == client_ptr)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }

    *tx_total_size = client_ptr->tx_total_size;
    *send_size = client_ptr->send_payload_total_size;

    return QCM_WEB_ERR_OK;
}
