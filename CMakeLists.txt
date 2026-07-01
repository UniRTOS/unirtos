# CMakeList.txt: CMake project for new_QS SDK
#
cmake_minimum_required (VERSION 3.8)

# If supported, enable hot reload for MSVC compiler.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if(NOT DEFINED ENV{CMAKE_SYSTEM_NAME} OR "$ENV{CMAKE_SYSTEM_NAME}" STREQUAL "")

    # If it's a linux environment, directly load sub-files
    message(STATUS "Loading Linux configuration")
    include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists_linux.txt)

else()
    # cmake under windows environment does not support ninja, use include to reference other cmake named files

set(UNIRTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# $ENV gets environment variables, set by upper-level scripts
# Model name EC600UCN_LB
set(build_project $ENV{build_project})
# Platform name Unisoc
set(build_arch $ENV{build_arch})
set(COMPILER_PATH $ENV{COMPILER_PATH})
# Version name
set(build_version $ENV{build_version})
# Link dependent lib libraries, add platform dependencies to this item
set(MY_LINK_LIBRARIES ${MY_LINK_LIBRARIES})
# Packaging tool directory
set(tools_dir ${CMAKE_CURRENT_SOURCE_DIR}/qos_tools/${build_arch})
set(build_tasknum $ENV{build_tasknum})
set(python_local $ENV{UNIRTOS_TOOLCHAIN_PATH})
set(7Z_EXE ${python_local}/7z/7z.exe)
set(unirtos_csdk_build  $ENV{unirtos_csdk_build})
if(DEFINED ENV{UNIRTOS_OUTPUT_ROOT} AND NOT "$ENV{UNIRTOS_OUTPUT_ROOT}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{UNIRTOS_OUTPUT_ROOT}" UNIRTOS_OUTPUT_ROOT)
    get_filename_component(UNIRTOS_OUTPUT_ROOT "${UNIRTOS_OUTPUT_ROOT}" ABSOLUTE)
    set(UNIRTOS_BUILD_OUT ${UNIRTOS_OUTPUT_ROOT}/qos_build)
else()
    set(UNIRTOS_BUILD_OUT ${UNIRTOS_DIR}/qos_build)
endif()

set(CMAKE_SYSTEM_NAME           Generic)
set(CMAKE_SYSTEM_PROCESSOR          ARM)
# Set cross-compilation tools
set(CMAKE_C_COMPILER   $ENV{COMPILER_CC})
set(CMAKE_CXX_COMPILER $ENV{COMPILER_CXX})
set(CMAKE_READELF      $ENV{COMPILER_READELF})

# Open detailed compilation information
set(CMAKE_VERBOSE_MAKEFILE ON)

message("========COMPILER_PATH: ${COMPILER_PATH}")
message("========CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message("========CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message("========CMAKE_READELF: ${CMAKE_READELF}")

# Disable compiler checking, don't detect compiler by compiling a program, cross-compilation cannot successfully compile a program with default compilation parameters
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)
set(WITH_WERROR ON)


# LANGUAGES option: select programming languages needed for building project. If using ASM (assembly), put it at the end
project (${build_project} LANGUAGES C CXX  ASM)

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

# Load rule directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/extension.cmake) # Load extension library

# Add global all components libraries definition, used to collect all application libraries
define_property(GLOBAL PROPERTY components_libraries
    BRIEF_DOCS "components libraries"
    FULL_DOCS "components libraries"
)

# Add global all apps libraries definition, used to collect all application libraries
define_property(GLOBAL PROPERTY apps_libraries
    BRIEF_DOCS "apps libraries"
    FULL_DOCS "apps libraries"
)

# Libraries that should be linked into final firmware but are not app targets.
define_property(GLOBAL PROPERTY firmware_link_libraries
    BRIEF_DOCS "firmware link libraries"
    FULL_DOCS "firmware link libraries"
)

# Add compilation options
# TODO: Later can define configuration macros to enable/disable based on different projects
set(CXX_STACK_PROTECTOR "-fstack-protector-all")
# Reduce target file size
set(CXX_FLAGS_DEBUG "-Os")

set(SOURCE_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(BINARY_TOP_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(out_inc_dir ${CMAKE_CURRENT_BINARY_DIR}/include)
# Optional external app injection (SDK-driven build from user app dir)
if(NOT DEFINED UNIRTOS_EXTERNAL_APP_DIR)
    if(DEFINED ENV{UNIRTOS_EXTERNAL_APP_DIR} AND NOT "$ENV{UNIRTOS_EXTERNAL_APP_DIR}" STREQUAL "")
        set(UNIRTOS_EXTERNAL_APP_DIR "$ENV{UNIRTOS_EXTERNAL_APP_DIR}")
    endif()
endif()

if(NOT DEFINED UNIRTOS_EXTERNAL_APP_NAME)
    if(DEFINED ENV{UNIRTOS_EXTERNAL_APP_NAME} AND NOT "$ENV{UNIRTOS_EXTERNAL_APP_NAME}" STREQUAL "")
        set(UNIRTOS_EXTERNAL_APP_NAME "$ENV{UNIRTOS_EXTERNAL_APP_NAME}")
    endif()
endif()

if(NOT DEFINED UNIRTOS_APP_MENUCONFIG_DIR)
    if(DEFINED ENV{UNIRTOS_APP_MENUCONFIG_DIR} AND NOT "$ENV{UNIRTOS_APP_MENUCONFIG_DIR}" STREQUAL "")
        set(UNIRTOS_APP_MENUCONFIG_DIR "$ENV{UNIRTOS_APP_MENUCONFIG_DIR}")
    endif()
endif()

if(DEFINED UNIRTOS_EXTERNAL_APP_DIR)
    file(TO_CMAKE_PATH "${UNIRTOS_EXTERNAL_APP_DIR}" UNIRTOS_EXTERNAL_APP_DIR)
    get_filename_component(UNIRTOS_EXTERNAL_APP_DIR "${UNIRTOS_EXTERNAL_APP_DIR}" ABSOLUTE)
    if(EXISTS ${UNIRTOS_EXTERNAL_APP_DIR}/CMakeLists.txt)
        if(NOT DEFINED UNIRTOS_EXTERNAL_APP_NAME OR "${UNIRTOS_EXTERNAL_APP_NAME}" STREQUAL "")
            get_filename_component(UNIRTOS_EXTERNAL_APP_NAME ${UNIRTOS_EXTERNAL_APP_DIR} NAME)
        endif()
        message(STATUS "INFO: External app mode enabled: ${UNIRTOS_EXTERNAL_APP_DIR}")
    else()
        message(FATAL_ERROR "UNIRTOS_EXTERNAL_APP_DIR is set but CMakeLists.txt not found: ${UNIRTOS_EXTERNAL_APP_DIR}")
    endif()
endif()

if(DEFINED UNIRTOS_EXTERNAL_APP_DIR)
    message(STATUS "[build-mode] external-app")
else()
    message(STATUS "[build-mode] builtin-demo")
endif()

# =========================================================================
# External Libraries Support (SDK-integrated build)
# =========================================================================
# Libraries are discovered from env_config.json via UNIRTOS_LIBRARIES_JSON
# and compiled as part of the SDK build process (like external apps)

function(discover_and_link_external_libraries)
    set(UNIRTOS_EXTERNAL_LIBRARY_TARGETS "" PARENT_SCOPE)
    set_property(GLOBAL PROPERTY unirtos_external_library_targets "")

    # Read libraries from UNIRTOS_LIBRARIES_JSON environment variable
    if(NOT DEFINED ENV{UNIRTOS_LIBRARIES_JSON})
        return()  # No libraries to process
    endif()
    
    set(libraries_json "$ENV{UNIRTOS_LIBRARIES_JSON}")
    if("${libraries_json}" STREQUAL "")
        return()  # Empty library list
    endif()
    
    # Resolve unirtos root (same logic as build.py)
    # Priority: UNIRTOS_ROOT env var > HOME/.unirtos > defaults
    set(unirtos_root)
    if(DEFINED ENV{UNIRTOS_ROOT})
        file(TO_CMAKE_PATH "$ENV{UNIRTOS_ROOT}" unirtos_root)
    else()
        if(WIN32)
            # Windows: use USERPROFILE fallback
            if(DEFINED ENV{USERPROFILE})
                set(unirtos_root "$ENV{USERPROFILE}/.unirtos")
            else()
                set(unirtos_root "$ENV{HOME}/.unirtos")
            endif()
        else()
            # Unix-like: use HOME/.unirtos
            set(unirtos_root "$ENV{HOME}/.unirtos")
        endif()
    endif()
    
    get_filename_component(unirtos_root "${unirtos_root}" ABSOLUTE)
    message(STATUS "[external-libs] root: ${unirtos_root}")
    string(JSON libraries_count ERROR_VARIABLE libraries_json_error LENGTH "${libraries_json}")
    if(NOT libraries_json_error STREQUAL "NOTFOUND")
        message(FATAL_ERROR "Invalid UNIRTOS_LIBRARIES_JSON: ${libraries_json_error}")
    endif()

    if(libraries_count EQUAL 0)
        return()
    endif()

    set(external_library_targets)
    math(EXPR libraries_last_index "${libraries_count} - 1")
    foreach(lib_index RANGE ${libraries_last_index})
        string(JSON lib_name ERROR_VARIABLE lib_name_error GET "${libraries_json}" ${lib_index} name)
        if(NOT lib_name_error STREQUAL "NOTFOUND")
            message(FATAL_ERROR "Invalid library entry at index ${lib_index}: missing or invalid 'name'")
        endif()

        string(JSON lib_version ERROR_VARIABLE lib_version_error GET "${libraries_json}" ${lib_index} version)
        if(NOT lib_version_error STREQUAL "NOTFOUND")
            message(FATAL_ERROR "Invalid library entry at index ${lib_index}: missing or invalid 'version'")
        endif()

        set(lib_version_dir "v${lib_version}")
        message(STATUS "[external-libs] discovered: ${lib_name}/${lib_version_dir}")

        # Libraries are pulled by env-setup into ~/.unirtos/libraries/<lib_name>/v<lib_version>/
        set(lib_source_dir "${unirtos_root}/libraries/${lib_name}/${lib_version_dir}")
        set(lib_cmake "${lib_source_dir}/CMakeLists.txt")

        if(EXISTS "${lib_cmake}")
            message(STATUS "[external-libs] source: ${lib_source_dir}")

            set(LIB_NAME "${lib_name}")
            set(UNIRTOS_EXTERNAL_LIBRARY_NAME "${lib_name}")
            set(UNIRTOS_EXTERNAL_LIBRARY_VERSION "${lib_version}")

            # Output will be in: app_dir/qos_build/build/libs/<lib_name>/v<lib_version>/
            add_subdirectory("${lib_source_dir}" "${CMAKE_CURRENT_BINARY_DIR}/libs/${lib_name}/${lib_version_dir}")

            if(TARGET ${lib_name})
                add_firmware_link_libraries($<TARGET_FILE:${lib_name}>)
                list(APPEND external_library_targets ${lib_name})
                message(STATUS "[firmware-link] registered external library target: ${lib_name}")
            else()
                message(FATAL_ERROR "External library '${lib_name}' must define a CMake target named '${lib_name}'")
            endif()

            unset(LIB_NAME)
            unset(UNIRTOS_EXTERNAL_LIBRARY_NAME)
            unset(UNIRTOS_EXTERNAL_LIBRARY_VERSION)
        else()
            message(WARNING "Library CMakeLists.txt not found: ${lib_cmake}, skipping")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES external_library_targets)
    set_property(GLOBAL PROPERTY unirtos_external_library_targets "${external_library_targets}")
    set(UNIRTOS_EXTERNAL_LIBRARY_TARGETS "${external_library_targets}" PARENT_SCOPE)
endfunction()

# Discover and configure external libraries early (before they're needed)
discover_and_link_external_libraries()
# App storage
set(BINARY_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
#demo PATH
set(QL_DEMO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/qos_applications)
#kernel PATH
set(QL_KERNEL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/qos_kernel)
set(QL_COMPONENTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/qos_components)
set(UNIRTOS_COMPONENTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/qos_components/components)
# Final packaged firmware path
set(FIRMWARE_DIR ${CMAKE_CURRENT_BINARY_DIR}/output)
set(system_dir ${CMAKE_CURRENT_SOURCE_DIR}/qos_components/system)
# Create folder
execute_process( COMMAND ${CMAKE_COMMAND} -E make_directory ${BINARY_BIN_DIR})
execute_process( COMMAND ${CMAKE_COMMAND} -E make_directory ${FIRMWARE_DIR}/script)

# Set linking script and map output
set(ldscript ${FIRMWARE_DIR}/script/ldscript.ld)
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${ldscript} -Wl,-Map=${BINARY_BIN_DIR}/${build_project}.map")

# Set output directory for generated libs
set(out_unir_lib_dir ${CMAKE_CURRENT_BINARY_DIR}/unir_libs)
# Set path for tools utility scripts
set(tools_dir ${CMAKE_CURRENT_SOURCE_DIR}/qos_tools)

# Parse the .config file generated by menuconfig
# Set to generate .cmake file usable by cmake from .config and include it
set(SRC_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/.config)
set(EFFECTIVE_CONFIG ${BINARY_TOP_DIR}/effective.config)
set(TARGET_CONFIG ${BINARY_TOP_DIR}/target.config)
set(CHECK_CONFIG ${BINARY_TOP_DIR}/check.config)
set(KCONFIG_MODEM ${QL_COMPONENTS_DIR}/system/modem/Kconfig.modem)
set(QOS_APPLICATIONS_DIR ${QL_DEMO_DIR})
# Builtin demo selection/depend merge is only enabled in builtin mode.
set(SELECTED_APP_DEMO_DEPEND_CONFIGS)
set(SELECTED_APP_DEMO_CONFIGURE_DEPENDS)
if(NOT DEFINED UNIRTOS_EXTERNAL_APP_DIR)
    set(APP_INIT_ENTRIES ${QOS_APPLICATIONS_DIR}/app_init/apps_init_entries.def)
    resolve_builtin_demo_selection(
        ${QOS_APPLICATIONS_DIR}
        ${APP_INIT_ENTRIES}
        ${build_arch}
        ${BINARY_TOP_DIR}/qos_applications
        ${py_exe}
        ${tools_dir}
        ${UNIRTOS_DIR}
    )
endif()

collect_qos_application_projects(${QOS_APPLICATIONS_DIR} EXTERNAL_QOS_APPLICATION_DIRS)
if(DEFINED UNIRTOS_EXTERNAL_APP_DIR)
    list(APPEND EXTERNAL_QOS_APPLICATION_DIRS ${UNIRTOS_EXTERNAL_APP_DIR})
endif()
set(EXTERNAL_APP_DEPEND_CONFIGS)
foreach(external_app_dir ${EXTERNAL_QOS_APPLICATION_DIRS})
    if(IS_ABSOLUTE ${external_app_dir})
        set(external_app_depend_config ${external_app_dir}/depend.config)
    else()
        set(external_app_depend_config ${QOS_APPLICATIONS_DIR}/${external_app_dir}/depend.config)
    endif()

    if(EXISTS ${external_app_depend_config})
        list(APPEND EXTERNAL_APP_DEPEND_CONFIGS ${external_app_depend_config})
    endif()
endforeach()

# Resolve menuconfig source config path with the following priority:
# 1) KCONFIG_CONFIG environment variable
# 2) UNIRTOS_APP_MENUCONFIG_DIR (CLI/build injected)
# 3) External app default: <external_app>/menuconfig/.config
# 4) Builtin demo default when exactly one selected build dir exists: qos_applications/<demo>/menuconfig/.config
# 5) SDK root .config
set(RESOLVED_SRC_CONFIG "")
if(DEFINED ENV{KCONFIG_CONFIG} AND NOT "$ENV{KCONFIG_CONFIG}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{KCONFIG_CONFIG}" RESOLVED_SRC_CONFIG)
elseif(DEFINED UNIRTOS_APP_MENUCONFIG_DIR AND NOT "${UNIRTOS_APP_MENUCONFIG_DIR}" STREQUAL "")
    file(TO_CMAKE_PATH "${UNIRTOS_APP_MENUCONFIG_DIR}" APP_MENUCONFIG_DIR)
    if(NOT IS_ABSOLUTE "${APP_MENUCONFIG_DIR}")
        get_filename_component(APP_MENUCONFIG_DIR "${APP_MENUCONFIG_DIR}" ABSOLUTE)
    endif()
    set(RESOLVED_SRC_CONFIG "${APP_MENUCONFIG_DIR}/.config")
elseif(DEFINED UNIRTOS_EXTERNAL_APP_DIR)
    set(RESOLVED_SRC_CONFIG "${UNIRTOS_EXTERNAL_APP_DIR}/menuconfig/.config")
elseif(DEFINED SELECTED_APP_BUILD_DIRS)
    list(LENGTH SELECTED_APP_BUILD_DIRS SELECTED_APP_BUILD_DIRS_LEN)
    if(SELECTED_APP_BUILD_DIRS_LEN EQUAL 1)
        list(GET SELECTED_APP_BUILD_DIRS 0 SINGLE_SELECTED_APP_DIR)
        set(RESOLVED_SRC_CONFIG "${QOS_APPLICATIONS_DIR}/${SINGLE_SELECTED_APP_DIR}/menuconfig/.config")
    endif()
endif()

if(NOT "${RESOLVED_SRC_CONFIG}" STREQUAL "")
    file(TO_CMAKE_PATH "${RESOLVED_SRC_CONFIG}" SRC_CONFIG)
endif()

if(NOT IS_ABSOLUTE "${SRC_CONFIG}")
    get_filename_component(SRC_CONFIG "${SRC_CONFIG}" ABSOLUTE)
endif()

get_filename_component(SRC_CONFIG_DIR "${SRC_CONFIG}" DIRECTORY)
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${SRC_CONFIG_DIR})
set(ENV{KCONFIG_CONFIG} "${SRC_CONFIG}")
message(STATUS "INFO: menuconfig source config path: ${SRC_CONFIG}")


message("toolchain.cmake=${QL_KERNEL_DIR}/${build_arch}/scripts/toolchain.cmake")
# Load custom functions for each platform
include(${QL_KERNEL_DIR}/${build_arch}/scripts/toolchain.cmake)

message("build_arch version: ${build_arch}")
message("build_project version: ${build_project}")
message("build_project version: ${SRC_CONFIG}")
message("MODEM_SRC_CONFIG: ${MODEM_SRC_CONFIG}")
message("SYSTEM_SRC_CONFIG version: ${SYSTEM_SRC_CONFIG}")
message("CHECK_CONFIG: ${CHECK_CONFIG}")


# Bootstrap root .config from Kconfig defaults when it is missing or empty.
set(BOOTSTRAP_ROOT_CONFIG OFF)
if(NOT EXISTS ${SRC_CONFIG})
    set(BOOTSTRAP_ROOT_CONFIG ON)
else()
    file(SIZE ${SRC_CONFIG} SRC_CONFIG_SIZE)
    if(SRC_CONFIG_SIZE EQUAL 0)
        set(BOOTSTRAP_ROOT_CONFIG ON)
    endif()
endif()

if(BOOTSTRAP_ROOT_CONFIG)
    message("Root .config missing or empty, generating default config from Kconfig")
    execute_process(
        COMMAND ${py_exe} ${tools_dir}/python/gen_default_config.py --kconfig ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig --output ${SRC_CONFIG}
        WORKING_DIRECTORY ${UNIRTOS_DIR}
        RESULT_VARIABLE GEN_DEFAULT_CONFIG_RET
        OUTPUT_VARIABLE GEN_DEFAULT_CONFIG_STDOUT
        ERROR_VARIABLE GEN_DEFAULT_CONFIG_STDERR
    )
    if(NOT "${GEN_DEFAULT_CONFIG_STDOUT}" STREQUAL "")
        message("${GEN_DEFAULT_CONFIG_STDOUT}")
    endif()
    if(NOT "${GEN_DEFAULT_CONFIG_STDERR}" STREQUAL "")
        message("${GEN_DEFAULT_CONFIG_STDERR}")
    endif()
    if(NOT GEN_DEFAULT_CONFIG_RET EQUAL 0)
        message(FATAL_ERROR "Failed to generate default root .config from Kconfig defaults")
    endif()
endif()

# Process config files:
# 1. Merge system.config and modem.config to generate capability check.config
execute_process(
    COMMAND ${py_exe} ${tools_dir}/python/merge_files.py ${SYSTEM_SRC_CONFIG} ${MODEM_SRC_CONFIG} ${CHECK_CONFIG}
    WORKING_DIRECTORY ${UNIRTOS_DIR}
    RESULT_VARIABLE MERGE_CHECK_CONFIG_RET
)
if(NOT MERGE_CHECK_CONFIG_RET EQUAL 0)
    message(FATAL_ERROR "Failed to generate check.config from system/modem config files")
endif()

# 2. Initialize build-time effective.config from root .config directly,
#    then merge selected demo depend.config (builtin mode) and
#    external app depend.config fragments (if present).
execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC_CONFIG} ${EFFECTIVE_CONFIG}
    WORKING_DIRECTORY ${UNIRTOS_DIR}
    RESULT_VARIABLE COPY_EFFECTIVE_CONFIG_RET
)
if(NOT COPY_EFFECTIVE_CONFIG_RET EQUAL 0)
    message(FATAL_ERROR "Failed to initialize effective.config from root .config")
endif()

foreach(selected_app_demo_depend_config ${SELECTED_APP_DEMO_DEPEND_CONFIGS})
    message("Merging selected demo depend config: ${selected_app_demo_depend_config}")
    execute_process(
        COMMAND ${py_exe} ${tools_dir}/python/merge_files.py ${EFFECTIVE_CONFIG} ${selected_app_demo_depend_config} ${EFFECTIVE_CONFIG}
        WORKING_DIRECTORY ${UNIRTOS_DIR}
        RESULT_VARIABLE MERGE_SELECTED_DEMO_CONFIG_RET
    )
    if(NOT MERGE_SELECTED_DEMO_CONFIG_RET EQUAL 0)
        message(FATAL_ERROR "Failed to merge selected demo depend config: ${selected_app_demo_depend_config}")
    endif()
endforeach()

foreach(external_app_depend_config ${EXTERNAL_APP_DEPEND_CONFIGS})
    message("Merging external app depend config: ${external_app_depend_config}")
    execute_process(
        COMMAND ${py_exe} ${tools_dir}/python/merge_files.py ${EFFECTIVE_CONFIG} ${external_app_depend_config} ${EFFECTIVE_CONFIG}
        WORKING_DIRECTORY ${UNIRTOS_DIR}
        RESULT_VARIABLE MERGE_EFFECTIVE_CONFIG_RET
    )
    if(NOT MERGE_EFFECTIVE_CONFIG_RET EQUAL 0)
        message(FATAL_ERROR "Failed to merge external app depend config: ${external_app_depend_config}")
    endif()
endforeach()

# 3. Check effective.config is within check.config capability range
execute_process(
    COMMAND ${py_exe} ${tools_dir}/python/check_config_cap.py ${CHECK_CONFIG} ${EFFECTIVE_CONFIG}
    WORKING_DIRECTORY ${UNIRTOS_DIR}
    RESULT_VARIABLE CHECK_CAP_RET
    OUTPUT_VARIABLE CHECK_CAP_STDOUT
    ERROR_VARIABLE CHECK_CAP_STDERR
)
if(NOT CHECK_CAP_RET EQUAL 0)
    if(NOT "${CHECK_CAP_STDOUT}" STREQUAL "")
        message("${CHECK_CAP_STDOUT}")
    endif()
    if(NOT "${CHECK_CAP_STDERR}" STREQUAL "")
        message("${CHECK_CAP_STDERR}")
    endif()
    message(FATAL_ERROR "Build effective.config exceeds check.config capability limits")
endif()

# 4. Keep generating binary-side target.config for downstream tools from the merged build config
execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy ${EFFECTIVE_CONFIG} ${TARGET_CONFIG}
    WORKING_DIRECTORY ${UNIRTOS_DIR}
    RESULT_VARIABLE COPY_TARGET_CONFIG_RET
)
if(NOT COPY_TARGET_CONFIG_RET EQUAL 0)
    message(FATAL_ERROR "Failed to generate target.config from effective.config")
endif()

# Monitor file changes, if files change, rebuild file files during cmake generation
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${EFFECTIVE_CONFIG}
    ${TARGET_CONFIG}
    ${CHECK_CONFIG}
    ${SRC_CONFIG}
    ${SYSTEM_SRC_CONFIG}
    ${MODEM_SRC_CONFIG}
    ${SELECTED_APP_DEMO_CONFIGURE_DEPENDS}
    ${EXTERNAL_APP_DEPEND_CONFIGS}
)
set(KCONFIG_DEP ${BINARY_TOP_DIR}/kconfig_dep.cmake)
execute_process(
    COMMAND ${py_exe} ${tools_dir}/python/cmakedep.py ${KCONFIG_DEP}
    WORKING_DIRECTORY ${SOURCE_TOP_DIR}
)
include(${KCONFIG_DEP})

# Generate unified target.cmake from target.config according to dependencies in root directory kconfig files, and load into cmake environment
set(TARGET_CMAKE ${BINARY_TOP_DIR}/target.cmake)
execute_process(
    COMMAND ${py_exe} ${tools_dir}/python/cmakeconfig.py ${TARGET_CONFIG} ${TARGET_CMAKE}
    WORKING_DIRECTORY ${UNIRTOS_DIR}
)
include(${TARGET_CMAKE})

# Later generate unirtos_version.h file and put it in bat script to generate
# Get current date and time, generate unirtos_version.h file
string(TIMESTAMP CURRENT_TIME "%Y_%m_%d_%H_%M_%S")
set(LINUX_VERSION UNIRTOS_${build_project}_BATE_${CURRENT_TIME})
execute_process(
    COMMAND ${py_exe} ${tools_dir}/python/auto_unir_version.py ${build_project} ${LINUX_VERSION} V01 FFF 01
    WORKING_DIRECTORY ${BINARY_TOP_DIR}
)


# Convert .h.in files to corresponding .h files in cmake using SRC_CONFIG
configure_file(${QL_COMPONENTS_DIR}/components/include/qcm_proj_config.h.in ${out_inc_dir}/qcm_proj_config.h)
configure_file(${QL_DEMO_DIR}/include/unirtos_apps_config.h.in ${out_inc_dir}/unirtos_apps_config.h)
if(CONFIG_QAPP_UNIRTOS_STD_FUNC)
    configure_file(${QL_DEMO_DIR}/unirtos_std/inc/unirtos_atcmd_cfg.h.in ${out_inc_dir}/unirtos_atcmd_cfg.h)
endif()

# Load public .h header file paths needed during compilation
include_directories(${BINARY_TOP_DIR})
include_directories(${UNIRTOS_DIR})
include_directories(${out_inc_dir})
include_directories(${QL_COMPONENTS_DIR}/system/os)
include_directories(${QL_COMPONENTS_DIR}/system/include)
include_directories(${QL_COMPONENTS_DIR}/system/at)
include_directories(${QL_COMPONENTS_DIR}/system/audio)
include_directories(${QL_COMPONENTS_DIR}/system/debug)
include_directories(${QL_COMPONENTS_DIR}/system/dev)
include_directories(${QL_COMPONENTS_DIR}/system/rt_drivers/device)
include_directories(${QL_COMPONENTS_DIR}/system/rt_drivers/serial)
include_directories(${QL_COMPONENTS_DIR}/system/fota)
include_directories(${QL_COMPONENTS_DIR}/system/ftm)
include_directories(${QL_COMPONENTS_DIR}/system/hal)
include_directories(${QL_COMPONENTS_DIR}/system/modem/include)
include_directories(${QL_COMPONENTS_DIR}/system/modem/internal_inc)
include_directories(${QL_COMPONENTS_DIR}/system/modem/port/inc)
include_directories(${QL_COMPONENTS_DIR}/system/nvm)
include_directories(${QL_COMPONENTS_DIR}/system/public_comp/cJSON)
include_directories(${QL_COMPONENTS_DIR}/system/public_comp/event_notify)
include_directories(${QL_COMPONENTS_DIR}/system/public_comp/link_list)
include_directories(${QL_COMPONENTS_DIR}/system/public_comp/buffer_block)
include_directories(${QL_COMPONENTS_DIR}/system/public_comp/watermark)
include_directories(${QL_COMPONENTS_DIR}/system/public_comp/utils)
include_directories(${QL_COMPONENTS_DIR}/system/socket)
include_directories(${QL_COMPONENTS_DIR}/system/startup)
include_directories(${QL_COMPONENTS_DIR}/system/urc)
include_directories(${QL_COMPONENTS_DIR}/system/vfs)
include_directories(${QL_COMPONENTS_DIR}/system/wifiscan)
include_directories(${QL_COMPONENTS_DIR}/system/ppp/interface)
include_directories(${QL_COMPONENTS_DIR}/system/sys_callbacks)
include_directories(${QL_COMPONENTS_DIR}/components/include)
include_directories(${QL_COMPONENTS_DIR}/components/vtls/public)
include_directories(${QL_COMPONENTS_DIR}/system/public_comp/led)
include_directories(${QL_COMPONENTS_DIR}/system/public_comp/license)
include_directories(${QL_COMPONENTS_DIR}/system/cam)
include_directories(${QL_COMPONENTS_DIR}/system/secboot)
include_directories(${QL_COMPONENTS_DIR}/system/vsim_adapt)
include_directories(${QL_DEMO_DIR}/app_init)

# Conditional include paths for menuconfig-driven component targets (global scope)
# Auto-generated from all CONFIG_QCM_* symbols defined in kernel.cmake and qos_components/components/CMakeLists.txt
if(CONFIG_QCM_FILE_API_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_file/public)
endif()
if(CONFIG_QCM_SOCKET_ADP_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/socket/socket_adapter/public)
endif()
if(CONFIG_QCM_VTLS_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/vtls/public)
endif()
if(CONFIG_QCM_QURL_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qurl/include)
endif()
if(CONFIG_QCM_UTILS_BASE64_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/utils/base64/public)
endif()
if(CONFIG_QCM_UTILS_TLV_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/utils/tlv/public)
endif()
if(CONFIG_QCM_UTILS_UTF8_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/utils/utf8/public)
endif()
if(CONFIG_QCM_UTILS_UTILS_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/utils/utils/public)
endif()
if(CONFIG_QCM_MMS_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_mms/public)
endif()
if(CONFIG_QCM_MQTT_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_mqtt/public)
endif()
if(CONFIG_QCM_NTP_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_ntp/public)
endif()
if(CONFIG_QCM_PING_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/ping/public)
endif()
if(CONFIG_QCM_URC_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_urc/public)
endif()
if(CONFIG_QCM_WEBSOCKET_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_websocket/public)
endif()
if(CONFIG_QCM_LBS_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_lbs/public)
endif()
if(CONFIG_QCM_VIRT_AT_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_virt_at/public)
endif()
if(CONFIG_QCM_SPI_NOR_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_spi_nor/public)
endif()
if(CONFIG_QCM_COAP_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/coap/public)
endif()
if(CONFIG_QCM_COAP_API_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_coap/public)
endif()
if(CONFIG_QCM_LWM2M_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/lwm2m/public)
endif()
if(CONFIG_QCM_LWM2M_FOTA_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/lwm2m_fota/public)
endif()
if(CONFIG_QCM_SBFOTA_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_sbfota/public)
endif()
if(CONFIG_QCM_DMHTTP_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_dmhttp/public)
endif()
if(CONFIG_QCM_AUDIO_STREAM_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_audio/public)
endif()
if(CONFIG_QCM_AUDIO_MP3_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_mp3_dec/public)
endif()
if(CONFIG_QCM_AUDIO_AMR_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_amr/public)
endif()
if(CONFIG_QCM_QVSIM_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qvsim/public)
endif()
if(CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/mbedtls/mbedtls4/include)
elseif(CONFIG_QCM_MBEDTLS_2_LIBRARY_FUNC OR CONFIG_QCM_MBEDTLS_LIBRARY_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/mbedtls/mbedtls2/include)
endif()
if(CONFIG_QCM_UART_LOG_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_uart_log)
endif()
if(CONFIG_QCM_MINI_HTTP_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/mini_http/public)
endif()
if(CONFIG_QCM_ESIM_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/esim_sdk/public)
endif()
if(CONFIG_QCM_GNSS_FUNC)
    include_directories(${UNIRTOS_COMPONENTS_DIR}/qcm_gnss/public)
endif()

message("out_inc_dir=${out_inc_dir}")

include(cmake/build_config_summary.cmake)

#Link the components.a library
include(${QL_KERNEL_DIR}/kernel.cmake)

# Add compilation subdirectory
add_subdirectory_if_exist(qos_components/components)

# Cache menuconfig-driven component auto-link targets after component targets are created.
if(COMMAND unirtos_collect_auto_link_component_targets)
    unirtos_collect_auto_link_component_targets(UNIRTOS_AUTO_LINK_COMPONENT_TARGETS)
    set_property(GLOBAL PROPERTY unirtos_auto_link_component_targets "${UNIRTOS_AUTO_LINK_COMPONENT_TARGETS}")
    if(UNIRTOS_AUTO_LINK_COMPONENT_TARGETS)
        message(STATUS "[auto-link] component targets: ${UNIRTOS_AUTO_LINK_COMPONENT_TARGETS}")
    endif()
endif()

# Define all_components_libs.a library, used to aggregate all lib libraries from global components_libraries together
set(target all_components_libs)
set(all_components_libs_out ${out_unir_lib_dir}/all_components_libs.ld)
get_property(components_libraries GLOBAL PROPERTY components_libraries)
# Use python script to aggregate global libs together
add_custom_command(OUTPUT ${all_components_libs_out}
    COMMAND ${py_exe} ${tools_dir}/python/groupgen.py --base ${BINARY_TOP_DIR} ${all_components_libs_out} ${components_libraries}
    DEPENDS ${groupgen_py} ${components_libraries}
    WORKING_DIRECTORY ${BINARY_TOP_DIR}
)
add_custom_target(build_${target} DEPENDS ${all_components_libs_out})
add_library(${target} STATIC IMPORTED GLOBAL)
set_target_properties(${target} PROPERTIES IMPORTED_LOCATION ${all_components_libs_out})
add_dependencies(${target} build_${target})

add_subdirectory_if_exist(qos_applications)

# Define all_apps_libs.a library, used to aggregate all lib libraries from global apps_libraries together
set(target all_apps_libs)
set(all_apps_libs_out ${out_unir_lib_dir}/all_apps_libs.ld)
get_property(apps_libraries GLOBAL PROPERTY apps_libraries)
get_property(firmware_link_libraries GLOBAL PROPERTY firmware_link_libraries)
set(all_app_side_libraries ${apps_libraries} ${firmware_link_libraries})
message(STATUS "[firmware-link] app libraries: ${apps_libraries}")
message(STATUS "[firmware-link] external firmware libraries: ${firmware_link_libraries}")
# Use python script to aggregate global libs together
add_custom_command(OUTPUT ${all_apps_libs_out}
    COMMAND ${py_exe} ${tools_dir}/python/groupgen.py --base ${BINARY_TOP_DIR} ${all_apps_libs_out} ${all_app_side_libraries}
    DEPENDS ${groupgen_py} ${all_app_side_libraries}
    WORKING_DIRECTORY ${BINARY_TOP_DIR}
)
add_custom_target(build_${target} DEPENDS ${all_apps_libs_out})
add_library(${target} STATIC IMPORTED GLOBAL)
set_target_properties(${target} PROPERTIES IMPORTED_LOCATION ${all_apps_libs_out})
add_dependencies(${target} build_${target})

#message("build link ----------------------MY_LINK_LIBRARIES=${MY_LINK_LIBRARIES}")

target_link_libraries(ap_application PRIVATE -Wl,--whole-archive all_apps_libs -Wl,--no-whole-archive)
# Unify the collection of all libraries that need to be linked.
set(virt_libs "${CMAKE_CURRENT_SOURCE_DIR}/qos_kernel/${chip}/com_libs/libvirt_func.a")
if(components_libraries)
    set(ALL_LIBRARIES ${MY_LINK_LIBRARIES} ${components_libraries} ${virt_libs})
else()
    set(ALL_LIBRARIES ${MY_LINK_LIBRARIES} ${virt_libs})
endif()


# This needs to be modified to search for the lib library in a loop to avoid undefined references during linking.
target_link_libraries(ap_application  PRIVATE -Wl,--start-group ${ALL_LIBRARIES} -Wl,--end-group)


add_custom_target(app_pac ALL)
# Each platform adds corresponding functions
# Copy ld file to specified location
ldscript_copy(ap_application EXCLUDE_FROM_ALL ${ARGN} OUTPUT_DIR ${BINARY_BIN_DIR})
# Generate app
add_appimg(${target} EXCLUDE_FROM_ALL ${ARGN} OUTPUT_DIR ${BINARY_BIN_DIR})
# Package
pack_firmware(${target} EXCLUDE_FROM_ALL ${ARGN} OUTPUT_DIR ${FIRMWARE_DIR})

# Print Flash and RAM usage
if(NOT CMAKE_SIZE)
    set(CMAKE_SIZE "${COMPILER_PATH}/bin/arm-none-eabi-size")
endif()

add_custom_command(TARGET ap_application POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:ap_application>
    COMMENT "Flash and RAM usage for ap_application:"
)

endif()
