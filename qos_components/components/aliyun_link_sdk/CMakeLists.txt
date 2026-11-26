message(STATUS "cmake config ${CMAKE_CURRENT_SOURCE_DIR}")

set(target aliyun_link_sdk)
add_components_libraries($<TARGET_FILE:${target}>)
add_library(${target} STATIC)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_unir_lib_dir})

file(GLOB_RECURSE aliyun_link_sdk_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

list(FILTER aliyun_link_sdk_sources EXCLUDE REGEX "/demos/")
list(FILTER aliyun_link_sdk_sources EXCLUDE REGEX "/external/mbedtls/")


set(aliyun_link_sdk_public_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/components/bootstrap
    ${CMAKE_CURRENT_SOURCE_DIR}/components/compress
    ${CMAKE_CURRENT_SOURCE_DIR}/components/data-model
    ${CMAKE_CURRENT_SOURCE_DIR}/components/diag
    ${CMAKE_CURRENT_SOURCE_DIR}/components/dynreg
    ${CMAKE_CURRENT_SOURCE_DIR}/components/dynreg-mqtt
    ${CMAKE_CURRENT_SOURCE_DIR}/components/logpost
    ${CMAKE_CURRENT_SOURCE_DIR}/components/mqtt_upload
    ${CMAKE_CURRENT_SOURCE_DIR}/components/ntp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/ota
    ${CMAKE_CURRENT_SOURCE_DIR}/components/task
)

target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR})
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/libdeflate)
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/libdeflate/lib)
# if(NOT CONFIG_QCM_MBEDTLS_LIBRARY_FUNC AND NOT FEATURE_MBEDTLS_ENABLE)
#    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/mbedtls/include)
# endif()
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/portfiles/aiot_port)
target_include_directories(${target} PUBLIC ${aliyun_link_sdk_public_include_dirs})
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/sysdep)
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/utils)

target_sources(${target} PRIVATE ${aliyun_link_sdk_sources})

target_link_libraries_if(CONFIG_QOSA_LINK_SYSTEM_LIBRARIES_FUNC THEN ${target} system_os)
target_link_libraries_if(CONFIG_QCM_MBEDTLS_LIBRARY_FUNC OR CONFIG_QOSA_MBEDTLS_LIBRARY_FUNC THEN ${target} mbedtls)
