set(incs include)
set(srcs "")

if (CONFIG_VIDEO_RECORDER)
    list(APPEND srcs
        src/bk_video_recorder.c
        src/bk_video_recorder_ctlr.c
        src/bk_video_recorder_avi.c
        src/bk_video_recorder_mp4.c
    )
endif()

armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES bk_common avdk_utils bk_vfs
)


