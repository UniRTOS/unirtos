message(STATUS "cmake config ${CMAKE_CURRENT_SOURCE_DIR}")

set(target agora-rtc-agent)
add_components_libraries($<TARGET_FILE:${target}>)
add_library(${target} STATIC)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_unir_lib_dir})
target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR})
target_include_directories(${target} PRIVATE ${SOURCE_SYS_DEBUG_DIR})
target_include_directories(${target} PRIVATE ${SOURCE_SYS_OS_DIR})
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)
target_include_directories(${target} PRIVATE ${UNIRTOS_COMPONENTS_DIR}/include)
target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/unirtos/generated)


target_add_sources(${target} PRIVATE
    agora_http.c
	agora_rtc.c
	agora_token.c
	agora_rtc_agent.c
 )

target_link_libraries_if(CONFIG_QOSA_LINK_SYSTEM_LIBRARIES_FUNC THEN ${target} system_os)
target_link_libraries_if(CONFIG_QCM_MONGOOS_LIB THEN ${target} mongoos-7.10)

