/*****************************************************************/ /**
* @file qcm_audio.h
* @brief
* @author Kevin.wang@quectel.com
* @date 2025-09-10
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-09-10 <td>1.0 <td>Kevin.wang <td> Init
* </table>
**********************************************************************/

#ifndef __QCM_AUDIO_H__
#define __QCM_AUDIO_H__

#include "qosa_audio.h"

/*===========================================================================
 * Macro
===========================================================================*/
#define QCM_AUD_FILE_NAME_MAX (64)

/**
 * @enum qosa_aud_event_e
 * @brief Event Reporting Code
 */
typedef enum
{
    QOSA_AUD_EVT_START_PLAY = 1,      /*!< Playback started*/
    QOSA_AUD_EVT_START_RECORD,        /*!< Recording start*/
    QOSA_AUD_EVT_PLAY_DONE,           /*!< All data playback completed, determined based on the reporting capabilities of each platform.*/
    QOSA_AUD_EVT_PLAY_LOW_LEVEL,      /*!< Playback buffer is about to run out*/
    QOSA_AUD_EVT_RECORD_FIFO_OVERRUN, /*!< Buffer overflow, used during recording, to alert the user that some data has been lost due to the buffer being full.*/
    QOSA_AUD_EVT_FIFO_HIGH_LEVEL,     /*!< Cache is nearing overflow, used during recording to prompt the user to immediately read out the cached data.*/
    QOSA_AUD_EVT_FIFO_LOW_LEVEL,      /*!< The buffer is nearing playback control, data needs to be filled, otherwise it will cause playback stuttering.*/
    QOSA_AUD_EVT_PLAY_ERR,            /*!< Playback abnormally exited*/
    QOSA_AUD_EVT_RECORD_ERR,          /*!< Recording abnormally exited*/
} qosa_aud_event_e;

/**
 * @enum qcm_aud_type_e
 * @brief Audio Playback Mode
 */
typedef enum
{
    QCM_AUD_TYPE_LOCAL,
    QCM_AUD_TYPE_VOICE
} qcm_aud_type_e;

/**
 * @struct  qosa_aud_file_cfg_t
 * @brief Audio file playback configuration structure
 */
typedef struct
{
    qosa_uint8_t        file_name[QCM_AUD_FILE_NAME_MAX]; /*!< File name*/
    qcm_aud_type_e      type;                             /*!< Playback type, local or remote*/
    qosa_uint32_t       cache_size;                       /*!< Cache size*/
    qosa_uint32_t       skip_frams;                       /*!< Number of frame skips (to be discussed if needed?)*/
    qosa_aud_callback_t callback;                         /*!< Callback function*/
    qosa_uint64_t       options;                          /*!< Platform-specific configuration, each platform interprets independently*/
    qosa_aud_route_e    route;                            /*!< Playback destination: near / far / both*/
} qosa_aud_file_cfg_t;

/**
 * @struct  qosa_aud_rec_file_cfg_t
 * @brief audio recording file configuration structure
 */
typedef struct
{
    qosa_uint8_t        file_name[QCM_AUD_FILE_NAME_MAX]; /*!< File name*/
    qosa_uint32_t       samprate;                         /*!< Sampling rate*/
    qosa_uint8_t        channels;                         /*!< Number of channels*/
    qosa_aud_fmt_e      format;                           /*!< Stream format*/
    qosa_aud_callback_t callback;                         /*!< Callback function*/
    qosa_uint64_t       options;                          /*!< Platform-specific configuration, each platform interprets independently*/
    qosa_aud_route_e    route;                            /*!< Recording source: near / far / both*/
} qosa_aud_rec_file_cfg_t;

/**
 * @brief Start recording and save to file.
 *
 * @param[in]  qosa_aud_rec_file_cfg_t *config
 *               - Pointer to the audio recording configuration parameter structure, containing information such as the recording file name.
 *
 * @param[out] qosa_aud_handle_t *handle
 *               - The returned audio handle, used for subsequent control and stopping of recording.
 *
 * @return qosa_aud_errcode_e
 *               - Returns QOSA_AUD_SUCCESS on success, or the corresponding error code on failure.
 */
qosa_aud_errcode_e qcm_aud_record_file(qosa_aud_rec_file_cfg_t *config, qosa_aud_handle_t *handle);

/**
 * @brief Play the specified audio file
 *
 * @param[in]  qosa_aud_file_cfg_t *config
 *              - Pointer to the audio file configuration structure, containing information such as the file name to be played
 *
 * @param[out] qosa_aud_handle_t *handle
 *              - Returns the created audio playback handle for subsequent control of playback operations
 *
 * @return qosa_aud_errcode_e
 *         - QOSA_AUD_SUCCESS: Successfully started playback
 *         - QOSA_AUD_INVALID_PARAM: Invalid parameter (e.g., config or handle is null)
 *         - QOSA_AUD_MUTEX_ERR: Failed to acquire mutex
 *         - QOSA_AUD_NO_MEMORY_ERR: Memory allocation failed
 *         - QOSA_AUD_FILE_OPEN_ERR: File open failed
 *         - QOSA_AUD_SYSTEM_ERR: Audio stream open failure or other system error
 */
qosa_aud_errcode_e qcm_aud_play_file(qosa_aud_file_cfg_t *config, qosa_aud_handle_t *handle);

/**
 * @brief Stop audio file recording
 *
 * @param qosa_aud_handle_t handle
 *          - Audio file handle, pointing to the audio file context structure
 *
 * @return qosa_aud_errcode_e
 *          - Error code, returns 0 for success
 */
qosa_aud_errcode_e qcm_aud_record_file_stop(qosa_aud_handle_t handle);

/**
 * @brief Stop audio file playback
 *
 * @param qosa_aud_handle_t handle
 * 		   - Audio file handle, pointing to the audio file context structure
 *
 * @return qosa_aud_errcode_e
 *         - Error code, returns 0 for success
 */
qosa_aud_errcode_e qcm_aud_play_file_stop(qosa_aud_handle_t handle);

/**
 * @brief Close the audio stream
 *
 * @param qosa_aud_handle_t handle
 *          - Audio stream handle, pointing to the audio stream object to be closed
 *
 * @param qosa_bool_t force
 *          - Whether to force close, TRUE indicates forced close, FALSE indicates normal close
 *
 * @return qosa_int32_t
 *          - Returns the operation result, where 0 indicates success and any other value indicates failure.
 */
qosa_int32_t qcm_aud_stream_close(qosa_aud_handle_t handle, qosa_bool_t force);

/**
 * @brief Read data from the audio stream
 *
 * @param qosa_aud_handle_t handle
 *           - Audio stream handle
 *
 * @param qosa_uint8_t *data
 *           - Pointer to the buffer for storing read data
 *
 * @param qosa_uint32_t size
 *           - Size of data to be read
 *
 * @return qosa_int32_t
 *           - Returns the number of bytes read on success, or an error code on failure.
 */
qosa_int32_t qcm_aud_stream_read(qosa_aud_handle_t handle, qosa_uint8_t *data, qosa_uint32_t size);

/**
 * @brief Write data to the audio playback stream
 *
 * @param qosa_aud_handle_t handle
 *          - Audio playback stream handle, returned by qcm_aud_stream_open
 *
 * @param qosa_uint8_t *data
 *           - Points to the buffer for audio data to be written
 *
 * @param qosa_uint32_t size
 *           - Number of bytes to write
 *
 * @return Returns the actual written data size (bytes) on success, or an error code on failure:
 *         - QOSA_AUD_MUTEX_ERR: Invalid parameter or mutex acquisition failed
 *         - QOSA_AUD_SYSTEM_ERR: System-level error, such as player write failure
 *         - QOSA_AUD_SUCCESS: Successfully completed in synchronous mode
 */
qosa_int32_t qcm_aud_stream_write(qosa_aud_handle_t handle, qosa_uint8_t *data, qosa_uint32_t size);

/**
 * @brief Open audio stream
 *
 * @param[in] qosa_aud_stream_cfg_t * config
 *              - Audio stream configuration parameter pointer
 *
 * @param[out] qosa_aud_handle_t *handle
 *              - Audio stream handle pointer
 *
 * @return qosa_aud_errcode_e Error Code
 *         - QOSA_AUD_INVALID_PARAM: Invalid parameter
 *         - Other error codes: Return corresponding error codes based on the specific operation
 */
qosa_aud_errcode_e qcm_aud_stream_open(qosa_aud_stream_cfg_t *config, qosa_aud_handle_t *handle);
#endif /* __QCM_AUDIO_H__ */
