set(incs .)

set(srcs)

if (CONFIG_ADK_I2S_STREAM)

list(APPEND srcs
		i2s_stream.c
	)

if (CONFIG_ADK_I2S_STREAM_TEST)
	list(APPEND srcs
		i2s_stream_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

