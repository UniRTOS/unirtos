message(STATUS "cmake config ${CMAKE_CURRENT_SOURCE_DIR}")

set(target agora_rtsa_sdk)
add_components_libraries($<TARGET_FILE:${target}>)
add_library(${target} STATIC)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_unir_lib_dir})

# AOSL runtime, platform porting layer and mbedtls crypto wrapper.
file(GLOB_RECURSE agora_rtsa_sdk_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

if(NOT agora_rtsa_sdk_sources)
    message(FATAL_ERROR "agora_rtsa_sdk: no source files found under ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# Prebuilt Agora core archive ships with the component; file name is not hard-coded.
file(GLOB_RECURSE agora_rtsa_sdk_prebuilt_libs CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.a"
)

if(NOT agora_rtsa_sdk_prebuilt_libs)
    message(WARNING "agora_rtsa_sdk: no prebuilt archive (*.a) found under ${CMAKE_CURRENT_SOURCE_DIR}")
else()
    add_components_libraries(${agora_rtsa_sdk_prebuilt_libs})
endif()

target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR})

# Public: Agora RTC API consumed by the applications and demos.
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/agora_sdk/inc)

# Private: AOSL runtime and mbedtls porting internals.
target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/agora_sdk/porting/mbedtls_wrapper/include
    ${CMAKE_CURRENT_SOURCE_DIR}/aosl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/aosl/include/api
    ${CMAKE_CURRENT_SOURCE_DIR}/aosl/include/kernel
    ${CMAKE_CURRENT_SOURCE_DIR}/aosl/include/kernel/byteorder
    ${CMAKE_CURRENT_SOURCE_DIR}/aosl/platform/include
    ${CMAKE_CURRENT_SOURCE_DIR}/aosl/platform/include/hal
    ${CMAKE_CURRENT_SOURCE_DIR}/aosl/platform/src/config
    ${CMAKE_CURRENT_SOURCE_DIR}/aosl/platform/src/config/hal
)

target_sources(${target} PRIVATE ${agora_rtsa_sdk_sources})

target_compile_definitions(${target} PRIVATE
    RTSA_CRYPTO_WRAPPER_ENABLE_CIPHER
    RTSA_CRYPTO_WRAPPER_ENABLE_MD
    RTSA_CRYPTO_WRAPPER_ENABLE_PBKDF2
)

target_link_libraries_if(CONFIG_QOSA_LINK_SYSTEM_LIBRARIES_FUNC THEN ${target} system_os)
target_link_libraries_if(CONFIG_QCM_MBEDTLS_LIBRARY_FUNC OR CONFIG_QOSA_MBEDTLS_LIBRARY_FUNC THEN ${target} mbedtls)
