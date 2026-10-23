set(incs include)
set(srcs "")

if (CONFIG_ASR_SERVICE)
	list(APPEND srcs
		src/bk_asr.c
		src/bk_asr_service.c
	)

if (CONFIG_WANSON_ASR OR CONFIG_WANSON_ARMINO_ASR)
    set(incs wanson)
	list(APPEND incs
		wanson
	)
	list(APPEND srcs
		wanson/bk_wanson_asr_intf.c
	)
endif()
endif()

set(BK_ASR_SERVICE_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND BK_ASR_SERVICE_SRCS "${source_abs}")
endforeach()

set(BK_ASR_SERVICE_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND BK_ASR_SERVICE_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(bk_asr_service
        SRCS ${BK_ASR_SERVICE_SRCS}
        INCS ${BK_ASR_SERVICE_INCS}
    )
endif()
