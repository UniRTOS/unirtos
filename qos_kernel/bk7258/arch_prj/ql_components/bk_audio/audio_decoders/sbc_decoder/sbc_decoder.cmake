set(incs .)

set(srcs)

if (CONFIG_ADK_SBC_DECODER)

list(APPEND srcs
		sbc_decoder.c
	)

if (CONFIG_ADK_SBC_DECODER_TEST)
	list(APPEND srcs
		sbc_decoder_test.c
	)
endif()

endif()

set(SBC_DECODER_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND SBC_DECODER_SRCS "${source_abs}")
endforeach()

set(SBC_DECODER_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND SBC_DECODER_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(sbc_decoder
        SRCS ${SBC_DECODER_SRCS}
        INCS ${SBC_DECODER_INCS}
    )
endif()
