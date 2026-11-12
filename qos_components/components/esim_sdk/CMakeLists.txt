message(STATUS "cmake config ${CMAKE_CURRENT_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# eSIM SDK component
#
# components/CMakeLists.txt adds this directory only when CONFIG_QCM_ESIM_FUNC
# is on: an OPEN/CSDK build gets the switch from
# opt_cust/OCPU/customer.config (merged as the OPEN layer), the standard build
# gets it from its OEM package. libesim_sdk.a is always built here from the four
# HAL sources; only the closed core library libunirtos_esim_core.a ships
# prebuilt. The SDK packaging drops libesim_sdk.a from com_libs (the component is
# on the open source list), so a customer build compiles these four sources again
# against the three public headers and links the prebuilt core library.
# ---------------------------------------------------------------------------

set(target esim_sdk)

add_library(${target} STATIC)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_unir_lib_dir})

target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR})
target_include_directories(${target} PRIVATE ${SOURCE_SYS_DEBUG_DIR})
target_include_directories(${target} PRIVATE ${SOURCE_SYS_OS_DIR})
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hal/inc)
# qurl / vtls / utils are opened side by side with this component: resolve them
# relative to the components directory, which is the same in the R02 tree
# (quecos/components) and in the delivered SDK (qos_components/components).
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../qurl/include)
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../vtls/public)
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../utils/utils/public)
target_include_directories(${target} PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/public)

target_sources(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/src/esim_hal_apdu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/src/esim_hal_fs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/src/esim_hal_http.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/src/esim_hal_platform.c
)

target_link_libraries_if(CONFIG_QOSA_LINK_SYSTEM_LIBRARIES_FUNC THEN ${target} system_os)
# qurl/utils come from the QCM_QURL_FUNC / QCM_UTILS_UTILS_FUNC symbols that
# QCM_ESIM_FUNC selects (components/Kconfig:220-225). Check the symbols, not
# if(TARGET ...): both component directories are added later in
# components/CMakeLists.txt (qurl:7-9, utils:167), so a TARGET test here is
# always false at configure time. A plain name is resolved at generate time.
if(NOT CONFIG_QCM_QURL_FUNC OR NOT CONFIG_QCM_UTILS_UTILS_FUNC)
    message(FATAL_ERROR "esim_sdk: CONFIG_QCM_QURL_FUNC or CONFIG_QCM_UTILS_UTILS_FUNC is off, check the QCM_ESIM_FUNC select chain")
endif()
target_link_libraries(${target} qurl utils)

target_link_libraries_if(CONFIG_QCM_VTLS_FUNC THEN ${target} vtls)

if(NOT CONFIG_QCM_VTLS_FUNC)
    # esim_hal_http.c compiles its TLS part only under CONFIG_QCM_VTLS_FUNC:
    # without VTLS the HTTPS client runs with no TLS configuration at all.
    message(WARNING "esim_sdk: CONFIG_QCM_VTLS_FUNC is off, the eSIM HTTPS client has no TLS configuration")
endif()

# ---------------------------------------------------------------------------
# Prebuilt eSIM core library (closed source, one binary per platform).
#
# R02 source tree: components/esim_sdk/lib/<platform> holds the binary. It is
# copied verbatim into the library output directory, from where the packaging
# step collects every *.a and ships it unchanged.
#
# Delivered SDK: lib/ is not part of the open source tree (unirtos_sdk.py
# OPEN_SOURCE_COMPONENT_EXCLUDED_DIRS excludes it), the binary is prebuilt in
# qos_kernel/<chip>/com_libs and qos_kernel/kernel.cmake links everything it
# finds there, so nothing has to be staged or added to the link line here.
# ---------------------------------------------------------------------------
set(ESIM_CORE_LIB_SRC "")
if(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
    set(ESIM_CORE_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/lib/EC718_PM/libunirtos_esim_core.a)
elseif(CONFIG_QOSA_ASR160X_PLATFORM_FUNC OR CONFIG_QOSA_ASR1805_PLATFORM_FUNC OR CONFIG_QOSA_ASR1903_PLATFORM_FUNC)
    set(ESIM_CORE_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/lib/ASR/libunirtos_esim_core.a)
elseif(CONFIG_PLATFORM_FUNC_NONE)
    # OPEN/CSDK package: PLATFORM_FUNC_NONE=y leaves every QOSA_*_PLATFORM_FUNC
    # symbol off by design (the package is built against prebuilt platform
    # libraries) and lib/ is excluded from the open source tree, so there is
    # nothing to stage here: the core library already sits in
    # qos_kernel/<chip>/com_libs and kernel.cmake links it unconditionally.
    message(STATUS "esim_sdk: OPEN/CSDK build (PLATFORM_FUNC_NONE), the prebuilt core library is provided by qos_kernel/<chip>/com_libs")
else()
    message(FATAL_ERROR "esim_sdk: there is no prebuilt eSIM core library for the current platform")
endif()

if(ESIM_CORE_LIB_SRC AND EXISTS ${ESIM_CORE_LIB_SRC})
    set(ESIM_CORE_LIB_DST ${out_unir_lib_dir}/libunirtos_esim_core.a)

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ESIM_CORE_LIB_SRC} ${ESIM_CORE_LIB_DST}
        COMMENT "Copying eSIM core library to output directory..."
    )

    add_library(esim_sdk_core STATIC IMPORTED)
    set_target_properties(esim_sdk_core PROPERTIES IMPORTED_LOCATION ${ESIM_CORE_LIB_SRC})
    target_link_libraries(${target} esim_sdk_core)
elseif(ESIM_CORE_LIB_SRC)
    message(STATUS "esim_sdk: ${ESIM_CORE_LIB_SRC} is not in the delivered tree, the prebuilt core library comes from com_libs")
endif()

# ---------------------------------------------------------------------------
# Register the library for the firmware link. It repeats the CONFIG_QCM_ESIM_FUNC
# gate that components/CMakeLists.txt already applies, so a build with the switch
# off neither compiles nor links eSIM.
# ---------------------------------------------------------------------------
if(CONFIG_QCM_ESIM_FUNC)
    add_components_libraries($<TARGET_FILE:${target}>)
endif()
