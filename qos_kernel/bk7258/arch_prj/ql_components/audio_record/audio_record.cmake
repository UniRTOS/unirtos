set(incs include)
set(srcs "")

if (CONFIG_AUDIO_RECORD)
	list(APPEND srcs
		src/audio_record.c
		src/onboard_mic_record.c
		src/ring_buffer.c
	)
endif()

set(AUDIO_RECORD_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AUDIO_RECORD_SRCS "${source_abs}")
endforeach()

set(AUDIO_RECORD_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AUDIO_RECORD_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(audio_record
        SRCS ${AUDIO_RECORD_SRCS}
        INCS ${AUDIO_RECORD_INCS}
    )
endif()
