// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic Audio Player Types
 *
 * This file defines the common data types, enumerations, and structures used by the
 * audio player module. These types are used to configure and control audio playback
 * operations including playlist management, playback control, and event handling.
 */

/**
 * @brief Audio player error code definitions
 */
typedef enum
{
    AUDIO_PLAYER_OK = 0,              /*!< Operation successful */
    AUDIO_PLAYER_ERR = -100,          /*!< General error */
    AUDIO_PLAYER_NOT_INIT,            /*!< Audio player not initialized */
    AUDIO_PLAYER_NO_MEM,              /*!< Out of memory */
    AUDIO_PLAYER_PROGRESS,            /*!< Operation in progress */
    AUDIO_PLAYER_INVALID,             /*!< Invalid parameter */
    AUDIO_PLAYER_TIMEOUT = -200,      /*!< Operation timeout */
} audio_player_error_code_t;

/**
 * @brief Audio player mode definitions
 */
typedef enum
{
    AUDIO_PLAYER_MODE_ONE_SONG = 0,        /*!< Play single song */
    AUDIO_PLAYER_MODE_SEQUENCE,            /*!< Play in sequence */
    AUDIO_PLAYER_MODE_ONE_SONG_LOOP,       /*!< Loop single song */
    AUDIO_PLAYER_MODE_SEQUENCE_LOOP,       /*!< Loop sequence */
    AUDIO_PLAYER_MODE_RANDOM               /*!< Random play */
} audio_player_mode_t;

/**
 * @brief Default audio player mode
 */
#define AUDIO_PLAYER_MODE_DEFAULT   AUDIO_PLAYER_MODE_SEQUENCE_LOOP

/**
 * @brief Audio player event type definitions
 */
typedef enum
{
    AUDIO_PLAYER_EVENT_SONG_START,           /*!< Song started */
    AUDIO_PLAYER_EVENT_SONG_FINISH,          /*!< Song finished */
    AUDIO_PLAYER_EVENT_SONG_FAILURE,         /*!< Song playback failed */
    AUDIO_PLAYER_EVENT_SONG_PAUSE,           /*!< Song paused */
    AUDIO_PLAYER_EVENT_SONG_RESUME,          /*!< Song resumed */

    AUDIO_PLAYER_EVENT_SONG_TICK,            /*!< Song playback tick */
    AUDIO_PLAYER_EVENT_SEEK_COMPLETE,        /*!< Seek operation finished */

    AUDIO_PLAYER_EVENT_LAST,
} audio_player_event_type_t;

/**
 * @brief Maximum length of metadata string fields (e.g. title, artist)
 */
#define AUDIO_METADATA_MAX_STRING_LEN   256

/**
 * @brief Audio format enumeration
 *
 * Used by metadata parser ops and decoder selection. Each value corresponds to
 * a supported container or codec type.
 */
typedef enum
{
    AUDIO_FORMAT_UNKNOWN = 0,  /*!< Unknown or unsupported format */
    AUDIO_FORMAT_MP3,          /*!< MP3 */
    AUDIO_FORMAT_WAV,          /*!< WAV */
    AUDIO_FORMAT_AAC,          /*!< AAC */
    AUDIO_FORMAT_AMR,          /*!< AMR */
    AUDIO_FORMAT_FLAC,         /*!< FLAC */
    AUDIO_FORMAT_OGG,          /*!< OGG */
    AUDIO_FORMAT_OPUS,         /*!< Opus */
    AUDIO_FORMAT_M4A,          /*!< M4A */
    AUDIO_FORMAT_TS,           /*!< MPEG-TS */
} audio_format_t;

/**
 * @brief Audio sink type
 *
 * Identifies the kind of output: device (e.g. speaker) or file.
 */
typedef enum
{
    AUDIO_SINK_DEVICE,  /*!< Output to audio device (e.g. onboard speaker) */
    AUDIO_SINK_FILE,    /*!< Output to file */
} audio_sink_type_t;

/**
 * @brief Audio sink control command
 *
 * Commands that can be sent to a sink via the control callback.
 */
typedef enum
{
    AUDIO_SINK_PAUSE,             /*!< Pause playback */
    AUDIO_SINK_RESUME,            /*!< Resume playback */
    AUDIO_SINK_MUTE,              /*!< Mute */
    AUDIO_SINK_UNMUTE,            /*!< Unmute */
    AUDIO_SINK_FRAME_INFO_CHANGE, /*!< Frame format changed */
    AUDIO_SINK_SET_VOLUME,        /*!< Set volume */
} audio_sink_control_t;

/**
 * @brief Audio metadata structure
 *
 * Holds parsed metadata for a track (title, artist, album, etc.) and basic
 * stream info (duration, bitrate, sample rate, channels).
 */
typedef struct
{
    char title[AUDIO_METADATA_MAX_STRING_LEN];          /*!< Track title */
    char artist[AUDIO_METADATA_MAX_STRING_LEN];         /*!< Artist */
    char album[AUDIO_METADATA_MAX_STRING_LEN];          /*!< Album */
    char album_artist[AUDIO_METADATA_MAX_STRING_LEN];   /*!< Album artist */
    char genre[AUDIO_METADATA_MAX_STRING_LEN];          /*!< Genre */
    char year[AUDIO_METADATA_MAX_STRING_LEN];           /*!< Year */
    char composer[AUDIO_METADATA_MAX_STRING_LEN];       /*!< Composer */
    char track_number[AUDIO_METADATA_MAX_STRING_LEN];   /*!< Track number */

    double duration;                                    /*!< Duration in seconds */
    int bitrate;                                        /*!< Bitrate in bps */
    int sample_rate;                                    /*!< Sample rate in Hz */
    int channels;                                       /*!< Channel count */

    int has_id3v1;                                      /*!< Whether ID3v1 tag is present */
    int has_id3v2;                                      /*!< Whether ID3v2 tag is present */
} audio_metadata_t;

/** Opaque source instance; see bk_audio_player_source_ops_t for operations */
typedef struct bk_audio_player_source bk_audio_player_source_t;

/**
 * @brief Audio source operations
 *
 * Callbacks implemented by a source plugin to open a URL, read data, and optionally seek.
 */
typedef struct bk_audio_player_source_ops
{
    int (*open)(char *url, bk_audio_player_source_t **source_pp);                   /*!< Open URL and create source instance */
    int (*get_codec_type)(bk_audio_player_source_t *source);                        /*!< Return audio_format_t for stream */
    uint32_t (*get_total_bytes)(bk_audio_player_source_t *source);                  /*!< Total bytes (optional) */
    int (*read)(bk_audio_player_source_t *source, char *buffer, int len);           /*!< Read data */
    int (*seek)(bk_audio_player_source_t *source, int offset, uint32_t whence);     /*!< Seek (optional) */
    int (*close)(bk_audio_player_source_t *source);                                 /*!< Close and release */
} bk_audio_player_source_ops_t;

/** Opaque sink instance; see bk_audio_player_sink_ops_t for operations */
typedef struct bk_audio_player_sink bk_audio_player_sink_t;

/**
 * @brief Audio sink operations
 *
 * Callbacks implemented by a sink plugin to open output, write PCM, and control (pause/resume/volume).
 */
typedef struct bk_audio_player_sink_ops
{
    int (*open)(audio_sink_type_t sink_type, void *param, bk_audio_player_sink_t **sink_pp); /*!< Open sink */
    int (*write)(bk_audio_player_sink_t *sink, char *buffer, int len);                       /*!< Write PCM data */
    int (*control)(bk_audio_player_sink_t *sink, audio_sink_control_t control);              /*!< Control (optional) */
    int (*close)(bk_audio_player_sink_t *sink);                                              /*!< Close and release */
} bk_audio_player_sink_ops_t;

/** Opaque decoder instance; see bk_audio_player_decoder_ops_t for operations */
typedef struct bk_audio_player_decoder bk_audio_player_decoder_t;

/**
 * @brief Decoded stream / frame information
 *
 * Describes channel count, sample rate, sample bits, frame size, bitrate, and duration.
 */
typedef struct audio_info_s
{
    int channel_number;     /*!< Number of channels */
    int sample_rate;        /*!< Sample rate in Hz */
    int sample_bits;        /*!< Bits per sample */
    int frame_size;         /*!< Frame size in bytes */

    int bps;                /*!< Bits per second */
    uint32_t total_bytes;   /*!< Total bytes in stream (if known) */
    uint32_t header_bytes;  /*!< Header size in bytes */
    double duration;        /*!< Duration in milliseconds */
} audio_info_t;

/**
 * @brief Audio decoder operations
 *
 * Callbacks implemented by a decoder plugin to open by format, get stream info, and decode data.
 */
typedef struct bk_audio_player_decoder_ops
{
    const char *name;                                                                           /*!< Decoder name identifier (e.g. "aac", "mp3", "wav") */
    int (*open)(audio_format_t format, void *param, bk_audio_player_decoder_t **decoder_pp);    /*!< Open decoder */
    int (*get_info)(bk_audio_player_decoder_t *decoder, audio_info_t *info);                    /*!< Get stream info */
    int (*get_chunk_size)(bk_audio_player_decoder_t *decoder);                                  /*!< Preferred chunk size */
    int (*get_data)(bk_audio_player_decoder_t *decoder, char *buffer, int len);                 /*!< Decode and return PCM */
    int (*close)(bk_audio_player_decoder_t *decoder);                                           /*!< Close */
    int (*calc_position)(bk_audio_player_decoder_t *decoder, int second);                       /*!< Byte offset for seek */
    int (*is_seek_ready)(bk_audio_player_decoder_t *decoder);                                   /*!< Whether seek is ready */
} bk_audio_player_decoder_ops_t;

/**
 * @brief Metadata parser operations
 *
 * Callbacks to probe file type and parse metadata (e.g. ID3, Vorbis comment) into audio_metadata_t.
 */
typedef struct bk_audio_player_metadata_parser_ops
{
    const char *name;                                                           /*!< Parser name identifier */
    audio_format_t format;                                                      /*!< Format this parser handles (e.g. AUDIO_FORMAT_MP3, AUDIO_FORMAT_WAV) */
    int (*probe)(const char *filepath);                                         /*!< Probe whether this parser supports the file */
    int (*parse)(int fd, const char *filepath, audio_metadata_t *metadata);     /*!< Parse metadata into structure */
} bk_audio_player_metadata_parser_ops_t;

/**
 * @brief Event handler function type
 *
 * @param event Event type, see audio_player_event_type_t
 * @param extra_info Extra information for the event
 * @param args User defined arguments
 */
typedef void (*audio_player_event_handler_func)(audio_player_event_type_t event, void *extra_info, void *args);

/**
 * @brief Result payload for AUDIO_PLAYER_EVENT_SEEK_COMPLETE
 */
typedef struct
{
    int second;     /*!< Target second requested */
    int status;     /*!< Result code (AUDIO_PLAYER_OK on success) */
} audio_player_seek_result_t;

/**
 * @brief Opaque handle for an audio player instance.
 * Each instance has its own plugin lists and state; multiple instances can coexist.
 */
typedef struct bk_audio_player *bk_audio_player_handle_t;

/**
 * @brief Audio player configuration structure
 *
 * This structure defines the configuration parameters for the audio player,
 * including event handling, user-defined parameters, and internal task settings.
 */
typedef struct
{
    uint32_t                         task_stack;     /*!< Stack size in bytes for the player internal task; 0 to use default */
    int                              task_prio;      /*!< Priority for the player internal task; follows RTOS convention (e.g. BEKEN_DEFAULT_WORKER_PRIORITY) */
    audio_player_event_handler_func  event_handler;  /*!< Event handler callback function */
    void *                           args;           /*!< User defined arguments for the event handler function */
} bk_audio_player_cfg_t;

/**
 * @brief Default stack size for the player internal task (bytes)
 */
#define AUDIO_PLAYER_DEFAULT_TASK_STACK  16384

/**
 * @brief Default audio player configuration
 *
 * This configuration defines default settings for audio player initialization:
 * - Event handler: NULL (no event handling)
 * - User arguments: NULL (no user data)
 * - task_stack: AUDIO_PLAYER_DEFAULT_TASK_STACK
 * - task_prio: BEKEN_DEFAULT_WORKER_PRIORITY
 */
#define DEFAULT_AUDIO_PLAYER_CONFIG() {             \
    .task_stack = AUDIO_PLAYER_DEFAULT_TASK_STACK,  \
    .task_prio = BEKEN_DEFAULT_WORKER_PRIORITY,     \
    .event_handler = NULL,                          \
    .args = NULL,                                   \
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
