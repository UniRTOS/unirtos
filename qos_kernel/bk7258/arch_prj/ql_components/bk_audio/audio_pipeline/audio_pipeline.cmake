set(incs ./include)

set(srcs)

if (CONFIG_ADK)
list(APPEND srcs
		audio_element.c
		audio_event_iface.c
		audio_pipeline.c
		ringbuf.c
		framebuf.c
		audio_mem.c
		audio_thread.c
		audio_port.c
		cb_port.c
		rb_port.c
		fb_port.c
		audio_port_info_list.c
	)

if (CONFIG_ADK_AUTO_TEST_PIPELINE)
	list(APPEND srcs
		auto_test/audio_pipeline_test.c
	)
endif()

if (CONFIG_ADK_AUTO_TEST_ELEMENT)
	list(APPEND srcs
		auto_test/audio_element_test.c
	)
endif()

if (CONFIG_ADK_AUTO_TEST_EVENT)
	list(APPEND srcs
		auto_test/audio_event_iface_test.c
	)
endif()
endif()

set(AUDIO_PIPELINE_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AUDIO_PIPELINE_SRCS "${source_abs}")
endforeach()

set(AUDIO_PIPELINE_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND AUDIO_PIPELINE_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(audio_pipeline
        SRCS ${AUDIO_PIPELINE_SRCS}
        INCS ${AUDIO_PIPELINE_INCS}
    )
endif()
