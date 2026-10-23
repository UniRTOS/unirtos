set(incs .)

set(srcs)

if (CONFIG_ADK_AAC_DECODER)

list(APPEND srcs
		aac_decoder.c
	)

if (CONFIG_ADK_AAC_DECODER_TEST)
	list(APPEND srcs
		aac_decoder_test.c
	)
endif()

endif()

set(ACC_DECODER_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND ACC_DECODER_SRCS "${source_abs}")
endforeach()

set(ACC_DECODER_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND ACC_DECODER_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(acc_decoder
        SRCS ${ACC_DECODER_SRCS}
        INCS ${ACC_DECODER_INCS}
    )
endif()
