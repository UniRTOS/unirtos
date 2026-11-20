set(incs include)
set(srcs "")

if (CONFIG_MEDIA_DISPLAY_SERVICE)
	list(APPEND srcs
		src/rgb/bk_display_rgb_ctlr.c
		src/mcu/bk_display_mcu_ctlr.c
		src/qspi/bk_display_qspi_ctlr.c
		src/spi/bk_display_spi_ctlr.c
		src/dual_qspi/bk_display_dual_qspi_ctlr.c
		src/dual_spi/bk_display_dual_spi_ctlr.c
		src/bk_display.c
	)
endif()

set(priv_req bk_common unity include)

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}" multimedia  avdk_utils
)

