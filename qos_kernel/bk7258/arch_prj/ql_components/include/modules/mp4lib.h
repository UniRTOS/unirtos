#ifndef MP4LIB_H
#define MP4LIB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// MP4 error codes
#define MP4_ERR_OK           0
#define MP4_ERR_OPEN         1
#define MP4_ERR_READ         2
#define MP4_ERR_WRITE        3
#define MP4_ERR_NO_MEM       4
#define MP4_ERR_NO_MP4       5
#define MP4_ERR_INVALID      6
#define MP4_ERR_NOT_PERM     7
#define MP4_ERR_CLOSE        8
#define MP4_ERR_SIZELIM      9

// MP4 file mode
#define MP4_MODE_READ        0
#define MP4_MODE_WRITE       1

// Memory type for buffer allocation
#define MP4_MEM_SRAM         0  // Use SRAM for buffer allocation
#define MP4_MEM_PSRAM        1  // Use PSRAM for buffer allocation

// Track types
#define MP4_TRACK_TYPE_VIDEO 0x76696465  // 'vide'
#define MP4_TRACK_TYPE_AUDIO 0x736F756E  // 'soun'

// Video codec types
#define MP4_CODEC_H264       0x61766331  // 'avc1'
#define MP4_CODEC_MJPEG      0x6A706567  // 'jpeg' (MJPEG)

// Audio codec types
#define MP4_CODEC_AAC        0x6D703461  // 'mp4a'
#define MP4_CODEC_MP3        0x6D703320  // 'mp3 '
#define MP4_CODEC_PCM_SOWT   0x736F7774  // 'sowt' - Little-endian signed 16-bit PCM
#define MP4_CODEC_PCM_TWOS   0x74776F73  // 'twos' - Big-endian signed 16-bit PCM
#define MP4_CODEC_G722       0x67373232  // 'g722' - G.722
#define MP4_CODEC_ALAW       0x616C6177  // 'alaw' - G.711 A-law
#define MP4_CODEC_ULAW       0x756C6177  // 'ulaw' - G.711 mu-law

// MP4 file structure
typedef struct
{
    // NOTE:
    // - `mp4_t` is a public handle type. Keep it minimal and stable.
    // - All MP4 container internals (file positions, tracks, index cache, codec-specific state)
    //   are stored in an internal private structure and must NOT be accessed by users.

    // MP4 file mode (MP4_MODE_READ / MP4_MODE_WRITE)
    uint32_t mode;

    // Memory type for buffer allocation (MP4_MEM_SRAM / MP4_MEM_PSRAM)
    int mem_type;
} mp4_t;

// Write mode functions
void MP4_open_output_file(mp4_t **mp4_p, const char *filename, int mem_type);
// Open MP4 output file with index cache buffer.
//
// Why:
// - Long recordings require an index (sample table) to be built for moov/stbl boxes.
// - Writing one index record per frame directly to filesystem causes excessive small writes.
// - This API lets upper layer provide a RAM buffer to batch index records and flush them
//   to the index file in large chunks (minimize write次数).
//
// Notes:
// - `index_cache_buf` must be writable memory. It is used as a staging buffer for index records.
// - `index_cache_size` should be large enough to reduce flush frequency.
// - When the cache is nearly full, mp4lib will flush the whole chunk with ONE write call.
void MP4_open_output_file_with_index_cache(mp4_t **mp4_p,
                                           const char *filename,
                                           void *index_cache_buf,
                                           uint32_t index_cache_size,
                                           int mem_type);
// Set the maximum MP4 file length (in bytes) for write mode.
// Default is 2,000,000,000 bytes.
int MP4_set_max_file_size(mp4_t *mp4, uint64_t max_len);
void MP4_set_video(mp4_t *mp4, uint32_t width, uint32_t height, double fps, uint32_t codec_type);
int MP4_set_audio(mp4_t *mp4, uint16_t channels, uint32_t sample_rate, uint16_t sample_size, uint32_t codec_type);
int MP4_write_frame(mp4_t *mp4, const uint8_t *data, uint32_t bytes);
int MP4_write_audio(mp4_t *mp4, const uint8_t *data, uint32_t bytes);
void MP4_update_video_frame_rate_by_duration(mp4_t *mp4, uint32_t duration_ms);
int MP4_close(mp4_t *mp4);
// end of write mode functions


// Read mode functions
mp4_t *MP4_open_input_file(const char *filename, int getIndex, int mem_type);
int MP4_close_input_file(mp4_t *mp4);

// Video info functions
uint32_t MP4_video_frames(mp4_t *mp4);
uint32_t MP4_video_width(mp4_t *mp4);
uint32_t MP4_video_height(mp4_t *mp4);
double MP4_video_frame_rate(mp4_t *mp4);
uint32_t MP4_video_codec(mp4_t *mp4);

// Audio info functions
uint16_t MP4_audio_channels(mp4_t *mp4);
uint16_t MP4_audio_bits(mp4_t *mp4);
uint32_t MP4_audio_format(mp4_t *mp4);
uint32_t MP4_audio_rate(mp4_t *mp4);
uint64_t MP4_audio_bytes(mp4_t *mp4);
uint32_t MP4_audio_samples(mp4_t *mp4);
uint32_t MP4_audio_timescale(mp4_t *mp4);
uint32_t MP4_audio_duration(mp4_t *mp4);

const uint8_t *MP4_audio_codec_config(mp4_t *mp4, uint32_t *out_size);
const uint8_t *MP4_video_codec_config(mp4_t *mp4, uint32_t *out_size);

// Index availability for seek/read (read mode)
bool MP4_has_index(mp4_t *mp4);

// Seek functions
int MP4_set_video_read_index(mp4_t *mp4, uint32_t sample, uint32_t *sample_size);
int MP4_set_audio_read_index(mp4_t *mp4, uint32_t sample, uint32_t *sample_size);

// Read functions
uint32_t MP4_read_next_video_sample(mp4_t *mp4, uint8_t *vidbuf, uint32_t max_bytes);
uint32_t MP4_read_next_audio_sample(mp4_t *mp4, uint8_t *audbuf, uint32_t max_bytes);

// Read-mode helpers (mainly for segmented sample tables)
// Return the nearest previous sync sample (keyframe) index for H.264 seek-to-I.
uint32_t MP4_find_prev_video_sync_sample(mp4_t *mp4, uint32_t sample);

#ifdef __cplusplus
}
#endif
#endif // MP4LIB_H

