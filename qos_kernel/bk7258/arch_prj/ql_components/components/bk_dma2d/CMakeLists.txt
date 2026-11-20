set(incs include include/bk_private)
set(srcs)

list(APPEND incs
	.
	)

if (CONFIG_MEDIA_DAM2D)

list(APPEND srcs
	src/bk_dma2d_ctlr.c
	src/bk_dma2d.c
)
endif()

set(priv_req bk_common bk_dma2d)



armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}" multimedia  avdk_utils
)

