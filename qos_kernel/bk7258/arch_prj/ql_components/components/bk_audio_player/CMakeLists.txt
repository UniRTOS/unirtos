set(incs . include plugins/sources/hls)
set(srcs "")
set(plugins "")

list(APPEND plugins
	"plugins/decoders/bk_audio_player_wav_decoder.c"
	"plugins/decoders/bk_audio_player_aac_decoder.c"
	"plugins/decoders/bk_audio_player_m4a_decoder.c"
	"plugins/decoders/bk_audio_player_ts_decoder.c"
	"plugins/decoders/bk_audio_player_amr_decoder.c"
	"plugins/decoders/bk_audio_player_opus_decoder.c"
	"plugins/decoders/bk_audio_player_opus_stream_decoder.c"
	"plugins/decoders/bk_audio_player_vorbis_stream_decoder.c"
	"plugins/decoders/bk_audio_player_mp3_decoder.c"
	"plugins/decoders/bk_audio_player_flac_decoder.c"
	"plugins/decoders/bk_audio_player_ogg_decoder.c"
	"plugins/decoders/frame_info_list.c"
	"plugins/metadata_parsers/bk_audio_player_metadata_parser_common.c"
	"plugins/metadata_parsers/bk_audio_player_mp3_metadata_parser.c"
	"plugins/metadata_parsers/bk_audio_player_wav_metadata_parser.c"
	"plugins/metadata_parsers/bk_audio_player_aac_metadata_parser.c"
	"plugins/metadata_parsers/bk_audio_player_amr_metadata_parser.c"
	"plugins/metadata_parsers/bk_audio_player_m4a_metadata_parser.c"
	"plugins/metadata_parsers/bk_audio_player_opus_metadata_parser.c"
	"plugins/metadata_parsers/bk_audio_player_ogg_metadata_parser.c"
	"plugins/metadata_parsers/bk_audio_player_flac_metadata_parser.c"
	)

# Optional plugins selected by dependent features
if (CONFIG_VFS)
	list(APPEND plugins
		"plugins/sources/bk_audio_player_file_source.c"
		"plugins/sinks/bk_audio_player_file_sink.c")
endif()

if (CONFIG_WEBCLIENT)
	list(APPEND plugins
		"plugins/sources/bk_audio_player_net_source.c"
		"plugins/sources/hls/bk_audio_player_hls_source.c"
		"plugins/sources/hls/m3u8_work.c")
endif()

if (CONFIG_ADK_RAW_STREAM AND CONFIG_ADK_ONBOARD_SPEAKER_STREAM)
	list(APPEND plugins
		"plugins/sinks/bk_audio_player_onboard_speaker_sink.c")
endif()

if(CONFIG_AUDIO_PLAYER)
	list(APPEND incs .)
	list(APPEND srcs
		"core/play_manager.c"
		"core/play_sm.c"
		"core/plugin_manager.c"
		"core/music_list.c"
		"core/ring_buffer.c"
		"core/linked_list.c"
		"core/player_osal.c"
		"core/source.c"
		"core/codec.c"
		"core/sink.c"
		"core/player_mem.c"
		"core/audio_metadata.c"
		${plugins})
endif()

armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES bk_common include bk_vfs lwip_intf_v2_1 webclient psa_mbedtls
		)
