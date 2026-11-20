set(incs .)

set(srcs)

if (CONFIG_ADK_SBC_DECODER)

list(APPEND srcs
		sbc_decoder.c
	)

if (CONFIG_ADK_SBC_DECODER_TEST)
	list(APPEND srcs
		sbc_decoder_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)