/*****************************************************************/ /**
* @file qosa_audio.h
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
#ifndef __QOSA_AUDIO_H__
#define __QOSA_AUDIO_H__

#define qosa_aud_err_exit(errcode)                                                                                                                             \
    {                                                                                                                                                          \
        ret = errcode;                                                                                                                                         \
        goto exit;                                                                                                                                             \
    }
#define QOSA_AUD_MIN(a, b) ((a) < (b) ? (a) : (b))

#define QOSA_AUD_TIMEOUT   (3000)  //3S timeout period
typedef void *qosa_aud_handle_t;
/*===========================================================================
 * Enum
===========================================================================*/
/**
 * @enum qosa_aud_errcode_e
 * @brief audio error code
 */
typedef enum
{
    QOSA_AUD_SUCCESS,
    QOSA_AUD_INVALID_PARAM = 1 | (QOSA_COMPONENT_AUDIO << 16), /*!< parameter error */
    QOSA_AUD_MUTEX_ERR,                                        /*!< mutex error */
    QOSA_AUD_NO_MEMORY_ERR,                                    /*!< no memory error */
    QOSA_AUD_SYSTEM_ERR,                                       /*!< system error */
    QSOA_AUD_REOPEN_ERR,                                       /*!< reopen error */
    QOSA_AUD_FILE_OPEN_ERR,                                    /*!< file open error */
    QOSA_AUD_FILE_WRITE_ERR,                                   /*!< file write error */
} qosa_aud_errcode_e;

/**
 * @enum qosa_stream_evt_e
 * @brief Event enumeration values
 */
typedef enum
{
    //Stream layer event ID
    QOSA_AUD_STREAM_RX_ARRIVE = 100, /*!< Received the data */
    QOSA_AUD_STREAM_RX_START,        /*!< Start receiving data */
    QOSA_AUD_STREAM_RX_LOW,          /*!< Notify the higher authorities that additional data can be provided */
    QOSA_AUD_STREAM_RX_HIGH,         /*!< Notify the upper level to suspend the data transmission */

    QOSA_AUD_STREAM_MAX = 199,

    //Decoder layer event ID
    QOSA_AUD_DECODE_COMPLETE = 200, /*!< Decoding completed */
    QOSA_AUD_DECODE_START,          /*!< Decoding start */
    QOSA_AUD_DECODE_LOW_LEVEL,      /*!< Notify the higher authorities that additional data can be provided */
    QOSA_AUD_DECODE_ERROR,          /*!< Decoding ERROR */

    QOSA_AUD_DECODE_MAX = 299,
    //Driver layer event
    QOSA_AUD_DRIVER_TX_COMPLETE = 300,
    QOSA_AUD_DRIVER_TX_LOW,
    QOSA_AUD_DRIVER_TX_START,
    QOSA_AUD_DRIVER_RX_OVERFLOW,
    QOSA_AUD_DRIVER_RX_HIGH,
    QOSA_AUD_DRIVER_MAX = 399,
} qosa_stream_evt_e;

/**
 * @enum qosa_aud_fmt_e
 * @brief audio format
 */
typedef enum
{
    QOSA_AUD_FMT_UNKNOWN,
    QOSA_AUD_FMT_PCM,
    QOSA_AUD_FMT_WAV,
    QOSA_AUD_FMT_MP3,
    QOSA_AUD_FMT_AMRNB,
    QOSA_AUD_FMT_AMRWB,
} qosa_aud_fmt_e;

/**
 * @enum qosa_decode_result_e
 * @brief Decoding error code
 */
typedef enum
{
    QOSA_DECODE_ERROR = -1,
    QOSA_DECODE_NONE = 0,
    QOSA_DECODE_SUCCESS,
    QOSA_DECODE_LOW_LEVEL,
    QOSA_DECODE_SETUP_ERROR,
} qosa_decode_result_e;

/**
 * @enum qosa_aud_vol_level_e
 * @brief audio Volume level
 */
typedef enum
{
    QOSA_AUD_VOLUME_NOT_SUPPORT = -1,
    QOSA_AUD_VOLUME_LEVEL_MUTE,
    QOSA_AUD_VOLUME_LEVEL_1,
    QOSA_AUD_VOLUME_LEVEL_2,
    QOSA_AUD_VOLUME_LEVEL_3,
    QOSA_AUD_VOLUME_LEVEL_4,
    QOSA_AUD_VOLUME_LEVEL_5,
    QOSA_AUD_VOLUME_LEVEL_6,
    QOSA_AUD_VOLUME_LEVEL_7,
    QOSA_AUD_VOLUME_LEVEL_8,
    QOSA_AUD_VOLUME_LEVEL_9,
    QOSA_AUD_VOLUME_LEVEL_10,
    QOSA_AUD_VOLUME_LEVEL_11,

    QOSA_AUD_VOLUME_LEVEL_MAX = QOSA_AUD_VOLUME_LEVEL_11
} qosa_aud_vol_level_e;

typedef enum {
    QOSA_AUDIO_OUTPUT_EXTERNAL,   /*!< Set as external codec output */
    QOSA_AUDIO_OUTPUT_INTERNAL,   /*!< Set as built-in codec output */
    QOSA_AUDIO_OUTPUT_PWM,        /*!< Set to output in PWM mode */
    QOSA_AUDIO_OUTPUT_SPI,        /*!< Set to output in SPI mode */
    QOSA_AUDIO_OUTPUT_MAX,
} qosa_audio_output_type_e;

/**
 * @brief Audio routing direction relative to the local device and call peer.
 */
typedef enum
{
    QOSA_AUD_ROUTE_NEAR = 0, /*!< Playback to local side / record local TX microphone */
    QOSA_AUD_ROUTE_FAR,      /*!< Playback to call peer / record remote RX downlink */
    QOSA_AUD_ROUTE_BOTH,     /*!< Playback to both sides / record mixed TX and RX */
    QOSA_AUD_ROUTE_MAX,
} qosa_aud_route_e;


/**
 * @brief Select I2S interface samples per second
 */
typedef enum {
    QOSA_AUDIO_HAL_08K_SAMPLES  = 8000,   /*!< set to  8k samples per second */
    QOSA_AUDIO_HAL_16K_SAMPLES  = 16000,   /*!< set to 16k samples in per second */
    QOSA_AUDIO_HAL_32K_SAMPLES  = 32000,   /*!< set to 32k samples in per second */
    QOSA_AUDIO_HAL_22K_SAMPLES  = 22050,   /*!< set to 22.050k samples per second */
    QOSA_AUDIO_HAL_44K_SAMPLES  = 44100,   /*!< set to 44.1k samples per second */
    QOSA_AUDIO_HAL_48K_SAMPLES  = 48000,   /*!< set to 48k samples per second */
    QOSA_AUDIO_HAL_SAMPLES_MAX,
} qosa_audio_hal_iface_samples_e;

/**
 * @struct  qosa_aud_cb_param_t
 * @brief audio callback Parameter configuration structure
 */
typedef struct
{
    qosa_aud_handle_t handle;
    qosa_int32_t      event_id;
    qosa_uint32_t     size;
    qosa_uint8_t     *ctx;
} qosa_aud_cb_param_t;

typedef void (*qosa_aud_callback_t)(qosa_aud_cb_param_t *param);

/** DTMF detection callback; tone is an ASCII digit, '*', '#', or A-D ,'F'. */
typedef void (*qosa_aud_dtmf_callback_t)(char tone);

typedef struct
{
    qosa_bool_t               enable;   /*!< Keep remote downlink detection enabled until disabled. */
    qosa_aud_dtmf_callback_t  callback; /*!< Required when enabled. */
} qosa_aud_dtmf_cfg_t;

/**
 * @struct  qosa_aud_stream_opt_t
 * @brief   Configuration for using audio stream playback
 */
typedef struct qosa_aud_stream_opt
{
    union
    {
        qosa_uint64_t   options;
        struct
        {
            /* bit0~bit11,  reserved, use on other platforms */
            qosa_uint32_t   reserved1       : 12;
            /* bit12~bit15, pcm pre-filled mute frames size(each frame = 20 ms); default 0 means no pre-fill */
            qosa_uint32_t   mute_frames     : 4;
            /* bit16~bit20, reserved */
            qosa_uint32_t   reserved2       : 5;
            /* bit21~bit52, reserved, use on other platforms */
            qosa_uint32_t   reserved3       : 32;
            /* bit53~bit64, reserved, use on other platforms */
            qosa_uint32_t   reserved4       : 11;
        };
    };
}qosa_aud_stream_opt_t;

/**
 * @struct  qosa_aud_stream_cfg_t
 * @brief audio Data flow parameter configuration structure
 */
typedef struct
{
    qosa_bool_t             is_record; /*!< Record/Playback */
    qosa_uint16_t           samprate;  /*!< sampling rate */
    qosa_uint8_t            channels;  /*!< channel number */
    qosa_bool_t             sync_mode; /*!< Synchronous mode (blocking playback) / Asynchronous mode (non-blocking) */
    qosa_aud_fmt_e          format;    /*!< stream format */
    qosa_aud_callback_t     callback;  /*!< callback function */
    qosa_uint8_t            *ctx;      /*!< Corresponding to the ctx in qosa_aud_cb_param_t */
    qosa_aud_stream_opt_t   options;   /*!< Use bits 12 to 15 to represent the number of pre-filled silent frames. */
    qosa_aud_route_e        route;     /*!< Near, far, or both audio route */
} qosa_aud_stream_cfg_t;

/**
 * @struct  qosa_aud_head_info_t
 * @brief File header information structure
 */
typedef struct
{
    qosa_uint32_t head_size; /*!< File header size */
    qosa_uint32_t samprate;  /*!< sampling rate */
    qosa_uint8_t  channels;  /*!< channel number */
    qosa_uint32_t data_size; /*!< PCM valid data length (for WAV format only) */
} qosa_aud_head_info_t;

/*===========================================================================
 * callback
===========================================================================*/
typedef qosa_aud_errcode_e (*qosa_aud_open)(qosa_aud_stream_cfg_t *config, qosa_aud_handle_t *handle);
typedef qosa_int32_t (*qosa_aud_read)(qosa_aud_handle_t handle, qosa_uint8_t *buffer, qosa_uint32_t size);
typedef qosa_int32_t (*qosa_aud_write)(qosa_aud_handle_t handle, qosa_uint8_t *buffer, qosa_uint32_t size);
typedef qosa_int32_t (*qosa_aud_write_avi)(qosa_aud_handle_t handle);
typedef qosa_int32_t (*qosa_aud_read_avi)(qosa_aud_handle_t handle);
typedef qosa_int32_t (*qosa_aud_close)(qosa_aud_handle_t handle);
typedef qosa_decode_result_e (*qosa_aud_decode)(qosa_aud_handle_t handle);
typedef qosa_bool_t (*qosa_aud_head_det)(qosa_aud_head_info_t *head, qosa_uint8_t *buffer, qosa_uint32_t size);
typedef void (*qosa_aud_pa_callback_t)(qosa_bool_t is_on);

/**
 * @struct  osa_aud_drv_cb_t
 * @brief audio ops callback
 */
typedef struct
{
    qosa_aud_open      open;      /*!< The upper layer requests to activate the lower layer driver */
    qosa_aud_read      read;      /*!< The upper layer requests to read data from the lower layer */
    qosa_aud_write     write;     /*!< The upper layer requests the lower layer to write data */
    qosa_aud_close     close;     /*!< Upper layer requests to close */
    qosa_aud_write_avi write_avi; /*!< The upper layer requests to know how much data the lower layer can write */
    qosa_aud_read_avi  read_avi;  /*!< The upper layer requests to know how much data the lower layer can read */
    qosa_aud_decode    decode;    /*!< Upper layer requests to start decoding */
    qosa_aud_head_det  head_det;  /*!< The upper layer requests to extract the format information from a frame of data */
} osa_aud_drv_cb_t;

/**
 * @struct  qosa_aud_event_t
 * @brief Event reporting parameter structure body
 */
typedef struct
{
    qosa_uint32_t event_id;
    qosa_uint32_t param1;
    qosa_uint32_t param2;
    qosa_uint32_t param3;
} qosa_aud_event_t;

typedef struct
{
    qosa_int32_t pwm_spkn;      /*!< Pin number used for PWM output SPK_N */
    qosa_int32_t pwm_spkp;      /*!< Pin number used for PWM output SPK_P,Enter 0 if not required. */
    qosa_int32_t pwm_num;  /*!< PWM number of the PWM pin used */
    qosa_int32_t pwm_func; /*!< PWM function number of the pin used */
} qosa_pwm_aud_cfg_t;

typedef struct
{
    qosa_bool_t enable_customer; /*!< Enable flag(When disabled, the default configuration is used.) */
    qosa_bool_t dataDly;         /*!< Data delay settings */
    qosa_bool_t
        bclkPolarity; /*!< Bit clock polarity: 0: Data is transmitted on the rising edge and sampled on the falling edge; 1: Data is transmitted on the falling edge and sampled on the rising edge. */
    qosa_bool_t
        fsPolarity; /*!< Frame synchronization polarity: 0: Transmission begins on the rising edge and ends on the falling edge; 1: Transmission begins on the falling edge and ends on the rising edge. */
} qosa_aud_i2s_cfg_t;

typedef struct
{
    qosa_uint8_t io;                        /*!< Output type */
    qosa_uint8_t mode;                      /*!< Master-slave mode is not currently supported; slave mode is not supported. */
    qosa_uint8_t fsync;                     /*!< Frame synchronization mode does not currently support long frame synchronization. */
    qosa_uint8_t clock;                     /*!< clock frequency */
    qosa_uint8_t format;                    /*!< Data format (bit width) */
    qosa_audio_hal_iface_samples_e sample;  /*!< Output sampling rate */
    qosa_uint8_t num_slots;                 /*!< The number of slots (data packets) within a period is not currently supported. */
    qosa_uint8_t slot_mapping;              /*!< Using which slot is not currently supported. */
}qosa_dai_t;

/**
 * @brief Register audio driver callback function
 *
 * @param osa_aud_drv_cb_t *ops
 *          - A pointer pointing to the audio driver callback function structure, which includes various operation functions that the driver needs to implement.
 *
 * @return qosa_int32_t Return the registration result status code
 *         - Return 0 or a positive number when successful
 *         - Return a negative error code when failure occurs
 */
qosa_int32_t qosa_aud_driver_register(osa_aud_drv_cb_t *ops);

/**
 * @brief set volume level
 *
 * @param[in] qosa_aud_vol_level_e volume
 *              - Volume level
 *
 * @return qosa_aud_errcode_e error code
 *         - QOSA_AUD_INVALID_PARAM: parameter error
 *         - Other error codes: Return the corresponding error code based on the specific operation.
 */
qosa_aud_errcode_e qosa_aud_set_volume(qosa_aud_vol_level_e volume);

/**
 * @brief Set linear volume function
 *
 * @param[in] qosa_bool_t en
 *         - Enable linear volume
 * @param[in] qosa_int16_t level
 *         - Linear volume level
 *
 * @return qosa_aud_errcode_e Error code
 *         - QOSA_AUD_INVALID_PARAM: Invalid parameter
 *         - Other error codes: Return corresponding error code based on specific operation
 */
qosa_aud_errcode_e qosa_aud_apply_pcm_gain(qosa_bool_t en, qosa_int16_t level);

/**
 * @brief Obtain volume level
 *
 * @param NULL
 *
 * @return qosa_aud_vol_level_e Volume level
 */
qosa_aud_vol_level_e qosa_aud_get_volume(void);

/**
 * @brief Set mute state for voice-call uplink and downlink paths.
 * @param[in] uplink_mute  QOSA_TRUE to mute TX, QOSA_FALSE to unmute.
 * @param[in] downlink_mute QOSA_TRUE to mute RX, QOSA_FALSE to unmute.
 * @return QOSA_AUD_SUCCESS on success, otherwise an audio error code.
 */
qosa_aud_errcode_e qosa_aud_voice_mute_set(qosa_bool_t uplink_mute, qosa_bool_t downlink_mute);

/**
 * @brief Get mute state for voice-call uplink and downlink paths.
 * @param[out] uplink_mute  QOSA_TRUE if TX is muted.
 * @param[out] downlink_mute QOSA_TRUE if RX is muted.
 * @return QOSA_AUD_SUCCESS on success, otherwise an audio error code.
 */
qosa_aud_errcode_e qosa_aud_voice_mute_get(qosa_bool_t *uplink_mute, qosa_bool_t *downlink_mute);

/**
 * @brief PA callback registration function
 *
 * @param qosa_aud_pa_callback_t callback
 *         - callback: callback function
 *
 * @return qosa_aud_errcode_e error code
 *         - QOSA_AUD_INVALID_PARAM: parameter error
 *         - Other error codes: Return the corresponding error code based on the specific operation.
 */
qosa_aud_errcode_e qosa_aud_bind_pa_cb(qosa_aud_pa_callback_t callback);

/**
 * @brief Enable or disable remote (RX) DTMF detection.
 * @param cfg Detection configuration; callback is required when enabled.
 * @return QOSA_AUD_SUCCESS on success, otherwise an audio error code.
 * @note The callback runs in the platform audio/voice callback context.
 */
qosa_aud_errcode_e qosa_aud_dtmf_detect_set_cfg(const qosa_aud_dtmf_cfg_t *cfg);

/**
 * @brief Query the current remote (RX) DTMF detection configuration.
 * @param cfg [out] Receives the current configuration; callback is the
 *        registered handler, or QOSA_NULL when detection is disabled.
 * @return QOSA_AUD_SUCCESS on success, otherwise an audio error code.
 */
qosa_aud_errcode_e qosa_aud_dtmf_detect_get_cfg(qosa_aud_dtmf_cfg_t *cfg);

/**
 * @brief Select audio stream output method
 *
 * @param qosa_audio_output_type_e type
 *         - type: Output method
 *		   - ctrl: Output mode control(pwm: qosa_pwm_aud_cfg_t;  EXTERNAL:qosa_aud_i2s_cfg_t; spi:(char)port)
 *
 * @return qosa_aud_errcode_e error code
 *         - QOSA_AUD_INVALID_PARAM: parameter error
 *         - Other error codes: Return the corresponding error code based on the specific operation.
 */
qosa_aud_errcode_e qosa_aud_output_ctrl(qosa_audio_output_type_e type, void *ctrl);

/**
 * @brief Get audio stream parameters qdai
 *
 * @param qosa_dai_t iface_t
 *         - iface_t: Audio stream parameters, refer to the structure qosa_dai_t
 *
 * @return qosa_aud_errcode_e error code
 *         - QOSA_AUD_INVALID_PARAM: parameter error
 *         - Other error codes: Return the corresponding error code based on the specific operation.
 */
qosa_aud_errcode_e qosa_audio_set_stream_iface(qosa_dai_t *iface_t);

/**
 * @brief Select audio stream output method
 *
 * @param qosa_dai_t iface_t
 *         - iface_t: Audio stream parameters, refer to the structure qosa_dai_t
 *
 * @return qosa_aud_errcode_e error code
 *         - QOSA_AUD_INVALID_PARAM: parameter error
 *         - Other error codes: Return the corresponding error code based on the specific operation.
 */
qosa_aud_errcode_e qosa_audio_get_stream_iface(qosa_dai_t *iface_t);

#endif
