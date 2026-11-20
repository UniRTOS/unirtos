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


#ifndef _M3U8_H_
#define _M3U8_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/bk_audio/audio_pipeline/bsd_queue.h>


#define M3U8_ITEM_SZ            (128)
#define M3U8_CODECS_SZ          (20)


typedef enum
{
    STREAM_TYPE_UNKNOW = 0,
    STREAM_TYPE_LIVE,           /*!< Live Stream */
    STREAM_TYPE_VOD,            /*!< Video on Demand */
} stream_type_t;

typedef enum
{
    M3U8_TYPE_UNKNOW = 0,
    M3U8_TYPE_TOP_M3U8,         /*!< Top level m3u8 file */
    M3U8_TYPE_SECOND_M3U8,      /*!< Second level m3u8 file */
    M3U8_TYPE_M3U,              /*!< The m3u file */
} m3u8_type_t;

typedef enum
{
    CODEC_TYPE_UNKNOW = 0,
    CODEC_TYPE_AAC,             /*!< AAC codec */
    CODEC_TYPE_TS,              /*!< TS codec */
} codec_type_t;

typedef struct url_format
{
    int sequence;               /*!< m3u8 url sequence in Live Stream mode. */
    double duration;               /*!< the time duration of this m3u8 url */
} url_format_t;

struct url_info
{
    int sequence;               /*!< m3u8 url sequence in Live Stream mode. */
    double duration;               /*!< the time duration of this m3u8 url */
    char item[M3U8_ITEM_SZ];    /*!< m3u8 url used to play */
};

typedef struct url_info *url_info_t;

typedef struct m3u8_url_info
{
    STAILQ_ENTRY(m3u8_url_info)    next;
    url_info_t                     url_item;
} m3u8_url_info_t;

typedef STAILQ_HEAD(m3u8_url_list, m3u8_url_info) m3u8_url_list_t;


typedef struct second_m3u8_format
{
    char                             *second_m3u8_url;     /*!< second m3u8 file url */
    uint32_t                          program_id;          /*!< PROGRAM-ID */
    uint32_t                          bandwidth;           /*!< band width of this m3u8 url */
    char                             *codecs;              /*!< audio codec type, mp4a.40.2: aac-lc */
} second_m3u8_format_t;

typedef struct second_m3u8_info
{
    STAILQ_ENTRY(second_m3u8_info)    next;

    second_m3u8_format_t              format;

    stream_type_t                     stream_type;
    codec_type_t                      codec_type;
    int                               version;
    int                               duration;             /*!< the max time duration of m3u8 url in Live Stream mode. Each m3u8 url time duration is less than the max time duration */
    int                               sequence;             /*!< current sequence in Live Stream mode */
    int                               latest_sequence;      /*!< the latest sequence of m3u8 url in Live Stream mode */
    int                               url_num;              /*!< the total number of m3u8 url list in Live Stream mode. The number can be used to calculate update time duration */
    m3u8_url_list_t                   m3u8_url_list;
} second_m3u8_info_t;

typedef STAILQ_HEAD(second_m3u8_info_list, second_m3u8_info) second_m3u8_info_list_t;

typedef struct top_m3u8_info
{
    char                             *top_m3u8_url;         /*!< m3u8 url */
    m3u8_type_t                       m3u8_type;            /*!< top m3u8 file type (top_m3u8_url) */
    m3u8_type_t                       current_m3u8_type;    /*!< current parse m3u8 file type */

    int                               version;

    beken_mutex_t                     lock;                 /*!< protect data when pop or push list */

    second_m3u8_format_t              current_use_m3u8;     /*!< second m3u8 format current used */

    uint8_t                           m3u8_num;             /*!< the total number of second m3u8 list */
    second_m3u8_info_list_t           m3u8_list;
} top_m3u8_info_t;


typedef struct
{
    void *(* malloc_memory)(uint32_t size);
    void (* free_memory)(void *ptr);
    int (* init_mutex)(beken_mutex_t *mutex);
    int (* deinit_mutex)(beken_mutex_t *mutex);
    int (* lock_mutex)(beken_mutex_t *mutex);
    int (* unlock_mutex)(beken_mutex_t *mutex);
    int (* read)(void *buffer, uint32_t length);        /*!< read data needed to parse */
} m3u8_osi_funcs_t;


/**
 * @brief   Malloc memory in player
 *
 * @param[in]  size   memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
m3u8_osi_funcs_t *m3u8_get_osi_funcs(void);

int m3u8_osi_funcs_init(void *config);

int m3u8_osi_funcs_deinit(void);

int m3u8_list_init(top_m3u8_info_t *top_m3u8, char *top_m3u8_url);

int m3u8_list_pop(top_m3u8_info_t *top_m3u8, second_m3u8_info_t **second_m3u8_info);

int m3u8_list_get_second_m3u8_num(top_m3u8_info_t *top_m3u8);

int m3u8_list_get_second_m3u8_item_by_id(top_m3u8_info_t *top_m3u8, uint8_t id, second_m3u8_info_t **second_m3u8_info);

int m3u8_list_get_second_m3u8_item_by_format(top_m3u8_info_t *top_m3u8, second_m3u8_format_t *second_m3u8_format, second_m3u8_info_t **second_m3u8_info);

int _get_current_use_second_m3u8_duration(top_m3u8_info_t *top_m3u8);

int _get_current_use_second_m3u8_duration_and_url_num(top_m3u8_info_t *top_m3u8, int *duration, int *number);

double _get_current_use_second_m3u8_total_time_duration(top_m3u8_info_t *top_m3u8);

m3u8_type_t get_m3u8_type(top_m3u8_info_t *top_m3u8);

char *get_used_complete_second_m3u8_url(top_m3u8_info_t *top_m3u8);

int free_url_obtained_from_m3u8(char *url);

int m3u8_list_set_use_second_m3u8_by_format(top_m3u8_info_t *top_m3u8, second_m3u8_format_t second_m3u8_format);

int m3u8_list_set_use_second_m3u8_by_id(top_m3u8_info_t *top_m3u8, uint8_t id);

int m3u8_url_list_pop(top_m3u8_info_t *top_m3u8, m3u8_url_info_t **item);

char *get_used_complete_m3u8_url(top_m3u8_info_t *top_m3u8);

bool m3u8_url_list_is_empty(top_m3u8_info_t *top_m3u8);

void m3u8_url_list_free_item(m3u8_url_info_t *item);

int m3u8_list_deinit(top_m3u8_info_t *top_m3u8);

int m3u8_parse(top_m3u8_info_t *top_m3u8);

void _url_printf(char *tag, char *url);

int debug_m3u8_lists(top_m3u8_info_t *top_m3u8);

#ifdef __cplusplus
}
#endif

#endif /*_M3U8_H_*/
