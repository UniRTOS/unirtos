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

#include <modules/aacdec.h>
#include "codec_api.h"
#include "source_api.h"
#include "player_osal.h"
#include "ring_buffer.h"
#include <components/bk_audio_player/bk_audio_player_types.h>

//#define AAC_AUDIO_BUF_SZ    (8 * 1024) /* feel free to change this, but keep big enough for >= one frame(AAC_MAINBUF_SIZE) at high bitrates */
#define AAC_AUDIO_BUF_SZ    (AAC_MAINBUF_SIZE) /* feel free to change this, but keep big enough for >= one frame(AAC_MAINBUF_SIZE) at high bitrates */

#define AAC_MIN_AUDIO_FRAME_SIZE          (50)    /* Minimum frame length (bytes) treated as valid audio frame */
#define AAC_SEEK_FRAME_ADJUST_THRESHOLD   (3)
#define AAC_SEEK_FRAME_ADJUST_MAX_FRAMES  (64)
#define AAC_SEEK_DEVIATION_THRESHOLD      (1000)  /* If CBR offset deviates from VBR estimate by more than this (bytes), use VBR algorithm */
#define AAC_SEEK_PRECISE_TIME_THRESHOLD   (3)     /* seconds; seek requests earlier than this use precise traversal */
#define AAC_VBR_MIN_DETECT_FRAMES         (48)    /* Minimum frames required before variance-based detection */
#define AAC_VBR_LENGTH_DIFF_THRESHOLD     (200)   /* Bytes; frame length swing beyond this suggests VBR */
#define AAC_VBR_COEF_VARIATION_THRESHOLD  (0.10f) /* Coefficient-of-variation threshold to judge VBR */
#define AAC_TRUE_CBR_MAX_FRAME_DIFF       (3)     /* Strict TRUE-CBR requirement: frame swing must be <= 3 bytes */

typedef struct aac_decoder_priv
{
    HAACDecoder decoder;
    AACFrameInfo aacFrameInfo;

    uint32_t bitRate;           /* save the average value of bitrate calculated */
    uint32_t frames;
    uint32_t stream_offset;

    /* aac read session */
    uint8_t *read_buffer;
    uint8_t *read_ptr;
    uint32_t bytes_left;

    int current_sample_rate;

    float T1;

    biterate_type_t biterate_type;

    uint32_t total_bytes;
    uint32_t total_frames;
    double total_duration;     // play duration of total bytes in milisecond (ms)

    uint32_t lead_bytes;       // bytes of leading metadata/config frames
    uint32_t lead_frames;      // number of leading metadata/config frames
    uint8_t eos_reached;
    uint8_t stream_aborted;
    uint32_t analyze_frame_min;
    uint32_t analyze_frame_max;
    uint64_t analyze_frame_sum;
    uint64_t analyze_frame_square_sum;
    uint32_t analyze_frame_count;
} aac_decoder_priv_t;


static int32_t codec_aac_fill_buffer(bk_audio_player_decoder_t *codec)
{
    int bytes_read;
    size_t bytes_to_read;

    aac_decoder_priv_t *priv = (aac_decoder_priv_t *)codec->decoder_priv;

    if (priv->eos_reached)
    {
        return RB_DONE;
    }

    /* adjust read ptr */
    if (priv->bytes_left > 0xffff0000)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "c: 0x%x, buf: 0x%x.\n", priv, priv->read_buffer);
        BK_LOGW(AUDIO_PLAYER_TAG, "rd: 0x%x, left: 0x%x.\n", priv->read_ptr, priv->bytes_left);
        return (-1);
    }

    if (priv->bytes_left > 0)
    {
        memmove(priv->read_buffer, priv->read_ptr, priv->bytes_left);
    }
    priv->read_ptr = priv->read_buffer;

    //    bytes_to_read = (AAC_AUDIO_BUF_SZ - priv->bytes_left) & ~(512 - 1);
    bytes_to_read = AAC_AUDIO_BUF_SZ - priv->bytes_left;
    //    BK_LOGI(AUDIO_PLAYER_TAG,"need size: %d \n", bytes_to_read);
    if (bytes_to_read == 0)
    {
        return 0;
    }

    bytes_read = audio_source_read_data(codec->source, (char *)(priv->read_buffer + priv->bytes_left), bytes_to_read);
    if (bytes_read > 0)
    {
        priv->bytes_left = priv->bytes_left + bytes_read;
        priv->eos_reached = 0;
        priv->stream_aborted = 0;
        return 0;
    }
    else if (bytes_read == AUDIO_PLAYER_TIMEOUT)
    {
        return AUDIO_PLAYER_TIMEOUT;
    }
    else if (bytes_read == 0 || bytes_read == RB_DONE)
    {
        /* normal end of stream: tail data is complete and should be decoded */
        priv->eos_reached = 1;
        priv->stream_aborted = 0;
        if (priv->bytes_left != 0)
        {
            return 0;
        }

        BK_LOGW(AUDIO_PLAYER_TAG, "can't read more data, end of stream. left=%d \n", priv->bytes_left);
        return RB_DONE;
    }
    else if (bytes_read == RB_ABORT)
    {
        /* aborted stream: residual tail may be a truncated frame and must be dropped */
        priv->eos_reached = 1;
        priv->stream_aborted = 1;
        if (priv->bytes_left != 0)
        {
            return 0;
        }

        BK_LOGW(AUDIO_PLAYER_TAG, "stream aborted, end of stream. left=%d \n", priv->bytes_left);
        return RB_DONE;
    }
    else if (priv->bytes_left != 0)
    {
        return 0;
    }

    BK_LOGW(AUDIO_PLAYER_TAG, "can't read more data, ret=%d, left=%d \n", bytes_read, priv->bytes_left);
    return -1;
}

static void aac_reset_read_state(aac_decoder_priv_t *priv)
{
    priv->read_ptr = priv->read_buffer;
    priv->bytes_left = 0;
    priv->eos_reached = 0;
    priv->stream_aborted = 0;
}

#if 0
static uint32_t data_seek_aac(void *handle, uint32_t pos, uint32_t whence)
{
    bk_audio_player_decoder_t *codec = (bk_audio_player_decoder_t *)handle;

    audio_source_seek(codec->source, pos, whence);

    return 0;
}
#endif

/**
 * @brief Skip ID3v2 tag at the beginning of AAC file
 * @param codec Audio codec context
 * @return Offset bytes skipped, 0 if no ID3v2 tag found
 */
static int codec_aac_skip_id3v2(bk_audio_player_decoder_t *codec)
{
    int offset = 0;
    uint8_t tag[10];
    aac_decoder_priv_t *priv = (aac_decoder_priv_t *)codec->decoder_priv;

    /* Reset read pointer */
    aac_reset_read_state(priv);

    /* Read first 3 bytes to check for ID3 signature */
    if (audio_source_read_data(codec->source, (char *)tag, 3) != 3)
    {
        goto __exit;
    }

    /* Check for ID3v2 signature */
    if (tag[0] == 'I' && tag[1] == 'D' && tag[2] == '3')
    {
        int size;

        /* Read remaining 7 bytes of ID3v2 header */
        if (audio_source_read_data(codec->source, (char *)tag + 3, 7) != 7)
        {
            goto __exit;
        }

        /* Parse ID3v2 tag size (synchsafe integer) */
        size = ((tag[6] & 0x7F) << 21) | ((tag[7] & 0x7F) << 14) |
              ((tag[8] & 0x7F) << 7) | (tag[9] & 0x7F);

        offset = size + 10; /* 10 bytes header + tag size */

        /* Skip the entire ID3v2 tag data */
        {
            int rest_size = size;
            uint8_t skip_buffer[512];
            int chunk;

            while (rest_size > 0)
            {
                if (rest_size > sizeof(skip_buffer))
                {
                    chunk = sizeof(skip_buffer);
                }
                else
                {
                    chunk = rest_size;
                }

                int length = audio_source_read_data(codec->source, (char *)skip_buffer, chunk);
                if (length > 0)
                {
                    rest_size -= length;
                }
                else
                {
                    break; /* Read failed */
                }
            }

            aac_reset_read_state(priv);
        }

        BK_LOGI(AUDIO_PLAYER_TAG, "%s, skipped ID3v2 tag, offset: %d\n", __func__, offset);
        return offset;
    }

__exit:
    /* No ID3v2 tag found, reset to beginning */
    /* If we read 3 bytes but didn't find ID3, we need to seek back */
    if (audio_source_seek(codec->source, 0, SEEK_SET) != 0)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, failed to seek to beginning\n", __func__);
    }
    aac_reset_read_state(priv);
    return offset;
}

static uint32_t aac_adts_bitrate_calc(bk_audio_player_decoder_t *codec)
{
    uint32_t bite_rate = 0;
    uint32_t frames = 0;
    uint32_t length = 0;
    int frame_length = 0;

    aac_decoder_priv_t *priv = (aac_decoder_priv_t *)codec->decoder_priv;

    /* reset read_ptr */
    aac_reset_read_state(priv);
    priv->analyze_frame_min = 0xFFFFFFFF;
    priv->analyze_frame_max = 0;
    priv->analyze_frame_sum = 0;
    priv->analyze_frame_square_sum = 0;
    priv->analyze_frame_count = 0;

    if (0 != audio_source_seek(codec->source, 0, SEEK_SET))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, audio_source_seek fail, %d\n", __func__, __LINE__);
        return 0;
    }

    /* Skip ID3v2 tag if present */
    priv->stream_offset = codec_aac_skip_id3v2(codec);
    if (priv->stream_offset < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, failed to skip ID3v2 tag, %d\n", __func__, __LINE__);
        return 0;
    }

    if (codec_aac_fill_buffer(codec) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, read data fail, %d\n", __func__, __LINE__);
        return 0;
    }

    /* first time through figure out what the file format is */
    if (priv->read_ptr[0] == 'A' && priv->read_ptr[1] == 'D' && priv->read_ptr[2] == 'I' && priv->read_ptr[3] == 'F')
    {
        /* ADIF header */
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, this aac file is ADIF format, not support, %d\n", __func__, __LINE__);
        return 0;
    }
    else
    {
        /* ADTS by default */
        //aacDecInfo->format = AAC_FF_ADTS;
    }

    priv->lead_bytes = 0;
    priv->lead_frames = 0;
    int found_audio_frame = 0;

    while (1)
    {
        /* check whether file is empty */
        if (priv->bytes_left == 0)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, file is empty\n", __func__);
            break;
        }

        frame_length = AACParseAdtsHeader(priv->decoder, (unsigned char *)priv->read_ptr, priv->bytes_left);
        if (frame_length > 0)
        {
            frames += 1;
            length += frame_length;

            if (!found_audio_frame)
            {
                if (frame_length < AAC_MIN_AUDIO_FRAME_SIZE)
                {
                    priv->lead_frames += 1;
                    priv->lead_bytes += frame_length;
                }
                else
                {
                    found_audio_frame = 1;
                }
            }

            if (frame_length >= AAC_MIN_AUDIO_FRAME_SIZE)
            {
                priv->analyze_frame_count += 1;
                priv->analyze_frame_sum += frame_length;
                priv->analyze_frame_square_sum += (uint64_t)frame_length * (uint64_t)frame_length;

                if ((uint32_t)frame_length < priv->analyze_frame_min)
                {
                    priv->analyze_frame_min = frame_length;
                }

                if ((uint32_t)frame_length > priv->analyze_frame_max)
                {
                    priv->analyze_frame_max = frame_length;
                }
            }

            /* get frame_info include samprate */
            AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);

            if (frame_length <= priv->bytes_left)
            {
                priv->read_ptr += frame_length;
                priv->bytes_left -= frame_length;
            }
            else
            {
                if (0 != audio_source_seek(codec->source, frame_length - priv->bytes_left, SEEK_CUR))
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, audio_source_seek fail, %d\n", __func__, __LINE__);
                    break;
                }
                aac_reset_read_state(priv);
            }
        }
        else
        {
            /* reset read_ptr again */
            aac_reset_read_state(priv);
        }

        codec_aac_fill_buffer(codec);
    }

    BK_LOGE(AUDIO_PLAYER_TAG, "%s, length: %d, frames: %d, outputSamps: %d, %d\n", __func__, length, frames, priv->aacFrameInfo.outputSamps, __LINE__);

    priv->total_bytes = length;
    priv->total_frames = frames;

    if (frames > 0)
    {
        bite_rate = (double)length * 8.0 * (double)priv->aacFrameInfo.sampRateOut / (double)(frames * 1024);
    }

    /* Seek back to beginning of audio data (after ID3v2 tag) */
    if (0 != audio_source_seek(codec->source, priv->stream_offset, SEEK_SET))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, audio_source_seek fail, %d\n", __func__, __LINE__);
    }
    /* reset read_ptr */
    aac_reset_read_state(priv);

    return bite_rate;
}

static void aac_finalize_bitrate_type_from_stats(aac_decoder_priv_t *priv)
{
    if (priv->biterate_type != BITERATE_TYPE_UNKNOW)
    {
        return;
    }

    if (priv->analyze_frame_min == 0xFFFFFFFF)
    {
        priv->analyze_frame_min = 0;
    }

    if (priv->analyze_frame_count == 0)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "[ADTS_BITRATE] no analyzable frames, keep type unknown\n");
        return;
    }

    double avg = (double)priv->analyze_frame_sum / (double)priv->analyze_frame_count;
    double mean_square = (double)priv->analyze_frame_square_sum / (double)priv->analyze_frame_count;
    double variance = mean_square - (avg * avg);
    if (variance < 0.0)
    {
        variance = 0.0;
    }

    double avg_frame_len = (double)priv->analyze_frame_sum / (double)priv->analyze_frame_count;
    double cv2 = (avg_frame_len > 0.0) ? (variance / (avg_frame_len * avg_frame_len)) : 0.0;
    double cv_threshold = AAC_VBR_COEF_VARIATION_THRESHOLD;
    double cv_threshold_sq = cv_threshold * cv_threshold;
    uint32_t frame_range = (priv->analyze_frame_max >= priv->analyze_frame_min) ?
                           (priv->analyze_frame_max - priv->analyze_frame_min) : 0;
    // Calculate relative frame range as percentage of average frame length
    double relative_range = (avg_frame_len > 0.0) ? ((double)frame_range / avg_frame_len) : 0.0;
    double relative_range_threshold = 0.5;  // 50% variation threshold

    if (frame_range <= AAC_TRUE_CBR_MAX_FRAME_DIFF)
    {
        priv->biterate_type = BITERATE_TYPE_CBR;
        BK_LOGI(AUDIO_PLAYER_TAG, "[ADTS_TRUE_CBR] frames:%d, len_range:%d~%d(diff=%d<=%d), cv2:%f, rel_range:%.2f%%\n",
                   priv->analyze_frame_count, priv->analyze_frame_min, priv->analyze_frame_max,
                   frame_range, AAC_TRUE_CBR_MAX_FRAME_DIFF, cv2, relative_range * 100.0);
        return;
    }

    if (priv->analyze_frame_count < AAC_VBR_MIN_DETECT_FRAMES)
    {
        priv->biterate_type = BITERATE_TYPE_VBR_ADTS;
        BK_LOGI(AUDIO_PLAYER_TAG, "[ADTS_VBR] insufficient frames yet diff=%d>%d, mark as VBR\n",
                   frame_range, AAC_TRUE_CBR_MAX_FRAME_DIFF);
        return;
    }

    // VBR detection: require both cv2 and relative_range to exceed thresholds
    // This is more conservative and avoids false positives for CBR files with minor variations
    if ((cv2 >= cv_threshold_sq) && (relative_range >= relative_range_threshold))
    {
        priv->biterate_type = BITERATE_TYPE_VBR_ADTS;
        BK_LOGI(AUDIO_PLAYER_TAG, "[ADTS_VBR] frames:%d, len_range:%d~%d(diff=%d>%d), cv2:%f, rel_range:%.2f%%\n",
                   priv->analyze_frame_count, priv->analyze_frame_min, priv->analyze_frame_max,
                   frame_range, AAC_TRUE_CBR_MAX_FRAME_DIFF, cv2, relative_range * 100.0);
    }
    else
    {
        priv->biterate_type = BITERATE_TYPE_VBR_ADTS;
        BK_LOGI(AUDIO_PLAYER_TAG, "[ADTS_VBR_RELAXED] frames:%d, len_range:%d~%d(diff=%d>%d) even though cv2/rel_range low\n",
                   priv->analyze_frame_count, priv->analyze_frame_min, priv->analyze_frame_max,
                   frame_range, AAC_TRUE_CBR_MAX_FRAME_DIFF);
    }
}

// Forward declaration
static int verify_frame_sequence(bk_audio_player_decoder_t *codec, aac_decoder_priv_t *priv, uint32_t frame_offset);
static uint32_t advance_frames_forward(bk_audio_player_decoder_t *codec, aac_decoder_priv_t *priv, uint32_t start_offset, uint32_t frames_to_skip);
static uint32_t refine_seek_offset(bk_audio_player_decoder_t *codec, aac_decoder_priv_t *priv, uint32_t offset, uint32_t target_frame);

/**
 * @brief Verify frame header matches known audio stream parameters
 * @param priv AAC codec private data
 * @return 1 if frame info matches, 0 otherwise
 */
static int verify_frame_header_info(aac_decoder_priv_t *priv)
{
    AACFrameInfo frame_info;
    AACGetLastFrameInfo(priv->decoder, &frame_info);

    // Verify sample rate matches
    if (priv->aacFrameInfo.sampRateOut > 0 && frame_info.sampRateOut > 0)
    {
        if (frame_info.sampRateOut != priv->aacFrameInfo.sampRateOut)
        {
            return 0;  // Sample rate mismatch
        }
    }

    // Verify channel count matches
    if (priv->aacFrameInfo.nChans > 0 && frame_info.nChans > 0)
    {
        if (frame_info.nChans != priv->aacFrameInfo.nChans)
        {
            return 0;  // Channel count mismatch
        }
    }

    // Verify bits per sample matches
    if (priv->aacFrameInfo.bitsPerSample > 0 && frame_info.bitsPerSample > 0)
    {
        if (frame_info.bitsPerSample != priv->aacFrameInfo.bitsPerSample)
        {
            return 0;  // Bits per sample mismatch
        }
    }

    // Verify profile matches (if available)
    if (priv->aacFrameInfo.profile > 0 && frame_info.profile > 0)
    {
        if (frame_info.profile != priv->aacFrameInfo.profile)
        {
            return 0;  // Profile mismatch
        }
    }

    return 1;  // Frame info matches
}

/**
 * @brief Find next valid ADTS frame header from given offset
 * @param codec Audio codec context
 * @param priv AAC codec private data
 * @param offset Starting offset to search from
 * @return Offset of found frame header, or 0 if not found
 */
static uint32_t find_next_frame_header(bk_audio_player_decoder_t *codec, aac_decoder_priv_t *priv, uint32_t offset)
{
    uint32_t search_range = 4096;  // Search range: 4KB
    uint32_t search_start = offset;
    uint32_t total_bytes = audio_source_get_total_bytes(codec->source);

    if (total_bytes > 0 && search_start >= total_bytes)
    {
        return 0;
    }

    uint32_t search_end = search_start + search_range;
    if (total_bytes > 0 && search_end > total_bytes)
    {
        search_end = total_bytes;
    }

    // Seek to search start position
    if (audio_source_seek(codec->source, search_start, SEEK_SET) != 0)
    {
        return 0;
    }

    aac_reset_read_state(priv);

    // Fill buffer
    if (codec_aac_fill_buffer(codec) != 0)
    {
        return 0;
    }

    uint32_t current_pos = search_start;

    while (current_pos < search_end && priv->bytes_left >= 7)
    {
        // Check for ADTS sync word (0xFFF)
        if (priv->read_ptr[0] == 0xFF && (priv->read_ptr[1] & 0xF0) == 0xF0)
        {
            // Try to parse frame header
            int frame_length = AACParseAdtsHeader(priv->decoder, (unsigned char *)priv->read_ptr, priv->bytes_left);
            if (frame_length > 0)
            {
                // Verify frame length is reasonable
                // Skip very small frames (< 50 bytes) which are likely metadata/configuration frames
                // These small frames may cause decoder errors if sent to decoder
                if (frame_length >= 50 && frame_length < 8192)
                {
                    // Verify frame info matches known stream parameters (to filter false sync words)
                    if (verify_frame_header_info(priv))
                    {
                        return current_pos;
                    }
                }
            }
        }

        // Move to next byte
        priv->read_ptr++;
        priv->bytes_left--;
        current_pos++;

        // Refill buffer if needed
        if (priv->bytes_left < 7)
        {
            if (codec_aac_fill_buffer(codec) != 0)
            {
                break;
            }
        }
    }

    return 0;
}

/**
 * @brief Align offset to nearest valid ADTS frame header
 * @param codec Audio codec context
 * @param priv AAC codec private data
 * @param offset Calculated offset to align
 * @return Aligned offset at valid frame header
 */
static uint32_t align_to_frame_header(bk_audio_player_decoder_t *codec, aac_decoder_priv_t *priv, uint32_t offset)
{
    uint32_t search_range = 4096;  // Search range: 4KB forward and backward (increased for better accuracy)
    uint32_t search_start = (offset > search_range) ? (offset - search_range) : priv->stream_offset;
    uint32_t search_end = offset + search_range;
    uint32_t total_bytes = audio_source_get_total_bytes(codec->source);

    if (total_bytes > 0 && search_end >= total_bytes)
    {
        search_end = total_bytes - 1;
    }

    // Save current state
    uint8_t *saved_read_ptr = priv->read_ptr;
    uint32_t saved_bytes_left = priv->bytes_left;
    uint8_t saved_eos_reached = priv->eos_reached;

    // Seek to search start position
    if (audio_source_seek(codec->source, search_start, SEEK_SET) != 0)
    {
        return offset;
    }

    aac_reset_read_state(priv);

    // Fill buffer
    if (codec_aac_fill_buffer(codec) != 0)
    {
        priv->read_ptr = saved_read_ptr;
        priv->bytes_left = saved_bytes_left;
        priv->eos_reached = saved_eos_reached;
        return offset;
    }

    // Search for frame headers: prefer frame before target, then frame after
    uint32_t best_before = 0;  // Best frame header before target
    uint32_t best_after = 0;    // Best frame header after target
    uint32_t min_distance_before = 0xFFFFFFFF;
    uint32_t min_distance_after = 0xFFFFFFFF;
    uint32_t current_pos = search_start;

    while (current_pos < search_end && priv->bytes_left >= 7)
    {
        // Check for ADTS sync word (0xFFF)
        if (priv->read_ptr[0] == 0xFF && (priv->read_ptr[1] & 0xF0) == 0xF0)
        {
            // Try to parse frame header
            int frame_length = AACParseAdtsHeader(priv->decoder, (unsigned char *)priv->read_ptr, priv->bytes_left);
            if (frame_length > 0)
            {
                // Verify frame length is reasonable
                // ADTS frame should be at least 7 bytes (header), typically < 8192
                // Skip very small frames (< 50 bytes) which are likely metadata/configuration frames
                // These small frames may cause decoder errors (e.g., assertion failures) if sent to decoder
                if (frame_length >= 50 && frame_length < 8192)
                {
                    // Verify frame info matches known stream parameters (to filter false sync words)
                    if (verify_frame_header_info(priv))
                    {
                        // Found valid frame header with matching parameters and reasonable size
                        if (current_pos <= offset)
                        {
                            // Frame before or at target position
                            uint32_t distance = offset - current_pos;
                            if (distance < min_distance_before)
                            {
                                min_distance_before = distance;
                                best_before = current_pos;
                            }
                        }
                        else
                        {
                            // Frame after target position
                            uint32_t distance = current_pos - offset;
                            if (distance < min_distance_after)
                            {
                                min_distance_after = distance;
                                best_after = current_pos;
                            }
                        }
                    }
                }
            }
        }

        // Move to next byte
        priv->read_ptr++;
        priv->bytes_left--;
        current_pos++;

        // Refill buffer if needed
        if (priv->bytes_left < 7)
        {
            if (codec_aac_fill_buffer(codec) != 0)
            {
                break;
            }
        }
    }

    // Restore state (read_ptr and bytes_left will be reset when seek is called)
    priv->read_ptr = saved_read_ptr;
    priv->bytes_left = saved_bytes_left;
    priv->eos_reached = saved_eos_reached;

    // Verify the selected frame by checking frame sequence continuity
    // This helps filter out false positives that pass parameter verification
    int is_valid_sequence = 0;

    if (best_before > 0)
    {
        // Verify frame sequence: check if next frame is at expected position
        is_valid_sequence = verify_frame_sequence(codec, priv, best_before);

        if (is_valid_sequence)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, aligned to frame before target: offset=%d->%d, distance=%d, %d\n",
                       __func__, offset, best_before, min_distance_before, __LINE__);
            return best_before;
        }
        else
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, frame at offset=%d failed sequence verification, trying alternative, %d\n",
                       __func__, best_before, __LINE__);
        }
    }

    // If best_before failed sequence verification, try best_after
    if (!is_valid_sequence && best_after > 0 && min_distance_after < 1024)
    {
        is_valid_sequence = verify_frame_sequence(codec, priv, best_after);

        if (is_valid_sequence)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, aligned to frame after target: offset=%d->%d, distance=%d, %d\n",
                       __func__, offset, best_after, min_distance_after, __LINE__);
            return best_after;
        }
        else
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, frame at offset=%d failed sequence verification, %d\n",
                       __func__, best_after, __LINE__);
        }
    }

    // If both failed, try to find any valid frame in a wider range
    if (!is_valid_sequence)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, no frame passed sequence verification, searching wider range, %d\n",
                   __func__, __LINE__);

        // Search in wider range (8KB) for a frame with valid sequence
        uint32_t wider_range = 8192;
        uint32_t wider_start = (offset > wider_range) ? (offset - wider_range) : priv->stream_offset;
        uint32_t wider_end = offset + wider_range;

        if (total_bytes > 0 && wider_end >= total_bytes)
        {
            wider_end = total_bytes - 1;
        }

        if (audio_source_seek(codec->source, wider_start, SEEK_SET) == 0)
        {
            aac_reset_read_state(priv);

            if (codec_aac_fill_buffer(codec) == 0)
            {
                uint32_t wider_pos = wider_start;
                while (wider_pos < wider_end && priv->bytes_left >= 7)
                {
                    if (priv->read_ptr[0] == 0xFF && (priv->read_ptr[1] & 0xF0) == 0xF0)
                    {
                        int frame_length = AACParseAdtsHeader(priv->decoder, (unsigned char *)priv->read_ptr, priv->bytes_left);
                        // Skip very small frames (< 50 bytes) to avoid decoder errors
                        if (frame_length > 0 && frame_length >= 50 && frame_length < 8192)
                        {
                            if (verify_frame_header_info(priv))
                            {
                                if (verify_frame_sequence(codec, priv, wider_pos))
                                {
                                    BK_LOGI(AUDIO_PLAYER_TAG, "%s, found valid frame with sequence at offset=%d, %d\n",
                                               __func__, wider_pos, __LINE__);
                                    priv->read_ptr = saved_read_ptr;
                                    priv->bytes_left = saved_bytes_left;
                                    priv->eos_reached = saved_eos_reached;
                                    return wider_pos;
                                }
                            }
                        }
                    }

                    priv->read_ptr++;
                    priv->bytes_left--;
                    wider_pos++;

                    if (priv->bytes_left < 7)
                    {
                        if (codec_aac_fill_buffer(codec) != 0)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    // Restore state
    priv->read_ptr = saved_read_ptr;
    priv->bytes_left = saved_bytes_left;
    priv->eos_reached = saved_eos_reached;

    // Last resort: return the best candidate even if sequence verification failed
    if (best_before > 0)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, using best_before without sequence verification: offset=%d->%d, %d\n",
                   __func__, offset, best_before, __LINE__);
        return best_before;
    }
    else if (best_after > 0 && min_distance_after < 1024)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, using best_after without sequence verification: offset=%d->%d, %d\n",
                   __func__, offset, best_after, __LINE__);
        return best_after;
    }
    else
    {
        // No valid frame found, return original offset (will be handled by decoder)
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, no valid frame header found near offset=%d, returning original, %d\n",
                   __func__, offset, __LINE__);
        return offset;
    }
}

/**
 * @brief Verify frame sequence continuity by checking next frame
 * @param codec Audio codec context
 * @param priv AAC codec private data
 * @param frame_offset Offset of the frame to verify
 * @return 1 if sequence is valid, 0 otherwise
 */
static int verify_frame_sequence(bk_audio_player_decoder_t *codec, aac_decoder_priv_t *priv, uint32_t frame_offset)
{
    uint8_t *saved_read_ptr = priv->read_ptr;
    uint32_t saved_bytes_left = priv->bytes_left;
    uint8_t saved_eos_reached = priv->eos_reached;
    int ret = 0;

    // Seek to frame position
    if (audio_source_seek(codec->source, frame_offset, SEEK_SET) != 0)
    {
        return 0;
    }

    aac_reset_read_state(priv);

    // Fill buffer
    if (codec_aac_fill_buffer(codec) != 0)
    {
        priv->read_ptr = saved_read_ptr;
        priv->bytes_left = saved_bytes_left;
        priv->eos_reached = saved_eos_reached;
        return 0;
    }

    // Parse current frame
    int frame_length = AACParseAdtsHeader(priv->decoder, (unsigned char *)priv->read_ptr, priv->bytes_left);
    // Skip very small frames (< 50 bytes) which are likely metadata/configuration frames
    // These small frames may cause decoder errors if sent to decoder
    if (frame_length <= 0 || frame_length < 50 || frame_length >= 8192)
    {
        priv->read_ptr = saved_read_ptr;
        priv->bytes_left = saved_bytes_left;
        priv->eos_reached = saved_eos_reached;
        return 0;
    }

    // Verify frame info
    if (!verify_frame_header_info(priv))
    {
        priv->read_ptr = saved_read_ptr;
        priv->bytes_left = saved_bytes_left;
        priv->eos_reached = saved_eos_reached;
        return 0;
    }

    // Calculate next frame expected position
    uint32_t next_frame_expected = frame_offset + frame_length;

    // Move to next frame position
    if (frame_length <= priv->bytes_left)
    {
        priv->read_ptr += frame_length;
        priv->bytes_left -= frame_length;
    }
    else
    {
        if (audio_source_seek(codec->source, next_frame_expected, SEEK_SET) != 0)
        {
            priv->read_ptr = saved_read_ptr;
            priv->bytes_left = saved_bytes_left;
            priv->eos_reached = saved_eos_reached;
            return 0;
        }
        aac_reset_read_state(priv);
        if (codec_aac_fill_buffer(codec) != 0)
        {
            priv->read_ptr = saved_read_ptr;
            priv->bytes_left = saved_bytes_left;
            priv->eos_reached = saved_eos_reached;
            return 0;
        }
    }

    // Check if next frame exists at expected position
    if (priv->bytes_left >= 7)
    {
        if (priv->read_ptr[0] == 0xFF && (priv->read_ptr[1] & 0xF0) == 0xF0)
        {
            int next_frame_length = AACParseAdtsHeader(priv->decoder, (unsigned char *)priv->read_ptr, priv->bytes_left);
            // Skip very small frames (< 50 bytes) to avoid decoder errors
            if (next_frame_length > 0 && next_frame_length >= 50 && next_frame_length < 8192)
            {
                // Verify next frame info matches
                if (verify_frame_header_info(priv))
                {
                    ret = 1;  // Sequence is valid
                }
            }
        }
    }

    // Restore state
    priv->read_ptr = saved_read_ptr;
    priv->bytes_left = saved_bytes_left;
    priv->eos_reached = saved_eos_reached;

    return ret;
}

static uint32_t advance_frames_forward(bk_audio_player_decoder_t *codec, aac_decoder_priv_t *priv, uint32_t start_offset, uint32_t frames_to_skip)
{
    if (frames_to_skip == 0)
    {
        return start_offset;
    }

    uint8_t *saved_read_ptr = priv->read_ptr;
    uint32_t saved_bytes_left = priv->bytes_left;
    uint8_t saved_eos_reached = priv->eos_reached;
    uint32_t saved_offset = start_offset;
    uint32_t current_offset = start_offset;
    uint32_t skipped = 0;

    while (skipped < frames_to_skip)
    {
        if (audio_source_seek(codec->source, current_offset, SEEK_SET) != 0)
        {
            break;
        }

        aac_reset_read_state(priv);

        if (codec_aac_fill_buffer(codec) != 0)
        {
            break;
        }

        int frame_length = AACParseAdtsHeader(priv->decoder, (unsigned char *)priv->read_ptr, priv->bytes_left);
        if (frame_length <= 0 || frame_length < AAC_MIN_AUDIO_FRAME_SIZE || frame_length >= 8192)
        {
            current_offset += 1;
            continue;
        }

        current_offset += frame_length;
        skipped++;
    }

    audio_source_seek(codec->source, saved_offset, SEEK_SET);
    priv->read_ptr = saved_read_ptr;
    priv->bytes_left = saved_bytes_left;
    priv->eos_reached = saved_eos_reached;

    if (skipped == frames_to_skip)
    {
        return current_offset;
    }

    return start_offset;
}

static uint32_t refine_seek_offset(bk_audio_player_decoder_t *codec, aac_decoder_priv_t *priv, uint32_t offset, uint32_t target_frame)
{
    uint32_t audio_start = priv->stream_offset + priv->lead_bytes;
    if (offset < audio_start)
    {
        offset = audio_start;
    }

    if (target_frame <= priv->lead_frames)
    {
        return offset;
    }

    uint32_t total_bytes = audio_source_get_total_bytes(codec->source);
    uint32_t audio_bytes = 0;
    if (priv->total_bytes > 0)
    {
        audio_bytes = (priv->total_bytes > priv->lead_bytes) ? (priv->total_bytes - priv->lead_bytes) : priv->total_bytes;
    }
    else if (total_bytes > audio_start)
    {
        audio_bytes = total_bytes - audio_start;
    }

    uint32_t audio_frames = 0;
    if (priv->total_frames > priv->lead_frames)
    {
        audio_frames = priv->total_frames - priv->lead_frames;
    }

    if (audio_bytes == 0 || audio_frames == 0)
    {
        return offset;
    }

    uint32_t avg_frame_size = audio_bytes / audio_frames;
    if (avg_frame_size == 0)
    {
        return offset;
    }

    double approx_frame = priv->lead_frames;
    if (offset > audio_start)
    {
        approx_frame += (double)(offset - audio_start) / (double)avg_frame_size;
    }

    int32_t frame_delta = (int32_t)target_frame - (int32_t)approx_frame;

    if (frame_delta > AAC_SEEK_FRAME_ADJUST_THRESHOLD)
    {
        if (frame_delta > AAC_SEEK_FRAME_ADJUST_MAX_FRAMES)
        {
            frame_delta = AAC_SEEK_FRAME_ADJUST_MAX_FRAMES;
        }

        uint32_t new_offset = advance_frames_forward(codec, priv, offset, (uint32_t)frame_delta);
        if (new_offset != offset)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, refine seek forward by %d frames: %d -> %d, %d\n",
                       __func__, (int)frame_delta, offset, new_offset, __LINE__);
            offset = new_offset;
        }
    }

    return offset;
}

/**
 * @brief Calculate position for CBR AAC file
 * @param codec Audio codec context
 * @param priv AAC codec private data
 * @param second Target time in seconds
 * @return Calculated file offset
 */
static uint32_t calc_aac_position_cbr(bk_audio_player_decoder_t *codec, aac_decoder_priv_t *priv, double target_time_sec)
{
    int sample_rate = priv->aacFrameInfo.sampRateOut;
    // Use core samples per frame (1024) for seek calculation, not outputSamps
    // outputSamps can be 2048 for SBR, but seek should use core frame size (1024)
    int samples_per_frame = 1024;  // AAC core frame always has 1024 samples
    uint32_t bitrate = priv->bitRate;
    uint32_t total_bytes = audio_source_get_total_bytes(codec->source);

    if (sample_rate == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid frame info, sample_rate=%d, %d\n",
                   __func__, sample_rate, __LINE__);
        return priv->stream_offset;
    }

    if (bitrate == 0)
    {
        // If no bitrate info, calculate average bitrate
        // Use core samples (1024) for bitrate calculation
        if (priv->total_bytes > 0 && priv->total_frames > 0)
        {
            bitrate = (priv->total_bytes * 8 * sample_rate) / (priv->total_frames * samples_per_frame);
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, calculated average bitrate: %d (using core samples=%d), %d\n",
                       __func__, bitrate, samples_per_frame, __LINE__);
        }
        else if (total_bytes > 0 && priv->stream_offset < total_bytes)
        {
            // Estimate from file size and duration
            uint32_t audio_bytes = total_bytes - priv->stream_offset;
            if (priv->total_duration > 0)
            {
                double duration_sec = priv->total_duration / 1000.0;
                bitrate = (audio_bytes * 8) / duration_sec;
                BK_LOGI(AUDIO_PLAYER_TAG, "%s, estimated bitrate from duration: %d, %d\n", __func__, bitrate, __LINE__);
            }
        }

        if (bitrate == 0)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, no bitrate info available, use default, %d\n", __func__, __LINE__);
            return priv->stream_offset;
        }
    }

    // Calculate target frame number using core samples (1024)
    double target_frame_f = target_time_sec * (double)sample_rate / (double)samples_per_frame;
    if (target_frame_f < 0.0)
    {
        target_frame_f = 0.0;
    }
    uint32_t target_frame = (uint32_t)(target_frame_f);

    // Calculate frame size in bytes
    // frame_size = (bitrate * samples_per_frame) / (8 * sample_rate)
    // Note: Use core samples (1024) for frame size calculation
    uint32_t frame_size = (bitrate * samples_per_frame) / (8 * sample_rate);
    if (frame_size == 0)
    {
        frame_size = 1;  // Prevent division by zero
    }

    // For better accuracy, prefer using average frame size from total frames/bytes information.
    // Exclude leading metadata/configuration frames from the calculation.
    uint32_t offset = priv->stream_offset;
    uint32_t lead_bytes = priv->lead_bytes;
    uint32_t lead_frames = priv->lead_frames;
    uint32_t total_frames = priv->total_frames;
    uint32_t total_audio_bytes = 0;
    uint32_t total_audio_frames = 0;

    if (priv->total_bytes > 0)
    {
        total_audio_bytes = (priv->total_bytes > lead_bytes) ? (priv->total_bytes - lead_bytes) : priv->total_bytes;
    }
    else if (total_bytes > priv->stream_offset)
    {
        // fallback to source total bytes if priv->total_bytes was not set
        total_audio_bytes = total_bytes - priv->stream_offset;
        if (lead_bytes < total_audio_bytes)
        {
            total_audio_bytes -= lead_bytes;
        }
    }

    if (total_frames > lead_frames)
    {
        total_audio_frames = total_frames - lead_frames;
    }

    uint32_t effective_target_frame = (target_frame > lead_frames) ? (target_frame - lead_frames) : 0;

    if (total_audio_frames > 0 && total_audio_bytes > 0)
    {
        uint32_t avg_frame_size = total_audio_bytes / total_audio_frames;
        if (avg_frame_size > 0)
        {
            offset = priv->stream_offset + lead_bytes + effective_target_frame * avg_frame_size;
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, using audio_avg_frame_size=%d, lead_frames=%d, lead_bytes=%d, %d\n",
                       __func__, avg_frame_size, lead_frames, lead_bytes, __LINE__);
        }
        else
        {
            offset = priv->stream_offset + target_frame * frame_size;
        }
    }
    else
    {
        // Fallback to calculated frame size
        offset = priv->stream_offset + target_frame * frame_size;
    }

    // Ensure not exceeding file size
    if (total_bytes > 0 && offset >= total_bytes)
    {
        offset = (total_bytes > 1) ? (total_bytes - 1) : priv->stream_offset;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, CBR: target_time=%.3f, target_frame=%d, frame_size=%d, offset=%d, lead_frames=%d, lead_bytes=%d, %d\n",
               __func__, target_time_sec, target_frame, frame_size, offset, lead_frames, lead_bytes, __LINE__);

    return offset;
}

/**
 * @brief Calculate position for VBR AAC file
 * @param codec Audio codec context
 * @param priv AAC codec private data
 * @param second Target time in seconds
 * @return Calculated file offset
 */
static uint32_t calc_aac_position_vbr(bk_audio_player_decoder_t *codec, aac_decoder_priv_t *priv, double target_time_sec)
{
    int sample_rate = priv->aacFrameInfo.sampRateOut;
    uint32_t total_bytes = audio_source_get_total_bytes(codec->source);
    if (sample_rate == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid frame info, sample_rate=%d, %d\n",
                   __func__, sample_rate, __LINE__);
        return priv->stream_offset;
    }

    uint32_t audio_start = priv->stream_offset + priv->lead_bytes;

    // Estimate starting position if we have total frames and duration
    uint32_t start_offset = audio_start;
    if (priv->total_frames > 0 && priv->total_duration > 0)
    {
        uint32_t effective_frames = (priv->total_frames > priv->lead_frames) ? (priv->total_frames - priv->lead_frames) : priv->total_frames;

        // Estimate: calculate frame number based on time ratio
        double time_ratio = target_time_sec / (priv->total_duration / 1000.0);
        if (time_ratio > 1.0)
        {
            time_ratio = 1.0;
        }
        uint32_t estimated_frame = (uint32_t)(time_ratio * effective_frames);
        // Rough estimate offset (using average frame size)
        uint32_t audio_bytes = 0;
        if (priv->total_bytes > 0)
        {
            audio_bytes = (priv->total_bytes > priv->lead_bytes) ? (priv->total_bytes - priv->lead_bytes) : priv->total_bytes;
        }
        else if (total_bytes > audio_start)
        {
            audio_bytes = total_bytes - audio_start;
        }
        uint32_t avg_frame_size = (effective_frames > 0) ? (audio_bytes / effective_frames) : 0;
        if (avg_frame_size > 0)
        {
            start_offset = audio_start + estimated_frame * avg_frame_size;
            // Limit to valid range
            if (total_bytes > 0 && start_offset >= total_bytes)
            {
                start_offset = (total_bytes > 1) ? (total_bytes - 1) : audio_start;
            }
        }
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, VBR: estimated_frame=%d, start_offset=%d, %d\n",
                   __func__, estimated_frame, start_offset, __LINE__);
    }

    // Reset read buffer
    aac_reset_read_state(priv);

    // Find next frame header from estimated position
    uint32_t frame_start = find_next_frame_header(codec, priv, start_offset);
    if (frame_start == 0)
    {
        // If not found, try from audio_start
        frame_start = find_next_frame_header(codec, priv, audio_start);
        if (frame_start == 0)
        {
            frame_start = audio_start;
        }
    }

    // Seek to frame start
    if (audio_source_seek(codec->source, frame_start, SEEK_SET) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, seek to frame_start failed, %d\n", __func__, __LINE__);
        return priv->stream_offset;
    }

    aac_reset_read_state(priv);

    // Calculate initial time based on estimated frame position
    double current_time = 0.0;
    if (priv->total_frames > 0 && priv->total_duration > 0 && frame_start > audio_start)
    {
        // Estimate time at frame_start based on file position ratio
        uint32_t audio_bytes = (total_bytes > audio_start) ? (total_bytes - audio_start) : 0;
        if (audio_bytes > 0)
        {
            uint32_t offset_from_start = frame_start - audio_start;
            double position_ratio = (double)offset_from_start / (double)audio_bytes;
            current_time = position_ratio * (priv->total_duration / 1000.0);
            // Adjust: if we're before target, we need to go forward; if after, go backward
            if (current_time > target_time_sec)
            {
                // We're past target, need to go backward - reset to audio_start
                frame_start = audio_start;
                current_time = 0.0;
                if (audio_source_seek(codec->source, frame_start, SEEK_SET) != 0)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, seek to audio_start failed, %d\n", __func__, __LINE__);
                    return audio_start;
                }
                aac_reset_read_state(priv);
            }
        }
    }

    // Traverse frames until reaching target time
    uint32_t current_offset = frame_start;
    int frame_length = 0;
    int max_iterations = 10000;  // Prevent infinite loop
    int iteration = 0;

    while (current_time < target_time_sec && iteration < max_iterations)
    {
        iteration++;

        if (codec_aac_fill_buffer(codec) != 0)
        {
            break;  // End of file
        }

        if (priv->bytes_left < AAC_MAINBUF_SIZE)
        {
            break;
        }

        // Parse frame header
        frame_length = AACParseAdtsHeader(priv->decoder, (unsigned char *)priv->read_ptr, priv->bytes_left);
        // Skip very small frames (< 50 bytes) which are likely metadata/configuration frames
        // These small frames may cause decoder errors if sent to decoder
        if (frame_length <= 0 || frame_length < 50)
        {
            // Invalid frame header or too small, search forward
            priv->read_ptr++;
            priv->bytes_left--;
            if (priv->bytes_left == 0)
            {
                break;
            }
            continue;
        }

        // Get frame info
        AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);
        // Use core samples (1024) for time calculation, not outputSamps
        // outputSamps can be 2048 for SBR, but time calculation should use core frame size
        int frame_samples = 1024;  // AAC core frame always has 1024 samples
        int frame_sample_rate = priv->aacFrameInfo.sampRateOut;

        // Accumulate time
        if (frame_sample_rate > 0)
        {
            current_time += (double)frame_samples / frame_sample_rate;
        }

        // If reached target time, return current frame start position
        if (current_time >= target_time_sec)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, VBR: found target at offset=%d, time=%.3f, target=%.3f, %d\n",
                       __func__, current_offset, current_time, target_time_sec, __LINE__);
            return current_offset;
        }

        // Move to next frame
        if (frame_length <= priv->bytes_left)
        {
            priv->read_ptr += frame_length;
            priv->bytes_left -= frame_length;
            current_offset += frame_length;
        }
        else
        {
            if (audio_source_seek(codec->source, frame_length - priv->bytes_left, SEEK_CUR) != 0)
            {
                break;
            }
            current_offset += frame_length;
            aac_reset_read_state(priv);
        }

        // Check if exceeded file size
        if (total_bytes > 0 && current_offset >= total_bytes)
        {
            break;
        }
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, VBR: reached end, offset=%d, time=%.3f, target=%.3f, %d\n",
               __func__, current_offset, current_time, target_time_sec, __LINE__);

    return current_offset;
}

/**
 * @brief Calculate file offset for seek to specified time position
 * @param codec Audio codec context
 * @param second Target time in seconds
 * @return File offset in bytes, or -1 on error
 */
static int calc_aac_position(bk_audio_player_decoder_t *codec, int second)
{
    aac_decoder_priv_t *priv = (aac_decoder_priv_t *)codec->decoder_priv;
    uint32_t offset = 0;

    // Parameter validation
    if (second < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid second=%d, %d\n", __func__, second, __LINE__);
        return -1;
    }

    // Flush decoder state
    AACFlushCodec(priv->decoder);

    // Get necessary info
    int sample_rate = priv->aacFrameInfo.sampRateOut;
    // Note: samples_per_frame is always 1024 for AAC core frame (not outputSamps which can be 2048 for SBR)
    int samples_per_frame = 1024;
    double target_time_sec = (double)second;

    if (sample_rate == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid frame info, sample_rate=%d, %d\n",
                   __func__, sample_rate, __LINE__);
        return -1;
    }

    // Check if target time exceeds file duration
    double frame_duration = (double)samples_per_frame / (double)sample_rate;
    if (frame_duration <= 0.0)
    {
        frame_duration = 0.0;
    }

    if (priv->total_duration > 0)
    {
        double max_time = priv->total_duration / 1000.0;
        if (target_time_sec >= max_time)
        {
            double clamp_time = max_time - frame_duration;
            if (clamp_time < 0.0)
            {
                clamp_time = 0.0;
            }
            if (target_time_sec != clamp_time)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "%s, target time %d exceeds duration %.3f, clamp to %.3f, %d\n",
                           __func__, second, max_time, clamp_time, __LINE__);
            }
            target_time_sec = clamp_time;
        }
    }

    uint32_t target_frame = 0;
    if (target_time_sec > 0.0)
    {
        target_frame = (uint32_t)(target_time_sec * (double)sample_rate / (double)samples_per_frame);
    }

    // Select algorithm based on bitrate type
    uint32_t cbr_offset = 0;
    uint32_t precise_offset = 0;

    switch (priv->biterate_type)
    {
        case BITERATE_TYPE_CBR:
        case BITERATE_TYPE_CBR_INFO:
            cbr_offset = calc_aac_position_cbr(codec, priv, target_time_sec);
            offset = cbr_offset;

            // For early seek targets, fall back to precise traversal if deviation is large
            if (second <= AAC_SEEK_PRECISE_TIME_THRESHOLD)
            {
                precise_offset = calc_aac_position_vbr(codec, priv, target_time_sec);
                uint32_t deviation = (cbr_offset > precise_offset) ?
                                     (cbr_offset - precise_offset) : (precise_offset - cbr_offset);
                if (deviation > AAC_SEEK_DEVIATION_THRESHOLD)
                {
                    BK_LOGW(AUDIO_PLAYER_TAG, "%s, CBR offset %d deviates from precise offset %d by %d bytes, using precise result, %d\n",
                               __func__, cbr_offset, precise_offset, deviation, __LINE__);
                    offset = precise_offset;
                }
            }
            break;

        case BITERATE_TYPE_VBR_XING:
        case BITERATE_TYPE_VBR_VBRI:
        case BITERATE_TYPE_VBR_ADTS:
            offset = calc_aac_position_vbr(codec, priv, target_time_sec);
            break;

        default:
            // Default to CBR algorithm (assume CBR)
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, unknown bitrate type, use CBR algorithm, %d\n", __func__, __LINE__);
            offset = calc_aac_position_cbr(codec, priv, target_time_sec);
            break;
    }

    // Refine offset if we're still far from the target frame
    offset = refine_seek_offset(codec, priv, offset, target_frame);

    // Align to frame header
    if (offset > 0 && offset >= priv->stream_offset)
    {
        offset = align_to_frame_header(codec, priv, offset);
    }
    else
    {
        offset = priv->stream_offset;
    }

    // Reset decoder buffer so playback restarts cleanly from new offset
    aac_reset_read_state(priv);
    AACFlushCodec(priv->decoder);

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, final offset=%d for request=%d (effective %.3f s), %d\n",
               __func__, offset, second, target_time_sec, __LINE__);

    return (offset > 0) ? (int)offset : -1;
}

/* must sure the buf_size is enough */
static int parse_vbr_xing_header(aac_decoder_priv_t *codec_aac, char *xing_header_buf, int buf_size)
{
    /* check whether buffer size is enough */
    if (buf_size < 156)
    {
        return -1;
    }

    /* check header id of "XING" */
    if ((xing_header_buf[0] == 'x' || xing_header_buf[0] == 'X')
        && (xing_header_buf[1] == 'i' || xing_header_buf[1] == 'I')
        && (xing_header_buf[2] == 'n' || xing_header_buf[2] == 'N')
        && (xing_header_buf[3] == 'g' || xing_header_buf[3] == 'G')
       )
    {
        codec_aac->biterate_type = BITERATE_TYPE_VBR_XING;
    }
    else if ((xing_header_buf[0] == 'i' || xing_header_buf[0] == 'I')
             && (xing_header_buf[1] == 'n' || xing_header_buf[1] == 'N')
             && (xing_header_buf[2] == 'f' || xing_header_buf[2] == 'F')
             && (xing_header_buf[3] == 'o' || xing_header_buf[3] == 'O')
            )
    {
        codec_aac->biterate_type = BITERATE_TYPE_CBR_INFO;
    }
    else
    {
        return -1;
    }

    int offset = 8;

    unsigned char flags = xing_header_buf[7];
    if (flags & 0x01)   //Frames field is present
    {
        codec_aac->total_frames = *((int *)(xing_header_buf + offset));
        offset += 4;
    }

    if (flags & 0x02)   //Bytes field is present
    {
        codec_aac->total_bytes = *((int *)(xing_header_buf + offset));
        offset += 4;
    }

    return 0;
}

/* must sure the buf_size is enough */
static int parse_vbr_vbri_header(aac_decoder_priv_t *codec_aac, char *vbri_header_buf, int buf_size)
{
    /* check whether buffer size is enough */
    if (buf_size < 156)
    {
        return -1;
    }

    /* check header id of "VBRI" */
    if ((vbri_header_buf[0] == 'v' || vbri_header_buf[0] == 'V')
        && (vbri_header_buf[1] == 'b' || vbri_header_buf[1] == 'B')
        && (vbri_header_buf[2] == 'r' || vbri_header_buf[2] == 'R')
        && (vbri_header_buf[3] == 'i' || vbri_header_buf[3] == 'I')
       )
    {
        codec_aac->biterate_type = BITERATE_TYPE_VBR_VBRI;
    }
    else
    {
        return -1;
    }

    unsigned char *offset = (unsigned char *)(vbri_header_buf + 10);

    codec_aac->total_bytes = *((int *)(offset));
    offset += 4;

    codec_aac->total_frames = *((int *)(offset));

    return 0;
}

static int aac_decoder_open(audio_format_t format, void *param, bk_audio_player_decoder_t **decoder_pp)
{
    bk_audio_player_decoder_t *decoder;
    aac_decoder_priv_t *priv;

    if (format != AUDIO_FORMAT_AAC)
    {
        return AUDIO_PLAYER_INVALID;
    }

    decoder = audio_codec_new(sizeof(aac_decoder_priv_t));
    if (!decoder)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    priv = (aac_decoder_priv_t *)decoder->decoder_priv;

    os_memset(priv, 0x0, sizeof(aac_decoder_priv_t));

    /* init read session */
    priv->read_buffer = NULL;
    priv->read_ptr = NULL;
    priv->bytes_left = 0;
    priv->frames = 0;
    priv->stream_offset = 0;
    priv->current_sample_rate = 0;

    priv->read_buffer = player_malloc(AAC_AUDIO_BUF_SZ);
    if (priv->read_buffer == NULL)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "priv->read_buffer malloc failed!");
        return AUDIO_PLAYER_ERR;
    }

    /* set aac decoder use psram memory */
    //AACSetMemType(AAC_MEM_TYPE_PSRAM);
    priv->decoder = AACInitDecoder();
    if (!priv->decoder)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "priv->decoder create failed!");
        return AUDIO_PLAYER_ERR;
    }

    //BK_LOGI(AUDIO_PLAYER_TAG,"aac decoder ptr: %p \n", priv->decoder);

    *decoder_pp = decoder;

    return AUDIO_PLAYER_OK;
}


static int aac_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    int ec;
    aac_decoder_priv_t *priv;
    priv = (aac_decoder_priv_t *)decoder->decoder_priv;

    /* Set the default object type and samplerate */
    /* This is useful for RAW AAC files */
    AACFrameInfo aacFrameInfo = {0};
    os_memset(&aacFrameInfo, 0, sizeof(AACFrameInfo));
#if 0
    /* Set the default object type and samplerate */
    /* This is useful for RAW AAC files */
    aacFrameInfo.nChans = 2;
    aacFrameInfo.sampRateCore = 44100;
    aacFrameInfo.profile = AAC_PROFILE_LC;
    aacFrameInfo.bitsPerSample = 16;
    if (AACSetRawBlockParams(priv->decoder, 0, &aacFrameInfo) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, AACSetRawBlockParams fail, %d\n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }
    BK_LOGI(AUDIO_PLAYER_TAG, "AACSetRawBlockParams for RAW AAC type \n");
#endif

    /* number of output samples = 1024 per channel (2048 if SBR enabled) * 16bit * channels */
    short *sample_buffer = player_malloc(AAC_MAX_NSAMPS * 2 * 2 * AAC_MAX_NCHANS);
    if (!sample_buffer)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, malloc sample_buffer: %d fail, %d\n", __func__, AAC_MAX_NSAMPS * AAC_MAX_NCHANS * 2, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    /* Skip ID3v2 tag if not already done */
    if (priv->stream_offset == 0)
    {
        if (audio_source_seek(decoder->source, 0, SEEK_SET) == 0)
        {
            priv->stream_offset = codec_aac_skip_id3v2(decoder);
            if (priv->stream_offset < 0)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "%s, failed to skip ID3v2 tag, %d\n", __func__, __LINE__);
                priv->stream_offset = 0;
            }
        }
    }

    /* check whether the url is sdcard music.
        If true, calculate the average value of bitrate.
        If false, calculate bitrate according the first frame.
    */
    if (audio_source_get_total_bytes(decoder->source))
    {
        /* calculate bitrate */
        priv->bitRate = aac_adts_bitrate_calc(decoder);
        BK_LOGI(AUDIO_PLAYER_TAG, "priv->bitRate: %d\n", priv->bitRate);
        decoder->info.bps = priv->bitRate;
        /* After bitrate calculation, file position is at audio data start (after ID3v2) */
    }
    else
    {
        /* For streaming sources, ensure we're at the audio data start */
        if (priv->stream_offset > 0)
        {
            if (audio_source_seek(decoder->source, priv->stream_offset, SEEK_SET) != 0)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "%s, failed to seek to audio data start, %d\n", __func__, __LINE__);
            }
        }
    }

    aac_reset_read_state(priv);

__retry:
    if ((priv->read_ptr == NULL) || priv->bytes_left < AAC_MAINBUF_SIZE)
    {
        int fill_ret = codec_aac_fill_buffer(decoder);
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
            if (priv->bytes_left == 0)
            {
                return AUDIO_PLAYER_ERR;
            }
        }
    }

    /* Protect aac decoder to avoid decoding assert when data is insufficient. */
    if (priv->bytes_left < AAC_MAINBUF_SIZE)
    {
        if (priv->eos_reached)
        {
            if (priv->bytes_left == 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, no more AAC data can be decoded, %d\n", __func__, __LINE__);
                return AUDIO_PLAYER_ERR;
            }
        }
        else
        {
            rtos_delay_milliseconds(5);
            goto __retry;
        }
    }

    /* check whether mp3 file is VBR when the frame is first frame */
    if (priv->biterate_type == BITERATE_TYPE_UNKNOW)
    {
        if (0 == parse_vbr_xing_header(priv, (char *)(priv->read_ptr), (int)(priv->bytes_left)))
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "[XING] biterate_type:%d, total_bytes:%d, total_frames:%d \n", priv->biterate_type, priv->total_bytes, priv->total_frames);
        }
        else if (0 == parse_vbr_vbri_header(priv, (char *)(priv->read_ptr), (int)(priv->bytes_left)))
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "[VBRI] biterate_type:%d, total_bytes:%d, total_frames:%d \n", priv->biterate_type, priv->total_bytes, priv->total_frames);
        }
        else
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "[ADTS] no explicit VBR header detected, defer to frame analysis \n");
        }
    }

    if (priv->biterate_type == BITERATE_TYPE_UNKNOW)
    {
        aac_finalize_bitrate_type_from_stats(priv);
    }

    /* not used */
    int byte_left = priv->bytes_left;
    /* first decoder frame to get aacFrameInfo */
    ec = AACDecode(priv->decoder, &(priv->read_ptr), &byte_left, sample_buffer);
    if (ec == 0)
    {
        /* no error */
        AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);

        /* check whether the url is sdcard music.
            If true, calculate the average value of bitrate.
            If false, calculate bitrate according the first frame.
        */
        if (decoder->info.bps == 0)
        {
            /* calculate bitrate as CBR */
            decoder->info.bps = (uint64_t)(priv->bytes_left - byte_left) * 8 * priv->aacFrameInfo.sampRateOut / 1024;
            BK_LOGI(AUDIO_PLAYER_TAG, "bitRate: %d\n", decoder->info.bps);
        }

        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.bitRate: %d \n", priv->aacFrameInfo.bitRate);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.nChans: %d \n", priv->aacFrameInfo.nChans);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.sampRateCore: %d \n", priv->aacFrameInfo.sampRateCore);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.sampRateOut: %d \n", priv->aacFrameInfo.sampRateOut);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.bitsPerSample: %d \n", priv->aacFrameInfo.bitsPerSample);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.outputSamps: %d \n", priv->aacFrameInfo.outputSamps);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.profile: %d \n", priv->aacFrameInfo.profile);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.tnsUsed: %d \n", priv->aacFrameInfo.tnsUsed);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.pnsUsed: %d \n", priv->aacFrameInfo.pnsUsed);
    }
    else
    {
        if (ec == ERR_AAC_INDATA_UNDERFLOW)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode finish, ec: %d, %d\n", __func__, ec, __LINE__);
            goto __retry;
        }
        else
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode fail, ec: %d, %d\n", __func__, ec, __LINE__);
            return -1;
        }
    }

    priv->current_sample_rate = priv->aacFrameInfo.sampRateOut;

    /* calculate total duration of mp3 */
    switch (priv->biterate_type)
    {
        case BITERATE_TYPE_CBR:
        case BITERATE_TYPE_CBR_INFO:
            /* CBR: total_duration = total_bytes / bitrate * 8000 ms */
            //priv->total_duration = priv->total_bytes / priv->frame_info.bitrate * 8000;
            /* set codec info */
            decoder->info.total_bytes = priv->total_bytes;
            decoder->info.header_bytes = 0;
            /* need user calculate */
            decoder->info.duration = 0.0;
            BK_LOGI(AUDIO_PLAYER_TAG, "[CBR] total_bytes:%d \n", priv->total_bytes);
            break;

        case BITERATE_TYPE_VBR_XING:
        case BITERATE_TYPE_VBR_VBRI:
        case BITERATE_TYPE_VBR_ADTS:
            /* VBR: total_duration = total_bytes / bitrate * 8000 ms */
            {
                uint32_t effective_frames = (priv->total_frames > priv->lead_frames) ? (priv->total_frames - priv->lead_frames) : priv->total_frames;
                priv->total_duration = (double)effective_frames * 1024.0 * (1.0 / (double)priv->aacFrameInfo.sampRateOut) * 1000.0;
            }
            /* set codec info */
            decoder->info.duration = priv->total_duration;
            BK_LOGI(AUDIO_PLAYER_TAG, "[VBR] total_duration:%f, lead_frames=%d, lead_bytes=%d \n",
                       priv->total_duration, priv->lead_frames, priv->lead_bytes);
            break;

        default:
            break;
    }

    info->channel_number = priv->aacFrameInfo.nChans;
    info->sample_rate = priv->aacFrameInfo.sampRateOut;
    info->sample_bits = priv->aacFrameInfo.bitsPerSample;
    info->frame_size = 2 * priv->aacFrameInfo.outputSamps;
    info->bps = priv->aacFrameInfo.bitRate;

    /* update audio frame info */
    decoder->info.channel_number = priv->aacFrameInfo.nChans;
    decoder->info.sample_rate = priv->aacFrameInfo.sampRateOut;
    decoder->info.sample_bits = priv->aacFrameInfo.bitsPerSample;
    decoder->info.frame_size = 2 * priv->aacFrameInfo.outputSamps;
    /* uase the average value of bitrate for VBR  */
    //decoder->info.bps = priv->aacFrameInfo.bitRate;

    /* reset read_ptr to read_buffer, and decode first frame again */
    priv->read_ptr = priv->read_buffer;

    if (sample_buffer)
    {
        player_free(sample_buffer);
        sample_buffer = NULL;
    }

    return AUDIO_PLAYER_OK;
}


int aac_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    int32_t ec;

    aac_decoder_priv_t *priv;
    priv = (aac_decoder_priv_t *)decoder->decoder_priv;

__retry:

    while (((priv->read_ptr == NULL) || priv->bytes_left < AAC_MAINBUF_SIZE) && !priv->eos_reached)
    {
        int fill_ret = codec_aac_fill_buffer(decoder);
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

    /* Protect aac decoder to avoid decoding assert when data is insufficient. */
    if (priv->bytes_left < AAC_MAINBUF_SIZE)
    {
        if (priv->eos_reached)
        {
            if (priv->bytes_left == 0)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "%s, AAC source end reached\n", __func__);
                return 0;
            }
            if (priv->stream_aborted)
            {
                /* Aborted stream: residual is likely a truncated frame, drop it instead
                 * of feeding a partial frame into the decoder. */
                BK_LOGW(AUDIO_PLAYER_TAG, "%s, aborted, drop tail: %d < %d\n", __func__, priv->bytes_left, AAC_MAINBUF_SIZE);
                priv->bytes_left = 0;
                return 0;
            }
            /* clean end of stream: fall through and decode the remaining whole frame(s) */
        }
        else
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, wait for more data, read: %d < %d, %d \n", __func__, priv->bytes_left, AAC_MAINBUF_SIZE, __LINE__);
            return AUDIO_PLAYER_TIMEOUT;
        }
    }

    //    uint32_t start_size = priv->bytes_left;
    ec = AACDecode(priv->decoder, &priv->read_ptr, (int *)&priv->bytes_left, (short *)buffer);
    if (ec == 0)
    {
        AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);
#if 0
        if (priv->aac_decoder_num_channels == 2)
        {
            os_memcpy((uint8_t *)buffer, priv->aac_decoder_pcm_buffer, priv->aac_decoder_pcm_samples * 2 * sizeof(uint16_t));
        }
        else
        {
            int i;
            int16_t *src, *dst;

            os_memcpy(buffer + priv->aac_decoder_pcm_samples * sizeof(uint16_t),
                      priv->aac_decoder_pcm_buffer,
                      priv->aac_decoder_pcm_samples * sizeof(uint16_t));

            // convert to two channel
            src = (int16_t *)(buffer + priv->aac_decoder_pcm_samples * sizeof(uint16_t));
            dst = (int16_t *)(buffer);
            for (i = 0; i < priv->aac_decoder_pcm_samples; i++)
            {
                dst[2 * i] = src[i];
                dst[2 * i + 1] = src[i];
            }
        }
#endif

#if 0
        //BK_LOGI(AUDIO_PLAYER_TAG, "priv->bytes_left: %d, byte_left: %d \n", priv->bytes_left, byte_left);
        priv->aacFrameInfo.bitRate = (uint64_t)(start_size - priv->bytes_left) * 8 * priv->aacFrameInfo.sampRateOut / 1024;

        /* check whether VBR for debug*/
        if (codec->info.bps != priv->aacFrameInfo.bitRate)
        {
            //            BK_LOGI(AUDIO_PLAYER_TAG, "[VBR] new bps:%d, old bps:%d\n", priv->aacFrameInfo.bitRate, codec->info.bps);
            codec->info.bps = priv->aacFrameInfo.bitRate;
        }
#endif

        /* check whether sample rate change */
        if (priv->aacFrameInfo.sampRateOut != priv->current_sample_rate)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "================== Frame_info Change Notion =================\n");
            BK_LOGI(AUDIO_PLAYER_TAG, "new frame_info=>> bitrate:%d, nChans:%d, samprate:%d, bitsPerSample:%d, outputSamps:%d\n", priv->aacFrameInfo.bitRate, priv->aacFrameInfo.nChans, priv->aacFrameInfo.sampRateOut, priv->aacFrameInfo.bitsPerSample, priv->aacFrameInfo.outputSamps);
            BK_LOGI(AUDIO_PLAYER_TAG, "new samprate:%d, current_sample_rate:%d\n", priv->aacFrameInfo.sampRateOut, priv->current_sample_rate);
            BK_LOGI(AUDIO_PLAYER_TAG, "\n");

            /* update audio frame info */
            decoder->info.channel_number = priv->aacFrameInfo.nChans;
            decoder->info.sample_rate = priv->aacFrameInfo.sampRateOut;
            decoder->info.sample_bits = priv->aacFrameInfo.bitsPerSample;
            decoder->info.frame_size = 2 * priv->aacFrameInfo.outputSamps;
            decoder->info.bps = priv->aacFrameInfo.bitRate;

            priv->current_sample_rate = priv->aacFrameInfo.sampRateOut;
        }

        return priv->aacFrameInfo.outputSamps * 2;
    }
    else
    {
        if (ec == ERR_AAC_INDATA_UNDERFLOW)
        {
            if (priv->eos_reached)
            {
                return 0;
            }
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode need more data, ec: %d, %d\n", __func__, ec, __LINE__);
            goto __retry;
        }

        BK_LOGE(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode fail, ec: %d, %d\n", __func__, ec, __LINE__);
        return -1;
    }

    return 0;
}

static int aac_decoder_close(bk_audio_player_decoder_t *decoder)
{
    aac_decoder_priv_t *priv;
    priv = (aac_decoder_priv_t *)decoder->decoder_priv;

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, %d\n", __func__, __LINE__);

    if (priv && priv->decoder)
    {
        AACFreeDecoder(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->read_buffer)
    {
        player_free(priv->read_buffer);
        priv->read_buffer = NULL;
    }

    return AUDIO_PLAYER_OK;
}


static int aac_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    aac_decoder_priv_t *priv;
    priv = (aac_decoder_priv_t *)decoder->decoder_priv;
    return 2 * priv->aacFrameInfo.outputSamps;
}


const bk_audio_player_decoder_ops_t aac_decoder_ops =
{
    .name = "aac",
    .open = aac_decoder_open,
    .get_info = aac_decoder_get_info,
    .get_chunk_size = aac_decoder_get_chunk_size,
    .get_data = aac_decoder_get_data,
    .close = aac_decoder_close,
    .calc_position = calc_aac_position,
    .is_seek_ready = NULL,
};

/* Get AAC decoder operations structure */
const bk_audio_player_decoder_ops_t *bk_audio_player_get_aac_decoder_ops(void)
{
    return &aac_decoder_ops;
}
