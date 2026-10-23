set(incs .)

set(srcs)

if (CONFIG_ADK_UART_STREAM)

list(APPEND srcs
		uart_stream.c
	)

if (CONFIG_ADK_UART_STREAM_TEST)
	list(APPEND srcs
		uart_stream_test.c
	)
endif()

endif()

set(UART_STREAM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND UART_STREAM_SRCS "${source_abs}")
endforeach()

set(UART_STREAM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND UART_STREAM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(uart_stream
        SRCS ${UART_STREAM_SRCS}
        INCS ${UART_STREAM_INCS}
    )
endif()
