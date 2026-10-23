set(incs .)

set(srcs)

if (CONFIG_ADK_MP3_DECODER)

list(APPEND srcs
		mp3_decoder.c
	)

if (CONFIG_ADK_MP3_DECODER_TEST)
	list(APPEND srcs
		mp3_decoder_test.c
	)
endif()

endif()

set(MP3_DECODER_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND MP3_DECODER_SRCS "${source_abs}")
endforeach()

set(MP3_DECODER_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND MP3_DECODER_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(mp3_decoder
        SRCS ${MP3_DECODER_SRCS}
        INCS ${MP3_DECODER_INCS}
    )
endif()
