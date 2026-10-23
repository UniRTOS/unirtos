

if(CONFIG_QCM_FILE_API_FUNC)
    add_subdirectory_if_exist(qcm_file)
endif()

if(CONFIG_QCM_QURL_FUNC)
    set(QURL_PLATFORM_CMAKE_FILE "${CMAKE_CURRENT_LIST_DIR}/qurl/platform/unirtos/platform.cmake")
    add_subdirectory_if_exist(qurl)

    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qurl)
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/qurl/CMakeLists.txt)
            add_components_libraries($<TARGET_FILE:qurl>)
        endif()
    endif()

endif()

if(CONFIG_QCM_SOCKET_ADP_FUNC)
    add_subdirectory_if_exist(socket)
endif()

if(CONFIG_QCM_MBEDTLS_LIBRARY_FUNC)
    add_subdirectory_if_exist(mbedtls)
endif()

if(CONFIG_QCM_VTLS_FUNC)
    add_subdirectory_if_exist(vtls)
endif()

if(CONFIG_QCM_COAP_FUNC)
    add_subdirectory_if_exist(coap)
endif()

if(CONFIG_QCM_COAP_API_FUNC)
    add_subdirectory_if_exist(qcm_coap)
endif()

if(CONFIG_QCM_LWM2M_FUNC)
    add_subdirectory_if_exist(lwm2m)
endif()

if(CONFIG_QCM_LWM2M_FOTA_FUNC)
    add_subdirectory_if_exist(lwm2m_fota)
endif()

if(CONFIG_QCM_MINI_HTTP_FUNC)
    add_subdirectory_if_exist(mini_http)
endif()

if(CONFIG_QCM_LBS_FUNC)
    add_subdirectory_if_exist(qcm_lbs)
endif()

if(CONFIG_QCM_MMS_FUNC)
    add_subdirectory_if_exist(qcm_mms)
endif()

if(CONFIG_QCM_MQTT_FUNC)
    add_subdirectory_if_exist(qcm_mqtt)
endif()

if(CONFIG_QCM_NTP_FUNC)
    add_subdirectory_if_exist(qcm_ntp)
endif()

if(CONFIG_QCM_SBFOTA_FUNC)
    add_subdirectory_if_exist(qcm_sbfota)
endif()

if(CONFIG_QCM_WEBSOCKET_FUNC)
    add_subdirectory_if_exist(qcm_websocket)
endif()

if(CONFIG_QCM_PING_FUNC)
    add_subdirectory_if_exist(ping)
endif()

if(CONFIG_QCM_URC_FUNC)
    add_subdirectory_if_exist(qcm_urc)
endif()

if(CONFIG_QCM_UART_LOG_FUNC)
    add_subdirectory_if_exist(qcm_uart_log)
endif()

if(CONFIG_QCM_SPI_NOR_FUNC)
    add_subdirectory_if_exist(qcm_spi_nor)
endif()

if(CONFIG_QCM_DMHTTP_FUNC)
    add_subdirectory_if_exist(qcm_dmhttp)
endif()

if(CONFIG_QCM_VIRT_AT_FUNC)
    add_subdirectory_if_exist(qcm_virt_at)
endif()

if(CONFIG_QCM_AUDIO_STREAM_FUNC)
    add_subdirectory_if_exist(qcm_audio)
endif()

if(CONFIG_QCM_AUDIO_MP3_FUNC)
    add_subdirectory_if_exist(qcm_mp3_dec)
endif()

if(CONFIG_QCM_AUDIO_AMR_FUNC)
    add_subdirectory_if_exist(qcm_amr)
endif()


if(CONFIG_QCM_ESIM_FUNC)
    add_subdirectory_if_exist(esim_sdk)
endif()

if(CONFIG_QCM_QVSIM_FUNC)
add_subdirectory_if_exist(qvsim)
endif()

if(CONFIG_QCM_GNSS_FUNC)
add_subdirectory_if_exist(qcm_gnss)
endif()

if(CONFIG_QOSA_OPENCPU_FUNC)
add_subdirectory_if_exist(qcm_app_init)
endif()

if(CONFIG_QCM_MONGOOS_LIB)
    add_subdirectory_if_exist(mongoos-7.10)
endif()

if(CONFIG_QCM_LLM_PROXY)
    add_subdirectory_if_exist(qcm_llm_proxy)
endif()

# Virtual function Make sure to put it at the end.
add_subdirectory_if_exist(virt_func)

add_subdirectory_if_exist(utils)
add_subdirectory_if_exist(init)
