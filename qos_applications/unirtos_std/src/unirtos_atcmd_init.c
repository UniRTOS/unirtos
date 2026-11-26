/*****************************************************************/ /**
* @file unirtos_atcmd_init.c
* @brief
* @author larson.li@quectel.com
* @date 2024-04-05
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-04-05 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#include "qcm_proj_config.h"
#include "unirtos_atcmd_cfg.h"
#include "qosa_def.h"
#include "qosa_sys.h"
#include "unirtos_atcmd_def.h"
#include "qosa_at_cmd.h"
#include "qosa_log.h"
#include "qosa_sys_registers.h"
#include "qosa_uart.h"

/** Redefine the TAG of this file */
#define QOS_LOG_TAG LOG_TAG

#ifdef CONFIG_QAPP_UNIRTOS_FILE_AT_FUNC
extern void unir_file_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_FILE_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_CAMERA_AT_FUNC
extern void unirtos_camera_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_CAMERA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SIM_AT_FUNC
extern void unir_sim_at_init(void);
#ifndef CONFIG_QOSA_SMS_FUNC
extern void unirtos_sms_urc_init(void);
#endif /* !CONFIG_QOSA_SMS_FUNC */
#endif /* CONFIG_QAPP_UNIRTOS_SIM_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QVSIM_FUNC
extern void unirtos_qvsim_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QVSIM_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SMS_AT_FUNC
extern void unirtos_sms_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_SMS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_NW_AT_FUNC
extern void unir_nw_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_NW_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_USBNET_AT_FUNC
extern void unirtos_usbnet_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_USBNET_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_CMUX_AT_FUNC
extern void unirtos_cmux_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_CMUX_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_TCPIP_AT_FUNC
extern void unir_tcpip_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_TCPIP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_CLAT_AT_FUNC
extern void unirtos_clat_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_CLAT_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC
extern void unir_tcpip_nb_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_PING_AT_FUNC
extern void unir_ping_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_PING_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_NTP_AT_FUNC
extern void unir_ntp_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_NTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LWM2M_AT_FUNC
extern void unir_lwm2m_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_LWM2M_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_ONENET_LWM2M_AT_FUN
extern void unir_onenet_miplwm2m_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_ONENET_LWM2M_AT_FUN */

#ifdef CONFIG_QAPP_UNIRTOS_AEP_LWM2M_AT_FUN
extern void unir_aepiot_lwm2m_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AEP_LWM2M_AT_FUN */

#ifdef CONFIG_QAPP_UNIRTOS_SMTP_AT_FUNC
extern void qstd_smtp_app_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_SMTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_MMS_AT_FUNC
extern void unir_mms_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_MMS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_UNIFY_IDENTITY_FUNC
extern void unir_uniot_identity_server_start(void);
#endif

#ifdef CONFIG_QAPP_UNIRTOS_PDP_AT_FUNC
extern void qstd_pdp_app_init(void);
#endif

#ifdef CONFIG_QAPP_UNIRTOS_QINDCFG_AT_FUNC
extern void qosa_qindcfg_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_QINDCFG_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LBS_AT_FUNC
extern void unir_lbs_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_LBS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_FIREWALL_AT_FUNC
extern void unir_firewall_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_FIREWALL_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_MODEM_AT_BYPASS_FUNC
extern void qstd_modem_at_bypass_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_MODEM_AT_BYPASS_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SBFOTA_AT_FUNC
extern void unir_sbfota_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_SBFOTA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_LPM_FUNC
extern void unir_lpm_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_LPM_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD
extern void unir_qdbgcfg_atcmd_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDIAGPORT_CMD
extern void unir_qdiagport_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDIAGPORT_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCLI_CMD
extern void qstd_qcli_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCLI_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_FTM_AT_FUNC
extern void qstd_exec_ftm_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_FTM_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_DMHTTP_AT_FUNC
extern void unir_dmhttp_at_auto_reg(void);
#endif /* CONFIG_QAPP_UNIRTOS_DMHTTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_FLASH_TEST_CMD
extern void unir_flash_test_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_FLASH_TEST_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCPULOAD_CMD
extern void unir_cpuload_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCPULOAD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QESIM_CMD
extern void ql_esim_service_start(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QESIM_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_JD_AT_FUNC
extern void qstd_jamm_detect_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_JD_AT_FUNC */

extern void qstd_exec_qcfg_cmd(qosa_at_cmd_t *cmd);
extern void qstd_qcfg_register_init(void);

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD
extern void qstd_qdbgcfg_register_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CIND_CMD
extern void unir_cind_server_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_CIND_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_GNSS_AT_FUNC
extern void qstd_gnss_at_autogps(void);
#endif /* CONFIG_QAPP_UNIRTOS_GNSS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LED_AT_FUNC
extern void unir_led_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_LED_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_FOTA_AT_FUNC
extern void unir_fota_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_FOTA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_URC_FUNC
extern void unir_urc_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_FOTA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_DATACALL_AT_FUNC
extern void unir_datacall_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_DATACALL_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_IMS_AT_FUNC
extern void unir_ims_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_IMS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_MIFI_APP_FUNC
extern void unir_mifi_app_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_MIFI_APP_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AGORA_RTC_AT_FUNC
extern void qstd_exec_agora_rtc_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AGORA_RTC_AT_FUNC */

#if (defined(CONFIG_QOSA_EIGEN_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX_PLATFORM_FUNC)) && defined(CONFIG_QAPP_UNIRTOS_AT_QCFEM_CMD)
extern void qstd_qcfem_boot_at_init(void);
#endif /* (CONFIG_QOSA_EIGEN_PLATFORM_FUNC || CONFIG_QOSA_QCX_PLATFORM_FUNC) && CONFIG_QAPP_UNIRTOS_AT_QCFEM_CMD */

/**
 * @brief UniRTOS AT Command Initialization Interface
 */
void unir_atcmd_init(void)
{
#ifdef CONFIG_QAPP_UNIRTOS_MODEM_AT_BYPASS_FUNC
    qstd_modem_at_bypass_init();
#endif /* CONFIG_QAPP_UNIRTOS_MODEM_AT_BYPASS_FUNC */

    // Load and register ATCMD table
    qosa_at_parser_add_cust_at((const qosa_at_desc_t *)unir_img_at_desc, QOSA_ARRAY_SIZE(unir_img_at_desc));

    // Register qcfg
    qosa_regedit_at_qcfg_func(qstd_exec_qcfg_cmd);
    qstd_qcfg_register_init();

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD
    qstd_qdbgcfg_register_init();
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD */

#ifdef CONFIG_QOSA_EIGEN718_PLATFORM_FUNC
    // Operations specific to the EC platform
    qosa_dev_ioctl_type ioctl_type = {0};

    ioctl_type.ccio_mode = QOSA_UART_MODE_AT;
    qosa_at_dev_port_ioctl(QOSA_DEV_SIOLIB_UART1_PORT, QOSA_DEV_IOCTL_SET_CCIO_MODE, &ioctl_type);
#endif /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC*/

#ifdef CONFIG_QAPP_UNIRTOS_FILE_AT_FUNC
    unir_file_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_FILE_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_CAMERA_AT_FUNC
    unirtos_camera_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_CAMERA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SIM_AT_FUNC
    unir_sim_at_init();
#ifndef CONFIG_QOSA_SMS_FUNC
    unirtos_sms_urc_init();
#endif /* !CONFIG_QOSA_SMS_FUNC */
#endif /* CONFIG_QAPP_UNIRTOS_SIM_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QVSIM_FUNC
    unirtos_qvsim_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_AT_QVSIM_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SMS_AT_FUNC
    unirtos_sms_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_SMS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_NW_AT_FUNC
    unir_nw_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_NW_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_USBNET_AT_FUNC
    unirtos_usbnet_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_USBNET_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_CMUX_AT_FUNC
    unirtos_cmux_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_CMUX_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_TCPIP_AT_FUNC
    unir_tcpip_at_init();  //AT APP should be initialized after TCPIP_APP
#endif                     /* CONFIG_QAPP_UNIRTOS_TCPIP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_CLAT_AT_FUNC
    unirtos_clat_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_CLAT_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC
    unir_tcpip_nb_at_init();  //AT APP should be initialized after TCPIP_APP
#endif                        /* CONFIG_QAPP_UNIRTOS_TCPIP_NB_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_PING_AT_FUNC
    unir_ping_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_PING_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SMTP_AT_FUNC
    qstd_smtp_app_init();
#endif /* CONFIG_QAPP_UNIRTOS_SMTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LWM2M_AT_FUNC
    unir_lwm2m_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_LWM2M_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_MMS_AT_FUNC
    unir_mms_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_MMS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_ONENET_LWM2M_AT_FUN
    unir_onenet_miplwm2m_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_ONENET_LWM2M_AT_FUN */

#ifdef CONFIG_QAPP_UNIRTOS_AEP_LWM2M_AT_FUN
    unir_aepiot_lwm2m_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_AEP_LWM2M_AT_FUN */

#ifdef CONFIG_QAPP_UNIRTOS_NTP_AT_FUNC
    unir_ntp_init();
#endif /* CONFIG_QAPP_UNIRTOS_NTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_UNIFY_IDENTITY_FUNC
    unir_uniot_identity_server_start();
#endif /* CONFIG_QAPP_UNIRTOS_UNIFY_IDENTITY_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_PDP_AT_FUNC
    qstd_pdp_app_init();
#endif /* CONFIG_QAPP_UNIRTOS_PDP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_QINDCFG_AT_FUNC
    qosa_qindcfg_init();
#endif /* CONFIG_QAPP_UNIRTOS_QINDCFG_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LBS_AT_FUNC
    unir_lbs_init();
#endif /* CONFIG_QAPP_UNIRTOS_LBS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_SBFOTA_AT_FUNC
    unir_sbfota_init();
#endif /* CONFIG_QAPP_UNIRTOS_SBFOTA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_LPM_FUNC
    unir_lpm_init();
#endif /* CONFIG_QAPP_UNIRTOS_AT_LPM_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD
    unir_qdbgcfg_atcmd_init();
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDBGCFG_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDIAGPORT_CMD
    unir_qdiagport_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDIAGPORT_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCLI_CMD
    qstd_qcli_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCLI_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_FTM_AT_FUNC
    qstd_exec_ftm_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_FTM_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_DMHTTP_AT_FUNC
    unir_dmhttp_at_auto_reg();
#endif /* CONFIG_QAPP_UNIRTOS_DMHTTP_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_FLASH_TEST_CMD
    unir_flash_test_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_AT_FLASH_TEST_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCPULOAD_CMD
    unir_cpuload_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCPULOAD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QESIM_CMD
    ql_esim_service_start();
#endif /* CONFIG_QAPP_UNIRTOS_AT_QESIM_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_JD_AT_FUNC
    qstd_jamm_detect_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_JD_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CIND_CMD
    unir_cind_server_init();
#endif /* CONFIG_QAPP_UNIRTOS_AT_CIND_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_GNSS_AT_FUNC
    qstd_gnss_at_autogps();
#endif /* CONFIG_QAPP_UNIRTOS_GNSS_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_LED_AT_FUNC
    unir_led_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_LED_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_FOTA_AT_FUNC
    unir_fota_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_FOTA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AT_URC_FUNC
    unir_urc_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_FOTA_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_DATACALL_AT_FUNC
    unir_datacall_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_DATACALL_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_IMS_AT_FUNC
    unir_ims_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_IMS_AT_FUNC */

#ifdef CONFIG_QOSA_OEM_SE_FUNC
    extern void unir_lt_tcp_nv_restore(void);
    unir_lt_tcp_nv_restore();
#endif /* CONFIG_QOSA_OEM_SE_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_MIFI_APP_FUNC
    unir_mifi_app_init();
#endif /* CONFIG_QAPP_UNIRTOS_MIFI_APP_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_FIREWALL_AT_FUNC
    unir_firewall_at_init();
#endif /* CONFIG_QAPP_UNIRTOS_FIREWALL_AT_FUNC */

#ifdef CONFIG_QAPP_UNIRTOS_AGORA_RTC_AT_FUNC
    qstd_exec_agora_rtc_init();
#endif /* CONFIG_QAPP_UNIRTOS_AGORA_RTC_AT_FUNC */

#if (defined(CONFIG_QOSA_EIGEN_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX_PLATFORM_FUNC)) && defined(CONFIG_QAPP_UNIRTOS_AT_QCFEM_CMD)
    qstd_qcfem_boot_at_init();
#endif /* (CONFIG_QOSA_EIGEN_PLATFORM_FUNC || CONFIG_QOSA_QCX_PLATFORM_FUNC) && CONFIG_QAPP_UNIRTOS_AT_QCFEM_CMD */

}

/**
 * @brief Query all registered AT commands.
 *
 * This function supports two operation modes:
 * - Execute mode (AT+QATLIST): Returns the list of all active AT commands from the customer command table.
 * - Test mode (AT+QATLIST=?): Returns OK directly as no parameter configuration is required.
 * - Other modes: Returns operation not supported error.
 *
 * @param  cmd  Pointer to the qosa_at_cmd_t structure, containing AT command details.
 */
void qstd_exec_qatlist_cmd(qosa_at_cmd_t *cmd)
{
    if (cmd->type == QOSA_AT_CMD_EXE)
    {
        // Get customer command table
        const qosa_at_desc_t *cust_table = (const qosa_at_desc_t *)unir_img_at_desc;
        qosa_uint32_t         cust_len = QOSA_ARRAY_SIZE(unir_img_at_desc);
        char                 *res_buf = QOSA_NULL;
        int                   total_len = 0;
        int                   pos = 0;
        qosa_uint32_t         i = 0;

        if (cust_table != QOSA_NULL && cust_len > 0)
        {
            for (i = 0; i < cust_len; i++)
            {
                if (cust_table[i].name != QOSA_NULL)
                {
                    total_len += qosa_strlen(cust_table[i].name) + 2;
                }
            }
        }
        // Reserve space for "OK\r\n"
        total_len += 4;

        res_buf = qosa_calloc(1, total_len);
        if (res_buf == QOSA_NULL)
        {
            QOSA_RETURN_ERROR(cmd->dev_port);
        }

        // Traverse the write instruction list
        if (cust_table != QOSA_NULL && cust_len > 0)
        {
            for (i = 0; i < cust_len; i++)
            {
                if (cust_table[i].name != QOSA_NULL)
                {
                    pos += qosa_snprintf(res_buf + pos, total_len - pos, "%s\r\n", cust_table[i].name);
                }
            }
        }

        qosa_at_resp_cmd(cmd->dev_port, QOSA_ATCI_RESULT_CODE_OK, QOSA_CMD_RC_OK, res_buf, 1);
        qosa_free(res_buf);
    }
    else if (cmd->type == QOSA_AT_CMD_TEST)
    {
        // Return OK directly
        qosa_at_resp_cmd(cmd->dev_port, QOSA_ATCI_RESULT_CODE_OK, QOSA_CMD_RC_OK, QOSA_NULL, 0);
    }
    else
    {
        // Other modes are not supported
        QOSA_RETURN_CME_ERR(cmd->dev_port, QOSA_ERR_AT_CME_OPERATION_NOT_SUPPORTED);
    }
}
