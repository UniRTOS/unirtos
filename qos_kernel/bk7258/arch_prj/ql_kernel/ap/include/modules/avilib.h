#ifndef AVILIB_H
#define AVILIB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Public AVI handle with media-related parameters only.
// Internal parse/encode/index/cache/segment state is private and not exposed.
typedef struct
{
    // Video media parameters
    long   width;             /* Width of video frame */
    long   height;            /* Height of video frame */
    double fps;               /* Frames per second */
    char   compressor[8];     /* FourCC compressor (4 bytes) + padding + '\\0' */
    long   video_frames;      /* Total video frames (best-effort for writer, exact for reader when indexed) */

    // Audio media parameters
    long   a_fmt;             /* Audio format tag (WAVEFORMATEX.wFormatTag) */
    long   a_chans;           /* Audio channels */
    long   a_rate;            /* Audio sample rate in Hz */
    long   a_bits;            /* PCM bits per sample (0 for packetized audio such as AAC) */
    long   a_byterate;        /* Avg bytes per second (best-effort) */
    long   audio_bytes;       /* Total audio bytes (best-effort) */
} avi_t;

#define AVI_MODE_WRITE  0
#define AVI_MODE_READ   1

// Memory type for buffer allocation
#define AVI_MEM_SRAM    0  // Use SRAM for buffer allocation
#define AVI_MEM_PSRAM   1  // Use PSRAM for buffer allocation

/* The error codes delivered by avi_open_input_file */

#define AVI_ERR_SIZELIM      1     /* The write of the data would exceed
                                      the maximum size of the AVI file.
                                      This is more a warning than an error
                                      since the file may be closed safely */

#define AVI_ERR_OPEN         2     /* Error opening the AVI file - wrong path
                                      name or file nor readable/writable */

#define AVI_ERR_READ         3     /* Error reading from AVI File */

#define AVI_ERR_WRITE        4     /* Error writing to AVI File,
                                      disk full ??? */

#define AVI_ERR_WRITE_INDEX  5     /* Could not write index to AVI file
                                      during close, file may still be
                                      usable */

#define AVI_ERR_CLOSE        6     /* Could not write header to AVI file
                                      or not truncate the file during close,
                                      file is most probably corrupted */

#define AVI_ERR_NOT_PERM     7     /* Operation not permitted:
                                      trying to read from a file open
                                      for writing or vice versa */

#define AVI_ERR_NO_MEM       8     /* malloc failed */

#define AVI_ERR_NO_AVI       9     /* Not an AVI file */

#define AVI_ERR_NO_HDRL     10     /* AVI file has no has no header list,
                                      corrupted ??? */

#define AVI_ERR_NO_MOVI     11     /* AVI file has no has no MOVI list,
                                      corrupted ??? */

#define AVI_ERR_NO_VIDS     12     /* AVI file contains no video data */

#define AVI_ERR_NO_IDX      13     /* The file has been opened with
                                      getIndex==0, but an operation has been
                                      performed that needs an index */

/* Possible Audio formats */
#define WAVE_FORMAT_UNKNOWN             (0x0000)
#define WAVE_FORMAT_PCM                 (0x0001)
#define WAVE_FORMAT_ADPCM               (0x0002)
#define WAVE_FORMAT_IBM_CVSD            (0x0005)
#define WAVE_FORMAT_ALAW                (0x0006)
#define WAVE_FORMAT_MULAW               (0x0007)
#define WAVE_FORMAT_OKI_ADPCM           (0x0010)
#define WAVE_FORMAT_DVI_ADPCM           (0x0011)
#define WAVE_FORMAT_DIGISTD             (0x0015)
#define WAVE_FORMAT_DIGIFIX             (0x0016)
#define WAVE_FORMAT_YAMAHA_ADPCM        (0x0020)
#define WAVE_FORMAT_DSP_TRUESPEECH      (0x0022)
#define WAVE_FORMAT_GSM610              (0x0031)
#define WAVE_FORMAT_MPEGLAYER3          (0x0055)
#define WAVE_FORMAT_G722                (0x0065)
#define IBM_FORMAT_MULAW                (0x0101)
#define IBM_FORMAT_ALAW                 (0x0102)
#define IBM_FORMAT_ADPCM                (0x0103)
#define WAVE_FORMAT_AAC                 (0x00FF)

//write mode functions
void AVI_open_output_file(avi_t **avi_p, char *filename, int mem_type);

// Set the maximum AVI file length (in bytes) for write mode.
// Default is 2,000,000,000 bytes.
int AVI_set_max_file_size(avi_t *AVI, uint64_t max_len);

void AVI_set_video(avi_t *AVI, int width, int height, double fps, char *compressor);
void AVI_set_audio(avi_t *AVI, int channels, long rate, int bits, int format);

int AVI_write_frame(avi_t *AVI, char *data, long bytes);
int AVI_write_audio(avi_t *AVI, char *data, long bytes);

long AVI_bytes_remain(avi_t *AVI);

void AVI_update_video_frame_rate_by_duration(avi_t *AVI, uint32_t duration_ms);
int AVI_close(avi_t *AVI);
//end of write mode functions

//read mode functions
avi_t *AVI_open_input_file(const char *filename, int getIndex, int mem_type);

//video info functions
long AVI_video_frames(avi_t *AVI);
int AVI_video_width(avi_t *AVI);
int AVI_video_height(avi_t *AVI);
double AVI_video_frame_rate(avi_t *AVI);
char *AVI_video_compressor(avi_t *AVI);

//audio info functions
int AVI_audio_channels(avi_t *AVI);
int AVI_audio_bits(avi_t *AVI);
int AVI_audio_format(avi_t *AVI);
long AVI_audio_rate(avi_t *AVI);
long AVI_audio_byterate(avi_t *AVI);
long AVI_audio_bytes(avi_t *AVI);
long AVI_audio_chunks(avi_t *AVI);

//seek functions
int AVI_set_video_read_index(avi_t *AVI, long frame, long *frame_len);
int AVI_set_audio_read_chunk(avi_t *AVI, long chunk, long *chunk_len);

//read functions
long AVI_read_next_video_frame(avi_t *AVI, char *vidbuf, long max_bytes);
long AVI_read_next_audio_chunk(avi_t *AVI, char *audbuf, long max_bytes);

int AVI_audio_byte_offset_of_chunk(avi_t *AVI, long chunk, uint64_t *byte_off);

int AVI_get_aac_stream_info(avi_t *AVI, int *out_is_adts, const uint8_t **out_asc, uint32_t *out_asc_size);

int AVI_find_prev_video_keyframe(avi_t *AVI, long frame, long *out_keyframe);


#ifdef __cplusplus
}
#endif
#endif
