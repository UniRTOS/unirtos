set(incs ./include)

set(srcs)

if (CONFIG_ADK_UART_UTIL)
	list(APPEND srcs
		uart_util.c
	)
endif()

if (CONFIG_ADK_COUNT_UTIL)
	list(APPEND srcs
		count_util.c
	)
endif()

if (CONFIG_ADK_VFS_UTIL)
	list(APPEND srcs
		vfs_util.c
	)
endif()

if (CONFIG_ADK_DEBUG_DUMP_UTIL)
	list(APPEND srcs
		debug_dump_util.c
		debug_dump_cli.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common bk_vfs)
