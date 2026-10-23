set(incs .)

set(srcs)

if (CONFIG_ADK_OPUS_DECODER)

list(APPEND srcs
		opus_dec.c
	)

if (CONFIG_ADK_OPUS_DECODER_TEST)
	list(APPEND srcs
		opus_dec_test.c
	)
endif()

endif()

set(OPUS_DEC_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND OPUS_DEC_SRCS "${source_abs}")
endforeach()

set(OPUS_DEC_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND OPUS_DEC_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(opus_dec
        SRCS ${OPUS_DEC_SRCS}
        INCS ${OPUS_DEC_INCS}
    )
endif()
