/*****************************************************************/ /**
* @file qcm_web_tcp.c
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

#include "qcm_base64.h"
#include "qosa_sys.h"
#include "qosa_def.h"
#include "qosa_log.h"
#include "qcm_socket_adp.h"
#include "qcm_websocket.h"
#include "qcm_web_internal.h"
#include "qcm_web_http.h"
#include "qosa_queue_list.h"
#include "qcm_proj_config.h"

#define QOS_LOG_TAG LOG_TAG_WEB_API

#define WEB_FREE_PTR(p)                                                                                                                                        \
    {                                                                                                                                                          \
        if ((p) != QOSA_NULL)                                                                                                                                  \
        {                                                                                                                                                      \
            qosa_free(p);                                                                                                                                      \
            (p) = QOSA_NULL;                                                                                                                                   \
        }                                                                                                                                                      \
    }

#ifdef CONFIG_QCM_VTLS_FUNC

/**
 * @brief Initialize and execute the SSL connection action.
 * This function is responsible for initializing the SSL session based on the incoming message and attempting to establish an SSL connection.
 *
 * @param msg points to a pointer of the qcm_web_msg_t structure, containing the parameters required to establish an SSL connection.
 *
 * Note: This function does not return an error code, but indicates success or failure by modifying the client state and sending different types of messages.
 */
void qcm_ws_ssl_session_setup(qcm_web_msg_t *msg)
{
    int               client_id = 0;                //Client ID
    qcm_web_client_t *client_ptr = QOSA_NULL;       //Pointer to the client structure
    int               result = QCM_WEB_ERR_OK;      //Operation result status code

    client_id = msg->param1;                        //Get the client ID from the message
    client_ptr = qcm_ws_client_get_ptr(client_id);  //Get client pointer based on client ID
    if (client_ptr == QOSA_NULL)
    {
        //Check if the client pointer is valid
        return ;
    }

    QLOGD("client_state : %d", client_ptr->status);

    //Check if SSL has been initialized
    if (qcm_socket_ssl_is_init_already(client_ptr->sock_hd) == QOSA_FALSE)
    {
        if (QOSA_NULL == client_ptr->ssl_config)
        {
            result = QCM_WEB_ERR_INVAL_PARM;
            goto exit;
        }

        //If not initialized, perform SSL configuration
        if (qcm_socket_ssl_config(client_ptr->sock_hd, client_ptr->ssl_config, client_ptr->url.host, QOSA_FALSE) != QCM_SOCK_SUCCESS)
        {
            result = QCM_WEB_ERR_INVAL_PARM;  //SSL configuration failed, setting error result code
            goto exit;
        }
    }

    if (result == QCM_WEB_ERR_OK)
    {
        //Attempt to establish SSL connection
        qcm_sock_err_code ret = qcm_socket_ssl_connect(client_ptr->sock_hd);

        if (ret != QCM_SOCK_SUCCESS)
        {
            if (ret == QCM_SOCK_WODBLOCK)
            {
                //SSL handshake not completed, update client status and continue waiting
                client_ptr->status = QCM_WEB_STATUS_TLS_HANDSHAKE;
                QLOGD("ssl_handshark continue!!!");
            }
            else
            {
                //SSL connection failed, update client status and send error message
                QLOGE("ssl_handshark error!!!");
                client_ptr->status = QCM_WEB_STATUS_TCP_CLOSING;
                result = QCM_WEB_STATUS_TLS_HANDSHAKE_FAIL;
                goto exit;
            }
        }
        else
        {
            //SSL connection successful, update client status and send success message
            client_ptr->status = QCM_WEB_STATUS_TLS_HANDSHAKE_OK;
            QLOGD("ssl_handshark success!!!");
            result = QCM_WEB_ERR_OK;
            goto exit;
        }
    }

    return;  //Function ended normally

exit:
    //Send different messages based on the operation result
    if (client_ptr->timer)
    {
        qosa_timer_stop(client_ptr->timer);
    }
    if (result == QCM_WEB_ERR_OK)
    {
        QLOGD("exit");
        qcm_ws_msg_send(QCM_WEB_MSG_HTTP_REQUEST, client_ptr->client_id, QOSA_NULL);
    }
    else
    {
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_CLOSING, client_ptr->client_id, QOSA_NULL);
    }
}

#endif

/**
 * @brief TCP open timeout callback function
 *
 * When this function is called, it indicates that the TCP connection attempt has timed out. It will send a message to notify the Web client that the TCP opening has failed.
 *
 * @param arg Pointer to the qcm_web_client_t structure, containing the context information of the Web client.
 */
static void qcm_ws_tcp_open_timer_cb(void *arg)
{
    qcm_web_client_t *client_ptr = QOSA_NULL;

    //Enter the function, start processing
    QLOGE("enter");
    //Convert the void pointer to a qcm_web_client_t type pointer
    client_ptr = (qcm_web_client_t *)arg;
    //Send TCP open timeout message
    qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_TIMEOUT, client_ptr->client_id, QOSA_NULL);
}

/**
 * @brief Confirmation function for opening a TCP connection
 *
 * This function is used to process the acknowledgment message for opening a TCP connection. It retrieves the client pointer based on the parameters in the message,
 * and perform corresponding operations based on the client's connection status, such as calling the receive callback function or releasing client resources.
 *
 * @param msg points to a structure containing the TCP connection open confirmation information.
 */
void qcm_ws_tcp_open_cnf(qcm_web_msg_t *msg)
{
    int               client_id = 0;           //Client ID
    qcm_web_client_t *client_ptr = QOSA_NULL;  //Client pointer

    //Enter the function, start processing the confirmation for opening a TCP connection
    QLOGD("enter");
    client_id = msg->param1;  //Get the client ID from the message
    //Get client pointer based on client ID
    client_ptr = qcm_ws_client_get_ptr(client_id);
    //If no corresponding client is found, return directly.
    if (client_ptr == QOSA_NULL)
    {
        return;
    }

    //Call the receive callback function to notify the result of the TCP connection opening.
    qcm_ws_recv_cb(client_id, QCM_WEB_TYPE_OPEN, client_ptr->result, QOSA_NULL, 0);

    QLOGD("result: %d, status: %d", client_ptr->result, client_ptr->status);
    //If the connection is successful and the client status is WebSocket, send a PING request.
    if (client_ptr->result == QCM_WEB_ERR_OK && client_ptr->status == QCM_WEB_STATUS_WEBSOCKET)
    {
        //First, check if there is any received data in the receive queue.
        //If present, it indicates: the response header and data frame were received together, and when parse_websocket_upgrade_response processes the data frame, it places the data frame's content into the queue.
        web_transfer_session *transfer_session = QOSA_NULL;
        web_socket_frame_t   *r_frame = QOSA_NULL;

        transfer_session = &client_ptr->transfer_session;
        QLOGD("current_recv_cnt: %d", transfer_session->current_recv_cnt);

        //current_recv_cnt is not 0, indicating there is data in the queue (data frames that arrived along with the response header), requiring URC notification for RECV arrival
        if (0 < transfer_session->current_recv_cnt)
        {
            //Whether in direct push mode (QCM_WEB_MODE_PUSH) or buffer mode (QCM_WEB_MODE_BUFFER), both are reported via QCM_WEB_MSG_TCP_RECV_DATA_REPORT notification.

            //Direct output mode, report data immediately (qcm_ws_recv_cb has already notified the TCP connection result, data can be reported directly here)

            //In buffer mode (QCM_WEB_MODE_BUFFER), when parse_websocket_upgrade_response executes qosa_buffer_enqueue, the client_ptr->status is not QCM_WEB_STATUS_WEBSOCKET.
            //ws_rx_wm_non_empty_cb Previously, there was no notification for RECV URC, so a notification is also required here.

            qcm_ws_msg_send_2(QCM_WEB_MSG_TCP_RECV_DATA_REPORT, client_ptr->client_id, transfer_session->current_recv_cnt, QOSA_NULL);
        }

        //Subsequent processing of additional control frames should follow the approach used in `qcm_ws_socket_read`.
        r_frame = &client_ptr->r_frame;

        //The additional frames following the websocket response header contain the complete PING frame.
        if (QOSA_TRUE == r_frame->ping)
        {
            //Process PING requests, send PONG packet requests
            QLOGD("PONG REQUEST");
            qcm_ws_msg_send(QCM_WEB_MSG_PONG_REQUEST, client_ptr->client_id, QOSA_NULL);
        }

        //The additional frames following the websocket response header contain a complete CLOSE frame, or trigger other closing actions.
        if (QOSA_TRUE == r_frame->close)
        {
            QLOGD("CLOSE");
            qcm_ws_msg_send(QCM_WEB_MSG_TCP_CLOSING, client_ptr->client_id, QOSA_NULL);

            r_frame->close = QOSA_FALSE;
        }

        qcm_ws_msg_send(QCM_WEB_MSG_PING_REQUEST, client_ptr->client_id, QOSA_NULL);
    }
    else  //If the connection is unsuccessful or the client status is not WebSocket, release the client resources.
    {
        //TCP is already connected, need to close this connection
        //No need to report URC here, qcm_ws_recv_cb has already notified the user that the TCP open failed, just close the connection directly here.
        //Refer to the implementation of qcm_ws_tcp_close
        if (QCM_WEB_STATUS_TCP_CONNECTED <= client_ptr->status && client_ptr->status <= QCM_WEB_STATUS_WEBSOCKET)
        {
            qcm_web_config_t *cfg_ptr = QOSA_NULL;
            cfg_ptr = qcm_ws_cfg_get_all(client_ptr->config_id);

            if (cfg_ptr != QOSA_NULL)
            {
                int                 sock_hd = 0;
                int                 close_timeout = 0;
                qcm_socket_linger_t linger = {0};

                close_timeout = cfg_ptr->close.wait_time;
                linger.on_off = 1;
                linger.linger_val = close_timeout;
                sock_hd = client_ptr->sock_hd;
                qcm_socket_set_opt(sock_hd, QCM_SOCK_LINGER_OPT, (void *)&linger);

                //Close the socket, this is a synchronous close, blocking wait.
                qcm_socket_close(sock_hd);

                client_ptr->status = QCM_WEB_STATUS_TCP_CLOSED;
            }
        }

        qcm_ws_client_free(client_id);
    }
}

/**
 * @brief Confirmation function for closing TCP connection
 *
 * This function is called to handle the acknowledgment of TCP closure. It releases the relevant client resources based on the incoming message and notifies the upper-layer application.
 *
 * @param msg points to a pointer of a message structure containing operation parameters.
 */
void qcm_ws_tcp_close_cnf(qcm_web_msg_t *msg)
{
    int               client_id = 0;           //Client ID
    qcm_web_client_t *client_ptr = QOSA_NULL;  //Client information pointer

    //Enter the function, start processing the close confirmation
    QLOGD("enter");
    client_id = msg->param1;  //Get the client ID from the message
    //Get client information pointer based on client ID
    client_ptr = qcm_ws_client_get_ptr(client_id);
    //If the corresponding client information cannot be found, return directly.
    if (client_ptr == QOSA_NULL)
    {
        return;
    }
    //Call the receive callback function to notify the upper-layer application that the TCP connection has been closed.
    qcm_ws_recv_cb(client_id, QCM_WEB_TYPE_CLOSE, client_ptr->result, QOSA_NULL, 0);
    qcm_ws_client_free(client_id);
}

/**
 * Get the cumulative payload data volume of transmitted frames (excluding the header length added internally by ws_http_add_customer_header, only concerned with the payload data passed by the user from TX)
 *
 * @param client_id Client ID, used to identify the target WebSocket client.
 * @return Returns 0 on success, returns an error code on failure.
 *
 * The send_len of qcm_socket_send is the total data amount of the transmission frame (frame header + payload). It is necessary to use qcm_ws_get_send_payload_total_size to record only the payload data amount of these frames.
 *
 * s_frame_list records the frame header length and payload length of each frame node in qcm_ws_construct_frame.
 * qcm_socket_send total data stream (frame header + payload) length send_len
 * In qcm_ws_get_send_payload_total_size, calculate the total payload length of the frame to be sent based on the above information.
 */
static void qcm_ws_get_send_payload_total_size(int client_id, int send_len)
{
    qcm_web_client_t     *client_ptr = QOSA_NULL;
    qosa_q_type_t        *s_frame_list = QOSA_NULL;
    web_socket_s_frame_t *s_frame_node = QOSA_NULL;

    int offset = 0;  //Skipped data (offset of current data, i.e., length of frames already acknowledged in send (frame header and payload))

    if (send_len <= 0)
    {
        return;
    }
    QLOGD("send_len: %d", send_len);

    client_ptr = qcm_ws_client_get_ptr(client_id);
    if (QOSA_NULL == client_ptr)
    {
        return;
    }

    s_frame_list = &client_ptr->s_frame_list;
    s_frame_node = (web_socket_s_frame_t *)qosa_q_check(s_frame_list);
    if (QOSA_NULL == s_frame_node)
    {
        return;
    }

    while (s_frame_node != QOSA_NULL && offset < send_len)
    {
        //send_len - offset is the remaining data amount, which has two possibilities:

        //1. Remaining data volume, can send one frame (frame header remaining data volume + payload remaining data volume) completely    (Condition: s_frame_node->header_len + s_frame_node->data_len <= send_len - offset)

        //2. The remaining data volume is insufficient to send a complete frame, possible scenarios include:
        //(1) The frame header data is insufficient to be fully sent, or exactly only enough to send the frame header data    (Condition: send_len - offset <= s_frame_node->header_len)
        //(2) Insufficient payload data to complete transmission    (Condition: s_frame_node->header_len < send_len - offset < s_frame_node->header_len + s_frame_node->data_len)

        QLOGD("offset: %d, header_len: %d, data_len: %d", offset, s_frame_node->header_len, s_frame_node->data_len);
        QLOGD("opcode: %d", s_frame_node->opcode);

        if (offset + s_frame_node->header_len + s_frame_node->data_len <= send_len)
        {
            //The remaining data volume can be sent in one frame, with the condition also being: s_frame_node->header_len + s_frame_node->data_len <= send_len - offset

            web_socket_s_frame_t *next_node = QOSA_NULL;

            if (s_frame_node->opcode == OPCODE_CONTINUATION || s_frame_node->opcode == OPCODE_TEXT || s_frame_node->opcode == OPCODE_BINARY)
            {
                //The remaining payload of this frame has already been sent out in the send operation.
                //Directly accumulate the remaining payload of this frame.
                //During the user's process of writing data and sending data frames, the module may also send corresponding PONG frames (containing data). These data are not actively sent by the user and should not be counted.
                client_ptr->send_payload_total_size += s_frame_node->data_len;
            }

            offset += (s_frame_node->header_len + s_frame_node->data_len);  //Directly skip this frame (offset by the length of this frame)

            //This frame (frame header + payload) has been completely sent via send.
            s_frame_node->header_len = 0;  //The frame header is set to 0, indicating that the frame header has been completely sent out via send.
            s_frame_node->data_len = 0;    //The load is set to 0, indicating that the payload of this frame has been completely sent out via send.

            //First, get the node of the next frame.
            next_node = (web_socket_s_frame_t *)qosa_q_next(s_frame_list, &s_frame_node->list);

            //Delete the current frame node
            qosa_q_delete(s_frame_list, &s_frame_node->list);
            WEB_FREE_PTR(s_frame_node);

            //Prepare the next frame node and compare the remaining data volume
            s_frame_node = next_node;
        }
        else
        {
            //The remaining data volume is insufficient to complete sending a frame.

            if (send_len - offset <= s_frame_node->header_len)
            {
                //The frame header data is insufficient to complete transmission, or exactly enough to only transmit the frame header data.

                //The length of the frame header sent in this transmission is: send_len - offset
                //The remaining unsent header length for this frame is: s_frame_node->header_len - (send_len - offset)
                //The remaining frame header of this frame can only be sent during the next execution of qcm_ws_socket_send, and the length of the remaining frame header needs to be updated.

                s_frame_node->header_len = s_frame_node->header_len - (send_len - offset);  //Update remaining frame header length

                //Update offset: original offset + remaining data amount sent this time (send_len - offset)
                // offset = offset + (send_len - offset);
                //also: offset = send_len;

                //For the query AT+QWSWRITE=<connectID>, the user only cares about how much of the data they passed into TX was actually sent via send.
                //The frame header is internally added by the module in ws_http_add_customer_header, users do not need to care about this data, here we only need to update the header_len.
            }
            else
            {
                // s_frame_node->header_len < send_len - offset < s_frame_node->header_len + s_frame_node->data_len
                //The remaining data amount (send_len - offset) is sufficient to send all the remaining frame header data of the current frame, but insufficient to send the remaining payload of the current frame.
                //Then the remaining data amount (send_len - offset) - the already sent remaining header length (s_frame_node->header_len) is the data amount of the current frame payload to be sent in this send operation.

                if (s_frame_node->opcode == OPCODE_CONTINUATION || s_frame_node->opcode == OPCODE_TEXT || s_frame_node->opcode == OPCODE_BINARY)
                {
                    //The cumulative payload length sent, which needs to be added to the payload length of the current frame that has already been sent via send.
                    //Similarly, the payload length of data frames not actively sent by the user cannot be accumulated.
                    client_ptr->send_payload_total_size += ((send_len - offset) - s_frame_node->header_len);
                }

                //Update the remaining payload length for the current frame: original data_len - payload length already sent via send ((send_len - offset) - s_frame_node->header_len)
                s_frame_node->data_len = s_frame_node->data_len - ((send_len - offset) - s_frame_node->header_len);
                //Since the header of this frame has been fully sent, the remaining header length is updated to 0 (the remaining payload length above needs to be calculated using the header length, so the header length is finally updated to 0)
                s_frame_node->header_len = 0;

                //Update offset: original offset + remaining data amount sent this time (send_len - offset)
                // offset = offset + (send_len - offset);
                //also: offset = send_len;
            }

            //The two offsets above, offset = offset + (send_len - offset), are uniformly updated here.
            offset = send_len;
        }
    }
}

/**
 * Send data to the specified WebSocket client.
 *
 * @param client_id Client ID, used to identify the target WebSocket client.
 * @return Returns 0 on success, returns an error code on failure.
 *
 * This function locates the WebSocket client with the specified ID and attempts to send data. It first checks whether the client's status is in the WebSocket state,
 * and ensure the send buffer is not full. Then, it takes data from the send buffer and attempts to send it out. Based on the send result,
 * It may rearrange the data positions within the send buffer.
 */
int qcm_ws_socket_send(int client_id)
{
    qcm_web_client_t     *client_ptr = QOSA_NULL;
    web_transfer_session *transfer_session = QOSA_NULL;
    int                   pullup_len = 0;
    char                  write_buf[QCM_WEB_TCP_WRITE_BUFFER_SIZE + 1] = {0};
    int                   write_cnt = 0;
    int                   send_len = 0;

    QLOGD("enter");
    //Get the WebSocket client pointer for the specified ID
    client_ptr = qcm_ws_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }
    transfer_session = &client_ptr->transfer_session;
    //Check if the client status is WebSocket status
    if (client_ptr->status != QCM_WEB_STATUS_WEBSOCKET)
    {
        return QCM_WEB_ERR_INVAL_PARM;
    }

    //Retrieve data from the watermark and prepare for transmission
    while (transfer_session->current_send_data_ptr != QOSA_NULL
           || (transfer_session->current_send_data_ptr = (qosa_buffer_block_t *)qosa_buffer_dequeue(&transfer_session->send_wm_ptr)) != QOSA_NULL)
    {
        write_cnt = MIN(qosa_buffer_length_packet(transfer_session->current_send_data_ptr), QCM_WEB_TCP_WRITE_BUFFER_SIZE);
        pullup_len = qosa_buffer_pullup(&(transfer_session->current_send_data_ptr), write_buf, write_cnt);
        QLOGD("pullup_len=%d", pullup_len);
        break;
    }

    //If there is no data to send, return
    if (pullup_len <= 0)
    {
        return 0;
    }

    //Network sends data
    //qcm_socket_send sends data that is a mixture of frame header and payload.
    send_len = qcm_socket_send(client_ptr->sock_hd, (char *)write_buf, pullup_len);

    //qcm_socket_send transmits data that is a mixture of the frame header and payload, not just the payload (but when users query, they do not care about the length of the frame header added internally by the module).
    //If only the cumulative payload length of frames sent via qcm_socket_send needs to be recorded, it must be calculated using the qcm_ws_get_send_payload_total_size function.
    qcm_ws_get_send_payload_total_size(client_id, send_len);

    QLOGD("send_len=%d", send_len);
    //Adjust the send buffer based on the transmission result.
    if (send_len != pullup_len)
    {
        if (send_len <= 0)
        {
            //Send failed, cache the data to be sent
            qosa_buffer_pushdown(&transfer_session->current_send_data_ptr, (void *)write_buf, pullup_len);
        }
        else
        {
            if (client_ptr->url.scheme == WS_SCHEME_WSS)
            {
                //If sending fails under TLS, the complete data needs to be cached.
                qosa_buffer_pushdown(&transfer_session->current_send_data_ptr, (void *)write_buf, pullup_len);
            }
            else
            {
                //Under TCP, only the remaining data needs to be cached.
                qosa_buffer_pushdown(&transfer_session->current_send_data_ptr, (void *)(write_buf + send_len), pullup_len - send_len);
            }
            qcm_ws_msg_send(QCM_WEB_MSG_TCP_WRITE, client_id, QOSA_NULL);
        }
    }
    else  //Successfully sent all data, notification that more data can be written.
    {
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_WRITE, client_id, QOSA_NULL);
    }

    return 0;
}

/**
 * Read data from the network and cache it into the watermark.
 *
 * @param client_id Client ID, used to identify a specific WebSocket client connection.
 *
 * This function is used to read data from the specified WebSocket client and perform corresponding processing based on the type of data received,
 * Including handling text, binary data, and responding to PING requests.
 */
void qcm_ws_socket_read(int client_id)
{
    //Define necessary pointers and variables
    qcm_web_client_t     *client_ptr = QOSA_NULL;
    qcm_web_config_t     *cfg_ptr = QOSA_NULL;
    web_socket_frame_t   *r_frame = QOSA_NULL;
    web_transfer_session *transfer_session = QOSA_NULL;
    qosa_buffer_block_t  *read_data_item = QOSA_NULL;
    char                 *read_buf = QOSA_NULL;
    int read_buf_size = QCM_WEB_TCP_READ_BUFFER_SIZE + 4;  //To pass automated testing, considering that a single frame of data returned by a typical server generally does not exceed 65535 bytes, a 4-byte header is added here.
    int   read_len = 0;
    char *data_buf = QOSA_NULL;
    int   data_len = 0;
    int   header_data_len = 0;  //Read, but the header length is interrupted

    QLOGD("enter");
    //Get client pointer
    client_ptr = qcm_ws_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return;
    }
    //Get configuration information pointer
    cfg_ptr = qcm_ws_cfg_get_all(client_ptr->config_id);
    if (cfg_ptr == QOSA_NULL)
    {
        return;
    }

    //Obtain the transmission session pointer
    transfer_session = &client_ptr->transfer_session;

    //Check if the client status is WebSocket status
    if (client_ptr->status != QCM_WEB_STATUS_WEBSOCKET)
    {
        QLOGE("status: %d", client_ptr->status);
        return;
    }
    //Check if the watermark has reached the receive limit
    if (transfer_session->recv_wm_high == QOSA_TRUE)
    {
        QLOGE("wm_high");
        return;
    }

    //Allocate read buffer and read data
    read_buf = qosa_malloc(read_buf_size + 1);
    if (read_buf == QOSA_NULL)
    {
        QLOGE("no mem");
        return;
    }
    qosa_memset(read_buf, 0, read_buf_size + 1);

    r_frame = &client_ptr->r_frame;

    if (r_frame->header_break)
    {
        //Take the incomplete frame header from the last time, then concatenate it with the data to be read this time, hoping to form a complete frame header.
        QLOGD("header_data_len: %d", r_frame->header_data_len);
        qosa_memcpy(read_buf, r_frame->header_data, r_frame->header_data_len);
        header_data_len = r_frame->header_data_len;
    }

    //Read data from the network
    read_len = qcm_socket_read(client_ptr->sock_hd, (char *)(read_buf + header_data_len), read_buf_size - header_data_len);
    if (read_len <= 0)
    {
        WEB_FREE_PTR(read_buf);
        return;
    }
    QLOGD("read_len: %d", read_len);

    read_len = read_len + header_data_len;  //plus the length of the incomplete frame header data from last time

    //Allocate data buffer and parse WebSocket frame
    data_buf = qosa_malloc(read_len + 1);
    if (data_buf == QOSA_NULL)
    {
        WEB_FREE_PTR(read_buf);
        return;
    }
    qosa_memset(data_buf, 0, read_len + 1);
    //Parsing WebSocket frames
    //read_buf is a data stream, which may contain multiple WebSocket frames, or a single frame split into multiple parts (header split, payload split).
    qcm_ws_parse_frame(client_ptr->client_id, read_buf, read_len, data_buf, &data_len);
    //data_buf obtains the payload content of the websocket data frame
    QLOGD("read_len=%d,data_len=%d", read_len, data_len);
    QLOGD("data_buf: %s", data_buf);

    //The payload content of the WebSocket data frame is stored in data_buf, and if data_len > 0, this content is directly reported.
    if (data_len > 0)
    {
        read_data_item = qosa_buffer_new_block();

        if (read_data_item != QOSA_NULL)
        {
            //Add the data to the receive queue
            qosa_buffer_pushdown_tail(&read_data_item, data_buf, data_len);
            transfer_session->current_recv_cnt = data_len;
            qosa_buffer_enqueue(&(transfer_session->recv_wm_ptr), &read_data_item);

            //qosa_buffer_enqueue/memoryi_enqueue => buffer_wm_check_levels => ws_rx_wm_non_empty_cb which is transfer_session->recv_wm_ptr.wm_become_nonempty_cb
            //QCM_WEB_MODE_BUFFER cache mode RECV URC notifications will be reported in ws_rx_wm_non_empty_cb

            QLOGD("recv");
            //If it is direct output mode, report the data immediately.
            if (cfg_ptr->read.mode == QCM_WEB_MODE_PUSH)
            {
                qcm_ws_msg_send_2(QCM_WEB_MSG_TCP_RECV_DATA_REPORT, client_ptr->client_id, data_len, QOSA_NULL);
            }
        }
        else
        {
            QLOGE("no mem");
        }
    }

    //There are multiple frames in the data stream, which include ping frames.
    if (QOSA_TRUE == r_frame->ping)
    {
        //Process PING requests, send PONG packet requests
        QLOGD("PONG REQUEST");
        qcm_ws_msg_send(QCM_WEB_MSG_PONG_REQUEST, client_ptr->client_id, QOSA_NULL);
    }

    //There are multiple frames in the data stream, which include a close frame or other undefined control frame types.
    //or these control frames do not meet the requirements of RFC 6455
    //In these cases, refer to libwebsocket for shutdown handling.
    if (QOSA_TRUE == r_frame->close)
    {
        //Received WebSocket Connection Close [FIN] from server, header->opcode == OPCODE_CLOSE
        //Reference P4-952851 2024/6/4 18:56 for processing: Upon receiving the server's WebSocket Close[FIN] notification, the module no longer replies with WebSocket Close[FIN][MASKED]; instead, the module directly initiates the WebSocket connection closure process.

        //libwebsocket handles other error codes by directly closing the websocket connection; the same approach is taken here.

        //Send TCP close message, refer to qcm_ws_close_proc to send close event
        QLOGD("CLOSE");
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_CLOSING, client_ptr->client_id, QOSA_NULL);

        //The close event has been sent, the close flag is set to QOSA_FALSE
        r_frame->close = QOSA_FALSE;
    }

    //Release allocated buffer resources
    WEB_FREE_PTR(read_buf);
    WEB_FREE_PTR(data_buf);
}

/**
 * @brief WebSocket client socket event callback function
 *
 * This function handles socket events for the WebSocket client, such as connection establishment, data reception, and connection closure.
 * Based on the incoming event type and code, perform the corresponding operation, such as closing a socket connection or sending an event message.
 *
 * @param sock_hd Socket handle, identifying a socket connection
 * @param event event type, indicating the specific event that occurred
 * @param code event code, provides additional information about the event
 * @param user_argv User-defined parameters, used to pass additional data to the event handler function
 */
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
static void qcm_ws_client_socket_evt_cb(qosa_int32_t sock_hd, qosa_int32_t event, qosa_int32_t code, void *user_argv)
#else
static void qcm_ws_client_socket_evt_cb(int sock_hd, int event, int code, void *user_argv)
#endif
{
    qcm_web_soc_event_t *evt = QOSA_NULL;

    QLOGD("evt_cb: %d,%x,%d", sock_hd, event, code);

    //If the user-defined parameter is NULL, close the socket connection.
    if (user_argv == QOSA_NULL)
    {
        //Set the socket close wait time to 0, then close the socket
        qcm_socket_linger_t linger = {0};
        linger.on_off = 1;
        linger.linger_val = 0;
        qcm_socket_set_opt(sock_hd, QCM_SOCK_LINGER_OPT, (void *)&linger);
        qcm_socket_close(sock_hd);
        return;
    }

    //Allocate memory to store event information, preparing to send event messages
    evt = (qcm_web_soc_event_t *)qosa_malloc(sizeof(qcm_web_soc_event_t)); /*!Destroyed after task exec*/
    //Ensure memory allocation is successful
    QOSA_ASSERT(evt != QOSA_NULL);

    //Fill event information
    evt->sock_fd = sock_hd;
    evt->event_mask = event;
    evt->code = code;
    evt->user_data = user_argv;
    //Send event message
    qcm_ws_msg_send(QCM_WEB_MSG_SOCKET_EVENT, 0, (void *)evt);
}

/**
 * @brief Handling WebSocket socket events
 *
 * @param msg points to a pointer to the qcm_web_msg_t structure, containing parameters related to the event
 * @return qcm_web_err_e function execution status, returns QCM_WEB_ERR_OK to indicate success
 */
qcm_web_err_e qcm_ws_socket_evt(qcm_web_msg_t *msg)
{
    qcm_web_soc_event_t *evt_ptr = QOSA_NULL;             //Define event pointer
    qcm_web_client_t    *client_ptr = QOSA_NULL;          //Define client pointer

    QLOGD("enter");                                       //Print function entry information
    evt_ptr = (qcm_web_soc_event_t *)msg->argv;           //Get the event pointer from the message
    client_ptr = (qcm_web_client_t *)evt_ptr->user_data;  //Get client pointer from event

    //Execute corresponding processing based on the event type
    if (evt_ptr->event_mask & QCM_SOCK_CONNECT_EVENT)
    {
        QLOGD("QCM_SOCK_CONNECT_EVENT client_id=%d,%d", client_ptr->client_id, client_ptr->config_id);  //Print connection event information
        client_ptr->status = QCM_WEB_STATUS_TCP_CONNECTED;

        //TCP connection failed, directly report error QCM_WEB_ERR_TCP_CONNECT_FAIL, no other actions required
        if (evt_ptr->code != QCM_SOCK_SUCCESS)
        {
            QLOGE("WEB ERR CONNECT FAIL");
            client_ptr->result = QCM_WEB_ERR_TCP_CONNECT_FAIL;
            qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, client_ptr->client_id, QOSA_NULL);

            return QCM_WEB_ERR_OK;  //Return directly, subsequent handling of connection failure will be done by QCM_WEB_MSG_TCP_OPEN_CNF
        }

        //Handle WSS and non-WSS connections separately
#ifdef CONFIG_QCM_VTLS_FUNC
        if (client_ptr->url.scheme == WS_SCHEME_WSS)
        {
            //Processing logic for WSS connections
            qcm_ws_msg_send(QCM_WEB_MSG_TLS_HANDSHAKE, client_ptr->client_id, QOSA_NULL);
        }
        else
#endif
        {
            //Processing logic for WebSocket connections
            if (client_ptr->timer)
            {
                qosa_timer_stop(client_ptr->timer);
            }
            qcm_ws_msg_send(QCM_WEB_MSG_HTTP_REQUEST, client_ptr->client_id, QOSA_NULL);
        }
    }
    else if (evt_ptr->event_mask & QCM_SOCK_ACCEPT_EVENT)
    {
        QLOGD("QCM_SOCK_ACCEPT_EVENT");  //Print the accepted connection event information
    }
    else if (evt_ptr->event_mask & QCM_SOCK_CLOSE_EVENT)
    {
        QLOGD("QCM_SOCK_CLOSE_EVENT");  //Print close connection event information
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_CLOSING, client_ptr->client_id, QOSA_NULL);
    }
    else if (evt_ptr->event_mask & QCM_SOCK_SSL_HD_TIMEOUT_EVENT)
    {
        QLOGD("QCM_SOCK_SSL_HD_TIMEOUT_EVENT");  //Print SSL handshake timeout event information
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_CLOSING, client_ptr->client_id, QOSA_NULL);
    }
    else if (evt_ptr->event_mask & QCM_SOCK_SENDENDACK_EVENT)
    {
        QLOGD("QCM_SOCK_SENDENDACK_EVENT");  //Print the transmission completion confirmation event information
    }
    else if (evt_ptr->event_mask & QCM_SOCK_NET_DOWN_EVENT)
    {
        QLOGD("QCM_SOCK_NET_DOWN_EVENT");  //Print network fault event information
        qcm_ws_msg_send(QCM_WEB_MSG_TCP_CLOSING, client_ptr->client_id, QOSA_NULL);
    }
    else if (evt_ptr->event_mask & QCM_SOCK_READ_EVENT)
    {
        QLOGD("QCM_SOCK_READ_EVENT=%d", client_ptr->status);  //Print read event information
        if (client_ptr->status == QCM_WEB_STATUS_HTTP_HANDSHAKE)
        {
            //Read HTTP response
            qcm_ws_msg_send(QCM_WEB_MSG_HTTP_RESPONSE, client_ptr->client_id, QOSA_NULL);
        }
        else if (client_ptr->status == QCM_WEB_STATUS_WEBSOCKET)
        {
            //This status indicates that the WebSocket connection has been successfully established.
            qcm_ws_socket_read(client_ptr->client_id);
        }
#ifdef CONFIG_QCM_VTLS_FUNC
        else if (client_ptr->status == QCM_WEB_STATUS_TLS_HANDSHAKE)
        {
            //WSS Handshake Processing
            qcm_ws_msg_send(QCM_WEB_MSG_TLS_HANDSHAKE, client_ptr->client_id, QOSA_NULL);
        }
#endif
    }
    else if (evt_ptr->event_mask & QCM_SOCK_WRITE_EVENT)
    {
        QLOGD("QCM_SOCK_WRITE_EVENT");  //Print write event information
        if (client_ptr->status == QCM_WEB_STATUS_WEBSOCKET)
        {
            qcm_ws_socket_send(client_ptr->client_id);
        }
    }

    return QCM_WEB_ERR_OK;  //Return execution successful
}

/**
 * @brief Open TCP WebSocket connection
 *
 * This function is used to initialize the TCP connection for the WebSocket client.
 *
 * @param msg points to a pointer to a message structure containing client information and parameters.
 * @return qcm_web_err_e returns the result status code of the operation, where success is QCM_WEB_ERR_OK and failure is other error codes.
 */
qcm_web_err_e qcm_ws_tcp_open(qcm_web_msg_t *msg)
{
    qcm_web_client_t *client_ptr = QOSA_NULL;  //Client pointer
    qcm_web_config_t *cfg_ptr = QOSA_NULL;
    int               ret = 0;                 //Operation return value
    int               client_id = 0;           //Client ID
    int               sock_hd = 0;             //File descriptor used to identify a TCP socket
    int               event_mask;              //Registered socket event mask
    int               timeout = 0;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    qosa_uint32_t     sim_id = 0;              //SIM Card ID
    qosa_uint32_t     pdp_id = 0;              //PDP Context ID
#endif

    QLOGD("enter");
    //Obtain the client ID and retrieve the client pointer based on the ID
    client_id = msg->param1;
    client_ptr = qcm_ws_client_get_ptr(client_id);

    //Get configuration information: SIM card ID and PDP context ID
    cfg_ptr = qcm_ws_cfg_get_all(client_ptr->config_id);
    if (cfg_ptr == QOSA_NULL)
    {
        ret = QCM_WEB_ERR_TCP_CONNECT_FAIL;
        QLOGE("ret=%d", ret);
        goto err;
    }
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    sim_id = cfg_ptr->conn.sim_cid;
    pdp_id = cfg_ptr->conn.pdp_cid;
#endif
    timeout = cfg_ptr->conn.timeout;
    //Create TCP socket
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    sock_hd = qcm_socket_create(sim_id, pdp_id, client_ptr->ai_family, QCM_SOCK_STREAM, QCM_TCP_PROTOCOL, 0 /*local_port*/, QOSA_FALSE);
#else
    sock_hd = qcm_socket_create(client_ptr->ai_family, QCM_SOCK_STREAM, QCM_TCP_PROTOCOL, 0 /*local_port*/, QOSA_FALSE);
#endif
    if (sock_hd <= 0)
    {
        //If the socket creation fails, return the connection failure error code.
        ret = QCM_WEB_ERR_TCP_CONNECT_FAIL;
        QLOGE("ret=%d", ret);
        goto err;
    }

    QLOGD("socketfd: %d", sock_hd);
    client_ptr->sock_hd = sock_hd;

    //Register the socket event callback function and set the event mask
    event_mask = QCM_SOCK_CONNECT_EVENT | QCM_SOCK_READ_EVENT | QCM_SOCK_WRITE_EVENT | QCM_SOCK_CLOSE_EVENT | QCM_SOCK_SSL_HD_TIMEOUT_EVENT;
    qcm_socket_register_event(sock_hd, event_mask, qcm_ws_client_socket_evt_cb, (void *)client_ptr);

    if (client_ptr->timer == QOSA_NULL)
    {
        qosa_timer_create(&client_ptr->timer, qcm_ws_tcp_open_timer_cb, client_ptr);
        qosa_timer_start(client_ptr->timer, timeout * 1000, 0);
    }

    //Attempt to connect to the TCP server
    ret = qcm_socket_connect(sock_hd, &client_ptr->remote_ip, client_ptr->url.port);
    QLOGD("connect: %d,%d", sock_hd, ret);
    if (ret == QCM_SOCK_SUCCESS)
    {
        //Connection succeeded immediately
        QLOGD("CONNECTED ");
        //Update client status to connected
        client_ptr->status = QCM_WEB_STATUS_TCP_CONNECTED;
    }
    else
    {
        //If the connection fails or is in progress (non-blocking), update the client status and return value accordingly
        if (QCM_SOCK_WODBLOCK == ret)
        {
            client_ptr->status = QCM_WEB_STATUS_TCP_CONNECTING;
            ret = QCM_WEB_ERR_OK;
            QLOGD("QCM_SOCK_WODBLOCK");
        }
        else
        {
            //Connection failed
            ret = QCM_WEB_ERR_TCP_CONNECT_FAIL;
            QLOGE("ret=%d", ret);
            goto err;
        }
    }
    //Returns the operation result status code
    return ret;

err:
    client_ptr->result = ret;
    qcm_ws_msg_send(QCM_WEB_MSG_TCP_OPEN_CNF, client_id, QOSA_NULL);
    return ret;
}

/**
 * Close TCP network connection
 *
 * This function is used to close the TCP network connection for a specified client. First, it will search for the corresponding client information based on the client ID provided in the incoming message,
 * Then set the socket's linger option to control the waiting time when closing the connection, finally close the socket connection, and send a close confirmation message.
 *
 * @param msg points to the message containing the operation parameters. The param1 field should be the client ID of the connection that needs to be closed.
 */
void qcm_ws_tcp_close(qcm_web_msg_t *msg)
{
    int                 client_id = 0;  //Client ID
    int                 sock_hd = 0;
    int                 close_timeout = 0;
    qcm_web_client_t   *client_ptr = QOSA_NULL;  //Client pointer
    qcm_web_config_t   *cfg_ptr = QOSA_NULL;
    qcm_socket_linger_t linger = {0};

    QLOGD("enter");
    //Get the client pointer based on the client ID in the message
    client_id = msg->param1;
    client_ptr = qcm_ws_client_get_ptr(client_id);

    if (client_ptr == QOSA_NULL)
    {
        return;
    }

    client_ptr->status = QCM_WEB_STATUS_TCP_CLOSING;
    //Get the close wait time, and set the linger option to control the close behavior

    cfg_ptr = qcm_ws_cfg_get_all(client_ptr->config_id);
    if (cfg_ptr == QOSA_NULL)
    {
        return;
    }
    close_timeout = cfg_ptr->close.wait_time;
    linger.on_off = 1;
    linger.linger_val = close_timeout;
    sock_hd = client_ptr->sock_hd;
    qcm_socket_set_opt(sock_hd, QCM_SOCK_LINGER_OPT, (void *)&linger);

    //Close the socket, this is a synchronous close, blocking wait
    qcm_socket_close(sock_hd);

    //Send close confirmation message
    qcm_ws_msg_send(QCM_WEB_MSG_TCP_CLOSE_CNF, client_id, QOSA_NULL);
}
