
# This code defines a function called print that can be used to print the value of CMake variables. The function contains one parameter, arg, which represents the variable name to print.
# set(MY_VAR "Hello, world!")
# print(MY_VAR)
# The function will output: MY_VAR: Hello, world!
function(print arg)
    message(STATUS "${arg}: ${${arg}}")
endfunction()

# This CMake code defines a macro named install_header, used when installing header files.
# The macro accepts two parameters, src and dst, representing the source file path and destination file path respectively.
# The macro body contains the following steps:
# Use the configure_file() function to copy the source file ${src} to the destination file ${dst}. The COPYONLY parameter indicates to only copy the file without performing variable substitution or other operations. The configure_file() function here is actually used to copy files, since there is no dedicated file copy function in CMake, so configure_file() function is used as an alternative.
# The macro's purpose is to copy the source file to the destination file so that it can be included in the installation directory during installation. Usually, header files are among the files that need to be installed, so this macro can be used to install header files.
macro(install_header src dst)
    # get_filename_component(dst_dir ${dst} DIRECTORY)
    # file(RELATIVE_PATH src_rel ${dst_dir} ${CMAKE_CURRENT_SOURCE_DIR}/${src})
    # file(GENERATE OUTPUT ${dst} CONTENT "#include \"${src_rel}\"\n")
    configure_file(${src} ${dst} COPYONLY)
endmacro()

# This CMake code defines a macro named target_sources_if, used to add source files to targets conditionally.
# The main purpose of this macro is to add source files to targets conditionally so they can be used when building targets. The macro accepts an optional condition parameter, along with a THEN keyword and a command parameter. If the condition is met, execute the command after THEN.
# The macro body contains the following steps:
# Use the cmake_parse_arguments() function to parse parameters. The first parameter MY represents the parsed parameter list, the second parameter is empty, the third parameter is also empty, the fourth parameter is THEN, and the last parameter is ARGN, representing all remaining parameters.
# Check if there are remaining parameters, if so, use the target_sources() function to add source files to the target. The target_sources() function is used to add source files to targets, with the first parameter being the target name and the remaining parameters being the source file list.
# The purpose of this macro is to add source files to targets conditionally. This macro can be used to selectively add needed source files when building targets based on conditions.
# If MY macro condition is met, add source files
# eg: target_sources_if(CONFIG_UNIR_PROJECT_FEATURE_NTP_AT THEN ${target} PRIVATE src/unir_atcmd_ntp.c)
macro(target_sources_if) # <conditon> THEN <command>
    cmake_parse_arguments(MY "" "" "THEN" ${ARGN})
    if(${MY_UNPARSED_ARGUMENTS})
        target_sources(${MY_THEN})
    endif()
endmacro()

# As defined in the target_sources_if macro above, if MY condition is met, add library
# eg: target_link_libraries_if(CONFIG_UNIR_PROJECT_FEATURE_BOOT_SDMMC THEN ${target} PRIVATE boot_ex_sdmmc)
macro(target_link_libraries_if)  # <conditon> THEN <command>
    cmake_parse_arguments(MY "" "" "THEN" ${ARGN})
    if(${MY_UNPARSED_ARGUMENTS})
        target_link_libraries(${MY_THEN})
    endif()
endmacro()

# As defined in the target_sources_if macro above, if MY condition is met, add specified header files
# eg: target_include_targets_if(CONFIG_UNIR_PROJECT_FEATURE_BOOT_EMMC THEN ${target} PRIVATE boot_ex_emmc)
macro(target_include_targets_if)  # <conditon> THEN <command>
    cmake_parse_arguments(MY "" "" "THEN" ${ARGN})
    if(${MY_UNPARSED_ARGUMENTS})
        target_include_targets(${MY_THEN})
    endif()
endmacro()

# As defined in the target_sources_if macro above, if MY condition is met, add directory
function(add_subdirectory_if)
    cmake_parse_arguments(MY "" "" "THEN" ${ARGN})
    if(${MY_UNPARSED_ARGUMENTS})
        add_subdirectory(${MY_THEN})
    endif()
endfunction()

# Set the paths of all lib libraries under the components directory to be added to the components_libraries global variable
function(add_components_libraries)
    get_property(components_libraries GLOBAL PROPERTY components_libraries)
    set_property(GLOBAL PROPERTY components_libraries ${components_libraries} ${ARGN})
endfunction()

# Set the paths of all lib libraries under the apps directory to be added to the apps_libraries global variable
function(add_apps_libraries)
    get_property(apps_libraries GLOBAL PROPERTY apps_libraries)
    set_property(GLOBAL PROPERTY apps_libraries ${apps_libraries} ${ARGN})
endfunction()

# Set libraries that must participate in final firmware link but are not app targets.
# Typical use case: external libraries discovered from env_config.json.
function(add_firmware_link_libraries)
    get_property(firmware_link_libraries GLOBAL PROPERTY firmware_link_libraries)
    set_property(GLOBAL PROPERTY firmware_link_libraries ${firmware_link_libraries} ${ARGN})
endfunction()

# Append a component target only when both the config symbol is enabled and the target exists.
function(_unirtos_append_component_if_enabled out_var cfg_sym lib_target)
    if(${cfg_sym} AND TARGET ${lib_target})
        set(_tmp_libs ${${out_var}})
        list(APPEND _tmp_libs ${lib_target})
        set(${out_var} ${_tmp_libs} PARENT_SCOPE)
    endif()
endfunction()

# Collect component targets from menuconfig CONFIG_QCM_* symbols.
# The result can be linked directly to app-side targets to avoid per-app manual library lists.
function(unirtos_collect_auto_link_component_targets out_var)
    set(_auto_link_libs)

    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_FILE_API_FUNC qcm_file)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_SOCKET_ADP_FUNC socket_adapter)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_VTLS_FUNC vtls)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_QURL_FUNC qurl)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_UTILS_BASE64_FUNC base64)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_UTILS_TLV_FUNC tlv)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_UTILS_UTF8_FUNC utf8)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_UTILS_UTILS_FUNC utils)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_MMS_FUNC qcm_mms)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_MQTT_FUNC qcm_mqtt)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_NTP_FUNC qcm_ntp)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_PING_FUNC ping)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_URC_FUNC qcm_urc)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_WEBSOCKET_FUNC qcm_websocket)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_LBS_FUNC qcm_lbs)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_VIRT_AT_FUNC qcm_virt_at)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_SPI_NOR_FUNC qcm_spi_nor)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_COAP_FUNC coap)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_COAP_API_FUNC qcm_coap)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_LWM2M_FUNC osa_lwm2m)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_LWM2M_FOTA_FUNC qcm_lwm2m_fota)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_SBFOTA_FUNC qcm_sbfota)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_DMHTTP_FUNC qcm_dmhttp)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_AUDIO_STREAM_FUNC qcm_audio)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_AUDIO_MP3_FUNC qcm_mp3_dec)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_AUDIO_AMR_FUNC qcm_amr)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_QVSIM_FUNC qvsim)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_MBEDTLS_LIBRARY_FUNC mbedtls)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_UART_LOG_FUNC qcm_uart_log)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_MINI_HTTP_FUNC mini_http)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_ESIM_FUNC esim_sdk)
    _unirtos_append_component_if_enabled(_auto_link_libs CONFIG_QCM_GNSS_FUNC qcm_gnss)

    list(REMOVE_DUPLICATES _auto_link_libs)
    set(${out_var} ${_auto_link_libs} PARENT_SCOPE)
endfunction()

# Get globally cached auto-link targets if available, otherwise collect on demand.
function(unirtos_get_auto_link_component_targets out_var)
    get_property(_cached_libs GLOBAL PROPERTY unirtos_auto_link_component_targets)
    if(_cached_libs)
        set(${out_var} ${_cached_libs} PARENT_SCOPE)
        return()
    endif()

    unirtos_collect_auto_link_component_targets(_resolved_libs)
    set(${out_var} ${_resolved_libs} PARENT_SCOPE)
endfunction()


# add_subdirectory_if_exist - If there is a CMakeLists.txt file in the current dir directory, add it
# Check if folder exists, if it does, add the corresponding folder directory
function(add_subdirectory_if_exist dir)
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
            add_subdirectory(${dir})
        endif()
    endif()
endfunction()

# Collect external application project directories directly under qos_applications.
# Built-in demos stay under apps/ and keep their existing Kconfig-driven flow.
function(collect_qos_application_projects base_dir out_var)
    set(qos_application_reserved_dirs
        apps
        app_init
        unirtos_std
        include
        common
        template
        templates
        cmake
    )

    file(GLOB qos_application_entries
        RELATIVE ${base_dir}
        ${base_dir}/*
    )

    set(qos_application_projects)
    foreach(entry ${qos_application_entries})
        set(entry_path ${base_dir}/${entry})
        if(NOT IS_DIRECTORY ${entry_path})
            continue()
        endif()

        list(FIND qos_application_reserved_dirs ${entry} reserved_index)
        if(NOT reserved_index EQUAL -1)
            continue()
        endif()

        if(EXISTS ${entry_path}/CMakeLists.txt)
            list(APPEND qos_application_projects ${entry})
        endif()
    endforeach()

    set(${out_var} ${qos_application_projects} PARENT_SCOPE)
endfunction()

function(resolve_builtin_demo_selection qos_applications_dir app_init_entries platform output_dir py_exe tools_dir work_dir)
    set(ALL_APP_DEMO_MANIFESTS)
    set(SELECTED_APP_DEMO_DIRS)
    set(SELECTED_APP_BUILD_DIRS)
    set(SELECTED_APP_DEMO_ENTRY_NAMES)
    set(SELECTED_APP_DEMO_DEPEND_CONFIGS)
    set(SELECTED_APP_DEMOS_CMAKE)
    set(GENERATED_APP_INIT_ENTRIES_C)
    set(SELECTED_APP_DEMO_CONFIGURE_DEPENDS)

    set(apps_dir ${qos_applications_dir}/apps)
    if(NOT IS_DIRECTORY ${apps_dir})
        set(ALL_APP_DEMO_MANIFESTS ${ALL_APP_DEMO_MANIFESTS} PARENT_SCOPE)
        set(SELECTED_APP_DEMO_DIRS ${SELECTED_APP_DEMO_DIRS} PARENT_SCOPE)
        set(SELECTED_APP_BUILD_DIRS ${SELECTED_APP_BUILD_DIRS} PARENT_SCOPE)
        set(SELECTED_APP_DEMO_ENTRY_NAMES ${SELECTED_APP_DEMO_ENTRY_NAMES} PARENT_SCOPE)
        set(SELECTED_APP_DEMO_DEPEND_CONFIGS ${SELECTED_APP_DEMO_DEPEND_CONFIGS} PARENT_SCOPE)
        set(SELECTED_APP_DEMOS_CMAKE ${SELECTED_APP_DEMOS_CMAKE} PARENT_SCOPE)
        set(GENERATED_APP_INIT_ENTRIES_C ${GENERATED_APP_INIT_ENTRIES_C} PARENT_SCOPE)
        set(SELECTED_APP_DEMO_CONFIGURE_DEPENDS ${SELECTED_APP_DEMO_CONFIGURE_DEPENDS} PARENT_SCOPE)
        return()
    endif()

    if(NOT EXISTS ${app_init_entries})
        message(STATUS "Builtin demo selection skipped: missing app init entries file: ${app_init_entries}")
        file(GLOB_RECURSE app_demo_manifests LIST_DIRECTORIES false ${apps_dir}/*/demo.manifest.json)
        set(ALL_APP_DEMO_MANIFESTS ${app_demo_manifests})
        set(SELECTED_APP_DEMO_CONFIGURE_DEPENDS ${ALL_APP_DEMO_MANIFESTS})
        set(ALL_APP_DEMO_MANIFESTS ${ALL_APP_DEMO_MANIFESTS} PARENT_SCOPE)
        set(SELECTED_APP_DEMO_DIRS ${SELECTED_APP_DEMO_DIRS} PARENT_SCOPE)
        set(SELECTED_APP_BUILD_DIRS ${SELECTED_APP_BUILD_DIRS} PARENT_SCOPE)
        set(SELECTED_APP_DEMO_ENTRY_NAMES ${SELECTED_APP_DEMO_ENTRY_NAMES} PARENT_SCOPE)
        set(SELECTED_APP_DEMO_DEPEND_CONFIGS ${SELECTED_APP_DEMO_DEPEND_CONFIGS} PARENT_SCOPE)
        set(SELECTED_APP_DEMOS_CMAKE ${SELECTED_APP_DEMOS_CMAKE} PARENT_SCOPE)
        set(GENERATED_APP_INIT_ENTRIES_C ${GENERATED_APP_INIT_ENTRIES_C} PARENT_SCOPE)
        set(SELECTED_APP_DEMO_CONFIGURE_DEPENDS ${SELECTED_APP_DEMO_CONFIGURE_DEPENDS} PARENT_SCOPE)
        return()
    endif()

    file(MAKE_DIRECTORY ${output_dir})
    set(SELECTED_APP_DEMOS_CMAKE ${output_dir}/selected_app_demos.cmake)
    set(GENERATED_APP_INIT_ENTRIES_C ${output_dir}/generated_app_init_entries.c)

    execute_process(
        COMMAND ${py_exe} ${tools_dir}/python/resolve_demo_selection.py
            --apps-root ${apps_dir}
            --entries ${app_init_entries}
            --platform ${platform}
            --output-cmake ${SELECTED_APP_DEMOS_CMAKE}
            --output-registration-c ${GENERATED_APP_INIT_ENTRIES_C}
        WORKING_DIRECTORY ${work_dir}
        RESULT_VARIABLE RESOLVE_SELECTED_DEMOS_RET
        OUTPUT_VARIABLE RESOLVE_SELECTED_DEMOS_STDOUT
        ERROR_VARIABLE RESOLVE_SELECTED_DEMOS_STDERR
    )
    if(NOT "${RESOLVE_SELECTED_DEMOS_STDOUT}" STREQUAL "")
        message("${RESOLVE_SELECTED_DEMOS_STDOUT}")
    endif()
    if(NOT "${RESOLVE_SELECTED_DEMOS_STDERR}" STREQUAL "")
        message("${RESOLVE_SELECTED_DEMOS_STDERR}")
    endif()
    if(NOT RESOLVE_SELECTED_DEMOS_RET EQUAL 0)
        message(FATAL_ERROR "Failed to resolve selected app demos from apps_init_entries.def")
    endif()

    include(${SELECTED_APP_DEMOS_CMAKE})
    set(SELECTED_APP_DEMO_CONFIGURE_DEPENDS
        ${app_init_entries}
        ${SELECTED_APP_DEMOS_CMAKE}
        ${GENERATED_APP_INIT_ENTRIES_C}
        ${ALL_APP_DEMO_MANIFESTS}
        ${SELECTED_APP_DEMO_DEPEND_CONFIGS}
    )

    set(ALL_APP_DEMO_MANIFESTS ${ALL_APP_DEMO_MANIFESTS} PARENT_SCOPE)
    set(SELECTED_APP_DEMO_DIRS ${SELECTED_APP_DEMO_DIRS} PARENT_SCOPE)
    set(SELECTED_APP_BUILD_DIRS ${SELECTED_APP_BUILD_DIRS} PARENT_SCOPE)
    set(SELECTED_APP_DEMO_ENTRY_NAMES ${SELECTED_APP_DEMO_ENTRY_NAMES} PARENT_SCOPE)
    set(SELECTED_APP_DEMO_DEPEND_CONFIGS ${SELECTED_APP_DEMO_DEPEND_CONFIGS} PARENT_SCOPE)
    set(SELECTED_APP_DEMOS_CMAKE ${SELECTED_APP_DEMOS_CMAKE} PARENT_SCOPE)
    set(GENERATED_APP_INIT_ENTRIES_C ${GENERATED_APP_INIT_ENTRIES_C} PARENT_SCOPE)
    set(SELECTED_APP_DEMO_CONFIGURE_DEPENDS ${SELECTED_APP_DEMO_CONFIGURE_DEPENDS} PARENT_SCOPE)
endfunction()

# This code defines a CMake function named cpp_only, used when compiling C++ files to ignore C++ code and only compile preprocessed C code. The function's purpose is to compile C++ files into C code for use in other projects.
# The function accepts three parameters: target, file, and ARGN. Where target represents the target file name to be generated, file represents the C++ file name to be compiled, and ARGN represents other dependent libraries.
# The function body includes the following steps:
# Use the add_library() function to create an object library named target with source file file.
# Use the set_source_files_properties() function to set the language property of file to C.
# Use the target_compile_options() function to set compile options for target. Where -E means only preprocessing, -P means output preprocessed code, -x c means treat input file as C code.
# Use the target_include_directories() function to set target's include directory to its dependency library's include directory.
# Use the target_compile_definitions() function to set target's compile definitions to its dependency library's compile definitions.
# The function's purpose is to compile C++ files into C code for use in other projects. In C++ files, if you want to use functions from C code, you can declare them as C functions using the extern "C" language modifier. Then, you can use the -include option to include preprocessed C code into C++ code, thus using these functions in C++ code.

# The foreach loop here is used to process dependent libraries in the ARGN list. ARGN represents the list of remaining parameters beyond the first two parameters target and file.
# The first foreach loop is used to add dependent library's include directories to target's include directories. It traverses each element in the ARGN list and uses the target_include_directories() function to add the INTERFACE_INCLUDE_DIRECTORIES property of the corresponding library to target's include directories. INTERFACE_INCLUDE_DIRECTORIES represents the interface include directories of the library, i.e. the header file paths needed when other projects use this library.
# The second foreach loop is used to add dependent library's compile definitions to target's compile definitions. It traverses each element in the ARGN list and uses the target_compile_definitions() function to add the INTERFACE_COMPILE_DEFINITIONS property of the corresponding library to target's compile definitions. INTERFACE_COMPILE_DEFINITIONS represents the interface compile definitions of the library, i.e. the compile macros that need to be defined when other projects use this library.
# The purpose of doing this is to pass the interface information of dependent libraries to target, allowing target to use functions and type definitions from dependent libraries.
function(cpp_only target file)
    add_library(${target} OBJECT ${file})
    set_source_files_properties(${file} PROPERTIES LANGUAGE C)
    target_compile_options(${target} PRIVATE -E -P -x c)
    foreach(dep ${ARGN})
        target_include_directories(${target}
            PRIVATE $<TARGET_PROPERTY:${dep},INTERFACE_INCLUDE_DIRECTORIES>)
    endforeach()
    foreach(dep ${ARGN})
        target_compile_definitions(${target}
            PRIVATE $<TARGET_PROPERTY:${dep},INTERFACE_COMPILE_DEFINITIONS>)
    endforeach()
endfunction()

# Function to exclude files
function(exclude_files sources_var exclude_list)
    foreach(file ${${sources_var}})
        list(FIND exclude_list ${file} exclude_index)
        if(NOT ${exclude_index} EQUAL -1)
            list(REMOVE_ITEM ${sources_var} ${file})
        endif()
    endforeach()
endfunction()

function(target_add_sources target flag)
    foreach(file ${ARGN})
        target_sources(${target} ${flag} ${file})
    endforeach(file ${ARGN})
endfunction()

function(target_add_sources_dir target flag)
    cmake_parse_arguments(is "debug" "" "" ${ARGN})
    file(GLOB files ${ARGN})
    foreach(file ${files})
        if(${is_debug})
            message("TARGET : \"${target}\" add file: ${file}")
        endif()
        target_sources(${target} ${flag} ${file})
    endforeach(file ${ARGN})
endfunction()
