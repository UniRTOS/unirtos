set(incs .)

set(srcs)

if (CONFIG_ADK_RAW_STREAM)

list(APPEND srcs
		raw_stream.c
	)

if (CONFIG_ADK_RAW_STREAM_TEST)
	list(APPEND srcs
		raw_stream_test.c
	)
endif()

endif()

set(RAW_STREAM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND RAW_STREAM_SRCS "${source_abs}")
endforeach()

set(RAW_STREAM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND RAW_STREAM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(raw_stream
        SRCS ${RAW_STREAM_SRCS}
        INCS ${RAW_STREAM_INCS}
    )
endif()
