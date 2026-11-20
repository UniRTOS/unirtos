cmake_minimum_required(VERSION 3.18)

if(CMAKE_HOST_WIN32 AND CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_NINJA_FORCE_RESPONSE_FILE ON CACHE BOOL "Force Ninja response files" FORCE)
endif()

set(UNIRTOS_APP_BUILD ON)
set(OUTPUT_DIR "${OUTPUT_RELEASE_DIR}/${build_version}")

set(standard_libs
    "libm.a"
    "libgcc.a"
    "libc.a"
    "libnosys.a"
)
#add the libs in toolchain
link_libraries(${standard_libs})
# link_libraries(libm.a) #contian sin() cos() ...
# link_libraries(libgcc.a) #contian __riscv_restore_2  __riscv_save_3 ...
# link_libraries(libc.a)#contain memset() memcopy() ....
# link_libraries(libnosys.a)#contain _write() ...

set(c_link_options "-mcpu=cortex-m33+nodsp -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mcmse")
set(OVERRIDE_LINK_OPTIONS
    "-fno-builtin-printf"
    "-Wl,--gc-sections"
    "-Wl,--wrap=malloc"
    "-Wl,--wrap=_malloc_r"
    "-Wl,--wrap=free"
    "-Wl,--wrap=_free_r"
    "-Wl,--wrap=zalloc"
    "-Wl,--wrap=calloc"
    "-Wl,--wrap=calloc_r"
    "-Wl,--wrap=realloc"
    "-Wl,--wrap=_realloc_r"
    "-Wl,--wrap=printf"
    "-Wl,--wrap=vsnprintf"
    "-Wl,--wrap=snprintf"
    "-Wl,--wrap=sprintf"
    "-Wl,--wrap=puts"
    "-Wl,--wrap=__assert_func"
)

#list(APPEND c_link_options ${OVERRIDE_LINK_OPTIONS})


list(APPEND link_options ${OVERRIDE_LINK_OPTIONS})
list(APPEND link_options "-fno-lto")
list(APPEND link_options "-fno-rtti")

set(CMAKE_C_FLAGS "${c_link_options} -DWIFI_BLE_COEXIST -DCONFIG_CMAKE=1 -DBK_MAC=1 -nostdlib")
set(CMAKE_CXX_FLAGS "${c_link_options} -DWIFI_BLE_COEXIST -DCONFIG_CMAKE=1 -DBK_MAC=1 -nostdlib")
set(compile_options )
set(compile_definitions )
set(cxx_compile_options)



list(APPEND compile_definitions "-D_GNU_SOURCE")
list(APPEND compile_options     "-g"
                                "-Os"
                                "-nostdlib"
                                "-Wall"
                                "-Werror"
                                "-Wno-address"
                                "-Wno-array-bounds"
                                "-Wno-format"
                                "-Wno-unknown-pragmas"
                                "-Wno-address-of-packed-member"  # gcc 5 don't recognize no-address-of-packed-member
                                "-Wno-deprecated-declarations"   # prefer to use -Wno-error=deprecated-declarations
                                "-Wno-unused-value"              # ignore statement with no effect warning
                                "-Wno-unused-function"           # ignore unused function warning
                                "-fno-builtin-printf"            # prevent printf from optimizing to puts
                                "-ffunction-sections"
                                "-fsigned-char"
                                "-fdata-sections"
                                "-fno-strict-aliasing"

                                #"-x assembler-with-cpp" #TODO fix it
                                "-ggdb")

list(APPEND c_compile_options   "-std=gnu99"
                                "-Wno-old-style-declaration"
                                "-nostdlib"
                                "-ffunction-sections"
                                )

list(APPEND cxx_compile_options "-std=gnu++11"
                                "-nostdlib"
                                 )

function(add_c_compile_options)
    foreach(option ${ARGV})
        # note: the Visual Studio Generator doesn't support this...
        add_compile_options($<$<COMPILE_LANGUAGE:C>:${option}>)
    endforeach()
endfunction()

function(add_cxx_compile_options)
    foreach(option ${ARGV})
        # note: the Visual Studio Generator doesn't support this...
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${option}>)
    endforeach()
endfunction()


add_compile_options("${compile_definitions}")
add_compile_options("${compile_options}")
add_c_compile_options("${c_compile_options}")
add_cxx_compile_options("${cxx_compile_options}")

set(UNIRTOS_STARTUP_DIR "${UNIRTOS_DIR}/qos_components/system/startup")
set(UNIRTOS_BK7258 "${UNIRTOS_DIR}/qos_kernel/bk7258")
set(SYSTEM_SRC_CONFIG "${UNIRTOS_BK7258}/arch_prj/ql_components/UniRtos/config/system.config")
set(CUSTOMER_SRC_CONFIG "${UNIRTOS_BK7258}/arch_prj/ql_components/UniRtos/config/customer.config")

if("$ENV{ARMINO_TOOLS_PATH}" STREQUAL "")
    set(ENV{ARMINO_TOOLS_PATH} "${UNIRTOS_BK7258}/arch_prj/ql_tools")
endif()

if(NOT EXISTS "$ENV{ARMINO_TOOLS_PATH}/build_tools/cmake/armino.cmake")
    message(FATAL_ERROR "Invalid ARMINO_TOOLS_PATH: $ENV{ARMINO_TOOLS_PATH}")
endif()
set(BK7258_AP_ROOT "${UNIRTOS_DIR}/qos_kernel/bk7258/arch_prj/ql_components")
set(ARMINO_PROJECT_DIR "${UNIRTOS_BK7258}/arch_prj/ql_application")
set(ENV{ARMINO_PATH} "${BK7258_AP_ROOT}")
set(ENV{ARMINO_AP_DIR} "${BK7258_AP_ROOT}")
set(ENV{ARMINO_SOC} "bk7258_ap")
set(ARMINO_PARTITIONS_DIR "${UNIRTOS_DIR}/qos_kernel/bk7258/arch_prj/ql_kernel/partitions")
set(ENV{PARTITIONS_DIR} "${ARMINO_PARTITIONS_DIR}")
set(ENV{PROJECT_DIR} "${ARMINO_PROJECT_DIR}")
set(ENV{TOOLCHAIN_DIR} "${COMPILER_PATH}/bin")
file(MAKE_DIRECTORY "${ARMINO_PARTITIONS_DIR}")
get_filename_component(ARMINO_PARTITIONS_ROOT "${ARMINO_PARTITIONS_DIR}" DIRECTORY)
set(ARMINO_PARTITION_CONFIG_DIR "${ARMINO_PROJECT_DIR}/partitions/bk7258")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${ARMINO_PARTITION_CONFIG_DIR}/auto_partitions.csv"
    "${ARMINO_PARTITION_CONFIG_DIR}/ram_regions.csv")

if(CMAKE_HOST_WIN32)
    set(ARMINO_PYTHONPATH "$ENV{ARMINO_TOOLS_PATH};$ENV{PYTHONPATH}")
else()
    set(ARMINO_PYTHONPATH "$ENV{ARMINO_TOOLS_PATH}:$ENV{PYTHONPATH}")
endif()

foreach(partition_tool bk_build_auto_partition.py bk_build_ram_regions.py)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "PYTHONPATH=${ARMINO_PYTHONPATH}"
            "PROJECT_DIR=${ARMINO_PROJECT_DIR}"
            "PROJECT_NAME=${build_project}"
            "PROJECT_BUILD_DIR=${ARMINO_PARTITIONS_ROOT}"
            "ARMINO_SOC_NAME=bk7258"
            "${py_exe}" "$ENV{ARMINO_TOOLS_PATH}/build_tools/build_process/${partition_tool}"
        RESULT_VARIABLE ARMINO_PARTITION_RESULT
    )
    if(NOT ARMINO_PARTITION_RESULT EQUAL 0)
        message(FATAL_ERROR "Armino partition generation failed: ${partition_tool}")
    endif()
endforeach()

if(NOT DEFINED ENV{LIB_HASH} OR "$ENV{LIB_HASH}" STREQUAL "")
    execute_process(
        COMMAND "${py_exe}" "$ENV{ARMINO_TOOLS_PATH}/build_tools/compute_files_hash.py"
            "${BK7258_AP_ROOT}/components/bk_libs/bk7258_ap/libs"
        RESULT_VARIABLE ARMINO_HASH_RESULT
        OUTPUT_VARIABLE ARMINO_LIB_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT ARMINO_HASH_RESULT EQUAL 0 OR NOT ARMINO_LIB_HASH MATCHES "^[0-9a-f]+$")
        message(FATAL_ERROR "Unable to calculate BK7258 platform library hash")
    endif()
    set(ENV{LIB_HASH} "${ARMINO_LIB_HASH}")
endif()

set(ARMINO_SOC bk7258_ap)
include("$ENV{ARMINO_TOOLS_PATH}/build_tools/cmake/colors.cmake")
define_colors()
include("$ENV{ARMINO_TOOLS_PATH}/build_tools/cmake/armino.cmake")
armino_build_component("${BK7258_AP_ROOT}/UniRtos")
armino_build_component("${BK7258_AP_ROOT}/components/bk_thirdparty/agora-iot-sdk")
armino_build_component("${ARMINO_PROJECT_DIR}")
armino_build_component("${BK7258_AP_ROOT}/components/bk_thirdparty/asr/wanson")
armino_build_component("${BK7258_AP_ROOT}/components/bk_thirdparty/asr/wanson_armino")
armino_build_component("${BK7258_AP_ROOT}/components/bk_thirdparty/cs2p2p")
armino_build_component("${BK7258_AP_ROOT}/components/bk_thirdparty/zlib")
function(__project_reload)
    if(COMPONENTS_DIRS)
        armino_build_set_property(__COMPONENT_TARGETS "")
        spaces2list(COMPONENTS_DIRS)
        foreach(component_dir ${COMPONENTS_DIRS})
            __build_add_components(${component_dir})
        endforeach()
        spaces2list(COMPONENT_DIRS)
        armino_build_get_property(prefix __PREFIX)
        foreach(component_dir ${COMPONENT_DIRS})
            get_filename_component(component_dir "${component_dir}" ABSOLUTE)
            __component_add("${component_dir}" "${prefix}")
        endforeach()
    endif()
endfunction()
file(TO_CMAKE_PATH "${py_exe}" ARMINO_PYTHON)
armino_build_set_property(PYTHON "${ARMINO_PYTHON}")
armino_build_set_property(__ROOT_SDKCONFIG_RENAME
    "$ENV{ARMINO_TOOLS_PATH}/build_tools/build_files/sdkconfig_ap.rename")
set(ARMINO_BUILD_DIR "${CMAKE_BINARY_DIR}/bk7258_ap")
set(BUILD_DIR "${ARMINO_BUILD_DIR}")
file(MAKE_DIRECTORY "${ARMINO_BUILD_DIR}")
configure_file("${ARMINO_PROJECT_DIR}/config/bk7258_ap/config"
    "${ARMINO_BUILD_DIR}/sdkconfig" COPYONLY)

set (inc_dirs
    "${CMAKE_BINARY_DIR}/bk7258_ap/config"
    "${CMAKE_BINARY_DIR}/bk7258_ap/armino/zlib/zlib_v1_3_1"
    "${BK7258_AP_ROOT}/include"
    "${BK7258_AP_ROOT}/include/driver"
    "${BK7258_AP_ROOT}/include/driver/hal"
    "${BK7258_AP_ROOT}/include/components"
    "${BK7258_AP_ROOT}/components/bk_rtos"
    "${BK7258_AP_ROOT}/components/bk_wifi"
    "${BK7258_AP_ROOT}/components/bk_wifi/bk_private"
    "${BK7258_AP_ROOT}/components/bk_wifi_driver"
    "${BK7258_AP_ROOT}/components/bk_common"
    "${BK7258_AP_ROOT}/components/bk_cli/bk_private"
    "${BK7258_AP_ROOT}/../ql_kernel/partitions"
    "${BK7258_AP_ROOT}/components/bk_thirdparty/zlib/zlib_v1_3_1"
    "${BK7258_AP_ROOT}/components/os_source/freertos_smp_v2p0/FreeRTOS-Kernel/include/freertos"
    "${BK7258_AP_ROOT}/components/os_source/freertos_smp_v2p0/FreeRTOS-Kernel/include/additions"
    "${BK7258_AP_ROOT}/components/os_source/freertos_smp_v2p0/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure"
    "${BK7258_AP_ROOT}/components/bk_rtos/include"
    "${BK7258_AP_ROOT}/components/bk_rtos/freertos"
    "${BK7258_AP_ROOT}/components/cmsis/CMSIS_5/Device/Beken/bk7236xx/Include"
    "${BK7258_AP_ROOT}/middleware/driver/spinlock"
    "${BK7258_AP_ROOT}/middleware/driver/include/bk_private"
    "${BK7258_AP_ROOT}/middleware/soc/bk7258_ap/hal"
    "${BK7258_AP_ROOT}/middleware/soc/bk7258_ap/soc"
    "${BK7258_AP_ROOT}/middleware/soc/common/soc/include"
    "${BK7258_AP_ROOT}/middleware/soc/common/hal/include"
    "${BK7258_AP_ROOT}/components/lwip_intf_v2_1/lwip-2.1.2"
    "${BK7258_AP_ROOT}/components/lwip_intf_v2_1/lwip-2.1.2/src"
    "${BK7258_AP_ROOT}/components/lwip_intf_v2_1/lwip-2.1.2/port"
    "${BK7258_AP_ROOT}/components/lwip_intf_v2_1/lwip-2.1.2/src/include"
    "${BK7258_AP_ROOT}/components/lwip_intf_v2_1/lwip-2.1.2/src/include/netif"
    "${BK7258_AP_ROOT}/components/lwip_intf_v2_1/lwip-2.1.2/src/include/lwip"
    "${BK7258_AP_ROOT}/components/lwip_intf_v2_1/lwip-2.1.2/src/include/lwip/priv"
    "${BK7258_AP_ROOT}/components/psa_mbedtls/mbedtls/include"
    "${BK7258_AP_ROOT}/components/psa_mbedtls/mbedtls/library"
    "${BK7258_AP_ROOT}/components/psa_mbedtls/mbedtls_port/inc"
    "${BK7258_AP_ROOT}/components/psa_mbedtls/mbedtls_port/mbedtls/include"
    "${BK7258_AP_ROOT}/components/psa_mbedtls/mbedtls_port/mbedtls/library"
    "${BK7258_AP_ROOT}/components/psa_mbedtls/mbedtls_port/configs"
    "${BK7258_AP_ROOT}/components/json"
    "${BK7258_AP_ROOT}/components/bk_thirdparty/agora-iot-sdk/include/bk7258"
    )
include_directories(${inc_dirs})

message("start armino build......")
armino_build_process(${ARMINO_SOC}
    PROJECT_DIR "${ARMINO_PROJECT_DIR}"
    BUILD_DIR "${ARMINO_BUILD_DIR}"
    SDKCONFIG "${ARMINO_BUILD_DIR}/sdkconfig"
    PROJECT_NAME "${CMAKE_PROJECT_NAME}"
    PROJECT_VER "${build_version}")

armino_build_get_property(ARMINO_SDKCONFIG_CMAKE SDKCONFIG_CMAKE)
include("${ARMINO_SDKCONFIG_CMAKE}")


add_executable(app
    "${UNIRTOS_BK7258}/main/unirtos_main.c"
    "${UNIRTOS_STARTUP_DIR}/unirtos_init.c"
)
set(ARMINO_BUILD_VERSION_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/armino_build_version.c")
file(GENERATE OUTPUT "${ARMINO_BUILD_VERSION_SOURCE}"
    CONTENT "volatile const char build_version[] = __DATE__ \" \" __TIME__;\n")
target_sources(app PRIVATE "${ARMINO_BUILD_VERSION_SOURCE}")

function(build_and_pack_firmware)
    set(packager_root "${UNIRTOS_BK7258}/arch_prj/ql_kernel")
    set(packager_script "${UNIRTOS_BK7258}/scripts/package_all.py")
    set(bootloader "${packager_root}/bootloader/bootloader.bin")
    set(partition_config "${ARMINO_PARTITIONS_DIR}/bk_package.json")
    set(cp_bin "${packager_root}/cp/cp.bin")
    set(app_bin "${OUTPUT_DIR}/app.bin")
    set(all_bin "${OUTPUT_DIR}/all-app.bin")
    set(ota_bin "${OUTPUT_DIR}/app_pack.rbl")
    set(ota_tools "${UNIRTOS_BK7258}/arch_prj/ql_tools/ota_rbl")
    set(unirtos_library "${BK7258_AP_ROOT}/UniRtos/libs/libUniRtos.a")
    set(agora_prebuilt_libraries
        "${BK7258_AP_ROOT}/components/bk_thirdparty/agora-iot-sdk/bk7258/libs/librtsa.a"
        "${BK7258_AP_ROOT}/components/bk_thirdparty/agora-iot-sdk/bk7258/libs/libagora-cjson.a"
    )

    armino_build_get_property(armino_components BUILD_COMPONENT_ALIASES)
    armino_build_get_property(armino_link_dependencies __LINK_DEPENDS)
    list(APPEND armino_components
        "${unirtos_library}"
        ${agora_prebuilt_libraries}
    )
    foreach(required_library ${unirtos_library} ${agora_prebuilt_libraries})
        if(NOT EXISTS "${required_library}")
            message(FATAL_ERROR "Required firmware library not found: ${required_library}")
        endif()
    endforeach()
    set_property(TARGET app APPEND PROPERTY LINK_DEPENDS "${armino_link_dependencies}")
    # target_include_directories(app PRIVATE ${inc_dirs})
    target_link_libraries(app PRIVATE
        "-Wl,--start-group" "-lstdc++" ${armino_components} ${ALL_LIBRARIES}
        "-Wl,--end-group" ${standard_libs})
    target_link_options(app PRIVATE
        "-Wl,--cref" "-Wl,--print-memory-usage" "-Wl,-Map,${OUTPUT_DIR}/app.map"
        ${link_options})
    set_target_properties(app PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}" OUTPUT_NAME app SUFFIX .elf)
    file(MAKE_DIRECTORY "${OUTPUT_DIR}")

    add_custom_command(OUTPUT "${app_bin}"
        COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:app>" "${app_bin}"
        DEPENDS app
        VERBATIM)
    add_custom_command(OUTPUT "${all_bin}" "${ota_bin}"
        COMMAND "${py_exe}" "${packager_script}"
            --app "${cp_bin}" --app1 "${app_bin}" --output "${all_bin}"
            --bootloader "${bootloader}" --partition-config "${partition_config}"
            --ota-output "${ota_bin}"
        DEPENDS "${cp_bin}" "${app_bin}" "${packager_script}" "${bootloader}" "${partition_config}"
            "${ota_tools}/ota_packager_python.py" "${ota_tools}/xts_aes.py"
            "${ota_tools}/aes.py" "${ota_tools}/aes_tables.py"
            "${ota_tools}/gzipHeader.bin" "${ota_tools}/ota_rbl.config"
        VERBATIM)
    add_custom_target(all_bin ALL DEPENDS "${all_bin}" "${ota_bin}")
endfunction()










