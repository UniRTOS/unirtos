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


#ifndef _TS_FORMAT_H_
#define _TS_FORMAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>


#define TS_PACKET_SIZE                  (188)
#define TS_BASE_PACKET_HEADER_SIZE      (4)

#define TS_BUFFER_SIZE                  ((768 * 2) * 2)

/* use list to save es_info and program_id_map */
//TODO
#define ESINFO_MAX  4
#define PROGRAM_MAX     4


typedef struct
{
    void *(* malloc_memory)(uint32_t size);
    void (* free_memory)(void *ptr);
    int (* read)(void *buffer, uint32_t length);        /*!< read data needed to parse */
} ts_format_osi_funcs_t;

/**
 * ts stream
 */
typedef struct ts_stream
{
    uint8_t ts_packet[TS_PACKET_SIZE];
    int pmt_PID;
    int pes_PID;
    int ts_buffer_size;
    uint8_t ts_buffer[TS_BUFFER_SIZE];
} ts_stream_t;


/**
 * @brief   Malloc memory in player
 *
 * @param[in]  size   memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
int ts_format_osi_funcs_init(void *config);

int ts_format_osi_funcs_deinit(void);

int ts_format_stream_init(ts_stream_t *ts_stream);

int ts_format_stream_deinit(ts_stream_t *ts_stream);

int ts_format_stream_read_aac_data(ts_stream_t *ts_stream, void *buffer, int size);

#ifdef __cplusplus
}
#endif

#endif /*_TS_FORMAT_H_*/
