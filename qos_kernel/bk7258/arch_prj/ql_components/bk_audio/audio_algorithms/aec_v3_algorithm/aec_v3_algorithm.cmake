set(incs .)

set(srcs)

if (CONFIG_ADK_AEC_V3_ALGORITHM)

list(APPEND srcs
		aec_v3_algorithm.c
	)


endif()

set(AEC_V3_ALGORITHM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AEC_V3_ALGORITHM_SRCS "${source_abs}")
endforeach()

set(AEC_V3_ALGORITHM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AEC_V3_ALGORITHM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(aec_v3_algorithm
        SRCS ${AEC_V3_ALGORITHM_SRCS}
        INCS ${AEC_V3_ALGORITHM_INCS}
    )
endif()
