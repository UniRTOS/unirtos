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

set(AUDIO_UTILS_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AUDIO_UTILS_SRCS "${source_abs}")
endforeach()

set(AUDIO_UTILS_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AUDIO_UTILS_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(audio_utils
        SRCS ${AUDIO_UTILS_SRCS}
        INCS ${AUDIO_UTILS_INCS}
    )
endif()
