/*****************************************************************/ /**
* @file qcm_web_ws.c
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
#include "qosa_sys.h"
#if defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)
#include "qosa_ip_addr.h"
#endif
#include "qcm_web_ws.h"
#include "qcm_websocket.h"
#include "qcm_web_internal.h"
#include "qcm_web_http.h"

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

/**
 * Mask the source data using the given mask key and store the result in the destination buffer.
 *
 * @param masking_key An integer array of length 4 for the masking key. Used to perform XOR operation on each data byte.
 * @param src The character array of the source data.
 * @param src_len The length of the source data array.
 * @param dst A character array for the target data, with a length of at least src_len. Used to store the data after mask processing.
 */
static void qcm_ws_mask_data(int *masking_key, char *src, int src_len, char *dst)
{
    //RFC 6455 Section 5.1 states:
    //The client must mask all frames sent to the server: To prevent misinterpretation and tampering of data by network intermediaries (such as proxy servers), all frames sent by the client must be masked.
    //The server must not mask any frames sent to the client (the server acts as a trusted party and does not need to perform masking): frames sent by the server must not be masked. If the client detects that a frame sent by the server contains a mask, it must close the connection and may use status code 1002 (protocol error).

    int mod = 0;
    int j = 0;

    //Perform an XOR operation on each byte in the source data using a mask key, and store the result in the destination array.
    qosa_memset(dst, 0, src_len);  //First, clear the target array.

    for (j = 0; j < src_len; j++)
    {
        mod = j % 4;                           //Calculate which byte of the mask key should be used for XOR with the current byte.
        dst[j] = (src[j] ^ masking_key[mod]);  //Perform XOR operation and store the result into the destination array
    }
}

/**
 * Parsing WebSocket Frame Header
 *
 * This function is used to parse the data frame header of the WebSocket protocol, determining the frame length and type based on the header information.
 *
 * @param client_ptr points to the pointer of the web client structure.
 * @param data pointer to the data to be parsed.
 * @param data_len The length of the data to be parsed.
 * @return If parsing is successful, return QCM_WEB_ERR_OK; if the data length is insufficient or the client pointer is null, return QCM_WEB_ERROR_UNKNOW.
 */
static int qcm_ws_parse_frame_header(qcm_web_client_t *client_ptr, char *data, int data_len)
{
    web_socket_frame_t  *frame = QOSA_NULL;
    web_socket_header_t *header = QOSA_NULL;

    QOSA_UNUSED(frame);
    QOSA_UNUSED(header);

    //Check if the client pointer is null
    if (client_ptr == QOSA_NULL)
    {
        return QCM_WEB_ERROR_UNKNOW;
    }

    //Initialize the frame and frame header pointer
    frame = &client_ptr->r_frame;
    header = &client_ptr->r_frame.header;

    //Possible header length (data sent from the server is always unmasked, only need to consider the unmasked header length):
    //2 bytes (basic frame header, no extension; no mask) (payload size range is 0 to 125 bytes) *
    //6 bytes  (base frame header + mask key)
    //4 bytes (Base frame header + Extended payload length 2 bytes, Payload Length = 126; no mask) (Payload size range is 126 to 65535 bytes) *
    //8 bytes  (base frame header + extended payload length 2 bytes + mask key)
    //10 bytes (base frame header + extended payload length 8 bytes, Payload Length = 127; no mask) (payload size range is 65536 to 18446744073709551615 bytes) *
    //14 bytes (base frame header + extended payload length 8 bytes + mask key)

    //Check if the data length is at least 2 bytes, which is the minimum length of the frame header.
    if (data_len < 2)
    {
        if (1 == data_len)
        {
            frame->header_break = QOSA_TRUE;
            qosa_memset(frame->header_data, 0x00, 15);

            qosa_memcpy(frame->header_data, data, data_len);
            frame->header_data_len = data_len;

            QLOGE("2 bit header break");
            return QCM_WEB_ERR_OPERATION_PENDING;
        }

        //Here data_len is less than or equal to 0, which is an unknown error
        return QCM_WEB_ERROR_UNKNOW;
    }

    //Parse each field in the frame header.
    header->FIN = (data[0] & 0x80) != 0;
    header->RSV = (data[0] & 0x70) >> 4;
    header->opcode = (data[0] & 0x0F);
    header->MASK = (data[1] & 0x80) != 0;
    header->payload_len = data[1] & 0x7F;

    //Initialize frame-related length to 0
    frame->header_len = 0;
    frame->data_len = 0;
    frame->read_len = 0;

    QLOGD("MASK: %d", header->MASK);
    if (1 == header->MASK)
    {
        //Upon receiving data containing a mask sent by the server, it can be directly closed (RFC 6455 Section 5.1 stipulates: frames sent by the server are not allowed to use a mask)
        //The server must not mask any frames sent to the client (the server acts as a trusted party and does not need to perform masking).
        //Set the close flag to true, close this connection, no further parsing is required.
        frame->close = QOSA_TRUE;

        frame->header_break = QOSA_FALSE;
        qosa_memset(frame->header_data, 0x00, 15);
        frame->header_data_len = 0;

        return QCM_WEB_ERR_UNSUPPORTED_PROTOCOL;
    }

    QLOGD("opcode: %d", header->opcode);

    //Judgment of Control Frames
    if (!(header->opcode == OPCODE_CONTINUATION || header->opcode == OPCODE_TEXT || header->opcode == OPCODE_BINARY))
    {
        //First, determine whether the requirements of RFC 6455 5.5 are met.
        // 5.5. Control Frames:
        // All control frames MUST have a payload length of 125 bytes or less and MUST NOT be fragmented.
        //The payload length of all control frames must be less than or equal to 125 bytes and must not be fragmented.

        //Control frames that do not comply with RFC specifications refer to libwebsocket and are directly handled as close. Subsequent parsing is also unnecessary.
        if (125 < header->payload_len || header->FIN != 1)
        {
            QLOGD("payload_len: %d", header->payload_len);
            QLOGD("FIN: %d", header->FIN);

            frame->close = QOSA_TRUE;

            frame->header_break = QOSA_FALSE;
            qosa_memset(frame->header_data, 0x00, 15);
            frame->header_data_len = 0;

            return QCM_WEB_ERR_UNSUPPORTED_PROTOCOL;
        }
    }

    //Calculate frame header length and data length based on opcode and payload length
    if (header->opcode == OPCODE_CONTINUATION || header->opcode == OPCODE_TEXT || header->opcode == OPCODE_BINARY || header->opcode == OPCODE_PING
        || header->opcode == OPCODE_PONG)
    {
        frame->header_len += 2;  //The frame header must contain at least 2 bytes.

        QLOGD("payload_len: %d", header->payload_len);
        QLOGD("data_len: %d", data_len);

        if (header->payload_len < 126)
        {
            //2 bytes (basic frame header, no extension; no mask)
            //The 2-byte length has already been checked earlier, so no need to repeat the check here.

            frame->data_len = header->payload_len;
        }
        else if (header->payload_len == 126)
        {
            //4 bytes (basic frame header + extended payload length 2 bytes, Payload Length = 126; no mask)

            if (data_len < 4)
            {
                frame->header_break = QOSA_TRUE;
                qosa_memset(frame->header_data, 0x00, 15);

                qosa_memcpy(frame->header_data, data, data_len);
                frame->header_data_len = data_len;

                QLOGE("4 bit header break");
                return QCM_WEB_ERR_OPERATION_PENDING;
            }

            //Parse the 16-bit payload length
            frame->data_len = (data[2] << 8) | data[3];
            frame->header_len += 2;
        }
        else
        {
            //10 bytes (base frame header + extended payload length 8 bytes, Payload Length = 127; no mask)
            if (data_len < 10)
            {
                frame->header_break = QOSA_TRUE;
                qosa_memset(frame->header_data, 0x00, 15);

                qosa_memcpy(frame->header_data, data, data_len);
                frame->header_data_len = data_len;

                QLOGE("10 bit header break");
                return QCM_WEB_ERR_OPERATION_PENDING;
            }

            //Parse the 64-bit payload length
            frame->data_len = ((qosa_uint64_t)data[2] << 56) | ((qosa_uint64_t)data[3] << 48) | ((qosa_uint64_t)data[4] << 40) | ((qosa_uint64_t)data[5] << 32)
                              | ((qosa_uint64_t)data[6] << 24) | ((qosa_uint64_t)data[7] << 16) | ((qosa_uint64_t)data[8] << 8) | (qosa_uint64_t)data[9];

            frame->header_len += 8;
        }

#if 0
        //Since frames sent by the server are not allowed to use masking, handling header->MASK == 1 is meaningless, so it is commented out.

        //If the MASK bit is 1, read and record the mask key
        if (header->MASK == 1)
        {
            if (data_len < frame->header_len + 4)
            {
                frame->header_break = QOSA_TRUE;
                qosa_memset(frame->header_data, 0x00, 15);

                qosa_memcpy(frame->header_data, data, data_len);
                frame->header_data_len = data_len;

                QLOGD("Masking-Key header break");
                return QCM_WEB_ERR_OPERATION_PENDING;
            }

            frame->masking_key[0] = data[frame->header_len];
            frame->masking_key[1] = data[frame->header_len + 1];
            frame->masking_key[2] = data[frame->header_len + 2];
            frame->masking_key[3] = data[frame->header_len + 3];
            frame->header_len += 4;
        }
#endif
        QLOGD("masking_key=%x %x %x %x", frame->masking_key[0], frame->masking_key[1], frame->masking_key[2], frame->masking_key[3]);
        QLOGD("header_len=%d,data_len=%d", frame->header_len, frame->data_len);

        //If the frame contains data, set the parsing type to BODY.
        if (frame->data_len > 0)
        {
            frame->parse_type = WS_PARSE_TYPE_BODY;
        }
    }
    //Processing of the close frame (code omitted here)
    else if (header->opcode == OPCODE_CLOSE)
    {
        //qcm_ws_parse_frame will handle the closing process, nothing needs to be done here
    }
    //Processing of other opcodes (code omitted here)
    else
    {
        //qcm_ws_parse_frame will handle the closing process, nothing needs to be done here
    }

    //Print frame header information for debugging
    QLOGD("fin=%d,rsv=%d,opcode=0x%x,MASK=%d,payload_len=%d", header->FIN, header->RSV, header->opcode, header->MASK, header->payload_len);

    //The current frame header is complete, and all frame header records are set to 0.
    frame->header_break = QOSA_FALSE;
    qosa_memset(frame->header_data, 0x00, 15);
    frame->header_data_len = 0;

    return QCM_WEB_ERR_OK;
}

/**
 * Parse WebSocket frame body
 *
 * This function is used to parse the data frame body of the WebSocket protocol. Based on the given source data, it parses and stores the result into the target buffer.
 *
 * @param client_ptr points to a pointer of the qcm_web_client_t structure, used to store the client's web connection status and data.
 * @param src Pointer to the source data buffer.
 * @param src_len The length of the source data buffer.
 * @param dst Pointer to the destination data buffer, used to store the parsed data.
 * @param dst_len Pointer to the current used length of the destination data buffer; this length will be updated after the function executes.
 * @param jump_len The length of leading bytes to skip, typically used to skip non-data parts such as frame headers.
 * @return Returns the actual length of data parsed and copied. If the source data length is insufficient, returns -1.
 */
static int qcm_ws_parse_frame_body(qcm_web_client_t *client_ptr, char *src, int src_len, char *dst, int *dst_len, int jump_len)
{
    int len = 0;
    int remain_len = 0;

    //Check whether the source data length has been fully processed
    if (src_len < jump_len)
    {
        return -1;
    }

    //Calculate the length of data that can be processed this time
    QLOGD("src_len=%d, jump_len=%d", src_len, jump_len);
    len = (src_len - jump_len);  //The amount of readable data in src
    QLOGD("data len=%d,dst_len=%d", len, *dst_len);

    //Calculate the remaining data length to be processed within the current frame.
    //In the currently processed frame, how much more data needs to be read to reach the Payload length
    //Payload length - Length of read data payload = Remaining payload length to be read in the current frame
    remain_len = (client_ptr->r_frame.data_len - client_ptr->r_frame.read_len);
    QLOGD("remain_len: %d", remain_len);

    //Determine whether the data of the current frame can be fully processed
    //The amount of readable data in src >= the remaining payload length of the current frame that needs to be read
    if (len >= remain_len)
    {
        //If the current frame data can be fully processed, set the next frame parsing type to header parsing.
        len = remain_len;
        client_ptr->r_frame.parse_type = WS_PARSE_TYPE_HEADER;
    }

    //Check if the data needs to be masked (masking of received data is unnecessary)
    //RFC 6455 Section 5.1 specifies: The server must not mask any frames sent to the client: frames sent by the server must not be masked. If a client detects that a frame sent by the server contains a mask, it must close the connection and may use status code 1002 (protocol error).
    if (client_ptr->r_frame.header.MASK == 1)
    {
        //If mask processing is required, call the qcm_ws_mask_data function to perform data masking.
        qcm_ws_mask_data(client_ptr->r_frame.masking_key, src + jump_len, len, dst + *dst_len);
    }
    else
    {
        //If mask processing is not required, copy the data directly.
        qosa_memcpy(dst + *dst_len, src + jump_len, len);
    }

    //Update the used length of the target buffer and the read length of the current frame
    *dst_len += len;
    client_ptr->r_frame.read_len += len;
    QLOGD("read_len=%d", client_ptr->r_frame.read_len);
    QLOGD("len=%d,dst_len=%d", len, *dst_len);

    return len;
}

/**
 * Skip the current WebSocket frame payload part
 *
 * This function is used to skip the payload part of PING/PONG frames (for PING frames, only the first PING frame payload in a data stream needs to be recorded and replied with a PONG; other PING frames can be skipped directly).
 *
 * @param client_ptr points to a qcm_web_client_t structure, used to store the client's web connection state and data.
 * @param src_len length of the source data buffer
 * @param jump_len is the length of leading bytes to skip, and this jump_len length is updated
 * @return Returns whether the current frame's payload part was successfully skipped.
 */
static qosa_bool_t qcm_ws_parse_frame_jump_payload(qcm_web_client_t *client_ptr, int src_len, int *jump_len)
{
    web_socket_frame_t *r_frame = QOSA_NULL;
    int                 payload_len = 0;
    int                 read_len = 0;
    int                 remain_payload_len = 0;

    r_frame = &client_ptr->r_frame;

    payload_len = r_frame->data_len;              //Length of the frame's payload
    read_len = r_frame->read_len;                 //Frame read payload length (since it is directly skipped, the read_len here is equivalent to the length already skipped)
    remain_payload_len = payload_len - read_len;  //Remaining unread payload length

    QLOGD("payload_len: %d, read_len: %d, remain_payload_len: %d", payload_len, read_len, remain_payload_len);
    QLOGD("src_len: %d, jump_len: %d", src_len, *jump_len);

    if (src_len < *jump_len + remain_payload_len)
    {
        //The data volume in src_len is insufficient to construct a complete PING/PONG frame payload.
        //then it must wait for the next data stream to arrive before it can continue assembling

        int current_read = src_len - *jump_len;  //The newly read length

        read_len = read_len + current_read;      //Update PING/PONG frame read (skipped) payload length
        r_frame->read_len = read_len;

        r_frame->parse_type = WS_PARSE_TYPE_BODY;  //When it arrives next time, it is necessary to continue constructing the PING/PONG frame payload that was not completed this time.
        *jump_len += current_read;                 //Skip read parts

        QLOGD("Payload not fully processed");

        return QOSA_FALSE;  //The payload part of the current frame was not skipped, returning false
    }
    else
    {
        //Completed the payload construction for the current frame of PING/PONG.
        read_len = read_len + remain_payload_len;  //Here, read_len is the length of payload_len.
        r_frame->read_len = read_len;

        r_frame->parse_type = WS_PARSE_TYPE_HEADER;  //Prepare for parsing the next frame (frames requiring payload processing will be set in qcm_ws_parse_frame_body)
        *jump_len += remain_payload_len;             //The payload portion of the frame has been completely skipped.

        QLOGD("jump all payload");
        return QOSA_TRUE;  //The payload portion of the current frame has been completely skipped, returning true.
    }
}

/**
 * Parsing WebSocket Frames
 *
 * @param client_id Client ID
 * @param src Input data source, points to the WebSocket frame data to be parsed
 * @param src_len Input data length
 * @param dst Output data destination, used to store the parsed data (only contains the payload content of data frames; non-data frame payload information is not recorded here)
 * @param dst_len points to the pointer of output data length, used to record the actual output data length
 * @return Always returns 0, possible errors are indicated through other means (such as setting a status code).
 */
int qcm_ws_parse_frame(int client_id, char *src, int src_len, char *dst, int *dst_len)
{
    //Get client pointer by client ID
    qcm_web_client_t    *client_ptr = qcm_ws_client_get_ptr(client_id);
    web_socket_frame_t  *r_frame = QOSA_NULL;
    web_socket_header_t *header = QOSA_NULL;
    int                  jump_len = 0;  //Skip length of parsed data
    int                  ret = 0;

    QLOGD("enter src_len=%d", src_len);
    //Parameter and Data Source Check
    if (client_ptr == QOSA_NULL || src == QOSA_NULL || src_len == 0 || dst == QOSA_NULL)
    {
        return 0;
    }

    //Initialize the frame parsing related pointers
    r_frame = &client_ptr->r_frame;
    header = &client_ptr->r_frame.header;
    *dst_len = 0;  //Initialize output length to 0

    //Loop processing until all parsable frames in the data stream (src) are completely parsed.
    while (1)
    {
        QLOGD("src_len: %d, jump_len: %d", src_len, jump_len);
        if (src_len <= jump_len)
        {
            break;
        }

        QLOGD("parse_type: %d", r_frame->parse_type);

        //Parse frame header
        if (r_frame->parse_type == WS_PARSE_TYPE_HEADER)
        {
            ret = qcm_ws_parse_frame_header(client_ptr, src + jump_len, (src_len - jump_len));
            if (ret != QCM_WEB_ERR_OK)
            {
                break;
            }
            jump_len += r_frame->header_len;  //Update the length of skipped leading bytes
        }

        //Process the frame body according to the frame opcode
        QLOGD("opcode: %d", header->opcode);

        if (header->opcode == OPCODE_CONTINUATION || header->opcode == OPCODE_TEXT || header->opcode == OPCODE_BINARY)
        {
            //Processing of Data Frames
            if (r_frame->parse_type == WS_PARSE_TYPE_BODY)
            {
                ret = qcm_ws_parse_frame_body(client_ptr, src, src_len, dst, dst_len, jump_len);
                if (ret < 0)
                {
                    break;
                }
                jump_len += ret;  //Update skipped length

                if (r_frame->parse_type == WS_PARSE_TYPE_BODY)
                {
                    //In qcm_ws_parse_frame_body, the data of the current frame cannot be fully processed (the amount of readable data in src < the remaining payload length that needs to be read for the current frame).
                    //If the data in src is insufficient for the length of the current frame (the remaining data in src < the Payload length of the frame being processed), then return via break to qcm_ws_socket_read first, and output the payload that has already been read.
                    //When subsequent data arrives, qcm_socket_read will continue reading data and will call qcm_ws_parse_frame again.
                    //Since parse_type is WS_PARSE_TYPE_BODY, the next time it enters, it will directly process the payload of the data frame.
                    QLOGD("Payload not fully processed");
                    break;
                }
            }
        }
        else
        {
            //Based on the type of the control frame, perform the corresponding processing.
            if (header->opcode == OPCODE_PING)
            {
                int payload_len = r_frame->data_len;
                QLOGD("ping payload_len: %d", payload_len);

                if (0 == payload_len && r_frame->ping != QOSA_TRUE)
                {
                    //A ping frame without load, and this data stream has no ping frame record so far.
                    QLOGD("ping no payload");

                    WEB_FREE_PTR(r_frame->ping_data);
                    r_frame->ping = QOSA_TRUE;  //The following will process when r_frame->ping is QOSA_TRUE
                    r_frame->ping_data = QOSA_NULL;
                    r_frame->ping_len = 0;
                }

                if (QOSA_TRUE == r_frame->ping)
                {
                    // RFC 6455 5.5.3. Pong: If an endpoint receives a Ping frame and has not yet sent Pong frame(s) in response to previous Ping frame(s), the endpoint MAY elect to send a Pong frame for only the most recently processed Ping frame.
                    //If an endpoint receives a Ping frame and has not yet sent a Pong frame in response to a previous Ping frame, the endpoint may choose to send a Pong frame only for the most recently processed Ping frame.

                    //Reference libwebsocket implementation: if a set of data contains multiple ping frames, only the first ping frame in that set needs to be replied to.
                    //Subsequent ping frame processing is the same as pong frame, directly skip the payload part.

                    qosa_bool_t jump_payload = QOSA_FALSE;

                    QLOGD("jump ping payload");
                    jump_payload = qcm_ws_parse_frame_jump_payload(client_ptr, src_len, &jump_len);
                    if (jump_payload)
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }

                //Processing of PING Frame Payload
                if (r_frame->parse_type == WS_PARSE_TYPE_BODY)
                {
                    char *ping_payload = QOSA_NULL;
                    int   ping_dst_len = 0;

                    QLOGD("Record ping payload");

                    //The ping frame that reaches this point must have a payload (payload_len is not 0).

                    if (QOSA_NULL == r_frame->ping_data && 0 < r_frame->read_len)
                    {
                        //Here, r_frame->ping is QOSA_FALSE

                        // QOSA_NULL == r_frame->ping_data && 0 < r_frame->read_len
                        //Indicates that the last packet parsed by qcm_ws_parse_frame was a ping frame that needed to be skipped (the data payload of the ping frame that needs to be sent (the ping frame to be sent is not the one currently being processed) has already been recorded for the pong frame to be sent), and the payload of this ping frame was not entirely skipped.

                        //Here continues the part of the payload that was not skipped during the parsing of the PING frame in qcm_ws_parse_frame (this part is also directly skipped)

                        qosa_bool_t jump_payload = QOSA_FALSE;

                        QLOGD("jump ping payload");
                        jump_payload = qcm_ws_parse_frame_jump_payload(client_ptr, src_len, &jump_len);
                        if (jump_payload)
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else if (QOSA_NULL != r_frame->ping_data && 0 < r_frame->read_len)
                    {
                        //0 < r_frame->read_len, indicating that it entered WS_PARSE_TYPE_BODY parsing without executing qcm_ws_parse_frame_header (calling qcm_ws_parse_frame_header would initialize r_frame->read_len to 0)
                        //Indicates that the parsing of the previous ping frame payload is not yet complete; the previously parsed payload and the currently parsed payload need to be concatenated to attempt to form a complete ping frame payload (only after the entire ping frame payload is received will a PONG frame be sent to the server)

                        QLOGD("Splicing ping payload");
                        ping_payload = r_frame->ping_data;  //Following the previous direct read, the length of the r_frame->ping_data space is exactly the r_frame->data_len length allocated by the previous malloc.
                        ping_dst_len = r_frame->read_len;  //Offset to the address after the last read data, continue receiving new data, and concatenate into a complete payload
                    }
                    else
                    {
                        QLOGD("Create a new Ping payload");
                        ping_payload = (char *)qosa_malloc(payload_len + 1);
                        if (QOSA_NULL == ping_payload)
                        {
                            QLOGE("no mem");
                            break;
                        }
                        qosa_memset(ping_payload, 0x00, payload_len + 1);

                        WEB_FREE_PTR(r_frame->ping_data);
                    }

                    ret = qcm_ws_parse_frame_body(client_ptr, src, src_len, ping_payload, &ping_dst_len, jump_len);

                    if (ret < 0)
                    {
                        WEB_FREE_PTR(ping_payload);
                        break;
                    }
                    else
                    {
                        if (r_frame->read_len == r_frame->data_len)
                        {
                            //Only when the read data length equals the payload length does it indicate that the entire ping frame payload has been fully processed.
                            //Only after the ping frame is fully parsed can the response pong frame be prepared for sending.
                            QLOGD("parse all ping");
                            r_frame->ping = QOSA_TRUE;
                        }
                        r_frame->ping_data = ping_payload;
                        r_frame->ping_len = payload_len;
                    }

                    jump_len += ret;  //Update skipped length

                    if (r_frame->parse_type == WS_PARSE_TYPE_BODY)
                    {
                        //If the data in src is insufficient for the current frame length (the remaining data in src < the Payload length of the frame being processed), return to qcm_ws_socket_read first, and qcm_socket_read will continue reading data later.
                        //Since parse_type is WS_PARSE_TYPE_BODY, the next entry will directly perform payload processing.
                        QLOGD("Payload not fully processed");
                        break;
                    }
                }
            }
            else if (header->opcode == OPCODE_PONG)
            {
                // RFC 6455 5.5.3. Pong: A Pong frame MAY be sent unsolicited. This serves as a unidirectional heartbeat. A response to an unsolicited Pong frame is not expected.

                //The server responded with PONG, no action is required; if the pong contains payload information, simply skip it.
                qosa_bool_t jump_payload = QOSA_FALSE;

                jump_payload = qcm_ws_parse_frame_jump_payload(client_ptr, src_len, &jump_len);
                if (jump_payload)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            else
            {
                //close frame, or other undefined control frame types
                //Refer to libwebsocket for shutdown handling
                QLOGE("close");
                r_frame->close = QOSA_TRUE;
                break;
            }
        }
    }

    return 0;  //The function always returns 0
}

/**
 * Construct WebSocket frame body
 *
 * This function is used to construct the WebSocket frame body based on the input data source. If the data source contains the MASK flag, the data will be processed with masking.
 *
 * @param client_ptr points to a pointer of the qcm_web_client_t structure, containing the construction context of the WebSocket frame.
 * @param src points to the source data.
 * @param src_len The length of the source data.
 * @param dst Pointer to the destination data buffer.
 * @param dst_len Pointer to the current length of the destination data buffer, which will be updated to the processed length after the function executes.
 * @return The function always returns 0.
 */
static int qcm_ws_construct_frame_body(qcm_web_client_t *client_ptr, char *src, int src_len, char *dst, int *dst_len)
{
    web_socket_frame_t  *frame = QOSA_NULL;
    web_socket_header_t *header = QOSA_NULL;

    //Get pointers to WebSocket frame and header
    frame = &client_ptr->s_frame;
    header = &client_ptr->s_frame.header;

    //If the source data is empty or has a length of 0, set the parsing type to HEADER and return.
    if (src == QOSA_NULL || src_len == 0)
    {
        frame->parse_type = WS_PARSE_TYPE_HEADER;
        return 0;
    }

    //If the MASK flag in the frame header is 1, apply mask processing to the data.
    if (header->MASK)
    {
        qcm_ws_mask_data(frame->masking_key, src, src_len, dst + *dst_len);
    }
    else
    {
        //If no mask is required, copy the data directly to the target buffer.
        qosa_memcmp(dst + *dst_len, src, src_len);
    }

    //Update target buffer length and written length
    *dst_len += src_len;
    frame->write_len += src_len;

    //If the written length reaches or exceeds the data length, set the parsing type to HEADER.
    if (frame->write_len >= frame->data_len)
    {
        frame->parse_type = WS_PARSE_TYPE_HEADER;
    }
    return 0;
}

/**
 * Construct WebSocket frame header
 *
 * This function is used to construct the WebSocket protocol data frame header based on the given parameters. The WebSocket frame header contains control information and data length information.
 *
 * @param client_ptr points to a pointer of the qcm_web_client_t structure, containing the relevant state and data for the WebSocket connection.
 * @param opcode specifies the operation code of the frame, indicating the data type of the frame.
 * @param src Pointer to the source data. If the source data needs to be masked, it is masked using a randomly generated mask key.
 * @param src_len The length of the source data.
 * @param dst Pointer to the destination buffer, used to store the constructed frame header.
 * @param dst_len points to the length of the destination buffer. After the function completes, this parameter will be updated to the actual length of the buffer used.
 * @return The function returns 0 upon successful execution.
 */
static int qcm_ws_construct_frame_header(qcm_web_client_t *client_ptr, web_opcode_e opcode, char *src, int src_len, char *dst, int *dst_len)
{
    web_socket_frame_t  *frame = QOSA_NULL;
    web_socket_header_t *header = QOSA_NULL;
    qosa_uint8_t         rand_buf[32] = {0};  //Buffer for generating random strings
    int                  len = *dst_len;

    QOSA_UNUSED(src);
    frame = &client_ptr->s_frame;
    header = &client_ptr->s_frame.header;

    //Initialize the basic fields of the frame header
    header->FIN = 1;
    header->RSV = 0;
    header->opcode = opcode;

    //According to the WebSocket protocol (RFC 6455), all frames sent from the client to the server must be masked, regardless of whether the frame contains payload data.
    //Set the mask flag and generate a random mask key
    header->MASK = 1;
    //Get a 4-byte long random string (whether truly random depends on system implementation)
    qcm_ws_get_random(rand_buf, 4);
    frame->masking_key[0] = rand_buf[0];
    frame->masking_key[1] = rand_buf[1];
    frame->masking_key[2] = rand_buf[2];
    frame->masking_key[3] = rand_buf[3];

    //Construct the first byte of the frame header
    dst[len] = (header->FIN ? 0x80 : 0x00) | (header->RSV << 4) | opcode;

    //Based on the data length, determine how to encode the payload length.
    if (src_len < 126)
    {
        header->payload_len = src_len;
        frame->data_len = src_len;
        dst[len + 1] = (header->MASK ? 0x80 : 0x00) | header->payload_len;
        len += 2;
    }
    else if (src_len >= 126 && src_len <= 0xffff)
    {
        header->payload_len = 126;
        frame->data_len = src_len;

        dst[len + 1] = (header->MASK ? 0x80 : 0x00) | header->payload_len;
        dst[len + 2] = (frame->data_len >> 8) & 0xFF;
        dst[len + 3] = frame->data_len & 0xFF;
        len += 4;
    }
    else
    {
        header->payload_len = 127;
        frame->data_len = src_len;
        int offset = (len + 2);
        dst[len + 1] = (header->MASK ? 0x80 : 0x00) | header->payload_len;
        //Encode the 64-bit data length in big-endian byte order
        int i = 7;
        for (i = 7; i >= 0; --i)
        {
            dst[offset++] = (frame->data_len >> (i * 8)) & 0xFF;
        }
        len += 10;
    }
    //If a mask is set, write the mask key into the frame header
    if (header->MASK)
    {
        dst[len] = frame->masking_key[0];
        dst[len + 1] = frame->masking_key[1];
        dst[len + 2] = frame->masking_key[2];
        dst[len + 3] = frame->masking_key[3];
        len += 4;
    }
    *dst_len = len;
    frame->header_len = len;
    QLOGD("header_len: %d", frame->header_len);

    //Update the frame status, prepare to receive or send the data body.
    frame->parse_type = WS_PARSE_TYPE_BODY;
    frame->write_len = 0;

    QLOGD("dst[0]=%2x,dst[1]=%2x", dst[0], dst[1]);

    return 0;
}

/**
 * Construct WebSocket frames
 *
 * This function is used to construct a WebSocket frame based on the given client ID, opcode, source data, and its length, and store the frame in the target buffer.
 *
 * @param client_id Client ID, used to identify the client initiating the WebSocket request.
 * @param opcode, representing the type of WebSocket frame (such as text frame, binary frame, close frame, etc.).
 * @param src Source data pointer, pointing to the data that needs to be encapsulated into a WebSocket frame.
 * @param src_len source data length, indicating the length of the data pointed to by src.
 * @param dst Pointer to the destination buffer, used to store the constructed WebSocket frame.
 * @param dst_len Pointer to the length of the destination buffer. As an input parameter, it points to the data length that the destination buffer can accommodate; as an output parameter, it will be updated to the actual filled data length.
 * @return The function returns 0 on success, otherwise returns an error code.
 */
int qcm_ws_construct_frame(int client_id, web_opcode_e opcode, char *src, int src_len, char *dst, int *dst_len)
{
    //Get client pointer by client ID
    qcm_web_client_t *client_ptr = qcm_ws_client_get_ptr(client_id);

    //Opcode and source data length upon entering the function
    QLOGD("enter opcode=%d,src_len=%d", opcode, src_len);

    //Check if the client pointer and target buffer pointer are null
    if (client_ptr == QOSA_NULL || dst == QOSA_NULL)
    {
        return 0;
    }

    //Initialize the target buffer length to 0
    *dst_len = 0;

    //Processing text frames, binary frames, and continuation frames
    //The construction method for PING frames and PONG frames is also the same.
    if (opcode == OPCODE_CONTINUATION || opcode == OPCODE_TEXT || opcode == OPCODE_BINARY || opcode == OPCODE_PING || opcode == OPCODE_PONG)
    {
        qosa_q_type_t        *s_frame_list = QOSA_NULL;
        web_socket_s_frame_t *s_frame_node = QOSA_NULL;

        //Current parsing type
        QLOGD("parse_type=%d", client_ptr->s_frame.parse_type);

        //If the current parsing type is header parsing, call the function to construct the frame header.
        if (client_ptr->s_frame.parse_type == WS_PARSE_TYPE_HEADER)
        {
            qcm_ws_construct_frame_header(client_ptr, opcode, src, src_len, dst, dst_len);
        }

        //Length of the target buffer after construction
        QLOGD("dst_len=%d", *dst_len);

        s_frame_list = &client_ptr->s_frame_list;
        //Use the new function to check if the queue has been initialized
        if (qosa_q_is_initialized(s_frame_list) == QOSA_FALSE)
        {
            qosa_q_init(s_frame_list);
        }

        s_frame_node = (web_socket_s_frame_t *)qosa_malloc(sizeof(web_socket_s_frame_t));
        if (s_frame_node != QOSA_NULL)
        {
            qosa_memset(s_frame_node, 0x00, sizeof(web_socket_s_frame_t));

            s_frame_node->opcode = opcode;
            s_frame_node->header_len = client_ptr->s_frame.header_len;
            s_frame_node->data_len = client_ptr->s_frame.data_len;

            //Add the newly constructed frame information to the linked list
            qosa_q_link((void *)s_frame_node, &s_frame_node->list);
            qosa_q_put(s_frame_list, &s_frame_node->list);
        }

        //If the current parsing type is body parsing, call the function to construct the frame body.
        if (client_ptr->s_frame.parse_type == WS_PARSE_TYPE_BODY)
        {
            qcm_ws_construct_frame_body(client_ptr, src, src_len, dst, dst_len);
        }

        //Length of the target buffer after construction
        QLOGD("dst_len=%d", *dst_len);

        return 0;
    }
    //Processing close frame
    else if (opcode == OPCODE_CLOSE)
    {
        //The handling of CLOSE is that the module directly closes the TCP connection, without needing to send a CLOSE frame.
        return 0;
    }
    //For unhandled opcodes, return directly
    else
    {
        return 0;
    }
}

/**
 * Parse WebSocket URL.
 *
 * This function is used to parse a given WebSocket URL string and store the parsing result in a structure.
 *
 * Example output (when parsing is successful): -----
 *
 * input_url=ws://www.example.com:8080/path/to/resource
 * scheme=ws
 * host=www.example.com
 * port=8080
 * path=/path/to/resource
 *
 * @param input_url Pointer to the WebSocket URL string to be parsed.
 * @param parsed_url points to the pointer of the structure used to store the parsing result.
 *
 * @return Function execution status code, status codes are defined in the qcm_web_err_e enumeration.
 *         - QCM_WEB_ERR_OK indicates successful parsing;
 *         - QCM_WEB_ERR_INVALID_URL_LENGTH indicates an invalid URL length;
 *         - QCM_WEB_ERR_UNSUPPORTED_PROTOCOL indicates an unsupported protocol;
 *         - QCM_WEB_ERR_INVALID_URL_FORMAT indicates an incorrect URL format;
 *         - QCM_WEB_ERR_INVALID_PORT indicates an invalid port number.
 */
qcm_web_err_e qcm_ws_parse_url(const char *input_url, web_socket_url *parsed_url)
{
    //Initialize structure members to 0
    WEB_FREE_PTR(parsed_url->host);
    WEB_FREE_PTR(parsed_url->path);
    qosa_memset(parsed_url, 0, sizeof(web_socket_url));

    //Check if the URL string length is valid
    qosa_size_t len = qosa_strlen(input_url);
    if (len < 5 || len > QCM_WEB_CFG_CONN_URL_LEN_MAX)
    {
        return QCM_WEB_ERR_INVALID_URL_FORMAT;
    }

    //Parse the protocol part of the URL
    if (qosa_strncmp(input_url, "ws://", 5) == 0)
    {
        parsed_url->scheme = WS_SCHEME_WS;  //Using the ws protocol
        input_url += 5;
    }
    else if (qosa_strncmp(input_url, "wss://", 6) == 0)
    {
        parsed_url->scheme = WS_SCHEME_WSS;  //Using the wss protocol
        input_url += 6;
    }
    else
    {
        return QCM_WEB_ERR_UNSUPPORTED_PROTOCOL;  //Unsupported protocol
    }

    //Parse the hostname part of the URL
    const char *host_start = input_url;
    const char *port_separator = qosa_strchr(input_url, ':');  //Find the delimiter for the port
    const char *host_end = qosa_strchr(input_url, '/');        //Find the first slash as the end marker for the hostname
    if (!host_end)
    {
        host_end = input_url + len;  //If there is only one or no '/', then take to the end of the string.
    }

    qosa_size_t host_len = port_separator ? (port_separator - host_start) : (host_end - host_start);

    //Remove the [] from the IPv6 address.
    const char *ipv6_start = qosa_strchr(host_start, '[');
    if (ipv6_start)
    {
        const char *ipv6_end = qosa_strchr(ipv6_start, ']');
        if (!ipv6_end || ipv6_end[1] != ':')
        {
            return QCM_WEB_ERR_INVALID_URL_FORMAT;  //IPv6 address format error
        }
        host_start = ipv6_start + 1;
        host_len = ipv6_end - host_start;
    }

    parsed_url->host = qosa_malloc(host_len + 1);
    if (parsed_url->host == QOSA_NULL)
    {
        return QCM_WEB_ERR_NOMEM;
    }
    qosa_memset(parsed_url->host, 0x00, host_len + 1);
    qosa_memcpy(parsed_url->host, host_start, host_len);  //Copy hostname to structure

    //Parse the port part of the URL (if present)
    if (port_separator)
    {
        parsed_url->port = (int)qosa_strtol(port_separator + 1, QOSA_NULL, 10);  //Parse port number
        if (parsed_url->port <= 0 || parsed_url->port > 65535)
        {
            return QCM_WEB_ERR_INVALID_PORT;  //Port number range is invalid
        }
    }
    else
    {
        parsed_url->port = parsed_url->scheme == WS_SCHEME_WS ? QCM_WEB_WS_PORT_DEFAULT : QCM_WEB_WSS_PORT_DEFAULT;  //Use the default port
    }

    //Parse the path portion of the URL
    if (*host_end == '/')  //Path exists
    {
        parsed_url->path = qosa_malloc(qosa_strlen(host_end) + 1);
        if (parsed_url->path == QOSA_NULL)
        {
            return 0;
        }
        qosa_memset(parsed_url->path, 0x00, qosa_strlen(host_end) + 1);
        qosa_memcpy(parsed_url->path, host_end, qosa_strlen(host_end));  //Copy path to structure (excluding the leading slash)
    }
    else                                                                 //No path
    {
        parsed_url->path = qosa_malloc(2);                               //Allocate an empty string
        if (parsed_url->path == QOSA_NULL)
        {
            return 0;
        }
        parsed_url->path[0] = '/';   //Set to an empty string
        parsed_url->path[1] = '\0';  //Set to an empty string
    }

    QLOGD("scheme=%d", parsed_url->scheme);
    QLOGD("host=%s", parsed_url->host);
    QLOGD("port=%d", parsed_url->port);
    QLOGD("path=%s", parsed_url->path);
    return QCM_WEB_ERR_OK;  //Parsing successful
}
