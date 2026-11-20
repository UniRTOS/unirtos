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

#include <os/mem.h>
#include <modules/aacdec.h>
#include <stdbool.h>
#include <stdint.h>
#include "codec_api.h"
#include "source_api.h"
#include "player_osal.h"
#include "ring_buffer.h"
#include <components/bk_audio_player/bk_audio_player_types.h>

#define M4A_AUDIO_BUF_SZ        (AAC_MAINBUF_SIZE)
#define M4A_READ_BUFFER_SIZE    (64 * 1024)
#define M4A_AAC_SAMPLES_PER_FRAME 1024
#define M4A_ADTS_HEADER_MIN_SIZE 7
#define M4A_SEEK_SEARCH_RANGE    (4 * 1024)

// MP4 atom type definitions
// Note: MP4 atom types are stored in big-endian format, so MKTAG must match big-endian
#define MKTAG(a,b,c,d) (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(c) << 8) | (uint32_t)(d))
#define ATOM_FTYP MKTAG('f','t','y','p')
#define ATOM_FREE MKTAG('f','r','e','e')
#define ATOM_MOOV MKTAG('m','o','o','v')
#define ATOM_MDAT MKTAG('m','d','a','t')
#define ATOM_MVHD MKTAG('m','v','h','d')
#define ATOM_TRAK MKTAG('t','r','a','k')
#define ATOM_MDIA MKTAG('m','d','i','a')
#define ATOM_MDHD MKTAG('m','d','h','d')
#define ATOM_HDLR MKTAG('h','d','l','r')
#define ATOM_MINF MKTAG('m','i','n','f')
#define ATOM_STBL MKTAG('s','t','b','l')
#define ATOM_STSD MKTAG('s','t','s','d')
#define ATOM_STTS MKTAG('s','t','t','s')
#define ATOM_STSC MKTAG('s','t','s','c')
#define ATOM_STSZ MKTAG('s','t','s','z')
#define ATOM_STCO MKTAG('s','t','c','o')
#define ATOM_CO64 MKTAG('c','o','6','4')
#define ATOM_MP4A MKTAG('m','p','4','a')
#define ATOM_ESDS MKTAG('e','s','d','s')
#define ATOM_WAVE MKTAG('w','a','v','e')
#define ATOM_LPCM MKTAG('l','p','c','m')
#define ATOM_TWOS MKTAG('t','w','o','s')
#define ATOM_SOWT MKTAG('s','o','w','t')

// M4A decoder private data
typedef struct m4a_decoder_priv
{
    HAACDecoder decoder;
    AACFrameInfo aacFrameInfo;

    // MP4 structure info
    uint64_t moov_offset;
    uint64_t moov_size;
    uint64_t mdat_offset;
    uint64_t mdat_size;

    // Audio track info
    uint32_t audio_track_id;
    uint32_t time_scale;
    uint64_t duration;
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t sample_bits;

    // Sample table info
    uint32_t sample_count;
    uint32_t default_sample_size;
    uint32_t *sample_sizes;
    uint64_t *chunk_offsets;
    uint32_t chunk_count;
    uint32_t *samples_per_chunk;
    uint32_t *chunk_first_sample;

    // Sample timing (stts)
    uint32_t stts_entry_count;
    uint32_t *stts_sample_counts;
    uint32_t *stts_sample_deltas;

    // Sample-to-chunk (stsc)
    uint32_t stsc_entry_count;
    uint32_t *stsc_first_chunk;
    uint32_t *stsc_samples_per_chunk;

    // Current read position
    uint32_t current_sample;
    uint64_t current_offset;
    uint32_t bytes_left_in_sample;

    // Read buffer
    uint8_t *read_buffer;
    uint32_t read_buffer_size;
    uint32_t read_buffer_pos;

    // AAC decoder state
    uint64_t stream_offset;
    uint8_t *aac_read_buffer;
    uint8_t *aac_read_ptr;
    uint32_t aac_bytes_left;
    bool     input_eof;
    bool     stream_aborted;

    uint16_t pcm_format;
    uint8_t is_pcm;
    uint32_t pcm_frame_bytes;
    uint64_t pcm_total_frames;
    uint64_t pcm_frames_read;

    // AAC config from ESDS (decoder specific info)
    uint8_t *aac_config;
    uint32_t aac_config_size;
} m4a_decoder_priv_t;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static const uint32_t kAacSampleRateTable[] =
{
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000, 7350
};

static const uint8_t kAacChannelCountTable[] =
{
    0,  /* 0: defined in program config */
    1,  /* 1: mono */
    2,  /* 2: stereo */
    3,  /* 3: 3 channels */
    4,  /* 4: 4 channels */
    5,  /* 5: 5 channels */
    6,  /* 6: 5.1 */
    8,  /* 7: 7.1 */
    0, 0, 0, 0, 0, 0, 0, 0  /* reserved */
};

typedef struct bit_reader_s
{
    const uint8_t *data;
    uint32_t bit_size;
    uint32_t bit_pos;
} bit_reader_t;

static int bit_reader_read(bit_reader_t *br, uint32_t num_bits, uint32_t *out_val)
{
    if (!br || !out_val || num_bits == 0 || num_bits > 32)
    {
        return -1;
    }

    if (br->bit_pos + num_bits > br->bit_size)
    {
        return -1;
    }

    uint32_t value = 0;
    for (uint32_t i = 0; i < num_bits; i++)
    {
        uint32_t bit_index = br->bit_pos + i;
        uint32_t byte_index = bit_index >> 3;
        uint32_t bit_in_byte = 7 - (bit_index & 0x07);
        value = (value << 1) | ((br->data[byte_index] >> bit_in_byte) & 0x01);
    }

    br->bit_pos += num_bits;
    *out_val = value;
    return 0;
}

static bool mp4_read_descriptor_length(const uint8_t *buf, uint32_t buf_size, uint32_t *offset, uint32_t *length)
{
    uint32_t len = 0;
    int count = 0;

    while (*offset < buf_size && count < 4)
    {
        uint8_t byte = buf[(*offset)++];
        len = (len << 7) | (byte & 0x7F);
        count++;

        if ((byte & 0x80) == 0)
        {
            *length = len;
            return true;
        }
    }

    return false;
}

static int parse_aac_audio_specific_config(const uint8_t *data, uint32_t size, m4a_decoder_priv_t *priv)
{
    if (!data || !priv || size == 0)
    {
        return -1;
    }

    bit_reader_t br = { data, size * 8, 0 };
    uint32_t audio_object_type;
    uint32_t sample_rate_index;
    uint32_t sample_rate = 0;
    uint32_t channel_config;

    if (bit_reader_read(&br, 5, &audio_object_type) != 0)
    {
        return -1;
    }

    if (bit_reader_read(&br, 4, &sample_rate_index) != 0)
    {
        return -1;
    }

    if (sample_rate_index == 0x0F)
    {
        if (bit_reader_read(&br, 24, &sample_rate) != 0)
        {
            return -1;
        }
    }
    else if (sample_rate_index < ARRAY_SIZE(kAacSampleRateTable))
    {
        sample_rate = kAacSampleRateTable[sample_rate_index];
    }
    else
    {
        return -1;
    }

    if (bit_reader_read(&br, 4, &channel_config) != 0)
    {
        return -1;
    }

    if (sample_rate > 0 && priv->sample_rate == 0)
    {
        priv->sample_rate = sample_rate;
    }

    if (channel_config < ARRAY_SIZE(kAacChannelCountTable))
    {
        uint8_t channel_count = kAacChannelCountTable[channel_config];
        if (channel_count > 0 && priv->channels == 0)
        {
            priv->channels = channel_count;
        }
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, ASC parsed - audio_object_type=%u, sample_rate=%u, channel_config=%u, %d\n",
              __func__, audio_object_type, sample_rate, channel_config, __LINE__);
    return 0;
}

static int parse_esds_payload(const uint8_t *buf, uint32_t size, m4a_decoder_priv_t *priv)
{
    if (!buf || !priv || size < 4)
    {
        return -1;
    }

    uint32_t offset = 0;
    uint8_t version = buf[offset++];
    (void)version;

    if (size < 4)
    {
        return -1;
    }

    offset += 3;  // flags occupy 3 bytes after version

    while (offset + 1 < size)
    {
        uint8_t tag = buf[offset++];
        uint32_t length = 0;

        if (!mp4_read_descriptor_length(buf, size, &offset, &length))
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid ESDS descriptor length, %d\n", __func__, __LINE__);
            return -1;
        }

        if (offset + length > size)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, truncated ESDS descriptor tag=0x%02x, len=%u, %d\n",
                      __func__, tag, length, __LINE__);
            return -1;
        }

        uint32_t tag_end = offset + length;

        switch (tag)
        {
            case 0x04:  // DecoderConfigDescriptor
            {
                if (length >= 13)
                {
                    uint8_t object_type = buf[offset];
                    if (object_type != 0x40)
                    {
                        BK_LOGW(AUDIO_PLAYER_TAG, "%s, ESDS object_type is 0x%02x (expected 0x40), %d\n",
                                  __func__, object_type, __LINE__);
                    }
                }
                break;
            }
            case 0x05:  // DecoderSpecificInfo (AudioSpecificConfig)
            {
                if (parse_aac_audio_specific_config(buf + offset, length, priv) != 0)
                {
                    BK_LOGW(AUDIO_PLAYER_TAG, "%s, failed to parse AudioSpecificConfig, len=%u, %d\n",
                              __func__, length, __LINE__);
                }
                break;
            }
            default:
                break;
        }

        offset = tag_end;
    }

    return 0;
}

// Helper functions to read big-endian values
static uint32_t read_be32(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) | buf[3];
}

static uint64_t read_be64(const uint8_t *buf)
{
    return ((uint64_t)read_be32(buf) << 32) | read_be32(buf + 4);
}

static uint16_t read_be16(const uint8_t *buf)
{
    return ((uint16_t)buf[0] << 8) | buf[1];
}

// MP4 atom structure
typedef struct mp4_atom_s
{
    uint32_t size;
    uint32_t type;
    uint64_t size64;  // for large atoms (size == 1)
    uint64_t offset;
} mp4_atom_t;

// Read atom header
static int read_atom_header(bk_audio_player_decoder_t *decoder, mp4_atom_t *atom)
{
    uint8_t header[8];
    int ret;

    ret = audio_source_read_data(decoder->source, (char *)header, 8);
    if (ret != 8)
    {
        return -1;
    }

    atom->size = read_be32(header);
    atom->type = read_be32(header + 4);
    atom->size64 = 0;

    if (atom->size == 1)
    {
        // Large size atom
        uint8_t size64_buf[8];
        ret = audio_source_read_data(decoder->source, (char *)size64_buf, 8);
        if (ret != 8)
        {
            return -1;
        }
        atom->size64 = read_be64(size64_buf);
    }
    else if (atom->size == 0)
    {
        // Atom extends to end of file
        atom->size64 = 0;  // Will be handled specially
    }
    else
    {
        atom->size64 = atom->size;
    }

    return 0;
}

// Skip atom data
static int skip_atom_data(bk_audio_player_decoder_t *decoder, mp4_atom_t *atom)
{
    // Handle special cases: size 0 means atom extends to end of file (not supported)
    if (atom->size == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, atom size is 0 (extends to end of file), not supported, %d\n",
                  __func__, __LINE__);
        return -1;
    }

    uint64_t size = (atom->size == 1) ? atom->size64 : atom->size;
    if (size > 8)
    {
        size -= 8;  // Subtract header size
        if (atom->size == 1)
        {
            size -= 8;  // Already read size64, subtract it
        }

        // For small atoms, use a small stack buffer; for large ones, allocate from heap
        // This avoids unnecessary heap allocations for common small atoms like ftyp
        uint8_t small_buf[64];
        uint8_t *skip_buf = NULL;
        int need_free = 0;

        if (size <= sizeof(small_buf))
        {
            skip_buf = small_buf;
        }
        else
        {
            // Allocate buffer from heap for large atoms
            skip_buf = player_malloc(512);
            if (!skip_buf)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, player_malloc(512) failed, size=%llu, %d\n",
                          __func__, (unsigned long long)size, __LINE__);
                return -1;
            }
            need_free = 1;
        }

        int ret = 0;
        uint64_t remaining = size;
        while (remaining > 0)
        {
            uint32_t to_skip = (remaining > 512) ? 512 : (uint32_t)remaining;
            int read_ret = audio_source_read_data(decoder->source, (char *)skip_buf, to_skip);
            if (read_ret != to_skip)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, read_data failed, expected=%u, got=%d, remaining=%llu, %d\n",
                          __func__, to_skip, read_ret, (unsigned long long)remaining, __LINE__);
                ret = -1;
                break;
            }
            remaining -= to_skip;
        }

        if (need_free && skip_buf)
        {
            player_free(skip_buf);
        }
        // Return 0 on success, -1 on failure (not the read return value)
        return ret;
    }
    return 0;
}

// Parse mvhd atom to get time scale and duration
static int parse_mvhd(bk_audio_player_decoder_t *decoder, mp4_atom_t *atom, m4a_decoder_priv_t *priv)
{
    uint8_t version;
    uint8_t buf[20];
    int ret;

    ret = audio_source_read_data(decoder->source, (char *)&version, 1);
    if (ret != 1)
    {
        return -1;
    }

    uint32_t flags_size = (version == 1) ? 28 : 16;
    ret = audio_source_read_data(decoder->source, (char *)buf, flags_size);
    if (ret != flags_size)
    {
        return -1;
    }

    if (version == 1)
    {
        priv->time_scale = read_be32(buf + 12);
        priv->duration = read_be64(buf + 16);
    }
    else
    {
        priv->time_scale = read_be32(buf + 4);
        priv->duration = read_be32(buf + 8);
    }

    // Skip remaining data
    uint64_t atom_size = (atom->size == 1) ? atom->size64 : atom->size;
    uint64_t remaining = atom_size - 8 - 1 - flags_size;
    if (atom->size == 1)
    {
        remaining -= 8;
    }

    if (remaining > 0)
    {
        // Allocate buffer from heap to avoid stack overflow in embedded systems
        uint8_t *skip_buf = player_malloc(512);
        if (!skip_buf)
        {
            return -1;
        }

        while (remaining > 0)
        {
            uint32_t to_skip = (remaining > 512) ? 512 : remaining;
            ret = audio_source_read_data(decoder->source, (char *)skip_buf, to_skip);
            if (ret != to_skip)
            {
                player_free(skip_buf);
                return -1;
            }
            remaining -= to_skip;
        }

        player_free(skip_buf);
    }

    return 0;
}

// Parse hdlr atom to check if it's an audio track
static int parse_hdlr(bk_audio_player_decoder_t *decoder, mp4_atom_t *atom, int *is_audio)
{
    uint8_t version;
    uint8_t buf[20];
    int ret;

    ret = audio_source_read_data(decoder->source, (char *)&version, 1);
    if (ret != 1)
    {
        return -1;
    }

    ret = audio_source_read_data(decoder->source, (char *)buf, 20);
    if (ret != 20)
    {
        return -1;
    }

    // Check handler type
    // HDLR structure: version(1) + flags(3) + component_type(4) + handler_type(4) + ...
    // handler_type is at offset 8 from start of data (after version + flags + component_type)

    // Log all bytes for debugging
    BK_LOGI(AUDIO_PLAYER_TAG, "%s, buf[0-19]: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x, %d\n",
              __func__, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
              buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15],
              buf[16], buf[17], buf[18], buf[19], __LINE__);

    // Try different offsets to find 'soun'
    // From log: handler_type bytes are 0x6f 0x75 0x6e 0x00 ('oun\0')
    // This suggests 's' is at buf[7], so handler_type starts at offset 7, not 8
    uint32_t handler_type_offset8 = read_be32(buf + 8);  // Standard offset
    uint32_t handler_type_offset7 = read_be32(buf + 7);  // Try offset -1 (in case of alignment issue)
    uint32_t expected_soun = MKTAG('s','o','u','n');

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, handler_type@offset8=0x%08x ('%c%c%c%c'), @offset7=0x%08x ('%c%c%c%c'), expected=0x%08x (soun), %d\n",
              __func__, handler_type_offset8,
              (buf[8] >= 32 && buf[8] < 127) ? buf[8] : '?',
              (buf[9] >= 32 && buf[9] < 127) ? buf[9] : '?',
              (buf[10] >= 32 && buf[10] < 127) ? buf[10] : '?',
              (buf[11] >= 32 && buf[11] < 127) ? buf[11] : '?',
              handler_type_offset7,
              (buf[7] >= 32 && buf[7] < 127) ? buf[7] : '?',
              (buf[8] >= 32 && buf[8] < 127) ? buf[8] : '?',
              (buf[9] >= 32 && buf[9] < 127) ? buf[9] : '?',
              (buf[10] >= 32 && buf[10] < 127) ? buf[10] : '?',
              expected_soun, __LINE__);

    uint32_t handler_type = handler_type_offset8;

    // Check if 's' is at buf[7] and we have 'soun' starting from there
    if (buf[7] == 's' && buf[8] == 'o' && buf[9] == 'u' && buf[10] == 'n')
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, found 'soun' pattern at offset 7-10, using offset 7, %d\n", __func__, __LINE__);
        handler_type = handler_type_offset7;
    }
    // If offset 7 matches but offset 8 doesn't, use offset 7
    else if (handler_type_offset7 == expected_soun && handler_type_offset8 != expected_soun)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, offset 7 matches 'soun', using offset 7, %d\n", __func__, __LINE__);
        handler_type = handler_type_offset7;
    }
    // If offset 8 matches, use it (standard case)
    else if (handler_type_offset8 == expected_soun)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, offset 8 matches 'soun', using standard offset 8, %d\n", __func__, __LINE__);
        handler_type = handler_type_offset8;
    }

    if (handler_type == expected_soun)
    {
        *is_audio = 1;
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, identified as AUDIO track (soun), %d\n", __func__, __LINE__);
    }
    else
    {
        *is_audio = 0;
        // Determine what type of track this is
        if (handler_type == MKTAG('v','i','d','e'))
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, identified as VIDEO track (vide), %d\n", __func__, __LINE__);
        }
        else if (handler_type == MKTAG('t','e','x','t'))
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, identified as TEXT track (text), %d\n", __func__, __LINE__);
        }
        else
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, identified as UNKNOWN track type (0x%08x), %d\n",
                      __func__, handler_type, __LINE__);
        }
    }

    // Skip remaining data
    uint64_t atom_size = (atom->size == 1) ? atom->size64 : atom->size;
    uint64_t remaining = atom_size - 8 - 1 - 20;
    if (atom->size == 1)
    {
        remaining -= 8;
    }

    if (remaining > 0)
    {
        // Allocate buffer from heap to avoid stack overflow in embedded systems
        uint8_t *skip_buf = player_malloc(512);
        if (!skip_buf)
        {
            return -1;
        }

        while (remaining > 0)
        {
            uint32_t to_skip = (remaining > 512) ? 512 : remaining;
            ret = audio_source_read_data(decoder->source, (char *)skip_buf, to_skip);
            if (ret != to_skip)
            {
                player_free(skip_buf);
                return -1;
            }
            remaining -= to_skip;
        }

        player_free(skip_buf);
    }

    return 0;
}

// Parse stsd atom to get audio codec info
static int parse_stsd(bk_audio_player_decoder_t *decoder, mp4_atom_t *atom, m4a_decoder_priv_t *priv)
{
    uint8_t version;
    int ret;

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, parsing stsd atom, %d\n", __func__, __LINE__);

    // STSD structure: version(1) + flags(3) + entry_count(4)
    ret = audio_source_read_data(decoder->source, (char *)&version, 1);
    if (ret != 1)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, read version failed, %d\n", __func__, __LINE__);
        return -1;
    }

    // Read flags (3 bytes) and entry_count (4 bytes) together
    uint8_t stsd_header[7];
    ret = audio_source_read_data(decoder->source, (char *)stsd_header, 7);
    if (ret != 7)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, read flags+entry_count failed, ret=%d, %d\n", __func__, ret, __LINE__);
        return -1;
    }

    // entry_count is at offset 3 (after flags)
    uint32_t entry_count = read_be32(stsd_header + 3);
    BK_LOGI(AUDIO_PLAYER_TAG, "%s, version=%u, flags=%02x%02x%02x, entry_count=%u, %d\n",
              __func__, version, stsd_header[0], stsd_header[1], stsd_header[2], entry_count, __LINE__);
    if (entry_count == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, entry_count is 0, %d\n", __func__, __LINE__);
        return -1;
    }

    // Read first entry
    mp4_atom_t entry_atom;
    ret = read_atom_header(decoder, &entry_atom);
    if (ret != 0)
    {
        return -1;
    }

    if (entry_atom.type == ATOM_MP4A)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, found mp4a entry, size=%u, %d\n", __func__, 
                  (unsigned int)entry_atom.size, __LINE__);

        // Read mp4a atom
        uint8_t reserved[6];
        uint16_t data_reference_index;
        ret = audio_source_read_data(decoder->source, (char *)reserved, 6);
        if (ret != 6)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, read reserved failed, %d\n", __func__, __LINE__);
            return -1;
        }
        ret = audio_source_read_data(decoder->source, (char *)&data_reference_index, 2);
        if (ret != 2)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, read data_reference_index failed, %d\n", __func__, __LINE__);
            return -1;
        }
        data_reference_index = read_be16((uint8_t *)&data_reference_index);

        // Read AudioSampleEntry fields (20 bytes total)
        // Structure after reserved(6) + data_reference_index(2):
        // - reserved (8 bytes) for SoundSampleEntry
        // - channel_count (2 bytes) at offset 8
        // - sample_size (2 bytes) at offset 10
        // - compression_id (2 bytes) at offset 12
        // - packet_size (2 bytes) at offset 14
        // - sample_rate (4 bytes, 16.16 fixed point) at offset 16
        uint8_t sample_desc[20];
        ret = audio_source_read_data(decoder->source, (char *)sample_desc, 20);
        if (ret != 20)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, read sample_desc failed, %d\n", __func__, __LINE__);
            return -1;
        }

        // Fix: channel_count is at offset 8-9, not 10-11
        // Offset 10-11 is sample_size field, not channel_count!
        priv->channels = read_be16(sample_desc + 8);
        priv->sample_bits = read_be16(sample_desc + 10);

        // Read sample_rate from 16.16 fixed point format (optional, we get it from ESDS)
        // Only use it if ESDS doesn't provide sample rate
        uint32_t sample_rate_fixed = read_be32(sample_desc + 16);
        if (priv->sample_rate == 0 && sample_rate_fixed > 0)
        {
            priv->sample_rate = sample_rate_fixed / 65536;
        }

        BK_LOGI(AUDIO_PLAYER_TAG, "%s, parsed AudioSampleEntry - channels=%u, sample_bits=%u, sample_rate_fixed=%u, sample_rate=%u, %d\n",
                  __func__, priv->channels, priv->sample_bits, sample_rate_fixed, priv->sample_rate, __LINE__);

        // Parse child atoms (esds, wave, etc.)
        // Remaining = entry_size - atom_header(8) - reserved(6) - data_ref(2) - AudioSampleEntry(20)
        uint64_t remaining = entry_atom.size64 - 8 - 6 - 2 - 20;
        if (entry_atom.size == 1)
        {
            remaining -= 8;
        }

        while (remaining > 8)
        {
            mp4_atom_t child_atom;
            ret = read_atom_header(decoder, &child_atom);
            if (ret != 0)
            {
                break;
            }

            uint64_t child_size = (child_atom.size == 1) ? child_atom.size64 : child_atom.size;
            if (child_atom.size == 1)
            {
                child_size -= 8;
            }
            child_size -= 8;

            if (child_atom.type == ATOM_ESDS)
            {
                // Parse ESDS to get sample rate and codec config
                uint32_t esds_buf_len = (child_size > 2048) ? 2048 : (uint32_t)child_size;
                uint8_t *esds_buf = player_malloc(esds_buf_len);
                if (!esds_buf)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, ESDS buffer alloc %u bytes failed, %d\n",
                              __func__, esds_buf_len, __LINE__);
                    remaining -= (8 + child_size);
                    continue;
                }

                ret = audio_source_read_data(decoder->source, (char *)esds_buf, esds_buf_len);

                BK_LOGI(AUDIO_PLAYER_TAG, "%s, ESDS read: ret=%d, requested=%u, child_size=%llu, %d\n",
                          __func__, ret, esds_buf_len, (unsigned long long)child_size, __LINE__);

                if (child_size > esds_buf_len)
                {
                    BK_LOGW(AUDIO_PLAYER_TAG, "%s, ESDS truncated to %u/%llu bytes for parsing, %d\n",
                              __func__, esds_buf_len, (unsigned long long)child_size, __LINE__);
                }

                if (ret == (int)esds_buf_len)
                {
                    if (parse_esds_payload(esds_buf, esds_buf_len, priv) != 0)
                    {
                        BK_LOGW(AUDIO_PLAYER_TAG, "%s, parse_esds_payload failed, %d\n", __func__, __LINE__);
                    }
                }
                else
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, ESDS read incomplete, ret=%d, need=%u, %d\n",
                              __func__, ret, esds_buf_len, __LINE__);
                }

                // Skip remaining ESDS data if we didn't read it all (reuse esds_buf as scratch)
                if (child_size > esds_buf_len)
                {
                    uint64_t esds_remaining = child_size - esds_buf_len;
                    while (esds_remaining > 0)
                    {
                        uint32_t to_skip = (esds_remaining > esds_buf_len) ? esds_buf_len : (uint32_t)esds_remaining;
                        ret = audio_source_read_data(decoder->source, (char *)esds_buf, to_skip);
                        if (ret != (int)to_skip)
                        {
                            break;
                        }
                        esds_remaining -= to_skip;
                    }
                }

                player_free(esds_buf);
                remaining -= (8 + child_size);
            }
            else
            {
                // Skip other child atoms
                ret = skip_atom_data(decoder, &child_atom);
                if (ret != 0)
                {
                    break;
                }
                remaining -= (8 + child_size);
            }
        }
    }

    return 0;
}

// Parse stts atom (time-to-sample)
static int parse_stts(bk_audio_player_decoder_t *decoder, mp4_atom_t *atom, m4a_decoder_priv_t *priv)
{
    uint8_t header[8];
    uint64_t atom_size = (atom->size == 1) ? atom->size64 : atom->size;
    uint64_t data_size = atom_size > 8 ? (atom_size - 8) : 0;

    if (data_size < 8)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid stts atom size=%llu, %d\n",
                  __func__, (unsigned long long)atom_size, __LINE__);
        return -1;
    }

    if (audio_source_read_data(decoder->source, (char *)header, 8) != 8)
    {
        return -1;
    }

    uint32_t entry_count = read_be32(header + 4);
    if (entry_count == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, stts entry_count=0, %d\n", __func__, __LINE__);
        return -1;
    }

    if (priv->stts_sample_counts)
    {
        player_free(priv->stts_sample_counts);
        priv->stts_sample_counts = NULL;
    }
    if (priv->stts_sample_deltas)
    {
        player_free(priv->stts_sample_deltas);
        priv->stts_sample_deltas = NULL;
    }

    priv->stts_sample_counts = player_malloc(entry_count * sizeof(uint32_t));
    priv->stts_sample_deltas = player_malloc(entry_count * sizeof(uint32_t));
    if (!priv->stts_sample_counts || !priv->stts_sample_deltas)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, alloc stts arrays failed, entry_count=%u, %d\n",
                  __func__, entry_count, __LINE__);
        goto fail;
    }

    for (uint32_t i = 0; i < entry_count; i++)
    {
        uint8_t entry_buf[8];
        if (audio_source_read_data(decoder->source, (char *)entry_buf, 8) != 8)
        {
            goto fail;
        }
        priv->stts_sample_counts[i] = read_be32(entry_buf);
        priv->stts_sample_deltas[i] = read_be32(entry_buf + 4);
        if (priv->stts_sample_counts[i] == 0 || priv->stts_sample_deltas[i] == 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid stts entry index=%u, count=%u, delta=%u, %d\n",
                      __func__, i, priv->stts_sample_counts[i], priv->stts_sample_deltas[i], __LINE__);
            goto fail;
        }
    }

    priv->stts_entry_count = entry_count;

    // Skip any remaining data (should be zero)
    uint64_t consumed = 8 + (uint64_t)entry_count * 8;
    if (consumed < data_size)
    {
        uint64_t remain = data_size - consumed;
        while (remain > 0)
        {
            uint8_t skip_buf[64];
            uint32_t chunk = (remain > sizeof(skip_buf)) ? sizeof(skip_buf) : (uint32_t)remain;
            if (audio_source_read_data(decoder->source, (char *)skip_buf, chunk) != (int)chunk)
            {
                return -1;
            }
            remain -= chunk;
        }
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, parsed stts entries=%u, %d\n", __func__, entry_count, __LINE__);
    return 0;

fail:
    if (priv->stts_sample_counts)
    {
        player_free(priv->stts_sample_counts);
        priv->stts_sample_counts = NULL;
    }
    if (priv->stts_sample_deltas)
    {
        player_free(priv->stts_sample_deltas);
        priv->stts_sample_deltas = NULL;
    }
    priv->stts_entry_count = 0;
    return -1;
}

// Parse stsc atom (sample-to-chunk)
static int parse_stsc(bk_audio_player_decoder_t *decoder, mp4_atom_t *atom, m4a_decoder_priv_t *priv)
{
    uint8_t header[8];
    uint64_t atom_size = (atom->size == 1) ? atom->size64 : atom->size;
    uint64_t data_size = atom_size > 8 ? (atom_size - 8) : 0;

    if (data_size < 8)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid stsc atom size=%llu, %d\n",
                  __func__, (unsigned long long)atom_size, __LINE__);
        return -1;
    }

    if (audio_source_read_data(decoder->source, (char *)header, 8) != 8)
    {
        return -1;
    }

    uint32_t entry_count = read_be32(header + 4);
    if (entry_count == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, stsc entry_count=0, %d\n", __func__, __LINE__);
        return -1;
    }

    if (priv->stsc_first_chunk)
    {
        player_free(priv->stsc_first_chunk);
        priv->stsc_first_chunk = NULL;
    }
    if (priv->stsc_samples_per_chunk)
    {
        player_free(priv->stsc_samples_per_chunk);
        priv->stsc_samples_per_chunk = NULL;
    }

    priv->stsc_first_chunk = player_malloc(entry_count * sizeof(uint32_t));
    priv->stsc_samples_per_chunk = player_malloc(entry_count * sizeof(uint32_t));
    if (!priv->stsc_first_chunk || !priv->stsc_samples_per_chunk)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, alloc stsc arrays failed, entry_count=%u, %d\n",
                  __func__, entry_count, __LINE__);
        goto fail;
    }

    for (uint32_t i = 0; i < entry_count; i++)
    {
        uint8_t entry_buf[12];
        if (audio_source_read_data(decoder->source, (char *)entry_buf, 12) != 12)
        {
            goto fail;
        }
        priv->stsc_first_chunk[i] = read_be32(entry_buf);
        priv->stsc_samples_per_chunk[i] = read_be32(entry_buf + 4);

        if (priv->stsc_first_chunk[i] == 0 || priv->stsc_samples_per_chunk[i] == 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid stsc entry index=%u, first_chunk=%u, samples_per_chunk=%u, %d\n",
                      __func__, i, priv->stsc_first_chunk[i], priv->stsc_samples_per_chunk[i], __LINE__);
            goto fail;
        }
    }

    priv->stsc_entry_count = entry_count;

    // Skip remaining data if any
    uint64_t consumed = 8 + (uint64_t)entry_count * 12;
    if (consumed < data_size)
    {
        uint64_t remain = data_size - consumed;
        while (remain > 0)
        {
            uint8_t skip_buf[64];
            uint32_t chunk = (remain > sizeof(skip_buf)) ? sizeof(skip_buf) : (uint32_t)remain;
            if (audio_source_read_data(decoder->source, (char *)skip_buf, chunk) != (int)chunk)
            {
                return -1;
            }
            remain -= chunk;
        }
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, parsed stsc entries=%u, %d\n", __func__, entry_count, __LINE__);
    return 0;

fail:
    if (priv->stsc_first_chunk)
    {
        player_free(priv->stsc_first_chunk);
        priv->stsc_first_chunk = NULL;
    }
    if (priv->stsc_samples_per_chunk)
    {
        player_free(priv->stsc_samples_per_chunk);
        priv->stsc_samples_per_chunk = NULL;
    }
    priv->stsc_entry_count = 0;
    return -1;
}

// Parse stsz atom (sample size table)
static int parse_stsz(bk_audio_player_decoder_t *decoder, mp4_atom_t *atom, m4a_decoder_priv_t *priv)
{
    uint8_t header[12];
    uint64_t atom_size = (atom->size == 1) ? atom->size64 : atom->size;
    uint64_t data_size = atom_size > 8 ? (atom_size - 8) : 0;

    if (data_size < 12)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid stsz atom size=%llu, %d\n",
                  __func__, (unsigned long long)atom_size, __LINE__);
        return -1;
    }

    if (audio_source_read_data(decoder->source, (char *)header, 12) != 12)
    {
        return -1;
    }

    uint32_t sample_size = read_be32(header + 4);
    uint32_t sample_count = read_be32(header + 8);
    if (sample_count == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, stsz sample_count=0, %d\n", __func__, __LINE__);
        return -1;
    }

    if (priv->sample_sizes)
    {
        player_free(priv->sample_sizes);
        priv->sample_sizes = NULL;
    }
    priv->sample_count = sample_count;
    priv->default_sample_size = sample_size;

    if (sample_size == 0)
    {
        uint64_t table_size = (uint64_t)sample_count * sizeof(uint32_t);
        priv->sample_sizes = player_malloc(table_size);
        if (!priv->sample_sizes)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, alloc sample_sizes failed, count=%u, %d\n",
                      __func__, sample_count, __LINE__);
            return -1;
        }

        for (uint32_t i = 0; i < sample_count; i++)
        {
            uint8_t size_buf[4];
            if (audio_source_read_data(decoder->source, (char *)size_buf, 4) != 4)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, read sample_sizes failed at index=%u, %d\n",
                          __func__, i, __LINE__);
                player_free(priv->sample_sizes);
                priv->sample_sizes = NULL;
                priv->sample_count = 0;
                return -1;
            }
            priv->sample_sizes[i] = read_be32(size_buf);
            if (priv->sample_sizes[i] == 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid sample size at index=%u, %d\n", __func__, i, __LINE__);
                player_free(priv->sample_sizes);
                priv->sample_sizes = NULL;
                priv->sample_count = 0;
                return -1;
            }
        }
    }
    else
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, fixed sample size=%u, sample_count=%u, %d\n",
                  __func__, sample_size, sample_count, __LINE__);
    }

    // Skip remaining bytes if any
    uint64_t consumed = 12;
    if (sample_size == 0)
    {
        consumed += (uint64_t)sample_count * 4;
    }
    if (consumed < data_size)
    {
        uint64_t remain = data_size - consumed;
        while (remain > 0)
        {
            uint8_t skip_buf[64];
            uint32_t chunk = (remain > sizeof(skip_buf)) ? sizeof(skip_buf) : (uint32_t)remain;
            if (audio_source_read_data(decoder->source, (char *)skip_buf, chunk) != (int)chunk)
            {
                return -1;
            }
            remain -= chunk;
        }
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, parsed stsz sample_count=%u, default_size=%u, %d\n",
              __func__, priv->sample_count, priv->default_sample_size, __LINE__);
    return 0;
}

// Parse stco atom (chunk offsets 32-bit)
static int parse_stco(bk_audio_player_decoder_t *decoder, mp4_atom_t *atom, m4a_decoder_priv_t *priv)
{
    uint8_t header[8];
    uint64_t atom_size = (atom->size == 1) ? atom->size64 : atom->size;
    uint64_t data_size = atom_size > 8 ? (atom_size - 8) : 0;

    if (data_size < 8)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid stco atom size=%llu, %d\n",
                  __func__, (unsigned long long)atom_size, __LINE__);
        return -1;
    }

    if (audio_source_read_data(decoder->source, (char *)header, 8) != 8)
    {
        return -1;
    }

    uint32_t entry_count = read_be32(header + 4);
    if (entry_count == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, stco entry_count=0, %d\n", __func__, __LINE__);
        return -1;
    }

    if (priv->chunk_offsets)
    {
        player_free(priv->chunk_offsets);
        priv->chunk_offsets = NULL;
    }

    priv->chunk_offsets = player_malloc((uint64_t)entry_count * sizeof(uint64_t));
    if (!priv->chunk_offsets)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, alloc chunk_offsets failed, entry_count=%u, %d\n",
                  __func__, entry_count, __LINE__);
        return -1;
    }

    for (uint32_t i = 0; i < entry_count; i++)
    {
        uint8_t offset_buf[4];
        if (audio_source_read_data(decoder->source, (char *)offset_buf, 4) != 4)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, read stco entry fail idx=%u, %d\n", __func__, i, __LINE__);
            player_free(priv->chunk_offsets);
            priv->chunk_offsets = NULL;
            return -1;
        }
        priv->chunk_offsets[i] = read_be32(offset_buf);
    }

    priv->chunk_count = entry_count;

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, parsed stco chunk_count=%u, %d\n", __func__, entry_count, __LINE__);
    return 0;
}

// Parse co64 atom (chunk offsets 64-bit)
static int parse_co64(bk_audio_player_decoder_t *decoder, mp4_atom_t *atom, m4a_decoder_priv_t *priv)
{
    uint8_t header[8];
    uint64_t atom_size = (atom->size == 1) ? atom->size64 : atom->size;
    uint64_t data_size = atom_size > 8 ? (atom_size - 8) : 0;

    if (data_size < 8)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid co64 atom size=%llu, %d\n",
                  __func__, (unsigned long long)atom_size, __LINE__);
        return -1;
    }

    if (audio_source_read_data(decoder->source, (char *)header, 8) != 8)
    {
        return -1;
    }

    uint32_t entry_count = read_be32(header + 4);
    if (entry_count == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, co64 entry_count=0, %d\n", __func__, __LINE__);
        return -1;
    }

    if (priv->chunk_offsets)
    {
        player_free(priv->chunk_offsets);
        priv->chunk_offsets = NULL;
    }

    priv->chunk_offsets = player_malloc((uint64_t)entry_count * sizeof(uint64_t));
    if (!priv->chunk_offsets)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, alloc chunk_offsets(co64) failed, entry_count=%u, %d\n",
                  __func__, entry_count, __LINE__);
        return -1;
    }

    for (uint32_t i = 0; i < entry_count; i++)
    {
        uint8_t offset_buf[8];
        if (audio_source_read_data(decoder->source, (char *)offset_buf, 8) != 8)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, read co64 entry fail idx=%u, %d\n", __func__, i, __LINE__);
            player_free(priv->chunk_offsets);
            priv->chunk_offsets = NULL;
            return -1;
        }
        priv->chunk_offsets[i] = read_be64(offset_buf);
    }

    priv->chunk_count = entry_count;

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, parsed co64 chunk_count=%u, %d\n", __func__, entry_count, __LINE__);
    return 0;
}

// Build per-chunk sample mapping from stsc + chunk offsets
static int build_chunk_map(m4a_decoder_priv_t *priv)
{
    if (!priv->chunk_offsets || priv->chunk_count == 0 ||
        !priv->stsc_first_chunk || !priv->stsc_samples_per_chunk || priv->stsc_entry_count == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, chunk map prerequisites missing (chunk_count=%u, stsc_entry_count=%u), %d\n",
                  __func__, priv->chunk_count, priv->stsc_entry_count, __LINE__);
        return -1;
    }

    if (priv->samples_per_chunk)
    {
        player_free(priv->samples_per_chunk);
        priv->samples_per_chunk = NULL;
    }
    if (priv->chunk_first_sample)
    {
        player_free(priv->chunk_first_sample);
        priv->chunk_first_sample = NULL;
    }

    priv->samples_per_chunk = player_malloc((uint64_t)priv->chunk_count * sizeof(uint32_t));
    priv->chunk_first_sample = player_malloc((uint64_t)priv->chunk_count * sizeof(uint32_t));
    if (!priv->samples_per_chunk || !priv->chunk_first_sample)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, alloc chunk map arrays failed, chunk_count=%u, %d\n",
                  __func__, priv->chunk_count, __LINE__);
        goto fail;
    }

    uint64_t current_sample = 0;
    for (uint32_t chunk = 0; chunk < priv->chunk_count; chunk++)
    {
        priv->samples_per_chunk[chunk] = 0;
        priv->chunk_first_sample[chunk] = 0;
    }

    for (uint32_t i = 0; i < priv->stsc_entry_count; i++)
    {
        uint32_t first_chunk = priv->stsc_first_chunk[i];
        uint32_t samples_per_chunk = priv->stsc_samples_per_chunk[i];
        if (first_chunk == 0 || samples_per_chunk == 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid stsc entry i=%u, first_chunk=%u, samples=%u, %d\n",
                      __func__, i, first_chunk, samples_per_chunk, __LINE__);
            goto fail;
        }

        uint32_t last_chunk_exclusive = priv->chunk_count;
        if (i + 1 < priv->stsc_entry_count)
        {
            if (priv->stsc_first_chunk[i + 1] > 0)
            {
                last_chunk_exclusive = priv->stsc_first_chunk[i + 1] - 1;
            }
        }

        uint32_t start_index = (first_chunk > 0) ? (first_chunk - 1) : 0;
        if (start_index >= priv->chunk_count)
        {
            break;
        }
        if (last_chunk_exclusive > priv->chunk_count)
        {
            last_chunk_exclusive = priv->chunk_count;
        }

        for (uint32_t chunk = start_index; chunk < last_chunk_exclusive; chunk++)
        {
            priv->samples_per_chunk[chunk] = samples_per_chunk;
            priv->chunk_first_sample[chunk] = (uint32_t)current_sample;
            current_sample += samples_per_chunk;
        }
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, built chunk map, chunk_count=%u, total_samples=%llu, %d\n",
              __func__, priv->chunk_count, (unsigned long long)current_sample, __LINE__);
    return 0;

fail:
    if (priv->samples_per_chunk)
    {
        player_free(priv->samples_per_chunk);
        priv->samples_per_chunk = NULL;
    }
    if (priv->chunk_first_sample)
    {
        player_free(priv->chunk_first_sample);
        priv->chunk_first_sample = NULL;
    }
    return -1;
}

static uint32_t get_sample_size(const m4a_decoder_priv_t *priv, uint32_t sample_index)
{
    if (!priv || sample_index >= priv->sample_count)
    {
        return 0;
    }

    if (priv->sample_sizes)
    {
        return priv->sample_sizes[sample_index];
    }
    return priv->default_sample_size;
}

static uint32_t time_to_sample_index(const m4a_decoder_priv_t *priv, double target_time_sec)
{
    if (!priv)
    {
        return 0;
    }

    if (priv->stts_entry_count == 0 || !priv->stts_sample_counts || !priv->stts_sample_deltas || priv->time_scale == 0)
    {
        if (priv->sample_rate == 0)
        {
            return 0;
        }
        double frames = target_time_sec * (double)priv->sample_rate / (double)M4A_AAC_SAMPLES_PER_FRAME;
        if (frames < 0.0)
        {
            frames = 0.0;
        }
        uint64_t sample_index = (uint64_t)frames;
        if (sample_index >= priv->sample_count)
        {
            sample_index = (priv->sample_count > 0) ? (priv->sample_count - 1) : 0;
        }
        return (uint32_t)sample_index;
    }

    uint64_t target_units = (uint64_t)(target_time_sec * (double)priv->time_scale);
    uint64_t accumulated_units = 0;
    uint64_t accumulated_samples = 0;

    for (uint32_t i = 0; i < priv->stts_entry_count; i++)
    {
        uint32_t count = priv->stts_sample_counts[i];
        uint32_t delta = priv->stts_sample_deltas[i];
        uint64_t segment_units = (uint64_t)count * (uint64_t)delta;

        if (target_units < accumulated_units + segment_units)
        {
            uint64_t remaining = target_units - accumulated_units;
            uint32_t samples_in_segment = (delta > 0) ? (uint32_t)(remaining / delta) : 0;
            if (samples_in_segment >= count)
            {
                samples_in_segment = (count > 0) ? (count - 1) : 0;
            }
            uint64_t result = accumulated_samples + samples_in_segment;
            if (result >= priv->sample_count)
            {
                result = (priv->sample_count > 0) ? (priv->sample_count - 1) : 0;
            }
            return (uint32_t)result;
        }

        accumulated_units += segment_units;
        accumulated_samples += count;
    }

    if (priv->sample_count == 0)
    {
        return 0;
    }
    return priv->sample_count - 1;
}

static uint64_t sample_to_file_offset(const m4a_decoder_priv_t *priv, uint32_t sample_index)
{
    if (!priv || !priv->chunk_offsets || !priv->samples_per_chunk || !priv->chunk_first_sample)
    {
        return 0;
    }

    for (uint32_t chunk = 0; chunk < priv->chunk_count; chunk++)
    {
        uint32_t first_sample = priv->chunk_first_sample[chunk];
        uint32_t samples_in_chunk = priv->samples_per_chunk[chunk];
        if (samples_in_chunk == 0)
        {
            continue;
        }

        if (sample_index >= first_sample && sample_index < first_sample + samples_in_chunk)
        {
            uint32_t offset_in_chunk = sample_index - first_sample;
            uint64_t file_offset = priv->chunk_offsets[chunk];
            for (uint32_t i = 0; i < offset_in_chunk; i++)
            {
                uint32_t size = get_sample_size(priv, first_sample + i);
                if (size == 0)
                {
                    return 0;
                }
                file_offset += size;
            }
            return file_offset;
        }
    }

    return 0;
}

static int adts_sample_rate_index(uint32_t sample_rate)
{
    static const uint32_t sample_rates[] = {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000, 7350
    };
    for (int i = 0; i < (int)(sizeof(sample_rates) / sizeof(sample_rates[0])); i++)
    {
        if (sample_rates[i] == sample_rate)
        {
            return i;
        }
    }
    return -1;
}

static int is_valid_adts_header(const uint8_t *buf, uint32_t expected_sample_rate)
{
    if (!buf)
    {
        return 0;
    }

    if (buf[0] != 0xFF || (buf[1] & 0xF0) != 0xF0)
    {
        return 0;
    }

    uint8_t layer = (buf[1] >> 1) & 0x03;
    if (layer != 0)
    {
        return 0;
    }

    uint8_t protection_absent = buf[1] & 0x01;
    uint8_t sample_rate_idx = (buf[2] >> 2) & 0x0F;
    int expected_idx = adts_sample_rate_index(expected_sample_rate);
    if (expected_idx >= 0 && sample_rate_idx != (uint8_t)expected_idx)
    {
        return 0;
    }

    uint16_t frame_length = ((buf[3] & 0x03) << 11) | (buf[4] << 3) | ((buf[5] >> 5) & 0x07);
    uint16_t header_size = protection_absent ? 7 : 9;
    if (frame_length < header_size || frame_length > 8192)
    {
        return 0;
    }

    return 1;
}

static uint64_t align_offset_to_adts(bk_audio_player_decoder_t *decoder, m4a_decoder_priv_t *priv, uint64_t target_offset)
{
    if (!priv || !priv->chunk_offsets)
    {
        return target_offset;
    }

    uint64_t total_bytes = audio_source_get_total_bytes(decoder->source);
    uint64_t mdat_start = priv->mdat_offset ? priv->mdat_offset : 0;
    uint64_t mdat_end = (priv->mdat_offset + priv->mdat_size > priv->mdat_offset) ?
                        (priv->mdat_offset + priv->mdat_size) : total_bytes;
    if (mdat_end == 0 || mdat_end > total_bytes)
    {
        mdat_end = total_bytes;
    }

    uint64_t search_start = (target_offset > M4A_SEEK_SEARCH_RANGE) ? (target_offset - M4A_SEEK_SEARCH_RANGE) : mdat_start;
    if (search_start < mdat_start)
    {
        search_start = mdat_start;
    }
    uint64_t search_end = target_offset + M4A_SEEK_SEARCH_RANGE;
    if (mdat_end > 0 && search_end > mdat_end)
    {
        search_end = mdat_end;
    }

    if (audio_source_seek(decoder->source, search_start, SEEK_SET) != 0)
    {
        return target_offset;
    }

    uint8_t buffer[512];
    size_t carry = 0;
    uint8_t carry_buf[6];
    uint64_t current_pos = search_start;
    uint64_t best_before = 0;
    uint64_t best_after = 0;

    while (current_pos < search_end)
    {
        size_t to_read = sizeof(buffer) - carry;
        if ((search_end - current_pos) < to_read)
        {
            to_read = (size_t)(search_end - current_pos);
        }

        int read_bytes = audio_source_read_data(decoder->source, (char *)(buffer + carry), to_read);
        if (read_bytes <= 0)
        {
            break;
        }

        size_t total = carry + (size_t)read_bytes;
        for (size_t i = 0; i + M4A_ADTS_HEADER_MIN_SIZE <= total; i++)
        {
            if (is_valid_adts_header(buffer + i, priv->sample_rate))
            {
                uint64_t header_offset = current_pos - carry + i;
                if (header_offset <= target_offset)
                {
                    best_before = header_offset;
                }
                else if (best_after == 0)
                {
                    best_after = header_offset;
                }
            }
        }

        if (total >= sizeof(carry_buf))
        {
            carry = sizeof(carry_buf);
            os_memcpy(carry_buf, buffer + total - carry, carry);
        }
        else
        {
            carry = total;
            os_memcpy(carry_buf, buffer, carry);
        }
        os_memcpy(buffer, carry_buf, carry);

        current_pos += read_bytes;
    }

    uint64_t aligned = (best_before > 0) ? best_before : ((best_after > 0) ? best_after : target_offset);
    if (aligned != target_offset)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, align offset from %llu to %llu, %d\n",
                  __func__, (unsigned long long)target_offset, (unsigned long long)aligned, __LINE__);
    }

    if (audio_source_seek(decoder->source, aligned, SEEK_SET) != 0)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, seek back to aligned offset %llu failed, %d\n",
                  __func__, (unsigned long long)aligned, __LINE__);
    }

    return aligned;
}

// Simplified MP4 parser - only parse essential atoms for audio
static int parse_mp4_structure(bk_audio_player_decoder_t *decoder, m4a_decoder_priv_t *priv)
{
    mp4_atom_t atom;
    int ret;
    int found_moov = 0;
    int found_mdat = 0;
    uint64_t current_offset = 0;

    // Reset to beginning
    if (audio_source_seek(decoder->source, 0, SEEK_SET) != 0)
    {
        return -1;
    }

    // Parse top-level atoms
    while (!found_moov || !found_mdat)
    {
        // Record current position before reading atom header
        uint64_t atom_start_offset = current_offset;

        ret = read_atom_header(decoder, &atom);
        if (ret != 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, read_atom_header failed, found_moov=%d, found_mdat=%d, offset=%llu, %d\n",
                      __func__, found_moov, found_mdat, (unsigned long long)current_offset, __LINE__);
            break;
        }

        current_offset += 8;  // atom header size
        if (atom.size == 1)
        {
            current_offset += 8;  // size64 field
        }

        BK_LOGI(AUDIO_PLAYER_TAG, "%s, found atom type=0x%08x, size=%u, size64=%llu, %d\n",
                  __func__, atom.type, (unsigned int)atom.size, 
                  (unsigned long long)atom.size64, __LINE__);

        if (atom.type == ATOM_FTYP)
        {
            // Skip ftyp
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, skipping ftyp atom, data_size=%llu, %d\n",
                      __func__, (unsigned long long)((atom.size == 1) ? atom.size64 : atom.size) - 8, __LINE__);
            ret = skip_atom_data(decoder, &atom);
            if (ret != 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, skip_atom_data(ftyp) failed, ret=%d, %d\n", 
                          __func__, ret, __LINE__);
                return -1;
            }
            current_offset += ((atom.size == 1) ? atom.size64 : atom.size) - 8;
            if (atom.size == 1)
            {
                current_offset -= 8;
            }
        }
        else if (atom.type == ATOM_FREE)
        {
            // Skip free atom (padding atom, may have size 8 with no data)
            ret = skip_atom_data(decoder, &atom);
            if (ret != 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, skip_atom_data(free) failed, %d\n", __func__, __LINE__);
                return -1;
            }
            current_offset += ((atom.size == 1) ? atom.size64 : atom.size) - 8;
            if (atom.size == 1)
            {
                current_offset -= 8;
            }
        }
        else if (atom.type == ATOM_MOOV)
        {
            found_moov = 1;
            priv->moov_size = (atom.size == 1) ? atom.size64 : atom.size;

            // Parse moov children
            uint64_t remaining = (atom.size == 1) ? atom.size64 : atom.size;
            remaining -= 8;
            if (atom.size == 1)
            {
                remaining -= 8;
            }

            while (remaining > 8)
            {
                mp4_atom_t child_atom;
                ret = read_atom_header(decoder, &child_atom);
                if (ret != 0)
                {
                    break;
                }

                uint64_t child_size = (child_atom.size == 1) ? child_atom.size64 : child_atom.size;
                if (child_atom.size == 1)
                {
                    child_size -= 8;
                }
                child_size -= 8;

                if (child_atom.type == ATOM_MVHD)
                {
                    ret = parse_mvhd(decoder, &child_atom, priv);
                    if (ret != 0)
                    {
                        return -1;
                    }
                }
                else if (child_atom.type == ATOM_TRAK)
                {
                    // Parse trak to find audio track
                    uint64_t trak_remaining = child_size;
                    int is_audio_track = 0;
                    int audio_track_found = 0;  // Flag to indicate if we found and parsed audio track

                    BK_LOGI(AUDIO_PLAYER_TAG, "%s, parsing TRAK atom, %d\n", __func__, __LINE__);

                    while (trak_remaining > 8)
                    {
                        mp4_atom_t trak_child;
                        ret = read_atom_header(decoder, &trak_child);
                        if (ret != 0)
                        {
                            break;
                        }

                        uint64_t trak_child_size = (trak_child.size == 1) ? trak_child.size64 : trak_child.size;
                        if (trak_child.size == 1)
                        {
                            trak_child_size -= 8;
                        }
                        trak_child_size -= 8;

                        if (trak_child.type == ATOM_MDIA)
                        {
                            // Parse mdia - parse atoms sequentially
                            // HDLR typically comes before MINF, so we can determine track type first
                            uint64_t mdia_remaining = trak_child_size;
                            int minf_parsed = 0;

                            BK_LOGI(AUDIO_PLAYER_TAG, "%s, found MDIA atom in TRAK, %d\n", __func__, __LINE__);

                            while (mdia_remaining > 8 && !minf_parsed)
                            {
                                mp4_atom_t mdia_child;
                                ret = read_atom_header(decoder, &mdia_child);
                                if (ret != 0)
                                {
                                    break;
                                }

                                uint64_t mdia_child_size = (mdia_child.size == 1) ? mdia_child.size64 : mdia_child.size;
                                if (mdia_child.size == 1)
                                {
                                    mdia_child_size -= 8;
                                }
                                mdia_child_size -= 8;

                                if (mdia_child.type == ATOM_HDLR)
                                {
                                    ret = parse_hdlr(decoder, &mdia_child, &is_audio_track);
                                    if (ret != 0)
                                    {
                                        break;
                                    }
                                    BK_LOGI(AUDIO_PLAYER_TAG, "%s, parse_hdlr result: is_audio_track=%d, %d\n",
                                              __func__, is_audio_track, __LINE__);
                                    mdia_remaining -= (8 + mdia_child_size);
                                }
                                else if (mdia_child.type == ATOM_MINF && is_audio_track)
                                {
                                    BK_LOGI(AUDIO_PLAYER_TAG, "%s, found MINF for audio track, parsing stsd, %d\n",
                                              __func__, __LINE__);
                                    // Parse minf -> stbl -> stsd
                                    uint64_t minf_remaining = mdia_child_size;
                                    while (minf_remaining > 8)
                                    {
                                        mp4_atom_t minf_child;
                                        ret = read_atom_header(decoder, &minf_child);
                                        if (ret != 0)
                                        {
                                            break;
                                        }

                                        uint64_t minf_child_size = (minf_child.size == 1) ? minf_child.size64 : minf_child.size;
                                        if (minf_child.size == 1)
                                        {
                                            minf_child_size -= 8;
                                        }
                                        minf_child_size -= 8;

                                        if (minf_child.type == ATOM_STBL)
                                        {
                                            // Parse stbl -> stsd
                                            uint64_t stbl_remaining = minf_child_size;
                                            while (stbl_remaining > 8)
                                            {
                                                mp4_atom_t stbl_child;
                                                ret = read_atom_header(decoder, &stbl_child);
                                                if (ret != 0)
                                                {
                                                    break;
                                                }

                                                uint64_t stbl_child_size = (stbl_child.size == 1) ? stbl_child.size64 : stbl_child.size;
                                                if (stbl_child.size == 1)
                                                {
                                                    stbl_child_size -= 8;
                                                }
                                                stbl_child_size -= 8;

                                                if (stbl_child.type == ATOM_STSD)
                                                {
                                                    BK_LOGI(AUDIO_PLAYER_TAG, "%s, found stsd atom, size=%u, %d\n",
                                                              __func__, (unsigned int)stbl_child.size, __LINE__);
                                                    ret = parse_stsd(decoder, &stbl_child, priv);
                                                    if (ret != 0)
                                                    {
                                                        BK_LOGE(AUDIO_PLAYER_TAG, "%s, parse_stsd failed, ret=%d, %d\n",
                                                                  __func__, ret, __LINE__);
                                                        return -1;
                                                    }
                                                    BK_LOGI(AUDIO_PLAYER_TAG, "%s, parse_stsd succeeded, channels=%u, sample_rate=%u, %d\n",
                                                              __func__, priv->channels, priv->sample_rate, __LINE__);
                                                    audio_track_found = 1;  // Mark that we found audio track
                                                    stbl_remaining -= (8 + stbl_child_size);
                                                }
                                                else if (stbl_child.type == ATOM_STTS)
                                                {
                                                    ret = parse_stts(decoder, &stbl_child, priv);
                                                    if (ret != 0)
                                                    {
                                                        BK_LOGE(AUDIO_PLAYER_TAG, "%s, parse_stts failed, %d\n", __func__, __LINE__);
                                                        return -1;
                                                    }
                                                    stbl_remaining -= (8 + stbl_child_size);
                                                }
                                                else if (stbl_child.type == ATOM_STSC)
                                                {
                                                    ret = parse_stsc(decoder, &stbl_child, priv);
                                                    if (ret != 0)
                                                    {
                                                        BK_LOGE(AUDIO_PLAYER_TAG, "%s, parse_stsc failed, %d\n", __func__, __LINE__);
                                                        return -1;
                                                    }
                                                    stbl_remaining -= (8 + stbl_child_size);
                                                }
                                                else if (stbl_child.type == ATOM_STSZ)
                                                {
                                                    ret = parse_stsz(decoder, &stbl_child, priv);
                                                    if (ret != 0)
                                                    {
                                                        BK_LOGE(AUDIO_PLAYER_TAG, "%s, parse_stsz failed, %d\n", __func__, __LINE__);
                                                        return -1;
                                                    }
                                                    stbl_remaining -= (8 + stbl_child_size);
                                                }
                                                else if (stbl_child.type == ATOM_STCO)
                                                {
                                                    ret = parse_stco(decoder, &stbl_child, priv);
                                                    if (ret != 0)
                                                    {
                                                        BK_LOGE(AUDIO_PLAYER_TAG, "%s, parse_stco failed, %d\n", __func__, __LINE__);
                                                        return -1;
                                                    }
                                                    stbl_remaining -= (8 + stbl_child_size);
                                                }
                                                else if (stbl_child.type == ATOM_CO64)
                                                {
                                                    ret = parse_co64(decoder, &stbl_child, priv);
                                                    if (ret != 0)
                                                    {
                                                        BK_LOGE(AUDIO_PLAYER_TAG, "%s, parse_co64 failed, %d\n", __func__, __LINE__);
                                                        return -1;
                                                    }
                                                    stbl_remaining -= (8 + stbl_child_size);
                                                }
                                                else
                                                {
                                                    ret = skip_atom_data(decoder, &stbl_child);
                                                    if (ret != 0)
                                                    {
                                                        break;
                                                    }
                                                    stbl_remaining -= (8 + stbl_child_size);
                                                }
                                            }
                                            minf_remaining -= (8 + minf_child_size);
                                            break;
                                        }
                                        else
                                        {
                                            ret = skip_atom_data(decoder, &minf_child);
                                            if (ret != 0)
                                            {
                                                break;
                                            }
                                            minf_remaining -= (8 + minf_child_size);
                                        }
                                    }
                                    minf_parsed = 1;
                                    mdia_remaining -= (8 + mdia_child_size);
                                }
                                else if (mdia_child.type == ATOM_MINF && !is_audio_track)
                                {
                                    // Skip MINF for non-audio tracks
                                    BK_LOGI(AUDIO_PLAYER_TAG, "%s, skipping MINF for non-audio track, %d\n",
                                              __func__, __LINE__);
                                    ret = skip_atom_data(decoder, &mdia_child);
                                    if (ret != 0)
                                    {
                                        break;
                                    }
                                    mdia_remaining -= (8 + mdia_child_size);
                                }
                                else
                                {
                                    // Skip other atoms (MDHD, etc.)
                                    ret = skip_atom_data(decoder, &mdia_child);
                                    if (ret != 0)
                                    {
                                        break;
                                    }
                                    mdia_remaining -= (8 + mdia_child_size);
                                }
                            }

                            // If this was an audio track and we parsed it, we can break
                            // Otherwise, continue to next trak child
                            if (audio_track_found)
                            {
                                BK_LOGI(AUDIO_PLAYER_TAG, "%s, audio track found and parsed, breaking, %d\n",
                                          __func__, __LINE__);
                                trak_remaining -= (8 + trak_child_size);
                                break;
                            }

                            trak_remaining -= (8 + trak_child_size);
                            // Continue to parse other trak children if not audio track
                        }
                        else
                        {
                            ret = skip_atom_data(decoder, &trak_child);
                            if (ret != 0)
                            {
                                break;
                            }
                            trak_remaining -= (8 + trak_child_size);
                        }
                    }

                    // If we found audio track in this TRAK, we can break from moov children loop
                    // But we need to continue parsing other moov children to complete the structure
                    if (audio_track_found)
                    {
                        BK_LOGI(AUDIO_PLAYER_TAG, "%s, audio track found in this TRAK, %d\n",
                                  __func__, __LINE__);
                    }
                }
                else
                {
                    ret = skip_atom_data(decoder, &child_atom);
                    if (ret != 0)
                    {
                        break;
                    }
                }

                remaining -= (8 + child_size);
            }
            // Update current_offset after parsing moov
            uint64_t moov_size = (atom.size == 1) ? atom.size64 : atom.size;
            current_offset = atom_start_offset + moov_size;
        }
        else if (atom.type == ATOM_MDAT)
        {
            found_mdat = 1;
            uint64_t mdat_size = (atom.size == 1) ? atom.size64 : atom.size;

            // Record mdat offset (current position is after atom header, which is where data starts)
            priv->mdat_offset = current_offset;

            // Calculate mdat data size (excluding header)
            priv->mdat_size = mdat_size - 8;
            if (atom.size == 1)
            {
                priv->mdat_size -= 8;
            }

            // Skip mdat data to continue parsing and find moov (if moov comes after mdat)
            // We'll seek back to mdat_offset later when we need to read audio data
            ret = skip_atom_data(decoder, &atom);
            if (ret != 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, skip_atom_data(mdat) failed, %d\n", __func__, __LINE__);
                return -1;
            }
            // Update current_offset: mdat starts at atom_start_offset, ends at atom_start_offset + mdat_size
            current_offset = atom_start_offset + mdat_size;
        }
        else
        {
            // Skip unknown atoms
            ret = skip_atom_data(decoder, &atom);
            if (ret != 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, skip_atom_data(unknown) failed, atom_type=0x%08x, %d\n",
                          __func__, atom.type, __LINE__);
                break;
            }
            current_offset += ((atom.size == 1) ? atom.size64 : atom.size) - 8;
            if (atom.size == 1)
            {
                current_offset -= 8;
            }
        }
    }

    if (!found_moov || !found_mdat)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, missing required atoms, found_moov=%d, found_mdat=%d, %d\n",
                  __func__, found_moov, found_mdat, __LINE__);
        return -1;
    }

    if (build_chunk_map(priv) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, build_chunk_map failed, sample tables incomplete, %d\n",
                  __func__, __LINE__);
        return -1;
    }

    if (priv->sample_count == 0 || (priv->default_sample_size == 0 && priv->sample_sizes == NULL))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid sample sizes, sample_count=%u, default_size=%u, %d\n",
                  __func__, priv->sample_count, priv->default_sample_size, __LINE__);
        return -1;
    }

    if (priv->chunk_count == 0 || !priv->chunk_offsets)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid chunk table, chunk_count=%u, %d\n",
                  __func__, priv->chunk_count, __LINE__);
        return -1;
    }

    return 0;
}

// Fill AAC decoder buffer
static int32_t m4a_fill_aac_buffer(bk_audio_player_decoder_t *decoder, m4a_decoder_priv_t *priv)
{
    int bytes_read;
    size_t bytes_to_read;
    uint8_t enforce_data_limit = 0;
    uint64_t mdat_end = 0;

    if (priv->aac_bytes_left > 0)
    {
        memmove(priv->aac_read_buffer, priv->aac_read_ptr, priv->aac_bytes_left);
    }
    priv->aac_read_ptr = priv->aac_read_buffer;

    bytes_to_read = M4A_AUDIO_BUF_SZ - priv->aac_bytes_left;

    if (priv->mdat_size > 0 && priv->mdat_offset <= (UINT64_MAX - priv->mdat_size))
    {
        enforce_data_limit = 1;
        mdat_end = priv->mdat_offset + priv->mdat_size;
    }

    if (enforce_data_limit)
    {
        if (priv->stream_offset < priv->mdat_offset)
        {
            priv->stream_offset = priv->mdat_offset;
        }

        if (priv->stream_offset >= mdat_end)
        {
            priv->input_eof = true;
            return 0;
        }

        uint64_t remaining = mdat_end - priv->stream_offset;
        if (remaining == 0)
        {
            priv->input_eof = true;
            return 0;
        }

        if (remaining < bytes_to_read)
        {
            bytes_to_read = (size_t)remaining;
        }
    }

    if (bytes_to_read == 0)
    {
        if (enforce_data_limit && priv->stream_offset >= mdat_end)
        {
            priv->input_eof = true;
        }
        return 0;
    }

    bytes_read = audio_source_read_data(decoder->source, (char *)(priv->aac_read_buffer + priv->aac_bytes_left), bytes_to_read);
    if (bytes_read > 0)
    {
        priv->input_eof = false;
        priv->stream_aborted = false;
        priv->aac_bytes_left = priv->aac_bytes_left + bytes_read;
        priv->stream_offset += (uint64_t)bytes_read;
        return 0;
    }
    if (bytes_read == AUDIO_PLAYER_TIMEOUT)
    {
        return AUDIO_PLAYER_TIMEOUT;
    }
    if (bytes_read == 0 || bytes_read == RB_DONE)
    {
        /* normal end of stream: tail data is complete and should be decoded */
        priv->input_eof = true;
        priv->stream_aborted = false;
        if (priv->aac_bytes_left == 0)
        {
            return RB_DONE;
        }
        return 0;
    }
    if (bytes_read == RB_ABORT)
    {
        /* aborted stream: residual tail may be a truncated frame and must be dropped */
        priv->input_eof = true;
        priv->stream_aborted = true;
        if (priv->aac_bytes_left == 0)
        {
            return RB_DONE;
        }
        return 0;
    }
    if (priv->aac_bytes_left != 0)
    {
        return 0;
    }

    BK_LOGW(AUDIO_PLAYER_TAG, "can't read more data, end of stream. left=%d \n", priv->aac_bytes_left);
    return -1;
}

static int calc_m4a_position(bk_audio_player_decoder_t *decoder, int second)
{
    if (!decoder || !decoder->decoder_priv)
    {
        return -1;
    }

    if (second < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid seek time=%d\n", __func__, second);
        return -1;
    }

    m4a_decoder_priv_t *priv = (m4a_decoder_priv_t *)decoder->decoder_priv;

    if (!priv->decoder)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, decoder not ready\n", __func__);
        return -1;
    }

    if (priv->sample_count == 0 || (!priv->sample_sizes && priv->default_sample_size == 0))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, sample table missing (sample_count=%u, default_sample_size=%u)\n",
                  __func__, priv->sample_count, priv->default_sample_size);
        return -1;
    }

    if (!priv->chunk_offsets || priv->chunk_count == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, chunk table missing\n", __func__);
        return -1;
    }

    if (!priv->samples_per_chunk || !priv->chunk_first_sample)
    {
        if (build_chunk_map(priv) != 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, chunk map unavailable\n", __func__);
            return -1;
        }
    }

    double duration_sec = 0.0;
    if (priv->time_scale > 0 && priv->duration > 0)
    {
        duration_sec = (double)priv->duration / (double)priv->time_scale;
    }

    double target_time_sec = (double)second;
    if (duration_sec > 0.0 && target_time_sec > duration_sec)
    {
        double frame_duration = 0.0;
        if (priv->sample_rate > 0)
        {
            frame_duration = (double)M4A_AAC_SAMPLES_PER_FRAME / (double)priv->sample_rate;
        }
        double last_frame_time = duration_sec - frame_duration;
        if (last_frame_time < 0.0)
        {
            last_frame_time = 0.0;
        }
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, clamp seek time from %.3f to %.3f (duration %.3f)\n",
                  __func__, target_time_sec, last_frame_time, duration_sec);
        target_time_sec = last_frame_time;
    }

    uint32_t target_sample = time_to_sample_index(priv, target_time_sec);
    if (target_sample >= priv->sample_count)
    {
        target_sample = (priv->sample_count > 0) ? (priv->sample_count - 1) : 0;
    }

    uint64_t raw_offset = sample_to_file_offset(priv, target_sample);
    if (raw_offset == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, failed to map sample %u to file offset\n", __func__, target_sample);
        return -1;
    }

    uint64_t aligned_offset = raw_offset;
    bool has_chunk_map = (priv->samples_per_chunk && priv->chunk_first_sample && priv->chunk_count > 0);
    if (!has_chunk_map)
    {
        aligned_offset = align_offset_to_adts(decoder, priv, raw_offset);
    }
    uint64_t total_bytes = audio_source_get_total_bytes(decoder->source);
    if (total_bytes > 0 && aligned_offset >= total_bytes)
    {
        aligned_offset = (total_bytes > 1) ? (total_bytes - 1) : 0;
    }

    priv->current_sample = target_sample;
    priv->current_offset = aligned_offset;
    priv->bytes_left_in_sample = 0;
    priv->aac_bytes_left = 0;
    priv->aac_read_ptr = priv->aac_read_buffer;
    priv->input_eof = false;
    priv->stream_aborted = false;
    AACFlushCodec(priv->decoder);

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, seek second=%d -> sample=%u, offset=%llu (raw=%llu)\n",
              __func__, second, target_sample,
              (unsigned long long)aligned_offset, (unsigned long long)raw_offset);

    if (audio_source_seek(decoder->source, aligned_offset, SEEK_SET) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, audio_source_seek to %llu failed\n",
                  __func__, (unsigned long long)aligned_offset);
        return -1;
    }
    priv->stream_offset = aligned_offset;

    return (int)((aligned_offset <= 0x7FFFFFFF) ? aligned_offset : 0x7FFFFFFF);
}

static int m4a_decoder_open(audio_format_t format, void *param, bk_audio_player_decoder_t **decoder_pp)
{
    bk_audio_player_decoder_t *decoder;
    m4a_decoder_priv_t *priv;

    if (format != AUDIO_FORMAT_M4A)
    {
        return AUDIO_PLAYER_INVALID;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    decoder = audio_codec_new(sizeof(m4a_decoder_priv_t));
    if (!decoder)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    priv = (m4a_decoder_priv_t *)decoder->decoder_priv;
    os_memset(priv, 0x0, sizeof(m4a_decoder_priv_t));

    // Initialize AAC decoder
    priv->decoder = AACInitDecoder();
    if (!priv->decoder)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "AAC decoder create failed!");
        player_free(decoder);
        return AUDIO_PLAYER_ERR;
    }

    // Allocate read buffers
    priv->aac_read_buffer = player_malloc(M4A_AUDIO_BUF_SZ);
    if (priv->aac_read_buffer == NULL)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "aac_read_buffer malloc failed!");
        AACFreeDecoder(priv->decoder);
        player_free(decoder);
        return AUDIO_PLAYER_ERR;
    }

    priv->aac_read_ptr = priv->aac_read_buffer;
    priv->aac_bytes_left = 0;
    priv->input_eof = false;
    priv->stream_aborted = false;
    priv->is_pcm = 0;
    priv->pcm_frame_bytes = 0;
    priv->pcm_total_frames = 0;
    priv->pcm_frames_read = 0;

    *decoder_pp = decoder;

    return AUDIO_PLAYER_OK;
}

static int m4a_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    int ec;
    m4a_decoder_priv_t *priv;
    priv = (m4a_decoder_priv_t *)decoder->decoder_priv;

    // Parse MP4 structure
    if (parse_mp4_structure(decoder, priv) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, parse_mp4_structure fail, %d\n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    // Seek to mdat data offset (where audio data starts)
    if (priv->mdat_offset > 0)
    {
        if (audio_source_seek(decoder->source, priv->mdat_offset, SEEK_SET) != 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, seek to mdat_offset=%llu failed, %d\n",
                      __func__, (unsigned long long)priv->mdat_offset, __LINE__);
            return AUDIO_PLAYER_ERR;
        }
        priv->stream_offset = priv->mdat_offset;
    }
    else
    {
        // Fallback: Reset to beginning and find mdat atom
        if (audio_source_seek(decoder->source, 0, SEEK_SET) != 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, seek to beginning fail, %d\n", __func__, __LINE__);
            return AUDIO_PLAYER_ERR;
        }

        // Find mdat atom again and skip to its data
        mp4_atom_t atom;
        while (1)
        {
            if (read_atom_header(decoder, &atom) != 0)
            {
                break;
            }
            if (atom.type == ATOM_MDAT)
            {
                // Skip mdat header, now we're at the data
                break;
            }
            else
            {
                skip_atom_data(decoder, &atom);
            }
        }
        if (priv->mdat_offset > 0)
        {
            priv->stream_offset = priv->mdat_offset;
        }
        else
        {
            priv->stream_offset = 0;
        }
    }

    priv->aac_read_ptr = priv->aac_read_buffer;
    priv->aac_bytes_left = 0;
    priv->input_eof = false;
    priv->stream_aborted = false;

    // Fill buffer and decode first frame to get AAC info
__retry:
    if (priv->aac_bytes_left < AAC_MAINBUF_SIZE)
    {
        int fill_ret = m4a_fill_aac_buffer(decoder, priv);
        if (fill_ret == AUDIO_PLAYER_TIMEOUT)
        {
            rtos_delay_milliseconds(5);
            goto __retry;
        }
        if (fill_ret == RB_DONE)
        {
            /* fall through to the short-buffer EOF check below */
        }
        else if (fill_ret != 0)
        {
            return AUDIO_PLAYER_ERR;
        }
    }

    if (priv->aac_bytes_left < AAC_MAINBUF_SIZE)
    {
        if (priv->input_eof)
        {
            return AUDIO_PLAYER_ERR;
        }
        rtos_delay_milliseconds(5);
        goto __retry;
    }

    // Decode first frame
    short *sample_buffer = player_malloc(AAC_MAX_NSAMPS * 2 * 2 * AAC_MAX_NCHANS);
    if (!sample_buffer)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, malloc sample_buffer fail, %d\n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    int byte_left = priv->aac_bytes_left;

    // Log parsed MP4 info for debugging
    BK_LOGI(AUDIO_PLAYER_TAG, "%s, MP4 parsed info - channels: %u, sample_rate: %d, aac_bytes_left: %d\n",
              __func__, priv->channels, priv->sample_rate, priv->aac_bytes_left);

    // Log first few bytes of AAC data for debugging
    if (priv->aac_read_ptr && priv->aac_bytes_left >= 4)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, AAC data first 4 bytes: 0x%02x 0x%02x 0x%02x 0x%02x, %d\n",
                  __func__, priv->aac_read_ptr[0], priv->aac_read_ptr[1], 
                  priv->aac_read_ptr[2], priv->aac_read_ptr[3], __LINE__);
    }

    // M4A files contain raw AAC frames (without ADTS headers)
    // Set decoder parameters for raw AAC blocks if we have valid info
    if (priv->sample_rate > 0 && priv->channels > 0)
    {
        AACFrameInfo aacFrameInfo = {0};
        aacFrameInfo.nChans = priv->channels;
        aacFrameInfo.sampRateCore = priv->sample_rate;
        aacFrameInfo.profile = AAC_PROFILE_LC;  // Default to LC profile
        aacFrameInfo.bitsPerSample = 16;

        BK_LOGI(AUDIO_PLAYER_TAG, "%s, setting raw block params - nChans: %d, sampRateCore: %d, %d\n",
                  __func__, aacFrameInfo.nChans, aacFrameInfo.sampRateCore, __LINE__);

        int raw_block_ret = AACSetRawBlockParams(priv->decoder, 0, &aacFrameInfo);
        if (raw_block_ret != 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, AACSetRawBlockParams failed, ret=%d, %d\n", __func__, raw_block_ret, __LINE__);
            // This is critical for M4A - raw AAC frames need this
            player_free(sample_buffer);
            return AUDIO_PLAYER_ERR;
        }
        else
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, AACSetRawBlockParams successful, %d\n", __func__, __LINE__);
        }
    }

    ec = AACDecode(priv->decoder, &(priv->aac_read_ptr), &byte_left, sample_buffer);
    if (ec == 0)
    {
        AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);

        // Log decoded AAC info for debugging
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, AAC decoded info - nChans: %d, sampRateOut: %d, bitRate: %d\n",
                  __func__, priv->aacFrameInfo.nChans, priv->aacFrameInfo.sampRateOut, priv->aacFrameInfo.bitRate);

        // Always use decoded info from AAC decoder as it's the source of truth
        // MP4 container info may be inaccurate (e.g., channels field may be wrong)
        info->channel_number = priv->aacFrameInfo.nChans;

        // Use parsed sample rate from MP4 if available, otherwise use AAC decoder info
        // Sample rate from MP4 is usually reliable
        if (priv->sample_rate > 0)
        {
            info->sample_rate = priv->sample_rate;
        }
        else
        {
            info->sample_rate = priv->aacFrameInfo.sampRateOut;
        }

        info->sample_bits = priv->aacFrameInfo.bitsPerSample;
        info->frame_size = 2 * priv->aacFrameInfo.outputSamps;
        info->bps = priv->aacFrameInfo.bitRate;

        // Calculate duration
        if (priv->duration > 0 && priv->time_scale > 0)
        {
            info->duration = (double)priv->duration * 1000.0 / (double)priv->time_scale;
        }
        else
        {
            info->duration = 0.0;
        }

        // Get total bytes from source if available
        info->total_bytes = audio_source_get_total_bytes(decoder->source);
        info->header_bytes = 0;  // Will be calculated if needed

        // Update codec info
        decoder->info.channel_number = info->channel_number;
        decoder->info.sample_rate = info->sample_rate;
        decoder->info.sample_bits = info->sample_bits;
        decoder->info.frame_size = info->frame_size;
        decoder->info.bps = info->bps;
        decoder->info.total_bytes = info->total_bytes;
        decoder->info.header_bytes = info->header_bytes;
        decoder->info.duration = info->duration;

        BK_LOGI(AUDIO_PLAYER_TAG, "m4a channels: %d, sample_rate: %d, sample_bits: %d, bps: %d, duration: %f\n",
                  info->channel_number, info->sample_rate, info->sample_bits, info->bps, info->duration);
    }
    else
    {
        if (ec == ERR_AAC_INDATA_UNDERFLOW)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode finish, ec: %d, %d\n", __func__, ec, __LINE__);
            goto __retry;
        }
        else if (ec == ERR_AAC_NCHANS_TOO_HIGH)
        {
            // Get frame info even if decode failed to see what channels were detected
            AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode fail, ERR_AAC_NCHANS_TOO_HIGH, detected channels: %d, max: %d, %d\n",
                      __func__, priv->aacFrameInfo.nChans, AAC_MAX_NCHANS, __LINE__);
            player_free(sample_buffer);
            return AUDIO_PLAYER_ERR;
        }
        else
        {
            // Get more info about the error
            AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);
            const char *error_name = "UNKNOWN";
            if (ec == ERR_AAC_INVALID_ADTS_HEADER)
            {
                error_name = "ERR_AAC_INVALID_ADTS_HEADER";
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode fail, %s (-3) - M4A contains raw AAC frames, need to set raw block params, %d\n",
                          __func__, error_name, __LINE__);
            }
            else if (ec == ERR_AAC_INVALID_FRAME)
            {
                error_name = "ERR_AAC_INVALID_FRAME";
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode fail, %s (-5) - Invalid AAC frame, %d\n",
                          __func__, error_name, __LINE__);
            }
            else
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode fail, error code: %d, %d\n", __func__, ec, __LINE__);
            }
            player_free(sample_buffer);
            return AUDIO_PLAYER_ERR;
        }
    }

    // Reset read pointer
    priv->aac_read_ptr = priv->aac_read_buffer;

    if (sample_buffer)
    {
        player_free(sample_buffer);
        sample_buffer = NULL;
    }

    return AUDIO_PLAYER_OK;
}

static int m4a_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    int32_t ec;
    m4a_decoder_priv_t *priv;
    priv = (m4a_decoder_priv_t *)decoder->decoder_priv;

__retry:
    while (priv->aac_bytes_left < AAC_MAINBUF_SIZE && !priv->input_eof)
    {
        int fill_ret = m4a_fill_aac_buffer(decoder, priv);
        if (fill_ret == AUDIO_PLAYER_TIMEOUT)
        {
            return AUDIO_PLAYER_TIMEOUT;
        }
        if (fill_ret == RB_DONE)
        {
            break;
        }
        if (fill_ret != 0)
        {
            return AUDIO_PLAYER_ERR;
        }
    }

    bool stream_eof = priv->input_eof;
    if (!stream_eof && priv->mdat_size > 0 &&
        priv->stream_offset >= priv->mdat_offset + priv->mdat_size)
    {
        stream_eof = true;
        priv->input_eof = true;
    }

    if (priv->aac_bytes_left < AAC_MAINBUF_SIZE)
    {
        if (stream_eof)
        {
            if (priv->aac_bytes_left == 0)
            {
                BK_LOGI(AUDIO_PLAYER_TAG, "%s, AAC data drained at end of stream, %d\n", __func__, __LINE__);
                return 0;
            }
            if (priv->stream_aborted)
            {
                /* Aborted stream: residual is likely a truncated frame, drop it instead
                 * of feeding a partial frame into the decoder. */
                BK_LOGW(AUDIO_PLAYER_TAG, "%s, aborted, drop tail: %d < %d, %d\n", __func__, priv->aac_bytes_left, AAC_MAINBUF_SIZE, __LINE__);
                priv->aac_bytes_left = 0;
                return 0;
            }
            /* clean end of stream: fall through and decode the remaining whole frame(s) */
        }
        else
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, wait for more data, read: %d < %d, %d \n", __func__, priv->aac_bytes_left, AAC_MAINBUF_SIZE, __LINE__);
            return AUDIO_PLAYER_TIMEOUT;
        }
    }

    ec = AACDecode(priv->decoder, &priv->aac_read_ptr, (int *)&priv->aac_bytes_left, (short *)buffer);
    if (ec == 0)
    {
        AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);
        return priv->aacFrameInfo.outputSamps * 2;
    }
    else
    {
        if (ec == ERR_AAC_INDATA_UNDERFLOW)
        {
            if (stream_eof)
            {
                BK_LOGI(AUDIO_PLAYER_TAG, "%s, AAC decoder reached end of stream (underflow), %d\n", __func__, __LINE__);
                return 0;
            }
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode need more data, ec: %d, %d\n", __func__, ec, __LINE__);
            goto __retry;
        }
        else if (ec == ERR_AAC_NCHANS_TOO_HIGH)
        {
            // Get frame info even if decode failed to see what channels were detected
            AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode fail, ERR_AAC_NCHANS_TOO_HIGH, detected channels: %d, max: %d, %d\n",
                      __func__, priv->aacFrameInfo.nChans, AAC_MAX_NCHANS, __LINE__);
            return -1;
        }

        BK_LOGE(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode fail, ec: %d, %d\n", __func__, ec, __LINE__);
        return -1;
    }

    return 0;
}

static int m4a_decoder_close(bk_audio_player_decoder_t *decoder)
{
    m4a_decoder_priv_t *priv;
    priv = (m4a_decoder_priv_t *)decoder->decoder_priv;

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, %d\n", __func__, __LINE__);

    if (priv && priv->decoder)
    {
        AACFreeDecoder(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->aac_read_buffer)
    {
        player_free(priv->aac_read_buffer);
        priv->aac_read_buffer = NULL;
    }

    if (priv->sample_sizes)
    {
        player_free(priv->sample_sizes);
        priv->sample_sizes = NULL;
    }

    if (priv->chunk_offsets)
    {
        player_free(priv->chunk_offsets);
        priv->chunk_offsets = NULL;
    }

    if (priv->samples_per_chunk)
    {
        player_free(priv->samples_per_chunk);
        priv->samples_per_chunk = NULL;
    }

    if (priv->chunk_first_sample)
    {
        player_free(priv->chunk_first_sample);
        priv->chunk_first_sample = NULL;
    }

    if (priv->stts_sample_counts)
    {
        player_free(priv->stts_sample_counts);
        priv->stts_sample_counts = NULL;
    }

    if (priv->stts_sample_deltas)
    {
        player_free(priv->stts_sample_deltas);
        priv->stts_sample_deltas = NULL;
    }

    if (priv->stsc_first_chunk)
    {
        player_free(priv->stsc_first_chunk);
        priv->stsc_first_chunk = NULL;
    }

    if (priv->stsc_samples_per_chunk)
    {
        player_free(priv->stsc_samples_per_chunk);
        priv->stsc_samples_per_chunk = NULL;
    }

    return AUDIO_PLAYER_OK;
}

static int m4a_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    m4a_decoder_priv_t *priv;
    priv = (m4a_decoder_priv_t *)decoder->decoder_priv;
    return 2 * priv->aacFrameInfo.outputSamps;
}

const bk_audio_player_decoder_ops_t m4a_decoder_ops =
{
    .name = "m4a",
    .open = m4a_decoder_open,
    .get_info = m4a_decoder_get_info,
    .get_chunk_size = m4a_decoder_get_chunk_size,
    .get_data = m4a_decoder_get_data,
    .close = m4a_decoder_close,
    .calc_position = calc_m4a_position,
    .is_seek_ready = NULL,
};

/* Get M4A decoder operations structure */
const bk_audio_player_decoder_ops_t *bk_audio_player_get_m4a_decoder_ops(void)
{
    return &m4a_decoder_ops;
}
