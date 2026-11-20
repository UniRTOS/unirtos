set(incs .)

set(srcs)

if (CONFIG_ADK_MIX_ALGORITHM)

list(APPEND srcs
		mix_algorithm.c
	)

if (CONFIG_ADK_MIX_ALGORITHM_TEST)
	list(APPEND srcs
		mix_algorithm_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)