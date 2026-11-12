
# Executable program suffix
set(CMAKE_EXECUTABLE_SUFFIX_ASM .elf)
set(CMAKE_EXECUTABLE_SUFFIX_C .elf)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .elf)


if(NOT unirtos_csdk_build STREQUAL "y" AND NOT unirtos_ocpu_build STREQUAL "y")

set(UNIRTOS_STARTUP_DIR ${UNIRTOS_DIR}/qos_components/system/startup)
set(UNIRTOS_EIGEN_718 ${UNIRTOS_DIR}/qos_kernel/eigen_718)
set(eigen718_csdk_root ${UNIRTOS_DIR}/qos_kernel/eigen_718/arch_prj)
set(EIGEN718_CSDK_COMMON ${eigen718_csdk_root}/ql_common)

else()

set(UNIRTOS_STARTUP_DIR ${UNIRTOS_DIR}/system/startup)
set(UNIRTOS_EIGEN_718 ${UNIRTOS_DIR}/system/platform/${build_arch})
set(eigen718_csdk_root ${UNIRTOS_DIR}/system/platform/${build_arch}/arch_prj)
set(EIGEN718_CSDK_COMMON ${UNIRTOS_DIR}/system/platform/${build_arch}/arch_prj/ql_common)

endif()

set(ARCH_GCCOUT_DIR ${UNIRTOS_BUILD_OUT}/gccout)

# Also protect callers using an older packaged unirtos.exe, which may bypass
# the Python make entry in this checkout and otherwise reuse a 0511 cache.
if(NOT unirtos_csdk_build STREQUAL "y" AND NOT unirtos_ocpu_build STREQUAL "y")
    execute_process(
        COMMAND ${py_exe} ${tools_dir}/python/unirtos_platform_artifact.py
            --root ${UNIRTOS_DIR} --project ${build_project} --gccout ${ARCH_GCCOUT_DIR}
        RESULT_VARIABLE _a04_platform_result
    )
    if(NOT _a04_platform_result EQUAL 0)
        message(FATAL_ERROR "A04 platform artifacts not validated. Follow A04_BUILD_GUIDE.md; do not reuse 0511 gccout.")
    endif()
endif()

message("-----EIGEN718_CSDK_COMMON=${EIGEN718_CSDK_COMMON}---")
# The Yixin 718 platform requires loading the chip_type and PKG_PRODUCT.
include(${EIGEN718_CSDK_COMMON}/middleware/quectel/project/config/public/${build_project}/chip.cmake)
message("-----chip_type=${chip_type}--PKG_PRODUCT=${PKG_PRODUCT}---")

set(FIRMWARE_7Z "${UNIRTOS_BUILD_OUT}/gccout/images/firmware_base_release_pack.7z")
set(FIRMWARE_DIR "${UNIRTOS_BUILD_OUT}/gccout/images/firmware_base_release_pack")
set(FIRMWARE_REQUIRED_FILE_1 "${FIRMWARE_DIR}/tmp/ap_bootloader.bin")
set(FIRMWARE_REQUIRED_FILE_2 "${FIRMWARE_DIR}/tmp/ap_at_command.bin")
set(FIRMWARE_REQUIRED_FILE_3 "${FIRMWARE_DIR}/tmp/cp-demo-flash.bin")

# Load all header file paths on the platform side
include_directories(${EIGEN718_CSDK_COMMON}/prebuild/PS/inc)
include_directories(${EIGEN718_CSDK_COMMON}/prebuild/PLAT/inc)
include_directories(${EIGEN718_CSDK_COMMON}/prebuild/PS/inc)
include_directories(${EIGEN718_CSDK_COMMON}/prebuild/PLAT/inc/usb)
include_directories(${EIGEN718_CSDK_COMMON}/device/target/board/ec7xx_0h00/common/pkginc)
include_directories(${EIGEN718_CSDK_COMMON}/device/target/board/ec7xx_0h00/ap/inc)
include_directories(${EIGEN718_CSDK_COMMON}/device/target/board/ec7xx_0h00/common/inc)
include_directories(${EIGEN718_CSDK_COMMON}/device/target/board/common/ARMCM3/inc)
include_directories(${EIGEN718_CSDK_COMMON}/device/target/board/ec7xx_0h00/common/pkginc)
include_directories(${EIGEN718_CSDK_COMMON}/device/target/board/ec7xx_0h00/ap/inc)
include_directories(${EIGEN718_CSDK_COMMON}/device/target/board/ec7xx_0h00/common/inc)
include_directories(${EIGEN718_CSDK_COMMON}/device/target/board/ec7xx_0h00/ap/gcc)
include_directories(${EIGEN718_CSDK_COMMON}/device/target/board/common/ARMCM3/inc)
include_directories(${EIGEN718_CSDK_COMMON}/os/freertos/CMSIS/ap/inc)
include_directories(${EIGEN718_CSDK_COMMON}/os/freertos/inc)
include_directories(${EIGEN718_CSDK_COMMON}/os/freertos/portable/gcc)
include_directories(${EIGEN718_CSDK_COMMON}/os/freertos/CMSIS/common/inc)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/quectel/open_service/ql_service/inc)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/quectel/cust/inc)
include_directories(${EIGEN718_CSDK_COMMON}/ver_tag)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/lwip/src/include/lwip)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/lwip/src/include/lwip/priv)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/lwip/src/include/lwip/prot)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/lwip/src/include/arch)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/lwip/src/include)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/lwip/src/include/netif)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/lwip/src/include/posix)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/mbedtls/configs)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/mbedtls/include)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/mbedtls/library)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/lzma2201/c)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/thirdparty/lzma2201/c/wrapper)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/quectel/ql_api/include)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/developed/ccio/device/inc)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/developed/cms/cms/inc)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/developed/ccio/custom/inc)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/developed/cms/psdial/inc)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/developed/debug/inc)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/developed/cms/sockmgr/inc)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/developed/common/inc)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/developed/ccio/pub)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/quectel/ql_app/include)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/quectel/cust/inc)
include_directories(${EIGEN718_CSDK_COMMON}/driver/hal/ec7xx/ap/inc)
include_directories(${EIGEN718_CSDK_COMMON}/driver/hal/ec7xx/ap/inc/ec718)
include_directories(${EIGEN718_CSDK_COMMON}/driver/hal/common/inc)
include_directories(${EIGEN718_CSDK_COMMON}/driver/chip/ec7xx/ap/inc)
include_directories(${EIGEN718_CSDK_COMMON}/driver/chip/ec7xx/ap/inc/ec718)
include_directories(${EIGEN718_CSDK_COMMON}/driver/chip/ec7xx/ap/inc_cmsis)
include_directories(${EIGEN718_CSDK_COMMON}/driver/board/ec7xx_0h00/inc)
include_directories(${EIGEN718_CSDK_COMMON}/driver/board/ec7xx_0h00/inc/codec)
include_directories(${EIGEN718_CSDK_COMMON}/project/ec7xx_0h00/ap/apps/at_command/inc)
include_directories(${EIGEN718_CSDK_COMMON}/project/ec7xx_0h00/ap/apps/bootloader/code/include)
include_directories(${EIGEN718_CSDK_COMMON}/unirtos_adapter/dynamic_create)
include_directories(${EIGEN718_CSDK_COMMON}/unirtos_adapter/system_source/plat_source/vfs)
include_directories(${EIGEN718_CSDK_COMMON}/unirtos_adapter/dynamic_create)
include_directories(${EIGEN718_CSDK_COMMON}/middleware/quectel/open_service/ql_service/bridge/inc)

#Load the platform-side system modem config configuration file
set(SYSTEM_SRC_CONFIG ${EIGEN718_CSDK_COMMON}/middleware/quectel/project/config/public/${build_project}/system.config)
set(MODEM_SRC_CONFIG ${EIGEN718_CSDK_COMMON}/middleware/quectel/project/config/public/${build_project}/modem.config)

# OPEN/CSDK build only: the OCPU default package (opt_cust/OCPU/customer.config)
# is merged as the last layer of the application-side configuration, so an
# OPEN/CSDK package can carry application-side switches (eSIM, AT set, ...)
# without touching the project configuration. The standard build does not read
# it: the standard build keeps using its OEM package (build_oem).
# This block only covers the eigen_718 platform; every other platform resolves
# its own OPEN layer inside its own toolchain.cmake.
set(OPEN_TARGET_CONFIG "")
if((unirtos_csdk_build STREQUAL "y") OR (unirtos_ocpu_build STREQUAL "y"))
    # Prefer the copy refreshed into arch_prj; fall back to the platform source
    # tree when the copy is not populated (arch_prj is regenerated per release).
    # Only this revision string has to be bumped when the arch layer is raised.
    set(_ec718_arch_rev "R01")
    set(_ocpu_cust_candidates
        "${EIGEN718_CSDK_COMMON}/middleware/quectel/project/config/public/opt_cust/OCPU/customer.config"
        "${UNIRTOS_DIR}/../arch/ec718/${_ec718_arch_rev}/PLAT/quectel_build/config/public/opt_cust/OCPU/customer.config")
    foreach(_ocpu_cust ${_ocpu_cust_candidates})
        if(EXISTS "${_ocpu_cust}")
            set(OPEN_TARGET_CONFIG "${_ocpu_cust}")
            break()
        endif()
    endforeach()
    if(OPEN_TARGET_CONFIG STREQUAL "")
        # Fail loudly instead of shipping an OPEN package whose eSIM switch
        # silently disappears: a missing OCPU package means the release tree is
        # incomplete. Create the file (or fix the paths above) before packaging.
        message(FATAL_ERROR "OPEN_TARGET_CONFIG not found, checked: ${_ocpu_cust_candidates}")
    else()
        message(STATUS "OPEN_TARGET_CONFIG: ${OPEN_TARGET_CONFIG}")
    endif()
endif()

# Setting up the packaging tool
if(CMAKE_HOST_WIN32)
   set(FCELF_TOOL ${tools_dir}/${build_arch}/fcelf.exe)
else()
   set(FCELF_TOOL ${tools_dir}/${build_arch}/fcelf)
endif()

# Files generated after AP side link
set(AP_ELF ${BINARY_TOP_DIR}/ap_application.elf)
# Convert ELF to binary BIN file using OBJCOPY
set(AP_BIN ${BINARY_BIN_DIR}/ap_application.bin)
# ELF Disassembly and Segment Table Information
set(AP_TXT ${BINARY_BIN_DIR}/ap_application.txt)
# map information
set(AP_MAP ${BINARY_BIN_DIR}/ap_application.map)

set(OUTPUT_RELEASE_DIR "${UNIRTOS_BUILD_OUT}/release")

if(NOT unirtos_csdk_build STREQUAL "y")
if(NOT EXISTS "${FIRMWARE_REQUIRED_FILE_1}" OR NOT EXISTS "${FIRMWARE_REQUIRED_FILE_2}" OR NOT EXISTS "${FIRMWARE_REQUIRED_FILE_3}")
    message(STATUS "Extracting base firmware package: ${FIRMWARE_7Z}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${FIRMWARE_DIR}"
        RESULT_VARIABLE remove_result
    )
    if(NOT remove_result EQUAL 0)
        message(FATAL_ERROR "remove failed, result=${remove_result}")
    endif()

    execute_process(
        COMMAND "${7Z_EXE}" x -y -aoa "${FIRMWARE_7Z}" "-o${FIRMWARE_DIR}"
        RESULT_VARIABLE unzip_result
    )
    if(NOT unzip_result EQUAL 0)
        message(FATAL_ERROR "7z extract failed, result=${unzip_result}")
    endif()
else()
    message(STATUS "Reusing extracted base firmware package: ${FIRMWARE_DIR}")
endif()
endif()

# Platform-side bootloader
set(BOOTLOADER_BIN ${FIRMWARE_DIR}/tmp/ap_bootloader.bin)
# Platform-side cp
set(CP_BIN ${FIRMWARE_DIR}/tmp/cp-demo-flash.bin)
# Platform-side copied mem_map, mainly macro definitions
set(MEM_MAP ${FIRMWARE_DIR}/tmp/mem_map.txt)
# Platform-side kernel
set(KERNEL_BIN ${FIRMWARE_DIR}/tmp/ap_at_command.bin)
# Platform-side unir_updater
set(UNIR_UPDATER ${FIRMWARE_DIR}/tmp/ap_updater.bin)
# Platform-side GNSS
set(GNSS_IMG ${FIRMWARE_DIR}/tmp/gnss_img.bin)

set(EC718_PRD_EFS_SPI0 ${UNIRTOS_DIR}/qos_tools/eigen_718/customer_fs.bin)
set(EC718_PRD_EFS_SPI1 ${UNIRTOS_DIR}/qos_tools/eigen_718/customer_fs_spi1.bin)
set(EC718_PRD_EXT_UPDATER_SPI0 ${UNIRTOS_DIR}/qos_tools/eigen_718/customer_updater.bin)
set(EC718_PRD_EXT_UPDATER_SPI1 ${UNIRTOS_DIR}/qos_tools/eigen_718/customer_updater_spi1.bin)
set(EC718_CUS_APP2 ${UNIRTOS_DIR}/qos_tools/eigen_718/customer_app2.bin)

set(BINPKG_EXT_FS_ENABLED FALSE)
set(BINPKG_EXT_UPDATER_ENABLED FALSE)
set(BINPKG_EXT_IN_PACK "")
set(BINPKG_EXT_DEPENDS)

# ---- resolve external FS (SPI0/SPI1 by filename) ----
if(EXISTS "${EC718_PRD_EFS_SPI1}")
    if(EXISTS "${EC718_PRD_EFS_SPI0}")
        message(FATAL_ERROR
            "Both customer_fs.bin and customer_fs_spi1.bin found in "
            "${UNIRTOS_DIR}/qos_tools/eigen_718/. Keep only one.")
    endif()
    set(BINPKG_EXT_FS_ENABLED TRUE)
    list(APPEND BINPKG_EXT_IN_PACK
        -input ${EC718_PRD_EFS_SPI1}
        -addrname EFID01_EFS_IMG_BURNADDR_LNA -flashsize EFID01_EFS_IMG_LIMIT_SIZE
    )
    list(APPEND BINPKG_EXT_DEPENDS ${EC718_PRD_EFS_SPI1})
elseif(EXISTS "${EC718_PRD_EFS_SPI0}")
    set(BINPKG_EXT_FS_ENABLED TRUE)
    list(APPEND BINPKG_EXT_IN_PACK
        -input ${EC718_PRD_EFS_SPI0}
        -addrname EF_EFS_IMG_BURNADDR_LNA -flashsize EF_EFS_IMG_LIMIT_SIZE
    )
    list(APPEND BINPKG_EXT_DEPENDS ${EC718_PRD_EFS_SPI0})
endif()

# ---- resolve external updater (SPI0/SPI1 by filename) ----
if(EXISTS "${EC718_PRD_EXT_UPDATER_SPI1}")
    if(EXISTS "${EC718_PRD_EXT_UPDATER_SPI0}")
        message(FATAL_ERROR
            "Both customer_updater.bin and customer_updater_spi1.bin found in "
            "${UNIRTOS_DIR}/qos_tools/eigen_718/. Keep only one.")
    endif()
    set(BINPKG_EXT_UPDATER_ENABLED TRUE)
    list(APPEND BINPKG_EXT_IN_PACK
        -input ${EC718_PRD_EXT_UPDATER_SPI1}
        -addrname EFID01_EXT_UPDATER_IMG_BURNADDR_LNA
        -flashsize EFID01_EXT_UPDATER_IMG_LIMIT_SIZE
    )
    list(APPEND BINPKG_EXT_DEPENDS ${EC718_PRD_EXT_UPDATER_SPI1})
elseif(EXISTS "${EC718_PRD_EXT_UPDATER_SPI0}")
    set(BINPKG_EXT_UPDATER_ENABLED TRUE)
    list(APPEND BINPKG_EXT_IN_PACK
        -input ${EC718_PRD_EXT_UPDATER_SPI0}
        -addrname EF_EXT_UPDATER_IMG_BURNADDR_LNA
        -flashsize EF_EXT_UPDATER_IMG_LIMIT_SIZE
    )
    list(APPEND BINPKG_EXT_DEPENDS ${EC718_PRD_EXT_UPDATER_SPI0})
else()
    # no external updater, fallback to platform-side updater
    list(APPEND BINPKG_EXT_IN_PACK
        -input ${UNIR_UPDATER}
        -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
    )
endif()

if(NOT EXISTS "${EC718_CUS_APP2}")
    set(BINPKG_CUS_APP2_ENABLED FALSE)
else()
    set(BINPKG_CUS_APP2_ENABLED TRUE)
endif()

#secboot
if(CHIP_TYPE_BUILD STREQUAL "TYPE_EC718PM" OR CHIP_TYPE_BUILD STREQUAL "TYPE_EC718HM")
    find_package(Python REQUIRED)
    if(Python_FOUND)
        # If system has python environment variable, use the system python program
        message("Python found: ${Python_EXECUTABLE}")
        message("Python version: ${Python_VERSION}")
        set(py_exe ${Python_EXECUTABLE})
    else()
        message("Python not found. Use local python env ")
        # If system doesn't have it, use the built-in program
        set(py_exe ${python_local}/Python38/python.exe)
    endif()

    #Decide whether to sign or not
    set(SECBOOT_SIGN FALSE)
    set(ECDSA_P256_MODE TRUE)

    # Setting up the secure sign tool
    if(CMAKE_HOST_WIN32)
        set(SECURE_TOOL ${tools_dir}/${build_arch}/ecsecure.exe)
    else()
        set(SECURE_TOOL ${tools_dir}/${build_arch}/ecsecure)
    endif()
    if(BINPKG_EXT_UPDATER_ENABLED)
        set(SECURE_JSON ${eigen718_csdk_root}/ql_application/qosa_secureboot_cfg_ec718pm_ext.json)
    else()
        set(SECURE_JSON ${eigen718_csdk_root}/ql_application/qosa_secureboot_cfg_ec718pm.json)
    endif()
    # Setting up the platform sec header
    set(BLSC_HEAD ${FIRMWARE_DIR}/tmp/bl_sec_header.bin)
    set(SYSSC_HEAD ${FIRMWARE_DIR}/tmp/sys_sec_header.bin)
    set(UPDATERSC_HEAD ${FIRMWARE_DIR}/tmp/updater_sec_header.bin)
    set(APPSC_HEAD ${FIRMWARE_DIR}/tmp/application_sec_header.bin)
    set(SIGN_KEY ${eigen718_csdk_root}/ql_application/sign_key.pem)

    if(CHIP_TYPE_BUILD STREQUAL "TYPE_EC718HM")
        set(SYSBNADR_VALUE cd000)
    else()
        set(SYSBNADR_VALUE 82000)
    endif()
    # Setting up the sec download config
    if(ECDSA_P256_MODE)
        set(EC718_PRMGRCFG_SEC ${FIRMWARE_DIR}/${chip_type}_sec256_PrMgrCfg.json.bin)
        set(EC718_FORMAT_SEC ${FIRMWARE_DIR}/${chip_type}_format.json.bin)
        set(EC718_PRD_UART_SEC ${FIRMWARE_DIR}/${chip_type}_sec256_prd_uart.baseini.bin)
        set(EC718_PRD_USB_SEC ${FIRMWARE_DIR}/${chip_type}_sec256_prd_usb.baseini.bin)
    else()
        set(EC718_PRMGRCFG_SEC ${FIRMWARE_DIR}/${chip_type}_sec_PrMgrCfg.json.bin)
        set(EC718_FORMAT_SEC ${FIRMWARE_DIR}/${chip_type}_format.json.bin)
        set(EC718_PRD_UART_SEC ${FIRMWARE_DIR}/${chip_type}_sec_prd_uart.baseini.bin)
        set(EC718_PRD_USB_SEC ${FIRMWARE_DIR}/${chip_type}_sec_prd_usb.baseini.bin)
    endif()

    # Dynamically update the binfile/binheader paths in qosa_secureboot_cfg_ec718pm.json
    function(fix_secureboot_json)
        set(input_json ${SECURE_JSON})
        set(output_json ${input_json}.fixed)

        message(STATUS "=== SECUREBOOT JSON FIX DEBUG ===")
        # Handling JSON using Python scripts
        execute_process(
            COMMAND ${py_exe} ${tools_dir}/python/sec_paths_fix.py
                    ${input_json}
                    ${output_json}
                    ${UNIRTOS_BUILD_OUT}
                    ${BINARY_BIN_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )

        # Replace the original file
        file(REMOVE ${input_json})
        file(RENAME ${output_json} ${input_json})

        message(STATUS "Fixed secureboot JSON paths: ${input_json}")
    endfunction()

    fix_secureboot_json()
endif()

set(EC718_PRMGRCFG ${FIRMWARE_DIR}/${chip_type}_PrMgrCfg.json.bin)
set(EC718_FORMAT ${FIRMWARE_DIR}/${chip_type}_format.json.bin)
set(EC718_PRD_UART ${FIRMWARE_DIR}/${chip_type}_prd_uart.baseini.bin)
set(EC718_PRD_USB ${FIRMWARE_DIR}/${chip_type}_prd_usb.baseini.bin)

# Firmware finally packaged and generated
set(OUTPUT_BINPKG ${BINARY_BIN_DIR}/at_command.hbinpkg)



# Platform-side generated .a libraries, after mapping can no longer link .a
file(GLOB_RECURSE platform_lib "${ARCH_GCCOUT_DIR}/libraries/*.a")
# Platform-side generated .o libraries, including app entry functions, API mapping files
file(GLOB_RECURSE platform_o "${ARCH_GCCOUT_DIR}/libraries/*.o")


# gcc libraries
set(libgcc_file_name ${UNIRTOS_EIGEN_718}/libraries/libgcc.a)
set(libc_file_name ${UNIRTOS_EIGEN_718}/libraries/libc_nano.a)
set(libm_file_name ${UNIRTOS_EIGEN_718}/libraries/libm.a)

# Set dependency libraries to global variable
set(MY_LINK_LIBRARIES ${MY_LINK_LIBRARIES} ${libc_file_name} ${libm_file_name} ${libgcc_file_name} )
# set(MY_LINK_LIBRARIES ${MY_LINK_LIBRARIES} ${libm_file_name} ${libgcc_file_name} )
set(MY_LINK_LIBRARIES ${MY_LINK_LIBRARIES} ${platform_lib} ${platform_o})

message("-----MY_LINK_LIBRARIES=${MY_LINK_LIBRARIES}-----")

# Rewrite C language compilation template (similar for C++, can use CMAKE_CXX_COMPILE_OBJECT)
set(CMAKE_C_COMPILE_OBJECT
  "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -MMD -MP -MT <OBJECT> -MF <OBJECT>.d -o <OBJECT> -c <SOURCE>"
)

# Set C compilation rules, -nostartfiles option disables startup files (such as crt0.o, crti.o, crtn.o, etc.), app startup function is custom-defined
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -Wall -Wno-format -Werror -nostartfiles")
# Set CPU type ARM processor hard float
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m3 -mthumb")
# Program optimization, uses platform-side cflags
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -fno-isolate-erroneous-paths-dereference -freorder-blocks-algorithm=stc")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -specs=nano.specs")
# Some necessary macro controls, need to consider whether they are needed
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DQL_UNIRTOS_SUPPORT=y -DQL_OPEN_PLATFORM_SUPPORT -DCHIP_QCX217  -DUNIRTOS_OPEN_MODE_SUPPORT")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -mcpu=cortex-m3 -mthumb -mapcs-frame -std=gnu99 -nostartfiles -specs=nano.specs -Os -ffunction-sections  -fstack-usage -fdata-sections -fno-isolate-erroneous-paths-dereference -freorder-blocks-algorithm=stc -Wall -Wno-format -gdwarf-2 -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -DPSRAM_FEATURE_ENABLE -DconfigUSE_NEWLIB_REENTRANT=1 -DFEATURE_MBEDTLS_ENABLE -DCHIP_EC718 -DTYPE_EC718M -DFEATURE_FREERTOS_ENABLE -DQL_UNIRTOS_SUPPORT -DUNIRTOS_LWIP_MODIFY -DUNIRTOS_MODEM_MODIFY -DUNIRTOS_PLATFORM_MODIFY -DUNIRTOS_COMMON_MODIFY -DUNIRTOS_OPEN_MODE_SUPPORT -DRAMCODE_COMPRESS_EN -DREL_COMPRESS_EN -DFEATURE_EXCEPTION_FLASH_DUMP_ENABLE -DOPEN_CPU_MODE -DTRACE_LEVEL=5 -DQL_CP_OC_BIN_MODE -DQL_AP_LIB_OC -DFEATURE_VPU_ENHANCED -DFEATURE_OS_ENABLE -DFEATURE_FOTAPAR_ENABLE -D__EC718 -DCORE_IS_AP -DQL_SSL_SUPPORT -DQL_RTOS_SUPPORT -DWDT_FEATURE_ENABLE=1 -DFEATURE_LZMA_ENABLE -DFEATURE_MBEDTLS_ENABLE -DEC_ASSERT_FLAG")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -DUNIRTOS_APPS_COMPILE")
add_compile_definitions(MBEDTLS_CONFIG_FILE=\"config-mini-tls1_3.h\")
add_compile_definitions(LWIP_CONFIG_FILE=\"lwip_config_cat.h\")
add_compile_definitions(SOFTPACK_VERSION=\"\")
add_compile_definitions(DEBUG_LOG_HEADER_FILE=\"debug_log_dummy.h\")
add_compile_definitions(${CHIP_TYPE_BUILD})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${ldscript} --specs=nosys.specs -Wl,--gc-sections -Wl,-Map=${AP_MAP} -Wl,--wrap=_malloc_r -Wl,--wrap=_free_r -Wl,--wrap=_realloc_r -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--wrap=calloc -Wl,--wrap=realloc -u _printf_float")

if(NOT unirtos_csdk_build STREQUAL "y")
# Add the source code to the executable file of this project.
add_executable(ap_application
    ${UNIRTOS_EIGEN_718}/main/unirtos_main.c
    ${UNIRTOS_STARTUP_DIR}/unirtos_init.c
)
endif()

# Each platform if no need to regenerate ld, can directly copy
function(ldscript_copy target)

    # Create folder
    execute_process( COMMAND ${CMAKE_COMMAND} -E make_directory ${BINARY_TOP_DIR}/output/ldscript)
    set(ld_file ${ARCH_GCCOUT_DIR}/scripts/ec7xx_0h00_flash_output.ld)
    message("ldscript_copy ${ld_file}")
    # Directly copy the platform-side ld to the specified path
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${ld_file} ${ldscript})

endfunction()



#***********************************************************************************************************************************************#
# qcx217 packaging process, ELF file converted to bin file via OBJDUMP, then packaged with platform-side bootloader.bin ap_at_command.bin ap_updater.bin cp-demo-flash.bin into .hbinpkg

# app executes when ELF is generated, for generating BIN file from ELF, need to consider whether to use platform tools for compression after generating BIN
function(add_appimg target)

    # Generate BIN file
    add_custom_command(
        OUTPUT ${AP_BIN}
        COMMAND ${CMAKE_OBJCOPY} -O binary ${AP_ELF} ${AP_BIN}
        COMMENT "OBJCOPY BIN"
        DEPENDS ${AP_ELF} ${AP_TXT}
    )

    # Generate disassembly and section table information
    add_custom_command(
        OUTPUT ${AP_TXT}
        COMMAND ${CMAKE_OBJDUMP} -d -h ${AP_ELF} > ${AP_TXT}
        COMMENT "Generating disassembly and section info"
        DEPENDS ${AP_ELF}
    )

    # Define final target dependencies
    add_custom_target(post_build ALL
        DEPENDS ${AP_BIN} ${AP_TXT}
    )

    # Preprocess mem_map.h
    #add_custom_command(
    #    OUTPUT ${MEM_MAP}
    #    COMMAND ${CMAKE_C_COMPILER} -E -P
    #        ${CMAKE_CURRENT_SOURCE_DIR}/system/platform/${build_arch}/boards/${build_project}/include/mem_map.h
    #        -o ${MEM_MAP}
    #    DEPENDS ${AP_ELF}
    #    COMMENT "Preprocessing mem_map.h"
    #)

    # Print target file size
    #add_custom_command(
    #    TARGET add_appimg
    #    POST_BUILD
    #    COMMAND ${CMAKE_SIZE} ${AP_ELF}
    #    COMMENT "Printing ELF size"
    #)

    # Add compressed binary target
    #add_custom_command(
    #    OUTPUT ${BUILDDIR}/${BINNAME}_compressed.bin
    #    COMMAND binCompress ${BIN_FILE} ${BUILDDIR}/${BINNAME}_compressed.bin
    #    COMMENT "Compressing binary"
    #    DEPENDS ${BIN_FILE}
    #)
endfunction()

# app packaging, when ap.bin is generated, needs to be packaged with bootloader.bin cp.bin, whether other bins are needed depends on the situation
function(pack_firmware target)
if(BINPKG_EXT_FS_ENABLED OR BINPKG_EXT_UPDATER_ENABLED)
    if(SECBOOT_SIGN)
        add_custom_command(
            OUTPUT ${OUTPUT_BINPKG}
            COMMAND ${SECURE_TOOL}
                    APIMAGE=${KERNEL_BIN}
                    CPIMAGE=${CP_BIN}
                    BLIMAGE=${BOOTLOADER_BIN}
                    BLHASH=1 SYSHASH=1
                    CURV_256_COMP=1
                    HEAD1=${BLSC_HEAD}
                    HEAD2=${SYSSC_HEAD}
                    UNIPEM=${SIGN_KEY}
                    CFGJSON=${SECURE_JSON}
                    SYSBNADR=82000
            COMMAND ${FCELF_TOOL} -M
                    -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                    -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                    -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                    -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                    ${BINPKG_EXT_IN_PACK}
                    -input ${UPDATERSC_HEAD} -addrname PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LIMIT_SIZE
                    -input ${APPSC_HEAD} -addrname PKGFLX_APP_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_APP_SEC_HEAD_PKGIMG_LIMIT_SIZE
                    -input ${BLSC_HEAD} -addrname XPKGDBLSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                    -input ${SYSSC_HEAD} -addrname XPKGDSYSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                    -input ${EC718_PRMGRCFG_SEC} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                    -input ${EC718_FORMAT_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                    -input ${EC718_PRD_UART_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                    -input ${EC718_PRD_USB_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                    -pkgmode 1
                    -banoldtool 1
                    -productname ${chip_type}_SEC
                    -def ${MEM_MAP}
                    -outfile ${OUTPUT_BINPKG}
            DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
            COMMENT "Generating .binpkg file using fcelf (ext+secboot)"
        )
    else()
    add_custom_command(
        OUTPUT ${OUTPUT_BINPKG}
        COMMAND ${FCELF_TOOL} -M
                -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                -input ${EC718_PRMGRCFG} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                -input ${EC718_FORMAT} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                -input ${EC718_PRD_UART} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                -input ${EC718_PRD_USB} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                ${BINPKG_EXT_IN_PACK}
                -pkgmode 1
                -banoldtool 1
                -productname ${chip_type}
                -def ${MEM_MAP}
                -outfile ${OUTPUT_BINPKG}
        DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
        COMMENT "Generating .binpkg file using fcelf (ext)"
    )
    endif()
elseif(BINPKG_CUS_APP2_ENABLED)
    add_custom_command(
        OUTPUT ${OUTPUT_BINPKG}
        COMMAND ${FCELF_TOOL} -M
                -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                -input ${UNIR_UPDATER} -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
                -input ${EC718_PRMGRCFG} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                -input ${EC718_FORMAT} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                -input ${EC718_PRD_UART} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                -input ${EC718_PRD_USB} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                -input ${EC718_CUS_APP2} -addrname PKGFLX_CUST_PKGIMG_LNA -flashsize PKGFLX_CUST_PKGIMG_LIMIT_SIZE
                -pkgmode 1
                -banoldtool 1
                -productname ${chip_type}
                -def ${MEM_MAP}
                -outfile ${OUTPUT_BINPKG}
        DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
        COMMENT "Generating .binpkg file using fcelf"
    )
else()
    if(SECBOOT_SIGN)
        if(ECDSA_P256_MODE)
            if(${build_project} STREQUAL "EG800ZCN_LG")
                add_custom_command(
                    OUTPUT ${OUTPUT_BINPKG}
                    COMMAND ${SECURE_TOOL}
                            APIMAGE=${KERNEL_BIN}
                            CPIMAGE=${CP_BIN}
                            BLIMAGE=${BOOTLOADER_BIN}
                            BLHASH=1 SYSHASH=1
                            CURV_256_COMP=1
                            HEAD1=${BLSC_HEAD}
                            HEAD2=${SYSSC_HEAD}
                            UNIPEM=${SIGN_KEY}
                            CFGJSON=${SECURE_JSON}
                            SYSBNADR=${SYSBNADR_VALUE}

                    COMMAND ${FCELF_TOOL} -M
                            -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                            -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                            -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                            -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                            -input ${UNIR_UPDATER} -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
                            -input ${GNSS_IMG} -addrname PKGFLX_GNSS_IMG_LNA -flashsize PKGFLX_GNSS_IMG_LIMIT_SIZE
                            -input ${UPDATERSC_HEAD} -addrname PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${APPSC_HEAD} -addrname PKGFLX_APP_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_APP_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${BLSC_HEAD} -addrname XPKGDBLSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${SYSSC_HEAD} -addrname XPKGDSYSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRMGRCFG_SEC} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_FORMAT_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_UART_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_USB_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -pkgmode 1
                            -banoldtool 1
                            -productname ${chip_type}_SEC
                            -def ${MEM_MAP}
                            -outfile ${OUTPUT_BINPKG}
                    DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
                    COMMENT "Generating .binpkg file using fcelf"
                )
            elseif(${build_project} STREQUAL "EG800ZEU_LG")
                add_custom_command(
                    OUTPUT ${OUTPUT_BINPKG}
                    COMMAND ${SECURE_TOOL}
                            APIMAGE=${KERNEL_BIN}
                            CPIMAGE=${CP_BIN}
                            BLIMAGE=${BOOTLOADER_BIN}
                            BLHASH=1 SYSHASH=1
                            CURV_256_COMP=1
                            HEAD1=${BLSC_HEAD}
                            HEAD2=${SYSSC_HEAD}
                            UNIPEM=${SIGN_KEY}
                            CFGJSON=${SECURE_JSON}
                            SYSBNADR=${SYSBNADR_VALUE}

                    COMMAND ${FCELF_TOOL} -M
                            -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                            -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                            -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                            -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                            -input ${UNIR_UPDATER} -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
                            -input ${GNSS_IMG} -addrname PKGFLX_GNSS_IMG_LNA -flashsize PKGFLX_GNSS_IMG_LIMIT_SIZE
                            -input ${UPDATERSC_HEAD} -addrname PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${APPSC_HEAD} -addrname PKGFLX_APP_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_APP_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${BLSC_HEAD} -addrname XPKGDBLSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${SYSSC_HEAD} -addrname XPKGDSYSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRMGRCFG_SEC} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_FORMAT_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_UART_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_USB_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -pkgmode 1
                            -banoldtool 1
                            -productname ${chip_type}_SEC
                            -def ${MEM_MAP}
                            -outfile ${OUTPUT_BINPKG}
                    DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
                    COMMENT "Generating .binpkg file using fcelf"
                )
            else()
                add_custom_command(
                    OUTPUT ${OUTPUT_BINPKG}
                    COMMAND ${SECURE_TOOL}
                            APIMAGE=${KERNEL_BIN}
                            CPIMAGE=${CP_BIN}
                            BLIMAGE=${BOOTLOADER_BIN}
                            BLHASH=1 SYSHASH=1
                            CURV_256_COMP=1
                            HEAD1=${BLSC_HEAD}
                            HEAD2=${SYSSC_HEAD}
                            UNIPEM=${SIGN_KEY}
                            CFGJSON=${SECURE_JSON}
                            SYSBNADR=${SYSBNADR_VALUE}

                    COMMAND ${FCELF_TOOL} -M
                            -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                            -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                            -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                            -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                            -input ${UNIR_UPDATER} -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
                            -input ${UPDATERSC_HEAD} -addrname PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${APPSC_HEAD} -addrname PKGFLX_APP_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_APP_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${BLSC_HEAD} -addrname XPKGDBLSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${SYSSC_HEAD} -addrname XPKGDSYSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRMGRCFG_SEC} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_FORMAT_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_UART_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_USB_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -pkgmode 1
                            -banoldtool 1
                            -productname ${chip_type}_SEC
                            -def ${MEM_MAP}
                            -outfile ${OUTPUT_BINPKG}
                    DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
                COMMENT "Generating .binpkg file using fcelf"
            )
            endif()
        else() #ECDSA_P256_MODE
            if(${build_project} STREQUAL "EG800ZCN_LG")
                add_custom_command(
                    OUTPUT ${OUTPUT_BINPKG}
                    COMMAND ${SECURE_TOOL}
                            APIMAGE=${KERNEL_BIN}
                            CPIMAGE=${CP_BIN}
                            BLIMAGE=${BOOTLOADER_BIN}
                            BLHASH=1 SYSHASH=1
                            HEAD1=${BLSC_HEAD}
                            HEAD2=${SYSSC_HEAD}
                            UNIPEM=${SIGN_KEY}
                            CFGJSON=${SECURE_JSON}
                            SYSBNADR=${SYSBNADR_VALUE}

                    COMMAND ${FCELF_TOOL} -M
                            -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                            -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                            -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                            -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                            -input ${UNIR_UPDATER} -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
                            -input ${GNSS_IMG} -addrname PKGFLX_GNSS_IMG_LNA -flashsize PKGFLX_GNSS_IMG_LIMIT_SIZE
                            -input ${UPDATERSC_HEAD} -addrname PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${APPSC_HEAD} -addrname PKGFLX_APP_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_APP_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${BLSC_HEAD} -addrname XPKGDBLSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${SYSSC_HEAD} -addrname XPKGDSYSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRMGRCFG_SEC} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_FORMAT_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_UART_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_USB_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -pkgmode 1
                            -banoldtool 1
                            -productname ${chip_type}_SEC
                            -def ${MEM_MAP}
                            -outfile ${OUTPUT_BINPKG}
                    DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
                    COMMENT "Generating .binpkg file using fcelf"
                )
            elseif(${build_project} STREQUAL "EG800ZEU_LG")
                add_custom_command(
                    OUTPUT ${OUTPUT_BINPKG}
                    COMMAND ${SECURE_TOOL}
                            APIMAGE=${KERNEL_BIN}
                            CPIMAGE=${CP_BIN}
                            BLIMAGE=${BOOTLOADER_BIN}
                            BLHASH=1 SYSHASH=1
                            HEAD1=${BLSC_HEAD}
                            HEAD2=${SYSSC_HEAD}
                            UNIPEM=${SIGN_KEY}
                            CFGJSON=${SECURE_JSON}
                            SYSBNADR=${SYSBNADR_VALUE}

                    COMMAND ${FCELF_TOOL} -M
                            -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                            -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                            -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                            -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                            -input ${UNIR_UPDATER} -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
                            -input ${GNSS_IMG} -addrname PKGFLX_GNSS_IMG_LNA -flashsize PKGFLX_GNSS_IMG_LIMIT_SIZE
                            -input ${UPDATERSC_HEAD} -addrname PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${APPSC_HEAD} -addrname PKGFLX_APP_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_APP_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${BLSC_HEAD} -addrname XPKGDBLSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${SYSSC_HEAD} -addrname XPKGDSYSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRMGRCFG_SEC} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_FORMAT_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_UART_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_USB_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -pkgmode 1
                            -banoldtool 1
                            -productname ${chip_type}_SEC
                            -def ${MEM_MAP}
                            -outfile ${OUTPUT_BINPKG}
                    DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
                    COMMENT "Generating .binpkg file using fcelf"
                )
            else()
                add_custom_command(
                    OUTPUT ${OUTPUT_BINPKG}
                    COMMAND ${SECURE_TOOL}
                            APIMAGE=${KERNEL_BIN}
                            CPIMAGE=${CP_BIN}
                            BLIMAGE=${BOOTLOADER_BIN}
                            BLHASH=1 SYSHASH=1
                            HEAD1=${BLSC_HEAD}
                            HEAD2=${SYSSC_HEAD}
                            UNIPEM=${SIGN_KEY}
                            CFGJSON=${SECURE_JSON}
                            SYSBNADR=${SYSBNADR_VALUE}

                    COMMAND ${FCELF_TOOL} -M
                            -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                            -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                            -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                            -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                            -input ${UNIR_UPDATER} -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
                            -input ${UPDATERSC_HEAD} -addrname PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${APPSC_HEAD} -addrname PKGFLX_APP_SEC_HEAD_PKGIMG_LNA -flashsize PKGFLX_APP_SEC_HEAD_PKGIMG_LIMIT_SIZE
                            -input ${BLSC_HEAD} -addrname XPKGDBLSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${SYSSC_HEAD} -addrname XPKGDSYSH_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRMGRCFG_SEC} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_FORMAT_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_UART_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -input ${EC718_PRD_USB_SEC} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                            -pkgmode 1
                            -banoldtool 1
                            -productname ${chip_type}_SEC
                            -def ${MEM_MAP}
                            -outfile ${OUTPUT_BINPKG}
                    DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
                    COMMENT "Generating .binpkg file using fcelf"
            )
            endif()
        endif() #ECDSA_P256_MODE
    else() #SECBOOT_SIGN
        if(${build_project} STREQUAL "EG800ZCN_LG")
            add_custom_command(
                OUTPUT ${OUTPUT_BINPKG}
                COMMAND ${FCELF_TOOL} -M
                        -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                        -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                        -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                        -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                        -input ${UNIR_UPDATER} -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
                        -input ${GNSS_IMG} -addrname PKGFLX_GNSS_IMG_LNA -flashsize PKGFLX_GNSS_IMG_LIMIT_SIZE
                        -input ${EC718_PRMGRCFG} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -input ${EC718_FORMAT} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -input ${EC718_PRD_UART} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -input ${EC718_PRD_USB} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -pkgmode 1
                        -banoldtool 1
                        -productname ${chip_type}
                        -def ${MEM_MAP}
                        -outfile ${OUTPUT_BINPKG}
                DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
                COMMENT "Generating .binpkg file using fcelf"
            )
        elseif(${build_project} STREQUAL "EG800ZEU_LG")
            add_custom_command(
                OUTPUT ${OUTPUT_BINPKG}
                COMMAND ${FCELF_TOOL} -M
                        -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                        -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                        -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                        -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                        -input ${UNIR_UPDATER} -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
                        -input ${GNSS_IMG} -addrname PKGFLX_GNSS_IMG_LNA -flashsize PKGFLX_GNSS_IMG_LIMIT_SIZE
                        -input ${EC718_PRMGRCFG} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -input ${EC718_FORMAT} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -input ${EC718_PRD_UART} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -input ${EC718_PRD_USB} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -pkgmode 1
                        -banoldtool 1
                        -productname ${chip_type}
                        -def ${MEM_MAP}
                        -outfile ${OUTPUT_BINPKG}
                DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
                COMMENT "Generating .binpkg file using fcelf"
            )
        else()
            add_custom_command(
                OUTPUT ${OUTPUT_BINPKG}
                COMMAND ${FCELF_TOOL} -M
                        -input ${BOOTLOADER_BIN} -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE
                        -input ${KERNEL_BIN} -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE
                        -input ${CP_BIN} -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE
                        -input ${AP_BIN} -addrname PKGFLX_APP_PKGIMG_LNA -flashsize PKGFLX_APP_PKGIMG_LIMIT_SIZE
                        -input ${UNIR_UPDATER} -addrname PKGFLX_UPDATER_PKGIMG_LNA -flashsize PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE
                        -input ${EC718_PRMGRCFG} -addrname XPKGDPRM_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -input ${EC718_FORMAT} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -input ${EC718_PRD_UART} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -input ${EC718_PRD_USB} -addrname XPKGDCMN_VIRTIMG_MERGE_LNA -flashsize XPKG_VIRTIMG_LOAD_SIZE
                        -pkgmode 1
                        -banoldtool 1
                        -productname ${chip_type}
                        -def ${MEM_MAP}
                        -outfile ${OUTPUT_BINPKG}
                DEPENDS ${BOOTLOADER_BIN} ${AP_BIN} ${CP_BIN}
                COMMENT "Generating .binpkg file using fcelf"
            )
        endif()
    endif()#SECBOOT_SIGN

endif()
    set(merge_target firmware)
    add_custom_target(${merge_target} DEPENDS ${OUTPUT_BINPKG})

    # 7z extraction operation (standalone target)
    # set(OUTPUT_DIR "${SOURCE_TOP_DIR}/qos_build/release/${build_version}")

    # Set directory variable
    set(OUTPUT_DIR "${OUTPUT_RELEASE_DIR}/${build_version}")

    add_custom_target(unpack_firmware
        # Call clean_release.cmake script to clean subdirectories under OUTPUT_RELEASE_DIR
        COMMAND ${CMAKE_COMMAND} -DOUTPUT_RELEASE_DIR="${OUTPUT_RELEASE_DIR}"
                                -Dbuild_version="${build_version}"
                                -P "${UNIRTOS_DIR}/cmake/clean_release.cmake"
        # Recreate empty OUTPUT_RELEASE_DIR and build_version directories
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_RELEASE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
        # Copy
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${FIRMWARE_DIR} ${OUTPUT_DIR}
        # Remove temporary directory
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${OUTPUT_DIR}/tmp
        COMMENT "Clean and extract to ${OUTPUT_DIR}"
        DEPENDS ${merge_target}
    )

    # File copy operation (standalone target)
    add_custom_target(copy_binpkg
        COMMAND ${CMAKE_COMMAND} -E copy "${OUTPUT_BINPKG}" "${OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "${AP_BIN}" "${OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "${AP_MAP}" "${OUTPUT_DIR}/DBG"
        COMMAND ${CMAKE_COMMAND} -E copy "${AP_ELF}" "${OUTPUT_DIR}/DBG"
        COMMENT "Copying binpkg to output directory"
        DEPENDS unpack_firmware  # Ensure execution after extraction
    )

    # Final integration dependencies
    add_dependencies(app_pac copy_binpkg)
endfunction()
