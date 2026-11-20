set(incs .)

set(srcs)

if (CONFIG_ADK_RSP_ALGORITHM)

list(APPEND srcs
		rsp_algorithm.c
	)

if (CONFIG_ADK_RSP_ALGORITHM_TEST)
	list(APPEND srcs
		rsp_algorithm_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)