#pragma once

#include "components/bk_video_player/bk_video_player_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Get G711 audio decoder operations for registration (G711A/G711U).
const video_player_audio_decoder_ops_t *bk_video_player_get_g711_audio_decoder_ops(void);

#ifdef __cplusplus
}
#endif

