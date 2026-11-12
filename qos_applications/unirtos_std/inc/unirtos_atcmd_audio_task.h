/*****************************************************************/ /**
* @file unirtos_atcmd_audio_task.h
* @brief
* @author larson.li@quectel.com
* @date 2025-09-22
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-09-22 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef _UNIRTOS_ATCMD_AUDIO_TASK_H_
#define _UNIRTOS_ATCMD_AUDIO_TASK_H_
#include "qcm_audio.h"
#include "qosa_audio.h"
#include "qcm_file_api.h"
#include "qosa_at_param.h"
#include "qosa_system_config.h"
#include "qosa_tts.h"
#if defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC)/* adapter_todo */
#include "QuecPrjName.h"
#elif defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC)
#include "qosa_plt_pro_config.h"
#else // adapter_todo
#include "QosaPrjName.h"
#endif

#define QOSA_ES8311_I2C_ADDR   0x18 /*!< 8311 codec slave address */
#define QOSA_ALC5616_I2C_ADDR  0x1b /*!< 5616 codec slave address */
#ifdef __QUECTEL_PROJECT_EG915ZEU_LA__
#define QOSA_IIC_SDA_NUM       41  /*!< IIC SDA pin number */
#define QOSA_IIC_SCL_NUM       40  /*!< IIC SCL pin number */
#define QOSA_IIC_FUNC          3    /*!< FUNC corresponding to IIC */
#elif defined(__QUECTEL_PROJECT_EC800UGCN_LB__)
#define QOSA_IIC_SDA_NUM       66  /*!< IIC SDA pin number */
#define QOSA_IIC_SCL_NUM       67  /*!< IIC SCL pin number */
#define QOSA_IIC_FUNC          3    /*!< FUNC corresponding to IIC */
#elif defined(CONFIG_PROJECT_EG810M_EU_LA__)
#define QOSA_IIC_SDA_NUM       66  /*!< IIC SDA pin number */
#define QOSA_IIC_SCL_NUM       67  /*!< IIC SCL pin number */
#define QOSA_IIC_FUNC          1    /*!< FUNC corresponding to IIC */
#elif defined(CONFIG_PROJECT_DEF_EC200M)
#define QOSA_IIC_SDA_NUM       42   /*!< IIC SDA pin number */
#define QOSA_IIC_SCL_NUM       41   /*!< IIC SCL pin number */
#define QOSA_IIC_FUNC          1    /*!< FUNC corresponding to IIC */
#else
#define QOSA_IIC_SDA_NUM       66   /*!< IIC SDA pin number */
#define QOSA_IIC_SCL_NUM       67   /*!< IIC SCL pin number */
#define QOSA_IIC_FUNC          2    /*!< FUNC corresponding to IIC */
#endif

#if defined(__QUECTEL_PROJECT_EC800ZCN_MF__) || defined(__QUECTEL_PROJECT_EC800ZCN_LF__)
#define QOSA_IIC_NO          QOSA_I2C_0    /*!< serial number to IIC */
#elif defined(__QUECTEL_PROJECT_EC800UGCN_LB__)
#define QOSA_IIC_NO          QOSA_I2C_2    /*!< serial number to IIC */
#elif defined(CONFIG_PROJECT_EG810M_EU_LA__) || defined(CONFIG_PROJECT_DEF_EC200M)
#define QOSA_IIC_NO          QOSA_I2C_0    /*!< serial number to IIC */
#elif defined(__UNIRTOS_PROJECT_RG255AA__)
#define QOSA_IIC_NO          QOSA_I2C_0    /*!< serial number to IIC */
#else
#define QOSA_IIC_NO          QOSA_I2C_1    /*!< serial number to IIC */
#endif
#define QOSA_IIC_INIT_DELAY    1000 /*!< Delay time after IIC initialization */
#define QOSA_IIC_REG_CFG_DELAY 1    /*!< Delay time after IIC register configuration */
#define QOSA_TTS_PLAY_DELAY    200  /*!< Delay time after TTS playback */
#define QOSA_CODEC_8311_ID     0x83   /*!< ES8311 codec ID */
#define QOSA_CODEC_5616_ID     0x10EC /*!< ALC5616 codec ID */

 //Standard project default differential output, PWM0 function on pins 100/101. If other projects require changes later, simply use macros to differentiate and change the pin numbers and PWM numbers.
#if defined(QL_OEM_VERSION_BBN)
#define QOSA_AUD_PWM_SPK_N    32//pin32
#define QOSA_AUD_PWM_SPK_P    0//not use
#define QOSA_AUD_PWM_NUM    2//pwm2 reference pinmux
#else/* QL_OEM_VERSION_BBN */
#define QOSA_AUD_PWM_SPK_N    100//pin100
#define QOSA_AUD_PWM_SPK_P    101//pin101
#define QOSA_AUD_PWM_NUM    0//pwm0 reference pinmux
#endif/* QL_OEM_VERSION_BBN */
#define QOSA_AUD_PWM_FUNC   5//func5 reference pinmux
#define QOSA_AUD_PWMN_FUNC   3//func3 reference pinmux

#if defined(CONFIG_PROJECT_EG810M_EU_LA__)
#define QOSA_AUD_PA_PIN        21   /*!< PA control pin, standard project only    */
#define QOSA_AUD_PA_GPIO       124   /*!< PA control GPIO, standard projects only */
#else
#define QOSA_AUD_PA_PIN        76   /*!< PA control pin, standard project only    */
#define QOSA_AUD_PA_GPIO       17   /*!< PA control GPIO, standard projects only */
#endif

// es8311 codec register configuration
#if defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC)
#define ES8311_INIT_REG                                                                                                                                        \
    {                                                                                                                                                          \
        {0x01, 0x30, 0x00}, {0x02, 0x00, 0x00}, {0x03, 0x20, 0x00}, {0x16, 0x20, 0x00}, {0x04, 0x20, 0x00}, {0x05, 0x00, 0x00}, {0x0B, 0x00, 0x00},            \
            {0x0C, 0x00, 0x00}, {0x0F, 0x44, 0x00}, {0x10, 0x1F, 0x00}, {0x11, 0x7F, 0x00}, {0x00, 0x80, 0x00}, {0x00, 0x80, 0x00}, {0x01, 0x3F, 0x00},        \
            {0x01, 0xBF, 0x00}, {0x02, 0x18, 0x00}, {0x05, 0x00, 0x00}, {0x03, 0x20, 0x00}, {0x16, 0x24, 0x00}, {0x04, 0x20, 0x00},{0x07, 0x00, 0x00}, {0x08, 0x3F, 0x00},        \
            {0x06, 0x23, 0x00}, {0x01, 0xBF, 0x00}, {0x06, 0x23, 0x00}, {0x13, 0x10, 0x00}, {0x1B, 0x0A, 0x00}, {0x1C, 0x6A, 0x00}, {0x09, 0x0F, 0x00},        \
            {0x0A, 0x0F, 0x00}, {0x09, 0x0F, 0x00}, {0x0A, 0x0F, 0x00}, {0x32, 0xBF, 0x00}, {0x09, 0x0F, 0x00}, {0x0A, 0x0F, 0x00}, {0x17, 0xBF, 0x00},        \
            {0x0E, 0x02, 0x00}, {0x12, 0x00, 0x00}, {0x14, 0x1A, 0x00}, {0x14, 0x1A, 0x00}, {0x0D, 0x01, 0x00}, {0x15, 0x20, 0x00}, {0x37, 0x48, 0x00},        \
            {0x45, 0x00, 0x00}, {0x0D, 0x01, 0x00}, {0x45, 0x00, 0x00}, {0x37, 0x08, 0x00}, {0x14, 0x1A, 0x00}, {0x12, 0x00, 0x00}, {0x0E, 0x00, 0x00},        \
            {0x32, 0xBF, 0x00},                                                                                                                                \
    }
/* alc5616 codec register configuration, 2-byte register width */
#define ALC5616_INIT_REG                                                                                                                                        \
    {                                                                                                                                                          \
        {0x0002, 0x0808, 0x00}, {0x0003, 0x0808, 0x00}, {0x0005, 0x8000, 0x00}, {0x000D, 0x04C0, 0x00}, {0x000F, 0x0808, 0x00},                \
            {0x0019, 0xAFAF, 0x00}, {0x001C, 0x5C5C, 0x00}, {0x001E, 0x0000, 0x00}, {0x0027, 0x3820, 0x00}, {0x0029, 0x8080, 0x00},            \
            {0x002A, 0x1250, 0x00}, {0x003B, 0x0000, 0x00}, {0x003C, 0x004F, 0x00}, {0x003D, 0x2000, 0x00}, {0x003E, 0x004F, 0x00},            \
            {0x0045, 0x4000, 0x00}, {0x004D, 0x0000, 0x00}, {0x004E, 0x0000, 0x00}, {0x004F, 0x0278, 0x00}, {0x0050, 0x0000, 0x00},            \
            {0x0051, 0x0000, 0x00}, {0x0052, 0x0278, 0x00}, {0x0053, 0xC800, 0x00}, {0x0061, 0x9806, 0x00}, {0x0062, 0x8800, 0x00},            \
            {0x0063, 0xF8FE, 0x00}, {0x0064, 0x0A00, 0x00}, {0x0065, 0xCC00, 0x00}, {0x0066, 0x3F00, 0x00}, {0x006A, 0x003D, 0x00},            \
            {0x006C, 0x3600, 0x00}, {0x0070, 0x8082, 0x00}, {0x0073, 0x0000, 0x00}, {0x0074, 0x0C00, 0x00}, {0x0080, 0x5000, 0x00},            \
            {0x0081, 0x0E1C, 0x00}, {0x0082, 0x0800, 0x00}, {0x008E, 0x0019, 0x00}, {0x008F, 0x3100, 0x00}, {0x0093, 0x2C00, 0x00},            \
            {0x0094, 0x0200, 0x00}, {0x00B0, 0x2080, 0x00}, {0x00B1, 0x0000, 0x00}, {0x00FA, 0x0011, 0x00},                                  \
    }

#else
#define ES8311_INIT_REG                                                                                                                                        \
    {                                                                                                                                                          \
        {0x01, 0x30, 0x00}, {0x02, 0x00, 0x00}, {0x03, 0x20, 0x00}, {0x16, 0x20, 0x00}, {0x04, 0x20, 0x00}, {0x05, 0x00, 0x00}, {0x0B, 0x00, 0x00},            \
            {0x0C, 0x00, 0x00}, {0x0F, 0x44, 0x00}, {0x10, 0x1F, 0x00}, {0x11, 0x7F, 0x00}, {0x00, 0x80, 0x00}, {0x00, 0x80, 0x00}, {0x01, 0x3F, 0x00},        \
            {0x01, 0xBF, 0x00}, {0x02, 0x18, 0x00}, {0x05, 0x00, 0x00}, {0x03, 0x10, 0x00}, {0x04, 0x10, 0x00}, {0x07, 0x00, 0x00}, {0x08, 0xFF, 0x00},        \
            {0x06, 0x03, 0x00}, {0x01, 0xBF, 0x00}, {0x06, 0x03, 0x00}, {0x13, 0x10, 0x00}, {0x1B, 0x0A, 0x00}, {0x1C, 0x6A, 0x00}, {0x09, 0x0D, 0x00},        \
            {0x0A, 0x0D, 0x00}, {0x09, 0x0D, 0x00}, {0x0A, 0x0D, 0x00}, {0x32, 0xBF, 0x00}, {0x09, 0x0D, 0x00}, {0x0A, 0x0D, 0x00}, {0x17, 0xBF, 0x00},        \
            {0x0E, 0x02, 0x00}, {0x12, 0x00, 0x00}, {0x14, 0x1A, 0x00}, {0x14, 0x1A, 0x00}, {0x0D, 0x01, 0x00}, {0x15, 0x20, 0x00}, {0x37, 0x48, 0x00},        \
            {0x45, 0x00, 0x00}, {0x0D, 0x01, 0x00}, {0x45, 0x00, 0x00}, {0x37, 0x08, 0x00}, {0x14, 0x1A, 0x00}, {0x12, 0x00, 0x00}, {0x0E, 0x00, 0x00},        \
            {0x32, 0xBF, 0x00},                                                                                                                                \
    }

#endif
#define QOSA_FILE_RESEVED_SIZE   8 * 1024 /*!< Recording reserved space. An error will be reported directly if the value is less than this. */
#define QOSA_MSQ_RELEASE_TIMEOUT 5        /*!< release wait timeout */
#define QOSA_MSQ_CREAT_CNT_NUM   20       /*!< Number of message queues to create */
#define QOSA_TASK_STACK_SIZE     4096     /*!< AUDIO task stack size */

typedef enum
{
    QOSA_AUDIO_PLAY_TYPE_NOT_SUPPORTED, /*!< Format not supported */
    QOSA_AUDIO_PLAY_TYPE_PCM,           /*!< PCM format */
    QOSA_AUDIO_PLAY_TYPE_WAV,           /*!< WAV format */
    QOSA_AUDIO_PLAY_TYPE_MP3,           /*!< MP3 format */
    QOSA_AUDIO_PLAY_TYPE_AMRNB,         /*!< AMRNB format */
    QOSA_AUDIO_PLAY_TYPE_AMRWB,         /*!< AMRWB format */
} qosa_audio_play_type_e;

typedef enum
{
    QOSA_AUDIO_UNINIT,          /*!< AUDIO not initialized */
    QOSA_AUDIO_RESOURCE_DONE,   /*!< AUDIO resource initialization completed */
    QOSA_AUDIO_INIT_DONE        /*!< AUDIO initialization completed */
} qosa_audio_init_status_e;

typedef enum
{
    QOSA_AUDIO_NOT_WORK, /*!< AUDIO is not in working state */
    QOSA_AUDIO_WORKING   /*!< AUDIO is in working state */
} qosa_audio_work_status_e;

typedef enum
{
    QOSA_AT_AUDIO_PLAY, /*!< AT PLAY command playback */
    QOSA_AT_QPSND,      /*!< Play via AT QPSND command */
    QOSA_AT_CTRL_MAX
} qosa_audio_at_type_e;

typedef enum
{
    QOSA_TTS_NOT_PLAY,   /*!< TTS playback started */
    QOSA_TTS_PLAYING,    /*!< TTS is playing */
    QOSA_TTS_PLAY_FINISH, /*!< TTS playback completed */
    QOSA_TTS_STOP         /*!< TTS playback stop request */
} qosa_audio_tts_control_e;

typedef struct
{
    qosa_uint32_t regAddr; /*!< codec register address */
    qosa_uint16_t val;     /*!< value of codec configuration */
    qosa_uint16_t delay;   /*!< codec configuration delay time */
} qosa_audio_codec_reg_t;

/**
 * @brief Codec driver descriptor.
 * Each entry describes how to probe and initialize a codec chip via I2C.
 * The table g_codec_drivers[] is populated per-platform.
 */
typedef struct
{
    qosa_uint8_t            i2c_addr;   /*!< codec I2C slave address     */
    qosa_uint8_t            probe_reg;  /*!< register addr for chip ID   */
    qosa_uint16_t           probe_val;  /*!< expected chip ID            */
    qosa_uint8_t            swap_bytes; /*!< 1 = swap to big-endian      */
    qosa_audio_codec_reg_t *reg_table;  /*!< init register table         */
    qosa_uint16_t           reg_count;  /*!< number of init registers    */
    qosa_uint8_t            reg_bytes;  /*!< register width (1 or 2)     */
} qosa_codec_drv_t;

typedef struct
{
    char                   file_name[QCM_FILE_MAX_PATH_LEN + 1]; /*!< File name */
    qosa_bool_t            audioplay_repeat;                     /*!< Play count single/repeat */
    qosa_audio_play_type_e type;                                 /*!< Playback file type */
    qosa_audio_at_type_e   at_type;                              /*!< AT command type */
    qosa_bool_t            control;                              /*!< QPSND control: 0=stop, 1=start */
    qosa_int32_t           ulmute;                               /*!< QPSND uplink: -1=not given, 0=mute, 1=unmute */
    qosa_int32_t           dlmute;                               /*!< QPSND downlink: -1=not given, 0=mute, 1=unmute */
} qosa_atcmd_audio_play_t;

typedef enum
{
    QL_REC_TYPE_NONE = 0, /*!< Recording type None */
    QL_REC_TYPE_MIC,      /*!< Recording type MIC */

    QL_REC_TYPE_MAX,
} qosa_record_type_e;

typedef struct
{
    char               file_name[QCM_FILE_MAX_PATH_LEN + 1]; /*!< File name */
    char               control;                              /*!< Start/Stop Control */
    qosa_record_type_e Type;                                 /*!< Recording type */
    qosa_uint8_t       dlink;                                /*!< 0: uplink, 1: downlink */
} qosa_atcmd_audio_record_t;

typedef struct
{
    int                      mode;     /*!< TTS mode */
    char                    *text;     /*!< TTS text */
    int                      len;      /*!< TTS text length */
    qosa_audio_tts_control_e set_type; /*!< TTS control type */
} qosa_atcmd_audio_tts_t;

/**
 * @brief Audio loopback control message structure
 */
typedef struct
{
    qosa_uint8_t        control;      /*!< 0=stop, 1=start */
    qosa_uint16_t       samprate;     /*!< Sample rate, e.g. 8000, 16000 */
    qosa_uint8_t        channels;     /*!< Number of channels, 1 or 2 */
} qosa_atcmd_audio_loop_t;

/**
 * @brief Records the audio initialization status
 *
 */
qosa_uint8_t qosa_at_aud_get_init_done(void);

/**
 * @brief Sets the audio initialization status
 *
 */
void qosa_at_aud_set_init_done(qosa_uint8_t init_done);

/**
 * @brief AT+QAUDPLAY command processing function
 *
 */
void unir_exec_qaudplay_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QPSND command processing function
 *
 */
void unir_exec_qpsnd_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QAUDRD command processing function
 *
 */
void unir_exec_qaudrd_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QTONEDET command function.
 *
 */
void unir_exec_qtonedet_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AUDIO task initialization function
 *
 */
int qosa_atcmd_audio_task_init(void);

/**
 * @brief Codec probe and initialization function
 *
 */
int qosa_codec_probe_and_init(void);

/**
 * @brief Get audio working status
 *
 */
qosa_uint8_t qosa_at_aud_get_work_status(void);

/**
 * @brief Get audio loopback running status
 *
 */
qosa_bool_t qosa_atcmd_audio_loop_is_running(void);

/**
 * @brief Set audio working status
 *
 */
void qosa_at_aud_set_work_status(qosa_uint8_t work_status);

/**
 * @brief AT+QAUDSTOP command processing function
 *
 */
void unir_exec_qaudstop_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+CLVL command processing function
 *
 */
void unir_exec_clvl_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QDAI command processing function
 *
 */
void unir_exec_qdai_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QAUDSW command processing function
 *
 */
void unir_exec_qaudsw_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QAUDLOOP command processing function - audio loopback test
 *
 */
void unir_exec_qaudloop_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief Stop audio loopback
 *
 */
void qosa_atcmd_audio_loop_stop(void);

/**
 * @brief AT+QIIC command processing function - I2C bus read/write
 *
 */
void unir_exec_qiic_cmd(qosa_at_cmd_t *cmd);

#ifdef CONFIG_QOSA_TTS_SUPPORT
/**
 * @brief AT+QTTS command processing function
 *
 */
void unir_exec_qtts_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief Get TTS running status
 *
 */
qosa_uint8_t qosa_tts_is_running(void);

/**
 * @brief Set TTS running status
 *
 */
void qosa_tts_set_running(qosa_uint8_t work_status);

/**
 * @brief AT+QTTSETUP command processing function
 *
 */
void unir_exec_qttsetup_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief Get SIO output device parameters
 *
 */
qosa_sio_output_e qosa_sio_output_device_get(void);

/**
 * @brief Set SIO output device parameters
 *
 */
void qosa_sio_output_device_set(qosa_sio_output_e sio_output);
#endif /* CONFIG_QOSA_TTS_SUPPORT */

#endif /* _UNIRTOS_ATCMD_AUDIO_TASK_H_ */
