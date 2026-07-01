message(STATUS "cmake config ${CMAKE_CURRENT_SOURCE_DIR}")
# This file is shared for csdk

set(target unirtos_std)
add_apps_libraries($<TARGET_FILE:${target}>)
add_library(${target} STATIC)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_unir_lib_dir})

# Include header files; do not add header files arbitrarily here.
include_directories(${UNIRTOS_COMPONENTS_DIR}/include)
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files or precompiled files directory
set(apps_file_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")
message("apps_file_dir: ${apps_file_dir}")

# Search for .o or .c.obj files in this directory
file(GLOB PREBUILT_OBJECTS
    "${apps_file_dir}/*.c.o"
    "${apps_file_dir}/*.c.obj"
)

# Check if .o or .c.obj files exist
if (PREBUILT_OBJECTS)
    # csdk
    set(HAS_PREBUILT_OBJECTS FALSE)
    message(STATUS "CSDK compilation")
else()
    # Headers needed for std compilation
    set(HAS_PREBUILT_OBJECTS TRUE)
    message(STATUS "STD compilation")
    target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR})
    target_include_directories(${target} PRIVATE ${SOURCE_SYS_DEBUG_DIR})
    target_include_directories(${target} PRIVATE ${SOURCE_SYS_OS_DIR})
    target_include_directories(${target} PRIVATE ${SOURCE_SYS_SOCKET_DIR})
    target_include_directories(${target} PRIVATE ${SOURCE_COMPONENTS_PUBLIC_INC})
    target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR}/system/ppp/interface)
    target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR}/system/sys_callbacks)
    target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR}/system/flash)
    target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR}/components/lwm2m/public)
    target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR}/components/lwm2m_fota/public)
    target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR}/components/coap/public)
    target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR}/components/qurl/include)
    
    # Conditional include paths for menuconfig-driven component targets (STD compilation only)
    # Auto-generated from all CONFIG_QCM_* symbols
    if(CONFIG_QCM_FILE_API_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_file/public)
    endif()
    if(CONFIG_QCM_SOCKET_ADP_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/socket/socket_adapter/public)
    endif()
    if(CONFIG_QCM_VTLS_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/vtls/public)
    endif()
    if(CONFIG_QCM_QURL_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qurl/include)
    endif()
    if(CONFIG_QCM_UTILS_BASE64_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/utils/base64/public)
    endif()
    if(CONFIG_QCM_UTILS_TLV_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/utils/tlv/public)
    endif()
    if(CONFIG_QCM_UTILS_UTF8_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/utils/utf8/public)
    endif()
    if(CONFIG_QCM_UTILS_UTILS_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/utils/utils/public)
    endif()
    if(CONFIG_QCM_MMS_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_mms/public)
    endif()
    if(CONFIG_QCM_MQTT_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_mqtt/public)
    endif()
    if(CONFIG_QCM_NTP_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_ntp/public)
    endif()
    if(CONFIG_QCM_PING_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/ping/public)
    endif()
    if(CONFIG_QCM_URC_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_urc/public)
    endif()
    if(CONFIG_QCM_WEBSOCKET_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_websocket/public)
    endif()
    if(CONFIG_QCM_LBS_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_lbs/public)
    endif()
    if(CONFIG_QCM_VIRT_AT_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_virt_at/public)
    endif()
    if(CONFIG_QCM_SPI_NOR_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_spi_nor/public)
    endif()
    if(CONFIG_QCM_COAP_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/coap/public)
    endif()
    if(CONFIG_QCM_COAP_API_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_coap/public)
    endif()
    if(CONFIG_QCM_LWM2M_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/lwm2m/public)
    endif()
    if(CONFIG_QCM_LWM2M_FOTA_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/lwm2m_fota/public)
    endif()
    if(CONFIG_QCM_SBFOTA_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_sbfota/public)
    endif()
    if(CONFIG_QCM_DMHTTP_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_dmhttp/public)
    endif()
    if(CONFIG_QCM_AUDIO_STREAM_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_audio/public)
    endif()
    if(CONFIG_QCM_AUDIO_MP3_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_mp3_dec/public)
    endif()
    if(CONFIG_QCM_AUDIO_AMR_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_amr/public)
    endif()
    if(CONFIG_QCM_QVSIM_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qvsim/public)
    endif()
    if(CONFIG_QCM_MBEDTLS_LIBRARY_FUNC)
        if(CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC)
            target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/mbedtls/mbedtls4/include)
        else()
            target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/mbedtls/mbedtls2/include)
        endif()
    endif()
    if(CONFIG_QCM_UART_LOG_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_uart_log)
    endif()
    if(CONFIG_QCM_MINI_HTTP_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/mini_http/public)
    endif()
    if(CONFIG_QCM_ESIM_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/esim_sdk/public)
    endif()
    if(CONFIG_QCM_GNSS_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/qcm_gnss/public)
    endif()
    if(CONFIG_QCM_UTILS_XMODEM_FUNC)
        target_include_directories(${target} PUBLIC ${UNIRTOS_COMPONENTS_DIR}/utils/xmodem/public)
    endif()
endif()


# Files that need secondary compilation and open source on SDK side, other files need macro control wrapping
target_add_sources(${target} PRIVATE
    src/unirtos_atcmd_init.c
    src/unirtos_atcmd_version.c
)

# Set targets to link for std compilation
if(COMMAND unirtos_get_auto_link_component_targets)
    unirtos_get_auto_link_component_targets(LINK_LIB)
elseif(COMMAND unirtos_collect_auto_link_component_targets)
    unirtos_collect_auto_link_component_targets(LINK_LIB)
endif()

if(CONFIG_QOSA_LINK_SYSTEM_LIBRARIES_FUNC)
    list(APPEND LINK_LIB system_os)
endif()

# Define source files to add, if dependent targets are needed as well
if(CONFIG_QCM_FILE_API_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_file_handler.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_file.c")
    list(APPEND LINK_LIB qcm_file)
endif()

if(CONFIG_QAPP_UNIRTOS_CAMERA_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_camera_handler.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_camera.c")
endif()

if(CONFIG_QAPP_UNIRTOS_TCPIP_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_tcpip.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_tcpip_app.c")
endif()

if(CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_tcpip_nb.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_tcpip_nb_app.c")
endif()

if(CONFIG_QAPP_UNIRTOS_NB_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_nb.c")
endif()

if(CONFIG_QAPP_UNIRTOS_PING_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_ping.c")
    list(APPEND LINK_LIB ping)
endif()

if(CONFIG_QAPP_UNIRTOS_SSL_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_ssl.c")
endif()

if(CONFIG_QAPP_UNIRTOS_HTTP_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_http.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_http_app.c")
    list(APPEND LINK_LIB qurl)
endif()

if(CONFIG_QAPP_UNIRTOS_FTP_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_ftp.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_ftp_app.c")
    list(APPEND LINK_LIB qurl)
endif()

if(CONFIG_QAPP_UNIRTOS_GCOV_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_gcov.c")
endif()

if(CONFIG_QAPP_UNIRTOS_COAP_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_coap.c")
    list(APPEND LINK_LIB qcm_coap)
endif()

if(CONFIG_QCM_LWM2M_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_lwm2m_common.c")
    list(APPEND LINK_LIB qcm_file)
endif()

if(CONFIG_QAPP_UNIRTOS_LWM2M_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_lwm2m.c")
    list(APPEND LINK_LIB qcm_file osa_lwm2m qcm_lwm2m_fota)
endif()

if(CONFIG_QAPP_UNIRTOS_ONENET_LWM2M_AT_FUN)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_mipl_lwm2m.c")
    list(APPEND LINK_LIB qcm_file osa_lwm2m qcm_lwm2m_fota)
endif()

if(CONFIG_QAPP_UNIRTOS_AEP_LWM2M_AT_FUN)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_aepiot_lwm2m.c")
    list(APPEND LINK_LIB qcm_file osa_lwm2m qcm_lwm2m_fota)
endif()

if(CONFIG_QAPP_UNIRTOS_SMTP_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_smtp.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_smtp_app.c")
    list(APPEND LINK_LIB base64 qurl)
endif()

if(CONFIG_QAPP_UNIRTOS_MMS_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_mms.c")
    list(APPEND LINK_LIB qcm_mms)
endif()

if(CONFIG_QAPP_UNIRTOS_MQTT_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_mqtt.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_onenet_token.c")
    list(APPEND LINK_LIB qcm_mqtt)
endif()

if(CONFIG_QAPP_UNIRTOS_UNIFY_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_unify.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_cuiot_auth.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_uniot_identity.c")
    list(APPEND LINK_LIB qcm_mqtt)
endif()

if(CONFIG_QAPP_UNIRTOS_UNIFY_IDENTITY_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_uniot_identity.c")
    list(APPEND LINK_LIB qcm_mqtt)
endif()

if(CONFIG_QAPP_UNIRTOS_NTP_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_ntp.c")
    list(APPEND LINK_LIB qcm_ntp)
endif()

if(CONFIG_QAPP_UNIRTOS_WEBSOCKET_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_websocket.c")
    list(APPEND LINK_LIB qcm_websocket)
endif()

if(CONFIG_QAPP_UNIRTOS_DMHTTP_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_dmhttp.c")
    list(APPEND LINK_LIB qcm_dmhttp)
endif()

if(CONFIG_QAPP_UNIRTOS_QINDCFG_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_qindcfg.c")
endif()

if(CONFIG_QAPP_UNIRTOS_FOTA_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_qfotadl.c")
endif()

if(CONFIG_QAPP_UNIRTOS_PPP_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_ppp.c")
endif()

if(CONFIG_QAPP_UNIRTOS_PDP_KALIVE_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_pdp_kalive_app.c")
endif()

if(CONFIG_QAPP_UNIRTOS_SIM_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_sim.c")
endif()

if(CONFIG_QAPP_UNIRTOS_SMS_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_sms.c")
endif()

if(CONFIG_QAPP_UNIRTOS_NW_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_network.c")
endif()

if(CONFIG_QAPP_UNIRTOS_USBNET_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_usbnet.c")
endif()

if(CONFIG_QAPP_UNIRTOS_QNWPREFCFG_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_qnwprefcfg.c")
endif()

if(CONFIG_QAPP_UNIRTOS_QNWPREFCFG_AT_FUNC OR CONFIG_QAPP_UNIRTOS_NW_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_network_common.c")
endif()

if(CONFIG_QAPP_UNIRTOS_DATACALL_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_datacall.c")
endif()

if(CONFIG_QAPP_UNIRTOS_MODEM_AT_BYPASS_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_modem_at.c")
endif()

if(CONFIG_QAPP_UNIRTOS_DEV_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_dev.c")
endif()

if(CONFIG_QAPP_UNIRTOS_WIFISCAN_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_wifiscan.c")
endif()

if(CONFIG_QAPP_UNIRTOS_SBFOTA_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_sbfota.c")
    list(APPEND LINK_LIB qcm_sbfota)
endif()

if(CONFIG_QAPP_UNIRTOS_LBS_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_lbs.c")
    list(APPEND LINK_LIB qcm_lbs)
endif()

if(CONFIG_QAPP_UNIRTOS_UART_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_uart.c")
endif()

if(CONFIG_QAPP_UNIRTOS_AT_LPM_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_lpm.c")
endif()

if(CONFIG_QAPP_UNIRTOS_AT_URC_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_urc.c")
    list(APPEND LINK_LIB qcm_urc)
endif()

if(CONFIG_QAPP_UNIRTOS_FTM_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_ftm.c")
endif()

if(CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_qdbgcfg.c")
endif()

if(CONFIG_QAPP_UNIRTOS_AT_QVSIM_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_qvsim.c")
    list(APPEND LINK_LIB qvsim)
endif()

if(CONFIG_QOSA_UIS8852_PLATFORM_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_uis8852.c")
endif()

if(CONFIG_QAPP_UNIRTOS_AT_FLASH_TEST_CMD)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_flash.c")
endif()

if(CONFIG_QAPP_UNIRTOS_LED_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_led.c")
endif()

if(CONFIG_QAPP_UNIRTOS_LICENSE_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_license.c")
endif()

if(CONFIG_QAPP_UNIRTOS_PDP_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_pdp_app.c")
endif()

if(CONFIG_QAPP_UNIRTOS_AT_QESIM_CMD)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_esim.c")
    list(APPEND LINK_LIB esim_sdk)
endif()

if(CONFIG_QOSA_EIGEN_PLATFORM_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_eigen.c")
endif()

if(CONFIG_QAPP_UNIRTOS_GNSS_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_gnss.c")
    list(APPEND LINK_LIB qcm_gnss)
endif()

if(CONFIG_QAPP_UNIRTOS_AT_CIND_CMD)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_cind.c")
endif()

if(CONFIG_QAPP_UNIRTOS_JD_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_jd.c")
endif()

if(CONFIG_QAPP_UNIRTOS_AT_QAUDIO_CMD)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_audio.c")
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_audio_task.c")
endif()

if(CONFIG_QAPP_UNIRTOS_SECBOOT_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_secboot.c")
endif()

if(CONFIG_QAPP_OEM_LT_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_LT.c")
endif()

if(CONFIG_QAPP_UNIRTOS_AT_CALL_CONTROL_CMD)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_voice_call.c")
endif()

if(CONFIG_QAPP_UNIRTOS_IMS_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_ims.c")
endif()

if(CONFIG_QAPP_UNIRTOS_CLAT_AT_FUNC)
    list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_clat.c")
endif()

# Default compiled files
list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_qcfg.c")
list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_lasterror.c")
list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_general.c")
# csdk compiles but does not copy, if this feature is needed apply separately
list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_factory.c")


# Virtual functions for AT are compiled and linked last
list(APPEND FILES_TO_ADD "${apps_file_dir}/unirtos_atcmd_virt_func.c")


# Targets to link for STD compilation
if (NOT PREBUILT_OBJECTS)
    list(REMOVE_DUPLICATES LINK_LIB)
    target_link_libraries(${target} ${LINK_LIB})
endif()

# Add files to list
foreach(LOGICAL_PATH IN LISTS FILES_TO_ADD)
    # Concatenate possible filenames, try two extensions
    set(BASENAME "${LOGICAL_PATH}")

    #message("BASENAME: ${BASENAME}")
    if(EXISTS "${BASENAME}")
        # message(STATUS "Found source file (original file): ${BASENAME}, adding to compile list")
        list(APPEND MY_SOURCES "${BASENAME}")
    endif()

    foreach(SUFFIX IN ITEMS ".o" ".obj")
        set(FULL_PATH "${BASENAME}${SUFFIX}")
        if(EXISTS "${FULL_PATH}")
            # message(STATUS "Found source file, adding to compile list: ${FULL_PATH}")
            list(APPEND MY_SOURCES "${FULL_PATH}")
        endif()
    endforeach()

    # Optional: output warning if no extension file is found
    # message(STATUS "No valid source file found: ${BASENAME} (tried .o .obj)")
endforeach()

# Add files to compilation
#message(STATUS "Final compilation files: ${MY_SOURCES}")
target_sources(${target} PRIVATE
    ${MY_SOURCES}
)
