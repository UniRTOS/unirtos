set(incs include)
set(srcs "")

if (CONFIG_AUDIO_PLAY)
	list(APPEND srcs
		src/audio_play.c
		src/onboard_speaker_play.c
		src/ring_buffer.c
	)
endif()

set(AUDIO_PLAY_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AUDIO_PLAY_SRCS "${source_abs}")
endforeach()

set(AUDIO_PLAY_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AUDIO_PLAY_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(audio_play
        SRCS ${AUDIO_PLAY_SRCS}
        INCS ${AUDIO_PLAY_INCS}
    )
endif()
