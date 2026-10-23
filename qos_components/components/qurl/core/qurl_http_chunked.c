/** @file         qurl_http_chunked.c
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-10-27 19:59:11
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qurl_config.h"
#include "qurl_code_in.h"
#include "qurl_log.h"
#include "qurl_def_in.h"
#include "qurl_core.h"
#include "qurl_conn.h"
#include "qurl_ctype.h"
#include "qurl_http_def.h"
#include "qurl_http_chunked.h"
#include "qurl_http.h"

#if QURL_CFG_ENABLE_HTTP

void qurl_http_chunked_init(qurl_conn_t *conn_ptr)
{
    qurl_http_chunked_ctrl_t *chunked_ptr = &conn_ptr->prot_data_u.http_ctrl.chunked;

    chunked_ptr->state = QURL_HTTP_CHUNKED_STATE_HEX; /*Wait for hex character*/
    chunked_ptr->data_size = 0;
    chunked_ptr->hex_index = 0;
    /*Should we use dbuf?*/
}

qurl_ecode_t qurl_http_chunked_read_body(qurl_conn_t *conn_ptr)
{
    qurl_bit_t                continue_flag = QOSA_TRUE;
    long                      valid_size = 0;
    long                      read_len = 0;
    long                      write_len = 0;
    long                      ret_len = 0;
    long                      buf_len = 0;
    char                     *buf_ptr = QOSA_NULL;
    qurl_core_ctrl_t         *core_ptr = conn_ptr->core_ptr;
    qurl_http_ctrl_t         *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);
    qurl_http_chunked_ctrl_t *chunked_ptr = &conn_ptr->prot_data_u.http_ctrl.chunked;

    /*If development from RAW to the client side is needed, implement it here.*/

    do {
        /*Process using a chunked state machine*/
        read_len = QURL_CONN_DEAL_CACHE_SIZE;
        qurl_dbuf_read_data_try(core_ptr->recv_cache, conn_ptr->deal_cache_ptr, &read_len, 0);
        buf_ptr = (char *)conn_ptr->deal_cache_ptr;

        continue_flag = QOSA_TRUE;
        buf_len = read_len;
        while (buf_len > 0 && continue_flag)
        {
            switch (chunked_ptr->state)
            {
                case QURL_HTTP_CHUNKED_STATE_HEX: {
                    if (qurl_ctype_isxdigit(*buf_ptr))
                    {
                        if (chunked_ptr->hex_index < QURL_HTTP_CHUNKED_NUMLEN_MAX)
                        {
                            chunked_ptr->hex_buf[chunked_ptr->hex_index] = *buf_ptr;
                            buf_ptr++;
                            buf_len--;
                            chunked_ptr->hex_index++;
                        }
                        else
                        {
                            return QURL_ECODE_HTTP_CHUNKED_HEX_TOO_LONG;
                        }
                    }
                    else
                    {
                        char *end_ptr = QOSA_NULL;
                        if (chunked_ptr->hex_index == 0)
                        {
                            /*Received illegal data*/
                            return QURL_ECODE_HTTP_CHUNKED_HEX_ILLEGAL;
                        }

                        /*After receiving the hex field*/
                        chunked_ptr->hex_buf[chunked_ptr->hex_index] = 0x00;
                        chunked_ptr->data_size = qurl_strtol(chunked_ptr->hex_buf, &end_ptr, 16);
                        if (end_ptr == QOSA_NULL)
                        {
                            return QURL_ECODE_HTTP_CHUNKED_HEX_ILLEGAL;
                        }
                        chunked_ptr->state = QURL_HTTP_CHUNKED_STATE_HEX_LF;
                    }
                    break;
                }
                case QURL_HTTP_CHUNKED_STATE_HEX_LF: {
                    /*After receiving the hex field, wait for an additional \n, then proceed to receive data of the specified length.*/
                    if (*buf_ptr == 0x0a)
                    {
                        /*Check if it is the last block*/
                        if (chunked_ptr->data_size == 0)
                        {
                            chunked_ptr->state = QURL_HTTP_CHUNKED_STATE_TRAILER;
                        }
                        else
                        {
                            chunked_ptr->state = QURL_HTTP_CHUNKED_STATE_DATA;
                        }
                    }

                    buf_ptr++;
                    buf_len--;
                    break;
                }
                case QURL_HTTP_CHUNKED_STATE_DATA: {
                    /*Receive data and decode for output*/
                    /*[lzm][note]Decoding is temporarily not supported.*/
                    write_len = QURL_MIN(buf_len, chunked_ptr->data_size);

                    if (core_ptr->usr_cfg.write_to_client_func && !http_ctrl_ptr->ignore_body)
                    {
                        core_idle_timeout_feed_dog(core_ptr);
                        ret_len = core_ptr->usr_cfg.write_to_client_func((unsigned char *)buf_ptr, write_len, core_ptr->usr_cfg.write_to_client_arg);
                        QURL_CORE_IDLE_TIMEOUT_CHECK_DOG_FUNC(core_ptr);
                        if (ret_len < write_len)
                        {
                            qurl_loge("write_to_client_func faild:%ld,%ld", buf_len, ret_len);
                            return QURL_ECODE_CLIENT_WRITE_FAILD;
                        }
                        core_ptr->trans.client_w_len += write_len;
                    }
                    buf_ptr += write_len;
                    buf_len -= write_len;
                    chunked_ptr->data_size -= write_len;
                    core_ptr->trans.downloaded_len += write_len;

                    if (chunked_ptr->data_size == 0)
                    {
                        /*The tail marker of the received data field is \r\n*/
                        chunked_ptr->state = QURL_HTTP_CHUNKED_STATE_DATA_LF;
                    }
                    break;
                }
                case QURL_HTTP_CHUNKED_STATE_DATA_LF: {
                    if (*buf_ptr == 0x0a)  // \n
                    {
                        /*Received successfully, ready to receive the next block*/
                        qurl_http_chunked_init(conn_ptr);
                    }
                    else if (*buf_ptr != 0x0d)
                    {
                        return QURL_ECODE_HTTP_CHUNKED_BAD;
                    }
                    buf_ptr++;
                    buf_len--;
                    break;
                }
                case QURL_HTTP_CHUNKED_STATE_TRAILER: {
                    char *end_ptr = QOSA_NULL;
                    char *trailer_ptr = QOSA_NULL;

                    /*Check for line terminator '\n'*/
                    end_ptr = qurl_memchr(buf_ptr, 0x0a, buf_len);

                    if (end_ptr == QOSA_NULL)
                    {
                        /*Incomplete line received, more data needed*/
                        continue_flag = QOSA_FALSE;
                        break;
                    }

                    if ((*buf_ptr == 0x0a) || (*buf_ptr == 0x0d))
                    {
                        /*Received*/
                        if (*buf_ptr == '\r')
                        {
                            buf_len--;
                            buf_ptr++;
                        }
                        if (*buf_ptr == '\n')
                        {
                            buf_len--;
                            buf_ptr++;
                        }
                        chunked_ptr->state = QURL_HTTP_CHUNKED_STATE_STOP; /*Received*/
                        conn_ptr->bits.chunked_ok = QOSA_TRUE;
                        break;
                    }

                    /*Received a complete line trailer*/
                    /*[lzm][todo]This is to be developed based on customer requirements; data will be printed in log format for now.*/
                    trailer_ptr = qurl_malloc(end_ptr - buf_ptr + 2);
                    if (trailer_ptr == QOSA_NULL)
                    {
                        qurl_loge("trailer:no mem\r\n");
                    }
                    else
                    {
                        trailer_ptr[end_ptr - buf_ptr + 1] = 0x00;
                        qurl_memcpy(trailer_ptr, buf_ptr, end_ptr - buf_ptr + 1);
                        qurl_loge("trailer:[%s]\r\n", trailer_ptr);
                        qurl_free(trailer_ptr);
                        trailer_ptr = QOSA_NULL;
                    }

                    /*Update cursor*/
                    buf_len -= (end_ptr - buf_ptr + 1);
                    buf_ptr = end_ptr + 1;

                    break;
                }
                case QURL_HTTP_CHUNKED_STATE_STOP: {
                    /*Normal protocol packets do not reach here.*/
                    qurl_loge("???\r\n");
                    break;
                }
                default:
                    break;
            }
        }
        /*Delete processed data*/
        qurl_dbuf_pull_data(core_ptr->recv_cache, QOSA_NULL, read_len - buf_len);
        valid_size = qurl_dbuf_vaild_size(core_ptr->recv_cache);
    } while (valid_size > buf_len); /*There is still unread data*/

    return QURL_OK;
}

#endif
