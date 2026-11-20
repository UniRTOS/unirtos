set(incs include)
set(srcs "")

if (CONFIG_ASR_SERVICE)
	list(APPEND srcs
		src/bk_asr.c
		src/bk_asr_service.c
	)

if (CONFIG_WANSON_ASR OR CONFIG_WANSON_ARMINO_ASR)
    set(incs wanson)
	list(APPEND incs
		wanson
	)
	list(APPEND srcs
		wanson/bk_wanson_asr_intf.c
	)
endif()
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES avdk_utils bk_common include wanson wanson_armino
)
