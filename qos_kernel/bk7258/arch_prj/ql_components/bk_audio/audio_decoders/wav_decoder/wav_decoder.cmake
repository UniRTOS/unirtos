set(incs .)

set(srcs)

if (CONFIG_ADK_WAV_DECODER)

list(APPEND srcs
		wav_decoder.c
	)

if (CONFIG_ADK_WAV_DECODER_TEST)
	list(APPEND srcs
		wav_decoder_test.c
	)
endif()

endif()

set(WAV_DECODER_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND WAV_DECODER_SRCS "${source_abs}")
endforeach()

set(WAV_DECODER_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND WAV_DECODER_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(wav_decoder
        SRCS ${WAV_DECODER_SRCS}
        INCS ${WAV_DECODER_INCS}
    )
endif()
