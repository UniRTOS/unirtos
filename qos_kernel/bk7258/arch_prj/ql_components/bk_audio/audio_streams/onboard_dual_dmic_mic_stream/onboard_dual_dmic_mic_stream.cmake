set(incs .)

set(srcs)

if (CONFIG_ADK_ONBOARD_DUAL_DMIC_MIC_STREAM)

list(APPEND srcs
		onboard_dual_dmic_mic_stream.c
	)

endif()

set(ONBOARD_DUAL_DMIC_MIC_STREAM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND ONBOARD_DUAL_DMIC_MIC_STREAM_SRCS "${source_abs}")
endforeach()

set(ONBOARD_DUAL_DMIC_MIC_STREAM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND ONBOARD_DUAL_DMIC_MIC_STREAM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(onboard_dual_dmic_mic_stream
        SRCS ${ONBOARD_DUAL_DMIC_MIC_STREAM_SRCS}
        INCS ${ONBOARD_DUAL_DMIC_MIC_STREAM_INCS}
    )
endif()
