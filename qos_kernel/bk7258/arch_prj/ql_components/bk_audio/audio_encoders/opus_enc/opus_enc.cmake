set(incs .)

set(srcs)

if (CONFIG_ADK_OPUS_ENCODER)

list(APPEND srcs
		opus_enc.c
	)

if (CONFIG_ADK_OPUS_ENCODER_TEST)
	list(APPEND srcs
		opus_enc_test.c
	)
endif()

endif()

set(OPUS_ENC_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND OPUS_ENC_SRCS "${source_abs}")
endforeach()

set(OPUS_ENC_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND OPUS_ENC_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(opus_enc
        SRCS ${OPUS_ENC_SRCS}
        INCS ${OPUS_ENC_INCS}
    )
endif()
