set(incs .)

set(srcs)

if (CONFIG_ADK_SBC_ENCODER)

list(APPEND srcs
		sbc_enc.c
	)

if (CONFIG_ADK_SBC_ENCODER_TEST)
	list(APPEND srcs
		sbc_encoder_test.c
	)
endif()

endif()

set(SBC_ENCODER_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND SBC_ENCODER_SRCS "${source_abs}")
endforeach()

set(SBC_ENCODER_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND SBC_ENCODER_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(sbc_encoder
        SRCS ${SBC_ENCODER_SRCS}
        INCS ${SBC_ENCODER_INCS}
    )
endif()
