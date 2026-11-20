set(incs)
set(srcs)

if (CONFIG_VOLC_RTC_EN) 
list(APPEND incs
            .
            include
            # hal/configs
            hal/inc
            # hal/third_party/include
)

list(APPEND srcs
            hal/src/common/volc_crypto.c
            hal/src/common/volc_string.c
            hal/src/common/volc_tls.c
            hal/src/platform/bk7258/volc_atomic.c
            hal/src/platform/bk7258/volc_device.c
            hal/src/platform/bk7258/volc_device_finger_print.c
            hal/src/platform/bk7258/volc_directory.c
            hal/src/platform/bk7258/volc_errno.c
            hal/src/platform/bk7258/volc_fileio.c
            hal/src/platform/bk7258/volc_log.c
            hal/src/platform/bk7258/volc_memory.c
            hal/src/platform/bk7258/volc_mutex.c
            hal/src/platform/bk7258/volc_network.c
            hal/src/platform/bk7258/volc_socket.c
            hal/src/platform/bk7258/volc_thread.c
            hal/src/platform/bk7258/volc_time.c
            hal/src/platform/bk7258/volc_uuid.c
            hal/src/platform/bk7258/volc_zlib.c
)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common lwip_intf_v2_1 bk_vfs bk_rtos psa_mbedtls zlib)
add_dependencies(${COMPONENT_LIB} zlib)

if (CONFIG_VOLC_RTC_EN) 
    set(volc_define
        -I${CMAKE_CURRENT_SOURCE_DIR}/hal/configs
        -DMBEDTLS_USER_CONFIG_FILE=\<config_mbedtls.h\>
        -fvisibility=hidden -Wno-attributes)
    armino_build_set_property(COMPILE_OPTIONS "${volc_define}" APPEND)
endif()

if (CONFIG_VOLC_RTC_EN)
get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/libVolcEngineRTCLite.a NAME)
add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/lib/libVolcEngineRTCLite.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()

