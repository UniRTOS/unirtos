set(incs include include/bk_private)

set(srcs)


if (CONFIG_MEDIA_OSD)
list(APPEND srcs
    "src/bk_draw_icon.c"
    "src/bk_draw_icon_ctlr.c"
    "src/bk_draw_osd.c"
    "src/bk_draw_osd_ctlr.c"
)
endif()

set(priv_req bk_common)

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}" multimedia  avdk_utils
)






