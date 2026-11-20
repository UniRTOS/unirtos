set(incs include decode legacy/include modules/container_parser modules/video_decoder modules/audio_decoder)
set(srcs "")

if (CONFIG_AVI_PLAYER)
    # Legacy AVI player sources depend on AVI_PLAYER Kconfig symbols.
    # Build them only when AVI_PLAYER is enabled to avoid undefined CONFIG_* macros.
    list(APPEND srcs
        legacy/src/avi_player_jpeg_decode.c
        legacy/src/avi_player.c
    )

    list(APPEND srcs
        src/bk_video_player_ctlr.c
        src/bk_video_player_engine.c
        src/bk_video_player_playlist.c
        common/bk_video_player_buffer_pool.c
        common/bk_video_player_file_list.c
        common/bk_video_player_pipeline.c
        src/bk_video_player_container_parse.c
        src/bk_video_player_audio_decode.c
        src/bk_video_player_video_decode.c
    )

    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_AVI_PARSER)
        list(APPEND srcs modules/container_parser/bk_video_player_avi_parser.c)
    endif()
    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_MP4_PARSER)
        list(APPEND srcs modules/container_parser/bk_video_player_mp4_parser.c)
    endif()
    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_AVI_PARSER OR CONFIG_BK_VIDEO_PLAYER_ENABLE_MP4_PARSER)
        list(APPEND srcs modules/container_parser/bk_video_player_jpeg_probe.c)
    endif()

    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_JPEG_DECODER)
        list(APPEND srcs modules/video_decoder/bk_video_player_hw_jpeg_decoder.c)
    endif()
    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_SW_JPEG_DECODER)
        list(APPEND srcs modules/video_decoder/bk_video_player_sw_jpeg_decoder.c)
    endif()
    list(APPEND srcs modules/video_decoder/bk_video_player_virtual_h264_decoder.c)

    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_AAC_DECODER)
        list(APPEND srcs modules/audio_decoder/bk_video_player_aac_decoder.c)
    endif()

    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_G711_DECODER)
        list(APPEND srcs modules/audio_decoder/bk_video_player_g711_decoder.c)
    endif()

    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_G722_DECODER)
        list(APPEND srcs modules/audio_decoder/bk_video_player_g722_decoder.c)
    endif()

endif()

armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES bk_common include multimedia avdk_utils lvgl bk_jpeg_decoder bk_vfs bk_dma2d
)

