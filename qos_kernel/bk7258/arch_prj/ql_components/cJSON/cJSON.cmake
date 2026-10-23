set(incs .)
set(srcs)
if (CONFIG_CJSON_USE)
list(APPEND srcs cJSON.c
)
endif()
set(CJSON_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND CJSON_SRCS "${source_abs}")
endforeach()

set(CJSON_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND CJSON_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(cJSON
        SRCS ${CJSON_SRCS}
        INCS ${CJSON_INCS}
    )
endif()
