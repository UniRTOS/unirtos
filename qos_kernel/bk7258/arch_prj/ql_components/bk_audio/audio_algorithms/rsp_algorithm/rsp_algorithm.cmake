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

set(RSP_ALGORITHM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND RSP_ALGORITHM_SRCS "${source_abs}")
endforeach()

set(RSP_ALGORITHM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND RSP_ALGORITHM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(rsp_algorithm
        SRCS ${RSP_ALGORITHM_SRCS}
        INCS ${RSP_ALGORITHM_INCS}
    )
endif()
