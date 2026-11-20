#ifndef BK_VIDEO_PLAYER_JPEG_PROBE_H
#define BK_VIDEO_PLAYER_JPEG_PROBE_H

#include <stdint.h>

#include "components/avdk_utils/avdk_types.h"
#include "components/bk_video_player/bk_video_player_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parse JPEG header and report subsampling format.
//
// This helper is intentionally container-agnostic:
// the caller is responsible for reading the first JPEG frame bytes from AVI/MP4.
//
// Return AVDK_ERR_OK on success.
avdk_err_t bk_video_player_probe_jpeg_subsampling(const uint8_t *jpeg_buf,
                                                  uint32_t jpeg_len,
                                                  video_player_jpeg_subsampling_t *out_subsampling);

#ifdef __cplusplus
}
#endif

#endif // BK_VIDEO_PLAYER_JPEG_PROBE_H

