set(incs .)

set(srcs)

if (CONFIG_ADK_AGC_ALGORITHM)

list(APPEND srcs
		agc_algorithm.c
	)

if (CONFIG_ADK_AGC_ALGORITHM_TEST)
	list(APPEND srcs
		agc_algorithm_test.c
	)
endif()

endif()

set(AGC_ALGORITHM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AGC_ALGORITHM_SRCS "${source_abs}")
endforeach()

set(AGC_ALGORITHM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AGC_ALGORITHM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(agc_algorithm
        SRCS ${AGC_ALGORITHM_SRCS}
        INCS ${AGC_ALGORITHM_INCS}
    )
endif()
