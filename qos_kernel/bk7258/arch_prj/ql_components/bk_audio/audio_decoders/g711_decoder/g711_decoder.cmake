set(incs .)

set(srcs)

if (CONFIG_ADK_G711_DECODER)

list(APPEND srcs
		g711_decoder.c
	)

if (CONFIG_ADK_G711_DECODER_TEST)
	list(APPEND srcs
		g711_decoder_test.c
	)
endif()

endif()

set(G711_DECODER_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND G711_DECODER_SRCS "${source_abs}")
endforeach()

set(G711_DECODER_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND G711_DECODER_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(g711_decoder
        SRCS ${G711_DECODER_SRCS}
        INCS ${G711_DECODER_INCS}
    )
endif()
