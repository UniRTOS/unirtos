set(incs .)

set(srcs)

if (CONFIG_ADK_SBC_ENCODER)

list(APPEND srcs
		sbc_enc.c
	)

if (CONFIG_ADK_SBC_ENCODER_TEST)
	list(APPEND srcs
		sbc_encoder_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)