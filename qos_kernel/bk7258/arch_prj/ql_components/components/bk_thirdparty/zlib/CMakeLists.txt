if(UNIRTOS_APP_BUILD)
    set(build_path ${CMAKE_CURRENT_BINARY_DIR}/zlib_v1_3_1)
else()
    set(build_path ${CMAKE_BINARY_DIR}/armino/zlib/zlib_v1_3_1)
endif()

set(incs zlib_v1_3_1 ${build_path})

set(srcs)

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}")



if(CONFIG_ZLIB_ENABLE)
    add_subdirectory(zlib_v1_3_1)
    if(UNIRTOS_APP_BUILD)
        target_link_libraries(${COMPONENT_LIB} INTERFACE zlibstatic)
    else()
        add_library(lib_zlib STATIC IMPORTED GLOBAL)
        set_target_properties(lib_zlib PROPERTIES IMPORTED_LOCATION ${build_path}/libzlibstatic.a)
        add_dependencies(${COMPONENT_LIB} lib_zlib)
        target_link_libraries(${COMPONENT_LIB} INTERFACE lib_zlib)
    endif()
endif()

