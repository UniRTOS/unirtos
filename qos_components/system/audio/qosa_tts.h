/*****************************************************************/ /**
* @file qosa_tts.h
* @brief
* @author Kevin.wang@quectel.com
* @date 2025-9-16
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-9-16 <td>1.0 <td>Kevin.wang <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_TTS_H__
#define __QOSA_TTS_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_audio.h"

typedef enum
{
    QOSA_TTS_EVENT_PLAY_START,     /*!< TTS playback starts */
    QOSA_TTS_EVENT_PLAY_FINISH,    /*!< The TTS playback has ended */
    QOSA_TTS_EVENT_PLAY_INTERRUPT, /*!< The TTS playback was interrupted */
} qosa_tts_event_t;

typedef enum
{
    QOSA_TTS_LANGUAGE_CHN,    /*!< chinese */
    QOSA_TTS_LANGUAGE_ENG,    /*!< english */
    QOSA_TTS_LANGUAGE_CHN_ENG /*!< chinese + english */
} qosa_tts_language_e;

typedef enum
{
    QOSA_TTS_ENCODING_GKB,  /*!< GKB  encode */
    QOSA_TTS_ENCODING_UTF8, /*!< UTF8  encode */
    QOSA_TTS_ENCODING_UCS2, /*!< UCS2  encode */
    QOSA_TTS_ENCODING_MAX
} qosa_tts_encoding_e;

typedef enum
{
    QOSA_TTS_DIGIT_MODE_AUTO, /*!< Based on the context, automatically identify how to read numbers */
    QOSA_TTS_DIGIT_MODE_NUM,  /*!< The numbers are always pronounced according to their position, such as reading 168 as "one six eight". */
    QOSA_TTS_DIGIT_MODE_VALUE /*!< Always read the numbers according to their numerical values. For example, read 168 as "one hundred sixty-eight". */
} qsosa_tts_digit_mode_e;

typedef void (*qosa_tts_callback_t)(qosa_tts_event_t event, qosa_uint8_t *data, qosa_uint32_t size);
/**
 * @brief When a client needs to use a separate TTS solution, this callback is used to read the TTS repository. 
 * The location of the repository can be customized by the client, but file system interfaces cannot be directly manipulated within this interface.
 *
 * @param pBuffer: This address is used internally by the engine library. Data with an offset of ipos and a length of nsize is read into this address, 
                        and the internal engine will process it automatically.
 * @param iPos: The engine library needs to read the offset of the data relative to the parameter pParameter each time.
 * @param nSize: The length to be read does not need to be processed by the customer.
 *
 * @return No return value
 */
typedef void (*qosa_tts_resource_cb_t)(void* pBuffer,qosa_uint32_t iPos,qosa_uint32_t nSize);

typedef enum
{
    QOSA_SIO_OUTPUT_NONE,      /*!< Do not output audio via USB, use default audio output device */
    QOSA_TTS_USB_AT_CHANNEL,   /*!< Output audio via USB AT channel */
    QOSA_TTS_USB_MODEM_CHANNEL /*!< Output audio via USB Modem channel */
} qosa_sio_output_e;

typedef struct
{
    qosa_uint8_t            res_path[256]; /*!< The path where the resource file is located. If there is no resource file, fill in NULL. */
    qosa_tts_language_e     langusge;      /*!< Language selection */
    qosa_tts_callback_t     callback;      /*!< callback function */
    qosa_bool_t             dat_output;    /*!< Whether to synchronize and output the data to the callback function */
    qosa_uint64_t           options;       /*!< Each platform provides its own explanation. */
    qosa_sio_output_e       sio_output;    /*!< Select which sio port to output from. */
    qosa_tts_resource_cb_t  read_cb;       /*!< read_cb function */
} qosa_tts_cfg_t;

typedef enum
{
    QOSA_TTS_CONFIG_SPEED = 1, /*!< Set the speaking speed */
    QOSA_TTS_CONFIG_VOLUME,    /*!< Adjust the volume */
    QOSA_TTS_CONFIG_ENCODING,  /*!< Set the encode mode */
    QOSA_TTS_CONFIG_DIGITS,    /*!< The pronunciation of numerical figures */
    QOSA_TTS_CONFIG_DGAIN,     /*!< Adjust the gain */

    QOSA_TTS_CONFIG_MAX,
} qosa_tts_config_e;

typedef struct
{
    int          qosa_tts_volume;        /*!< play volume */
    int          qosa_tts_speed;         /*!< play speed */
    int          qosa_tts_encoding_type; /*!< utf8 , gbk,ucs2 */
    int          qosa_tts_running_flag;  /*!< running flag */
    int          qosa_tts_readdigit;     /*!<0:TTS_DIGIT_AUTO,	1:TTS_DIGIT_NUMBER, 2:TTS_DIGIT_VALUE */
    qosa_uint8_t qosa_tts_language;      /*!<TTS_LANGUAGE_CHINESE or TTS_LANGUAGE_ENGLISH */
    int          qosa_tts_dgain;         /*!< volume gain */
} qosa_tts_params_t;

/**
 * @brief TTS open
 *
 * @param[in] qosa_tts_cfg_t *config
 *          - tts configuration：Resource path, callback, etc.
 *
 * @return qosa_aud_errcode_e
 *          - QOSA_AUD_SUCCESS: Success
 *          - other: Error code
 */
qosa_aud_errcode_e qosa_tts_open(qosa_tts_cfg_t *config);

/**
 * @brief TTS paly
 *
 * @param[in] qosa_tts_encoding_e code
 *          - Play text format
 *
 * @param[in] qosa_tts_encoding_e code
 *          - Play the text content
 *
 * @param[in] qosa_uint32_t length
 *          - Play the text content length
 *
 * @return qosa_aud_errcode_e
 *          - QOSA_AUD_SUCCESS: Success
 *          - other: Error code
 */
qosa_aud_errcode_e qosa_tts_play(qosa_tts_encoding_e code, const char *text, qosa_uint32_t length);

/**
 * @brief TTS close
 *
 * @param[in] qosa_bool_t keep_resource
 *          - Should the TTS resources be destroyed
 *
 * @return qosa_aud_errcode_e
 *          - QOSA_AUD_SUCCESS: Success
 *          - other: Error code
 */
qosa_aud_errcode_e qosa_tts_close(qosa_bool_t keep_resource);

/**
 * @brief TTS param get
 *
 * @param[in] qosa_tts_config_e type
 *          - To obtain the parameter type
 *
 * @return int
 *          - To obtain the parameter value
 */
int qosa_tts_param_cfg_get(qosa_tts_config_e type);

/**
 * @brief TTS param set
 *
 * @param[in] qosa_tts_config_e type
 *          - The type of parameters to be set
 *
 * @param[in] int val
 *          - The parameter values to be set
 *
 * @return qosa_aud_errcode_e
 *          - QOSA_AUD_SUCCESS: Success
 *          - other: Error code
 */
qosa_aud_errcode_e qosa_tts_param_cfg_set(qosa_tts_config_e type, int val);

/**
 * @brief TTS param set
 *
 * @param[in] qosa_int8_t *path_name
 *          - tts resource path
 *
 * @param[in] qosa_uint8_t *ram_addr
 *          - tts resource ram address
 *
 * @return qosa_aud_errcode_e
 *          - QOSA_AUD_SUCCESS: Success
 *          - other: Error code
 */
qosa_aud_errcode_e qosa_tts_resource_init(qosa_int8_t *path_name, qosa_uint8_t *ram_addr);

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_TTS_H__ */
