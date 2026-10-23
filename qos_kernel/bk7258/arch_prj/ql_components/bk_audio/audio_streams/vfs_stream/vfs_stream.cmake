set(incs .)

set(srcs)

if (CONFIG_ADK_VFS_STREAM)
list(APPEND srcs
		vfs_stream.c
	)

if (CONFIG_ADK_VFS_STREAM_TEST)
	list(APPEND srcs
		vfs_stream_test.c
	)
endif()
endif()#(CONFIG_ADK_VFS_STREAM)

set(VFS_STREAM_SRCS)
foreach(source ${srcs})
    get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND VFS_STREAM_SRCS "${source_abs}")
endforeach()

set(VFS_STREAM_INCS)
foreach(include_dir ${incs})
    get_filename_component(include_abs "${include_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    list(APPEND VFS_STREAM_INCS "${include_abs}")
endforeach()

if(srcs)
    add_component_static_lib(vfs_stream
        SRCS ${VFS_STREAM_SRCS}
        INCS ${VFS_STREAM_INCS}
    )
endif()
