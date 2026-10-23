# Get platform and board values
set(chip "${build_arch}")
set(board "${build_project}")

message("qos_kernel")
function(add_wifi_module_libraries chip output_var)
    set(kernel_libraries "${${output_var}}")

    if("${chip}" STREQUAL "bk7238" OR "${chip}" STREQUAL "ecr6600")
        set(libs_dir "${CMAKE_SOURCE_DIR}/qos_kernel/${chip}/arch_prj/ql_kernel/libs")
    else()
        set(libs_dir "${CMAKE_SOURCE_DIR}/qos_kernel/${chip}/arch_prj/ql_kernel/ap/libs")
    endif()

    file(GLOB_RECURSE libs CONFIGURE_DEPENDS "${libs_dir}/*.a")

    if(NOT libs)
        message(FATAL_ERROR "No ${chip} libs found in ${libs_dir}")
    endif()


    foreach(lib ${libs})
        get_filename_component(lib_name "${lib}" NAME)
        # get_filename_component(lib_dir "${lib}" DIRECTORY)
        # libandes.a is already linked explicitly via "-landes" for ecr6600
        # (see toolchain.cmake), so skip it here to avoid linking it twice.
        if("${chip}" STREQUAL "ecr6600" AND "${lib_name}" STREQUAL "libandes.a")
            continue()
        endif()
        list(APPEND kernel_libraries "${lib_name}")
        # list(APPEND lib_dirs "${lib_dir}")
    endforeach()

    if("${chip}" STREQUAL "bk7238" OR "${chip}" STREQUAL "ecr6600")
        list(APPEND kernel_libraries
            lwip
            mbedtls
        )
    else()
        get_property(arch_component_libraries GLOBAL PROPERTY ARCH_COMPONENT_LIBRARIES)
        list(APPEND kernel_libraries
            ${arch_component_libraries}
        )
    endif()

    set(${output_var} "${kernel_libraries}" PARENT_SCOPE)
endfunction()

if("${chip}" STREQUAL "bk7258" OR "${chip}" STREQUAL "bk7238" OR "${chip}" STREQUAL "ecr6600")
    add_wifi_module_libraries("${chip}" MY_LINK_LIBRARIES)
else()
    # Path to build static library
    set(com_libs_dir "${CMAKE_CURRENT_SOURCE_DIR}/qos_kernel/${chip}/com_libs")

    message("com_libs_dir=${com_libs_dir}")

    # Find all .a files
    file(GLOB com_libs "${com_libs_dir}/*.a")

    if(NOT CONFIG_QCM_FILE_API_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_file.a")
    endif()
    if(NOT CONFIG_QCM_SOCKET_ADP_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libsocket_adapter.a")
    endif()
    if(NOT CONFIG_QCM_VTLS_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libvtls.a")
    endif()
    if(NOT CONFIG_QCM_QURL_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqurl.a")
    endif()
    if(NOT CONFIG_QCM_UTILS_BASE64_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libbase64.a")
    endif()
    if(NOT CONFIG_QCM_UTILS_TLV_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libtlv.a")
    endif()
    if(NOT CONFIG_QCM_UTILS_UTF8_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libutf8.a")
    endif()
    if(NOT CONFIG_QCM_UTILS_UTILS_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libutils.a")
    endif()
    if(NOT CONFIG_QCM_MMS_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_mms.a")
    endif()
    if(NOT CONFIG_QCM_MQTT_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_mqtt.a")
    endif()
    if(NOT CONFIG_QCM_NTP_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_ntp.a")
    endif()
    if(NOT CONFIG_QCM_PING_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libping.a")
    endif()
    if(NOT CONFIG_QCM_URC_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_urc.a")
    endif()
    if(NOT CONFIG_QCM_WEBSOCKET_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_websocket.a")
    endif()
    if(NOT CONFIG_QCM_LBS_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_lbs.a")
    endif()
    if(NOT CONFIG_QCM_VIRT_AT_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_virt_at.a")
    endif()
    if(NOT CONFIG_QCM_SPI_NOR_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_spi_nor.a")
    endif()
    if(NOT CONFIG_QCM_COAP_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libcoap.a")
    endif()
    if(NOT CONFIG_QCM_COAP_API_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_coap.a")
    endif()
    if(NOT CONFIG_QCM_LWM2M_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libosa_lwm2m.a")
    endif()
    if(NOT CONFIG_QCM_LWM2M_FOTA_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_lwm2m_fota.a")
    endif()
    if(NOT CONFIG_QCM_SBFOTA_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_sbfota.a")
    endif()
    if(NOT CONFIG_QCM_DMHTTP_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_dmhttp.a")
    endif()
    if(NOT CONFIG_QCM_AUDIO_STREAM_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_audio.a")
    endif()
    if(NOT CONFIG_QCM_AUDIO_MP3_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_mp3_dec.a")
    endif()
    if(NOT CONFIG_QCM_AUDIO_AMR_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqcm_amr.a")
    endif()
    if(NOT CONFIG_QCM_QVSIM_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libqvsim.a")
    endif()
    if(NOT CONFIG_QCM_MBEDTLS_LIBRARY_FUNC)
        list(REMOVE_ITEM com_libs "${com_libs_dir}/libmbedtls.a")
    endif()


    # No more lib libraries can be added below, virtual function libraries must be placed at the end of the compilation link!!!

    # Virtual function purpose: To avoid compilation errors after disabling certain macro controls, for example, if a.lib contains apis from b.lib, even if b.lib's macro control is disabled, a.lib will still call these apis
    # Virtual function library placed at the end of linking: Because when identical apis exist during linking, whoever provides the symbol first will be used by the linker, placing it at the end prevents overriding existing functionality

    # First remove from com_libs, then append to the end
    list(REMOVE_ITEM com_libs "${com_libs_dir}/libvirt_func.a")
    #list(APPEND com_libs "${com_libs_dir}/libvirt_func.a")

    #message("unirtos com_libs=${com_libs}    ")
    set(MY_LINK_LIBRARIES ${MY_LINK_LIBRARIES} ${com_libs})
endif()

