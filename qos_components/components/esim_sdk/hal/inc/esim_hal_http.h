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
#ifndef __ESIM_HAL_HTTP_H__
#define __ESIM_HAL_HTTP_H__
#include "qosa_def.h"
#include "qosa_sys.h"
#include "esim_error.h"

/*
The client connection eim CA certificate path, cannot be changed
If you need to configure SSL via a path, you can use;
When using the buffer method, it is not necessary to use
*/
#define ESIM_CRT_PATH "temp_cert_path"

/**
 * @enum esim_lpa_https_request_result_e
 * @brief LPA and SM-DP+ communication, HTTP data response result types
 */
typedef enum
{
    ESIM_HTTPS_REQUEST_SUCCESS = 0,
    ESIM_HTTPS_REQUEST_FAIL,
    ESIM_HTTPS_REQUEST_TIMEOUT,
} esim_https_request_result_e;

/**
 * @brief HTTP request internal error codes
 */
typedef enum
{
    ESIM_HTTP_OK = 0,
    ESIM_HTTP_ERROR_UNKNOWN,        /*!< Unknown error*/
    ESIM_HTTP_REQUEST_ERROR,        /*!< HTTP request error*/
    ESIM_HTTP_DOWN_WANT_SIZE_ERROR, /*!< HTTP download request SIZE exceeded limit*/
    ESIM_HTTP_NO_MEMORY_ERROR,      /*!< Memory Error*/
    ESIM_HTTP_URL_ERRO,             /*!< HTTP request URL error*/
    ESIM_HTTP_WRITE_SIZE_ERROR,     /*!< HTTP write body space error*/
} esim_http_req_err_e;

/**
 * @enum esim_http_msg_type_e
 * @brief For ESIM HTTP service thread message task processing
 */
typedef enum
{
    ESIM_DPP_HTTP_SEND_CMD = 0, /*!< ESIM HTTP active message event sent to DP+*/
    ESIM_EIM_HTTP_SEND_CMD,     /*!< ESIM HTTP active message event sent to EIM*/
    ESIM_HTTP_MSG_EVENT_REPORT, /*!< HTTP Service Layer Message Notification*/
} esim_http_msg_type_e;

/**
 * @struct esim_http_msg_info_t
 * @brief For HTTP message event notification
 */
typedef struct
{
    esim_http_msg_type_e cmd;  /*!< Corresponding HTTP message event type*/
    void                *argv; /*!< HTTP message content pointer address*/
} esim_http_msg_info_t;

/**
 * @struct esim_https_body_t
 * @brief Used to store the body data returned by the server
 */
typedef struct
{
    char *body;     /*!< If the execution is successful and reported, the returned body content; if successful, the SDK will release the memory.*/
    int   body_len; /*!< body length*/
} esim_https_body_t;

/**
 * @struct esim_https_resp_t
 * @brief For storing dp+ return data
 */
typedef struct
{
    esim_https_request_result_e result;    /*!< HTTP execution result status report*/
    esim_https_body_t           body_info; /*!< dp+ returns the HTTPS body information; if execution fails, body_info.body should be NULL*/
} esim_https_resp_t;

/**
 * @brief Explanation of the Internal Processing Mechanism of Callback Functions
 * @details The callback interface adopts a non-blocking design, delivering received data to an internal task queue via a notification mechanism.
 *
 * @param resp The HTTPS response body data returned by the server
 */
typedef void (*http_resp_cb)(esim_https_resp_t *resp);

/**
 * @enum esim_https_request_method_e
 * @brief Used to represent the HTTPS sending method
 */
typedef enum
{
    ESIM_HTTPS_METHOD_GET,
    ESIM_HTTPS_METHOD_POST
} esim_https_request_method_e;

/**
 * @struct esim_http_request_t
 * @brief HTTPS Request Configuration Structure
 */
typedef struct
{
    char                        domain[256];       /*!< Server domain name*/
    char                        path[256];         /*!< Request URL path*/
    esim_https_request_method_e type;              /*!< Request method type (POST/GET)*/
    char                       *header;            /*!< HTTP request header data*/
    int                         header_len;        /*!< Request header data length*/
    char                       *playload;          /*!< Request body data (Note: should be payload)*/
    int                         playload_len;      /*!< Request body data length*/
    int                         http_max_req_time; /*!< Request timeout (unit: seconds)*/

    /*Parameters related to one-way authentication*/
    char *trustedPK;     /*!< EIM server public key (currently not in use)*/
    int   trustedPK_len; /*!< EIM public key length*/
    char *CA_cert;       /*!< CA root certificate (used for one-way authentication)*/
    int   CA_cert_len;   /*!< CA certificate length*/

    http_resp_cb cb;     /*!< Response callback function, used to return the HTTPS response body to the SDK.*/
} esim_http_request_t;

/**
 * @brief Secure HTTPS communication with the SM-DP+/eIM server
 * This function supports both synchronous and asynchronous modes.
 * This function should use the parameters configured in config to send the request to the server and return the body data of the server's response to the SDK via the cb function in config.
 *
 * @param[in] esim_http_request_t *config
 *          - HTTPS request parameters
 *
 * @return esim_result_errno_e
 *          - Synchronization: Blocking wait for request completion, with results notified via callback
 *          - Asynchronous: Returns immediately, then notifies the result via a callback
 *          - Returns ESIM_OK on success
 *          - Returns other values on failure
 */
esim_result_errno_e esim_https_request_to_server(esim_http_request_t *config);

char esim_http_running_status_get(void);

void esim_http_process_init(void);
void esim_http_process_deinit(void);

#endif
