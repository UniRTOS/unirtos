if (CONFIG_MEDIA)
set(incs include
		pipeline
	)

set(srcs
		comm/frame_buffer.c
	)

if (CONFIG_MEDIA_PIPELINE)
	list(APPEND srcs
		pipeline/jpeg_decode_pipeline.c
		pipeline/lcd_scale_pipeline.c
		pipeline/yuv_rotate_pipeline.c
		pipeline/uvc_pipeline_act.c
		pipeline/jpeg_decode_list.c
		pipeline/jpeg_get_pipeline.c
	)

if (CONFIG_H264)
	list(APPEND srcs
		pipeline/h264_encode_pipeline.c
	)
endif()

endif() #CONFIG_MEDIA_PIPELINE

endif() #CONFIG_MEDIA

list(APPEND srcs core/media_mem_share.c)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_cli lvgl unity media_utils bk_display avdk_utils)
armino_component_register_call_subdirs()