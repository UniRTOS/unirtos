#pragma once

#include "components/bk_video_player/bk_video_player_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Get AAC audio decoder operations for registration.
// Note: The returned ops is a static template. Do not modify it.
const video_player_audio_decoder_ops_t *bk_video_player_get_aac_audio_decoder_ops(void);

#ifdef __cplusplus
}
#endif


