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
#ifndef __ESIM_HAL_PLATFORM_H__
#define __ESIM_HAL_PLATFORM_H__

#include "esim_error.h"
#include "esim_interface.h"

/**
 * @enum esim_network_bearer_e
 * @brief Used to indicate network bearer
 */
typedef enum
{
    ESIM_NETWORK_BEARER_GSM = 0,
    ESIM_NETWORK_BEARER_TD_SCDMA = 1,
    ESIM_NETWORK_BEARER_WCDMA = 2,
    ESIM_NETWORK_BEARER_CDMA2000 = 3,
    ESIM_NETWORK_BEARER_WIMAX = 4,
    ESIM_NETWORK_BEARER_LTE_TDD = 5,
    ESIM_NETWORK_BEARER_LTE_FDD = 6,
    ESIM_NETWORK_BEARER_NB_IOT = 7,
    ESIM_NETWORK_BEARER_5G_NR_TDD = 8,
    ESIM_NETWORK_BEARER_5G_NR_FDD = 9,
    ESIM_NETWORK_BEARER_WLAN = 21,
    ESIM_NETWORK_BEARER_BLUETOOTH = 22,
    ESIM_NETWORK_BEARER_IEEE = 23,
    ESIM_NETWORK_BEARER_ETHERNET = 41,
    ESIM_NETWORK_BEARER_DSL = 42,
    ESIM_NETWORK_BEARER_PLC = 43,
    ESIM_NETWORK_BEARER_UNKNOWN = 99,
} esim_network_bearer_e;

typedef struct esim_ipa_hal_nw_param
{
    /*
    Indicates the network bearer used for the current LwM2M communication session from the network bearer list below.
    The number range is split into three categories:
    0 - 20 are Cellular Bearers
    21 - 40 are Wireless Bearers
    41 - 50 are Wireline Bearers
    More specifically:
    0: GSM cellular network
    1: TD-SCDMA cellular network
    2: WCDMA cellular network
    3: CDMA2000 cellular network
    4: WiMAX cellular network
    5: LTE-TDD cellular network
    6: LTE-FDD cellular network
    7: NB-IoT
    8: 5G-NR TDD
    9: 5G-NR FDD
    10 - 20: Reserved for other types of cellular network
    21: WLAN network
    22: Bluetooth network
    23: IEEE 802.15.4 network
    24 - 40: Reserved for other types of local wireless network
    41: Ethernet
    42: DSL
    43: PLC
    44 - 50: reserved for other types of wireline networks.
    */
    int network_bearer;

    /*
Available DRB
    Indicates a list of current available network bearer. Each Resource Instance has a value from the network bearer list.
    */
    int available_network_bearer[16];
    int available_network_bearer_num;

    /*
    Indicates the average value of the received signal strength indication used in the
    current network bearer (as indicated by Resource 0 of this Object). The value is expressed in dBm.
    For the following network bearers the signal strength parameters indicated below are represented by this resource:
    GSM: RSSI
    UMTS: RSCP
    LTE: RSRP
    NB-IoT: NRSRP
    NR: SS-RSRP
    For more details on Network Measurement Report, refer to the appropriate Cellular or Wireless Network standards,
    (e.g. for LTE Cellular Network    refer to 3GPP TS 36.133 specification).
    */
    qosa_uint32_t radio_signal_strength;

    /*
    This contains received link quality e.g. LQI for IEEE 802.15.4 (range 0...255),
    RxQual Downlink for GSM (range 0...7, refer to [3GPP 44.018] for more details on Network Measurement Report encoding),
    RSRQ for LTE, (refer to [3GPP 36.214]),
    NRSRQ for NB-IoT (refer to [3GPP 36.214]).
    */
    qosa_uint32_t link_quality;

    /*
     The IP addresses assigned to the connectivity interface. (e.g. IPv4, IPv6, etc.)
    */
    esim_data_array_t ip_dddresses;

    /*
    The IP address of the next-hop IP router, on each of the interfaces specified in resource 4 (IP Addresses).
    Note: This IP Address doesn't indicate the Server IP address.
    */
    //esim_data_array_t router_ip_dddresses;

    /*
    The percentage indicating the average utilization of the link to the next-hop IP router.
    Range 0-100, units /100
    */
    //qosa_uint32_t link_utilization;

    /*
    Access Point Name in case Network Bearer Resource is a Cellular Network.
    */
    esim_data_array_t apn;

    /*
    Serving Cell ID in case Network Bearer Resource is a Cellular Network.
    As specified in TS [3GPP 23.003] and in [3GPP. 24.008]. Range (0...65535) in GSM/EDGE
    UTRAN Cell ID has a length of 28 bits.
    Cell Identity in WCDMA/TD-SCDMA. Range: (0...268435455).
    LTE Cell ID has a length of 28 bits.
    */
    qosa_uint32_t cellId;

    /*
    Serving Mobile Network Code. This is applicable when the Network Bearer Resource value is referring to a cellular network.
    Range 0-999
    */
    qosa_uint32_t smnc;

    /*
    Serving Mobile Country Code. This is applicable when the Network Bearer Resource value is referring to a cellular network.
    Range 0-999
    */
    qosa_uint32_t smcc;

    /*
    SINR: Signal to Interference plus Noise Ratio SINR is the ratio of the strength of the received signal to
    the strength of the received interference signal (noise and interference).
    units dB
    */
    qosa_uint32_t sinr;

    /* Location Area Code in case Network Bearer Resource is a Cellular Network.  */
    qosa_uint32_t lac;

    /*
    Indicates the Coverage Enhancement Level of the UE in the serving cell. The Coverage Enhancement levels are defined and specified in 3GPP TS 36.331 and 36.213.
    0: No Coverage Enhancement in the serving cell
    1: Coverage Enhancement level 0
    2: Coverage Enhancement level 1
    3: Coverage Enhancement level 2
    4: Coverage Enhancement level 3
    */
    qosa_uint32_t cel;
} esim_ipa_hal_nw_param_t;

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
esim_result_errno_e esim_hal_get_nw_param(esim_ipa_hal_nw_param_t* param);

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
esim_result_errno_e esim_hal_get_imei(char* imei, int* imei_len);

/**
 * @brief SIM reinsertion interface. To prevent the refresh function from not meeting expectations, this feature needs to be implemented.
 */
void esim_hal_card_reupload(void);

/**
 * @brief Set APN information
 *
 * @param[in] char * apn
 *          - The APN content sent from the ESIM SDK to the platform; if the APN is NULL, the APN should be configured as an empty value.
 *
 * @param[in] int * apn_len
 *          - apn character length; if apn_len is 0, apn should be configured as an empty value
 *
 * @return esim_result_errno_e
 *          - Returns ESIM_OK on success
 *          - Returns other values on failure
 */
esim_result_errno_e esim_hal_set_apn(char* apn, int apn_len);

/**
 * @brief Check if PDP can be activated successfully
 *
 * @return int
 *          - Can be activated, returns 0
 *          - Unable to activate, returns other values
 */
int esim_hal_pdp_activation_capability_check(void);

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
esim_result_errno_e esim_hal_base64_encode(const unsigned char* indata, int inlen, unsigned char* outdata, int* outlen);

/**
 * @brief Base64 Decoding Function
 *
 * Decode a Base64-encoded string into the original binary data.
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
 *          - If successful, returns ESIM_OK
 *          - If it fails, return other
 */
esim_result_errno_e esim_hal_base64_decode(const unsigned char* indata, int inlen, unsigned char* outdata, int* outlen);

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
 *          - Returns ESIM_OK if successful
 *          - If it fails, return other
 */
esim_result_errno_e esim_hal_sha256(const unsigned char* indata, int inlen, unsigned char* outdata, int outdata_size);

/**
 * @brief Check if the network is registered
 *
 * @return  int
 *          - Success, returns 0
 *          - Not registered, returns 1
 */
int esim_hal_get_network_reg_status(void);

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
unsigned int esim_hal_crc32_computebuf(unsigned int in_crc32, const void* buf, unsigned int buf_len);

#endif
