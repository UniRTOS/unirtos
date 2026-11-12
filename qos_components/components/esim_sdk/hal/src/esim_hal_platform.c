/*****************************************************************/ /**
* @file esim_hal_platform.h
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
#include "esim_hal_platform.h"
#include "qosa_log.h"
#include "qosa_network.h"
#include "qcm_sha256.h"
#include "qosa_datacall.h"
#include "qosa_dev1.h"
#include "qosa_dev.h"

#define QOS_LOG_TAG LOG_TAG_ESIM

/** Invalid PLMN value */
#define ESIM_NW_PLMN_INVALID                                                                                                                                   \
    {                                                                                                                                                          \
        0xffff, 0xffff, 0xff                                                                                                                                   \
    }

qosa_sem_t g_cfun_sem = QOSA_NULL;

static const char g_base64_alphabet[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                                         'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                                         's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

//Used during decoding
static const unsigned char g_base64_suffix_map[256]
    = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 255, 255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
       255, 255, 255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255, 255, 255, 63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,
       255, 255, 255, 254, 255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
       22,  23,  24,  25,  255, 255, 255, 255, 255, 255, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
       45,  46,  47,  48,  49,  50,  51,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
       255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
       255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
       255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
       255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

static char cmove_bits(unsigned char src, unsigned lnum, unsigned rnum)
{
    src <<= lnum;  // src = src << lnum;
    src >>= rnum;  // src = src >> rnum;
    return src;
}

//EC200U cannot obtain the current available network bearer list
esim_result_errno_e esim_hal_get_nw_DRB(esim_ipa_hal_nw_param_t *param)
{
    esim_result_errno_e ret = ESIM_OK;
    param->available_network_bearer_num = 0;

    return ret;
}

//cat1 without enhanced coverage level
esim_result_errno_e esim_hal_get_nw_cel(esim_ipa_hal_nw_param_t *param)
{
    esim_result_errno_e ret = ESIM_OK;
    param->cel = 0;

    return ret;
}

//Obtain signal quality, signal-to-noise ratio, and signal strength based on QCSQ command data.
esim_result_errno_e esim_hal_get_nw_rsrp_sinr_rsrq(qosa_nw_rat_e cur_rat, qosa_nw_scell_info_t scell, esim_ipa_hal_nw_param_t *param)
{
    esim_result_errno_e ret = ESIM_OK;

    switch (cur_rat)
    {
#ifdef CONFIG_QOSA_NW_GSM_SUPPORT
        case QOSA_NW_RAT_2G: {
            /* gsm_rssi */
            param->radio_signal_strength = scell.gsm.signal_level;
            param->sinr = 0;
            param->link_quality = 0;
        }
#endif
#ifdef CONFIG_QOSA_NW_WCDMA_SUPPORT
        case QOSA_NW_RAT_3G: {
            /* wcdma_rssi */
            param->radio_signal_strength = scell.wcdma.rssi_meas;
            param->sinr = 0;
            param->link_quality = 0;
            break;
        }
#endif
        case QOSA_NW_RAT_4G: {
            /* lte.rssi lte.rsrp lte.sinr lte.rsrq */
            param->radio_signal_strength = scell.lte.rsrp;
            param->sinr = scell.lte.sinr;
            param->link_quality = scell.lte.rsrq;
            break;
        }

        case QOSA_NW_RAT_5G:
            param->radio_signal_strength = scell.nr5g.rsrp;
            param->sinr = scell.nr5g.sinr;
            param->link_quality = scell.nr5g.rsrq;
            break;

        default:
            QLOGE("unknown rat!");
            param->radio_signal_strength = 0;
            param->sinr = 0;
            param->link_quality = 0;
            break;
    }
    return ret;
}

//Obtain the location area <tac> based on CEREG command data.
esim_result_errno_e esim_hal_get_nw_lac(qosa_nw_rat_e cur_rat, qosa_nw_scell_info_t scell, esim_ipa_hal_nw_param_t *param)
{
    esim_result_errno_e ret = ESIM_OK;

    if (QOSA_NW_RAT_4G == cur_rat)
    {
        param->lac = scell.lte.tac;
    }

    return ret;
}

//Obtain APN and IP information based on CGDCONT/CGPADDR command data
//Convert the IP address from network byte order to string format based on the IP type (IPv4/IPv6/IPv4v6):
esim_result_errno_e esim_hal_get_nw_apn_ip(esim_ipa_hal_nw_param_t *param)
{
    esim_result_errno_e ret = ESIM_OK;
    qosa_uint8_t        sim_id = 0;
    qosa_pdp_context_t  pdp_ctx = {0};

    char ip4addr_buf[CONFIG_QOSA_INET_ADDRSTRLEN] = {0};
    char ip6addr_buf[CONFIG_QOSA_INET6_ADDRSTRLEN] = {0};

    if (QOSA_DATACALL_OK == qosa_datacall_get_pdp_context(sim_id, 1, &pdp_ctx))
    {
        //Failed to read PDP configuration information
        QLOGE("read  PdpCxt fail");
    }
    else
    {
        if (pdp_ctx.apn_valid == QOSA_TRUE)
        {
            QLOGI("Apn:%s", pdp_ctx.apn);
            param->apn.data_len = qosa_strlen(pdp_ctx.apn);
            qosa_strcpy((char *)param->apn.data, pdp_ctx.apn);
        }
        if (pdp_ctx.ipv4_ip_valid)
        {
            qosa_memset(ip4addr_buf, 0, sizeof(ip4addr_buf));
            qosa_ip_addr_inet_ntop(QOSA_IP_ADDR_AF_INET, &pdp_ctx.ipv4_ip.addr.ipv4_addr, ip4addr_buf, sizeof(ip4addr_buf));
            QLOGI("ipv4 addr:%s", ip4addr_buf);
            param->ip_dddresses.data_len += qosa_snprintf((char *)param->ip_dddresses.data, ESIM_DATA_ARRAY_MAX_LEN, "\"%s\"", ip4addr_buf);
        }
        if (pdp_ctx.ipv6_ip_valid)
        {
            qosa_memset(ip6addr_buf, 0, sizeof(ip6addr_buf));
            qosa_ip_addr_inet_ntop(QOSA_IP_ADDR_AF_INET, &pdp_ctx.ipv6_ip.addr.ipv6_addr, ip6addr_buf, sizeof(ip6addr_buf));
            QLOGI("ipv6 addr:%s", ip6addr_buf);
            param->ip_dddresses.data_len += qosa_snprintf(
                (char *)param->ip_dddresses.data + param->ip_dddresses.data_len,
                ESIM_DATA_ARRAY_MAX_LEN - param->ip_dddresses.data_len,
                "\"%s\"",
                ip6addr_buf
            );
        }
    }
    return ret;
}

//Get network_bearer
int esim_hal_get_nw_rat(int sim_id, qosa_nw_rat_e cur_rat, qosa_nw_scell_info_t scell)
{
    qosa_bool_t  is_tdd = QOSA_FALSE;
    qosa_uint8_t reg_status = 0;
    int          rat = 0xff;

    qosa_nw_get_reg_status(sim_id, QOSA_NULL, &reg_status);
    QLOGI("cur_rat:%d", cur_rat);
    if (cur_rat == QOSA_NW_RAT_4G)
    {
        is_tdd = scell.lte.is_tdd;
    }
    else if (cur_rat == QOSA_NW_RAT_5G)
    {
        is_tdd = (1 == scell.nr5g.duplex_mode) ? QOSA_TRUE : QOSA_FALSE;
    }

    /* convert act type from qosa_nw_act_e to esim_network_bearer_e */
    QLOGI("qstd_act:%d ", scell.act);
    switch (scell.act)
    {
#ifdef CONFIG_QOSA_NW_GSM_SUPPORT
        case QOSA_NW_ACT_GSM:
        case QOSA_NW_ACT_GSM_COMPACT:
        case QOSA_NW_ACT_GSM_EGPRS:
            rat = ESIM_NETWORK_BEARER_GSM;
            break;
#endif
#ifdef CONFIG_QOSA_NW_WCDMA_SUPPORT
        case QOSA_NW_ACT_UTRAN:
        case QOSA_NW_ACT_UTRAN_HSDPA:
        case QOSA_NW_ACT_UTRAN_HSUPA:
        case QOSA_NW_ACT_UTRAN_HSPA:
            rat = ESIM_NETWORK_BEARER_WCDMA;
            break;
#endif
        case QOSA_NW_ACT_EUTRAN:
            rat = is_tdd ? ESIM_NETWORK_BEARER_LTE_TDD : ESIM_NETWORK_BEARER_LTE_FDD;
            break;
#ifdef CONFIG_QOSA_NW_NR_SUPPORT
        case QOSA_NW_ACT_EUTRA_CONNECTED_TO_A_5GCN:
        case QOSA_NW_ACT_NR_CONNECTED_TO_A_5GCN:
        case QOSA_NW_ACT_NG_RAN:
        case QOSA_NW_ACT_EUTRA_NR_DUAL_CONNECTIVITY:
            rat = is_tdd ? ESIM_NETWORK_BEARER_5G_NR_TDD : ESIM_NETWORK_BEARER_5G_NR_FDD;
            break;
#endif

        default:
            rat = ESIM_NETWORK_BEARER_UNKNOWN;
            break;
    }

    QLOGI("rat:%d", rat);

    return rat;
}

qosa_plmn_t esim_hal_get_scell_plmn_by_rat(qosa_nw_rat_e rat, qosa_nw_scell_info_t *scell)
{
    qosa_plmn_t qstd_nw_plmn_invalid = ESIM_NW_PLMN_INVALID;
    switch (rat)
    {
        case QOSA_NW_RAT_2G:
            return scell->gsm.plmn;
        case QOSA_NW_RAT_3G:
            return scell->wcdma.plmn;
        case QOSA_NW_RAT_4G:
            return scell->lte.plmn;
        case QOSA_NW_RAT_5G:
            return scell->nr5g.plmn;
        default:
            return qstd_nw_plmn_invalid;
    }
}

int esim_hal_get_nw_mcc_mnc(int sim_id, qosa_nw_rat_e cur_rat, qosa_nw_scell_info_t scell, esim_ipa_hal_nw_param_t *param)
{
    qosa_plmn_t plmn = {0};
    plmn = esim_hal_get_scell_plmn_by_rat(cur_rat, &scell);
    param->smnc = plmn.mnc;
    param->smcc = plmn.mcc;
    return 0;
}

/**
 * @brief Obtain platform-specific network parameters
 *        Functions can be selectively implemented based on actual conditions,
 *        If some data cannot be obtained, set it directly to 0.
 *
 * @param[out] esim_ipa_hal_nw_param_t *param
 *          - Network parameter storage for storage platforms
 *
 * @return esim_result_errno_e
 *          - if success return ESIM_OK
 *          - Failed to return other
 */
esim_result_errno_e esim_hal_get_nw_param(esim_ipa_hal_nw_param_t *param)
{
    esim_result_errno_e ret = ESIM_INVALID_PARAM_ERR;
    int                 net_status = 0;
    int                 sim_id = 0;
    qosa_nw_err_e       err = QOSA_NW_ERR_OK;

    if (param != QOSA_NULL)
    {
        net_status = esim_hal_get_network_reg_status();
        if (net_status == 0)
        {
            qosa_nw_scell_info_t scell = {0};
            qosa_nw_rat_e        cur_rat = qosa_nw_get_current_rat(sim_id);
            qosa_memset(param, 0x00, sizeof(esim_ipa_hal_nw_param_t));
            err = qosa_nw_get_scell_info(sim_id, &scell);
            if (err == QOSA_NW_ERR_OK)
            {
                param->cellId = scell.lte.cellid;
                param->network_bearer = esim_hal_get_nw_rat(sim_id, cur_rat, scell);
                esim_hal_get_nw_mcc_mnc(sim_id, cur_rat, scell, param);
                esim_hal_get_nw_apn_ip(param);
                esim_hal_get_nw_lac(cur_rat, scell, param);
                esim_hal_get_nw_rsrp_sinr_rsrq(cur_rat, scell, param);
                esim_hal_get_nw_cel(param);
                esim_hal_get_nw_DRB(param);
            }

            ret = ESIM_OK;
        }
        else
        {
            ret = ESIM_NET_WORK_ERR;
        }
    }

    return ret;
}

/**
 * @brief Get the platform's IMEI number
 *
 * @param[out] char * imei
 *          - ESIM SDK reads the IMEI content of the platform
 *
 * @param[in/out] int * imei_len
 *          - IMEI address space length, also need to return the actual read IMEI length
 *
 * @return esim_result_errno_e
 *          - Returns ESIM_OK on success
 *          - Returns other values on failure
 */
esim_result_errno_e esim_hal_get_imei(char *imei, int *imei_len)
{
    int                 sim_id = 0;
    esim_result_errno_e ret = ESIM_ERROR_GENERAL;
    qosa_dev_error_e    err = QOSA_DEV_ERRID_SUCCESS;

    if (imei == QOSA_NULL || imei_len == QOSA_NULL || *imei_len <= 0)
    {
        return ESIM_INVALID_PARAM_ERR;
    }

    err = qosa_dev_get_sim_imei(sim_id, imei, imei_len);

    if (err == QOSA_DEV_ERRID_SUCCESS)
    {
        ret = ESIM_OK;
    }

    return ret;
}

/**
 * @brief SIM Demo Setup CFUN Callback Function
 *
 * This function serves as the callback for the qosa_dev_set_cfun API and is invoked upon completion of the API execution.
 * The main function is to record the CFUN setting result and release the semaphore to allow the program to continue execution.
 *
 * @param[in] ctx
 *          - Context pointer
 *
 * @param[in] argv
 *          - Callback parameter pointer, pointing to the qosa_dev_general_cnf_t structure, containing result information of the CFUN setting operation
 */
static void sim_demo_set_cfun_cb(void *ctx, void *argv)
{
    qosa_dev_general_cnf_t *cnf = argv;
    QLOGI("cfun result=%d", cnf->err_code);

    //Release the CFUN semaphore to allow the program to continue running.
    qosa_sem_release(g_cfun_sem);
}

/**
 * @brief SIM re-insertion interface. To prevent the refresh function from behaving unexpectedly, this feature needs to be implemented.
 */
void esim_hal_card_reupload(void)
{
    int ret = 0;

    //Create demo general semaphore
    ret = qosa_sem_create(&g_cfun_sem, 0);
    if (ret != QOSA_OK)
    {
        QLOGI("Create g_sim_demo_sem failed: %d", ret);
        return;
    }

    //Set CFUN=0 to disable the SIM card function
    QLOGI("[CFUN]: 0");
    ret = qosa_dev_set_cfun(0, 0, sim_demo_set_cfun_cb, QOSA_NULL);
    QLOGI("ret=%d,qosa_dev_set_cfun0", ret);
    //Wait for the semaphore to be released by the sim_demo_set_cfun_cb callback function
    qosa_sem_wait(g_cfun_sem, QOSA_WAIT_FOREVER);

    //Set CFUN=1 to enable the SIM card function
    QLOGI("[CFUN]: 1");
    ret = qosa_dev_set_cfun(0, 1, sim_demo_set_cfun_cb, QOSA_NULL);
    QLOGI("ret=%d,qosa_dev_set_cfun1", ret);

    //Wait for the semaphore to be released by the sim_demo_set_cfun_cb callback function
    qosa_sem_wait(g_cfun_sem, QOSA_WAIT_FOREVER);

    qosa_sem_delete(g_cfun_sem);
    g_cfun_sem = QOSA_NULL;
    return;
}

/**
 * @brief Set APN information
 *
 * @param[in] char * apn
 *          - The APN content sent by the ESIM SDK to the platform; if the APN is NULL, the APN should be configured as an empty value.
 *
 * @param[in] int * apn_len
 *          - apn character length; if apn_len is 0, apn should be configured as an empty value
 *
 * @return esim_result_errno_e
 *          - Returns ESIM_OK on success
 *          - Returns other on failure
 */
esim_result_errno_e esim_hal_set_apn(char *apn, int apn_len)
{
    esim_result_errno_e   ret = ESIM_ERROR_GENERAL;
    qosa_pdp_context_t    pdp_ctx = {0};
    qosa_uint8_t          sim_id = 0;
    int                   val = 0;
    int                   apn_size = 0;
    qosa_datacall_errno_e err = QOSA_DATACALL_OK;

    QLOGI("apn:%s, apn_len:%d", apn, apn_len);
    do {
        if (QOSA_DATACALL_OK == qosa_datacall_get_pdp_context(sim_id, 1, &pdp_ctx))
        {
            //Failed to read PDP configuration information
            QLOGI("read  PdpCxt fail");
            pdp_ctx.pdp_type = QOSA_PDP_TYPE_IPV4V6;  //ipv4v6
        }
        else
        {
            if (apn_len > 0 && apn != QOSA_NULL)
            {
                if (pdp_ctx.apn_valid == QOSA_TRUE)
                {
                    QLOGI("old Apn:%s", pdp_ctx.apn);
                    val = qosa_memcmp(apn, pdp_ctx.apn, apn_len);
                    if (val == 0)
                    {
                        QLOGI("apn same");
                        ret = ESIM_OK;
                        break;
                    }
                }
            }
        }
        qosa_memset(pdp_ctx.apn, 0x00, QOSA_APN_MAX_LEN);
        if (apn_len > 0 && apn != QOSA_NULL)
        {
            apn_size = MIN(apn_len, QOSA_APN_MAX_LEN);
            qosa_memcpy(pdp_ctx.apn, apn, apn_size);
            pdp_ctx.apn_valid = QOSA_TRUE;
        }
        else
        {
            pdp_ctx.apn_valid = QOSA_FALSE;
        }

        err = qosa_datacall_set_pdp_context(sim_id, 1, &pdp_ctx);
        QLOGI("err:0x%x", err);
        if (err == QOSA_DATACALL_OK)
        {
            QLOGI("apn set SUCCESS");
            ret = ESIM_OK;
        }

    } while (0);

    return ret;
}

/**
 * @brief Check if PDP can be activated successfully
 *
 * @return int
 *          - Can be activated, returns 0
 *          - Unable to activate, returns other values
 */
int esim_hal_pdp_activation_capability_check(void)
{
    int                  status = 1;
    int                  ret = 0;
    int                  datacall_status = 0;
    qosa_uint8_t         sim_id = 0;
    int                  profile_idx = 1;
    qosa_datacall_conn_t conn;

    conn = qosa_datacall_conn_new(sim_id, profile_idx, QOSA_DATACALL_CONN_TCPIP);
    QLOGI("ret=%d pdp_cid=%d", ret, profile_idx);

    // Get datacall status (0: deactive 1: active)
    datacall_status = qosa_datacall_get_status(conn);
    QLOGI("datacall status=%d", datacall_status);

    if (datacall_status == 1)
    {
        QLOGI("pdp already activied");
        status = 0;
    }
    else if (esim_hal_get_network_reg_status() != 0)
    {
        status = 1;
        QLOGI("not reg");
    }
    else if (datacall_status == 0)
    {
        // Start execute datacall (sync)
        ret = qosa_datacall_start(conn, 30);
        if (ret != QOSA_DATACALL_OK)
        {
            QLOGI("datacall fail ,ret=%d", ret);
            status = 1;
        }
    }

    return status;
}

/**
 * @brief Base64 Encoding Function
 *
 * Encode the given binary data into a Base64 formatted string.
 *
 * @param[in] const char * indata
 *          - Data to be encoded
 *
 * @param[in] int inlen
 *          - Length of the data to be encoded
 *
 * @param[out] char * outdata
 *          - Base64 string output after encoding
 *
 * @param[out] int * outlen
 *          - Length of the encoded string
 *
 * @return int
 *          - Returns ESIM_OK on success
 *          - Returns other values on failure
 */
esim_result_errno_e esim_hal_base64_encode(const unsigned char *indata, int inlen, unsigned char *outdata, int *outlen)
{
    esim_result_errno_e ret = ESIM_OK;  // return value
    int i = 0;
    if (indata == QOSA_NULL || inlen == 0)
    {
        return ESIM_ERROR_GENERAL;
    }

    int in_len = 0;   //Source string length, if in_len is not a multiple of 3, then it needs to be padded to a multiple of 3
    int pad_num = 0;  //Number of characters that need to be filled, so there are only 2, 1, 0 (if 0, no concatenation is needed, )
    if (inlen % 3 != 0)
    {
        pad_num = 3 - inlen % 3;
    }
    in_len = inlen + pad_num;      //The concatenated length, the actual length required for encoding (multiple of 3)

    int out_len = in_len * 8 / 6;  //Encoded length

    unsigned char *p = outdata;    //Define a pointer to point to the first address of the outgoing data.

    //Encoding, length is the adjusted length, 3 bytes per group
    for (i = 0; i < in_len; i += 3)
    {
        int  value = *indata >> 2;          //Shift the first character of indata to the right by 2 bits (discard 2 bits)
        char c = g_base64_alphabet[value];  //Characters corresponding to the base64 conversion table
        *p = c;                             //Assign the corresponding character (encoded character) to the first byte of outdata

        //Process the data of the last group (the last 3 bytes)
        if (i == inlen + pad_num - 3 && pad_num != 0)
        {
            if (pad_num == 1)
            {
                *(p + 1) = g_base64_alphabet[(int)(cmove_bits(*indata, 6, 2) + cmove_bits(*(indata + 1), 0, 4))];
                *(p + 2) = g_base64_alphabet[(int)cmove_bits(*(indata + 1), 4, 2)];
                *(p + 3) = '=';
            }
            else if (pad_num == 2)
            {  //The encoded data needs to be padded with two '='
                *(p + 1) = g_base64_alphabet[(int)cmove_bits(*indata, 6, 2)];
                *(p + 2) = '=';
                *(p + 3) = '=';
            }
        }
        else
        {  //Processing normal 3-byte data
            *(p + 1) = g_base64_alphabet[cmove_bits(*indata, 6, 2) + cmove_bits(*(indata + 1), 0, 4)];
            *(p + 2) = g_base64_alphabet[cmove_bits(*(indata + 1), 4, 2) + cmove_bits(*(indata + 2), 0, 6)];
            *(p + 3) = g_base64_alphabet[*(indata + 2) & 0x3f];
        }

        p += 4;
        indata += 3;
    }

    if (outlen != QOSA_NULL)
    {
        *outlen = out_len;
    }

    return ret;
}

/**
 * @brief Base64 Decoding Function
 *
 * Decode a Base64-encoded string into its original binary data.
 *
 * @param[in] const unsigned char * indata
 *          - Base64 string to be decoded
 *
 * @param[in] int inlen
 *          - Length of the Base64 string to be decoded
 *
 * @param[out] char * outdata
 *          - Decoded raw binary data
 *
 * @param[out] int * outlen
 *          - Length of decoded data
 *
 * @return int
 *          - Returns ESIM_OK if successful
 *          - If it fails, return other
 */
esim_result_errno_e esim_hal_base64_decode(const unsigned char *indata, int inlen, unsigned char *outdata, int *outlen)
{
    esim_result_errno_e ret = ESIM_OK;
    if (indata == QOSA_NULL || inlen <= 0 || outdata == QOSA_NULL || outlen == QOSA_NULL)
    {
        return ESIM_INVALID_PARAM_ERR;
    }
    if (inlen % 4 != 0)
    {  //The data to be decoded is not a multiple of 4 bytes
        return ESIM_ERROR_GENERAL;
    }

    int           t = 0, x = 0, y = 0, i = 0;
    unsigned char c = 0;
    int           g = 3;

    while (indata[x] != 0)
    {
        //The ASCII value of the data to be decoded corresponds to the value in the base64_suffix_map.
        c = g_base64_suffix_map[indata[x++]];
        if (c == 255)
            return -1;  //The corresponding value is not in the transcoding table
        if (c == 253)
            continue;   //The corresponding value is a line break or carriage return.
        if (c == 254)
        {
            c = 0;
            g--;
        }                  //The corresponding value is '='
        t = (t << 6) | c;  //Place them sequentially into an int type occupying 3 bytes
        if (++y == 4)
        {
            outdata[i++] = (unsigned char)((t >> 16) & 0xff);
            if (g > 1)
                outdata[i++] = (unsigned char)((t >> 8) & 0xff);
            if (g > 2)
                outdata[i++] = (unsigned char)(t & 0xff);
            y = t = 0;
        }
    }
    if (outlen != QOSA_NULL)
    {
        *outlen = i;
    }
    return ret;
}

/**
 * @brief Compute the hash of the input data using the SHA-256 algorithm
 *
 * This function uses the SHA-256 algorithm to compute the hash of the input binary data and stores the result in the output buffer.
 *
 * @param[in] const unsigned char * indata
 *          - Points to a buffer containing binary data to be hashed
 *
 * @param[in] unsigned int inlen
 *          - Length of input data, in bytes
 *
 * @param[out] char * outdata
 *          - Pointer to the output buffer for storing the hash calculation result
 *
 * @param[out] int outdata_size (32)
 *          - The size of the output buffer, in bytes
 *
 * @return int
 *          - If successful, returns ESIM_OK
 *          - If it fails, return other
 */
esim_result_errno_e esim_hal_sha256(const unsigned char *indata, int inlen, unsigned char *outdata, int outdata_size)
{
    qcm_core_sha256(indata, inlen, outdata);
    return ESIM_OK;
}

/**
 * @brief Check if the network is registered
 *
 * @return  int
 *          - Success, returns 0
 *          - Not registered, returns 1
 */
int esim_hal_get_network_reg_status(void)
{
    int          ret = 0;
    qosa_uint8_t sim_id = 0;
    qosa_uint8_t ps_status = 0;

    qosa_nw_get_reg_status(sim_id, QOSA_NULL, &ps_status);

    QLOGI("ret: 0x%x, current data reg status=%d", ret, ps_status);
    if ((QOSA_NW_REG_HOME != ps_status) && (QOSA_NW_REG_ROAMING != ps_status))
    {
        ret = 1;
    }
    return ret;
}

/**
 * @brief Calculate CRC32 checksum
 *
 * @param[in] unsigned int in_crc32
 *          - Initial CRC32 value
 *
 * @param[in] const void *buf
 *          - Pointer to the data buffer for which CRC32 needs to be calculated
 *
 * @param[in] unsigned int buf_len
 *          - Length of input data, in bytes
 *
 * @return unsigned int
 *          - Returns the CRC32 checksum value
 */
unsigned int esim_hal_crc32_computebuf(unsigned int in_crc32, const void *buf, unsigned int buf_len)
{
    unsigned long  crc32;
    unsigned char *byteBuf;
    size_t         i;
    unsigned long  crc32_tab[]
        = {0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL,
           0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL, 0x1adad47dL, 0x6ddde4ebL,
           0xf4d4b551L, 0x83d385c7L, 0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L,
           0x4c69105eL, 0xd56041e4L, 0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL, 0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
           0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
           0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L, 0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L, 0x01db7106L,
           0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L, 0x7f6a0dbbL,
           0x086d3d2dL, 0x91646c97L, 0xe6635c01L, 0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
           0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L,
           0xd4bb30e2L, 0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
           0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL, 0x5edef90eL,
           0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
           0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L, 0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L,
           0x7d079eb1L, 0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L,
           0x10da7a5aL, 0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L, 0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
           0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
           0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L, 0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L,
           0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL, 0x9c0906a9L, 0xeb0e363fL,
           0x72076785L, 0x05005713L, 0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L,
           0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
           0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L, 0x4969474dL,
           0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L, 0xbdbdf21cL, 0xcabac28aL,
           0x53b39330L, 0x24b4a3a6L, 0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L,
           0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL};

    /** accumulate crc32 for buffer **/
    crc32 = in_crc32 ^ 0xFFFFFFFF;
    byteBuf = (unsigned char *)buf;
    for (i = 0; i < buf_len; i++)
    {
        crc32 = (crc32 >> 8) ^ crc32_tab[(crc32 ^ byteBuf[i]) & 0xFF];
    }
    return (crc32 ^ 0xFFFFFFFF);
}
