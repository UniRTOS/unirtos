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

set(I2S_STREAM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND I2S_STREAM_SRCS "${source_abs}")
endforeach()

set(I2S_STREAM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND I2S_STREAM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(i2s_stream
        SRCS ${I2S_STREAM_SRCS}
        INCS ${I2S_STREAM_INCS}
    )
endif()
