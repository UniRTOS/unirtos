set(incs include)
set(srcs
)

if (CONFIG_VIDEO_PIPELINE)

    list(APPEND srcs
		src/bk_video_pipeline_ctlr.c
		bk_video_pipeline.c
	)

endif()

set(priv_req avdk_utils multimedia)


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
