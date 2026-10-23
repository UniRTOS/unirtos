cmake_minimum_required(VERSION 3.18)

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

function(add_sub_cmake cmake_root)
    set(cmake_files)
    if("${ARGN}" STREQUAL "RECURSIVE")
        file(GLOB_RECURSE cmake_files CONFIGURE_DEPENDS
            "${cmake_root}/*/*.cmake"
        )
    else()
        file(GLOB cmake_files CONFIGURE_DEPENDS
            "${cmake_root}/*/*.cmake"
        )
    endif()

    foreach(cmake_file ${cmake_files})
        get_property(cmake_loaded GLOBAL PROPERTY UNIRTOS_INCLUDED_CMAKE_FILES)
        if(NOT cmake_file IN_LIST cmake_loaded)
            include("${cmake_file}")
            set_property(GLOBAL APPEND PROPERTY UNIRTOS_INCLUDED_CMAKE_FILES
                "${cmake_file}"
            )
        endif()
    endforeach()

endfunction()

function(add_component_static_lib target_name)
    cmake_parse_arguments(ARG "" "" "SRCS;INCS;DEFS" ${ARGN})

    if(ARG_SRCS)
        add_library(${target_name} STATIC ${ARG_SRCS})
    else()
        # 兼容直接传源文件列表的情况
        add_library(${target_name} STATIC ${ARG_UNPARSED_ARGUMENTS})
    endif()

    set_property(GLOBAL APPEND PROPERTY ARCH_COMPONENT_LIBRARIES ${target_name})

    if(ARG_INCS)
        target_include_directories(${target_name} PRIVATE ${ARG_INCS})
    endif()

    if(ARG_DEFS)
        target_compile_definitions(${target_name} PRIVATE ${ARG_DEFS})
    endif()
endfunction()

add_compile_options("${compile_definitions}")
add_compile_options("${compile_options}")
add_c_compile_options("${c_compile_options}")
add_cxx_compile_options("${cxx_compile_options}")

set(UNIRTOS_STARTUP_DIR "${UNIRTOS_DIR}/qos_components/system/startup")
set(UNIRTOS_BK7258 "${UNIRTOS_DIR}/qos_kernel/bk7258")
set(APP_ROOT "${UNIRTOS_DIR}/qos_kernel/bk7258/arch_prj/ql_kernel/ap")
include("${APP_ROOT}/config/sdkconfig.cmake")

add_sub_cmake("${UNIRTOS_BK7258}/arch_prj/ql_components")

add_executable(app
    "${UNIRTOS_BK7258}/main/unirtos_main.c"
    "${UNIRTOS_STARTUP_DIR}/unirtos_init.c"
    "${APP_ROOT}/bk7258_ap_elf_src.c"
)
set (inc_dirs 
    "${APP_ROOT}/config"
    "${APP_ROOT}/include"
    "${APP_ROOT}/include/driver/bk_private"
    "${APP_ROOT}/include/driver/bk_private/legacy"
    "${APP_ROOT}/include/components"
    "${APP_ROOT}/include/components/bk_rtos"
    "${APP_ROOT}/include/components/bk_wifi"
    "${APP_ROOT}/include/components/bk_wifi/bk_private"
    "${APP_ROOT}/include/components/bk_wifi_driver"
    "${APP_ROOT}/include/components/bk_common"
    "${APP_ROOT}/include/components/bk_cli/bk_private"
    "${APP_ROOT}/include/os_source/freertos"
    "${APP_ROOT}/include/os_source/portable/GCC/ARM_CM33_NTZ/non_secure"
    "${APP_ROOT}/include/components/bk_rtos/freertos"
    "${APP_ROOT}/include/components/cmsis"   
    "${APP_ROOT}/include/soc/bk7258/hal" 
    "${APP_ROOT}/include/soc/bk7258/soc"  
    "${APP_ROOT}/include/soc/common/hal"  
    "${APP_ROOT}/include/soc/common/soc"  
    "${APP_ROOT}/include/driver/spinlock"
    "${APP_ROOT}/include/driver/bk7258_ap"
    "${APP_ROOT}/include/components/bk_vfs"
    "${APP_ROOT}/include/components/bk_cli"
    "${APP_ROOT}/include/components/bk_cli/bk_private"
    "${APP_ROOT}/include/components/bk_ps"
    "${APP_ROOT}/../partitions"   
    )

if (CONFIG_WANSON_ASR)
    list(APPEND inc_dirs 
        "${APP_ROOT}/include/components/bk_thirdparty/asr/wanson/include"
        "${APP_ROOT}/include/components/bk_thirdparty/asr/wanson/bk7258"
        )
endif()
if (CONFIG_WANSON_ARMINO_ASR)
    list(APPEND inc_dirs 
        "${APP_ROOT}/include/components/bk_thirdparty/asr/wanson_armino/include"
        "${APP_ROOT}/include/components/bk_thirdparty/asr/wanson_armino/bk7258"
        )
endif()
if (CONFIG_WANSON_ASR_WORDS_VERSION_V1)
	list (APPEND inc_dirs "${APP_ROOT}/include/components/bk_thirdparty/asr/wanson_armino/bk7258/words_version_v1")
endif()

if (CONFIG_WANSON_ASR_WORDS_VERSION_V2)
	list (APPEND inc_dirs "${APP_ROOT}/include/components/bk_thirdparty/asr/wanson_armino/bk7258/words_version_v2")
endif()

if (CONFIG_WANSON_ASR_GROUP_VERSION_WORDS_V1)
	list (APPEND inc_dirs "${APP_ROOT}/include/components/bk_thirdparty/asr/wanson_armino/bk7258/group_version_words_v1")
endif()

if (CONFIG_WANSON_ASR_GROUP_VERSION_WORDS_V2)
    list (APPEND inc_dirs "${APP_ROOT}/include/components/bk_thirdparty/asr/wanson_armino/bk7258/group_version_words_v2")
endif()

include_directories(${inc_dirs})

set(SYSTEM_SRC_CONFIG ${UNIRTOS_BK7258}/arch_prj/ql_components/unirtos/config/system.config)
set(CUSTOMER_SRC_CONFIG ${UNIRTOS_BK7258}/arch_prj/ql_components/unirtos/config/customer.config)

function(build_and_pack_firmware)
    set(PACKAGER_SCRIPT "${UNIRTOS_BK7258}/scripts/package_all.py")
    set(PACKAGER_ROOT "${CMAKE_SOURCE_DIR}/qos_kernel/${build_arch}/arch_prj/ql_kernel")
    set(PACKAGER_BOOTLOADER "${PACKAGER_ROOT}/bootloader/bootloader.bin")
    set(PACKAGER_CONFIG "${PACKAGER_ROOT}/partitions/bk_package.json")
    set(CP_BIN_FILE "${PACKAGER_ROOT}/cp/cp.bin")
    set(ALL_BIN_FILE "${OUTPUT_DIR}/all-app.bin")
    set(OTA_RBL_FILE "${OUTPUT_DIR}/app_pack.rbl")
    set(OTA_PACKAGER_DIR "${UNIRTOS_BK7258}/arch_prj/ql_tools/ota_rbl")
    set(OTA_PACKAGER_FILES
        "${OTA_PACKAGER_DIR}/ota_packager_python.py"
        "${OTA_PACKAGER_DIR}/xts_aes.py"
        "${OTA_PACKAGER_DIR}/aes.py"
        "${OTA_PACKAGER_DIR}/aes_tables.py"
        "${OTA_PACKAGER_DIR}/gzipHeader.bin"
        "${OTA_PACKAGER_DIR}/ota_rbl.config"
    )
    set(APP_ROOT "${PACKAGER_ROOT}/ap")
    set(APP_LINKER_SCRIPT "${APP_ROOT}/bk7258_ap.ld")
    set(APP_ELF_FILE "${OUTPUT_DIR}/app.elf")
    set(APP_BIN_FILE "${OUTPUT_DIR}/app.bin")
    set(APP_NM_FILE "${OUTPUT_DIR}/app.nm")
    set(APP_BIN_MARKER "${OUTPUT_DIR}/app.bin_tmp")

    target_compile_definitions(app PRIVATE ${compile_definitions})
    target_compile_options(app PRIVATE ${compile_options})
    target_compile_options(app PRIVATE
        "$<$<COMPILE_LANGUAGE:C>:${c_compile_options}>"
        "$<$<COMPILE_LANGUAGE:CXX>:${cxx_compile_options}>"
    )

    target_link_options(app PRIVATE
        "-Wl,--cref"
        "-Wl,--print-memory-usage"
        "-Wl,-Map,${OUTPUT_DIR}/app.map"
        "-Wl,-T,${APP_LINKER_SCRIPT}"
        "-Wl,--gc-sections"
        "-L${APP_ROOT}/libs"
        ${link_options}
    )

    # get_property(component_libraries GLOBAL PROPERTY ARCH_COMPONENT_LIBRARIES)
    target_link_libraries(app PRIVATE
        ${standard_libs}
        "-Wl,--start-group"
        "-lstdc++"
        ${ALL_LIBRARIES}
        # ${component_libraries}
        "-Wl,--end-group"
        ${standard_libs}
        ${standard_libs}
    )

    set_target_properties(app PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
        OUTPUT_NAME app
        SUFFIX .elf
    )

    add_custom_command(
        OUTPUT "${APP_BIN_FILE}" "${APP_NM_FILE}" "${APP_BIN_MARKER}"
        COMMAND "${CMAKE_OBJCOPY}" -O binary "${APP_ELF_FILE}" "app.bin"
        COMMAND "${CMAKE_READELF}" -a -h -l -S -g -s "${APP_ELF_FILE}" > "app.txt"
        COMMAND "${CMAKE_NM}" -n -l -C -a -A -g "${APP_ELF_FILE}" > "app.nm"
        COMMAND "${CMAKE_OBJDUMP}" -d "${APP_ELF_FILE}" > "app.dis"
        COMMAND "${CMAKE_COMMAND}" -E touch "${APP_BIN_MARKER}"
        DEPENDS app "${APP_LINKER_SCRIPT}"
        VERBATIM
        WORKING_DIRECTORY "${OUTPUT_DIR}"
        COMMENT "Generating app.bin and app.nm"
    )

    foreach(required_file ${py_exe} ${PACKAGER_SCRIPT} ${PACKAGER_BOOTLOADER} ${PACKAGER_CONFIG} ${CP_BIN_FILE} ${OTA_PACKAGER_FILES})
        if(NOT EXISTS "${required_file}")
            message(FATAL_ERROR "Required packaging file not found: ${required_file}")
        endif()
    endforeach()

    add_custom_command(
        OUTPUT "${ALL_BIN_FILE}" "${OTA_RBL_FILE}"
        COMMAND "${py_exe}" "${PACKAGER_SCRIPT}"
            --app "${CP_BIN_FILE}"
            --app1 "${APP_BIN_FILE}"
            --output "${ALL_BIN_FILE}"
            --bootloader "${PACKAGER_BOOTLOADER}"
            --partition-config "${PACKAGER_CONFIG}"
            --ota-output "${OTA_RBL_FILE}"
        DEPENDS "${CP_BIN_FILE}" "${APP_BIN_FILE}" "${PACKAGER_SCRIPT}" "${PACKAGER_BOOTLOADER}" "${PACKAGER_CONFIG}" ${OTA_PACKAGER_FILES}
        VERBATIM
        COMMENT "Packaging all-app.bin and app_pack.rbl OTA firmware"
    )

    add_custom_target(all_bin ALL DEPENDS "${ALL_BIN_FILE}" "${OTA_RBL_FILE}")
endfunction()



