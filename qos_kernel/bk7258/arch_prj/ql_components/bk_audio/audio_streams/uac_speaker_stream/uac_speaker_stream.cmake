set(incs .)

set(srcs)

if (CONFIG_ADK_UAC_SPEAKER_STREAM)

list(APPEND srcs
		uac_speaker_stream.c
	)

if (CONFIG_ADK_UAC_SPEAKER_STREAM_TEST)
	list(APPEND srcs
		uac_speaker_stream_test.c
	)
endif()

endif()

set(UAC_SPEAKER_STREAM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND UAC_SPEAKER_STREAM_SRCS "${source_abs}")
endforeach()

set(UAC_SPEAKER_STREAM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND UAC_SPEAKER_STREAM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(uac_speaker_stream
        SRCS ${UAC_SPEAKER_STREAM_SRCS}
        INCS ${UAC_SPEAKER_STREAM_INCS}
    )
endif()
