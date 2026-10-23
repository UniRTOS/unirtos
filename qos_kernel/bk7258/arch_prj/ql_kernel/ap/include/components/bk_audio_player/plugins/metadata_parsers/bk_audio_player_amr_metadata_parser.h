#ifndef __BK_AUDIO_PLAYER_AMR_METADATA_PARSER_H__
#define __BK_AUDIO_PLAYER_AMR_METADATA_PARSER_H__

#include <components/bk_audio_player/bk_audio_player_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get AMR metadata parser operations structure */
const bk_audio_player_metadata_parser_ops_t *bk_audio_player_get_amr_metadata_parser_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* __BK_AUDIO_PLAYER_AMR_METADATA_PARSER_H__ */
