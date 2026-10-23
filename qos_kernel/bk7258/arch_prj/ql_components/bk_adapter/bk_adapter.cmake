set(incs include include/bk_private) #The lib_adapter.h is for armino internal libs only
set(srcs lib_adapter.c)
set(BK_ADAPTER_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND BK_ADAPTER_SRCS "${source_abs}")
endforeach()

set(BK_ADAPTER_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND BK_ADAPTER_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(bk_adapter
        SRCS ${BK_ADAPTER_SRCS}
        INCS ${BK_ADAPTER_INCS}
    )
endif()
