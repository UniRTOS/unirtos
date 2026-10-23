message(STATUS "cmake config ${CMAKE_CURRENT_SOURCE_DIR}")

set(target qcm_llm_proxy)
add_components_libraries($<TARGET_FILE:${target}>)
add_library(${target} STATIC)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_unir_lib_dir})
target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR})
target_include_directories(${target} PRIVATE ${SOURCE_SYS_DEBUG_DIR})
target_include_directories(${target} PRIVATE ${SOURCE_SYS_OS_DIR})
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/backends)
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/media)
target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR}/components/include)

target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/unirtos/generated)

if (CONFIG_QOSA_BK7258_PLATFORM_FUNC)	
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_kernel/ap/config)
    target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_kernel/ap/include)	
    target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf_v2_1/lwip-2.1.2/src)
    target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf_v2_1/lwip-2.1.2/src/include)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf_v2_1/lwip-2.1.2/port)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf_v2_1/lwip-2.1.2/src/include/netif)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf_v2_1/lwip-2.1.2/src/include/lwip)

    target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/psa_mbedtls/mbedtls_port/inc)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/psa_mbedtls/mbedtls_port/mbedtls/include)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/psa_mbedtls/mbedtls_port/mbedtls/library)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/psa_mbedtls/mbedtls_port/configs)

	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/psa_mbedtls/mbedtls/include)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/psa_mbedtls/mbedtls/library)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/psa_mbedtls/mbedtls/tests/src)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/psa_mbedtls/mbedtls/tests/include)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/cJSON)
endif()

if (CONFIG_QOSA_BK7238_PLATFORM_FUNC)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf/dhcpd)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf/lwip-2.0.2/src)
    target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf/lwip-2.0.2/src/include)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf/lwip-2.0.2/port)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf/lwip-2.0.2/port/arch)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf/lwip-2.0.2/src/include/netif)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip_intf/lwip-2.0.2/src/include/lwip)

    target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/mbedtls/mbedtls-2.27.0/include)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/mbedtls/mbedtls-2.27.0/include/mbedtls)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/mbedtls/mbedtls-2.27.0/library)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/mbedtls/mbedtls_port/inc)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/mbedtls/mbedtls_ui)
endif()

if (CONFIG_QOSA_ECR6600_PLATFORM_FUNC)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip/contrib/port)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip/contrib/port/arch)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip/contrib/port/netif)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip/lwip-2.1.0/src)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip/lwip-2.1.0/src/apps)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/lwip/lwip-2.1.0/src/include)

	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/mbedtls/mbedtls/include)
	target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/mbedtls/port/include/mbedtls)
endif()

target_add_sources(${target} PRIVATE
    qcm_llm_api.c
	qcm_llm_runtime.c
	qcm_llm_transport.c
	backends/qcm_llm_backend_anthropic.c
	backends/qcm_llm_backend_custom.c
	backends/qcm_llm_backend_openai_compatible.c
	media/qcm_media_pipeline.c
 )

target_link_libraries_if(CONFIG_QOSA_LINK_SYSTEM_LIBRARIES_FUNC THEN ${target} system_os)
target_link_libraries_if(CONFIG_QCM_MONGOOS_LIB THEN ${target} mongoose-7.10)

