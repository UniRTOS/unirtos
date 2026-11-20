set(incs)

if (CONFIG_WANSON_ARMINO_ASR)

	list(APPEND incs
		include
		bk7258
	)

if (CONFIG_WANSON_ASR_WORDS_VERSION_V1)
	list(APPEND incs
		bk7258/words_version_v1
	)
endif()

if (CONFIG_WANSON_ASR_WORDS_VERSION_V2)
	list(APPEND incs
		bk7258/words_version_v2
	)
endif()

if (CONFIG_WANSON_ASR_GROUP_VERSION_WORDS_V1)
	list(APPEND incs
		bk7258/group_version_words_v1/
	)
endif()

if (CONFIG_WANSON_ASR_GROUP_VERSION_WORDS_V2)
	list(APPEND incs
		bk7258/group_version_words_v2/
	)
endif()

endif()	#(CONFIG_WANSON_ARMINO_ASR)

if(CONFIG_WANSON_CN_LICENSE)
	list(APPEND incs
		include
		bk7258
	)
endif()

armino_component_register(INCLUDE_DIRS "${incs}")

if (CONFIG_WANSON_ARMINO_ASR)

if (CONFIG_WANSON_ASR_GROUP_VERSION)
if (CONFIG_WANSON_CN_LICENSE)
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrfst_with_auth.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrfst_with_auth.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
else()
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrfst.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrfst.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()
else()
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrbase.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrbase.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()

endif()

if(CONFIG_WANSON_CN_LICENSE)
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/license/libwanson_license.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/license/libwanson_license.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()

