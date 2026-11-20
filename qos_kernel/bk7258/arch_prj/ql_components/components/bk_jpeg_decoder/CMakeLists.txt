set(incs include)
set(srcs
)

if (CONFIG_JPEG_DECODER)

	list(APPEND srcs
		src/jpeg_decode_utils.c
	)
if (CONFIG_JPEG_DECODER_HW)
	list(APPEND srcs
		src/bk_jpeg_decode_hw.c
		src/hw/bk_hw_jpeg_decode_ctlr.c
		src/hw/hw_jpeg_decode.c
		src/hw/hw_jpeg_decode_opt.c
		src/hw/bk_hw_jpeg_decode_opt_ctlr.c
	)
endif()


if (CONFIG_JPEG_DECODER_SW)
    list(APPEND srcs
		src/bk_jpeg_decode_sw.c
		src/sw/bk_sw_jpeg_decode_ctlr.c
		src/sw/sw_jpeg_decode_cp1.c
		src/sw/sw_jpeg_decode_cp2.c
		src/sw/sw_jpeg_decode_dual_core.c
		src/sw/bk_sw_jpeg_decode_on_multi_core_ctlr.c
	)
endif()

endif()

set(priv_req avdk_utils multimedia)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
