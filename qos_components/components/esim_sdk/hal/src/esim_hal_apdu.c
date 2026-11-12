/*****************************************************************/ /**
* @file esim_adapt.c
* @brief
* @author larson.li@quectel.com
* @date 2024-10-21
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-10-21 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#include "esim_hal_apdu.h"
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "esim_error.h"

#ifdef CONFIG_QOSA_EIGEN718_PLATFORM_FUNC
#include "qosa_dev_eigen.h"
#endif /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */

#define QOS_LOG_TAG LOG_TAG_ESIM

static qosa_msgq_t g_esim_apdu_msgq = QOSA_NULL;

qosa_uint8_t g_open_channel_cmd[5] = {0x00, 0x70, 0x00, 0x00, 0x01};
qosa_uint8_t g_terminal_capability_cmd[13] = {0x80, 0xAA, 0x00, 0x00, 0x08, 0xA9, 0x06, 0x83, 0x01, 0x03, 0x84, 0x01, 0x01};
qosa_uint8_t g_lpa_aid_cmd[21] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0xA0, 0x00, 0x00, 0x05, 0x59, 0x10, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0x89, 0x00, 0x00, 0x01, 0x00};
qosa_uint8_t g_close_channel_cmd[5] = {0x00, 0x70, 0x80, 0x00, 0x00};

#define ESIM_APDU_RESP_WAIT_TIME_MS_NORMAL 5000
#define ESIM_APDU_RESP_WAIT_TIME_MS_MAX    30000

int esim_apdu_init(void)
{
    int err = QOSA_OK;
#if defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
    /*!< Using the size of qosa_sim_generic_access_cnf_t directly on 160x to create a msg queue will result in failure.*/
    err = qosa_msgq_create(&g_esim_apdu_msgq, sizeof(esim_apdu_msg_info_t), 1);
#else /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
    err = qosa_msgq_create(&g_esim_apdu_msgq, sizeof(qosa_sim_generic_access_cnf_t), 1);
#endif /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
    if (err != QOSA_OK)
    {
        QLOGE("create g_esim_csim_msg fail");
    }
    return err;
}

void esim_apdu_deinit(void)
{
    qosa_msgq_delete(g_esim_apdu_msgq);
    g_esim_apdu_msgq = QOSA_NULL;
}

/**
* @brief Print the byte array in hexadecimal format
*
* @param[in] const qosa_uint8_t* buf
*          - Byte array to be printed
*
* @param[in] qosa_uint16_t len
*          - Length of the byte array to be printed
*/
void esim_hal_apdu_printf(const char *tag, const qosa_uint8_t *apdu, qosa_uint16_t apdu_len)
{
    char *hex_str = QOSA_NULL;
    int   i = 0;
    int   offest = 0;
    int   str_len = 0;
    char *print_str = QOSA_NULL;
    int   print_offest = 0;

#define APDU_PTINT_LEN_MAX 140

    QLOGI("hal apdu len :%d ", apdu_len);
    if (apdu_len == QOSA_NULL || apdu_len < 1)
    {
        return;
    }

    str_len = apdu_len * 2 + 2 + qosa_strlen(tag);
    hex_str = (char *)qosa_malloc(str_len);
    if (hex_str == QOSA_NULL)
    {
        QLOGI("Memory allocation failed\n");
        return;
    }
    qosa_memset(hex_str, 0x00, str_len);
    for (i = 0; i < apdu_len; i++)
    {
        qosa_snprintf(&hex_str[offest], (str_len - offest), "%02X", apdu[i]);
        offest += 2;
    }

    if (offest > APDU_PTINT_LEN_MAX)
    {
        print_str = (char *)qosa_malloc(APDU_PTINT_LEN_MAX + 1);

        for (i = 0; offest > print_offest; print_offest += APDU_PTINT_LEN_MAX)
        {
            qosa_memset(print_str, 0x00, APDU_PTINT_LEN_MAX + 1);
            qosa_strncpy(print_str, &hex_str[print_offest], APDU_PTINT_LEN_MAX);
            QLOGI("%s [%d] %s", tag, i, print_str);
        }
        qosa_free(print_str);
    }
    else
    {
        QLOGI("%s %s", tag, hex_str);
    }

    qosa_free(hex_str);
    hex_str = QOSA_NULL;
}

/*
 * EC616 has a SIM card sleep mechanism
 * When SIM card sleep is enabled, AT+CSIM cannot be used.
 * esim requires the use of the AT+CSIM command, so a SIM sleep configuration interface is added.
 */
#ifdef CONFIG_QOSA_EIGEN718_PLATFORM_FUNC

/**
 * @brief SIM Card set sleep state Function (Reference Count Management)
 *
 * @param [in] state
 *           - Target sleep state: QOSA_TRUE (allow sleep) or QOSA_FALSE (not allow sleep)
 *
 * @note Using a reference counting mechanism ensures that only the last disable operation actually restores the original state after multiple enables.
 */
void esim_set_sim_sleep_state(qosa_bool_t state)
{
    qosa_bool_t    original_state = QOSA_FALSE;   //the original sim sleep state
    static int     change_count = 0;              //State change reference counter, balancing enable/disable counts
    qosa_bool_t    state_cur = QOSA_FALSE;        //Current actual sim sleep state
    qosa_sim_err_e ret = QOSA_SIM_ERR_OK;         //Operation return value
    qosa_uint8_t   sim_id = 0;

    /*Record the original system state as the recovery baseline on the first call.*/
    if (change_count == 0)
    {
        //Obtain and record the current sim sleep state of the system
        qosa_sim_get_sim_sleep_state(sim_id, &original_state);
        QLOGI("Original SIM sleep state: %s", original_state == QOSA_TRUE ? "enabled" : "disabled");
        /*If the original state is disabled or the original state is the same as the target state, return directly to avoid unnecessary state modifications.*/
        if (original_state == QOSA_FALSE || original_state == state)
        {
            QLOGI("Do not modify SIM sleep state");
            return;
        }
    }

    /*sim sleep state processing branch*/
    if (state == QOSA_FALSE)
    {
        //Get the current actual state for comparison
        qosa_sim_get_sim_sleep_state(sim_id, &state_cur);
        if (state != state_cur)
        {
            //The current state differs from the target, performing the actual state setting.
            ret = qosa_sim_set_sim_sleep_state(sim_id, state);
            if (ret == QOSA_SIM_ERR_OK)
            {
                //State set successfully, reference count increased
                change_count++;
                QLOGI("SIM sleep state disabled, change count: %d", change_count);
            }
            else
            {
                //State setting failed
                QLOGI("SIM sleep state disable fail: %d", ret);
            }
        }
        else
        {
            //State is already disabled, only increasing the reference count
            change_count++;
            QLOGI("SIM sleep state is already disabled, change count: %d", change_count);
        }
    }
    else if (state == QOSA_TRUE)
    {
        if (change_count == 1)
        {
            /*The last enable request: restore the enabled state and decrement the reference count*/
            change_count--;  //Reference count reaches zero
            ret = qosa_sim_set_sim_sleep_state(sim_id, state);
            QLOGI("SIM sleep state enabled, change count: %d", change_count);
        }
        else if (change_count > 1)
        {
            /*Middleware enable request: only decrements the reference count, does not modify the actual state*/
            change_count--;
            QLOGI("Intermediate enable request - count decreased, change count: %d", change_count);
        }
        else if (change_count == 0)
        {
            /*Edge case: Reference count is already 0, ignore redundant enable requests (error-proof handling).*/
            QLOGI("Ignore redundant enable - count already zero, change count: %d", change_count);
        }
    }
}

#endif /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */

void esim_apdu_msg_clean(void)
{
    qosa_uint32_t                 msg_num = 0;
#if defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
    esim_apdu_msg_info_t          cnf = {0};
#else /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
    qosa_sim_generic_access_cnf_t cnf = {0};
#endif /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
    qosa_msgq_get_cnt(g_esim_apdu_msgq, &msg_num);
    if (msg_num > 0)
    {
        do {
#if defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
            qosa_msgq_wait(g_esim_apdu_msgq, (qosa_uint8_t *)&cnf, sizeof(esim_apdu_msg_info_t), 5);
            qosa_free(cnf.apdu_info);
#else /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
            qosa_msgq_wait(g_esim_apdu_msgq, (qosa_uint8_t *)&cnf, sizeof(qosa_sim_generic_access_cnf_t), 5);
#endif /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
            msg_num--;
        } while (msg_num == 0);
    }
}

/**
 * @brief SIM Card APDU Communication Callback Function
 *
 * This function is used to process the response result of the SIM card APDU communication command.
 *
 * @param [in] ctx
 *           - AT command context pointer, containing command-related information
 * @param [in] argv
 *           - SIM card generic logical channel access confirmation message pointer, containing execution result and response data
 */
static void esim_csim_cb(void *ctx, void *argv)
{
    QOSA_UNUSED(ctx);
    QLOGI("apdu release");
#if defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
    esim_apdu_msg_info_t msg_info = {0, QOSA_NULL};
    msg_info.apdu_info = qosa_malloc(sizeof(qosa_sim_generic_access_cnf_t));
    if (msg_info.apdu_info == NULL)
    {
        QLOGI("apdu release malloc error");
        return;
    }
    else
    {
        qosa_memcpy(msg_info.apdu_info,argv,sizeof(qosa_sim_generic_access_cnf_t));
        qosa_msgq_release(g_esim_apdu_msgq, sizeof(esim_apdu_msg_info_t), (qosa_uint8_t *)&msg_info, 0);
    }
#else /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
    qosa_msgq_release(g_esim_apdu_msgq, sizeof(qosa_sim_generic_access_cnf_t), (qosa_uint8_t *)argv, 0);
#endif /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */

}

/**
 * @brief Send APDU data to the specified channel ID and simultaneously receive the corresponding response content.
 *
 * @param[in] qosa_uint8_t channel_id
 *         - The specified channel ID to operate on
 *
 * @param[in] esim_apdu_data_t * send_apdu_ptr
 *         - The content of the APDU to be sent
 *
 * @param[in] qosa_uint16_t req_apdu_len
 *         - Length of the sent APDU
 *
 * @param[in] qosa_uint8_t* resp_apdu
 *         - Receive APDU response buffer
 *
 * @param[in] qosa_uint16_t resp_apdu_size
 *         - APDU response buffer size
 *
 * @return int
 *         - On successful operation, returns the length of the APDU response
 *         - Returns 0 on failure
 */
int esim_hal_send_apdu_get_responce(qosa_uint8_t *req_apdu, qosa_uint16_t req_apdu_len, qosa_uint8_t *resp_apdu, qosa_uint16_t resp_apdu_size, int time_out_ms)
{
    qosa_int32_t                  qosa_err = 0;
    qosa_uint8_t                  sim_id = 0;
#if defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
    esim_apdu_msg_info_t          cnf = {0};
#else /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
    qosa_sim_generic_access_cnf_t cnf = {0};
#endif /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
    int                           err = QOSA_OK;
    int                           resp_len = 0;

    do {
        if (req_apdu == QOSA_NULL || req_apdu_len == 0 || resp_apdu == QOSA_NULL || resp_apdu_size == 0)
        {
            QLOGE("invalid param");
            break;
        }

        //Clear the message queue to prevent abnormal messages from affecting the current result.
        esim_apdu_msg_clean();
        esim_hal_apdu_printf("hal apdu tx", req_apdu, req_apdu_len);
        //Call the SIM general access interface to execute APDU commands and register callback functions
        qosa_err = qosa_sim_generic_access(sim_id, req_apdu, req_apdu_len, esim_csim_cb, QOSA_NULL);
        if (QOSA_SIM_ERR_OK != qosa_err)
        {
            QLOGE("qosa_sim_generic_access fail:0x%x", qosa_err);
            break;
        }
        QLOGI("wait apdu resp");
#if defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
        err = qosa_msgq_wait(g_esim_apdu_msgq, (qosa_uint8_t *)&cnf, sizeof(esim_apdu_msg_info_t), time_out_ms);
        //Check if the SIM card command execution is successful, copy APDU data
        if (err == QOSA_OK && QOSA_SIM_ERR_OK == cnf.apdu_info->err_code)
        {
            if (resp_apdu_size >= cnf.apdu_info->data_len)
            {
                QLOGI("apdu = %s,len = %d",cnf.apdu_info->data,cnf.apdu_info->data_len);
                qosa_memcpy(resp_apdu, cnf.apdu_info->data, cnf.apdu_info->data_len);
                resp_len = cnf.apdu_info->data_len;
                esim_hal_apdu_printf("hal apdu rx", resp_apdu, resp_len);
                break;
            }
            else
            {
                QLOGE("apdu length[%d] is too long ", cnf.apdu_info->data_len);
                break;
            }
        }
        else
        {
            //SIM card command execution failed
            QLOGE("err:0x%x, apdu response error:0x%x ", err, cnf.apdu_info->err_code);
        }
        qosa_free(cnf.apdu_info);
#else /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
        err = qosa_msgq_wait(g_esim_apdu_msgq, (qosa_uint8_t *)&cnf, sizeof(qosa_sim_generic_access_cnf_t), time_out_ms);
        //Check if the SIM card command execution is successful, copy APDU data
        if (err == QOSA_OK && QOSA_SIM_ERR_OK == cnf.err_code)
        {
            if (resp_apdu_size >= cnf.data_len)
            {
                qosa_memcpy(resp_apdu, cnf.data, cnf.data_len);
                resp_len = cnf.data_len;
                esim_hal_apdu_printf("hal apdu rx", resp_apdu, resp_len);
                break;
            }
            else
            {
                QLOGE("apdu length[%d] is too long ", cnf.data_len);
                break;
            }
        }
        else
        {
            //SIM card command execution failed
            QLOGE("err:0x%x, apdu response error:0x%x ", err, cnf.err_code);
        }
#endif /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */
    } while (0);

    return resp_len;
}

/**
 * @brief Open an APDU logical channel
 * Typically, this function also needs to include configuring terminal capabilities and selecting the AID.
 *
 * @param[out] qosa_uint8_t * channel_id
 *          - Returns the channel ID that has been successfully opened
 *
 * @return esim_result_errno_e
 *          - Returns ESIM_OK on success
 *          - Returns other values on failure
 */
esim_result_errno_e esim_open_isdr_channel(qosa_uint8_t *channel_id)
{
    qosa_uint8_t *tmp_resp_apdu = QOSA_NULL;
    qosa_uint16_t tmp_resp_apdu_len = 0;

    tmp_resp_apdu = qosa_malloc(QOSA_SIM_R_APDU_LEN_MAX + 1);
    if (tmp_resp_apdu == QOSA_NULL)
    {
        QLOGE("mallco tmp_resp_apdu error ");
        return ESIM_MEMROY_ERR;
    }

#ifdef CONFIG_QOSA_EIGEN718_PLATFORM_FUNC
    esim_set_sim_sleep_state(QOSA_FALSE);  //Disable SIM sleep
#endif
    // open channel
    qosa_memset(tmp_resp_apdu, 0x00, tmp_resp_apdu_len);
    tmp_resp_apdu_len = esim_hal_send_apdu_get_responce(
        g_open_channel_cmd,
        sizeof(g_open_channel_cmd),
        tmp_resp_apdu,
        QOSA_SIM_R_APDU_LEN_MAX,
        ESIM_APDU_RESP_WAIT_TIME_MS_NORMAL
    );
    if (tmp_resp_apdu_len == 3 && tmp_resp_apdu[tmp_resp_apdu_len - 2] == 0x90 && tmp_resp_apdu[tmp_resp_apdu_len - 1] == 0x00)
    {
        *channel_id = tmp_resp_apdu[0];
    }
    else
    {
        qosa_free(tmp_resp_apdu);
#ifdef CONFIG_QOSA_EIGEN718_PLATFORM_FUNC
        esim_set_sim_sleep_state(QOSA_TRUE);  //Enable SIM sleep
#endif
        QLOGE("open logical channel error :0x%x ", tmp_resp_apdu_len);
        return ESIM_OPERATION_ERR;
    }

    //used default logical channel to set terminal capability
    qosa_memset(tmp_resp_apdu, 0x00, tmp_resp_apdu_len);
    tmp_resp_apdu_len = esim_hal_send_apdu_get_responce(
        g_terminal_capability_cmd,
        sizeof(g_terminal_capability_cmd),
        tmp_resp_apdu,
        QOSA_SIM_R_APDU_LEN_MAX,
        ESIM_APDU_RESP_WAIT_TIME_MS_NORMAL
    );
    if (tmp_resp_apdu_len == 0)
    {
        QLOGE("set terminal capability resp len :0x%x ", tmp_resp_apdu_len);
    }

    //select aid
    qosa_memset(tmp_resp_apdu, 0x00, tmp_resp_apdu_len);
    g_lpa_aid_cmd[0] = *channel_id;
    tmp_resp_apdu_len
        = esim_hal_send_apdu_get_responce(g_lpa_aid_cmd, sizeof(g_lpa_aid_cmd), tmp_resp_apdu, QOSA_SIM_R_APDU_LEN_MAX, ESIM_APDU_RESP_WAIT_TIME_MS_NORMAL);
    if (tmp_resp_apdu_len == 0)
    {
        QLOGE("select aid resp len:0x%x ", tmp_resp_apdu_len);
    }
    qosa_free(tmp_resp_apdu);
    return ESIM_OK;
}

/**
 * @brief Close the corresponding APDU logical channel, corresponding one-to-one with open.
 *
 * @param[in] qosa_uint8_t channel_id
 *          - The specified APDU logical channel.
 *
 * @return esim_result_errno_e
 *          - Returns ESIM_OK on success
 *          - Returns other values on failure
 */
esim_result_errno_e esim_close_channel(qosa_uint8_t channel_id)
{
    qosa_uint8_t *tmp_resp_apdu = QOSA_NULL;

    tmp_resp_apdu = qosa_malloc(QOSA_SIM_R_APDU_LEN_MAX + 1);
    if (tmp_resp_apdu == QOSA_NULL)
    {
        QLOGE("mallco tmp_resp_apdu error ");
        return ESIM_MEMROY_ERR;
    }
    // close channel
    QLOGI("close channel");
    g_close_channel_cmd[3] = channel_id;
    esim_hal_send_apdu_get_responce(
        g_close_channel_cmd,
        sizeof(g_close_channel_cmd),
        tmp_resp_apdu,
        QOSA_SIM_R_APDU_LEN_MAX,
        ESIM_APDU_RESP_WAIT_TIME_MS_NORMAL
    );
    qosa_free(tmp_resp_apdu);
#ifdef CONFIG_QOSA_EIGEN718_PLATFORM_FUNC
    esim_set_sim_sleep_state(QOSA_TRUE);  //Enable SIM sleep
#endif
    return ESIM_OK;
}

/**
 * @brief Send APDU data to the specified channel ID, and simultaneously receive the corresponding reply content.
 *
 * The memory management of rsp_apdu_ptr/send_apdu_ptr is handled within the SDK library; no external release action is required.
 *
 * @param[in] qosa_uint8_t channel_id
 *         - The specified channel ID to operate on
 *
 * @param[in] esim_apdu_data_t * send_apdu_ptr
 *         - The content of the APDU to be sent
 *
 * @param[in] esim_apdu_data_t * rsp_apdu_ptr
 *         - The received APDU content
 *
 * @return esim_result_errno_e
 *         - Returns ESIM_OK on successful operation
 *         - Returns other values on failure
 */
esim_result_errno_e esim_send_apdu_get_responce(qosa_uint8_t channel_id, esim_apdu_data_t *send_apdu_ptr, esim_apdu_data_t *rsp_apdu_ptr)
{
    esim_result_errno_e ret = ESIM_APDU_SENT_ERR;
    QOSA_UNUSED(channel_id);
    if (send_apdu_ptr != QOSA_NULL && rsp_apdu_ptr != QOSA_NULL)
    {
        rsp_apdu_ptr->apdu_data_len = esim_hal_send_apdu_get_responce(
            send_apdu_ptr->apdu_data_ptr,
            send_apdu_ptr->apdu_data_len,
            rsp_apdu_ptr->apdu_data_ptr,
            rsp_apdu_ptr->max_len,
            ESIM_APDU_RESP_WAIT_TIME_MS_MAX
        );
        if (rsp_apdu_ptr->apdu_data_len != 0)
        {
            ret = ESIM_OK;
        }
    }
    return ret;
}
