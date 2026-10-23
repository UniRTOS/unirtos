set(srcs "")

if (CONFIG_AUD_PARAM_CTRL)
	list(APPEND srcs
        audio_param_ctrl.c
        audio_dbgtool_intf.c
	)
endif()

set(BK_AUD_PARAM_CTRL_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND BK_AUD_PARAM_CTRL_SRCS "${source_abs}")
endforeach()

set(BK_AUD_PARAM_CTRL_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND BK_AUD_PARAM_CTRL_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(bk_aud_param_ctrl
        SRCS ${BK_AUD_PARAM_CTRL_SRCS}
        INCS ${BK_AUD_PARAM_CTRL_INCS}
    )
endif()
