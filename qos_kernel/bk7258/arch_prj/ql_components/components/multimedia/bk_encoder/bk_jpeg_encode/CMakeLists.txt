set(incs include)
set(srcs)

if (CONFIG_JPEG_ENCODER)
	list(APPEND srcs
		src/bk_jpeg_encoder.c
		src/bk_jpeg_encode_ctlr.c
	)
endif()

set(priv_req avdk_utils)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
