set(incs .)

set(srcs)

if (CONFIG_ADK_G711_ENCODER)

list(APPEND srcs
		g711_encoder.c
	)

if (CONFIG_ADK_G711_ENCODER_TEST)
	list(APPEND srcs
		g711_encoder_test.c
	)
endif()

endif()

set(G711_ENCODER_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND G711_ENCODER_SRCS "${source_abs}")
endforeach()

set(G711_ENCODER_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND G711_ENCODER_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(g711_encoder
        SRCS ${G711_ENCODER_SRCS}
        INCS ${G711_ENCODER_INCS}
    )
endif()
