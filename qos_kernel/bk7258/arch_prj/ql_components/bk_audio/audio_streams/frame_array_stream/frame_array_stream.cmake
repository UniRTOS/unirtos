set(incs .)

set(srcs)

if (CONFIG_ADK_FRAME_ARRAY_STREAM)
list(APPEND srcs
		frame_array_stream.c
	)

if (CONFIG_ADK_FRAME_ARRAY_STREAM_TEST)
	list(APPEND srcs
		frame_array_stream_test.c
	)
endif()
endif()#(CONFIG_ADK_FRAME_ARRAY_STREAM)

set(FRAME_ARRAY_STREAM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND FRAME_ARRAY_STREAM_SRCS "${source_abs}")
endforeach()

set(FRAME_ARRAY_STREAM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND FRAME_ARRAY_STREAM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(frame_array_stream
        SRCS ${FRAME_ARRAY_STREAM_SRCS}
        INCS ${FRAME_ARRAY_STREAM_INCS}
    )
endif()
