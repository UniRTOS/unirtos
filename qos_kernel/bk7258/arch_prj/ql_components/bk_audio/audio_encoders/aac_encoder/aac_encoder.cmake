set(incs .)

set(srcs)

if (CONFIG_ADK_AAC_ENCODER)

list(APPEND srcs
		aac_encoder.c
	)

if (CONFIG_ADK_AAC_ENCODER_TEST)
	list(APPEND srcs
		aac_encoder_test.c
	)
endif()

endif()

set(AAC_ENCODER_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AAC_ENCODER_SRCS "${source_abs}")
endforeach()

set(AAC_ENCODER_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AAC_ENCODER_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(aac_encoder
        SRCS ${AAC_ENCODER_SRCS}
        INCS ${AAC_ENCODER_INCS}
    )
endif()
