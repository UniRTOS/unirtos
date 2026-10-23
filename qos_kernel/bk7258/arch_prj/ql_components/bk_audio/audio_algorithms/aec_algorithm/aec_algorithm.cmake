set(incs .)

set(srcs)

if (CONFIG_ADK_AEC_ALGORITHM)

list(APPEND srcs
		aec_algorithm.c
	)

if (CONFIG_ADK_AEC_ALGORITHM_TEST)
	list(APPEND srcs
		aec_algorithm_test.c
	)
endif()

endif()

set(AEC_ALGORITHM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AEC_ALGORITHM_SRCS "${source_abs}")
endforeach()

set(AEC_ALGORITHM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AEC_ALGORITHM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(aec_algorithm
        SRCS ${AEC_ALGORITHM_SRCS}
        INCS ${AEC_ALGORITHM_INCS}
    )
endif()
