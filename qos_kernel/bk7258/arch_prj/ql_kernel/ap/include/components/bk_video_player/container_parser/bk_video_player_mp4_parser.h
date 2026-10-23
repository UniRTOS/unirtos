#ifndef BK_VIDEO_PLAYER_MP4_PARSER_H
#define BK_VIDEO_PLAYER_MP4_PARSER_H

#include "components/bk_video_player/bk_video_player_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Get MP4 container parser operations
// This function returns a pointer to the MP4 container parser operations structure
// The parser context is allocated automatically on first call
video_player_container_parser_ops_t *bk_video_player_get_mp4_parser_ops(void);

#ifdef __cplusplus
}
#endif

#endif // BK_VIDEO_PLAYER_MP4_PARSER_H

