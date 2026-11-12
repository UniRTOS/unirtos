/*****************************************************************/ /**
* @file unirtos_atcmd_def.h
* @brief
* @author larson.li@quectel.com
* @date 2024-04-05
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-04-05 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_DEF_H__
#define __UNIRTOS_ATCMD_DEF_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"
#include "unirtos_atcmd_cfg.h"

#include "unirtos_atcmd_qcfg.h"
#include "unirtos_atcmd_uart.h"
#include "unirtos_atcmd_general.h"
#include "unirtos_atcmd_version.h"

#ifdef CONFIG_QOSA_OEM_DL_FUNC
#include "unirtos_atcmd_gw_cfg.h"
#endif
#ifdef CONFIG_QAPP_UNIRTOS_SECBOOT_AT_FUNC
#include "unirtos_atcmd_secboot.h"
#endif /*CONFIG_QAPP_UNIRTOS_SECBOOT_AT_FUNC*/

#ifdef CONFIG_QAPP_UNIRTOS_FILE_AT_FUNC
#include "unirtos_atcmd_file.h"
#endif /* CONFIG_QAPP_UNIRTOS_FILE_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_CAMERA_AT_FUNC
#include "unirtos_atcmd_camera.h"
#endif /* CONFIG_QAPP_UNIRTOS_CAMERA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SIM_AT_FUNC
#include "unirtos_atcmd_sim.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_SMS_AT_FUNC
#include "unirtos_atcmd_sms.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_NW_AT_FUNC
#include "unirtos_atcmd_network.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_QNWPREFCFG_AT_FUNC
#include "unirtos_atcmd_qnwprefcfg.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_USBNET_AT_FUNC
#include "unirtos_atcmd_usbnet.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_CMUX_AT_FUNC
#include "unirtos_atcmd_cmux.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_AGORA_RTC_AT_FUNC
#include "unirtos_atcmd_agora_rtc.h"
#endif /* CONFIG_QAPP_UNIRTOS_AGORA_RTC_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_DATACALL_AT_FUNC
#include "unirtos_atcmd_datacall.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_MIFI_AT_FUNC
#include "unirtos_atcmd_mifi.h"
#endif /* CONFIG_QAPP_UNIRTOS_MIFI_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CFUN_CMD
#include "unirtos_atcmd_dev.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_QINDCFG_AT_FUNC
#include "unirtos_atcmd_qindcfg.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_TCPIP_AT_FUNC
#include "unirtos_atcmd_tcpip.h"
#ifdef CONFIG_QAPP_UNIRTOS_SSL_AT_FUNC
#include "unirtos_atcmd_ssl.h"
#endif /* CONFIG_QAPP_UNIRTOS_SSL_AT_FUNC */
#endif /* CONFIG_QAPP_UNIRTOS_TCPIP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC
#include "unirtos_atcmd_tcpip_nb.h"
#endif /* CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_PING_AT_FUNC
#include "unirtos_atcmd_ping.h"
#endif /* CONFIG_QAPP_UNIRTOS_PING_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_HY_ANDLINK_AT_FUNC
#include "unirtos_atcmd_hy_andlink.h"
#endif /* CONFIG_QAPP_UNIRTOS_HY_ANDLINK_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LWM2M_AT_FUNC
#include "unirtos_atcmd_lwm2m.h"
#endif /* CONFIG_QAPP_UNIRTOS_LWM2M_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_ONENET_LWM2M_AT_FUN
#include "unirtos_atcmd_mipl_lwm2m.h"
#endif /* CONFIG_QAPP_UNIRTOS_ONENET_LWM2M_AT_FUN */

#ifdef CONFIG_QAPP_UNIRTOS_AEP_LWM2M_AT_FUN
#include "unirtos_atcmd_aepiot_lwm2m.h"
#endif /* CONFIG_QAPP_UNIRTOS_AEP_LWM2M_AT_FUN */

#ifdef CONFIG_QAPP_UNIRTOS_UNICOM_LWM2M_AT_FUNC
#include "unirtos_atcmd_unicom_lwm2m.h"
#endif /* CONFIG_QAPP_UNIRTOS_UNICOM_LWM2M_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_HTTP_AT_FUNC
#include "unirtos_atcmd_http.h"
#endif /* CONFIG_QAPP_UNIRTOS_HTTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SMTP_AT_FUNC
#include "unirtos_atcmd_smtp.h"
#endif /* CONFIG_QAPP_UNIRTOS_SMTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_UNIFY_AT_FUNC
#include "unirtos_atcmd_unify.h"
#endif /* CONFIG_QAPP_UNIRTOS_UNIFY_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_FTP_AT_FUNC
#include "unirtos_atcmd_ftp.h"
#endif /* CONFIG_QAPP_UNIRTOS_FTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_GCOV_AT_FUNC
#include "unirtos_atcmd_gcov.h"
#endif /* CONFIG_QAPP_UNIRTOS_GCOV_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_MMS_AT_FUNC
#include "unirtos_atcmd_mms.h"
#endif /* CONFIG_QAPP_UNIRTOS_MMS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_MQTT_AT_FUNC
#include "unirtos_atcmd_mqtt.h"
#endif /* CONFIG_QAPP_UNIRTOS_MQTT_AT_FUNC */

#include "unirtos_atcmd_lasterror.h"

#ifdef CONFIG_QAPP_UNIRTOS_NTP_AT_FUNC
#include "unirtos_atcmd_ntp.h"
#endif /* CONFIG_QAPP_UNIRTOS_NTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_WEBSOCKET_AT_FUNC
#include "unirtos_atcmd_websocket.h"
#endif /* CONFIG_QAPP_UNIRTOS_WEBSOCKET_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_DMHTTP_AT_FUNC
#include "unirtos_atcmd_dmhttp.h"
#endif /* CONFIG_QAPP_UNIRTOS_DMHTTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_FOTA_AT_FUNC
#include "unirtos_atcmd_qfotadl.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_PPP_AT_FUNC
#include "unirtos_atcmd_ppp.h"
#endif /* CONFIG_QAPP_UNIRTOS_PPP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LBS_AT_FUNC
#include "unirtos_atcmd_lbs.h"
#endif /* CONFIG_QAPP_UNIRTOS_LBS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_FIREWALL_AT_FUNC
#include "unirtos_atcmd_firewall.h"
#endif /* CONFIG_QAPP_UNIRTOS_FIREWALL_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_UART_AT_FUNC
#include "unirtos_atcmd_uart.h"
#endif /* CONFIG_QAPP_UNIRTOS_UART_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SBFOTA_AT_FUNC
#include "unirtos_atcmd_sbfota.h"
#endif /* CONFIG_QAPP_UNIRTOS_SBFOTA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LED_AT_FUNC
#include "unirtos_atcmd_led.h"
#endif /* CONFIG_QAPP_UNIRTOS_LED_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LICENSE_AT_FUNC
#include "unirtos_atcmd_license.h"
#endif /* CONFIG_QAPP_UNIRTOS_LICENSE_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_QINGYA_CODE_DECODER_AT_FUNC
#include "unirtos_atcmd_qingya.h"
#endif /* CONFIG_QAPP_UNIRTOS_QINGYA_CODE_DECODER_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_LPM_FUNC
#include "unirtos_atcmd_lpm.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_LPM_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_URC_FUNC
#include "unirtos_atcmd_urc.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_URC_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_COAP_AT_FUNC
#include "unirtos_atcmd_coap.h"
#endif /* CONFIG_QAPP_UNIRTOS_COAP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_WIFISCAN_AT_FUNC
#include "unirtos_atcmd_wifiscan.h"
#endif /* CONFIG_QAPP_UNIRTOS_WIFISCAN_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_WIFI_AT_FUNC
#include "unirtos_atcmd_wifi.h"
#endif /* CONFIG_QAPP_UNIRTOS_WIFI_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD
#include "unirtos_atcmd_qdbgcfg.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCLI_CMD
#include "unirtos_atcmd_qcli.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCLI_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_FACTORY_AT_FUNC
#include "unirtos_atcmd_factory.h"
#endif /* CONFIG_QAPP_UNIRTOS_FACTORY_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QVSIM_FUNC
#include "unirtos_atcmd_qvsim.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_QVSIM_FUNC */

// clang-format off

#ifdef CONFIG_QOSA_UIS8852_PLATFORM_FUNC
#include "unirtos_atcmd_uis8852.h"
#endif /* CONFIG_QOSA_UIS8852_PLATFORM_FUNC */

#ifdef CONFIG_QOSA_UIS8360S_PLATFORM_FUNC
#include "unirtos_atcmd_uis8360s.h"
#endif /* CONFIG_QOSA_UIS8360S_PLATFORM_FUNC */

#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
#include "unirtos_atcmd_qt2131.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_AT_FLASH_TEST_CMD
#include "unirtos_atcmd_flash.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_FLASH_TEST_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_FTM_AT_FUNC
#include "unirtos_atcmd_ftm.h"
#endif /* CONFIG_QAPP_UNIRTOS_FTM_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_NB_AT_FUNC
#include "unirtos_atcmd_nb.h"
#endif /* CONFIG_QAPP_UNIRTOS_NB_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCPULOAD_CMD
#include "unirtos_atcmd_qcpuload.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCPULOAD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_JD_AT_FUNC
#include "unirtos_atcmd_jd.h"
#endif /* CONFIG_QAPP_UNIRTOS_JD_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QAUDIO_CMD
#include "unirtos_atcmd_audio_task.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_QAUDIO_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QESIM_CMD
#include "unirtos_atcmd_esim.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_QESIM_CMD */

#if defined(CONFIG_QOSA_EIGEN_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX_PLATFORM_FUNC)
#include "unirtos_atcmd_eigen.h"
#endif /* CONFIG_QOSA_EIGEN_PLATFORM_FUNC || CONFIG_QOSA_QCX_PLATFORM_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_GNSS_AT_FUNC
#include "unirtos_atcmd_gnss.h"
#endif /* CONFIG_QAPP_UNIRTOS_GNSS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CIND_CMD
#include "unirtos_atcmd_cind.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_CIND_CMD */

#ifdef CONFIG_QAPP_OEM_LT_AT_FUNC
#include "unirtos_atcmd_LT.h"
#include "unirtos_atcmd_lt_tcp.h"
#endif /* CONFIG_QAPP_OEM_LT_AT_FUNC*/
#ifdef CONFIG_QAPP_UNIRTOS_STD_NEOWAY_FUNC
#include "unirtos_atcmd_nw.h"
#endif /* CONFIG_QAPP_UNIRTOS_STD_NEOWAY_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CALL_CONTROL_CMD
#include "unirtos_atcmd_voice_call.h"
#endif /* CONFIG_QAPP_UNIRTOS_AT_CALL_CONTROL_CMD */

#ifdef  CONFIG_QOSA_OEM_DL_FUNC
#include "unirtos_atcmd_gw.h"
#endif

#ifdef CONFIG_QAPP_UNIRTOS_SPINET_AT_FUNC
#include "unirtos_atcmd_spinet.h"
#endif /* CONFIG_QAPP_UNIRTOS_SPINET_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_EG21_AT_FUNC
#include "unirtos_atcmd_eg21.h"
#endif /* CONFIG_QAPP_UNIRTOS_EG21_AT_FUNC */

void qstd_exec_qatlist_cmd(qosa_at_cmd_t *cmd);

qosa_at_desc_t unir_img_at_desc[] = {

    {"+QATLIST" , qstd_exec_qatlist_cmd, 0},

#ifdef CONFIG_QAPP_UNIRTOS_FILE_AT_FUNC
    {"+QFUPL" , qstd_exec_file_qfupl_cmd, 0},
    {"+QFDWL" , qstd_exec_file_qfdwl_cmd, 0},
    {"+QFMKDIR" , qstd_exec_file_qfmkdir_cmd, 0},
    {"+QFRMDIR" , qstd_exec_file_qfrmdir_cmd, 0},
    {"+QFOPEN" , qstd_exec_file_qfopen_cmd, 0},
    {"+QFLST" , qstd_exec_file_qflst_cmd, 0},
    {"+QFDEL" , qstd_exec_file_qfdel_cmd, 0},
    {"+QFCLOSE" , qstd_exec_file_qfclose_cmd, 0},
    {"+QFREAD" , qstd_exec_file_qfread_cmd, 0},
    {"+QFLDS" , qstd_exec_file_qflds_cmd, 0},
    {"+QFWRITE" , qstd_exec_file_qfwrite_cmd, 0},
    {"+QFSEEK" , qstd_exec_file_qfseek_cmd, 0},
    {"+QFPOSITION" , qstd_exec_file_qfposition_cmd, 0},
    {"+QFTUCAT" , qstd_exec_file_qftucat_cmd, 0},
    #ifdef CONFIG_QAPP_UNIRTOS_AT_QFMD5_CMD
    {"+QFMD5", qstd_exec_file_qfmd5_cmd, 0},
    #endif /*CONFIG_QAPP_UNIRTOS_AT_QFMD5_CMD*/
#endif /*CONFIG_QAPP_UNIRTOS_STD_FUNC*/

#ifdef CONFIG_QAPP_UNIRTOS_AT_FLASH_TEST_CMD
    {"+QFTEST" , unir_exec_qftest_cmd, 0},
#endif /*CONFIG_QAPP_UNIRTOS_AT_FLASH_TEST_CMD*/

#ifdef CONFIG_QAPP_UNIRTOS_CAMERA_AT_FUNC
    {"+QCAMOPEN" , qstd_exec_camera_qcamopen_cmd, 0},
    {"+QCAMIDFY" , qstd_exec_camera_qcamidfy_cmd, 0},
    {"+QCAMCAP" , qstd_exec_camera_qcamcap_cmd, 0},
    {"+QCAMREAD" , qstd_exec_camera_qcamread_cmd, 0},
    {"+QCAMPRE" , qstd_exec_camera_qcampre_cmd, 0},
    {"+QCAMCLOSE" , qstd_exec_camera_qcamclose_cmd, 0},
#ifdef CONFIG_QAPP_UNIRTOS_CAMERA_WATER_METER_AI_AT_FUNC
    {"+QCAMWMAIOPEN" , qstd_exec_camera_qcamwmaiopen_cmd, 0},
    {"+QCAMWMAIINFER" , qstd_exec_camera_qcamwmaiinfer_cmd, 0},
    {"+QCAMWMAIVER" , qstd_exec_camera_qcamwmaiver_cmd, 0},
    {"+QCAMWMAICLOSE" , qstd_exec_camera_qcamwmaiclose_cmd, 0},
#endif /*CONFIG_QAPP_UNIRTOS_CAMERA_WATER_METER_AI_AT_FUNC*/
#endif /*CONFIG_QAPP_UNIRTOS_STD_FUNC*/

#ifdef CONFIG_QAPP_UNIRTOS_SIM_AT_FUNC
    {"+CIMI",          qstd_exec_cimi_cmd,             0},
    {"+CLCK",          qstd_exec_clck_cmd,             0},
    {"+CPIN",          qstd_exec_cpin_cmd,             0},
    {"+CPWD",          qstd_exec_cpwd_cmd,             0},
    {"+CSIM",          qstd_exec_csim_cmd,             0},
    {"+CRSM",          qstd_exec_crsm_cmd,             0},
    {"+CCID",          qstd_exec_qccid_cmd,            0},
    {"+ICCID",         qstd_exec_qccid_cmd,            0},
    {"+QCCID",         qstd_exec_qccid_cmd,            0},
#if defined (CONFIG_QOSA_NW_NB_SUPPORT) || defined(CONFIG_QOSA_NW_BJU_SUPPORT)
    {"+NCCID",         qstd_exec_nccid_cmd,            0},
#endif /*CONFIG_QOSA_NW_NB_SUPPORT*/
    {"+QINISTAT",      qstd_exec_qinistat_cmd,         0},
    {"+QSIMDET",       qstd_exec_qsimdet_cmd,          0},
    {"+QSIMSTAT",      qstd_exec_qsimstat_cmd,         0},
    {"+CCHO",          qstd_exec_ccho_cmd,             0},
    {"+CGLA",          qstd_exec_cgla_cmd,             0},
    {"+CCHC",          qstd_exec_cchc_cmd,             0},
    {"+CNUM",          qstd_exec_cnum_cmd,             0},

#ifdef CONFIG_QAPP_UNIRTOS_AT_STK_CMD
    {"^STA",           qstd_exec_sta_cmd,              0},
    {"^STGI",          qstd_exec_stgi_cmd,             0},
    {"^STR",           qstd_exec_str_cmd,              0},
#endif

#ifdef CONFIG_QOSA_DSDS_FUNC
    {"+QDSCCID",       qstd_exec_qdsccid_cmd,          0},
    {"+QDSIMI",        qstd_exec_qdsimi_cmd,           0},
#endif /* CONFIG_QOSA_DSDS_FUNC */

    {"+QDSIM",         qstd_exec_qdsim_cmd,            0},
    {"+QUIMSLOT",      qstd_exec_qdsim_cmd,            0},

#ifdef CONFIG_QOSA_DSDS_FUNC
    {"+QDSPIN",        qstd_exec_qdspin_cmd,           0},
#endif /* CONFIG_QOSA_DSDS_FUNC */

    {"+qdstype",       qstd_exec_qdstype_cmd,          0},
    {"+QPINC",         qstd_exec_qpinc_cmd,            0},
    {"+qslotstat",     qstd_exec_qslotstat_cmd,        0},
#ifdef CONFIG_QAPP_UNIRTOS_AT_QWRITESIM_CMD
    {"+qwritesim",     qstd_exec_qwritesim_cmd,        0},
#endif
#endif

#ifdef CONFIG_QAPP_UNIRTOS_NW_AT_FUNC
    {"+CREG",          qstd_exec_creg_cmd,         0},
    {"+CGREG",         qstd_exec_cgreg_cmd,        0},
    {"+CEREG",         qstd_exec_cereg_cmd,        0},
    {"+C5GREG",        qstd_exec_c5greg_cmd,       0},
    {"+C5GNSSAI",      qstd_exec_c5gnssai_cmd,     0},
    {"+C5GNSSAIRDP",   qstd_exec_c5gnssairdp_cmd,  0},
    {"+QNWINFO",       qstd_exec_qnwinfo_cmd,      0},
    {"+QENG",          qstd_exec_qeng_cmd,         0},
    {"+QCELL",         qstd_exec_qcell_cmd,        0},
    {"+QCELLEX",       qstd_exec_qcellex_cmd,      0},
    {"+QCELLINFO",     qstd_exec_qcellinfo_cmd,    0},
    {"+CSQ",           qstd_exec_csq_cmd,          0},
    {"+CESQ",          qstd_exec_cesq_cmd,         0},
    {"+QCSQ",          qstd_exec_qcsq_cmd,         0},
    {"+CTZR",          qstd_exec_ctzr_cmd,         0},
    {"+CTZU",          qstd_exec_ctzu_cmd,         0},
    {"+COPS",          qstd_exec_cops_cmd,         0},
#ifdef CONFIG_QOSA_NW_EDRX_FUNC
#ifdef CONFIG_QAPP_UNIRTOS_EDRX_AT_FUNC
    {"+CEDRXS",        qstd_exec_cedrxs_cmd,       0},
    {"+QPTWEDRXS",     qstd_exec_qptwedrxs_cmd,    0},
    {"+CEDRXRDP",      qstd_exec_cedrxrdp_cmd,     0},
#endif /*CONFIG_QAPP_UNIRTOS_EDRX_AT_FUNC*/
#endif /* CONFIG_QOSA_NW_EDRX_FUNC */
#ifdef CONFIG_QOSA_SIM_FUNC
    {"+QSPN",          qstd_exec_qspn_cmd,         0},
#endif /* CONFIG_QOSA_SIM_FUNC */
    {"+QLTS",          qstd_exec_qlts_cmd,         0},
    {"+COPN",          qstd_exec_copn_cmd,         0},
    // {"+CPLS",          qstd_exec_cpls_cmd,         0},
    // {"+CPOL",          qstd_exec_cpol_cmd,         0},
    {"+QNWLOCK",       qstd_exec_qnwlock_cmd,      0},
    {"+CSCON",         qstd_exec_cscon_cmd,        0},
    {"+QOPS",          qstd_exec_qops_cmd,         0},
    {"+QSCAN",         qstd_exec_qscan_cmd,        0},
#ifdef CONFIG_QAPP_UNIRTOS_AT_QSCANEX_CMD
    {"+QSCANEX",       qstd_exec_qscanex_cmd,      0},
#endif
#ifdef CONFIG_QAPP_UNIRTOS_AT_QANTINFO_CMD
    {"+QANTINFO",      qstd_exec_qantinfo_cmd,     0},
#endif /* CONFIG_QAPP_UNIRTOS_AT_QANTINFO_CMD */
    // {"+QOPSCFG",       qstd_exec_qopscfg_cmd,      0},
    // {"+CVMOD",         qstd_exec_cvmod_cmd,        0},
    // {"+CEUS",          qstd_exec_ceus_cmd,         0},
    // {"+QNWCFG",        qstd_exec_qnwcfg_cmd,       0},
#ifdef CONFIG_QAPP_UNIRTOS_CEMODE_AT_FUNC
     {"+CEMODE",        qstd_exec_cemode_cmd,       0},
#endif /*CONFIG_QAPP_UNIRTOS_CEMODE_AT_FUNC*/
#ifdef CONFIG_QAPP_UNIRTOS_CGEREP_AT_FUNC
     {"+CGEREP",        qstd_exec_cgerep_cmd,       0},
#endif /*CONFIG_QAPP_UNIRTOS_CGEREP_AT_FUNC*/
#ifdef CONFIG_QAPP_UNIRTOS_QBAND_AT_FUNC
    {"+QBAND",         qstd_exec_qband_cmd,        0},
#endif /* #ifdef CONFIG_QAPP_UNIRTOS_QBAND_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_ABNORMAL_BASESTATION_AT_FUNC
     {"+QBLACKCELL",        qstd_exec_qblackcell_cmd,       0},
     {"+QBLACKCELLCFGEX",   qstd_exec_qblackcellcfgex_cmd,  0},
     {"+QBLACKCELLWEIGHT",  qstd_exec_qblackcellweight_cmd, 0},
#endif /*CONFIG_QAPP_UNIRTOS_ABNORMAL_BASESTATION_AT_FUNC*/


#endif /*CONFIG_QAPP_UNIRTOS_NW_AT_FUNC*/

#ifdef CONFIG_QAPP_UNIRTOS_QCEER_AT_FUNC
    {"+QCEER",         qstd_exec_qceer_cmd,        0},
#endif

#ifdef CONFIG_QAPP_UNIRTOS_CEER_AT_FUNC
    {"+CEER",          qstd_exec_ceer_cmd,        0},
#endif

#ifdef CONFIG_QAPP_UNIRTOS_WIFISCAN_AT_FUNC
    {"+QWIFISCAN",     qstd_exec_qwifiscan_cmd,        0},
#endif

#ifdef CONFIG_QAPP_UNIRTOS_WIFI_AT_FUNC
    {"+QWIFI",         qstd_exec_qwifi_cmd,            0},
    {"+QWSSID",        qstd_exec_qwssid_cmd,           0},
    {"+QWAUTH",        qstd_exec_qwauth_cmd,           0},
    {"+QWMOCH",        qstd_exec_qwmoch_cmd,           0},
    {"+QWCLIP",        qstd_exec_qwclip_cmd,           0},
#endif

#ifdef CONFIG_QAPP_UNIRTOS_QINDCFG_AT_FUNC
    {"+QINDCFG",       qstd_exec_qindcfg_cmd,      0},
#endif

#ifdef CONFIG_QAPP_UNIRTOS_QNWPREFCFG_AT_FUNC
    {"+QNWPREFCFG",    qstd_exec_qnwprefcfg_cmd,     0 },
#endif

#ifdef CONFIG_QAPP_UNIRTOS_USBNET_AT_FUNC
    {"+QNETDEVCTL",    qstd_exec_qnetdevctl_cmd,     0 },
#ifdef CONFIG_QAPP_UNIRTOS_QDMZ_AT_FUNC
    {"+QDMZ",          qstd_exec_qdmz_cmd,         0},
#endif /* CONFIG_QAPP_UNIRTOS_QDMZ_AT_FUNC */
#endif // !CONFIG_QAPP_UNIRTOS_USBNET_AT_FUNC

#ifdef CONFIG_QAPP_UNIRTOS_DATACALL_AT_FUNC
    {"+CGDCONT",       qstd_exec_cgdcont_cmd,      0},
    {"+CGAUTH",        qstd_exec_cgauth_cmd,       0},
    {"+CGATT",         qstd_exec_cgatt_cmd,        0},
    {"+CGACT",         qstd_exec_cgact_cmd,        0},
    // {"+CGDATA",        qstd_exec_cgdata_cmd,       0},
    {"+CGPADDR",       qstd_exec_cgpaddr_cmd,      0},
    {"+CGQREQ",        qstd_exec_cgqreq_cmd,       0},
    {"+QGDCNT",        qstd_exec_qgdcnt_cmd,       0},
    {"+QPDPGDCNT",     qstd_exec_qpdpgdcnt_cmd,    0},
    {"+QAUGDCNT",      qstd_exec_qaugdcnt_cmd,     0},
    {"+CGCONTRDP",     qstd_exec_cgcontrdp_cmd,    0},
#endif

//#ifdef CONFIG_QAPP_UNIRTOS_MIFI_AT_FUNC
#if defined(CONFIG_QAPP_UNIRTOS_MIFI_AT_FUNC) && defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)

#ifdef CONFIG_QAPP_UNIRTOS_MIFI_NET_AT_FUNC
    {"+QMIFINET",      qstd_exec_qmifinet_cmd,      0},
#endif

#ifdef CONFIG_QAPP_UNIRTOS_MIFI_SYSTEST_AT_FUNC
    {"+QLAPPTEST",     qstd_exec_qlapptest_cmd,     0},
    {"+QLAPPTESTEX",   qstd_exec_qlapptestex_cmd,   0},
    {"+QLAPPTESTACT",  qstd_exec_qlapptestact_cmd,  0},
    {"+QSLEEPTEST",    qstd_exec_qsleeptest_cmd,    0},
    {"+QLEDTEST",      qstd_exec_qledtest_cmd,      0},
    {"+QKEYTEST",      qstd_exec_qkeytest_cmd,      0},
    {"+QBATTERYTEST",  qstd_exec_qbatterytest_cmd,  0},
    {"+QBATTERYSOC",   qstd_exec_qbatterysoc_cmd,   0},
    {"+OGMR",          qstd_exec_ogmr_cmd,           0},
    {"+QODMVER",       qstd_exec_qodmver_cmd,        0},
    {"+QODMSUB",       qstd_exec_qodmsub_cmd,        0},
#endif

#ifdef CONFIG_QOSA_ASR1805_PLATFORM_FUNC   /* 8360 by unirtos_atcmd_uis8360s.c */
    {"+QWIFITEST",     qstd_exec_qwifitest_cmd,     0},
#endif

#ifdef CONFIG_QAPP_UNIRTOS_MIFI_WIFI_AT_FUNC
    {"+QWIFI",         qstd_exec_wifi_cmd,           0},
    {"+QWSSID",        qstd_exec_qwssid_cmd,         0},
    {"+QWAUTH",        qstd_exec_qwauth_cmd,         0},
#endif

#ifdef CONFIG_QAPP_UNIRTOS_MIFI_SIM_AT_FUNC
    {"+QSWSIM",        qstd_exec_qswsim_cmd,         0},
    {"+QSIMBAN",       qstd_exec_qsimban_cmd,        0},
    {"+QSIMPOLL",      qstd_exec_qsimpoll_cmd,       0},
#endif

#ifdef CONFIG_QAPP_UNIRTOS_MIFI_OPAUTH_AT_FUNC
    {"+QOPAUTHSIMINFO", qstd_exec_qopauthsiminfo_cmd, 0},
#endif

#endif /* CONFIG_QAPP_UNIRTOS_MIFI_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_PPP_AT_FUNC
    {"+QPPPDROP",      qstd_exec_qpppdrop_cmd,     0},
#endif /* CONFIG_QAPP_UNIRTOS_PPP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SMS_AT_FUNC
    {"+CMGD",          qstd_exec_cmgd_cmd,         0},
    {"+CMGF",          qstd_exec_cmgf_cmd,         0},
    {"+CMGL",          qstd_exec_cmgl_cmd,         0},
    {"+CMGR",          qstd_exec_cmgr_cmd,         0},
    {"+QCMGR",         qstd_exec_qcmgr_cmd,        0},
    {"+CMGS",          qstd_exec_cmgs_cmd,         0},
    {"+QCMGS",         qstd_exec_qcmgs_cmd,        0},
    {"+CMGW",          qstd_exec_cmgw_cmd,         0},
    {"+CMMS",          qstd_exec_cmms_cmd,         0},
    {"+CMSS",          qstd_exec_cmss_cmd,         0},
    {"+CNMA",          qstd_exec_cnma_cmd,         0},
    {"+CNMI",          qstd_exec_cnmi_cmd,         0},
    {"+CPMS",          qstd_exec_cpms_cmd,         0},
#ifdef CONFIG_QAPP_UNIRTOS_AT_CPNER_CMD
    {"+CPNER",         qstd_exec_cpner_cmd,        0},
#endif /* CONFIG_QAPP_UNIRTOS_AT_CPNER_CMD */
    {"+CSCA",          qstd_exec_csca_cmd,         0},
    // {"+CSCB",          qstd_exec_cscb_cmd,         0},
    {"+CSDH",          qstd_exec_csdh_cmd,         0},
    {"+CSMP",          qstd_exec_csmp_cmd,         0},
    {"+CSMS",          qstd_exec_csms_cmd,         0},
#endif

#ifdef CONFIG_QAPP_UNIRTOS_AT_CFUN_CMD
    {"+CFUN",      qstd_exec_cfun_cmd,             0},
#endif /* CONFIG_QAPP_UNIRTOS_DEV_AT_FUNC */
#ifdef CONFIG_QAPP_UNIRTOS_AT_CCLK_CMD
#ifndef CONFIG_QAPP_UNIRTOS_NB_AT_FUNC
    {"+CCLK",      qstd_exec_cclk_cmd,             0},
#endif/*CONFIG_QAPP_UNIRTOS_NB_AT_FUNC*/
#endif /* CONFIG_QAPP_UNIRTOS_AT_CCLK_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC
    {"+NSOCR",      unir_exec_nb_nsocr_cmd,        0},
    {"+NSOCO",      unir_exec_nb_nsoco_cmd,        0},
    {"+NSOST",      unir_exec_nb_nsost_cmd,        0},
    {"+NSOSTEX",    unir_exec_nb_nsostex_cmd,      0},
    {"+NSOSD",      unir_exec_nb_nsosd_cmd,        0},
    {"+NSOSDEX",    unir_exec_nb_nsosdex_cmd,      0},
    {"+NQSOS",      unir_exec_nb_nqsos_cmd,        0},
    {"+NSORF",      unir_exec_nb_nsorf_cmd,        0},
    {"+NSOCL",      unir_exec_nb_nsocl_cmd,        0},
    {"+NSONMI",     unir_exec_nb_nsonmi_cmd,       0},
    {"+NSOSTATUS",  unir_exec_nb_nsostatus_cmd,    0},
    {"+QIDNSCFG",   unir_exec_nb_qidnscfg_cmd,     0},
    {"+QDNS",       unir_exec_nb_qdns_cmd,         0},
#endif /* CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_NB_AT_FUNC
    {"+NUESTATS",   qstd_exec_nuestats_cmd,        0},
    {"+CCLK",       qstd_exec_nb_cclk_cmd,         0},
    {"+NRB",        qstd_exec_nrb_cmd,             0},
#endif /* CONFIG_QAPP_UNIRTOS_NB_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NPSMR_CMD
    {"+NPSMR",      qstd_exec_npsmr_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_NPSMR_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NCONFIG_CMD
    {"+NCONFIG",    qstd_exec_nconfig_cmd,             0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_NCONFIG_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NPIN_CMD
    {"+NPIN",       qstd_exec_npin_cmd,            0},
#endif /* CONFIG_QAPP_UNIRTOS_AT_NPIN_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NBAND_CMD
    {"+NBAND",         qstd_exec_nband_cmd,        0},
#endif /* CONFIG_QAPP_UNIRTOS_AT_NBAND_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NCSEARFCN_CMD
    {"+NCSEARFCN",     qstd_exec_ncsearfcn_cmd,    0},
#endif /* CONFIG_QAPP_UNIRTOS_AT_NCSEARFCN_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NEARFCN_CMD
    {"+NEARFCN",        qstd_exec_nearfcn_cmd,      0},
#endif /* CONFIG_QAPP_UNIRTOS_AT_NEARFCN_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_JD_AT_FUNC
    {"+QJDCFG",   qstd_exec_qjdcfg_cmd,             0},
    {"+QJDR",     qstd_exec_qjdr_cmd,              0},
#endif /* CONFIG_QAPP_UNIRTOS_JD_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_TCPIP_AT_FUNC
    {"+QIOPEN" , qstd_exec_tcpip_qiopen_cmd, 0},
    {"+QISENDEX" , qstd_exec_tcpip_qisendex_cmd, 0},
    {"+QIRD" , qstd_exec_tcpip_qird_cmd, 0},
    {"+QICLOSE" , qstd_exec_tcpip_qiclose_cmd, 0},
    {"+QISEND" , qstd_exec_tcpip_qisend_cmd, 0},
    {"+QISENDHEX" , qstd_exec_tcpip_qisendhex_cmd, 0},
    {"+QISTATE" , qstd_exec_tcpip_qistate_cmd, 0},
    {"+QISWTMD" , qstd_exec_tcpip_qiswtmd_cmd, 0},
    {"+QISDE" , qstd_exec_tcpip_qisde_cmd, 0},
    {"+QIGETERROR" , qstd_exec_tcpip_qigeterror_cmd, 0},
    {"+QIACCEPT" , qstd_exec_tcpip_qiaccept_cmd, 0},
    {"+QICFG" , qstd_exec_tcpip_qicfg_cmd, 0},
    {"+QICSGP" , qstd_exec_tcpip_qicsgp_cmd, 0},
    {"+QIACT" , qstd_exec_tcpip_qiact_cmd, 0},
    {"+QIACTEX" , qstd_exec_tcpip_qiactex_cmd, 0},
    {"+QIDEACT" , qstd_exec_tcpip_qideact_cmd, 0},
    {"+QIDEACTEX" , qstd_exec_tcpip_qideactex_cmd, 0},
#ifndef CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC
    {"+QIDNSCFG" , qstd_exec_tcpip_qidnscfg_cmd, 0},
#endif/*CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC*/
    {"+QIDNSGIP" , qstd_exec_tcpip_qidnsgip_cmd, 0},
#ifdef CONFIG_QAPP_UNIRTOS_PING_AT_FUNC
    {"+QPING", qstd_exec_tcpip_qping_cmd, 0},
#endif /* CONFIG_QAPP_UNIRTOS_PING_AT_FUNC */
#ifdef CONFIG_QAPP_UNIRTOS_NTP_AT_FUNC
    {"+QNTP", qstd_exec_tcpip_qntp_cmd, 0},
#endif /* CONFIG_QAPP_UNIRTOS_NTP_AT_FUNC */
#ifdef CONFIG_QAPP_UNIRTOS_SSL_AT_FUNC
    {"+QSSLCFG" , qstd_ssl_qsslcfg_cmd_func, 0},
    {"+QSSLSEND" , qstd_exec_ssl_qsslsend_cmd_func, 0},
    {"+QSSLSTATE" , qstd_exec_ssl_qsslstate_cmd, 0},
    {"+QSSLOPEN" , qstd_exec_ssl_qsslopen_cmd, 0},
    {"+QSSLRECV" , qstd_exec_ssl_qsslrecv_cmd, 0},
    {"+QSSLCLOSE" , qstd_exec_ssl_qsslclose_cmd, 0},
    {"+QSSLOPENEEX" , qstd_exec_ssl_qsslopenex_cmd, 0},
#endif /* CONFIG_QAPP_UNIRTOS_SSL_AT_FUNC */
#endif /* CONFIG_QAPP_UNIRTOS_TCPIP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LBS_AT_FUNC
    {"+QLBSCFG", qsd_exec_lbs_qlbscfg_cmd_func, 0},
    {"+QLBS", qsd_exec_lbs_qlbs_cmd_func, 0},
    {"+QLBSEX", qsd_exec_lbs_qlbs_ext_input_cmd_func, 0},
#endif /* CONFIG_QAPP_UNIRTOS_LBS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_FIREWALL_AT_FUNC
    {"+QFIREWALLCFG", qstd_exec_firewall_qfcfg_cmd, 0},
#endif /* CONFIG_QAPP_UNIRTOS_FIREWALL_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LWM2M_AT_FUNC
    {"+QLWCFG" , qstd_exec_lwm2m_qlwcfg_cmd, 0},
    {"+QLWCONFIG" , qstd_exec_lwm2m_qlwconfig_cmd, 0},
    {"+QLWREG" , qstd_exec_lwm2m_qlwreg_cmd, 0},
    {"+QLWUPDATE" , qstd_exec_lwm2m_qlwupdate_cmd, 0},
    {"+QLWDEREG" , qstd_exec_lwm2m_qlwdereg_cmd, 0},
    {"+QLWSTATUS" , qstd_exec_lwm2m_qlwstatus_cmd, 0},
    {"+QLWADDOBJ" , qstd_exec_lwm2m_qlwaddobj_cmd, 0},
    {"+QLWDELOBJ" , qstd_exec_lwm2m_qlwdelobj_cmd, 0},
    {"+QLWRDRSP" , qstd_exec_lwm2m_qlwrdrsp_cmd, 0},
    {"+QLWWRRSP" , qstd_exec_lwm2m_qlwwrrsp_cmd, 0},
    {"+QLWEXERSP" , qstd_exec_lwm2m_qlwexersp_cmd, 0},
    {"+QLWOBSRSP" , qstd_exec_lwm2m_qlwobsrsp_cmd, 0},
    {"+QLWNOTIFY" , qstd_exec_lwm2m_qlwnotify_cmd, 0},
    {"+QLWRD" , qstd_exec_lwm2m_qlwrd_cmd, 0},
    {"+QLWRESET" , qstd_exec_lwm2m_qlwreset_cmd, 0},
#endif /*CONFIG_QAPP_UNIRTOS_LWM2M_AT_FUNC*/

#ifdef CONFIG_QAPP_UNIRTOS_HY_ANDLINK_AT_FUNC
    {"+QADLCFG",        qstd_exec_hy_andlink_qadlcfg_cmd_func,          0},
    {"+QADLSTART",      qstd_exec_hy_andlink_qadlstart_cmd_func,        0},
    {"+QADLSTATUS",     qstd_exec_hy_andlink_qadlstatus_cmd_func,       0},
    {"+QADLREPORT",     qstd_exec_hy_andlink_qadlreport_cmd_func,       0},
    {"+QADLSTOP",       qstd_exec_hy_andlink_qadlstop_cmd_func,         0},
    {"+QADLBINDSTATE",  qstd_exec_hy_andlink_qadlbindstate_cmd_func,    0},
    // {"+QADLRESET",  qstd_exec_hy_andlink_qadlreset_cmd_func,    0},
#endif /* CONFIG_QAPP_UNIRTOS_HY_ANDLINK_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_COAP_AT_FUNC
    {"+QCOAPCFG",          qstd_exec_coap_qcoapcfg_cmd,          0},
    {"+QCOAPOPEN",         qstd_exec_coap_qcoapopen_cmd,         0},
    {"+QCOAPCLOSE",        qstd_exec_coap_qcoapclose_cmd,        0},
    {"+QCOAPOPTION",       qstd_exec_coap_qcoapoption_cmd,       0},
    {"+QCOAPHEAD",         qstd_exec_coap_qcoaphead_cmd,         0},
    {"+QCOAPSEND",         qstd_exec_coap_qcoapsend_cmd,         0},
#endif /* CONFIG_QAPP_UNIRTOS_COAP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_UNICOM_LWM2M_AT_FUNC
    {"+UNILWCFG",       qstd_exec_unicom_lwm2m_at_unilwcfg_cmd,     0 },
    {"+UNILWREG",       qstd_exec_unicom_lwm2m_at_unilwreg_cmd,     0 },
    {"+UNILWUPDATE",    qstd_exec_unicom_lwm2m_at_unilwupdate_cmd,  0 },
    {"+UNILWUNREG",     qstd_exec_unicom_lwm2m_at_unilwunreg_cmd,   0 },
    {"+UNILWSTATE",     qstd_exec_unicom_lwm2m_at_unilwstate_cmd,   0 },
    {"+UNILWSEND",      qstd_exec_unicom_lwm2m_at_unilwsend_cmd,    0 },
    {"+UNILWVER",       qstd_exec_unicom_lwm2m_at_unilwver_cmd,     0 },
    {"+UNIVERSION",     qstd_exec_unicom_lwm2m_at_universion_cmd,   0 },
#endif /*CONFIG_QAPP_UNIRTOS_UNICOM_LWM2M_AT_FUNC*/

#ifdef CONFIG_QAPP_UNIRTOS_ONENET_LWM2M_AT_FUN
    {"+MIPLCONFIG",         qstd_exec_lwm2m_miplconfig_cmd,        0 },
    {"+MIPLCREATE",         qstd_exec_lwm2m_miplcreate_cmd,        0 },
    {"+MIPLDELETE",         qstd_exec_lwm2m_mipldelete_cmd,        0 },
    {"+MIPLVER",            qstd_exec_lwm2m_miplver_cmd,           0 },
    {"+MIPLADDOBJ",         qstd_exec_lwm2m_mipladdobj_cmd,        0 },
    {"+MIPLDELOBJ",         qstd_exec_lwm2m_mipldelobj_cmd,        0 },
    {"+MIPLOPEN",           qstd_exec_lwm2m_miplopen_cmd,          0 },
    {"+MIPLCLOSE",          qstd_exec_lwm2m_miplclose_cmd,         0 },
    {"+MIPLDISCOVERRSP",    qstd_exec_lwm2m_mipldiscoverrsp_cmd,   0 },
    {"+MIPLOBSERVERSP",     qstd_exec_lwm2m_miplobserversp_cmd,    0 },
    {"+MIPLREADRSP",        qstd_exec_lwm2m_miplreadrsp_cmd,       0 },
    {"+MIPLWRITERSP",       qstd_exec_lwm2m_miplwritersp_cmd,      0 },
    {"+MIPLEXECUTERSP",     qstd_exec_lwm2m_miplexecutersp_cmd,    0 },
    {"+MIPLPARAMETERRSP",   qstd_exec_lwm2m_miplparameterrsp_cmd,  0 },
    {"+MIPLNOTIFY",         qstd_exec_lwm2m_miplnotify_cmd,        0 },
    {"+MIPLUPDATE",         qstd_exec_lwm2m_miplupdate_cmd,        0 },
    {"+MIPLRESET",          qstd_exec_lwm2m_miplreset_cmd,         0 },
    {"+MIPLRD",             qstd_exec_lwm2m_miplrd_cmd,            0 },
#endif /*CONFIG_QAPP_UNIRTOS_ONENET_LWM2M_AT_FUN*/

#ifdef CONFIG_QAPP_UNIRTOS_AEP_LWM2M_AT_FUN
    //{"+QCFG",             qstd_exec_aep_lwm2m_qcfg_cmd,            0 },
    {"+NCDP",             qstd_exec_aep_lwm2m_ncdp_cmd,            0 },
    {"+QLWSREGIND",       qstd_exec_aep_lwm2m_qlwsregind_cmd,      0 },
    {"+QLWULDATA",        qstd_exec_aep_lwm2m_qlwuldata_cmd,       0 },
    {"+QLWULDATAEX",      qstd_exec_aep_lwm2m_qlwuldataex_cmd,     0 },
    {"+QLWULDATASTATUS",  qstd_exec_aep_lwm2m_qlwuldatastatus_cmd, 0 },
    {"+QLWFOTAIND",       qstd_exec_aep_lwm2m_qlwfotaind_cmd,      0 },
    {"+QREGSWT",          qstd_exec_aep_lwm2m_qregswt_cmd,         0 },
    {"+NMGS",             qstd_exec_aep_lwm2m_nmgs_cmd,            0 },
    {"+NMGR",             qstd_exec_aep_lwm2m_nmgr_cmd,            0 },
    {"+NNMI",             qstd_exec_aep_lwm2m_nnmi_cmd,            0 },
    {"+NSMI",             qstd_exec_aep_lwm2m_nsmi_cmd,            0 },
    {"+NQMGR",            qstd_exec_aep_lwm2m_nqmgr_cmd,           0 },
    {"+NQMGS",            qstd_exec_aep_lwm2m_nqmgs_cmd,           0 },
    {"+QLWEVTIND",        qstd_exec_aep_lwm2m_qlwevtind_cmd,       0 },
    {"+NMSTATUS",         qstd_exec_aep_lwm2m_nmstatus_cmd,        0 },
    {"+QLWSERVERIP",      qstd_exec_aep_lwm2m_qlwserverip_cmd,     0 },
    {"+QLWAEPRESET",      qstd_exec_aep_lwm2m_qlwaepreset_cmd,     0 },
#endif /* CONFIG_QAPP_UNIRTOS_AEP_LWM2M_AT_FUN */

#ifdef CONFIG_QAPP_UNIRTOS_HTTP_AT_FUNC
    {"+QHTTPURL",       qstd_exec_http_qhttpurl_cmd_func,        0 },
    {"+QHTTPCFG",       qstd_exec_http_qhttpcfg_cmd_func,        0 },
    {"+QHTTPGET",       qstd_exec_http_qhttpget_cmd_func,        0 },
    {"+QHTTPGETEX",     qstd_exec_http_qhttpgetex_cmd_func,      0 },
    {"+QHTTPPOST",      qstd_exec_http_qhttppost_cmd_func,       0 },
    {"+QHTTPPOSTFILE",  qstd_exec_http_qhttpptfile_cmd_func,     0 },
    {"+QHTTPPUT",       qstd_exec_http_qhttpput_cmd_func,        0 },
    {"+QHTTPPUTFILE",   qstd_exec_http_qhttpptfile_cmd_func,     0 },
    {"+QHTTPPATCH",     qstd_exec_http_qhttppatch_cmd_func,      0 },
    {"+QHTTPPATCHFILE", qstd_exec_http_qhttpptfile_cmd_func,     0 },
    {"+QHTTPREAD",      qstd_exec_http_qhttpread_cmd_func,       0 },
    {"+QHTTPREADFILE",  qstd_exec_http_qhttpreadfile_cmd_func,   0 },
    {"+QHTTPSTOP",      qstd_exec_http_qhttpstop_cmd_func,       0 },
#endif /* CONFIG_QAPP_UNIRTOS_HTTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SMTP_AT_FUNC
    {"+QSMTPCFG",       qstd_exec_smtp_qsmtpcfg_cmd_func,        0 },
    {"+QSMTPDST",       qstd_exec_smtp_qsmtpdst_cmd_func,        0 },
    {"+QSMTPSUB",       qstd_exec_smtp_qsmtpsub_cmd_func,        0 },
    {"+QSMTPBODY",      qstd_exec_smtp_qsmtpbody_cmd_func,       0 },
    {"+QSMTPATT",       qstd_exec_smtp_qsmtpatt_cmd_func,        0 },
    {"+QSMTPCLR",       qstd_exec_smtp_qsmtpclr_cmd_func,        0 },
    {"+QSMTPPUT",       qstd_exec_smtp_qsmtpput_cmd_func,        0 },
#endif /* CONFIG_QAPP_UNIRTOS_SMTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_FTP_AT_FUNC
    {"+QFTPCFG",          qstd_exec_ftp_qftpcfg_cmd_func,          0},
    {"+QFTPOPEN",         qstd_exec_ftp_qftpopen_cmd_func,         0},
    {"+QFTPCLOSE",        qstd_exec_ftp_qftpclose_cmd_func,        0},
    {"+QFTPPUT",          qstd_exec_ftp_qftpput_cmd_func,          0},
    {"+QFTPPUTEX",        qstd_exec_ftp_qftpputex_cmd_func,        0},
    {"+QFTPGET",          qstd_exec_ftp_qftpget_cmd_func,          0},
    {"+QFTPSIZE",         qstd_exec_ftp_qftpsize_cmd_func,         0},
    {"+QFTPSTAT",         qstd_exec_ftp_qftpstat_cmd_func,         0},
    {"+QFTPLEN",          qstd_exec_ftp_qftplen_cmd_func,          0},
    {"+QFTPRENAME",       qstd_exec_ftp_qftprename_cmd_func,       0},
    {"+QFTPDEL",          qstd_exec_ftp_qftpdel_cmd_func,          0},
    {"+QFTPMKDIR",        qstd_exec_ftp_qftpmkdir_cmd_func,        0},
    {"+QFTPRMDIR",        qstd_exec_ftp_qftprmdir_cmd_func,        0},
    {"+QFTPLIST",         qstd_exec_ftp_qftplist_cmd_func,         0},
    {"+QFTPNLST",         qstd_exec_ftp_qftpnlst_cmd_func,         0},
    {"+QFTPCWD",          qstd_exec_ftp_qftpcwd_cmd_func,          0},
    {"+QFTPPWD",          qstd_exec_ftp_qftppwd_cmd_func,          0},
    {"+QFTPMDTM",         qstd_exec_ftp_qftpmdtm_cmd_func,         0},
    {"+QFTPMLSD",         qstd_exec_ftp_qftpmlsd_cmd_func,         0},
    {"+QFTPDBG",          qstd_exec_ftp_qftpdbg_cmd_func,          0},
#endif /* CONFIG_QAPP_UNIRTOS_FTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_GCOV_AT_FUNC
    {"+QGCOV",            qstd_exec_qgcov_cmd,                     0},
#endif /* CONFIG_QAPP_UNIRTOS_GCOV_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_MMS_AT_FUNC
    {"+QMMSCFG",          qstd_exec_mms_qmmscfg_cmd_func,          0},
    {"+QMMSEDIT",         qstd_exec_mms_qmmsedit_cmd_func,         0},
    {"+QMMSEND",          qstd_exec_mms_qmmsend_cmd_func,          0},
#endif /* CONFIG_QAPP_UNIRTOS_MMS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_UNIFY_AT_FUNC
    {"+UNIKEYINFO",       qstd_exec_mqtt_unikeyinfo_cmd_func,      0},
    {"+UNIKEYINFOM",      qstd_exec_mqtt_unikeyinfom_cmd_func,     0},
    {"+UNIDELKEYINFO",    qstd_exec_mqtt_unidelkeyinfo_cmd_func,   0},
    {"+UNIDELKEYINFOM",   qstd_exec_mqtt_unidelfkeyinfom_cmd_func, 0},
    {"+UNIMQTTCON",       qstd_exec_mqtt_unimqttconn_cmd_func,     0},
    {"+UNIMQTTDISCON",    qstd_exec_mqtt_unimqttdiscon_cmd_func,   0},
    {"+UNIMQTTSTATE",     qstd_exec_mqtt_unimqttstate_cmd_func,    0},
    {"+UNIMQTTSUB",       qstd_exec_mqtt_unimqttsub_cmd_func,      0},
    {"+UNIMQTTPUB",       qstd_exec_mqtt_unimqttpub_cmd_func,      0},
#endif  /* CONFIG_QAPP_UNIRTOS_UNIFY_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_MQTT_AT_FUNC
    {"+QMTCFG",           qstd_exec_mqtt_qmtcfg_cmd,          QOSA_AT_CONSTRAIN_CJSON},
    {"+QMTOPEN",          qstd_exec_mqtt_qmtopen_cmd,         0},
    {"+QMTCONN",          qstd_exec_mqtt_qmtconn_cmd,         0},
    {"+QMTSUB",           qstd_exec_mqtt_qmtsub_cmd,          0},
    {"+QMTUNS",           qstd_exec_mqtt_qmtunsub_cmd,        0},
    {"+QMTPUBEX",         qstd_exec_mqtt_qmtpub_cmd,          0},
    {"+QMTPUB",           qstd_exec_mqtt_qmtpub_cmd,          0},
    {"+QMTRECV",          qstd_exec_mqtt_qmtrecv_cmd,         0},
    {"+QMTDISC",          qstd_exec_mqtt_qmtdisc_cmd,         0},
    {"+QMTCLOSE",         qstd_exec_mqtt_qmtclose_cmd,        0},
#ifdef CONFIG_QCM_MQTT5_FUNC
    {"+QMTSVRINFO",       qstd_exec_mqtt_qmtsvrinfo_cmd,      0},
    {"+QMTSUBPROP",       qstd_exec_mqtt_qmtsubprop_cmd,      QOSA_AT_CONSTRAIN_CJSON},
    {"+QMTUNSPROP",       qstd_exec_mqtt_qmtunsprop_cmd,      QOSA_AT_CONSTRAIN_CJSON},
    {"+QMTPUBPROP",       qstd_exec_mqtt_qmtpubprop_cmd,      QOSA_AT_CONSTRAIN_CJSON},
    {"+QMTDISCPROP",      qstd_exec_mqtt_qmtdiscprop_cmd,     QOSA_AT_CONSTRAIN_CJSON},
#endif /* CONFIG_QCM_MQTT5_FUNC */
#endif  /* CONFIG_QAPP_UNIRTOS_MQTT_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SBFOTA_AT_FUNC
    {"+QSBFOTA",         qstd_exec_qsbfota_cmd,         0},
    {"+QSBFOTACFG",      qstd_exec_qsbfota_cfg_cmd,     0},
#endif  /* CONFIG_QAPP_UNIRTOS_SBFOTA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_WEBSOCKET_AT_FUNC
    {"+QWSOPEN",          qstd_exec_web_qwsopen_cmd_func,      0},
    {"+QWSCLOSE",         qstd_exec_web_qwsclose_cmd_func,     0},
    {"+QWSREAD",          qstd_exec_web_qwsread_cmd_func,      0},
    {"+QWSWRITE",         qstd_exec_web_qwswrite_cmd_func,     0},
    {"+QWSCFG",           qstd_exec_web_qwscfg_cmd_func,       0},
    {"+QWSTATE",          qstd_exec_web_qwstate_cmd_func,      0},

#endif  /* CONFIG_QAPP_UNIRTOS_WEBSOCKET_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_DMHTTP_AT_FUNC
    {"+QDMHTTPCFG",       qstd_exec_dmhttpcfg_cmd_func,        0},
    {"+QDMHTTPINFO",      qstd_exec_dmhttpinfo_cmd_func,       0},
    {"+QDMHTTPREG",       qstd_exec_dmhttpreg_cmd_func,        0},
#endif  /* CONFIG_QAPP_UNIRTOS_DMHTTP_AT_FUNC */

/* GET LAST CLOSE ERROR */
    {"+QLASTERROR",    qstd_exec_all_qerror_cmd,      0},
/* GET LAST CLOSE ERROR */

#ifdef CONFIG_QAPP_UNIRTOS_UART_AT_FUNC
#ifdef CONFIG_QAPP_UNIRTOS_AT_IPR_CMD
    {"+IPR",           qstd_exec_ipr_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_IPR_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_ICF_CMD
    {"+ICF",           qstd_exec_icf_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_ICF_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CBAUD_CMD
    {"+CBAUD",           qstd_exec_cbaud_cmd,          0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_CBAUD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_IFC_CMD
    {"+IFC",           qstd_exec_ifc_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_IFC_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NATSPEED_CMD
    {"+NATSPEED",      unir_exec_natspeed_cmd,         0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_NATSPEED_CMD */
#endif /* CONFIG_QAPP_UNIRTOS_UART_AT_FUNC */

/** On the EIGEN718, it is a dual qcfg solution, and the AT entry for qcfg is on the platform side */
#if defined(CONFIG_QAPP_UNIRTOS_AT_QCFG_CMD)
    {"+QCFG",          qstd_exec_qcfg_cmd,           0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCFG_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_ATO_CMD
    {"O",              qstd_exec_ato_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_ATO_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_FOTA_AT_FUNC
    {"+QFOTADL",       qstd_exec_qfotadl_cmd,          0 },
#endif /* CONFIG_QAPP_UNIRTOS_FOTA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_RETSET_CMD
    {"+RSTSET",       qstd_exec_rstset_cmd,            0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_RETSET_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CSUB_CMD
    {"+CSUB",       qstd_exec_csub_cmd,                0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_CSUB_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QINF_CMD
    {"+QINF",       qstd_exec_qinf_cmd,                0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QINF_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCLI_CMD
    {"+QCLI",       qstd_exec_qcli_cmd,                0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCLI_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QGMR_CMD
    {"+QGMR",       qstd_exec_qgmr_cmd,                0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QGMR_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QPROD_CMD
    {"+QPROD",       qstd_exec_qprod_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QPROD_CMD */
#ifdef CONFIG_QAPP_UNIRTOS_AT_PROD_CMD
    {"*PROD",       qstd_exec_prod_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_PROD_CMD */

#if !defined(CONFIG_QOSA_UIS8360S_PLATFORM_FUNC) && defined(CONFIG_QAPP_UNIRTOS_AT_QWSETMAC_CMD)
    {"+QWSETMAC",     qstd_exec_qwsetmac_cmd,          0 },
#endif /* !CONFIG_QOSA_UIS8360S_PLATFORM_FUNC && CONFIG_QAPP_UNIRTOS_AT_QWSETMAC_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_EGMR_CMD
    {"+EGMR",       qstd_exec_egmr_cmd,                0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_EGMR_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QSVN_CMD
    {"+QSVN",       qstd_exec_qsvn_cmd,                0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QSVN_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_HVERSION_CMD
    {"+HVersion",       qstd_exec_qcert_hversion_cmd,  0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_HVERSION_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_MODULE_CMD
    {"+Module",       qstd_exec_qcert_module_cmd,      0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_MODULE_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QPOWD_CMD
    {"+QPOWD",       qstd_exec_qpowd_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QPOWD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QBOOTREASON_CMD
    {"+QBOOTREASON", qstd_exec_qbootreason_cmd,        0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QBOOTREASON_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CBC_CMD
    {"+CBC",       qstd_exec_cbc_cmd,                  0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_CBC_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QTEMP_CMD
    {"+QTEMP",       qstd_exec_qtemp_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QTEMP_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCHIPINFO_CMD
    {"+QCHIPINFO",   unir_exec_qchipinfo_cmd,          0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCHIPINFO_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QADC_CMD
    {"+QADC",        qstd_exec_qadc_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QADC_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QRESET_CMD
    {"+QRESET",       qstd_exec_qreset_cmd,            0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QRESET_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDOWNLOAD_CMD
    {"+QDOWNLOAD",    qstd_exec_qdownload_cmd,         0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDOWNLOAD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CVERSION_CMD
    {"+CVERSION",     qstd_exec_cversion_cmd,          0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_CVERSION_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QVERINFO_CMD
    {"*QVERINFO",     qstd_exec_qverinfo_cmd,          0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QVERINFO_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_GPIO_CMD
    {"+QGPIOCFG",     qstd_exec_qgpiocfg_cmd,          0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_GPIO_CMD */
    {"+GMI",        qstd_exec_gmi_cmd,                 0 },
    {"+CGMI",       qstd_exec_cgmi_cmd,                0 },

    {"+GMM",        qstd_exec_gmm_cmd,                 0 },
    {"+CGMM",       qstd_exec_cgmm_cmd,                0 },

    {"+GMR",        qstd_exec_gmr_cmd,                 0 },
    {"+CGMR",       qstd_exec_cgmr_cmd,                0 },

    {"+CGSN",       qstd_exec_cgsn_cmd,                0 },
    {"+GSN",        qstd_exec_gsn_cmd,                 0 },
    {"+QGSN",       qstd_exec_qgsn_cmd,                0 },

#ifdef CONFIG_QAPP_UNIRTOS_LED_AT_FUNC
    {"+QLEDSTAT",   qstd_exec_qledstat_cmd,             0 },
    {"+QLEDCFG",    qstd_exec_qledcfg_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_LED_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LICENSE_AT_FUNC
    {"+QLICENSEINFO",           qosa_exec_qlicenseinfo_cmd,       0},
    {"+QLICENSE",               qosa_exec_qlicense_cmd,           0},
#endif /* CONFIG_QAPP_UNIRTOS_LICENSE_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_QINGYA_CODE_DECODER_AT_FUNC
    {"+QAUTHCODE",              qosa_exec_qauthcode_cmd,          0},
#endif /* CONFIG_QAPP_UNIRTOS_QINGYA_CODE_DECODER_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_LPM_FUNC
    {"+QSCLK",      qstd_exec_qsclk_cmd,               0 },
    {"+QSCLKEX",    qstd_exec_qsclkex_cmd,            0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_LPM_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_URC_FUNC
    {"+QURCCFG",    qstd_exec_qurccfg_cmd,             0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_URC_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QAPPVERSION_CMD
    {"+QAPPVERINFO", qstd_exec_qappverinfo_cmd,        0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QAPPVERSION_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD
    {"+QDBGCFG",     qstd_exec_qdbgcfg_cmd,            0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDIAGPORT_CMD
    {"+QDIAGPORT",     qstd_exec_qdiagport_cmd,          0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDIAGPORT_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QVSIM_FUNC
    {"+QVSIM",          qstd_exec_qvsim_cmd,           0 },
    {"+QVSIMW",         qstd_exec_qvsimw_cmd,          0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QVSIM_FUNC */

#if defined(CONFIG_QOSA_UIS8852_PLATFORM_FUNC) && !defined(CONFIG_QOSA_OPENCPU_FUNC)
    //8852 platform calibration related AT instructions
    {"+PLATCFG",     qstd_exec_platcfg_cmd,            0 },
    {"+PRODTEST",    qstd_exec_prodtest_cmd,           0 },
    {"+AMTPHY",      qstd_exec_amtphy_cmd,             0 },
    {"+CMEPCG",      qstd_exec_cmepcg_cmd,             0 },
    {"+FTM",         qstd_exec_ftm_cmd,                0 },
    {"+REBOOT",      qstd_exec_reboot_cmd,             0 },
    {"+AMTNV",       qstd_exec_amtnv_cmd,              0 },
    {"^FSLSTFILE",   qstd_exec_flistfile_cmd,          0 },
    {"^FSLSTPART",   qstd_exec_flistpart_cmd,          0 },
    {"+CALIBINFO",   qstd_exec_calibinfo_cmd,          0 },
#endif /* CONFIG_QOSA_UIS8852_PLATFORM_FUNC */

#if defined(CONFIG_QOSA_UIS8360S_PLATFORM_FUNC) && defined(CONFIG_QOSA_WIFI_FUNC)
    {"+QWIFIFTMTEST", qstd_exec_qwififtmtest_cmd,      0 },
    {"+QWIFITEST",    qstd_exec_qwifitest_cmd,         0 },
    {"+QWSETCALI",    qstd_exec_qwsetcali_cmd,         0 },
    {"+QWSETSSID",    qstd_exec_qwsetssid_uis_cmd,     0 },
    {"+QWSETMAC",     qstd_exec_qwsetmac_uis_cmd,      0 },
    {"+QNVW",         qstd_exec_qnvw_uis_cmd,          0 },
    {"+QLEDTEST",     qstd_exec_qledtest_cmd,          0 },
#endif /* CONFIG_QOSA_UIS8360S_PLATFORM_FUNC */

#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
    {"+NFLST",       qstd_exec_nflst_cmd,              0 },
    {"+NQSTATE",     qstd_exec_nqstate_cmd,            0 },
    {"+NCHIPTYPE",   qstd_exec_nchiptype_cmd,          0 },
    {"+NRDTEST",     qstd_exec_nrdtest_cmd,            0 },
    {"+NRDCTRL",     qstd_exec_nrdctl_cmd,             0 },
    {"+NRDSET",      qstd_exec_nrdset_cmd,             0 },
    {"+NRDEXEC",     qstd_exec_nrdexec_cmd,            0 },
    {"+NRCUPD",      qstd_exec_nrcupd_cmd,             0 },
    {"+NGOLD",       qstd_exec_ngold_cmd,              0 },
    {"+NLOGMODFT",   qstd_exec_nlogmodft_cmd,          0 },
    {"+NVETOSUPERDS",qstd_exec_nvetosuperds_cmd,       0 },
	{"+NUICC",       qstd_exec_nuicc_cmd,              0 },
    {"+NSIMAC",      qstd_exec_nsimac_cmd,             0 },
    {"+NFORCEBURN",  qstd_exec_nforceburn_cmd,         0 },
    {"+NBACKUP",    qstd_exec_nbackup_cmd,             0 },
    {"+NRECOVERY",  qstd_exec_nrecovery_cmd,           0 },
#endif

#ifdef CONFIG_QAPP_UNIRTOS_FTM_AT_FUNC
    {"+QRXFTM",      qstd_exec_qrxftm_cmd,           0 },
    {"+QRFTEST",     qstd_exec_qrftest_cmd,          0 },
    {"+QRFTESTMODE", qstd_exec_qrftestmode_cmd,      0 },
#endif /* CONFIG_QAPP_UNIRTOS_FTM_AT_FUNC */


#ifdef CONFIG_QAPP_UNIRTOS_AT_QAUDIO_CMD
    {"+QAUDPLAY",    unir_exec_qaudplay_cmd,           0 },
    {"+QAUDRD",      unir_exec_qaudrd_cmd,             0 },
    {"+QAUDSTOP",    unir_exec_qaudstop_cmd,           0 },
    {"+CLVL",        unir_exec_clvl_cmd,               0 },
    {"+QDAI",        unir_exec_qdai_cmd,               0 },
    {"+QAUDSW",      unir_exec_qaudsw_cmd,             0 },
    {"+QAUDLOOP",    unir_exec_qaudloop_cmd,           0 },
    {"+QIIC",        unir_exec_qiic_cmd,               0 },
#ifdef CONFIG_QOSA_VOICE_CALL_FUNC
    {"+QPSND",       unir_exec_qpsnd_cmd,              0 },
    {"+QTONEDET",    unir_exec_qtonedet_cmd,           0 },
#endif /* CONFIG_QOSA_VOICE_CALL_FUNC */
#ifdef CONFIG_QOSA_TTS_SUPPORT
    {"+QTTS",        unir_exec_qtts_cmd,               0 },
    {"+QTTSETUP",    unir_exec_qttsetup_cmd,           0 },
#endif /* CONFIG_QOSA_TTS_SUPPORT */
#endif /* CONFIG_QAPP_UNIRTOS_AT_QAUDIO_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QESIM_CMD
    {"+QESIM",       unir_exec_esim_cmd,               0 },
#elif defined(CONFIG_QAPP_UNIRTOS_AT_ESIM_EID_CMD)
    {"+QESIM",       qstd_exec_qesim_cmd,              0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QESIM_CMD */


#if defined(CONFIG_QOSA_EIGEN_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX_PLATFORM_FUNC)
    {"+QUPCHECK",    qstd_exec_qupcheck_cmd,           0 },
    {"+QUPDATER",    qstd_exec_qupdater_cmd,           0 },
#ifdef CONFIG_QAPP_UNIRTOS_AT_QGPIOV_CMD
    {"+QGPIOV",       qstd_exec_qgpiov_cmd,            0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QGPIOV_CMD */
#ifdef CONFIG_QAPP_UNIRTOS_AT_QCFEM_CMD
    {"+QCFEM",        qstd_exec_qcfem_cmd,             0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCFEM_CMD */
#endif /* CONFIG_QOSA_EIGEN_PLATFORM_FUNC || CONFIG_QOSA_QCX_PLATFORM_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_GNSS_AT_FUNC
    {"+QGPSCFG" , qstd_exec_qgpscfg_cmd_func, 0},
    {"+QGPSDEL" , qstd_exec_qgpsdel_cmd_func, 0},
    {"+QGPS" , qstd_exec_qgps_cmd_func, 0},
    {"+QGPSEND" , qstd_exec_qgpsend_cmd_func, 0},
    {"+QGPSLOC" , qstd_exec_qgpsloc_cmd_func, 0},
    {"+QGPSCMDSEND" , qstd_exec_qgpscmdsend_cmd_func, 0},
    {"+QAGPS" , qstd_exec_qagps_cmd_func, 0},
    {"+QAGPSCFG" , qstd_exec_qagpscfg_cmd_func, 0},
    {"+QGPSGNMEA" , qstd_exec_qgpsgnmea_cmd_func, 0},
    {"+QGPSVBCKP", qstd_exec_qgpsvbckp_cmd_func, 0},
    {"+QGPSINFO" , qstd_exec_qgpsinfo_cmd_func, 0},
    {"+QGPSDEBUG" , qstd_exec_qgpsdebug_cmd_func, 0},
#endif /* CONFIG_QAPP_UNIRTOS_GNSS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CIND_CMD
    {"+CIND",    unir_exec_cind_cmd,           0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_CIND_CMD */
#ifdef CONFIG_QAPP_UNIRTOS_SECBOOT_AT_FUNC
    {"+QSECCFG",    qstd_exec_qseccfg_cmd_func,           0 },
#endif /* CONFIG_QAPP_UNIRTOS_SECBOOT_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QREDDAAVN_CMD
    {"+QREDDAVN",    qstd_exec_qreddavn_cmd,           0 },
#endif /* CONFIG_QAPP_UNIRTOS_AT_QREDDAAVN_CMD */
#ifdef CONFIG_QAPP_OEM_LT_AT_FUNC
    {"+QNWSCAN",    unir_exec_qnwscan_cmd,             0 },
    {"+QAVGRSSI",   unir_exec_qavgrssi_cmd,            0 },
    {"+QLTCFG",      qstd_exec_qltcfg_cmd,             0 },
#ifdef CONFIG_QOSA_OEM_SE_FUNC
    {"+QLTACT",      qstd_exec_qltact_cmd,             0 },
#endif
#endif /* CONFIG_QAPP_OEM_LT_AT_FUNC */
#ifdef CONFIG_QAPP_UNIRTOS_AUTOAPN_AT_FUNC
    {"+QAUTOAPN",    qstd_exec_qautoapn_cmd,             0 },
#endif/*CONFIG_QAPP_UNIRTOS_AUTOAPN_AT_FUNC*/
#ifdef CONFIG_QAPP_UNIRTOS_AT_CALL_CONTROL_CMD
    {"D",              unir_exec_atd_cmd,          0},
    {"H",              unir_exec_ath_cmd,          0},
    {"A",              unir_exec_ata_cmd,          0},
#endif /* CONFIG_QAPP_UNIRTOS_AT_CALL_CONTROL_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_CMUX_AT_FUNC
    {"+CMUX",          qstd_exec_cmux_cmd,         0},
#endif
#ifdef CONFIG_QAPP_UNIRTOS_STD_NEOWAY_FUNC
#include "unirtos_atcmd_nw_def.h"
#endif /* CONFIG_QAPP_UNIRTOS_STD_NEOWAY_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCPULOAD_CMD
    {"+QCPULOAD",      qstd_exec_qcpuload_cmd,     0},
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCPULOAD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_SPINET_AT_FUNC
    {"+QSPINET",       qstd_exec_qspinet_cmd,      0},
    {"+QSPINETGPIO",    qstd_exec_qspinetgpio_cmd,   0},
#endif /* CONFIG_QAPP_UNIRTOS_SPINET_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_STD_DL_FUNC
#include "unirtos_atcmd_gw_def.h"
#endif /* CONFIG_QAPP_UNIRTOS_STD_DL_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AGORA_RTC_AT_FUNC
    {"+QARTCCFG",          qstd_exec_qartccfg_cmd,            0},
    {"+QARTCOPEN",         qstd_exec_qartcopen_cmd,           0},
    {"+QARTCJOIN",         qstd_exec_qartcjoin_cmd,           0},
    {"+QARTCLEAVE",        qstd_exec_qartcleave_cmd,          0},
    {"+QARTCCLOSE",        qstd_exec_qartcclose_cmd,          0},
#endif /* CONFIG_QAPP_UNIRTOS_AGORA_RTC_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_EG21_AT_FUNC
    /* EG21 compatibility commands. The fragment below lists the entries; a new
     * command only touches its domain .c/.h and the matching section of
     * apps/unirtos_std_eg21/inc/unirtos_atcmd_eg21_def.h. */
#include "unirtos_atcmd_eg21_def.h"
#endif /* CONFIG_QAPP_UNIRTOS_EG21_AT_FUNC */

    {QOSA_NULL, QOSA_NULL, 0}
};

// clang-format on

#endif /* __UNIRTOS_ATCMD_DEF_H__ */
