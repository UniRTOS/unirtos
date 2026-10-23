set(incs .)

set(srcs)

if (CONFIG_ADK_EQ_ALGORITHM)

list(APPEND srcs
		eq_algorithm.c
	)

if (CONFIG_ADK_EQ_ALGORITHM_TEST)
	list(APPEND srcs
		eq_algorithm_test.c
	)
endif()

endif()

set(EQ_ALGORITHM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND EQ_ALGORITHM_SRCS "${source_abs}")
endforeach()

set(EQ_ALGORITHM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND EQ_ALGORITHM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(eq_algorithm
        SRCS ${EQ_ALGORITHM_SRCS}
        INCS ${EQ_ALGORITHM_INCS}
    )
endif()
