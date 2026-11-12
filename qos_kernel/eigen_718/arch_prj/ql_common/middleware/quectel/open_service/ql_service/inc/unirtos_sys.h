/**  @file
  func.h

  @brief Automatically generated function declarations header file.

*/

/*================================================================
  Copyright (c) 2022 Quectel Wireless Solution, Co., Ltd.  All Rights Reserved.
  Quectel Wireless Solution Proprietary and Confidential.
=================================================================*/
/*=================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN              WHO         WHAT, WHERE, WHY
------------     -------     -------------------------------------------------------------------------------

=================================================================*/

#ifndef _UNIRTOS_SYS_H_
#define _UNIRTOS_SYS_H_

#include "qosa_at_sio.h"
#include "qosa_center_task.h"
#include "qosa_at_cmd.h"
#include "qosa_at_config.h"
#include "qosa_at_param.h"
#include "qosa_dev.h"
#include "qosa_event_notify.h"
#include "qosa_fota.h"
#include "qosa_power.h"
#include "qosa_rtc.h"
#include "qosa_adc.h"
#include "qosa_gpio.h"
#include "qosa_iic.h"
#include "qosa_spi.h"
#include "qosa_uart.h"
#include "modem_sender_recycle.h"
#include "qosa_datacall.h"
#include "qosa_dev1.h"
#include "qosa_ip_addr.h"
#include "qosa_modem.h"
#include "qosa_network.h"
#include "qosa_modem_feature_diff.h"
#include "qosa_network_param.h"
#include "qosa_sim.h"
#include "qosa_sms.h"
#include "qosa_ftm.h"
#include "unirtos_ppp_app.h"
#include "qosa_usbnet.h"
#include "qosa_nvitem.h"
#include "qosa_def.h"
#include "qosa_errno.h"
#include "qosa_sys.h"
#include "qosa_cJSON.h"
#include "qosa_queue_list.h"
#include "qosa_buffer_block.h"
#include "qosa_system_utils.h"
#include "qosa_watermark.h"
#include "qosa_asyn_dns.h"
#include "qosa_sockets.h"
#include "qosa_virtual_file.h"
#include "qosa_virtual_file_ops.h"
#include "qosa_wifiscan.h"
#include "qosa_pwm.h"
#include "qosa_lpm.h"
#include "qosa_sys_registers.h"
#include "qosa_camera.h"
#include "qosa_lcd.h"
#include "qosa_usb.h"
#include "qosa_led_rule.h"
#include "qosa_built_in_flash.h"
#include "qosa_log.h"
#include "qosa_audio.h"
#include "cmsis_os2.h"
#include "qosa_tts.h"
#include "qosa_vsim_adapt.h"
#include "qosa_dev_eigen.h"
#include "qosa_license.h"
#include "qosa_can_eigen.h"
#include "qosa_cam_decoder.h"
#include "qosa_secboot.h"
#include "qosa_clat.h"
#include "qosa_pinctrl.h"
#include "qosa_vpu_code.h"
#include "qosa_hw_timer.h"
#include "qosa_ims.h"
#ifdef __cplusplus
extern "C" {
#endif


extern qosa_ftm_err_e qosa_ftm_ant_tuner_fem_gpio_get_cfg(qosa_ftm_ant_tuner_cfg_t *pCfg);
typedef qosa_ftm_err_e (* _api_qosa_ftm_ant_tuner_fem_gpio_get_cfg_t)(qosa_ftm_ant_tuner_cfg_t *pCfg);

extern qosa_ftm_err_e qosa_ftm_ant_tuner_fem_gpio_set_cfg(qosa_ftm_ant_tuner_cfg_t *pCfg);
typedef qosa_ftm_err_e (* _api_qosa_ftm_ant_tuner_fem_gpio_set_cfg_t)(qosa_ftm_ant_tuner_cfg_t *pCfg);

extern qosa_ftm_err_e qosa_ftm_ant_tuner_fem_gpio_set_group(qosa_uint8_t groupIdx, qosa_ftm_ant_tuner_one_group_cfg_t *pGroupCfg);
typedef qosa_ftm_err_e (* _api_qosa_ftm_ant_tuner_fem_gpio_set_group_t)(qosa_uint8_t groupIdx, qosa_ftm_ant_tuner_one_group_cfg_t *pGroupCfg);

extern qosa_ftm_err_e qosa_ftm_ant_tuner_fem_gpio_reset_cfg(void);
typedef qosa_ftm_err_e (* _api_qosa_ftm_ant_tuner_fem_gpio_reset_cfg_t)(void);

extern qosa_int32_t qosa_tim_ic_init(qosa_tim_ic_handle_t *htim);
typedef qosa_int32_t (* _api_qosa_tim_ic_init_t)(qosa_tim_ic_handle_t *htim);

extern qosa_int32_t qosa_tim_ic_deinit(qosa_tim_ic_handle_t *htim);
typedef qosa_int32_t (* _api_qosa_tim_ic_deinit_t)(qosa_tim_ic_handle_t *htim);

extern qosa_int32_t qosa_tim_ic_config_channel(qosa_tim_ic_handle_t *htim,
                                               const qosa_tim_ic_config_t *ch_config);
typedef qosa_int32_t (* _api_qosa_tim_ic_config_channel_t)(qosa_tim_ic_handle_t *htim,
                                                           const qosa_tim_ic_config_t *ch_config);

extern qosa_int32_t qosa_tim_ic_start_it(qosa_tim_ic_handle_t *htim);
typedef qosa_int32_t (* _api_qosa_tim_ic_start_it_t)(qosa_tim_ic_handle_t *htim);

extern qosa_int32_t qosa_tim_ic_stop_it(qosa_tim_ic_handle_t *htim);
typedef qosa_int32_t (* _api_qosa_tim_ic_stop_it_t)(qosa_tim_ic_handle_t *htim);

extern qosa_int32_t qosa_tim_ic_get_captured_value(qosa_tim_ic_handle_t *htim);
typedef qosa_int32_t (* _api_qosa_tim_ic_get_captured_value_t)(qosa_tim_ic_handle_t *htim);

extern qosa_int32_t qosa_tim_ic_get_counter(qosa_tim_ic_handle_t *htim);
typedef qosa_int32_t (* _api_qosa_tim_ic_get_counter_t)(qosa_tim_ic_handle_t *htim);

extern qosa_gpio_error_e qosa_pin_config_out_clk(qosa_uint32_t pin_num, qosa_uint32_t func_sel, qosa_uint32_t freq, qosa_FracDivRootClk_e clkSrc);
typedef qosa_gpio_error_e (*_api_qosa_pin_config_out_clk_t)(qosa_uint32_t pin_num, qosa_uint32_t func_sel, qosa_uint32_t freq, qosa_FracDivRootClk_e clkSrc);

extern qosa_secboot_error_e qosa_secboot_get_enable_state(qosa_uint8_t *enable_state);
typedef qosa_secboot_error_e (*_api_qosa_secboot_get_enable_state_t)(qosa_uint8_t *enable_state);

extern qosa_secboot_error_e qosa_secboot_enable(void);
typedef qosa_secboot_error_e (*_api_qosa_secboot_enable_t)(void);

extern qosa_secboot_error_e qosa_secboot_ioctl(qosa_secboot_ioctl_cmd_e cmd, void *arg);
typedef qosa_secboot_error_e (*_api_qosa_secboot_ioctl_t)(qosa_secboot_ioctl_cmd_e cmd, void *arg);

extern void qosa_redda_set_dm_log_ctl(qosa_uint8_t onoff);
typedef void (*_api_qosa_redda_set_dm_log_ctl_t)(qosa_uint8_t onoff);

extern void qosa_redda_set_dm_dump_ctl(qosa_uint8_t onoff);
typedef void (*_api_qosa_redda_set_dm_dump_ctl_t)(qosa_uint8_t onoff);

extern qosa_uint32_t qosa_gnss_img_addr(void);
typedef qosa_uint32_t (*_api_qosa_gnss_img_addr_t)(void);

extern void qosa_32k_clock_enable(qosa_bool_t enable);
typedef void (*_api_qosa_32k_clock_enable_t)(qosa_bool_t enable);

// int qosa_license_digest(unsigned char *plaintest, int plaintext_len, unsigned char *digest_txt, int *digest_len);
extern int qosa_license_digest(unsigned char *plaintest, int plaintext_len, unsigned char *digest_txt, int *digest_len);
typedef int (*_api_qosa_license_digest_t)(unsigned char *plaintest, int plaintext_len, unsigned char *digest_txt, int *digest_len);

// qosa_license_error_e qosa_license_get_apppid(qosa_license_app_type_t app_type, int *appid);
extern qosa_license_error_e qosa_license_get_apppid(qosa_license_app_type_t app_type, int *appid);
typedef qosa_license_error_e (*_api_qosa_license_get_apppid_t)(qosa_license_app_type_t app_type, int *appid);

// int qosa_license_get_dev_info(unsigned char *dev_info, int *dev_info_len);
extern int qosa_license_get_dev_info(unsigned char *dev_info, int *dev_info_len);
typedef int (*_api_qosa_license_get_dev_info_t)(unsigned char *dev_info, int *dev_info_len);

extern qosa_license_error_e qosa_license_set_auth_info(unsigned char *auth_info, int auth_info_len);
typedef qosa_license_error_e (*_api_qosa_license_set_auth_info_t)(unsigned char *auth_info, int auth_info_len);

extern qosa_license_error_e qosa_license_get_auth_info(unsigned char *auth_info, int *auth_info_len);
typedef qosa_license_error_e (*_api_qosa_license_get_auth_info_t)(unsigned char *auth_info, int *auth_info_len);

extern int qosa_qingya_get_qrlience(char *qrcode_info);
typedef int (*_api_qosa_qingya_get_qrlience_t)(char *qrcode_info);

extern int qosa_qingya_get_imei(char *qrcode_info);
typedef int (*_api_qosa_qingya_get_imei_t)(char *qrcode_info);

//qosa_license_app_type_t qosa_license_appid_to_apptype(int appid);
extern qosa_license_app_type_t qosa_license_appid_to_apptype(int appid);
typedef qosa_license_app_type_t (*_api_qosa_license_appid_to_apptype_t)(int appid);

//qosa_license_error_e qosa_license_save_license_info(qosa_license_app_type_t app_type, unsigned char *data, int data_len);
extern qosa_license_error_e qosa_license_save_license_info(qosa_license_app_type_t app_type, unsigned char *data, int data_len);
typedef qosa_license_error_e (*_api_qosa_license_save_license_info_t)(qosa_license_app_type_t app_type, unsigned char *data, int data_len);

//qosa_license_error_e qosa_license_get_active_result(qosa_license_app_type_t app_type);
extern qosa_license_error_e qosa_license_get_active_result(qosa_license_app_type_t app_type);
typedef qosa_license_error_e (*_api_qosa_license_get_active_result_t)(qosa_license_app_type_t app_type);

//qosa_license_error_e qosa_license_register_app(qosa_license_app_type_t app_type, qosa_license_app_info_t *app_info);
extern qosa_license_error_e qosa_license_register_app(qosa_license_app_type_t app_type, qosa_license_app_info_t *app_info);
typedef qosa_license_error_e (*_api_qosa_license_register_app_t)(qosa_license_app_type_t app_type, qosa_license_app_info_t *app_info);

extern qosa_bool_t qosa_uart_check_support_baudrate(qosa_uart_port_number_e port, qosa_uint32_t baudrate);
typedef qosa_bool_t (*_api_qosa_uart_check_support_baudrate_t)(qosa_uart_port_number_e port, qosa_uint32_t baudrate);

extern qosa_bool_t qosa_dev_get_prod_mode(void);
typedef qosa_bool_t (*_api_qosa_dev_get_prod_mode_t)(void);

extern qosa_dev_error_e qosa_dev_set_prod_mode(qosa_bool_t prod_mode);
typedef qosa_dev_error_e (*_api_qosa_dev_set_prod_mode_t)(qosa_bool_t prod_mode);


extern qosa_dev_error_e qosa_dev_get_dumpcfg(qosa_dev_dumpcfg_e *dumpcfg);
typedef qosa_dev_error_e (*_api_qosa_dev_get_dumpcfg_t)(qosa_dev_dumpcfg_e *dumpcfg);

extern qosa_dev_error_e qosa_dev_set_dumpcfg(qosa_dev_dumpcfg_e dumpcfg);
typedef qosa_dev_error_e (*_api_qosa_dev_set_dumpcfg_t)(qosa_dev_dumpcfg_e dumpcfg);

extern qosa_dev_error_e qosa_dev_get_iptrace(qosa_uint32_t *percent);
typedef qosa_dev_error_e (*_api_qosa_dev_get_iptrace_t)(qosa_uint32_t *percent);

extern qosa_dev_error_e qosa_dev_set_iptrace(qosa_uint32_t percent);
typedef qosa_dev_error_e (*_api_qosa_dev_set_iptrace_t)(qosa_uint32_t percent);

extern qosa_dev_error_e qosa_dev_set_mac(const char *mac);
typedef qosa_dev_error_e (*_api_qosa_dev_set_mac_t)(const char *mac);

extern qosa_dev_error_e qosa_dev_get_mac(char *mac, qosa_int32_t *len);
typedef qosa_dev_error_e (*_api_qosa_dev_get_mac_t)(char *mac, qosa_int32_t *len);

extern void  dns_set_retry_params(int       retry_cnt, int retry_interval_time);
typedef void (*_api_dns_set_retry_params_t)(int  retry_cnt, int retry_interval_time);

extern void dns_disable_all_cache(void);
typedef void (*_api_dns_disable_all_cache_t)(void);

extern void dns_enable_all_cache(void);
typedef void (*_api_dns_enable_all_cache_t)(void);

extern struct qosa_vfs_dirent_t *qosa_vfs_readdir(QOSA_VFS_DIR *dirp);
typedef struct qosa_vfs_dirent_t *(*_api_qosa_vfs_readdir_t)(QOSA_VFS_DIR *dirp);

extern void qosa_ms_sim_init(void);
typedef void (* _api_qosa_ms_sim_init_t)(void);

extern void qosa_ms_nw_init(void);
typedef void (* _api_qosa_ms_nw_init_t)(void);

extern void qosa_ms_dev_init(void);
typedef void (* _api_qosa_ms_dev_init_t)(void);

extern void qosa_ms_usbnet_init(void);
typedef void (* _api_qosa_ms_usbnet_init_t)(void);

extern void qosa_ms_datacall_init(void);
typedef void (* _api_qosa_ms_datacall_init_t)(void);

extern void qosa_ms_sms_init(void);
typedef void (* _api_qosa_ms_sms_init_t)(void);

extern void qosa_ms_ims_init(void);
typedef void (* _api_qosa_ms_ims_init_t)(void);

extern QOSA_VFS_DIR *qosa_vfs_opendir(const char *name);
typedef QOSA_VFS_DIR *(* _api_qosa_vfs_opendir_t)(const char *name);

extern int qosa_task_create(qosa_task_t* taskRef, qosa_uint32_t stackSize, qosa_uint8_t priority, char* taskName, void (*taskStart)(void*), void* argv, ...);
typedef int (* _api_qosa_task_create_t)(qosa_task_t* taskRef, qosa_uint32_t stackSize, qosa_uint8_t priority, char* taskName, void (*taskStart)(void*), void* argv, ...);

extern qosa_uint32_t qosa_task_get_tcb_min_size(void);
typedef qosa_uint32_t (* _api_qosa_task_get_tcb_min_size_t)(void);

extern qosa_uint32_t qosa_task_get_tcb_align(void);
typedef qosa_uint32_t (* _api_qosa_task_get_tcb_align_t)(void);

extern int qosa_task_create_static(qosa_task_t* taskRef, void* stackMem, qosa_uint32_t stackSize, void* tcbMem, qosa_uint32_t tcbSize, qosa_uint8_t priority, char* taskName, void (*taskStart)(void*), void* argv, ...);
typedef int (* _api_qosa_task_create_static_t)(qosa_task_t* taskRef, void* stackMem, qosa_uint32_t stackSize, void* tcbMem, qosa_uint32_t tcbSize, qosa_uint8_t priority, char* taskName, void (*taskStart)(void*), void* argv, ...);

extern void qosa_enter_critical(void);
typedef void (* _api_qosa_enter_critical_t)(void);

extern qosa_uint32_t qosa_enter_critical_from_isr(void);
typedef qosa_uint32_t (* _api_qosa_enter_critical_from_isr_t)(void);

extern void qosa_exit_critical(void);
typedef void (* _api_qosa_exit_critical_t)(void);

extern void qosa_exit_critical_from_isr(qosa_uint32_t isrm);
typedef void (* _api_qosa_exit_critical_from_isr_t)(qosa_uint32_t isrm);
extern const char *qosa_at_param_hex_data(qosa_at_param_t *param, qosa_uint32_t len, qosa_bool_t *paramok);
typedef const char *(* _api_qosa_at_param_hex_data_t)(qosa_at_param_t *param, qosa_uint32_t len, qosa_bool_t *paramok);

extern const char *qosa_at_param_string(qosa_at_param_t *param, qosa_bool_t *paramok);
typedef const char *(* _api_qosa_at_param_string_t)(qosa_at_param_t *param, qosa_bool_t *paramok);

extern int qosa_timer_create(qosa_timer_t* timerRef, void (*callBackRoutine)(void*), void* argv);
typedef int (* _api_qosa_timer_create_t)(qosa_timer_t* timerRef, void (*callBackRoutine)(void*), void* argv);

extern const char *qosa_at_param_raw_text(qosa_at_param_t *param, qosa_bool_t *paramok);
typedef const char *(* _api_qosa_at_param_raw_text_t)(qosa_at_param_t *param, qosa_bool_t *paramok);

extern qosa_fota_t *qosa_fota_init(char *fota_name, qosa_bool_t del_old_file);
typedef qosa_fota_t *(* _api_qosa_fota_init_t)(char *fota_name, qosa_bool_t del_old_file);

extern const char *qosa_at_param_option_string(qosa_at_param_t *param, qosa_bool_t *paramok);
typedef const char *(* _api_qosa_at_param_option_string_t)(qosa_at_param_t *param, qosa_bool_t *paramok);

extern const char *qosa_at_param_default_string(qosa_at_param_t *param, const char *defval, qosa_bool_t *paramok);
typedef const char *(* _api_qosa_at_param_default_string_t)(qosa_at_param_t *param, const char *defval, qosa_bool_t *paramok);

extern qosa_rtc_time_t *qosa_rtc_gmtime_r(const qosa_time_t *timer, qosa_rtc_time_t *result);
typedef qosa_rtc_time_t *(* _api_qosa_rtc_gmtime_r_t)(const qosa_time_t *timer, qosa_rtc_time_t *result);

extern int qosa_hw_timer_create(qosa_hw_timer_t *timerRef, qosa_hw_timer_cfg_t *config);
typedef int (* _api_qosa_hw_timer_create_t)(qosa_hw_timer_t *timerRef, qosa_hw_timer_cfg_t *config);

extern int qosa_hw_timer_start(qosa_hw_timer_t timerRef, qosa_uint32_t time_us, qosa_bool_t cyclicalEn);
typedef int (* _api_qosa_hw_timer_start_t)(qosa_hw_timer_t timerRef, qosa_uint32_t time_us, qosa_bool_t cyclicalEn);

extern int qosa_hw_timer_stop(qosa_hw_timer_t timerRef);
typedef int (* _api_qosa_hw_timer_stop_t)(qosa_hw_timer_t timerRef);

extern int qosa_hw_timer_delete(qosa_hw_timer_t timerRef);
typedef int (* _api_qosa_hw_timer_delete_t)(qosa_hw_timer_t timerRef);

extern int qosa_hw_timer_get_count(qosa_hw_timer_t timerRef);
typedef int (* _api_qosa_hw_timer_get_count_t)(qosa_hw_timer_t timerRef);

extern int unir_async_select_mask(u16_t fd, u32_t interest_mask);
typedef int (* _api_unir_async_select_mask_t)(u16_t fd, u32_t interest_mask);

extern int unir_set_socket2(u16_t fd, void (*p)(u16_t, u32_t, void *), void *pam);
typedef int (* _api_unir_set_socket2_t)(u16_t fd, void (*p)(u16_t, u32_t, void *), void *pam);

extern int lwip_get_error(int s);
typedef int (* _api_lwip_get_error_t)(int s);

extern int32_t lwip_getAckedSize(int nSocket);
typedef int32_t (* _api_lwip_getAckedSize_t)(int s);

extern int32_t lwip_getRecvAvailSize(int nSocket);
typedef int32_t (* _api_lwip_getRecvAvailSize_t)(int s);

extern int lwip_get_last_close_event(int s);
typedef int (* _api_lwip_get_last_close_event_t)(int s);

extern int unir_get_lwip_tcp_rto_init(void);
typedef int (* _api_unir_get_lwip_tcp_rto_init_t)(void);

extern int unir_get_lwip_tcp_max_rtx(void);
typedef int (* _api_unir_get_lwip_tcp_max_rtx_t)(void);

extern int quecl_get_lwip_tcp_syn_max_rtx(void);
typedef int (* _api_quecl_get_lwip_tcp_syn_max_rtx_t)(void);

extern void  unir_set_lwip_tcp_rto_init(int lwip_tcp_rto_init);
typedef void (* _api_unir_set_lwip_tcp_rto_init_t)(int lwip_tcp_rto_init);

extern void  unir_set_lwip_tcp_max_rtx(int lwip_tcp_max_rtx);
typedef void (* _api_unir_set_lwip_tcp_max_rtx_t)(int lwip_tcp_max_rtx);

extern void  unir_set_lwip_tcp_syn_max_rtx(int lwip_tcp_syn_max_rtx);
typedef void (* _api_unir_set_lwip_tcp_syn_max_rtx_t)(int lwip_tcp_syn_max_rtx);

extern void  unir_set_lwip_tcp_window_scale(int lwip_tcp_window_scale);
typedef void (* _api_unir_set_lwip_tcp_window_scale_t)(int lwip_tcp_window_scale);

extern int  unir_get_lwip_tcp_window_scale(void);
typedef int (* _api_unir_get_lwip_tcp_window_scale_t)(void);

extern void  unir_set_lwip_tcp_twrecycle(int lwip_tcp_twrecycle);
typedef void (* _api_unir_set_lwip_tcp_twrecycle_t)(int lwip_tcp_twrecycle);

extern int  unir_get_lwip_tcp_twrecycle(void);
typedef int (* _api_unir_get_lwip_tcp_twrecycle_t)(void);

extern void  unir_set_lwip_udp_filter_mode(int lwip_udp_filter_mode);
typedef void (* _api_unir_set_lwip_udp_filter_mode_t)(int lwip_udp_filter_mode);

extern int  unir_get_lwip_udp_filter_mode(void);
typedef int (* _api_unir_get_lwip_udp_filter_mode_t)(void);

extern void  unir_set_lwip_tcp_recv_window(int lwip_tcp_recv_window);
typedef void (* _api_unir_set_lwip_tcp_recv_window_t)(int lwip_tcp_recv_window);

extern int  unir_get_lwip_tcp_recv_window(void);
typedef int (* _api_unir_get_lwip_tcp_recv_window_t)(void);

extern void  unir_set_lwip_tcp_send_window(int lwip_tcp_send_window);
typedef void (* _api_unir_set_lwip_tcp_send_window_t)(int lwip_tcp_send_window);

extern int  unir_get_lwip_tcp_send_window(void);
typedef int (* _api_unir_get_lwip_tcp_send_window_t)(void);

extern void  unir_set_lwip_tcp_mss(int lwip_tcp_mss);
typedef void (* _api_unir_set_lwip_tcp_mss_t)(int lwip_tcp_mss);

extern int  unir_get_lwip_tcp_mss(void);
typedef int (* _api_unir_get_lwip_tcp_mss_t)(void);

extern void qosa_log_vprintf(int level, int tag, const char *funcname, int line,
                           const char *fmt, va_list args);
typedef void (*_api_qosa_log_vprintf_t)(int level, int tag, const char *funcname,
                                      int line, const char *fmt, va_list args);

extern void qosa_log_printf(int level, int tag, const char *funcname, int line,
                           const char *fmt, ...);
typedef void (*_api_qosa_log_printf_t)(int level, int tag, const char *funcname,
                                      int line, const char *fmt, ...);

extern qosa_uint32_t qosa_log_control_get(void);
typedef qosa_uint32_t (* _api_qosa_log_control_get_t)(void);

extern qosa_log_error_e qosa_log_control_set(qosa_uint32_t mask);
typedef qosa_log_error_e (* _api_qosa_log_control_set_t)(qosa_uint32_t mask);

extern qosa_at_sio_dev_mode_e qosa_at_sio_get_at_dev_mode(qosa_at_dev_type_e dev_port);
typedef qosa_at_sio_dev_mode_e (* _api_qosa_at_sio_get_at_dev_mode_t)(qosa_at_dev_type_e dev_port);

extern void qosa_at_sio_set_port_last_ato_wait_owner(qosa_at_ato_switch_cb_t* func_tbl, qosa_at_dev_type_e dev_port);
typedef void (* _api_qosa_at_sio_set_port_last_ato_wait_owner_t)(qosa_at_ato_switch_cb_t* func_tbl, qosa_at_dev_type_e dev_port);

extern qosa_at_ato_switch_cb_t* qosa_at_sio_get_port_last_ato_wait_owner(qosa_at_dev_type_e dev_port);
typedef qosa_at_ato_switch_cb_t* (* _api_qosa_at_sio_get_port_last_ato_wait_owner_t)(qosa_at_dev_type_e dev_port);

extern void qosa_at_sio_clean_last_ato_owner(qosa_at_ato_switch_cb_t* func_tbl, qosa_at_dev_type_e dev_port);
typedef void (* _api_qosa_at_sio_clean_last_ato_owner_t)(qosa_at_ato_switch_cb_t* func_tbl, qosa_at_dev_type_e dev_port);

extern qosa_bool_t qosa_at_sio_register_callback_func(qosa_at_sio_func_tbl_t* func_tbl, qosa_at_dev_type_e dev_port);
typedef qosa_bool_t (* _api_qosa_at_sio_register_callback_func_t)(qosa_at_sio_func_tbl_t* func_tbl, qosa_at_dev_type_e dev_port);

extern qosa_bool_t qosa_at_sio_ex_change_mode(qosa_at_sio_dev_mode_e uart_mode, qosa_buffer_watermark_t* rx_wm_ptr, qosa_buffer_watermark_t* tx_wm_ptr, qosa_at_dev_type_e dev_port);
typedef qosa_bool_t (* _api_qosa_at_sio_ex_change_mode_t)(qosa_at_sio_dev_mode_e uart_mode, qosa_buffer_watermark_t* rx_wm_ptr, qosa_buffer_watermark_t* tx_wm_ptr, qosa_at_dev_type_e dev_port);

extern void qosa_at_sio_report_data_mode_switch_atcmd(qosa_at_dev_type_e dev_port);
typedef void (* _api_qosa_at_sio_report_data_mode_switch_atcmd_t)(qosa_at_dev_type_e dev_port);

extern char* qosa_at_sio_get_dev_name(qosa_at_dev_type_e dev_port);
typedef char* (* _api_qosa_at_sio_get_dev_name_t)(qosa_at_dev_type_e dev_port);

extern void qosa_at_sio_dev_write(qosa_at_dev_type_e dev_port, qosa_uint8_t* resp, qosa_uint16_t len);
typedef void (* _api_qosa_at_sio_dev_write_t)(qosa_at_dev_type_e dev_port, qosa_uint8_t* resp, qosa_uint16_t len);

extern int qosa_at_sio_dev_transmit(qosa_at_dev_type_e dev_port, qosa_buffer_block_t* data_ptr);
typedef int (* _api_qosa_at_sio_dev_transmit_t)(qosa_at_dev_type_e dev_port, qosa_buffer_block_t* data_ptr);

extern qosa_task_t qosa_center_get_task_ptr(void);
typedef qosa_task_t (* _api_qosa_center_get_task_ptr_t)(void);

extern void qosa_center_send_cmd(qosa_center_cmd_event_enum_e cmd, void *user_data_prt);
typedef void (* _api_qosa_center_send_cmd_t)(qosa_center_cmd_event_enum_e cmd, void *user_data_prt);

extern qosa_center_cmd_handler_ptr qosa_center_set_cmd_handler(qosa_center_cmd_event_enum_e cmd, qosa_center_cmd_handler_ptr cmd_handler);
typedef qosa_center_cmd_handler_ptr (* _api_qosa_center_set_cmd_handler_t)(qosa_center_cmd_event_enum_e cmd, qosa_center_cmd_handler_ptr cmd_handler);

extern qosa_uint32_t qosa_at_get_tx_avail_size(qosa_at_dev_type_e dev_port);
typedef qosa_uint32_t (* _api_qosa_at_get_tx_avail_size_t)(qosa_at_dev_type_e dev_port);

extern qosa_bool_t qosa_at_dev_port_ioctl(qosa_at_dev_type_e dev_port, qosa_dev_ioctl_cmd_e cmd, qosa_dev_ioctl_type *type);
typedef qosa_bool_t (* _api_qosa_at_dev_port_ioctl_t)(qosa_at_dev_type_e dev_port, qosa_dev_ioctl_cmd_e cmd, qosa_dev_ioctl_type *type);

extern void qosa_at_resp_basic_result_code(qosa_at_dev_type_e dev_port, qosa_at_resp_base_result_e code);
typedef void (* _api_qosa_at_resp_basic_result_code_t)(qosa_at_dev_type_e dev_port, qosa_at_resp_base_result_e code);

extern int qosa_at_resp_finish(qosa_at_dev_type_e dev_port);
typedef int (* _api_qosa_at_resp_finish_t)(qosa_at_dev_type_e dev_port);

extern int qosa_at_resp_cmd_info_text(qosa_at_dev_type_e dev_port, const char *text, size_t length, unsigned char padding);
typedef int (* _api_qosa_at_resp_cmd_info_text_t)(qosa_at_dev_type_e dev_port, const char *text, size_t length, unsigned char padding);

extern void qosa_at_resp_cmd(qosa_at_dev_type_e dev_port, qosa_atci_result_code_e resultCode, qosa_uint32_t report_code, char *rsp_buffer, qosa_uint8_t padding);
typedef void (* _api_qosa_at_resp_cmd_t)(qosa_at_dev_type_e dev_port, qosa_atci_result_code_e resultCode, qosa_uint32_t report_code, char *rsp_buffer, qosa_uint8_t padding);

extern void qosa_at_resp_cmd_out_put_prompt(qosa_at_dev_type_e dev_port, int formatcfg);
typedef void (* _api_qosa_at_resp_cmd_out_put_prompt_t)(qosa_at_dev_type_e dev_port, int formatcfg);

extern int qosa_at_enter_edit_mode(qosa_at_dev_type_e dev_port, qosa_at_edit_entry_cb_ptr cb, void *param);
typedef int (* _api_qosa_at_enter_edit_mode_t)(qosa_at_dev_type_e dev_port, qosa_at_edit_entry_cb_ptr cb, void *param);

extern int qosa_at_exit_edit_mode(qosa_at_dev_type_e dev_port);
typedef int (* _api_qosa_at_exit_edit_mode_t)(qosa_at_dev_type_e dev_port);

extern qosa_bool_t qosa_at_get_is_cmd_mode(qosa_at_dev_type_e dev_port);
typedef qosa_bool_t (* _api_qosa_at_get_is_cmd_mode_t)(qosa_at_dev_type_e dev_port);

extern qosa_uint8_t qosa_get_sim_by_dev(qosa_at_dev_type_e dev_port);
typedef qosa_uint8_t (* _api_qosa_get_sim_by_dev_t)(qosa_at_dev_type_e dev_port);

extern void qosa_at_parser_add_cust_at(const qosa_at_desc_t *desc, qosa_uint32_t list_len);
typedef void (* _api_qosa_at_parser_add_cust_at_t)(const qosa_at_desc_t *desc, qosa_uint32_t list_len);

extern qosa_uint8_t qosa_at_config_get(qosa_at_dev_type_e dev_port, qosa_at_config_e config, qosa_uint8_t sim_id);
typedef qosa_uint8_t (* _api_qosa_at_config_get_t)(qosa_at_dev_type_e dev_port, qosa_at_config_e config, qosa_uint8_t sim_id);

extern qosa_bool_t qosa_at_config_set(qosa_at_dev_type_e dev_port, qosa_at_config_e config, qosa_uint8_t sim_id, qosa_uint8_t value);
typedef qosa_uint8_t (* _api_qosa_at_config_set_t)(qosa_at_dev_type_e dev_port, qosa_at_config_e config, qosa_uint8_t sim_id, qosa_uint8_t value);

extern qosa_uint32_t qosa_at_uart_config_get(qosa_at_dev_type_e dev_port, qosa_at_uart_cfg_e config);
typedef qosa_uint32_t (* _api_qosa_at_uart_config_get_t)(qosa_at_dev_type_e dev_port, qosa_at_uart_cfg_e config);

extern qosa_bool_t qosa_at_uart_config_set(qosa_at_dev_type_e dev_port, qosa_at_uart_cfg_e config, qosa_uint32_t value);
typedef qosa_bool_t (* _api_qosa_at_uart_config_set_t)(qosa_at_dev_type_e dev_port, qosa_at_uart_cfg_e config, qosa_uint32_t value);

extern qosa_uint32_t qosa_at_param_uint(qosa_at_param_t *param, qosa_bool_t *paramok);
typedef qosa_uint32_t (* _api_qosa_at_param_uint_t)(qosa_at_param_t *param, qosa_bool_t *paramok);

extern qosa_uint32_t qosa_at_param_default_uint(qosa_at_param_t *param, qosa_uint32_t defval, qosa_bool_t *paramok);
typedef qosa_uint32_t (* _api_qosa_at_param_default_uint_t)(qosa_at_param_t *param, qosa_uint32_t defval, qosa_bool_t *paramok);

extern qosa_uint32_t qosa_at_param_uint_in_range(qosa_at_param_t *param, qosa_uint32_t minval, qosa_uint32_t maxval, qosa_bool_t *paramok);
typedef qosa_uint32_t (* _api_qosa_at_param_uint_in_range_t)(qosa_at_param_t *param, qosa_uint32_t minval, qosa_uint32_t maxval, qosa_bool_t *paramok);

extern qosa_uint32_t qosa_at_param_default_uint_in_range(qosa_at_param_t *param, qosa_uint32_t defval, qosa_uint32_t minval, qosa_uint32_t maxval, qosa_bool_t *paramok);
typedef qosa_uint32_t (* _api_qosa_at_param_default_uint_in_range_t)(qosa_at_param_t *param, qosa_uint32_t defval, qosa_uint32_t minval, qosa_uint32_t maxval, qosa_bool_t *paramok);

extern qosa_uint32_t qosa_at_param_uint_in_list(qosa_at_param_t *param, const qosa_uint32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);
typedef qosa_uint32_t (* _api_qosa_at_param_uint_in_list_t)(qosa_at_param_t *param, const qosa_uint32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

extern qosa_uint32_t qosa_at_param_default_uint_in_list(qosa_at_param_t *param, qosa_uint32_t defval, const qosa_uint32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);
typedef qosa_uint32_t (* _api_qosa_at_param_default_uint_in_list_t)(qosa_at_param_t *param, qosa_uint32_t defval, const qosa_uint32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

extern qosa_int32_t qosa_at_param_int(qosa_at_param_t *param, qosa_bool_t *paramok);
typedef qosa_int32_t (* _api_qosa_at_param_int_t)(qosa_at_param_t *param, qosa_bool_t *paramok);

extern qosa_int32_t qosa_at_param_default_int(qosa_at_param_t *param, qosa_int32_t defval, qosa_bool_t *paramok);
typedef qosa_int32_t (* _api_qosa_at_param_default_int_t)(qosa_at_param_t *param, qosa_int32_t defval, qosa_bool_t *paramok);

extern qosa_int32_t qosa_at_param_int_in_range(qosa_at_param_t *param, qosa_int32_t minval, qosa_int32_t maxval, qosa_bool_t *paramok);
typedef qosa_int32_t (* _api_qosa_at_param_int_in_range_t)(qosa_at_param_t *param, qosa_int32_t minval, qosa_int32_t maxval, qosa_bool_t *paramok);

extern qosa_int32_t qosa_at_param_default_int_in_range(qosa_at_param_t *param, qosa_int32_t defval, qosa_int32_t minval, qosa_int32_t maxval, qosa_bool_t *paramok);
typedef qosa_int32_t (* _api_qosa_at_param_default_int_in_range_t)(qosa_at_param_t *param, qosa_int32_t defval, qosa_int32_t minval, qosa_int32_t maxval, qosa_bool_t *paramok);

extern qosa_int32_t qosa_at_param_int_in_list(qosa_at_param_t *param, const qosa_int32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);
typedef qosa_int32_t (* _api_qosa_at_param_int_in_list_t)(qosa_at_param_t *param, const qosa_int32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

extern qosa_int32_t qosa_at_param_default_int_in_list(qosa_at_param_t *param, qosa_int32_t defval, const qosa_int32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);
typedef qosa_int32_t (* _api_qosa_at_param_default_int_in_list_t)(qosa_at_param_t *param, qosa_int32_t defval, const qosa_int32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

extern qosa_uint64_t qosa_at_param_uint64(qosa_at_param_t *param, qosa_bool_t *paramok);
typedef qosa_uint64_t (* _api_qosa_at_param_uint64_t)(qosa_at_param_t *param, qosa_bool_t *paramok);

extern qosa_uint64_t qosa_at_param_default_uint64(qosa_at_param_t *param, qosa_uint64_t defval, qosa_bool_t *paramok);
typedef qosa_uint64_t (* _api_qosa_at_param_default_uint64_t)(qosa_at_param_t *param, qosa_uint64_t defval, qosa_bool_t *paramok);

extern qosa_uint64_t qosa_at_param_uint64_in_range(qosa_at_param_t *param, qosa_uint64_t minval, qosa_uint64_t maxval, qosa_bool_t *paramok);
typedef qosa_uint64_t (* _api_qosa_at_param_uint64_in_range_t)(qosa_at_param_t *param, qosa_uint64_t minval, qosa_uint64_t maxval, qosa_bool_t *paramok);

extern qosa_uint64_t qosa_at_param_default_uint64_in_range(qosa_at_param_t *param, qosa_uint64_t defval, qosa_uint64_t minval, qosa_uint64_t maxval, qosa_bool_t *paramok);
typedef qosa_uint64_t (* _api_qosa_at_param_default_uint64_in_range_t)(qosa_at_param_t *param, qosa_uint64_t defval, qosa_uint64_t minval, qosa_uint64_t maxval, qosa_bool_t *paramok);

extern qosa_uint64_t qosa_at_param_uint64_in_list(qosa_at_param_t *param, const qosa_uint64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);
typedef qosa_uint64_t (* _api_qosa_at_param_uint64_in_list_t)(qosa_at_param_t *param, const qosa_uint64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

extern qosa_uint64_t qosa_at_param_default_uint64_in_list(qosa_at_param_t *param, qosa_uint64_t defval, const qosa_uint64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);
typedef qosa_uint64_t (* _api_qosa_at_param_default_uint64_in_list_t)(qosa_at_param_t *param, qosa_uint64_t defval, const qosa_uint64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

extern qosa_int64_t qosa_at_param_int64(qosa_at_param_t *param, qosa_bool_t *paramok);
typedef qosa_int64_t (* _api_qosa_at_param_int64_t)(qosa_at_param_t *param, qosa_bool_t *paramok);

extern qosa_int64_t qosa_at_param_default_int64(qosa_at_param_t *param, qosa_int64_t defval, qosa_bool_t *paramok);
typedef qosa_int64_t (* _api_qosa_at_param_default_int64_t)(qosa_at_param_t *param, qosa_int64_t defval, qosa_bool_t *paramok);

extern qosa_int64_t qosa_at_param_int64_in_range(qosa_at_param_t *param, qosa_int64_t minval, qosa_int64_t maxval, qosa_bool_t *paramok);
typedef qosa_int64_t (* _api_qosa_at_param_int64_in_range_t)(qosa_at_param_t *param, qosa_int64_t minval, qosa_int64_t maxval, qosa_bool_t *paramok);

extern qosa_int64_t qosa_at_param_default_int64_in_range(qosa_at_param_t *param, qosa_int64_t defval, qosa_int64_t minval, qosa_int64_t maxval, qosa_bool_t *paramok);
typedef qosa_int64_t (* _api_qosa_at_param_default_int64_in_range_t)(qosa_at_param_t *param, qosa_int64_t defval, qosa_int64_t minval, qosa_int64_t maxval, qosa_bool_t *paramok);

extern qosa_int64_t qosa_at_param_int64_in_list(qosa_at_param_t *param, const qosa_int64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);
typedef qosa_int64_t (* _api_qosa_at_param_int64_in_list_t)(qosa_at_param_t *param, const qosa_int64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

extern qosa_int64_t qosa_at_param_default_int64_in_list(qosa_at_param_t *param, qosa_int64_t defval, const qosa_int64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);
typedef qosa_int64_t (* _api_qosa_at_param_default_int64_in_list_t)(qosa_at_param_t *param, qosa_int64_t defval, const qosa_int64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

extern double qosa_at_param_double(qosa_at_param_t *param, qosa_bool_t *paramok);
typedef double (* _api_qosa_at_param_double_t)(qosa_at_param_t *param, qosa_bool_t *paramok);

extern qosa_uint32_t qosa_at_param_uint_by_string_map(qosa_at_param_t *param, const qosa_value_str_map_t *vsmap, qosa_bool_t *paramok);
typedef qosa_uint32_t (* _api_qosa_at_param_uint_by_string_map_t)(qosa_at_param_t *param, const qosa_value_str_map_t *vsmap, qosa_bool_t *paramok);

extern qosa_uint32_t qosa_at_param_default_uint_by_string_map(qosa_at_param_t *param, qosa_uint32_t defval, const qosa_value_str_map_t *vsmap, qosa_bool_t *paramok);
typedef qosa_uint32_t (* _api_qosa_at_param_default_uint_by_string_map_t)(qosa_at_param_t *param, qosa_uint32_t defval, const qosa_value_str_map_t *vsmap, qosa_bool_t *paramok);

extern qosa_uint32_t qosa_at_param_hex_string_uint(qosa_at_param_t *param, qosa_bool_t *paramok);
typedef qosa_uint32_t (* _api_qosa_at_param_hex_string_uint_t)(qosa_at_param_t *param, qosa_bool_t *paramok);

extern qosa_bool_t qosa_at_param_trim_tail(qosa_at_param_t *param, qosa_uint32_t len);
typedef qosa_bool_t (* _api_qosa_at_param_trim_tail_t)(qosa_at_param_t *param, qosa_uint32_t len);

extern qosa_bool_t qosa_at_param_is_empty(qosa_at_param_t *param);
typedef qosa_bool_t (* _api_qosa_at_param_is_empty_t)(qosa_at_param_t *param);

extern qosa_int64_t qosa_at_param_hex_int64_in_range(qosa_at_param_t *param, qosa_int64_t minval, qosa_int64_t maxval, qosa_bool_t *paramok);
typedef qosa_int64_t (* _api_qosa_at_param_hex_int64_in_range_t)(qosa_at_param_t *param, qosa_int64_t minval, qosa_int64_t maxval, qosa_bool_t *paramok);

extern qosa_dev_error_e qosa_dev_get_memory_size(qosa_ram_info_t *ram_info_ptr);
typedef qosa_dev_error_e (* _api_qosa_dev_get_memory_size_t)(qosa_ram_info_t *ram_info_ptr);

extern qosa_dev_error_e qosa_dev_get_memory_sub_size(qosa_ram_info_t *ram_info_ptr);
typedef qosa_dev_error_e (* _api_qosa_dev_get_memory_sub_size_t)(qosa_ram_info_t *ram_info_ptr);

extern qosa_dev_error_e qosa_dev_get_sn(char *SN, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_sn_t)(char *SN, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_hvn(char *HVN, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_hvn_t)(char *HVN, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_sim_imei(qosa_int32_t sim_id, char *imei, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_sim_imei_t)(qosa_int32_t sim_id, char *imei, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_product_id(char *product_id, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_product_id_t)(char *product_id, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_set_auto_download(void);
typedef qosa_dev_error_e (* _api_qosa_dev_set_auto_download_t)(void);

extern qosa_dev_error_e qosa_dev_get_cust_firmware_version(char *version, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_cust_firmware_version_t)(char *version, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_qgmr_firmware_version(char *firmware_version, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_qgmr_firmware_version_t)(char *firmware_version, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_firmware_version(char *firmware_version, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_firmware_version_t)(char *firmware_version, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_firmware_subversion(char *firmware_subversion, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_firmware_subversion_t)(char *firmware_subversion, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_firmware_qinf(char *firmware_qinf, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_firmware_qinf_t)(char *firmware_qinf, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_qversion(char *qversion, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_qversion_t)(char *qversion, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_model(char *model, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_model_t)(char *model, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_oem(char *oem, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_oem_t)(char *oem, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_usb_product(char *product, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_usb_product_t)(char *product, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_chip_type(char *chip_type, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_chip_type_t)(char *chip_type, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_chip_manufacturer(char *chip_manufacturer, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_chip_manufacturer_t)(char *chip_manufacturer, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_svn(char *svn, qosa_int32_t *len);
typedef qosa_dev_error_e (* _api_qosa_dev_get_svn_t)(char *svn, qosa_int32_t *len);

extern qosa_dev_error_e qosa_dev_get_temp_value(qosa_int32_t *temp_num);
typedef qosa_dev_error_e (* _api_qosa_dev_get_temp_value_t)(qosa_int32_t *temp_num);

extern qosa_dev_error_e qosa_dev_set_sim_imei(qosa_uint8_t simid, char *imei_str);
typedef qosa_dev_error_e (* _api_qosa_dev_set_sim_imei_t)(qosa_uint8_t simid, char *imei_str);

extern qosa_dev_error_e qosa_dev_set_sn(char *sn_str);
typedef qosa_dev_error_e (* _api_qosa_dev_set_sn_t)(char *sn_str);

extern qosa_dev_error_e qosa_dev_get_cpu_uid(qosa_uint64_t *chip_id);
typedef qosa_dev_error_e (* _api_qosa_dev_get_cpu_uid_t)(qosa_uint64_t *chip_id);

extern qosa_dev_error_e qosa_dev_get_sdmmc_is_mount(qosa_int32_t sdmmc_part, qosa_int64_t *log_size);
typedef qosa_dev_error_e (* _api_qosa_dev_get_sdmmc_is_mount_t)(qosa_int32_t sdmmc_part, qosa_int64_t *log_size);

extern qosa_bool_t qosa_dev_filesystem_reset(void);
typedef qosa_bool_t (* _api_qosa_dev_filesystem_reset_t)(void);

extern void qosa_dev_watch_dog_update(void);
typedef void (* _api_qosa_dev_watch_dog_update_t)(void);

extern qosa_bool_t qosa_dev_dump_occure_check(void);
typedef qosa_bool_t (* _api_qosa_dev_dump_occure_check_t)(void);

extern qosa_e_n_error_e qosa_event_notify_register(qosa_notify_event_e event, event_callback_ptr event_cb, void *user_argv);
typedef qosa_e_n_error_e (* _api_qosa_event_notify_register_t)(qosa_notify_event_e event, event_callback_ptr event_cb, void *user_argv);

extern qosa_e_n_error_e qosa_event_notify_unregister(qosa_notify_event_e event, event_callback_ptr event_cb);
typedef qosa_e_n_error_e (* _api_qosa_event_notify_unregister_t)(qosa_notify_event_e event, event_callback_ptr event_cb);

extern qosa_e_n_error_e qosa_event_notify_report(qosa_notify_event_e event, void *notifyData);
typedef qosa_e_n_error_e (* _api_qosa_event_notify_report_t)(qosa_notify_event_e event, void *notifyData);

extern void qosa_fota_deinit(qosa_fota_t *fota);
typedef void (* _api_qosa_fota_deinit_t)(qosa_fota_t *fota);

extern qosa_fota_verify_control_e qosa_get_fota_verify_config(void);
typedef qosa_fota_verify_control_e (* _api_qosa_get_fota_verify_config_t)(void);

extern qosa_fota_errno_e qosa_set_fota_verify_config(qosa_fota_verify_control_e value);
typedef qosa_fota_errno_e (* _api_qosa_set_fota_verify_config_t)(qosa_fota_verify_control_e value);

extern qosa_fota_errno_e qosa_fota_verify_image(qosa_fota_t *fota);
typedef qosa_fota_errno_e (* _api_qosa_fota_verify_image_t)(qosa_fota_t *fota);

extern qosa_int64_t qosa_fota_get_partition_space(qosa_fota_t *fota);
typedef qosa_int64_t (* _api_qosa_fota_get_partition_space_t)(qosa_fota_t *fota);

extern qosa_fota_errno_e qosa_fota_get_update_result(void);
typedef qosa_fota_errno_e (* _api_qosa_fota_get_update_result_t)(void);

extern qosa_fota_errno_e qosa_fota_write_packet_data(qosa_fota_t *fota, qosa_uint8_t *payload, qosa_size_t payload_len);
typedef qosa_fota_errno_e (* _api_qosa_fota_write_packet_data_t)(qosa_fota_t *fota, qosa_uint8_t *payload, qosa_size_t payload_len);

extern qosa_fota_errno_e qosa_fota_set_update_urc_num(qosa_fota_t *fota, qosa_uint8_t update_urc_num);
typedef qosa_fota_errno_e (* _api_qosa_fota_set_update_urc_num_t)(qosa_fota_t *fota, qosa_uint8_t update_urc_num);

extern qosa_int64_t qosa_fota_get_current_file_size(qosa_fota_t *fota);
typedef qosa_int64_t (* _api_qosa_fota_get_current_file_size_t)(qosa_fota_t *fota);

extern qosa_fota_errno_e qosa_fota_get_default_packname(char *packname, int *length);
typedef qosa_fota_errno_e (* _api_qosa_fota_get_default_packname_t)(char *packname, int *length);

extern qosa_fota_errno_e qosa_fota_set_packname(char *packetname, int length);
typedef qosa_fota_errno_e (* _api_qosa_fota_set_packname_t)(char *packetname, int length);

extern void qosa_fota_file_reset(void);
typedef void (* _api_qosa_fota_file_reset_t)(void);

extern qosa_fota_errno_e qosa_fota_set_opt(qosa_fota_opt_cmd_e cmd, void *param);
typedef qosa_fota_errno_e (* _api_qosa_fota_set_opt_t)(qosa_fota_opt_cmd_e cmd, void *param);

extern void qosa_fota_flag_set(void);
typedef void (* _api_qosa_fota_flag_set_t)(void);

extern qosa_bool_t qosa_fota_get_updater_info(void);
typedef qosa_bool_t (* _api_qosa_fota_get_updater_info_t)(void);

extern int qosa_fota_updater_start(void);
typedef int (* _api_qosa_fota_updater_start_t)(void);

extern qosa_power_error_e qosa_power_down(qosa_powd_mode_e powd_mode);
typedef qosa_power_error_e (* _api_qosa_power_down_t)(qosa_powd_mode_e powd_mode);

extern qosa_power_error_e qosa_power_reset(qosa_reset_mode_e reset_mode);
typedef qosa_power_error_e (* _api_qosa_power_reset_t)(qosa_reset_mode_e reset_mode);

qosa_power_error_e qosa_power_get_boot_cause(qosa_boot_cause_e *boot_cause);
typedef qosa_power_error_e (* _api_qosa_power_get_boot_cause_t)(qosa_boot_cause_e *boot_cause);

extern qosa_power_error_e qosa_power_get_charger_status(qosa_charge_status_e *nBcs, qosa_uint8_t *nBcl, qosa_uint32_t *vol);
typedef qosa_power_error_e (* _api_qosa_power_get_charger_status_t)(qosa_charge_status_e *nBcs, qosa_uint8_t *nBcl, qosa_uint32_t *vol);

extern qosa_power_error_e qosa_get_pwrkey_level(qosa_uint8_t *pwrkey_level);
typedef qosa_power_error_e (* _api_qosa_get_pwrkey_level_t)(qosa_uint8_t *pwrkey_level);

extern qosa_power_error_e qosa_pwrkey_callback_register(qosa_pwrkey_callback pwrkey_cb);
typedef qosa_power_error_e (* _api_qosa_pwrkey_callback_register_t)(qosa_pwrkey_callback pwrkey_cb);

extern qosa_bool_t qosa_get_system_time(qosa_time_info_t *time);
typedef qosa_bool_t (* _api_qosa_get_system_time_t)(qosa_time_info_t *time);

extern qosa_time_t qosa_get_system_time_seconds(void);
typedef qosa_time_t (* _api_qosa_get_system_time_seconds_t)(void);

extern qosa_time_t qosa_get_system_time_milliseconds(void);
typedef qosa_time_t (* _api_qosa_get_system_time_milliseconds_t)(void);

extern qosa_time_t qosa_get_system_time_microseconds(void);
typedef qosa_time_t (* _api_qosa_get_system_time_microseconds_t)(void);

extern int qosa_rtc_set_time(qosa_time_t time);
typedef int (* _api_qosa_rtc_set_time_t)(qosa_time_t time);

extern int qosa_rtc_get_time(qosa_time_t *time);
typedef int (* _api_qosa_rtc_get_time_t)(qosa_time_t *time);

extern int qosa_rtc_get_localtime(qosa_time_t *time);
typedef int (* _api_qosa_rtc_get_localtime_t)(qosa_time_t *time);

extern int qosa_rtc_get_timezone(void);
typedef int (* _api_qosa_rtc_get_timezone_t)(void);

extern int qosa_rtc_set_timezone(int time_zone);
typedef int (* _api_qosa_rtc_set_timezone_t)(int time_zone);

extern int qosa_rtc_mktime(qosa_rtc_time_t *rtc_time, qosa_time_t *timer);
typedef int (* _api_qosa_rtc_mktime_t)(qosa_rtc_time_t *rtc_time, qosa_time_t *timer);

extern qosa_gpio_error_e qosa_gpio_init(qosa_gpio_num_e gpio_num, qosa_gpio_direction_e gpio_direction, qosa_gpio_pull_e gpio_pull, qosa_gpio_level_e gpio_level);
typedef qosa_gpio_error_e (* _api_qosa_gpio_init_t)(qosa_gpio_num_e gpio_num, qosa_gpio_direction_e gpio_direction, qosa_gpio_pull_e gpio_pull, qosa_gpio_level_e gpio_level);

extern qosa_gpio_error_e qosa_gpio_deinit(qosa_gpio_num_e gpio_num);
typedef qosa_gpio_error_e (* _api_qosa_gpio_deinit_t)(qosa_gpio_num_e gpio_num);

extern qosa_gpio_error_e qosa_gpio_set_level(qosa_gpio_num_e gpio_num, qosa_gpio_level_e gpio_level);
typedef qosa_gpio_error_e (* _api_qosa_gpio_set_level_t)(qosa_gpio_num_e gpio_num, qosa_gpio_level_e gpio_level);

extern qosa_gpio_error_e qosa_gpio_get_level(qosa_gpio_num_e gpio_num, qosa_gpio_level_e *gpio_level);
typedef qosa_gpio_error_e (* _api_qosa_gpio_get_level_t)(qosa_gpio_num_e gpio_num, qosa_gpio_level_e *gpio_level);

extern qosa_gpio_error_e qosa_gpio_set_direction(qosa_gpio_num_e gpio_num, qosa_gpio_direction_e gpio_direction);
typedef qosa_gpio_error_e (* _api_qosa_gpio_set_direction_t)(qosa_gpio_num_e gpio_num, qosa_gpio_direction_e gpio_direction);

extern qosa_gpio_error_e qosa_gpio_get_direction(qosa_gpio_num_e gpio_num, qosa_gpio_direction_e *gpio_direction);
typedef qosa_gpio_error_e (* _api_qosa_gpio_get_direction_t)(qosa_gpio_num_e gpio_num, qosa_gpio_direction_e *gpio_direction);

extern qosa_gpio_error_e qosa_gpio_set_pull(qosa_gpio_num_e gpio_num, qosa_gpio_pull_e gpio_pull);
typedef qosa_gpio_error_e (* _api_qosa_gpio_set_pull_t)(qosa_gpio_num_e gpio_num, qosa_gpio_pull_e gpio_pull);

extern qosa_gpio_error_e qosa_gpio_get_pull(qosa_gpio_num_e gpio_num, qosa_gpio_pull_e *gpio_pull);
typedef qosa_gpio_error_e (* _api_qosa_gpio_get_pull_t)(qosa_gpio_num_e gpio_num, qosa_gpio_pull_e *gpio_pull);

extern void qosa_set_swd_func(void);
typedef void (* _api_qosa_set_swd_func_t)(void);

extern qosa_gpio_error_e qosa_interrupt_register(qosa_int_cfg_t *qosa_int_cfg);
typedef qosa_gpio_error_e (* _api_qosa_interrupt_register_t)(qosa_int_cfg_t *qosa_int_cfg);

extern qosa_gpio_error_e qosa_interrupt_unregister(qosa_gpio_num_e gpio_num);
typedef qosa_gpio_error_e (* _api_qosa_interrupt_unregister_t)(qosa_gpio_num_e gpio_num);

extern qosa_gpio_error_e qosa_interrupt_enable(qosa_gpio_num_e gpio_num, qosa_gpio_trigger_e pin_edge);
typedef qosa_gpio_error_e (* _api_qosa_interrupt_enable_t)(qosa_gpio_num_e gpio_num, qosa_gpio_trigger_e pin_edge);

extern qosa_gpio_error_e qosa_interrupt_disable(qosa_gpio_num_e gpio_num);
typedef qosa_gpio_error_e (* _api_qosa_interrupt_disable_t)(qosa_gpio_num_e gpio_num);

extern qosa_gpio_error_e qosa_get_pin_default_cfg(qosa_uint8_t pin_num, qosa_pin_cfg_t *pin_cfg);
typedef qosa_gpio_error_e (* _api_qosa_get_pin_default_cfg_t)(qosa_uint8_t pin_num, qosa_pin_cfg_t *pin_cfg);

extern qosa_gpio_error_e qosa_gpio_set_voltage(qosa_gpio_vol_e gpio_vol);
typedef qosa_gpio_error_e (* _api_qosa_gpio_set_voltage_t)(qosa_gpio_vol_e gpio_vol);

extern qosa_gpio_error_e qosa_aon_gpio_power_control(qosa_gpio_power_control_e power_control);
typedef qosa_gpio_error_e (* _api_qosa_aon_gpio_power_control_t)(qosa_gpio_power_control_e power_control);

extern qosa_pwm_error_e qosa_pwm_config(unsigned char pwm_sel, qosa_pwm_info_t *pwm_info);
typedef qosa_pwm_error_e (* _api_qosa_pwm_config_t)(unsigned char pwm_sel, qosa_pwm_info_t *pwm_info);

extern qosa_pwm_error_e qosa_pwm_enable(unsigned char pwm_sel,unsigned int high_one_cycle_duration);
typedef qosa_pwm_error_e (* _api_qosa_pwm_enable_t)(unsigned char pwm_sel,unsigned int high_one_cycle_duration);

extern qosa_pwm_error_e qosa_pwm_disable(unsigned char pwm_sel);
typedef qosa_pwm_error_e (* _api_qosa_pwm_disable_t)(unsigned char pwm_sel);

extern qosa_built_nor_error_e qosa_builtin_flash_read(qosa_uint32_t addr, qosa_uint8_t *data, qosa_uint32_t len);
typedef qosa_built_nor_error_e (* _api_qosa_builtin_flash_read_t)(qosa_uint32_t addr, qosa_uint8_t *data, qosa_uint32_t len);

extern qosa_built_nor_error_e qosa_builtin_flash_write(qosa_uint32_t addr, qosa_uint8_t *data, qosa_uint32_t len);
typedef qosa_built_nor_error_e (* _api_qosa_builtin_flash_write_t)(qosa_uint32_t addr, qosa_uint8_t *data, qosa_uint32_t len);

extern qosa_built_nor_error_e qosa_builtin_flash_erase(qosa_uint32_t addr, qosa_uint32_t len);
typedef qosa_built_nor_error_e (* _api_qosa_builtin_flash_erase_t)(qosa_uint32_t addr, qosa_uint32_t len);

extern qosa_int32_t qosa_aud_driver_register(osa_aud_drv_cb_t *ops);
typedef qosa_int32_t (* _api_qosa_aud_driver_register_t)(osa_aud_drv_cb_t *ops);

extern qosa_aud_errcode_e qosa_aud_set_volume(qosa_aud_vol_level_e volume);
typedef qosa_aud_errcode_e (* _api_qosa_aud_set_volume_t)(qosa_aud_vol_level_e volume);

extern qosa_aud_vol_level_e qosa_aud_get_volume(void);
typedef qosa_aud_vol_level_e (* _api_qosa_aud_get_volume_t)(void);

extern qosa_aud_errcode_e qosa_aud_apply_pcm_gain(qosa_bool_t en, qosa_int16_t level);
typedef qosa_aud_errcode_e (* _api_qosa_aud_apply_pcm_gain_t)(qosa_bool_t en, qosa_int16_t level);

extern qosa_int32_t qosa_aud_driver_setPinEn(qosa_uint32_t enMask);
typedef qosa_int32_t (* _api_qosa_aud_driver_setPinEn_t)(qosa_uint32_t enMask);

extern qosa_aud_errcode_e qosa_aud_bind_pa_cb(qosa_aud_pa_callback_t callback);
typedef qosa_aud_errcode_e (* _api_qosa_aud_bind_pa_cb_t)(qosa_aud_pa_callback_t callback);

extern qosa_aud_errcode_e qosa_aud_output_ctrl(qosa_audio_output_type_e type, void * ctrl);
typedef qosa_aud_errcode_e (* _api_qosa_aud_output_ctrl_t)(qosa_audio_output_type_e type, void * ctrl);

extern qosa_aud_errcode_e qosa_audio_set_stream_iface(qosa_dai_t *iface_t);
typedef qosa_aud_errcode_e (* _api_qosa_audio_set_stream_iface_t)(qosa_dai_t *iface_t);

extern qosa_aud_errcode_e qosa_audio_get_stream_iface(qosa_dai_t *iface_t);
typedef qosa_aud_errcode_e (* _api_qosa_audio_get_stream_iface_t)(qosa_dai_t *iface_t);

extern qosa_aud_errcode_e  qosa_tts_open(qosa_tts_cfg_t *config);
typedef qosa_aud_errcode_e (*_api_qosa_tts_open_t)(qosa_tts_cfg_t *config);

extern qosa_aud_errcode_e  qosa_tts_play(qosa_tts_encoding_e code, const char *text, qosa_uint32_t length);
typedef qosa_aud_errcode_e (*_api_qosa_tts_play_t)(qosa_tts_encoding_e code, const char *text, qosa_uint32_t length);

extern qosa_aud_errcode_e  qosa_tts_close(qosa_bool_t keep_resource);
typedef qosa_aud_errcode_e (*_api_qosa_tts_close_t)(qosa_bool_t keep_resource);

extern int qosa_tts_param_cfg_get(qosa_tts_config_e type);
typedef int (*_api_qosa_tts_param_cfg_get_t)(qosa_tts_config_e type);

extern qosa_aud_errcode_e  qosa_tts_param_cfg_set(qosa_tts_config_e type, int val);
typedef qosa_aud_errcode_e (*_api_qosa_tts_param_cfg_set_t)(qosa_tts_config_e type, int val);

extern qosa_aud_errcode_e qosa_tts_resource_init(qosa_int8_t *path_name, qosa_uint8_t *ram_addr);
typedef qosa_aud_errcode_e (*_api_qosa_tts_resource_init_t)(qosa_int8_t *path_name, qosa_uint8_t *ram_addr);

extern qosa_i2c_error_e qosa_i2c_init(qosa_i2c_channel_e i2c_no, qosa_i2c_mode_e Mode);
typedef qosa_i2c_error_e (* _api_qosa_i2c_init_t)(qosa_i2c_channel_e i2c_no, qosa_i2c_mode_e Mode);

extern qosa_i2c_error_e qosa_i2c_ioctl(qosa_i2c_channel_e i2c_no, qosa_i2c_ioctl_cmd_e cmd, void *arg);
typedef qosa_i2c_error_e (* _api_qosa_i2c_ioctl_t)(qosa_i2c_channel_e i2c_no, qosa_i2c_ioctl_cmd_e cmd, void *arg);

extern qosa_i2c_error_e qosa_i2c_write(qosa_i2c_channel_e i2c_no, qosa_uint8_t slave, qosa_uint16_t addr, qosa_uint8_t *data, qosa_uint32_t length);
typedef qosa_i2c_error_e (* _api_qosa_i2c_write_t)(qosa_i2c_channel_e i2c_no, qosa_uint8_t slave, qosa_uint16_t addr, qosa_uint8_t *data, qosa_uint32_t length);

extern qosa_i2c_error_e qosa_i2c_read(qosa_i2c_channel_e i2c_no, qosa_uint8_t slave, qosa_uint16_t addr, qosa_uint8_t *buf, qosa_uint32_t length);
typedef qosa_i2c_error_e (* _api_qosa_i2c_read_t)(qosa_i2c_channel_e i2c_no, qosa_uint8_t slave, qosa_uint16_t addr, qosa_uint8_t *buf, qosa_uint32_t length);

extern qosa_i2c_error_e qosa_i2c_deinit(qosa_i2c_channel_e i2c_no);
typedef qosa_i2c_error_e (* _api_qosa_i2c_deinit_t)(qosa_i2c_channel_e i2c_no);

extern qosa_uart_error_e qosa_uart_ioctl(qosa_uart_port_number_e port, qosa_uart_ioctl_cmd_e cmd, void *arg);
typedef qosa_uart_error_e (* _api_qosa_uart_ioctl_t)(qosa_uart_port_number_e port, qosa_uart_ioctl_cmd_e cmd, void *arg);

extern qosa_uart_error_e qosa_uart_open(qosa_uart_port_number_e port);
typedef qosa_uart_error_e (* _api_qosa_uart_open_t)(qosa_uart_port_number_e port);

extern qosa_uart_error_e qosa_uart_close(qosa_uart_port_number_e port);
typedef qosa_uart_error_e (* _api_qosa_uart_close_t)(qosa_uart_port_number_e port);

extern int qosa_uart_write(qosa_uart_port_number_e port, unsigned char *data, unsigned int data_len);
typedef int (* _api_qosa_uart_write_t)(qosa_uart_port_number_e port, unsigned char *data, unsigned int data_len);

extern int qosa_uart_read(qosa_uart_port_number_e port, unsigned char *data, unsigned int data_len);
typedef int (* _api_qosa_uart_read_t)(qosa_uart_port_number_e port, unsigned char *data, unsigned int data_len);

extern int qosa_uart_read_available(qosa_uart_port_number_e port);
typedef int (* _api_qosa_uart_read_available_t)(qosa_uart_port_number_e port);

extern int qosa_uart_write_available(qosa_uart_port_number_e port);
typedef int (* _api_qosa_uart_write_available_t)(qosa_uart_port_number_e port);

extern qosa_uart_error_e qosa_uart_register_cb(qosa_uart_port_number_e port, qosa_uart_status_monitor_t *arvg);
typedef qosa_uart_error_e (* _api_qosa_uart_register_cb_t)(qosa_uart_port_number_e port, qosa_uart_status_monitor_t *arvg);

extern qosa_spi_errcode_e qosa_spi_init(qosa_spi_port_e port, qosa_spi_transmit_mode_e mode, qosa_spi_clk_e clk);
typedef qosa_spi_errcode_e (* _api_qosa_spi_init_t)(qosa_spi_port_e port, qosa_spi_transmit_mode_e mode, qosa_spi_clk_e clk);

extern qosa_spi_errcode_e qosa_spi_deinit(qosa_spi_port_e port);
typedef qosa_uart_error_e (* _api_qosa_spi_deinit_t)(qosa_spi_port_e port);

extern qosa_spi_errcode_e qosa_spi_write(qosa_spi_port_e port, void *txData, qosa_uint32_t length);
typedef qosa_uart_error_e (* _api_qosa_spi_write_t)(qosa_spi_port_e port, void *txData, qosa_uint32_t length);

extern qosa_spi_errcode_e qosa_spi_read(qosa_spi_port_e port, void *rxData, qosa_uint32_t length);
typedef qosa_uart_error_e (* _api_qosa_spi_read_t)(qosa_spi_port_e port, void *rxData, qosa_uint32_t length);

extern qosa_spi_errcode_e qosa_spi_write_read(qosa_spi_port_e port, void *rxData, void *txData, qosa_uint32_t length);
typedef qosa_uart_error_e (* _api_qosa_spi_write_read_t)(qosa_spi_port_e port, void *rxData, void *txData, qosa_uint32_t length);

extern qosa_spi_errcode_e qosa_spi_ioctl(qosa_spi_port_e port, qosa_spi_ioctl_cmd_e cmd, void *arg);
typedef qosa_uart_error_e (* _api_qosa_spi_ioctl_t)(qosa_spi_port_e port, qosa_spi_ioctl_cmd_e cmd, void *arg);

extern qosa_camera_error_e qosa_camera_init(qosa_camera_channel_e camera_no, qosa_uint32_t width, qosa_uint32_t height, qosa_camera_output_format_opt_e image_output_format);
typedef qosa_camera_error_e (* _api_qosa_camera_init_t)(qosa_camera_channel_e camera_no, qosa_uint32_t width, qosa_uint32_t height, qosa_camera_output_format_opt_e image_output_format);

extern qosa_camera_error_e qosa_camera_deinit(qosa_camera_channel_e camera_no);
typedef qosa_camera_error_e (* _api_qosa_camera_deinit_t)(qosa_camera_channel_e camera_no);

extern qosa_camera_error_e qosa_camera_start(qosa_camera_channel_e camera_no);
typedef qosa_camera_error_e (* _api_qosa_camera_start_t)(qosa_camera_channel_e camera_no);

extern qosa_camera_error_e qosa_camera_stop(qosa_camera_channel_e camera_no);
typedef qosa_camera_error_e (* _api_qosa_camera_stop_t)(qosa_camera_channel_e camera_no);

extern qosa_camera_error_e qosa_camera_preview_image(qosa_camera_channel_e camera_no, qosa_uint8_t **imageBuf);
typedef qosa_camera_error_e (* _api_qosa_camera_preview_image_t)(qosa_camera_channel_e camera_no, qosa_uint8_t **imageBuf);

extern qosa_camera_error_e qosa_camera_free_preview_image(qosa_camera_channel_e camera_no, qosa_uint8_t *imageBuf);
typedef qosa_camera_error_e (* _api_qosa_camera_free_preview_image_t)(qosa_camera_channel_e camera_no, qosa_uint8_t *imageBuf);

extern qosa_camera_error_e qosa_camera_i2c_write(qosa_camera_channel_e camera_no, qosa_uint8_t addr, qosa_uint8_t *data, qosa_uint32_t len);
typedef qosa_camera_error_e (* _api_qosa_camera_i2c_write_t)(qosa_camera_channel_e camera_no, qosa_uint8_t addr, qosa_uint8_t *data, qosa_uint32_t len);

extern qosa_camera_error_e qosa_camera_i2c_read(qosa_camera_channel_e camera_no, qosa_uint8_t addr, qosa_uint8_t *data, qosa_uint32_t len);
typedef qosa_camera_error_e (* _api_qosa_camera_i2c_read_t)(qosa_camera_channel_e camera_no, qosa_uint8_t addr, qosa_uint8_t *data, qosa_uint32_t len);

extern qosa_camera_error_e qosa_camera_ioctl(qosa_camera_channel_e camera_no, qosa_camera_ioctl_cmd_e cmd, void *arg);
typedef qosa_camera_error_e (* _api_qosa_camera_ioctl_t)(qosa_camera_channel_e camera_no, qosa_camera_ioctl_cmd_e cmd, void *arg);

extern qosa_camera_error_e qosa_camera_convert_to_jpeg(qosa_uint8_t quality, qosa_camera_crop_info_t *crop_info, qosa_uint8_t *pInBuf, qosa_uint32_t uInWidth, qosa_uint32_t uInHeight, qosa_uint8_t *pOutBuf, qosa_uint32_t *pOutLength, qosa_camera_output_format_opt_e image_format);
typedef qosa_camera_error_e (* _api_qosa_camera_convert_to_jpeg_t)(qosa_uint8_t quality, qosa_camera_crop_info_t *crop_info, qosa_uint8_t *pInBuf, qosa_uint32_t uInWidth, qosa_uint32_t uInHeight, qosa_uint8_t *pOutBuf, qosa_uint32_t *pOutLength, qosa_camera_output_format_opt_e image_format);

extern qosa_camera_error_e qosa_camera_decode_jpeg(qosa_uint8_t *pInBuf, qosa_uint32_t uInLength, qosa_uint8_t *pOutBuf, qosa_uint32_t *pOutLength, qosa_uint32_t *pOutWidth, qosa_uint32_t *pOutHeight, qosa_camera_output_format_opt_e image_format);
typedef qosa_camera_error_e (* _api_qosa_camera_decode_jpeg_t)(qosa_uint8_t *pInBuf, qosa_uint32_t uInLength, qosa_uint8_t *pOutBuf, qosa_uint32_t *pOutLength, qosa_uint32_t *pOutWidth, qosa_uint32_t *pOutHeight, qosa_camera_output_format_opt_e image_format);

extern qosa_lcd_error_e qosa_lcd_init(qosa_lcd_channel_e lcd_no);
typedef qosa_lcd_error_e (* _api_qosa_lcd_init_t)(qosa_lcd_channel_e lcd_no);

extern qosa_lcd_error_e qosa_lcd_deinit(qosa_lcd_channel_e lcd_no);
typedef qosa_lcd_error_e (* _api_qosa_lcd_deinit_t)(qosa_lcd_channel_e lcd_no);

extern qosa_lcd_error_e qosa_lcd_clear_screen(qosa_lcd_channel_e lcd_no, qosa_uint16_t color);
typedef qosa_lcd_error_e (* _api_qosa_lcd_clear_screen_t)(qosa_lcd_channel_e lcd_no, qosa_uint16_t color);

extern qosa_lcd_error_e qosa_lcd_display_on(qosa_lcd_channel_e lcd_no);
typedef qosa_lcd_error_e (* _api_qosa_lcd_display_on_t)(qosa_lcd_channel_e lcd_no);

extern qosa_lcd_error_e qosa_lcd_display_off(qosa_lcd_channel_e lcd_no);
typedef qosa_lcd_error_e (* _api_qosa_lcd_display_off_t)(qosa_lcd_channel_e lcd_no);

extern qosa_lcd_error_e qosa_lcd_write_cmd(qosa_lcd_channel_e lcd_no, qosa_uint8_t cmd);
typedef qosa_lcd_error_e (* _api_qosa_lcd_write_cmd_t)(qosa_lcd_channel_e lcd_no, qosa_uint8_t cmd);

extern qosa_lcd_error_e qosa_lcd_write_cmd_data(qosa_lcd_channel_e lcd_no, qosa_uint8_t cmd_data);
typedef qosa_lcd_error_e (* _api_qosa_lcd_write_cmd_data_t)(qosa_lcd_channel_e lcd_no, qosa_uint8_t cmd_data);

extern qosa_lcd_error_e qosa_lcd_flush_cmd(qosa_lcd_channel_e lcd_no);
typedef qosa_lcd_error_e (* _api_qosa_lcd_flush_cmd_t)(qosa_lcd_channel_e lcd_no);

extern qosa_lcd_error_e qosa_lcd_read_cmd_data(qosa_lcd_channel_e lcd_no, qosa_uint8_t cmd, qosa_uint8_t *data, qosa_uint32_t length, qosa_uint32_t dummyCycleLen);
typedef qosa_lcd_error_e (* _api_qosa_lcd_read_cmd_data_t)(qosa_lcd_channel_e lcd_no, qosa_uint8_t cmd, qosa_uint8_t *data, qosa_uint32_t length, qosa_uint32_t dummyCycleLen);

extern qosa_lcd_error_e qosa_lcd_write(qosa_lcd_channel_e lcd_no, qosa_uint8_t *buffer, qosa_uint16_t start_x, qosa_uint16_t start_y, qosa_uint16_t end_x, qosa_uint16_t end_y);
typedef qosa_lcd_error_e (* _api_qosa_lcd_write_t)(qosa_lcd_channel_e lcd_no, qosa_uint8_t *buffer, qosa_uint16_t start_x, qosa_uint16_t start_y, qosa_uint16_t end_x, qosa_uint16_t end_y);

extern qosa_lcd_error_e qosa_lcd_ioctl(qosa_lcd_channel_e lcd_no, qosa_lcd_ioctl_cmd_e cmd, void *arg);
typedef qosa_lcd_error_e (* _api_qosa_lcd_ioctl_t)(qosa_lcd_channel_e lcd_no, qosa_lcd_ioctl_cmd_e cmd, void *arg);

extern qosa_decoder_errcode_e qosa_qr_decoder_init(void);
typedef qosa_decoder_errcode_e (* _api_qosa_qr_decoder_init_t)(void);

extern qosa_decoder_errcode_e qosa_qr_get_decoder_result(qosa_decoder_type_e *type, qosa_uint8_t *result);
typedef qosa_decoder_errcode_e (* _api_qosa_qr_get_decoder_result_t)(qosa_decoder_type_e *type, qosa_uint8_t *result);

extern qosa_decoder_errcode_e qosa_qr_image_decoder(qosa_camera_channel_e camera_no, qosa_uint8_t *img_buffer, qosa_uint32_t width, qosa_uint32_t height);
typedef qosa_decoder_errcode_e (* _api_qosa_qr_image_decoder_t)(qosa_camera_channel_e camera_no, qosa_uint8_t *img_buffer, qosa_uint32_t width, qosa_uint32_t height);

extern qosa_decoder_errcode_e qosa_destroy_decoder(void);
typedef qosa_decoder_errcode_e (* _api_qosa_destroy_decoder_t)(void);

extern qosa_decoder_errcode_e qosa_get_decoder_version(qosa_uint8_t *version);
typedef qosa_decoder_errcode_e (* _api_qosa_get_decoder_version_t)(qosa_uint8_t *version);

extern qosa_decoder_errcode_e qosa_set_decode_mirror(qosa_decode_mode_e decode_mode);
typedef qosa_decoder_errcode_e (* _api_qosa_set_decode_mirror_t)(qosa_decode_mode_e decode_mode);

extern qosa_usb_error_e qosa_usb_init(void);
typedef qosa_usb_error_e (* _api_qosa_usb_init_t)(void);

extern qosa_usb_error_e qosa_usb_deinit(void);
typedef qosa_usb_error_e (* _api_qosa_usb_deinit_t)(void);

extern qosa_usb_error_e qosa_usb_bind_vbus_cb(qosa_usb_vbus_cb vbus_callback);
typedef qosa_usb_error_e (* _api_qosa_usb_bind_vbus_cb_t)(qosa_usb_vbus_cb vbus_callback);

extern qosa_usb_vbus_state_e qosa_usb_get_vbus_state(void);
typedef qosa_usb_vbus_state_e (* _api_qosa_usb_get_vbus_state_t)(void);

extern qosa_usb_state_e qosa_usb_get_connect_state(void);
typedef qosa_usb_state_e (* _api_qosa_usb_get_connect_state_t)(void);

extern qosa_uint32_t qosa_mfd_get_cfg(qosa_mfd_e feature);
typedef qosa_uint32_t (* _api_qosa_mfd_get_cfg_t)(qosa_mfd_e feature);

extern void qosa_mfd_set_cfg(qosa_mfd_e feature, qosa_uint32_t value);
typedef void (* _api_qosa_mfd_set_cfg_t)(qosa_mfd_e feature, qosa_uint32_t value);

extern qosa_common_error_e qosa_ms_register_recycle(qosa_task_t taskid, ms_sender_recycle_callback_ptr callback);
typedef qosa_common_error_e (* _api_qosa_ms_register_recycle_t)(qosa_task_t taskid, ms_sender_recycle_callback_ptr callback);

extern qosa_common_error_e qosa_ms_cnf_signal_parsing(void* cnf_signal);
typedef qosa_common_error_e (* _api_qosa_ms_cnf_signal_parsing_t)(void* cnf_signal);

extern qosa_datacall_errno_e qosa_datacall_attach(qosa_uint8_t simid, qosa_uint8_t method, datacall_callback_cb_ptr cb, void* ctx);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_attach_t)(qosa_uint8_t simid, qosa_uint8_t method, datacall_callback_cb_ptr cb, void* ctx);

extern qosa_bool_t qosa_datacall_wait_attached(qosa_uint8_t simid, qosa_uint32_t timeout);
typedef qosa_bool_t (* _api_qosa_datacall_wait_attached_t)(qosa_uint8_t simid, qosa_uint32_t timeout);

extern qosa_datacall_conn_t qosa_datacall_conn_new(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_conn_type_e conn_type);
typedef qosa_datacall_conn_t (* _api_qosa_datacall_conn_new_t)(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_conn_type_e conn_type);

extern qosa_datacall_errno_e qosa_datacall_start(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_start_t)(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time);

extern qosa_datacall_errno_e qosa_datacall_start_async(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time, datacall_callback_cb_ptr cb, void* ctx);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_start_async_t)(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time, datacall_callback_cb_ptr cb, void* ctx);

extern qosa_datacall_errno_e qosa_datacall_stop(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_stop_t)(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time);

extern qosa_datacall_errno_e qosa_datacall_stop_async(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time, datacall_callback_cb_ptr cb, void* ctx);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_stop_async_t)(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time, datacall_callback_cb_ptr cb, void* ctx);

extern qosa_bool_t qosa_datacall_get_status(qosa_datacall_conn_t conn);
typedef qosa_bool_t (* _api_qosa_datacall_get_status_t)(qosa_datacall_conn_t conn);

extern qosa_datacall_errno_e qosa_datacall_get_ip_info(qosa_datacall_conn_t conn, qosa_datacall_ip_info_t* info);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_get_ip_info_t)(qosa_datacall_conn_t conn, qosa_datacall_ip_info_t* info);

extern qosa_datacall_errno_e qosa_datacall_get_dns_addr(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_dns_t* dns);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_get_dns_addr_t)(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_dns_t* dns);

extern qosa_datacall_errno_e qosa_datacall_set_dns_addr(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_dns_t* dns);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_set_dns_addr_t)(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_dns_t* dns);

extern qosa_bool_t qosa_datacall_is_defined(qosa_uint8_t simid, qosa_uint8_t pdpid);
typedef qosa_bool_t (* _api_qosa_datacall_is_defined_t)(qosa_uint8_t simid, qosa_uint8_t pdpid);

extern qosa_datacall_errno_e qosa_datacall_set_pdp_context(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_context_t* pdp_ctx);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_set_pdp_context_t)(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_context_t* pdp_ctx);

extern qosa_datacall_errno_e qosa_datacall_delete_pdp_context(qosa_uint8_t simid, qosa_uint8_t pdpid);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_delete_pdp_context_t)(qosa_uint8_t simid, qosa_uint8_t pdpid);

extern qosa_datacall_errno_e qosa_datacall_get_pdp_context(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_context_t* pdp_ctx);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_get_pdp_context_t)(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_context_t* pdp_ctx);

extern qosa_datacall_errno_e qosa_datacall_set_pdp_auth(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_auth_context_t* auth_ctx);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_set_pdp_auth_t)(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_auth_context_t* auth_ctx);

extern qosa_datacall_errno_e qosa_datacall_get_pdp_auth(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_auth_context_t* auth_ctx);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_get_pdp_auth_t)(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_auth_context_t* auth_ctx);

extern qosa_datacall_errno_e qosa_datacall_set_pdp_qos(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_qos_profile_t* qos_profile);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_set_pdp_qos_t)(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_qos_profile_t* qos_profile);

extern qosa_datacall_errno_e qosa_datacall_get_pdp_qos(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_qos_profile_t* qos_profile);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_get_pdp_qos_t)(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_qos_profile_t* qos_profile);

extern qosa_datacall_errno_e qosa_datacall_get_traffic_statistics(qosa_uint8_t simid, qosa_datacall_traffic_statistics_t* traffic_statistics);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_get_traffic_statistics_t)(qosa_uint8_t simid, qosa_datacall_traffic_statistics_t* traffic_statistics);

extern qosa_datacall_errno_e qosa_datacall_clear_traffic_statistics(qosa_uint8_t simid);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_clear_traffic_statistics_t)(qosa_uint8_t simid);

extern qosa_datacall_errno_e qosa_datacall_save_traffic_statistics(qosa_uint8_t simid);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_save_traffic_statistics_t)(qosa_uint8_t simid);

extern qosa_datacall_errno_e qosa_datacall_set_pdp_timer(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t procedure, qosa_datacall_pdp_timer_t* pdp_timer);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_set_pdp_timer_t)(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t procedure, qosa_datacall_pdp_timer_t* pdp_timer);

extern qosa_datacall_errno_e qosa_datacall_get_pdp_timer(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t procedure, qosa_datacall_pdp_timer_t* pdp_timer);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_get_pdp_timer_t)(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t procedure, qosa_datacall_pdp_timer_t* pdp_timer);

extern qosa_datacall_errno_e qosa_datacall_get_dynamic_pdp_ctx(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_dynamic_context_t *pdp_dynamic_ctx);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_get_dynamic_pdp_ctx_t)(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_dynamic_context_t *pdp_dynamic_ctx);

extern qosa_datacall_errno_e qosa_datacall_set_pdpreact_mode(qosa_uint8_t simid, qosa_bool_t enable);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_set_pdpreact_mode_t)(qosa_uint8_t simid, qosa_bool_t enable);

extern qosa_datacall_errno_e qosa_datacall_get_pdpreact_mode(qosa_uint8_t simid, qosa_bool_t *enable);
typedef qosa_datacall_errno_e (* _api_qosa_datacall_get_pdpreact_mode_t)(qosa_uint8_t simid, qosa_bool_t *enable);

extern qosa_dev1_error_e qosa_dev_set_cfun(qosa_uint8_t simid, qosa_uint8_t func, dev_callback_ptr cb, void *ctx);
typedef qosa_dev1_error_e (* _api_qosa_dev_set_cfun_t)(qosa_uint8_t simid, qosa_uint8_t func, dev_callback_ptr cb, void *ctx);

extern qosa_dev1_error_e qosa_dev_get_cfun(qosa_uint8_t simid, qosa_uint8_t *func);
typedef qosa_dev1_error_e (* _api_qosa_dev_get_cfun_t)(qosa_uint8_t simid, qosa_uint8_t *func);

extern qosa_dev1_error_e qosa_dev_set_airplanecontrol_config(qosa_uint8_t simid, qosa_uint8_t airplane_ctl_enable);
typedef qosa_dev1_error_e (* _api_qosa_dev_set_airplanecontrol_config_t)(qosa_uint8_t simid, qosa_uint8_t airplane_ctl_enable);

extern qosa_dev1_error_e qosa_dev_get_airplanecontrol_config(qosa_uint8_t simid, qosa_dev_airplanecontrol_param_t *config);
typedef qosa_dev1_error_e (* _api_qosa_dev_get_airplanecontrol_config_t)(qosa_uint8_t simid, qosa_dev_airplanecontrol_param_t *config);

extern qosa_nw_err_e qosa_nw_get_reg_status(qosa_uint8_t simid, qosa_uint8_t *cs_status, qosa_uint8_t *ps_status);
typedef qosa_nw_err_e (* _api_qosa_nw_get_reg_status_t)(qosa_uint8_t simid, qosa_uint8_t *cs_status, qosa_uint8_t *ps_status);

extern qosa_nw_rat_e qosa_nw_get_current_rat(qosa_uint8_t simid);
typedef qosa_nw_rat_e (* _api_qosa_nw_get_current_rat_t)(qosa_uint8_t simid);

extern qosa_nw_rat_e qosa_nw_act2rat(qosa_nw_act_e act);
typedef qosa_nw_rat_e (* _api_qosa_nw_act2rat_t)(qosa_nw_act_e act);

extern qosa_nw_err_e qosa_nw_get_rat_mode_from_nv(qosa_uint8_t simid, qosa_uint8_t *rat_mode);
typedef qosa_nw_err_e (* _api_qosa_nw_get_rat_mode_from_nv_t)(qosa_uint8_t simid, qosa_uint8_t *rat_mode);

extern qosa_nw_err_e qosa_nw_get_supported_rat_mode(qosa_uint8_t simid, qosa_uint8_t *rat_mode_ability);
typedef qosa_nw_err_e (* _api_qosa_nw_get_supported_rat_mode_t)(qosa_uint8_t simid, qosa_uint8_t *rat_mode_ability);

extern qosa_nw_err_e qosa_nw_set_rat_mode(qosa_uint8_t simid, qosa_uint8_t rat_mode, qosa_bool_t effect, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_rat_mode_t)(qosa_uint8_t simid, qosa_uint8_t rat_mode, qosa_bool_t effect, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_rat_order(qosa_uint8_t simid, qosa_uint8_t *rat_order);
typedef qosa_nw_err_e (* _api_qosa_nw_get_rat_order_t)(qosa_uint8_t simid, qosa_uint8_t *rat_order);

extern qosa_nw_err_e qosa_nw_set_rat_order(qosa_uint8_t simid, qosa_uint8_t *rat_order, qosa_bool_t effect, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_rat_order_t)(qosa_uint8_t simid, qosa_uint8_t *rat_order, qosa_bool_t effect, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_scell_info(qosa_uint8_t simid, qosa_nw_scell_info_t *scell);
typedef qosa_nw_err_e (* _api_qosa_nw_get_scell_info_t)(qosa_uint8_t simid, qosa_nw_scell_info_t *scell);

extern qosa_nw_err_e qosa_nw_get_ncell_basic(qosa_uint8_t simid, qosa_nw_ncell_info_t *ncell);
typedef qosa_nw_err_e (* _api_qosa_nw_get_ncell_basic_t)(qosa_uint8_t simid, qosa_nw_ncell_info_t *ncell);

extern qosa_nw_err_e qosa_nw_get_ncell_ext(qosa_uint8_t simid, qosa_nw_ncell_ext_config_t *config, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_get_ncell_ext_t)(qosa_uint8_t simid, qosa_nw_ncell_ext_config_t *config, nw_callback_t cb, void *ctx);

extern char *qosa_nw_plmn_stoa(qosa_plmn_t *plmn, char *plmn_str_buf);
typedef char *(* _api_qosa_nw_plmn_stoa_t)(qosa_plmn_t *plmn, char *plmn_str_buf);

extern int qosa_nw_plmn_atos(const char *plmn_str, qosa_plmn_t *plmn);
typedef int (* _api_qosa_nw_plmn_atos_t)(const char *plmn_str, qosa_plmn_t *plmn);

char *qosa_nw_oper_name_stoa(qosa_nw_oper_name_t *oper_name, char *oper_name_str_buf);
typedef char *(* _api_qosa_nw_oper_name_stoa_t)(qosa_nw_oper_name_t *oper_name, char *oper_name_str_buf);

extern qosa_nw_err_e qosa_nw_get_oper_name(qosa_uint8_t simid, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);
typedef qosa_nw_err_e (* _api_qosa_nw_get_oper_name_t)(qosa_uint8_t simid, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);

extern qosa_nw_err_e qosa_nw_get_cfg_oper_name(qosa_plmn_t *plmn, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);
typedef qosa_nw_err_e (* _api_qosa_nw_get_cfg_oper_name_t)(qosa_plmn_t *plmn, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);

extern qosa_nw_err_e qosa_nw_each_cfg_oper_name(qosa_uint32_t index, qosa_plmn_t *plmn, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);
typedef qosa_nw_err_e (* _api_qosa_nw_each_cfg_oper_name_t)(qosa_uint32_t index, qosa_plmn_t *plmn, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);

extern qosa_nw_err_e qosa_nw_query_supp_operator(qosa_uint8_t simid, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_query_supp_operator_t)(qosa_uint8_t simid, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_auto_register(qosa_uint8_t simid, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_auto_register_t)(qosa_uint8_t simid, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_manual_register(qosa_uint8_t               simid,
    qosa_nw_oper_name_format_e format,
    char                     *plmn_str,
    qosa_nw_act_e              act,
    qosa_bool_t                fail_to_auto,
    nw_callback_t             cb,
    void                     *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_manual_register_t)(qosa_uint8_t               simid,
    qosa_nw_oper_name_format_e format,
    char                     *plmn_str,
    qosa_nw_act_e              act,
    qosa_bool_t                fail_to_auto,
    nw_callback_t             cb,
    void                     *ctx);

extern qosa_nw_err_e qosa_nw_deregister(qosa_uint8_t simid, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_deregister_t)(qosa_uint8_t simid, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_cfg_band(qosa_uint8_t simid, qosa_nw_band_t *cfg_band);
typedef qosa_nw_err_e (* _api_qosa_nw_get_cfg_band_t)(qosa_uint8_t simid, qosa_nw_band_t *cfg_band);

extern qosa_nw_err_e qosa_nw_get_support_band(qosa_uint8_t simid, qosa_nw_band_t *support_band);
typedef qosa_nw_err_e (* _api_qosa_nw_get_support_band_t)(qosa_uint8_t simid, qosa_nw_band_t *support_band);

extern qosa_nw_err_e qosa_nw_set_cfg_band(qosa_uint8_t simid, const qosa_nw_band_t *cfg_band, qosa_bool_t effect, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_cfg_band_t)(qosa_uint8_t simid, const qosa_nw_band_t *cfg_band, qosa_bool_t effect, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_rrc_conn_info(qosa_uint8_t simid, qosa_nw_rrc_conn_info_t *conn_info);
typedef qosa_nw_err_e (* _api_qosa_nw_get_rrc_conn_info_t)(qosa_uint8_t simid, qosa_nw_rrc_conn_info_t *conn_info);

extern qosa_nw_err_e qosa_nw_trigger_freq_scan(qosa_uint8_t simid, qosa_nw_freq_scan_t *config, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_trigger_freq_scan_t)(qosa_uint8_t simid, qosa_nw_freq_scan_t *config, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_srv_domain(qosa_uint8_t simid, qosa_uint8_t *domain);
typedef qosa_nw_err_e (* _api_qosa_nw_get_srv_domain_t)(qosa_uint8_t simid, qosa_uint8_t *domain);

extern qosa_nw_err_e qosa_nw_set_srv_domain(qosa_uint8_t simid, qosa_uint8_t domain, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_srv_domain_t)(qosa_uint8_t simid, qosa_uint8_t domain, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_voice_domain(qosa_uint8_t simid, qosa_uint8_t *domain);
typedef qosa_nw_err_e (* _api_qosa_nw_get_voice_domain_t)(qosa_uint8_t simid, qosa_uint8_t *domain);

extern qosa_nw_err_e qosa_nw_set_voice_domain(qosa_uint8_t simid, qosa_uint8_t domain, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_voice_domain_t)(qosa_uint8_t simid, qosa_uint8_t domain, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_ue_usage_setting(qosa_uint8_t simid, qosa_uint8_t *setting);
typedef qosa_nw_err_e (* _api_qosa_nw_get_ue_usage_setting_t)(qosa_uint8_t simid, qosa_uint8_t *setting);

extern qosa_nw_err_e qosa_nw_set_ue_usage_setting(qosa_uint8_t simid, qosa_uint8_t setting, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_ue_usage_setting_t)(qosa_uint8_t simid, qosa_uint8_t setting, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_roaming_pref(qosa_uint8_t simid, qosa_uint8_t *setting);
typedef qosa_nw_err_e (* _api_qosa_nw_get_roaming_pref_t)(qosa_uint8_t simid, qosa_uint8_t *setting);

extern qosa_nw_err_e qosa_nw_set_roaming_pref(qosa_uint8_t simid, qosa_uint8_t setting, qosa_uint8_t effect, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_roaming_pref_t)(qosa_uint8_t simid, qosa_uint8_t setting,qosa_uint8_t effect, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_freq_lock(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t opcode, qosa_nw_freq_lock_list_t *freq_lock, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_freq_lock_t)(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t opcode, qosa_nw_freq_lock_list_t *freq_lock, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_freq_lock_config(qosa_uint8_t simid, qosa_uint8_t rat, qosa_nw_freq_lock_list_t *freq_lock);
typedef qosa_nw_err_e (* _api_qosa_nw_get_freq_lock_config_t)(qosa_uint8_t simid, qosa_uint8_t rat, qosa_nw_freq_lock_list_t *freq_lock);

extern qosa_nw_err_e qosa_nw_nssai_set_default_config(qosa_uint8_t simid, qosa_uint8_t length, const char *nssai_data);
typedef qosa_nw_err_e (* _api_qosa_nw_nssai_set_default_config_t)(qosa_uint8_t simid, qosa_uint8_t length, const char *nssai_data);

extern qosa_nw_err_e qosa_nw_nssai_get_default_config(qosa_uint8_t simid, qosa_uint8_t *length, char *nssai_data);
typedef qosa_nw_err_e (* _api_qosa_nw_nssai_get_default_config_t)(qosa_uint8_t simid, qosa_uint8_t *length, char *nssai_data);

extern qosa_nw_err_e qosa_nw_nssai_clear_default_config(qosa_uint8_t simid);
typedef qosa_nw_err_e (* _api_qosa_nw_nssai_clear_default_config_t)(qosa_uint8_t simid);

extern qosa_nw_err_e qosa_nw_nssai_get_nssai(qosa_uint8_t simid, qosa_nw_nssai_type_e type, qosa_plmn_t *plmn, qosa_nw_nssai_list_t *list);
typedef qosa_nw_err_e (* _api_qosa_nw_nssai_get_nssai_t)(qosa_uint8_t simid, qosa_nw_nssai_type_e type, qosa_plmn_t *plmn, qosa_nw_nssai_list_t *list);

extern qosa_nw_err_e qosa_nw_set_rrc_fast_release(qosa_uint8_t simid, qosa_nw_rrc_fast_release_t *fast_release, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_rrc_fast_release_t)(qosa_uint8_t simid, qosa_nw_rrc_fast_release_t *fast_release, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_rrc_fast_release(qosa_uint8_t simid, qosa_nw_rrc_fast_release_t *fast_release);
typedef qosa_nw_err_e (* _api_qosa_nw_get_rrc_fast_release_t)(qosa_uint8_t simid, qosa_nw_rrc_fast_release_t *fast_release);

extern qosa_nw_err_e qosa_nw_rrc_fast_release(qosa_uint8_t simid);
typedef qosa_nw_err_e (* _api_qosa_nw_rrc_fast_release_t)(qosa_uint8_t simid);

extern qosa_nw_err_e qosa_nw_get_reject_cause_string(qosa_nw_reject_type_e type,qosa_uint16_t reject_cause,char *cause_string);
typedef qosa_nw_err_e (* _api_qosa_nw_get_reject_cause_string_t)(qosa_nw_reject_type_e type,qosa_uint16_t reject_cause,char *cause_string);

extern qosa_nw_err_e qosa_nw_get_reject_cause_event_info(qosa_uint8_t simid,qosa_nw_reject_info_t *reject_info);
typedef qosa_nw_err_e (* _api_qosa_nw_get_reject_cause_event_info_t)(qosa_uint8_t simid,qosa_nw_reject_info_t *reject_info);

extern qosa_nw_err_e qosa_nw_set_jamm_detect_func(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *jdc_func, nw_callback_t cb, void* ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_jamm_detect_func_t)(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *jdc_func, nw_callback_t cb, void* ctx);

extern qosa_nw_err_e qosa_get_ims_mode(qosa_uint8_t simid, qosa_uint8_t *ims_conf, qosa_bool_t *volte_cap);
typedef qosa_nw_err_e (* _api_qosa_get_ims_mode_t)(qosa_uint8_t simid, qosa_uint8_t *ims_conf, qosa_bool_t *volte_cap);

extern qosa_nw_err_e qosa_set_ims_mode(qosa_uint8_t simid, qosa_uint8_t ims_conf);
typedef qosa_nw_err_e (* _api_qosa_set_ims_mode_t)(qosa_uint8_t simid, qosa_uint8_t ims_conf);

extern qosa_nw_err_e qosa_nw_get_jamm_detect_func(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *jdc_func);
typedef qosa_nw_err_e (* _api_qosa_nw_get_jamm_detect_func_t)(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *jdc_func);

extern qosa_nw_err_e qosa_nw_set_jamm_detect_param(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *jdc_config);
typedef qosa_nw_err_e (* _api_qosa_nw_set_jamm_detect_param_t)(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *jdc_config);

extern qosa_nw_err_e qosa_nw_get_jamm_detect_param(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *all_param);
typedef qosa_nw_err_e (* _api_qosa_nw_get_jamm_detect_param_t)(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *all_param);

extern qosa_nw_err_e qosa_nw_get_as_event(qosa_uint8_t simid,qosa_nw_as_event_t *as_event_info);
typedef qosa_nw_err_e (* _api_qosa_nw_get_as_event_t)(qosa_uint8_t simid,qosa_nw_as_event_t *as_event_info);

extern qosa_nw_err_e qosa_nw_set_edrx_param(qosa_uint8_t simid, qosa_nw_edrx_cfg_t *cfg, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_edrx_param_t)(qosa_uint8_t simid, qosa_nw_edrx_cfg_t *cfg, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e qosa_nw_get_edrx_param(qosa_uint8_t simid, qosa_nw_edrx_info_t *info);
typedef qosa_nw_err_e (* _api_qosa_nw_get_edrx_param_t)(qosa_uint8_t simid, qosa_nw_edrx_info_t *info);

extern qosa_nw_err_e qosa_nw_get_scell_is_changed(qosa_uint8_t simid,qosa_bool_t *result);
typedef qosa_nw_err_e (* _api_qosa_nw_get_scell_is_changed_t)(qosa_uint8_t simid,qosa_bool_t *result);

extern qosa_bool_t qosa_nw_nitz_is_ready(void);
typedef qosa_bool_t (* _api_qosa_nw_nitz_is_ready_t)(void);

extern qosa_nw_err_e qosa_nw_set_ue_operation_mode(qosa_uint8_t simid,qosa_uint8_t mode,nw_callback_t cb, void* ctx);
typedef qosa_nw_err_e (* _api_qosa_nw_set_ue_operation_mode_t)(qosa_uint8_t simid,qosa_uint8_t mode,nw_callback_t cb, void* ctx);

extern qosa_nw_err_e qosa_nw_get_ue_operation_mode(qosa_uint8_t simid,qosa_uint8_t *mode);
typedef qosa_nw_err_e (* _api_qosa_nw_get_ue_operation_mode_t)(qosa_uint8_t simid,qosa_uint8_t *mode);

extern qosa_nw_err_e qosa_nw_set_blacklist_param(qosa_uint8_t simid, qosa_nw_blacklist_param_t *all_param);
typedef qosa_nw_err_e (* _api_qosa_nw_set_blacklist_param_t)(qosa_uint8_t simid, qosa_nw_blacklist_param_t *all_param);

extern qosa_nw_err_e qosa_nw_get_blacklist_param(qosa_uint8_t simid, qosa_nw_blacklist_param_t *all_param);
typedef qosa_nw_err_e (* _api_qosa_nw_get_blacklist_param_t)(qosa_uint8_t simid, qosa_nw_blacklist_param_t *all_param);

extern qosa_nw_err_e qosa_nw_blacklist_set_cell_info(qosa_uint8_t simid, qosa_uint8_t opt, qosa_nw_blacklist_cell_list_t *cell_list);
typedef qosa_nw_err_e (* _api_qosa_nw_blacklist_set_cell_info_t)(qosa_uint8_t simid, qosa_uint8_t opt, qosa_nw_blacklist_cell_list_t *cell_list);

extern qosa_nw_err_e qosa_nw_blacklist_get_cell_info(qosa_uint8_t simid, qosa_nw_blacklist_cell_list_t *cell_list);
typedef qosa_nw_err_e (* _api_qosa_nw_blacklist_get_cell_info_t)(qosa_uint8_t simid, qosa_nw_blacklist_cell_list_t *cell_list);

extern qosa_nw_err_e qosa_nw_get_pdpremap(qosa_uint8_t simid, qosa_uint8_t *pdpremap);
typedef qosa_nw_err_e (*_api_qosa_nw_get_pdpremap_t)(qosa_uint8_t simid, qosa_uint8_t *pdpremap);

extern qosa_nw_err_e qosa_nw_set_pdpremap(qosa_uint8_t simid, qosa_uint8_t pdpremap, nw_callback_t cb, void *ctx);
typedef qosa_nw_err_e (*_api_qosa_nw_set_pdpremap_t)(qosa_uint8_t simid, qosa_uint8_t pdpremap, nw_callback_t cb, void *ctx);

extern qosa_nw_err_e  qosa_nw_set_anti_detect_state(qosa_uint8_t anti_detect_state);
typedef qosa_nw_err_e (* _api_qosa_nw_set_anti_detect_state_t)(qosa_uint8_t anti_detect_state);

extern qosa_nw_err_e qosa_nw_get_anti_detect_state(qosa_uint8_t *anti_detect_state);
typedef qosa_nw_err_e (* _api_qosa_nw_get_anti_detect_state_t)(qosa_uint8_t *anti_detect_state);

extern qosa_nw_err_e qosa_nw_anti_detect_register_cb(void(*cb)(qosa_uint32_t earfcn,qosa_uint16_t pci));
typedef qosa_nw_err_e (* _api_qosa_nw_anti_detect_register_cb_t)(void(*cb)(qosa_uint32_t earfcn,qosa_uint16_t pci));

extern int qosa_nr_rsrp_meas(qosa_rsrp_t x);
typedef int (* _api_qosa_nr_rsrp_meas_t)(qosa_rsrp_t x);

extern int qosa_nr_rsrp_report(qosa_rsrp_t x);
typedef int (* _api_qosa_nr_rsrp_report_t)(qosa_rsrp_t x);

extern int qosa_nr_rsrq_meas(qosa_rsrq_t x);
typedef int (* _api_qosa_nr_rsrq_meas_t)(qosa_rsrq_t x);

extern int qosa_nr_rsrq_report(qosa_rsrq_t x);
typedef int (* _api_qosa_nr_rsrq_report_t)(qosa_rsrq_t x);

extern int qosa_nr_sinr_meas(qosa_sinr_t x);
typedef int (* _api_qosa_nr_sinr_meas_t)(qosa_sinr_t x);

extern int qosa_nr_sinr_report(qosa_sinr_t x);
typedef int (* _api_qosa_nr_sinr_report_t)(qosa_sinr_t x);

extern int qosa_lte_rsrp_meas(qosa_rsrp_t x);
typedef int (* _api_qosa_lte_rsrp_meas_t)(qosa_rsrp_t x);

extern int qosa_lte_rsrp_report(qosa_rsrp_t x);
typedef int (* _api_qosa_lte_rsrp_report_t)(qosa_rsrp_t x);

extern int qosa_lte_rsrq_meas(qosa_rsrq_t x);
typedef int (* _api_qosa_lte_rsrq_meas_t)(qosa_rsrq_t x);

extern int qosa_lte_rsrq_report(qosa_rsrq_t x);
typedef int (* _api_qosa_lte_rsrq_report_t)(qosa_rsrq_t x);

extern int qosa_lte_rssi_meas(qosa_rssi_t x);
typedef int (* _api_qosa_lte_rssi_meas_t)(qosa_rssi_t x);

extern int qosa_lte_rssi_report(qosa_rssi_t x);
typedef int (* _api_qosa_lte_rssi_report_t)(qosa_rssi_t x);

extern int qosa_lte_sinr_meas(qosa_sinr_t x);
typedef int (* _api_qosa_lte_sinr_meas_t)(qosa_sinr_t x);

extern int qosa_lte_sinr_report(qosa_sinr_t x);
typedef int (* _api_qosa_lte_sinr_report_t)(qosa_sinr_t x);

extern int qosa_wcdma_rscp_meas(qosa_rscp_t x);
typedef int (* _api_qosa_wcdma_rscp_meas_t)(qosa_rscp_t x);

extern int qosa_wcdma_rscp_report(qosa_rscp_t x);
typedef int (* _api_qosa_wcdma_rscp_report_t)(qosa_rscp_t x);

extern int qosa_wcdma_ecno_meas(qosa_ecno_t x);
typedef int (* _api_qosa_wcdma_ecno_meas_t)(qosa_ecno_t x);

extern int qosa_wcdma_ecno_report(qosa_ecno_t x);
typedef int (* _api_qosa_wcdma_ecno_report_t)(qosa_ecno_t x);

extern qosa_wifiscan_error_e qosa_wifiscan_option_set(qosa_wifiscan_config_t *wifiscan_config);
typedef qosa_wifiscan_error_e (* _api_qosa_wifiscan_option_set_t)(qosa_wifiscan_config_t *wifiscan_config);

extern qosa_wifiscan_error_e qosa_wifiscan_get_config(qosa_wifiscan_config_t *wifiscan_config);
typedef qosa_wifiscan_error_e (* _api_qosa_wifiscan_get_config_t)(qosa_wifiscan_config_t *wifiscan_config);

extern qosa_wifiscan_error_e qosa_wifiscan_register_cb(qosa_wifiscan_callback wifiscan_cb, void *user_data);
typedef qosa_wifiscan_error_e (* _api_qosa_wifiscan_register_cb_t)(qosa_wifiscan_callback wifiscan_cb, void *user_data);

extern qosa_wifiscan_error_e qosa_wifiscan_do(qosa_uint16_t *p_ap_cnt, qosa_wifi_ap_info_t *p_ap_infos);
typedef qosa_wifiscan_error_e (* _api_qosa_wifiscan_do_t)(qosa_uint16_t *p_ap_cnt, qosa_wifi_ap_info_t *p_ap_infos);

extern qosa_wifiscan_error_e qosa_wifiscan_async(void);
typedef qosa_wifiscan_error_e (* _api_qosa_wifiscan_async_t)(void);

extern qosa_wifiscan_error_e qosa_wifiscan_close(void);
typedef qosa_wifiscan_error_e (* _api_qosa_wifiscan_close_t)(void);

extern qosa_wifiscan_error_e qosa_wifiscan_open(void);
typedef qosa_wifiscan_error_e (* _api_qosa_wifiscan_open_t)(void);

extern qosa_sim_type_e qosa_sim_read_sim_type(qosa_uint8_t simid);
typedef qosa_sim_type_e (* _api_qosa_sim_read_sim_type_t)(qosa_uint8_t simid);

extern qosa_sim_err_e qosa_sim_get_imsi(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_get_imsi_t)(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_read_imsi(qosa_uint8_t simid, qosa_sim_imsi_t *imsi);
typedef qosa_sim_err_e (* _api_qosa_sim_read_imsi_t)(qosa_uint8_t simid, qosa_sim_imsi_t *imsi);

extern qosa_sim_err_e qosa_sim_get_status(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_get_status_t)(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_read_status(qosa_uint8_t simid, qosa_sim_status_e *status);
typedef qosa_sim_err_e (* _api_qosa_sim_read_status_t)(qosa_uint8_t simid, qosa_sim_status_e *status);

extern int qosa_sim_read_init_stat(qosa_uint8_t simid);
typedef int (* _api_qosa_sim_read_init_stat_t)(qosa_uint8_t simid);

extern qosa_sim_insert_stat_e qosa_sim_read_insert_stat(qosa_uint8_t simid);
typedef qosa_sim_insert_stat_e (* _api_qosa_sim_read_insert_stat_t)(qosa_uint8_t simid);

extern qosa_sim_insert_stat_e qosa_sim_read_slot_stat(qosa_uint8_t slotid);
typedef qosa_sim_insert_stat_e (* _api_qosa_sim_read_slot_stat_t)(qosa_uint8_t slotid);

extern qosa_sim_err_e qosa_sim_set_sim_hot_swap(qosa_uint8_t slotid, qosa_sim_hot_swap_cfg_t *hot_swap_config);
typedef qosa_sim_err_e (* _api_qosa_sim_set_sim_hot_swap_t)(qosa_uint8_t slotid, qosa_sim_hot_swap_cfg_t *hot_swap_config);

extern qosa_sim_err_e qosa_sim_get_sim_hot_swap(qosa_uint8_t slotid, qosa_sim_hot_swap_cfg_t *hot_swap_config);
typedef qosa_sim_err_e (* _api_qosa_sim_get_sim_hot_swap_t)(qosa_uint8_t slotid, qosa_sim_hot_swap_cfg_t *hot_swap_config);

extern qosa_sim_err_e qosa_sim_get_slot_id(qosa_uint8_t simid, qosa_uint8_t *slotid);
typedef qosa_sim_err_e (* _api_qosa_sim_get_slot_id_t)(qosa_uint8_t simid, qosa_uint8_t *slotid);

extern qosa_sim_err_e qosa_sim_set_slot_id(qosa_uint8_t simid, qosa_uint8_t slotid);
typedef qosa_sim_err_e (* _api_qosa_sim_set_slot_id_t)(qosa_uint8_t simid, qosa_uint8_t slotid);

extern qosa_sim_err_e qosa_sim_verify_pin(qosa_uint8_t simid, qosa_sim_pin_t *pin, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_verify_pin_t)(qosa_uint8_t simid, qosa_sim_pin_t *pin, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_unblock_pin(qosa_uint8_t simid, qosa_sim_pin_t *unblock_pin, qosa_sim_pin_t *new_pin, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_unblock_pin_t)(qosa_uint8_t simid, qosa_sim_pin_t *unblock_pin, qosa_sim_pin_t *new_pin, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_change_pin(qosa_uint8_t simid, qosa_sim_facility_e fac, qosa_sim_pin_t *old_pin, qosa_sim_pin_t *new_pin, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_change_pin_t)(qosa_uint8_t simid, qosa_sim_facility_e fac, qosa_sim_pin_t *old_pin, qosa_sim_pin_t *new_pin, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_get_pin_remain_retries(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_get_pin_remain_retries_t)(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_get_facility_lock(qosa_uint8_t simid, qosa_uint8_t fac, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_get_facility_lock_t)(qosa_uint8_t simid, qosa_uint8_t fac, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_set_facility_lock(qosa_uint8_t simid, qosa_sim_facility_e fac, qosa_sim_pin_t *pin, qosa_uint8_t mode, qosa_uint8_t class, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_set_facility_lock_t)(qosa_uint8_t simid, qosa_sim_facility_e fac, qosa_sim_pin_t *pin, qosa_uint8_t mode, qosa_uint8_t class, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_generic_access(qosa_uint8_t simid, qosa_uint8_t *cmd_data, qosa_uint16_t data_len, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_generic_access_t)(qosa_uint8_t simid, qosa_uint8_t *cmd_data, qosa_uint16_t data_len, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_generic_logical_channel_access(qosa_uint8_t simid, qosa_uint32_t session_id, qosa_uint8_t *cmd_data, qosa_uint16_t data_len, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_generic_logical_channel_access_t)(qosa_uint8_t simid, qosa_uint32_t session_id, qosa_uint8_t *cmd_data, qosa_uint16_t data_len, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_restricted_access(qosa_uint8_t         simid,
    qosa_uint8_t         cmd,
    qosa_uint16_t        fileid,
    qosa_uint8_t         p1,
    qosa_uint8_t         p2,
    qosa_uint8_t         p3,
    qosa_sim_cmd_data_t *data,
    qosa_sim_path_id_t  *path,
    sim_callback_ptr      cb,
    void               *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_restricted_access_t)(qosa_uint8_t         simid,
    qosa_uint8_t         cmd,
    qosa_uint16_t        fileid,
    qosa_uint8_t         p1,
    qosa_uint8_t         p2,
    qosa_uint8_t         p3,
    qosa_sim_cmd_data_t *data,
    qosa_sim_path_id_t  *path,
    sim_callback_ptr      cb,
    void               *ctx);

extern qosa_sim_err_e qosa_sim_get_iccid(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_get_iccid_t)(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_read_iccid(qosa_uint8_t simid, qosa_sim_iccid_t *iccid);
typedef qosa_sim_err_e (* _api_qosa_sim_read_iccid_t)(qosa_uint8_t simid, qosa_sim_iccid_t *iccid);

extern qosa_sim_err_e qosa_sim_open_logical_channel(qosa_uint8_t simid, qosa_sim_dfname_t *dfname, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_open_logical_channel_t)(qosa_uint8_t simid, qosa_sim_dfname_t *dfname, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_close_logical_channel(qosa_uint8_t simid, qosa_uint32_t sessionid, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_close_logical_channel_t)(qosa_uint8_t simid, qosa_uint32_t sessionid, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_get_phonenumber(qosa_uint8_t simid, qosa_sim_phonenumber_t *phonenumber);
typedef qosa_sim_err_e (* _api_qosa_sim_get_phonenumber_t)(qosa_uint8_t simid, qosa_sim_phonenumber_t *phonenumber);

extern qosa_sim_err_e qosa_sim_get_sim_power_save(qosa_uint8_t simid, qosa_bool_t *status);
typedef qosa_sim_err_e (* _api_qosa_sim_get_sim_power_save_t)(qosa_uint8_t simid, qosa_bool_t *status);

extern qosa_sim_err_e qosa_sim_set_sim_power_save(qosa_uint8_t simid, qosa_bool_t status);
typedef qosa_sim_err_e (* _api_qosa_sim_set_sim_power_save_t)(qosa_uint8_t simid, qosa_bool_t status);

extern qosa_sim_err_e qosa_sim_get_simwprotect_mode(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_get_simwprotect_mode_t)(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);

extern qosa_sim_err_e qosa_sim_set_simwprotect_mode(qosa_uint8_t simid, qosa_sim_simwprotect_t simwprotect_cfg, sim_callback_ptr cb, void *ctx);
typedef qosa_sim_err_e (* _api_qosa_sim_set_simwprotect_mode_t)(qosa_uint8_t simid, qosa_sim_simwprotect_t simwprotect_cfg, sim_callback_ptr cb, void *ctx);

extern int qosa_vsim_adapt_init(qosa_vsim_adapt_event_cb event_cb);
typedef int (* _api_qosa_vsim_adapt_init_t)(qosa_vsim_adapt_event_cb event_cb);

int qosa_vsim_adapt_set_type(qosa_uint8_t simid, qosa_vsim_adapt_type_e type);
typedef int (* _api_qosa_vsim_adapt_set_type_t)(qosa_uint8_t simid, qosa_vsim_adapt_type_e type);

int qosa_vsim_adapt_register_apdu_cb(qosa_vsim_adapt_apdu_cb cb);
typedef int (* _api_qosa_vsim_adapt_register_apdu_cb_t)(qosa_vsim_adapt_apdu_cb cb);

extern qosa_usbnet_err_e qosa_usbnet_set_usbnetmac(qosa_uint8_t simid, qosa_uint8_t *pmac);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_set_usbnetmac_t)(qosa_uint8_t simid, qosa_uint8_t *pmac);

extern qosa_usbnet_err_e qosa_usbnet_get_usbnetmac(qosa_uint8_t simid, qosa_uint8_t *pmac);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_get_usbnetmac_t)(qosa_uint8_t simid, qosa_uint8_t *pmac);

extern qosa_usbnet_err_e qosa_usbnet_set_nat_mode(qosa_uint8_t simid, qosa_bool_t nat_mode, qosa_uint32_t cid);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_set_nat_mode_t)(qosa_uint8_t simid, qosa_bool_t nat_mode, qosa_uint32_t cid);

extern qosa_usbnet_err_e qosa_usbnet_get_nat_mode(qosa_uint8_t simid, qosa_bool_t *nat_mode, qosa_uint32_t *cid);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_get_nat_mode_t)(qosa_uint8_t simid, qosa_bool_t *nat_mode, qosa_uint32_t *cid);

extern qosa_usbnet_err_e qosa_usbnet_set_type(qosa_uint8_t simid, qosa_usbnet_type_e type);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_set_type_t)(qosa_uint8_t simid, qosa_usbnet_type_e type);

extern qosa_usbnet_err_e qosa_usbnet_get_type(qosa_uint8_t simid, qosa_usbnet_type_e *type);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_get_type_t)(qosa_uint8_t simid, qosa_usbnet_type_e *type);

extern qosa_usbnet_err_e qosa_usbnet_set_config(qosa_uint8_t simid, qosa_usbnet_config_t *config);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_set_config_t)(qosa_uint8_t simid, qosa_usbnet_config_t *config);

extern qosa_usbnet_err_e qosa_usbnet_get_config(qosa_uint8_t simid, qosa_usbnet_config_t *config);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_get_config_t)(qosa_uint8_t simid, qosa_usbnet_config_t *config);

extern qosa_usbnet_err_e qosa_usbnet_start(qosa_uint8_t simid, usbnet_callback_t cb, void *ctx);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_start_t)(qosa_uint8_t simid, usbnet_callback_t cb, void *ctx);

extern qosa_usbnet_err_e qosa_usbnet_stop(qosa_uint8_t simid, usbnet_callback_t cb, void *ctx);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_stop_t)(qosa_uint8_t simid, usbnet_callback_t cb, void *ctx);

extern qosa_usbnet_err_e qosa_usbnet_get_status(qosa_uint8_t simid, qosa_usbnet_status_e *status);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_get_status_t)(qosa_uint8_t simid, qosa_usbnet_status_e *status);

extern qosa_usbnet_err_e qosa_usbnet_set_subnet(qosa_uint8_t simid, qosa_usbnet_subnet_config_t config);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_set_subnet_t)(qosa_uint8_t simid, qosa_usbnet_subnet_config_t config);

extern qosa_usbnet_err_e qosa_usbnet_get_subnet(qosa_uint8_t simid, qosa_usbnet_subnet_config_t *config);
typedef qosa_usbnet_err_e (* _api_qosa_usbnet_get_subnet_t)(qosa_uint8_t simid, qosa_usbnet_subnet_config_t *config);

extern qosa_nvm_error_e qosa_nv_item_cfg_delete(char *name, qosa_nv_cfg_item_data_t *item_data, qosa_uint16_t item_count);
typedef qosa_nvm_error_e (* _api_qosa_nv_item_cfg_delete_t)(char *name, qosa_nv_cfg_item_data_t *item_data, qosa_uint16_t item_count);

extern qosa_nvm_error_e qosa_nv_item_json_write(char *name, qosa_nv_cfg_item_data_t *item_data, qosa_uint16_t item_count);
typedef qosa_nvm_error_e (* _api_qosa_nv_item_json_write_t)(char *name, qosa_nv_cfg_item_data_t *item_data, qosa_uint16_t item_count);

extern qosa_nvm_error_e qosa_nv_item_cfg_read(char *name, qosa_nv_cfg_item_data_t *item_data, qosa_uint16_t item_count);
typedef qosa_nvm_error_e (* _api_qosa_nv_item_cfg_read_t)(char *name, qosa_nv_cfg_item_data_t *item_data, qosa_uint16_t item_count);

extern void qosa_free(void* ptr);
typedef void (* _api_qosa_free_t)(void* ptr);

extern void* qosa_realloc(void* ptr, qosa_size_t nbytes);
typedef void* (* _api_qosa_realloc_t)(void* ptr, qosa_size_t nbytes);

extern void* qosa_calloc(qosa_size_t count, qosa_size_t size);
typedef void* (* _api_qosa_calloc_t)(qosa_size_t count, qosa_size_t size);

extern void* qosa_memset(void* s, int c, qosa_size_t size);
typedef void* (* _api_qosa_memset_t)(void* s, int c, qosa_size_t size);

extern void* qosa_memcpy(void* dest, const void* src, qosa_size_t n);
typedef void* (* _api_qosa_memcpy_t)(void* dest, const void* src, qosa_size_t n);

extern void* qosa_memmove(void* dest, const void* src, qosa_size_t n);
typedef void* (* _api_qosa_memmove_t)(void* dest, const void* src, qosa_size_t n);

extern int qosa_memcmp(const void* s1, const void* s2, qosa_size_t n);
typedef int (* _api_qosa_memcmp_t)(const void* s1, const void* s2, qosa_size_t n);

extern char* qosa_strstr(const char* s1, const char* s2);
typedef char* (* _api_qosa_strstr_t)(const char* s1, const char* s2);

extern qosa_int32_t qosa_strcasecmp(const char* a, const char* b);
typedef qosa_int32_t (* _api_qosa_strcasecmp_t)(const char* a, const char* b);

extern qosa_size_t qosa_strlcpy(char* dst, const char* src, qosa_size_t n);
typedef qosa_size_t (* _api_qosa_strlcpy_t)(char* dst, const char* src, qosa_size_t n);

extern char* qosa_strncpy(char* dst, const char* src, qosa_size_t n);
typedef char* (* _api_qosa_strncpy_t)(char* dst, const char* src, qosa_size_t n);

extern char* qosa_strcpy(char* dst, const char* src);
typedef char* (* _api_qosa_strcpy_t)(char* dst, const char* src);

extern qosa_int32_t qosa_strncmp(const char* cs, const char* ct, qosa_size_t count);
typedef qosa_int32_t (* _api_qosa_strncmp_t)(const char* cs, const char* ct, qosa_size_t count);

extern qosa_int32_t qosa_strcmp(const char* cs, const char* ct);
typedef qosa_int32_t (* _api_qosa_strcmp_t)(const char* cs, const char* ct);

extern qosa_size_t qosa_strlen(const char* s);
typedef qosa_size_t (* _api_qosa_strlen_t)(const char* s);

extern qosa_size_t qosa_strnlen(const char* s, qosa_size_t maxlen);
typedef qosa_size_t (* _api_qosa_strnlen_t)(const char* s, qosa_size_t maxlen);

extern char* qosa_strdup(const char* s);
typedef char* (* _api_qosa_strdup_t)(const char* s);

extern int qosa_vsnprintf(char* buf, qosa_size_t size, const char* fmt, va_list args);
typedef int (* _api_qosa_vsnprintf_t)(char* buf, qosa_size_t size, const char* fmt, va_list args);

extern int qosa_snprintf(char* buf, qosa_size_t size, const char* argv, ...);
typedef int (* _api_qosa_snprintf_t)(char* buf, qosa_size_t size, const char* argv, ...);

extern int qosa_vsprintf(char* buf, const char* format, va_list arg_ptr);
typedef int (* _api_qosa_vsprintf_t)(char* buf, const char* format, va_list arg_ptr);

extern int qosa_sprintf(char* buf, const char* argv, ...);
typedef int (* _api_qosa_sprintf_t)(char* buf, const char* argv, ...);

extern void qosa_assert(const char* funcname, qosa_size_t line, int a);
typedef void (* _api_qosa_assert_t)(const char* funcname, qosa_size_t line, int a);

extern int qosa_strncasecmp(const char* s1, const char* s2, qosa_size_t size);
typedef int (* _api_qosa_strncasecmp_t)(const char* s1, const char* s2, qosa_size_t size);

extern char* qosa_strchr(const char* _String, int const _Ch);
typedef char* (* _api_qosa_strchr_t)(const char* _String, int const _Ch);

extern unsigned long int qosa_strtoul(const char* str, char** endptr, int base);
typedef unsigned long int (* _api_qosa_strtoul_t)(const char* str, char** endptr, int base);

extern long int qosa_strtol(const char* str, char** endptr, int base);
typedef long int (* _api_qosa_strtol_t)(const char* str, char** endptr, int base);

extern unsigned long long qosa_strtoull(const char* str, char** endptr, int base);
typedef unsigned long long (* _api_qosa_strtoull_t)(const char* str, char** endptr, int base);

extern int qosa_sscanf(const char* str, const char* argv, ...);
typedef int (* _api_qosa_sscanf_t)(const char* str, const char* argv, ...);

extern char* qosa_strrchr(const char* _String, int const _Ch);
typedef char* (* _api_qosa_strrchr_t)(const char* _String, int const _Ch);

extern long long qosa_strtoll(const char* str, char** endptr, int base);
typedef long long (* _api_qosa_strtoll_t)(const char* str, char** endptr, int base);

extern double qosa_strtod(const char* str, char** endptr);
typedef double (* _api_qosa_strtod_t)(const char* str, char** endptr);

extern char* qosa_strcat(char* dest, const char* src);
typedef char* (* _api_qosa_strcat_t)(char* dest, const char* src);

extern char* qosa_strncat(char* dest, const char* src, qosa_size_t size);
typedef char* (* _api_qosa_strncat_t)(char* dest, const char* src, qosa_size_t size);

extern int qosa_toupper(int c);
typedef int (* _api_qosa_toupper_t)(int c);

extern int qosa_tolower(int c);
typedef int (* _api_qosa_tolower_t)(int c);

extern void qosa_touppercase(char *str);
typedef void (* _api_qosa_touppercase_t)(char *str);

extern char* qosa_strtok(char* s, const char* delim);
typedef char* (* _api_qosa_strtok_t)(char* s, const char* delim);

extern int qosa_strspn(char* s, const char* accept);
typedef int (* _api_qosa_strspn_t)(char* s, const char* accept);

extern double qosa_atof(const char* nptr);
typedef double (* _api_qosa_atof_t)(const char* nptr);

extern int qosa_atoi(const char* nptr);
typedef int (* _api_qosa_atoi_t)(const char* nptr);

extern int qosa_strcspn(const char* s, const char* reject);
typedef int (* _api_qosa_strcspn_t)(const char* s, const char* reject);

extern int qosa_get_errno(void);
typedef int (* _api_qosa_get_errno_t)(void);

extern char* qosa_strpbrk(const char* cs, const char* ct);
typedef char* (* _api_qosa_strpbrk_t)(const char* cs, const char* ct);

extern int qosa_system(char* command);
typedef int (* _api_qosa_system_t)(char* command);

extern void* qosa_memchr(void* s, int c, qosa_size_t size);
typedef void* (* _api_qosa_memchr_t)(void* s, int c, qosa_size_t size);

extern long qosa_atol(const char* nptr);
typedef long (* _api_qosa_atol_t)(const char* nptr);

extern int qosa_rand(void);
typedef int (* _api_qosa_rand_t)(void);

extern int qosa_stricmpl(char* str1, char* str2, int maxlength);
typedef int (* _api_qosa_stricmpl_t)(char* str1, char* str2, int maxlength);

extern char* qosa_strnstr(const char* s1, const char* s2, size_t n);
typedef char* (* _api_qosa_strnstr_t)(const char* s1, const char* s2, size_t n);

extern int qosa_msgq_create(qosa_msgq_t* msgQRef, qosa_uint32_t size, qosa_uint32_t maxNumber);
typedef int (* _api_qosa_msgq_create_t)(qosa_msgq_t* msgQRef, qosa_uint32_t size, qosa_uint32_t maxNumber);

extern int qosa_msgq_create_static(qosa_msgq_t *msgQRef, qosa_uint32_t size, qosa_uint32_t maxNumber, void *cb_mem, void *q_name, qosa_uint8_t *mq_mem, qosa_uint32_t mq_size);
typedef int (* _api_qosa_msgq_create_static_t)(qosa_msgq_t *msgQRef, qosa_uint32_t size, qosa_uint32_t maxNumber, void *cb_mem, void *q_name, qosa_uint8_t *mq_mem, qosa_uint32_t mq_size);

extern int qosa_msgq_delete(qosa_msgq_t msgQRef);
typedef int (* _api_qosa_msgq_delete_t)(qosa_msgq_t msgQRef);

extern int qosa_msgq_release(qosa_msgq_t msgQRef, qosa_uint32_t size, qosa_uint8_t* value, qosa_uint32_t timeout);
typedef int (* _api_qosa_msgq_release_t)(qosa_msgq_t msgQRef, qosa_uint32_t size, qosa_uint8_t* value, qosa_uint32_t timeout);

extern int qosa_msgq_wait(qosa_msgq_t msgQRef, qosa_uint8_t* value, qosa_uint32_t size, qosa_uint32_t timeout);
typedef int (* _api_qosa_msgq_wait_t)(qosa_msgq_t msgQRef, qosa_uint8_t* value, qosa_uint32_t size, qosa_uint32_t timeout);

extern int qosa_msgq_get_cnt(qosa_msgq_t msgQRef, qosa_uint32_t* cnt_ptr);
typedef int (* _api_qosa_msgq_get_cnt_t)(qosa_msgq_t msgQRef, qosa_uint32_t* cnt_ptr);

extern int qosa_msgq_get_space(qosa_msgq_t msgQRef, qosa_uint32_t *space);
typedef int (* _api_qosa_msgq_get_space_t)(qosa_msgq_t msgQRef, qosa_uint32_t *space);

extern int qosa_msgq_reset(qosa_msgq_t msgQRef);
typedef int (* _api_qosa_msgq_reset_t)(qosa_msgq_t msgQRef);

extern int qosa_timer_start(qosa_timer_t timerRef, qosa_uint32_t set_Time, qosa_bool_t cyclicalEn);
typedef int (* _api_qosa_timer_start_t)(qosa_timer_t timerRef, qosa_uint32_t set_Time, qosa_bool_t cyclicalEn);

extern int qosa_timer_stop(qosa_timer_t timerRef);
typedef int (* _api_qosa_timer_stop_t)(qosa_timer_t timerRef);

extern qosa_bool_t qosa_timer_is_running(qosa_timer_t timerRef);
typedef qosa_bool_t (* _api_qosa_timer_is_running_t)(qosa_timer_t timerRef);

extern int qosa_timer_delete(qosa_timer_t timerRef);
typedef int (* _api_qosa_timer_delete_t)(qosa_timer_t timerRef);

extern int qosa_task_delete(qosa_task_t taskRef);
typedef int (* _api_qosa_task_delete_t)(qosa_task_t taskRef);

extern int qosa_task_get_status(qosa_task_t task_ref, qosa_int32_t* status);
typedef int (* _api_qosa_task_get_status_t)(qosa_task_t task_ref, qosa_int32_t* status);

extern int qosa_task_get_stack_space(qosa_task_t task_ref, qosa_int32_t* space);
typedef int (* _api_qosa_task_get_stack_space_t)(qosa_task_t task_ref, qosa_int32_t* space);

extern void qosa_task_sleep_ms(qosa_uint32_t ms);
typedef void (* _api_qosa_task_sleep_ms_t)(qosa_uint32_t ms);

extern void qosa_task_sleep_sec(qosa_uint32_t s);
typedef void (* _api_qosa_task_sleep_sec_t)(qosa_uint32_t s);

extern int qosa_task_suspend(qosa_task_t taskRef);
typedef int (* _api_qosa_task_suspend_t)(qosa_task_t taskRef);

extern int qosa_task_resume(qosa_task_t taskRef);
typedef int (* _api_qosa_task_resume_t)(qosa_task_t taskRef);

extern int qosa_task_get_current_ref(qosa_task_t* taskRef);
typedef int (* _api_qosa_task_get_current_ref_t)(qosa_task_t* taskRef);

extern int qosa_sem_create(qosa_sem_t* semaRef, qosa_uint32_t initialCount);
typedef int (* _api_qosa_sem_create_t)(qosa_sem_t* semaRef, qosa_uint32_t initialCount);

extern int qosa_sem_create_ex(qosa_sem_t* semaRef, qosa_uint32_t initialCount, qosa_uint32_t max_cnt);
typedef int (* _api_qosa_sem_create_ex_t)(qosa_sem_t* semaRef, qosa_uint32_t initialCount, qosa_uint32_t max_cnt);

extern int qosa_sem_wait(qosa_sem_t semaRef, qosa_uint32_t timeout);
typedef int (* _api_qosa_sem_wait_t)(qosa_sem_t semaRef, qosa_uint32_t timeout);

extern int qosa_sem_release(qosa_sem_t semaRef);
typedef int (* _api_qosa_sem_release_t)(qosa_sem_t semaRef);

extern int qosa_sem_get_cnt(qosa_sem_t semaRef, qosa_uint32_t* cnt_ptr);
typedef int (* _api_qosa_sem_get_cnt_t)(qosa_sem_t semaRef, qosa_uint32_t* cnt_ptr);

extern int qosa_sem_delete(qosa_sem_t semaRef);
typedef int (* _api_qosa_sem_delete_t)(qosa_sem_t semaRef);

extern int qosa_mutex_create(qosa_mutex_t* mutexRef);
typedef int (* _api_qosa_mutex_create_t)(qosa_mutex_t* mutexRef);

extern int qosa_mutex_try_lock(qosa_mutex_t mutexRef);
typedef int (* _api_qosa_mutex_try_lock_t)(qosa_mutex_t mutexRef);

extern int qosa_mutex_lock(qosa_mutex_t mutexRef, qosa_uint32_t timeout);
typedef int (* _api_qosa_mutex_lock_t)(qosa_mutex_t mutexRef, qosa_uint32_t timeout);

extern int qosa_mutex_unlock(qosa_mutex_t mutexRef);
typedef int (* _api_qosa_mutex_unlock_t)(qosa_mutex_t mutexRef);

extern int qosa_mutex_delete(qosa_mutex_t mutexRef);
typedef int (* _api_qosa_mutex_delete_t)(qosa_mutex_t mutexRef);

extern qosa_errcode_os_e qosa_flag_create(qosa_flag_t *flag_ref);
typedef qosa_errcode_os_e (* _api_qosa_flag_create_t)(qosa_flag_t *flag_ref);

extern qosa_errcode_os_e qosa_flag_delete(qosa_flag_t flag_ref);
typedef qosa_errcode_os_e (* _api_qosa_flag_delete_t)(qosa_flag_t flag_ref);

extern qosa_errcode_os_e qosa_flag_set(qosa_flag_t flag_ref, qosa_uint32_t flag);
typedef qosa_errcode_os_e (* _api_qosa_flag_set_t)(qosa_flag_t flag_ref, qosa_uint32_t flag);

extern qosa_uint32_t qosa_flag_wait(qosa_flag_t flag_ref, qosa_uint32_t flag, qosa_uint32_t clear_exit, qosa_uint32_t wait_all, qosa_uint32_t timeout);
typedef qosa_uint32_t (* _api_qosa_flag_wait_t)(qosa_flag_t flag_ref, qosa_uint32_t flag, qosa_uint32_t clear_exit, qosa_uint32_t wait_all, qosa_uint32_t timeout);

extern qosa_errcode_os_e qosa_flag_clear(qosa_flag_t flag_ref, qosa_uint32_t flag);
typedef qosa_errcode_os_e (* _api_qosa_flag_clear_t)(qosa_flag_t flag_ref, qosa_uint32_t flag);

extern qosa_uint32_t qosa_flag_get(qosa_flag_t flag_ref);
typedef qosa_uint32_t (* _api_qosa_flag_get_t)(qosa_flag_t flag_ref);

extern const char* Q_cJSON_Version(void);
typedef const char* (* _api_Q_cJSON_Version_t)(void);

extern void Q_cJSON_InitHooks(Q_cJSON_Hooks* hooks);
typedef void (* _api_Q_cJSON_InitHooks_t)(Q_cJSON_Hooks* hooks);

extern Q_cJSON * Q_cJSON_Parse(const char *value);
typedef Q_cJSON * (* _api_Q_cJSON_Parse_t)(const char *value);

extern Q_cJSON * Q_cJSON_ParseWithLength(const char *value, qosa_size_t buffer_length);
typedef Q_cJSON * (* _api_Q_cJSON_ParseWithLength_t)(const char *value, qosa_size_t buffer_length);

extern Q_cJSON * Q_cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);
typedef Q_cJSON * (* _api_Q_cJSON_ParseWithOpts_t)(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);

extern Q_cJSON * Q_cJSON_ParseWithLengthOpts(const char *value, qosa_size_t buffer_length, const char **return_parse_end, cJSON_bool require_null_terminated);
typedef Q_cJSON * (* _api_Q_cJSON_ParseWithLengthOpts_t)(const char *value, qosa_size_t buffer_length, const char **return_parse_end, cJSON_bool require_null_terminated);

extern char * Q_cJSON_Print(const Q_cJSON *item);
typedef char * (* _api_Q_cJSON_Print_t)(const Q_cJSON *item);

extern char * Q_cJSON_PrintUnformatted(const Q_cJSON *item);
typedef char * (* _api_Q_cJSON_PrintUnformatted_t)(const Q_cJSON *item);

extern char * Q_cJSON_PrintBuffered(const Q_cJSON *item, int prebuffer, cJSON_bool fmt);
typedef char * (* _api_Q_cJSON_PrintBuffered_t)(const Q_cJSON *item, int prebuffer, cJSON_bool fmt);

extern cJSON_bool Q_cJSON_PrintPreallocated(Q_cJSON *item, char *buffer, const int length, const cJSON_bool format);
typedef cJSON_bool (* _api_Q_cJSON_PrintPreallocated_t)(Q_cJSON *item, char *buffer, const int length, const cJSON_bool format);

extern void Q_cJSON_Delete(Q_cJSON *item);
typedef void (* _api_Q_cJSON_Delete_t)(Q_cJSON *item);

extern int Q_cJSON_GetArraySize(const Q_cJSON *array);
typedef int (* _api_Q_cJSON_GetArraySize_t)(const Q_cJSON *array);

extern Q_cJSON * Q_cJSON_GetArrayItem(const Q_cJSON *array, int index);
typedef Q_cJSON * (* _api_Q_cJSON_GetArrayItem_t)(const Q_cJSON *array, int index);

extern Q_cJSON * Q_cJSON_GetObjectItem(const Q_cJSON * const object, const char * const string);
typedef Q_cJSON * (* _api_Q_cJSON_GetObjectItem_t)(const Q_cJSON * const object, const char * const string);

extern Q_cJSON * Q_cJSON_GetObjectItemCaseSensitive(const Q_cJSON * const object, const char * const string);
typedef Q_cJSON * (* _api_Q_cJSON_GetObjectItemCaseSensitive_t)(const Q_cJSON * const object, const char * const string);

extern cJSON_bool Q_cJSON_HasObjectItem(const Q_cJSON *object, const char *string);
typedef cJSON_bool (* _api_Q_cJSON_HasObjectItem_t)(const Q_cJSON *object, const char *string);

extern const char * Q_cJSON_GetErrorPtr(void);
typedef const char * (* _api_Q_cJSON_GetErrorPtr_t)(void);

extern char * Q_cJSON_GetStringValue(const Q_cJSON * const item);
typedef char * (* _api_Q_cJSON_GetStringValue_t)(const Q_cJSON * const item);

extern double Q_cJSON_GetNumberValue(const Q_cJSON * const item);
typedef double (* _api_Q_cJSON_GetNumberValue_t)(const Q_cJSON * const item);

extern cJSON_bool Q_cJSON_IsInvalid(const Q_cJSON * const item);
typedef cJSON_bool (* _api_Q_cJSON_IsInvalid_t)(const Q_cJSON * const item);

extern cJSON_bool Q_cJSON_IsFalse(const Q_cJSON * const item);
typedef cJSON_bool (* _api_Q_cJSON_IsFalse_t)(const Q_cJSON * const item);

extern cJSON_bool Q_cJSON_IsTrue(const Q_cJSON * const item);
typedef cJSON_bool (* _api_Q_cJSON_IsTrue_t)(const Q_cJSON * const item);

extern cJSON_bool Q_cJSON_IsBool(const Q_cJSON * const item);
typedef cJSON_bool (* _api_Q_cJSON_IsBool_t)(const Q_cJSON * const item);

extern cJSON_bool Q_cJSON_IsNull(const Q_cJSON * const item);
typedef cJSON_bool (* _api_Q_cJSON_IsNull_t)(const Q_cJSON * const item);

extern cJSON_bool Q_cJSON_IsNumber(const Q_cJSON * const item);
typedef cJSON_bool (* _api_Q_cJSON_IsNumber_t)(const Q_cJSON * const item);

extern cJSON_bool Q_cJSON_IsString(const Q_cJSON * const item);
typedef cJSON_bool (* _api_Q_cJSON_IsString_t)(const Q_cJSON * const item);

extern cJSON_bool Q_cJSON_IsArray(const Q_cJSON * const item);
typedef cJSON_bool (* _api_Q_cJSON_IsArray_t)(const Q_cJSON * const item);

extern cJSON_bool Q_cJSON_IsObject(const Q_cJSON * const item);
typedef cJSON_bool (* _api_Q_cJSON_IsObject_t)(const Q_cJSON * const item);

extern cJSON_bool Q_cJSON_IsRaw(const Q_cJSON * const item);
typedef cJSON_bool (* _api_Q_cJSON_IsRaw_t)(const Q_cJSON * const item);

extern Q_cJSON * Q_cJSON_CreateNull(void);
typedef Q_cJSON * (* _api_Q_cJSON_CreateNull_t)(void);

extern Q_cJSON * Q_cJSON_CreateTrue(void);
typedef Q_cJSON * (* _api_Q_cJSON_CreateTrue_t)(void);

extern Q_cJSON * Q_cJSON_CreateFalse(void);
typedef Q_cJSON * (* _api_Q_cJSON_CreateFalse_t)(void);

extern Q_cJSON * Q_cJSON_CreateBool(cJSON_bool boolean);
typedef Q_cJSON * (* _api_Q_cJSON_CreateBool_t)(cJSON_bool boolean);

extern Q_cJSON * Q_cJSON_CreateNumber(double num);
typedef Q_cJSON * (* _api_Q_cJSON_CreateNumber_t)(double num);

extern Q_cJSON * Q_cJSON_CreateString(const char *string);
typedef Q_cJSON * (* _api_Q_cJSON_CreateString_t)(const char *string);

extern Q_cJSON * Q_cJSON_CreateRaw(const char *raw);
typedef Q_cJSON * (* _api_Q_cJSON_CreateRaw_t)(const char *raw);

extern Q_cJSON * Q_cJSON_CreateArray(void);
typedef Q_cJSON * (* _api_Q_cJSON_CreateArray_t)(void);

extern Q_cJSON * Q_cJSON_CreateObject(void);
typedef Q_cJSON * (* _api_Q_cJSON_CreateObject_t)(void);

extern Q_cJSON * Q_cJSON_CreateStringReference(const char *string);
typedef Q_cJSON * (* _api_Q_cJSON_CreateStringReference_t)(const char *string);

extern Q_cJSON * Q_cJSON_CreateObjectReference(const Q_cJSON *child);
typedef Q_cJSON * (* _api_Q_cJSON_CreateObjectReference_t)(const Q_cJSON *child);

extern Q_cJSON * Q_cJSON_CreateArrayReference(const Q_cJSON *child);
typedef Q_cJSON * (* _api_Q_cJSON_CreateArrayReference_t)(const Q_cJSON *child);

extern Q_cJSON * Q_cJSON_CreateIntArray(const int *numbers, int count);
typedef Q_cJSON * (* _api_Q_cJSON_CreateIntArray_t)(const int *numbers, int count);

extern Q_cJSON * Q_cJSON_CreateFloatArray(const float *numbers, int count);
typedef Q_cJSON * (* _api_Q_cJSON_CreateFloatArray_t)(const float *numbers, int count);

extern Q_cJSON * Q_cJSON_CreateDoubleArray(const double *numbers, int count);
typedef Q_cJSON * (* _api_Q_cJSON_CreateDoubleArray_t)(const double *numbers, int count);

extern Q_cJSON * Q_cJSON_CreateStringArray(const char *const *strings, int count);
typedef Q_cJSON * (* _api_Q_cJSON_CreateStringArray_t)(const char *const *strings, int count);

extern cJSON_bool Q_cJSON_AddItemToArray(Q_cJSON *array, Q_cJSON *item);
typedef cJSON_bool (* _api_Q_cJSON_AddItemToArray_t)(Q_cJSON *array, Q_cJSON *item);

extern cJSON_bool Q_cJSON_AddItemToObject(Q_cJSON *object, const char *string, Q_cJSON *item);
typedef cJSON_bool (* _api_Q_cJSON_AddItemToObject_t)(Q_cJSON *object, const char *string, Q_cJSON *item);

extern cJSON_bool Q_cJSON_AddItemToObjectCS(Q_cJSON *object, const char *string, Q_cJSON *item);
typedef cJSON_bool (* _api_Q_cJSON_AddItemToObjectCS_t)(Q_cJSON *object, const char *string, Q_cJSON *item);

extern cJSON_bool Q_cJSON_AddItemReferenceToArray(Q_cJSON *array, Q_cJSON *item);
typedef cJSON_bool (* _api_Q_cJSON_AddItemReferenceToArray_t)(Q_cJSON *array, Q_cJSON *item);

extern cJSON_bool Q_cJSON_AddItemReferenceToObject(Q_cJSON *object, const char *string, Q_cJSON *item);
typedef cJSON_bool (* _api_Q_cJSON_AddItemReferenceToObject_t)(Q_cJSON *object, const char *string, Q_cJSON *item);

extern Q_cJSON * Q_cJSON_DetachItemViaPointer(Q_cJSON *parent, Q_cJSON * const item);
typedef Q_cJSON * (* _api_Q_cJSON_DetachItemViaPointer_t)(Q_cJSON *parent, Q_cJSON * const item);

extern Q_cJSON * Q_cJSON_DetachItemFromArray(Q_cJSON *array, int which);
typedef Q_cJSON * (* _api_Q_cJSON_DetachItemFromArray_t)(Q_cJSON *array, int which);

extern void Q_cJSON_DeleteItemFromArray(Q_cJSON *array, int which);
typedef void (* _api_Q_cJSON_DeleteItemFromArray_t)(Q_cJSON *array, int which);

extern Q_cJSON * Q_cJSON_DetachItemFromObject(Q_cJSON *object, const char *string);
typedef Q_cJSON * (* _api_Q_cJSON_DetachItemFromObject_t)(Q_cJSON *object, const char *string);

extern Q_cJSON * Q_cJSON_DetachItemFromObjectCaseSensitive(Q_cJSON *object, const char *string);
typedef Q_cJSON * (* _api_Q_cJSON_DetachItemFromObjectCaseSensitive_t)(Q_cJSON *object, const char *string);

extern void Q_cJSON_DeleteItemFromObject(Q_cJSON *object, const char *string);
typedef void (* _api_Q_cJSON_DeleteItemFromObject_t)(Q_cJSON *object, const char *string);

extern void Q_cJSON_DeleteItemFromObjectCaseSensitive(Q_cJSON *object, const char *string);
typedef void (* _api_Q_cJSON_DeleteItemFromObjectCaseSensitive_t)(Q_cJSON *object, const char *string);

extern cJSON_bool Q_cJSON_InsertItemInArray(Q_cJSON *array, int which, Q_cJSON *newitem);
typedef cJSON_bool (* _api_Q_cJSON_InsertItemInArray_t)(Q_cJSON *array, int which, Q_cJSON *newitem);

extern cJSON_bool Q_cJSON_ReplaceItemViaPointer(Q_cJSON * const parent, Q_cJSON * const item, Q_cJSON * replacement);
typedef cJSON_bool (* _api_Q_cJSON_ReplaceItemViaPointer_t)(Q_cJSON * const parent, Q_cJSON * const item, Q_cJSON * replacement);

extern cJSON_bool Q_cJSON_ReplaceItemInArray(Q_cJSON *array, int which, Q_cJSON *newitem);
typedef cJSON_bool (* _api_Q_cJSON_ReplaceItemInArray_t)(Q_cJSON *array, int which, Q_cJSON *newitem);

extern cJSON_bool Q_cJSON_ReplaceItemInObject(Q_cJSON *object,const char *string,Q_cJSON *newitem);
typedef cJSON_bool (* _api_Q_cJSON_ReplaceItemInObject_t)(Q_cJSON *object,const char *string,Q_cJSON *newitem);

extern cJSON_bool Q_cJSON_ReplaceItemInObjectCaseSensitive(Q_cJSON *object,const char *string,Q_cJSON *newitem);
typedef cJSON_bool (* _api_Q_cJSON_ReplaceItemInObjectCaseSensitive_t)(Q_cJSON *object,const char *string,Q_cJSON *newitem);

extern Q_cJSON * Q_cJSON_Duplicate(const Q_cJSON *item, cJSON_bool recurse);
typedef Q_cJSON * (* _api_Q_cJSON_Duplicate_t)(const Q_cJSON *item, cJSON_bool recurse);

extern cJSON_bool Q_cJSON_Compare(const Q_cJSON * const a, const Q_cJSON * const b, const cJSON_bool case_sensitive);
typedef cJSON_bool (* _api_Q_cJSON_Compare_t)(const Q_cJSON * const a, const Q_cJSON * const b, const cJSON_bool case_sensitive);

extern void Q_cJSON_Minify(char *json);
typedef void (* _api_Q_cJSON_Minify_t)(char *json);

extern Q_cJSON* Q_cJSON_AddNullToObject(Q_cJSON * const object, const char * const name);
typedef Q_cJSON* (* _api_Q_cJSON_AddNullToObject_t)(Q_cJSON * const object, const char * const name);

extern Q_cJSON* Q_cJSON_AddTrueToObject(Q_cJSON * const object, const char * const name);
typedef Q_cJSON* (* _api_Q_cJSON_AddTrueToObject_t)(Q_cJSON * const object, const char * const name);

extern Q_cJSON* Q_cJSON_AddFalseToObject(Q_cJSON * const object, const char * const name);
typedef Q_cJSON* (* _api_Q_cJSON_AddFalseToObject_t)(Q_cJSON * const object, const char * const name);

extern Q_cJSON* Q_cJSON_AddBoolToObject(Q_cJSON * const object, const char * const name, const cJSON_bool boolean);
typedef Q_cJSON* (* _api_Q_cJSON_AddBoolToObject_t)(Q_cJSON * const object, const char * const name, const cJSON_bool boolean);

extern Q_cJSON* Q_cJSON_AddNumberToObject(Q_cJSON * const object, const char * const name, const double number);
typedef Q_cJSON* (* _api_Q_cJSON_AddNumberToObject_t)(Q_cJSON * const object, const char * const name, const double number);

extern Q_cJSON* Q_cJSON_AddStringToObject(Q_cJSON * const object, const char * const name, const char * const string);
typedef Q_cJSON* (* _api_Q_cJSON_AddStringToObject_t)(Q_cJSON * const object, const char * const name, const char * const string);

extern Q_cJSON* Q_cJSON_AddRawToObject(Q_cJSON * const object, const char * const name, const char * const raw);
typedef Q_cJSON* (* _api_Q_cJSON_AddRawToObject_t)(Q_cJSON * const object, const char * const name, const char * const raw);

extern Q_cJSON* Q_cJSON_AddObjectToObject(Q_cJSON * const object, const char * const name);
typedef Q_cJSON* (* _api_Q_cJSON_AddObjectToObject_t)(Q_cJSON * const object, const char * const name);

extern Q_cJSON* Q_cJSON_AddArrayToObject(Q_cJSON * const object, const char * const name);
typedef Q_cJSON* (* _api_Q_cJSON_AddArrayToObject_t)(Q_cJSON * const object, const char * const name);

extern double Q_cJSON_SetNumberHelper(Q_cJSON *object, double number);
typedef double (* _api_Q_cJSON_SetNumberHelper_t)(Q_cJSON *object, double number);

extern char* Q_cJSON_SetValuestring(Q_cJSON *object, const char *valuestring);
typedef char* (* _api_Q_cJSON_SetValuestring_t)(Q_cJSON *object, const char *valuestring);

extern void * Q_cJSON_malloc(qosa_size_t size);
typedef void * (* _api_Q_cJSON_malloc_t)(qosa_size_t size);

extern void Q_cJSON_free(void *object);
typedef void (* _api_Q_cJSON_free_t)(void *object);

extern qosa_q_type_t* qosa_q_init(qosa_q_type_t* q_ptr);
typedef qosa_q_type_t* (* _api_qosa_q_init_t)(qosa_q_type_t* q_ptr);

extern qosa_q_link_type_t* qosa_q_link(void* item_ptr, qosa_q_link_type_t* link_ptr);
typedef qosa_q_link_type_t* (* _api_qosa_q_link_t)(void* item_ptr, qosa_q_link_type_t* link_ptr);

extern void qosa_q_put(qosa_q_type_t* q_ptr, qosa_q_link_type_t* link_ptr);
typedef void (* _api_qosa_q_put_t)(qosa_q_type_t* q_ptr, qosa_q_link_type_t* link_ptr);

extern void* qosa_q_get(qosa_q_type_t* q_ptr);
typedef void* (* _api_qosa_q_get_t)(qosa_q_type_t* q_ptr);

extern void* qosa_q_last_get(qosa_q_type_t* q_ptr);
typedef void* (* _api_qosa_q_last_get_t)(qosa_q_type_t* q_ptr);

extern qosa_int32_t qosa_q_cnt(qosa_q_type_t* q_ptr);
typedef qosa_int32_t (* _api_qosa_q_cnt_t)(qosa_q_type_t* q_ptr);

extern void* qosa_q_check(qosa_q_type_t* q_ptr);
typedef void* (* _api_qosa_q_check_t)(qosa_q_type_t* q_ptr);

extern void* qosa_q_last_check(qosa_q_type_t* q_ptr);
typedef void* (* _api_qosa_q_last_check_t)(qosa_q_type_t* q_ptr);

extern void* qosa_q_next(qosa_q_type_t* q_ptr, qosa_q_link_type_t* link_ptr);
typedef void* (* _api_qosa_q_next_t)(qosa_q_type_t* q_ptr, qosa_q_link_type_t* link_ptr);

extern void* qosa_q_prev(qosa_q_type_t* q_ptr, qosa_q_link_type_t* link_ptr);
typedef void* (* _api_qosa_q_prev_t)(qosa_q_type_t* q_ptr, qosa_q_link_type_t* link_ptr);

extern void qosa_q_insert(qosa_q_type_t* q_ptr, qosa_q_link_type_t* q_insert_ptr, qosa_q_link_type_t* q_item_ptr);
typedef void (* _api_qosa_q_insert_t)(qosa_q_type_t* q_ptr, qosa_q_link_type_t* q_insert_ptr, qosa_q_link_type_t* q_item_ptr);

extern void* qosa_q_linear_search(qosa_q_type_t* q_ptr, qosa_q_compare_func compare_func, void* compare_val);
typedef void* (* _api_qosa_q_linear_search_t)(qosa_q_type_t* q_ptr, qosa_q_compare_func compare_func, void* compare_val);

extern void qosa_q_delete(qosa_q_type_t* q_ptr, qosa_q_link_type_t* q_delete_ptr);
typedef void (* _api_qosa_q_delete_t)(qosa_q_type_t* q_ptr, qosa_q_link_type_t* q_delete_ptr);

extern void qosa_q_destroy(qosa_q_type_t* q_ptr);
typedef void (* _api_qosa_q_destroy_t)(qosa_q_type_t* q_ptr);

extern void qosa_q_put_head(qosa_q_type_t* q_ptr, qosa_q_link_type_t* link_ptr);
typedef void (* _api_qosa_q_put_head_t)(qosa_q_type_t* q_ptr, qosa_q_link_type_t* link_ptr);

extern qosa_bool_t qosa_q_is_initialized(qosa_q_type_t* q_ptr);
typedef qosa_bool_t (* _api_qosa_q_is_initialized_t)(qosa_q_type_t* q_ptr);

extern void qosa_buffer_init(void);
typedef void (* _api_qosa_buffer_init_t)(void);

extern qosa_buffer_block_t* qosa_buffer_new_block(void);
typedef qosa_buffer_block_t* (* _api_qosa_buffer_new_block_t)(void);

extern qosa_buffer_block_t* qosa_buffer_free_block(qosa_buffer_block_t* item_ptr);
typedef qosa_buffer_block_t* (* _api_qosa_buffer_free_block_t)(qosa_buffer_block_t* item_ptr);

extern void qosa_buffer_free_packet(qosa_buffer_block_t** pkt_head_ptr);
typedef void (* _api_qosa_buffer_free_packet_t)(qosa_buffer_block_t** pkt_head_ptr);

extern qosa_uint32_t qosa_buffer_length_packet(qosa_buffer_block_t* item_ptr);
typedef qosa_uint32_t (* _api_qosa_buffer_length_packet_t)(qosa_buffer_block_t* item_ptr);

extern qosa_uint32_t qosa_buffer_tail_size(qosa_buffer_block_t* buf_ptr);
typedef qosa_uint32_t (* _api_qosa_buffer_tail_size_t)(qosa_buffer_block_t* buf_ptr);

extern qosa_uint32_t qosa_buffer_pushdown(qosa_buffer_block_t** pkt_head_ptr, void* buf, qosa_uint32_t size);
typedef qosa_uint32_t (* _api_qosa_buffer_pushdown_t)(qosa_buffer_block_t** pkt_head_ptr, void* buf, qosa_uint32_t size);

extern qosa_uint32_t qosa_buffer_pushdown_tail(qosa_buffer_block_t**     pkt_head_ptr,
    const void*            buf,
    qosa_uint32_t           size);
typedef qosa_uint32_t (* _api_qosa_buffer_pushdown_tail_t)(qosa_buffer_block_t**     pkt_head_ptr,
    const void*            buf,
    qosa_uint32_t           size);

extern void qosa_buffer_append(qosa_buffer_block_t** pkt_head_ptr,
    qosa_buffer_block_t** data_item_ptr);
typedef void (* _api_qosa_buffer_append_t)(qosa_buffer_block_t** pkt_head_ptr,
    qosa_buffer_block_t** data_item_ptr);

extern qosa_uint32_t qosa_buffer_pullup(qosa_buffer_block_t** pkt_head_ptr, void* buf, qosa_uint32_t cnt);
typedef qosa_uint32_t (* _api_qosa_buffer_pullup_t)(qosa_buffer_block_t** pkt_head_ptr, void* buf, qosa_uint32_t cnt);

extern qosa_uint32_t qosa_buffer_pullup_tail(qosa_buffer_block_t** pkt_head_ptr, void* buf, qosa_uint32_t cnt);
typedef qosa_uint32_t (* _api_qosa_buffer_pullup_tail_t)(qosa_buffer_block_t** pkt_head_ptr, void* buf, qosa_uint32_t cnt);

extern qosa_uint32_t qosa_buffer_extract(qosa_buffer_block_t* packet_ptr, qosa_uint32_t offset, void* buf, qosa_uint32_t len);
typedef qosa_uint32_t (* _api_qosa_buffer_extract_t)(qosa_buffer_block_t* packet_ptr, qosa_uint32_t offset, void* buf, qosa_uint32_t len);

extern qosa_uint32_t qosa_buffer_insert(qosa_buffer_block_t**     pkt_head_ptr,
    qosa_uint32_t           offset,
    void*                  buf,
    qosa_uint32_t           len);
typedef qosa_uint32_t (* _api_qosa_buffer_insert_t)(qosa_buffer_block_t**     pkt_head_ptr,
    qosa_uint32_t           offset,
    void*                  buf,
    qosa_uint32_t           len);

extern qosa_uint32_t qosa_buffer_dup_packet(qosa_buffer_block_t**     dup_ptr,
    qosa_buffer_block_t*      src_ptr,
    qosa_uint32_t           offset,
    qosa_uint32_t           cnt);
typedef qosa_uint32_t (* _api_qosa_buffer_dup_packet_t)(qosa_buffer_block_t**     dup_ptr,
    qosa_buffer_block_t*      src_ptr,
    qosa_uint32_t           offset,
    qosa_uint32_t           cnt);

extern qosa_uint32_t qosa_buffer_get_block_free_cnt(void);
typedef qosa_uint32_t (* _api_qosa_buffer_get_block_free_cnt_t)(void);

extern qosa_size_t qosa_utils_int_to_str(qosa_int64_t data, qosa_uint8_t *string, qosa_size_t length);
typedef qosa_size_t (*_api_qosa_utils_int_to_str_t)(qosa_int64_t data, qosa_uint8_t *string, qosa_size_t length);

extern qosa_size_t qosa_utils_uint_to_str(qosa_uint64_t data, qosa_uint8_t *string, qosa_size_t length);
typedef qosa_size_t (*_api_qosa_utils_uint_to_str_t)(qosa_uint64_t data, qosa_uint8_t *string, qosa_size_t length);

extern qosa_size_t qosa_utils_double_to_str(double data, qosa_uint8_t *string, qosa_size_t length);
typedef qosa_size_t (*_api_qosa_utils_double_to_str_t)(double data, qosa_uint8_t *string, qosa_size_t length);

extern qosa_size_t qosa_utils_double_to_str_with_precision(double data, qosa_uint8_t *string, qosa_size_t length, qosa_uint8_t precision);
typedef qosa_size_t (*_api_qosa_utils_double_to_str_with_precision_t)(double data, qosa_uint8_t *string, qosa_size_t length, qosa_uint8_t precision);

extern const qosa_value_str_map_t *qosa_Vs_map_find_by_istr(const qosa_value_str_map_t *vsmap, const char *str);
typedef const qosa_value_str_map_t *(* _api_qosa_Vs_map_find_by_istr_t)(const qosa_value_str_map_t *vsmap, const char *str);

extern const qosa_value_str_map_t *qosa_Vs_map_find_by_value(const qosa_value_str_map_t *vsmap, qosa_uint32_t value);
typedef const qosa_value_str_map_t *(* _api_qosa_Vs_map_find_by_value_t)(const qosa_value_str_map_t *vsmap, qosa_uint32_t value);

extern void qosa_buffer_queue_init(qosa_buffer_watermark_t* wm_ptr, qosa_uint32_t dne, qosa_q_type_t* queue);
typedef void (* _api_qosa_buffer_queue_init_t)(qosa_buffer_watermark_t* wm_ptr, qosa_uint32_t dne, qosa_q_type_t* queue);

extern void qosa_buffer_enqueue(qosa_buffer_watermark_t* wm_ptr, qosa_buffer_block_t** pkt_head_ptr);
typedef void (* _api_qosa_buffer_enqueue_t)(qosa_buffer_watermark_t* wm_ptr, qosa_buffer_block_t** pkt_head_ptr);

extern qosa_buffer_block_t* qosa_buffer_dequeue(qosa_buffer_watermark_t* wm_ptr);
typedef qosa_buffer_block_t* (* _api_qosa_buffer_dequeue_t)(qosa_buffer_watermark_t* wm_ptr);

extern void qosa_buffer_empty_queue(qosa_buffer_watermark_t* wm_ptr);
typedef void (* _api_qosa_buffer_empty_queue_t)(qosa_buffer_watermark_t* wm_ptr);

extern qosa_bool_t qosa_buffer_is_wm_empty(qosa_buffer_watermark_t* wm_ptr);
typedef qosa_bool_t (* _api_qosa_buffer_is_wm_empty_t)(qosa_buffer_watermark_t* wm_ptr);

extern void qosa_buffer_set_wm_cb(qosa_buffer_watermark_t*       wm_ptr,
    qosa_buffer_wm_cb_type_e memory_wm_cb_type,
    qosa_bool_t                   change_cb,
    qosa_wm_cb_ptr                cb,
    qosa_bool_t                   change_cb_data,
    void*                         cb_data,
    qosa_bool_t                   change_val,
    qosa_uint32_t                 val);
typedef void (* _api_qosa_buffer_set_wm_cb_t)(qosa_buffer_watermark_t*       wm_ptr,
    qosa_buffer_wm_cb_type_e memory_wm_cb_type,
    qosa_bool_t                   change_cb,
    qosa_wm_cb_ptr                cb,
    qosa_bool_t                   change_cb_data,
    void*                         cb_data,
    qosa_bool_t                   change_val,
    qosa_uint32_t                 val);

extern qosa_uint32_t qosa_buffer_queue_cnt(qosa_buffer_watermark_t* wm_ptr);
typedef qosa_uint32_t (* _api_qosa_buffer_queue_cnt_t)(qosa_buffer_watermark_t* wm_ptr);

extern void qosa_buffer_queue_destroy(qosa_buffer_watermark_t* wm_ptr);
typedef void (* _api_qosa_buffer_queue_destroy_t)(qosa_buffer_watermark_t* wm_ptr);

extern void qosa_dns_result_free(struct qosa_addrinfo_s *info);
typedef void (* _api_qosa_dns_result_free_t)(struct qosa_addrinfo_s *info);

extern qosa_dns_error_e qosa_dns_asyn_getaddrinfo(qosa_uint8_t simcid, qosa_uint8_t pdpcid, const char *hostname, struct qosa_addrinfo_s *hints, dns_result_callback cb, void *user_argv);
typedef qosa_dns_error_e (* _api_qosa_dns_asyn_getaddrinfo_t)(qosa_uint8_t simcid, qosa_uint8_t pdpcid, const char *hostname, struct qosa_addrinfo_s *hints, dns_result_callback cb, void *user_argv);

extern qosa_dns_error_e qosa_dns_syn_getaddrinfo(qosa_uint8_t simcid, qosa_uint8_t pdpcid, const char *hostname, struct qosa_addrinfo_s *hints, struct qosa_addrinfo_s **res);
typedef qosa_dns_error_e (* _api_qosa_dns_syn_getaddrinfo_t)(qosa_uint8_t simcid, qosa_uint8_t pdpcid, const char *hostname, struct qosa_addrinfo_s *hints, struct qosa_addrinfo_s **res);

extern qosa_int32_t qosa_vfs_umount(const char *path);
typedef qosa_int32_t (* _api_qosa_vfs_umount_t)(const char *path);

extern qosa_int32_t qosa_vfs_remount(const char *path, unsigned flags);
typedef qosa_int32_t (* _api_qosa_vfs_remount_t)(const char *path, unsigned flags);

extern qosa_int32_t qosa_vfs_creat(const char *path, qosa_int32_t mode);
typedef qosa_int32_t (* _api_qosa_vfs_creat_t)(const char *path, qosa_int32_t mode);

extern qosa_int32_t qosa_vfs_open(const char *path, qosa_int32_t flags);
typedef qosa_int32_t (* _api_qosa_vfs_open_t)(const char *path, qosa_int32_t flags);

extern qosa_int32_t qosa_vfs_close(qosa_int32_t fd);
typedef qosa_int32_t (* _api_qosa_vfs_close_t)(qosa_int32_t fd);

extern qosa_ssize_t qosa_vfs_read(qosa_int32_t fd, void *buf, qosa_size_t count);
typedef qosa_ssize_t (* _api_qosa_vfs_read_t)(qosa_int32_t fd, void *buf, qosa_size_t count);

extern qosa_ssize_t qosa_vfs_write(qosa_int32_t fd, const void *buf, qosa_size_t count);
typedef qosa_ssize_t (* _api_qosa_vfs_write_t)(qosa_int32_t fd, const void *buf, qosa_size_t count);

extern qosa_int64_t qosa_vfs_lseek(qosa_int32_t fd, qosa_int64_t offset, qosa_int32_t whence);
typedef qosa_int64_t (* _api_qosa_vfs_lseek_t)(qosa_int32_t fd, qosa_int64_t offset, qosa_int32_t whence);

extern qosa_int32_t qosa_vfs_fstat(qosa_int32_t fd, struct qosa_vfs_stat_t *st);
typedef qosa_int32_t (* _api_qosa_vfs_fstat_t)(qosa_int32_t fd, struct qosa_vfs_stat_t *st);

extern qosa_int32_t qosa_vfs_stat(const char *path, struct qosa_vfs_stat_t *st);
typedef qosa_int32_t (* _api_qosa_vfs_stat_t)(const char *path, struct qosa_vfs_stat_t *st);

extern qosa_int32_t qosa_vfs_truncate(const char *path, long length);
typedef qosa_int32_t (* _api_qosa_vfs_truncate_t)(const char *path, long length);

extern qosa_int32_t qosa_vfs_ftruncate(qosa_int32_t fd, long length);
typedef qosa_int32_t (* _api_qosa_vfs_ftruncate_t)(qosa_int32_t fd, long length);

extern qosa_int32_t qosa_vfs_link(const char *oldpath, const char *newpath);
typedef qosa_int32_t (* _api_qosa_vfs_link_t)(const char *oldpath, const char *newpath);

extern qosa_int32_t qosa_vfs_unlink(const char *pathname);
typedef qosa_int32_t (* _api_qosa_vfs_unlink_t)(const char *pathname);

extern qosa_int32_t qosa_vfs_rename(const char *oldpath, const char *newpath);
typedef qosa_int32_t (* _api_qosa_vfs_rename_t)(const char *oldpath, const char *newpath);

extern qosa_int32_t qosa_vfs_fsync(qosa_int32_t fd);
typedef qosa_int32_t (* _api_qosa_vfs_fsync_t)(qosa_int32_t fd);

extern qosa_int32_t qosa_vfs_fcntl(qosa_int32_t fd, qosa_int32_t cmd);
typedef qosa_int32_t (* _api_qosa_vfs_fcntl_t)(qosa_int32_t fd, qosa_int32_t cmd);

extern qosa_int32_t qosa_vfs_ioctl(qosa_int32_t fd, unsigned long request);
typedef qosa_int32_t (* _api_qosa_vfs_ioctl_t)(qosa_int32_t fd, unsigned long request);

extern qosa_int32_t qosa_vfs_readdir_r(QOSA_VFS_DIR *dirp, struct qosa_vfs_dirent_t *entry, struct qosa_vfs_dirent_t **result);
typedef qosa_int32_t (* _api_qosa_vfs_readdir_r_t)(QOSA_VFS_DIR *dirp, struct qosa_vfs_dirent_t *entry, struct qosa_vfs_dirent_t **result);

extern long qosa_vfs_telldir(QOSA_VFS_DIR *pdir);
typedef long (* _api_qosa_vfs_telldir_t)(QOSA_VFS_DIR *pdir);

extern void qosa_vfs_seekdir(QOSA_VFS_DIR *pdir, long loc);
typedef void (* _api_qosa_vfs_seekdir_t)(QOSA_VFS_DIR *pdir, long loc);

extern void qosa_vfs_rewinddir(QOSA_VFS_DIR *pdir);
typedef void (* _api_qosa_vfs_rewinddir_t)(QOSA_VFS_DIR *pdir);

extern qosa_int32_t qosa_vfs_closedir(QOSA_VFS_DIR *pdir);
typedef qosa_int32_t (* _api_qosa_vfs_closedir_t)(QOSA_VFS_DIR *pdir);

extern qosa_int32_t qosa_vfs_mkdir(const char *pathname, qosa_int32_t mode);
typedef qosa_int32_t (* _api_qosa_vfs_mkdir_t)(const char *pathname, qosa_int32_t mode);

extern qosa_int32_t qosa_vfs_rmdir(const char *pathname);
typedef qosa_int32_t (* _api_qosa_vfs_rmdir_t)(const char *pathname);

extern qosa_int32_t qosa_vfs_chdir(const char *path);
typedef qosa_int32_t (* _api_qosa_vfs_chdir_t)(const char *path);

extern char *qosa_vfs_getcwd(char *buf, qosa_size_t size);
typedef char * (* _api_qosa_vfs_getcwd_t)(char *buf, qosa_size_t size);

extern char *qosa_vfs_resolve_path(const char *base_path, const char *path, char *resolved_path);
typedef char * (* _api_qosa_vfs_resolve_path_t)(const char *base_path, const char *path, char *resolved_path);

extern char *qosa_vfs_realpath(const char *path, char *resolved_path);
typedef char * (* _api_qosa_vfs_realpath_t)(const char *path, char *resolved_path);

extern void *qosa_vfs_fs_handle(const char *path);
typedef char * (* _api_qosa_vfs_fs_handle_t)(const char *path);

extern void *qosa_vfs_mount_handle(const char *path);
typedef char * (* _api_qosa_vfs_mount_handle_t)(const char *path);

extern qosa_int32_t qosa_vfs_statvfs(const char *path, struct qosa_vfs_statvfs_t *buf);
typedef qosa_int32_t (* _api_qosa_vfs_statvfs_t)(const char *path, struct qosa_vfs_statvfs_t *buf);

extern qosa_int32_t qosa_vfs_fstatvfs(qosa_int32_t fd, struct qosa_vfs_statvfs_t *buf);
typedef qosa_int32_t (* _api_qosa_vfs_fstatvfs_t)(qosa_int32_t fd, struct qosa_vfs_statvfs_t *buf);

extern qosa_int32_t qosa_vfs_mount_count(void);
typedef qosa_int32_t (* _api_qosa_vfs_mount_count_t)(void);

extern qosa_int32_t qosa_vfs_mount_points(char **mp, qosa_size_t count);
typedef qosa_int32_t (* _api_qosa_vfs_mount_points_t)(char **mp, qosa_size_t count);

extern qosa_int32_t qosa_vfs_mkpath(const char *path, qosa_int32_t mode);
typedef qosa_int32_t (* _api_qosa_vfs_mkpath_t)(const char *path, qosa_int32_t mode);

extern qosa_int32_t qosa_vfs_mkfilepath(const char *path, qosa_int32_t mode);
typedef qosa_int32_t (* _api_qosa_vfs_mkfilepath_t)(const char *path, qosa_int32_t mode);

extern qosa_ssize_t qosa_vfs_file_size(const char *path);
typedef qosa_ssize_t (* _api_qosa_vfs_file_size_t)(const char *path);

extern qosa_ssize_t qosa_vfs_file_read(const char *path, void *buf, qosa_size_t count);
typedef qosa_ssize_t (* _api_qosa_vfs_file_read_t)(const char *path, void *buf, qosa_size_t count);

extern qosa_ssize_t qosa_vfs_file_write(const char *path, const void *buf, qosa_size_t count);
typedef qosa_ssize_t (* _api_qosa_vfs_file_write_t)(const char *path, const void *buf, qosa_size_t count);

extern qosa_int32_t qosa_vfs_rmchildren(const char *path);
typedef qosa_int32_t (* _api_qosa_vfs_rmchildren_t)(const char *path);

extern qosa_int32_t qosa_vfs_rmdir_recursive(const char *path);
typedef qosa_int32_t (* _api_qosa_vfs_rmdir_recursive_t)(const char *path);

extern void qosa_vfs_umount_all(void);
typedef void (* _api_qosa_vfs_umount_all_t)(void);

extern qosa_int64_t qosa_vfs_dir_total_size(const char *path);
typedef qosa_int64_t (* _api_qosa_vfs_dir_total_size_t)(const char *path);

extern qosa_int32_t qosa_vfs_sync(qosa_int32_t fd);
typedef qosa_int32_t (* _api_qosa_vfs_sync_t)(qosa_int32_t fd);

extern int qosa_vfs_register(const char *base_path, const qosa_vfs_ops_t *ops, void *fs);
typedef int (* _api_qosa_vfs_register_t)(const char *base_path, const qosa_vfs_ops_t *ops, void *fs);

extern int qosa_vfs_unregister(const char *base_path);
typedef int (* _api_qosa_vfs_unregister_t)(const char *base_path);

extern qosa_int32_t qosa_ext_flash_register(qosa_ext_flash_t *ops);
typedef qosa_int32_t (* _api_qosa_ext_flash_register_t)(qosa_ext_flash_t *ops);

extern void qosa_regedit_at_qcfg_func(qosa_at_qcfg_func_ptr at_qcfg_func_ptr);
typedef void (*_api_qosa_regedit_at_qcfg_func_t)(qosa_at_qcfg_func_ptr at_qcfg_func_ptr);

extern void qosa_regedit_qcm_urc_flush_cache_func(qosa_qcm_urc_flush_cache_func_ptr urc_flush_cache_func_ptr);
typedef void (*_api_qosa_regedit_qcm_urc_flush_cache_func_t)(qosa_qcm_urc_flush_cache_func_ptr urc_flush_cache_func_ptr);

extern void qosa_regedit_qcm_urc_send_wait_complete_func(qosa_qcm_urc_send_wait_complete_ptr qcm_urc_send_wait_complete_ptr);
typedef void (*_api_qosa_regedit_qcm_urc_send_wait_complete_func_t)(qosa_qcm_urc_send_wait_complete_ptr qcm_urc_send_wait_complete_ptr);

extern void qosa_regedit_qcm_urc_send_report_func(qosa_qcm_urc_send_report_ptr urc_send_report_ptr);
typedef void (*_api_qosa_regedit_qcm_urc_send_report_func_t)(qosa_qcm_urc_send_report_ptr urc_send_report_ptr);

extern void qosa_regedit_qcm_urc_config_get_opt_func(qosa_qcm_urc_config_get_opt_ptr qcm_urc_config_get_opt_ptr);
typedef void (*_api_qosa_regedit_qcm_urc_config_get_opt_func_t)(qosa_qcm_urc_config_get_opt_ptr qcm_urc_config_get_opt_ptr);

extern qosa_bool_t qosa_at_is_parsing(qosa_at_dev_type_e      dev_port);
typedef qosa_bool_t (*_api_qosa_at_is_parsing_t)(qosa_at_dev_type_e dev_port);

extern qosa_adc_errcode_e qosa_adc_ioctl(qosa_adc_channel_e adc_id, qosa_adc_ioctl_cmd_e cmd, void *arg);
typedef qosa_adc_errcode_e (*_api_qosa_adc_ioctl_t)(qosa_adc_channel_e adc_id, qosa_adc_ioctl_cmd_e cmd, void *arg);

extern qosa_adc_errcode_e qosa_adc_get_volt(qosa_adc_channel_e adc_id, int *adc_value);
typedef qosa_adc_errcode_e (*_api_qosa_adc_get_volt_t)(qosa_adc_channel_e adc_id, int *adc_value);

extern qosa_adc_errcode_e qosa_get_vbat_volt(int *adc_value);
typedef qosa_adc_errcode_e (*_api_qosa_get_vbat_volt_t)(int *adc_value);

extern qosa_uint8_t qosa_adc_get_channel_num(void);
typedef qosa_uint8_t (*_api_qosa_adc_get_channel_num_t)(void);

extern qosa_uint32_t qosa_get_system_tick_cnt(void);
typedef qosa_uint32_t (*_api_qosa_get_system_tick_cnt_t)(void);

extern qosa_uint32_t qosa_get_running_time(void);
typedef qosa_uint32_t (*_api_qosa_get_running_time_t)(void);

extern qosa_rtc_error_e qosa_rtc_set_alarm(qosa_rtc_time_t *rtc_time);
typedef qosa_rtc_error_e (*_api_qosa_rtc_set_alarm_t)(qosa_rtc_time_t *rtc_time);

extern qosa_rtc_error_e qosa_rtc_get_alarm(qosa_rtc_time_t *rtc_time);
typedef qosa_rtc_error_e (*_api_qosa_rtc_get_alarm_t)(qosa_rtc_time_t *rtc_time);

extern qosa_rtc_error_e qosa_rtc_enable_alarm(qosa_uint8_t on_off);
typedef qosa_rtc_error_e (*_api_qosa_rtc_enable_alarm_t)(qosa_uint8_t on_off);

extern qosa_rtc_error_e qosa_rtc_register_cb(qosa_rtc_cb_ptr cb);
typedef qosa_rtc_error_e (*_api_qosa_rtc_register_cb_t)(qosa_rtc_cb_ptr cb);

extern qosa_rtc_error_e qosa_rtc_set_cfg(qosa_rtc_cfg_t *cfg);
typedef qosa_rtc_error_e (*_api_qosa_rtc_set_cfg_t)(qosa_rtc_cfg_t *cfg);

extern qosa_rtc_error_e qosa_rtc_get_cfg(qosa_rtc_cfg_t *cfg);
typedef qosa_rtc_error_e (*_api_qosa_rtc_get_cfg_t)(qosa_rtc_cfg_t *cfg);

extern qosa_rtc_error_e qosa_rtc_print_time(qosa_rtc_time_t *rtc_time);
typedef qosa_rtc_error_e (*_api_qosa_rtc_print_time_t)(qosa_rtc_time_t *rtc_time);

extern qosa_lpm_error_e qosa_lpm_config_set(qosa_lpm_config_t *info);
typedef qosa_lpm_error_e (*_api_qosa_lpm_config_set_t)(qosa_lpm_config_t *info);

extern void qosa_lpm_config_get(qosa_lpm_config_t *info);
typedef void (*_api_qosa_lpm_config_get_t)(qosa_lpm_config_t *info);

extern qosa_lpm_error_e qosa_lpm_app_vote_new_handle(const char *name, qosa_handle *handle_id);
typedef qosa_lpm_error_e (*_api_qosa_lpm_app_vote_new_handle_t)(const char *name, qosa_handle *handle_id);

extern qosa_lpm_error_e qosa_lpm_app_vote_enable(qosa_handle handle_id);
typedef qosa_lpm_error_e (*_api_qosa_lpm_app_vote_enable_t)(qosa_handle handle_id);

extern qosa_lpm_error_e qosa_lpm_app_vote_disable(qosa_handle handle_id);
typedef qosa_lpm_error_e (*_api_qosa_lpm_app_vote_disable_t)(qosa_handle handle_id);

extern qosa_lpm_error_e qosa_lpm_app_vote_del_handle(qosa_handle handle_id);
typedef qosa_lpm_error_e (*_api_qosa_lpm_app_vote_del_handle_t)(qosa_handle handle_id);

extern qosa_lpm_error_e qosa_lpm_wakeup_pin_init(qosa_uint8_t wakeup_pin_num,qosa_lpm_wakeup_pin_pull_e wakeup_pin_pull, qosa_lpm_wakeup_pin_trigger_e wakeup_pin_edge,qosa_wakeup_pin_interrupt_cb wakeup_pin_interrupt_cb,void *cb_ctx);
typedef qosa_lpm_error_e (*_api_qosa_lpm_wakeup_pin_init_t)(qosa_uint8_t wakeup_pin_num,qosa_lpm_wakeup_pin_pull_e wakeup_pin_pull, qosa_lpm_wakeup_pin_trigger_e wakeup_pin_edge,qosa_wakeup_pin_interrupt_cb wakeup_pin_interrupt_cb,void *cb_ctx);

extern qosa_lpm_error_e qosa_lpm_wakeup_pin_get_level(qosa_uint8_t wakeup_pin_num, qosa_lpm_wakeup_pin_level_e *level);
typedef qosa_lpm_error_e (*_api_qosa_lpm_wakeup_pin_get_level_t)(qosa_uint8_t wakeup_pin_num, qosa_lpm_wakeup_pin_level_e *level);

extern qosa_lpm_error_e qosa_lpm_wakeup_pin_deinit(qosa_uint8_t wakeup_pin_num);
typedef qosa_lpm_error_e (*_api_qosa_lpm_wakeup_pin_deinit_t)(qosa_uint8_t wakeup_pin_num);

extern int qppp_establish_req(qppp_establish_param_t* req);
typedef int (*_api_qppp_establish_req_t)(qppp_establish_param_t* req);

extern int qppp_terminate_req(qppp_terminate_param_t* ter);
typedef int (*_api_qppp_terminate_req_t)(qppp_terminate_param_t* ter);

extern int qppp_set_termframe_flag(qosa_uint8_t flag);
typedef int (*_api_qppp_set_termframe_flag_t)(qosa_uint8_t flag);

extern qosa_uint8_t qppp_get_termframe_flag(void);
typedef qosa_uint8_t (*_api_qppp_get_termframe_flag_t)(void);

extern uint32_t osTaskSetTimeOutState(uint32_t *curTimeTick);
typedef uint32_t (*_api_osTaskSetTimeOutState_t)(uint32_t *curTimeTick);

extern uint32_t osTaskCheckForTimeOut(uint32_t *curTimeTick, uint32_t *timeOutTick);
typedef uint32_t (*_api_osTaskCheckForTimeOut_t)(uint32_t *curTimeTick, uint32_t *timeOutTick);

extern qosa_ftm_err_e qosa_ftm_register_callback(qosa_ftm_callback callback, void *user_data);
typedef qosa_ftm_err_e (*_api_qosa_ftm_register_callback_t)(qosa_ftm_callback callback, void *user_data);

extern qosa_ftm_err_e qosa_ftm_unregister_callback(qosa_ftm_callback callback, void *user_data);
typedef qosa_ftm_err_e (*_api_qosa_ftm_unregister_callback_t)(qosa_ftm_callback callback, void *user_data);

extern qosa_ftm_err_e qosa_ftm_set_mode(qosa_uint32_t mode);
typedef qosa_ftm_err_e (*_api_qosa_ftm_set_mode_t)(qosa_uint32_t mode);

extern qosa_ftm_err_e qosa_ftm_get_mode(qosa_uint32_t *mode);
typedef qosa_ftm_err_e (*_api_qosa_ftm_get_mode_t)(qosa_uint32_t *mode);

extern qosa_ftm_err_e qosa_ftm_tx_set(qosa_ftm_tx_config_t *config);
typedef qosa_ftm_err_e (*_api_qosa_ftm_tx_set_t)(qosa_ftm_tx_config_t *config);

extern qosa_ftm_err_e qosa_ftm_rx_set(qosa_ftm_rx_config_t *config);
typedef qosa_ftm_err_e (*_api_qosa_ftm_rx_set_t)(qosa_ftm_rx_config_t *config);

extern qosa_sms_err_e qosa_sms_get_config(qosa_uint8_t simid, qosa_sms_cfg_t *cfg);
typedef qosa_sms_err_e (*_api_qosa_sms_get_config_t)(qosa_uint8_t simid, qosa_sms_cfg_t *cfg);

extern qosa_sms_err_e qosa_sms_set_config(qosa_uint8_t simid, qosa_sms_cfg_t *cfg);
typedef qosa_sms_err_e (*_api_qosa_sms_set_config_t)(qosa_uint8_t simid, qosa_sms_cfg_t *cfg);

extern qosa_sms_err_e qosa_sms_text_to_pdu(qosa_sms_msg_t *msg, qosa_sms_record_t *record);
typedef qosa_sms_err_e (*_api_qosa_sms_text_to_pdu_t)(qosa_sms_msg_t *msg, qosa_sms_record_t *record);

extern qosa_sms_err_e qosa_sms_pdu_to_text(qosa_sms_record_t *record, qosa_sms_msg_t *msg);
typedef qosa_sms_err_e (*_api_qosa_sms_pdu_to_text_t)(qosa_sms_record_t *record, qosa_sms_msg_t *msg);

extern qosa_sms_err_e qosa_sms_send_pdu_async(qosa_uint8_t simid, qosa_sms_send_param_t *send, sms_callback_t cb, void *ctx);
typedef qosa_sms_err_e (*_api_qosa_sms_send_pdu_async_t)(qosa_uint8_t simid, qosa_sms_send_param_t *send, sms_callback_t cb, void *ctx);

extern qosa_sms_err_e qosa_sms_send_spec_pdu_async(qosa_uint8_t simid, qosa_sms_send_spec_param_t *send, sms_callback_t cb, void *ctx);
typedef qosa_sms_err_e (*_api_qosa_sms_send_spec_pdu_async_t)(qosa_uint8_t simid, qosa_sms_send_spec_param_t *send, sms_callback_t cb, void *ctx);

extern qosa_sms_err_e qosa_sms_write_pdu_async(qosa_uint8_t simid, qosa_sms_write_param_t *write, sms_callback_t cb, void *ctx);
typedef qosa_sms_err_e (*_api_qosa_sms_write_pdu_async_t)(qosa_uint8_t simid, qosa_sms_write_param_t *write, sms_callback_t cb, void *ctx);

extern qosa_sms_err_e qosa_sms_read_pdu_async(qosa_uint8_t simid, qosa_sms_read_param_t *read, sms_callback_t cb, void *ctx);
typedef qosa_sms_err_e (*_api_qosa_sms_read_pdu_async_t)(qosa_uint8_t simid, qosa_sms_read_param_t *read, sms_callback_t cb, void *ctx);

extern qosa_sms_err_e qosa_sms_delete_async(qosa_uint8_t simid, qosa_sms_delete_param_t *delete, sms_callback_t cb, void *ctx);
typedef qosa_sms_err_e (*_api_qosa_sms_delete_async_t)(qosa_uint8_t simid, qosa_sms_delete_param_t *delete, sms_callback_t cb, void *ctx);

extern qosa_sms_err_e qosa_sms_get_stor_info(qosa_uint8_t simid, qosa_sms_stor_info_t *info);
typedef qosa_sms_err_e (*_api_qosa_sms_get_stor_info_t)(qosa_uint8_t simid, qosa_sms_stor_info_t *info);

extern qosa_sms_err_e qosa_sms_get_used_record(qosa_uint8_t simid, qosa_sms_stor_e storage, qosa_uint16_t *record_list, qosa_uint16_t *size);
typedef qosa_sms_err_e (*_api_qosa_sms_get_used_record_t)(qosa_uint8_t simid, qosa_sms_stor_e storage, qosa_uint16_t *record_list, qosa_uint16_t *size);

extern qosa_sms_err_e qosa_sms_get_spec_record(qosa_uint8_t simid, qosa_sms_stor_e storage, qosa_sms_status_e status, qosa_uint16_t *record_list, qosa_uint16_t *size);
typedef qosa_sms_err_e (*_api_qosa_sms_get_spec_record_t)(qosa_uint8_t simid, qosa_sms_stor_e storage, qosa_sms_status_e status, qosa_uint16_t *record_list, qosa_uint16_t *size);

extern qosa_sms_err_e qosa_sms_new_message_ack(qosa_uint8_t simid, qosa_sms_rp_type_e ack_type);
typedef qosa_sms_err_e (*_api_qosa_sms_new_message_ack_t)(qosa_uint8_t simid, qosa_sms_rp_type_e ack_type);

extern qosa_sms_err_e qosa_sms_set_sca(qosa_uint8_t simid, qosa_sms_address_info_t *sca);
typedef qosa_sms_err_e (*_api_qosa_sms_set_sca_t)(qosa_uint8_t simid, qosa_sms_address_info_t *sca);

extern qosa_sms_err_e qosa_sms_get_sca(qosa_uint8_t simid, qosa_sms_address_info_t *sca);
typedef qosa_sms_err_e (*_api_qosa_sms_get_sca_t)(qosa_uint8_t simid, qosa_sms_address_info_t *sca);

extern qosa_sms_err_e qosa_sms_set_more_msg_mode(qosa_uint8_t simid, qosa_sms_more_msg_mode_e mode);
typedef qosa_sms_err_e (*_api_qosa_sms_set_more_msg_mode_t)(qosa_uint8_t simid, qosa_sms_more_msg_mode_e mode);

extern qosa_sms_err_e qosa_sms_get_more_msg_mode(qosa_uint8_t simid, qosa_sms_more_msg_mode_e* mode);
typedef qosa_sms_err_e (*_api_qosa_sms_get_more_msg_mode_t)(qosa_uint8_t simid, qosa_sms_more_msg_mode_e* mode);

extern qosa_sms_err_e qosa_sms_address_to_text(qosa_sms_address_info_t *address, char *text, qosa_uint16_t len);
typedef qosa_sms_err_e (*_api_qosa_sms_address_to_text_t)(qosa_sms_address_info_t *address, char *text, qosa_uint16_t len);

extern qosa_sms_err_e qosa_sms_text_to_address(char *text, qosa_uint16_t len, qosa_uint8_t address_type, qosa_sms_address_info_t *address);
typedef qosa_sms_err_e (*_api_qosa_sms_text_to_address_t)(char *text, qosa_uint16_t len, qosa_uint8_t address_type, qosa_sms_address_info_t *address);

extern int qosa_virt_at_send(char *at_cmd, int at_cmd_len);
typedef int (*_api_qosa_virt_at_send_t)(char *at_cmd, int at_cmd_len);

extern void qosa_virt_at_result_func(const char *data, qosa_uint32_t size);
typedef void (*_api_qosa_virt_at_result_func_t)(const char *data, qosa_uint32_t size);

extern void qosa_regedit_qcm_virt_at_result_func(qosa_qcm_virt_at_result_func_ptr virt_at_result_func_ptr);
typedef void (*_api_qosa_regedit_qcm_virt_at_result_func_t)(qosa_qcm_virt_at_result_func_ptr virt_at_result_func_ptr);

extern qosa_bool_t qosa_register_watermark_callback(wm_item_report_cb_t cb, void *p_arg);
typedef qosa_bool_t (*_api_qosa_register_watermark_callback_t)(wm_item_report_cb_t cb, void *p_arg);

extern void qosa_set_app_runing_start_addr(qosa_uint32_t qos_app_flash_start_addr, qosa_uint32_t qos_app_ram_start_addr);
typedef void (*_api_qosa_set_app_runing_start_addr_t)(qosa_uint32_t qos_app_flash_start_addr, qosa_uint32_t qos_app_ram_start_addr);

extern qosa_sms_err_e qosa_sms_set_charset(qosa_at_chset_e charset);
typedef qosa_sms_err_e (*_api_qosa_sms_set_charset_t)(qosa_at_chset_e charset);

extern qosa_int32_t qosa_can_get_capabilities(qosa_can_dev_num_e dev, qosa_can_capabilities_t* temp);
typedef qosa_int32_t (*_api_qosa_can_get_capabilities_t)(qosa_can_dev_num_e dev, qosa_can_capabilities_t* temp);

extern int32_t qosa_can_init(qosa_can_dev_num_e dev, qosa_can_signal_unit_event_t cb_unit_event, qosa_can_signal_object_event_t cb_object_event);
typedef int32_t (*_api_qosa_can_init_t)(qosa_can_dev_num_e dev, qosa_can_signal_unit_event_t cb_unit_event, qosa_can_signal_object_event_t cb_object_event);

extern int32_t qosa_can_uninit(qosa_can_dev_num_e dev);
typedef int32_t (*_api_qosa_can_uninit_t)(qosa_can_dev_num_e dev);

extern int32_t qosa_can_set_power(qosa_can_dev_num_e dev, qosa_can_power_state_e state);
typedef int32_t (*_api_qosa_can_set_power_t)(qosa_can_dev_num_e dev, qosa_can_power_state_e state);

extern uint32_t qosa_can_get_clk(qosa_can_dev_num_e dev);
typedef uint32_t (*_api_qosa_can_get_clk_t)(qosa_can_dev_num_e dev);

extern int32_t qosa_can_set_bitrate(qosa_can_dev_num_e dev, qosa_can_bitrate_t* bitrate_ptr);
typedef int32_t (*_api_qosa_can_set_bitrate_t)(qosa_can_dev_num_e dev, qosa_can_bitrate_t* bitrate_ptr);

extern int32_t qosa_can_set_mode(qosa_can_dev_num_e dev, qosa_can_mode_e mode);
typedef int32_t (*_api_qosa_can_set_mode_t)(qosa_can_dev_num_e dev, qosa_can_mode_e mode);

extern qosa_int32_t qosa_can_get_obj_capabilities(qosa_can_dev_num_e dev, qosa_can_obj_capabilities_t* temp);
typedef qosa_int32_t (*_api_qosa_can_get_obj_capabilities_t)(qosa_can_dev_num_e dev, qosa_can_obj_capabilities_t* temp);

extern int32_t qosa_can_set_obj_filter(qosa_can_dev_num_e dev, qosa_can_obj_filter_t* can_obj_filter);
typedef int32_t (*_api_qosa_can_set_obj_filter_t)(qosa_can_dev_num_e dev, qosa_can_obj_filter_t* can_obj_filter);

extern int32_t qosa_can_set_obj_config(qosa_can_dev_num_e dev, qosa_can_obj_config_t* can_obj_config);
typedef int32_t (*_api_qosa_can_set_obj_config_t)(qosa_can_dev_num_e dev, qosa_can_obj_config_t* can_obj_config);

extern int32_t qosa_can_write(qosa_can_dev_num_e dev, uint32_t obj_idx, qosa_can_msg_info_t *msg_info, const uint8_t *data, uint8_t size);
typedef int32_t (*_api_qosa_can_write_t)(qosa_can_dev_num_e dev, uint32_t obj_idx, qosa_can_msg_info_t *msg_info, const uint8_t *data, uint8_t size);

extern int32_t qosa_can_read(qosa_can_dev_num_e dev, uint32_t obj_idx, qosa_can_msg_info_t *msg_info, uint8_t *data, uint8_t size);
typedef int32_t (*_api_qosa_can_read_t)(qosa_can_dev_num_e dev, uint32_t obj_idx, qosa_can_msg_info_t *msg_info, uint8_t *data, uint8_t size);

extern int32_t qosa_can_control(qosa_can_dev_num_e dev, qosa_can_control_t* can_control);
typedef int32_t (*_api_qosa_can_control_t)(qosa_can_dev_num_e dev, qosa_can_control_t* can_control);

extern qosa_int32_t qosa_can_get_status(qosa_can_dev_num_e dev, qosa_can_status_t* temp);
typedef qosa_int32_t (*_api_qosa_can_get_status_t)(qosa_can_dev_num_e dev, qosa_can_status_t* temp);

extern int32_t qosa_can_get_rx_message_count(qosa_can_dev_num_e dev, uint32_t obj_idx);
typedef int32_t (*_api_qosa_can_get_rx_message_count_t)(qosa_can_dev_num_e dev, uint32_t obj_idx);

extern qosa_sms_err_e qosa_sms_utf8_to_ucs2(const char* utf8_txt, char* ucs2_character, qosa_size_t buffer_size);
typedef qosa_sms_err_e (*_api_qosa_sms_utf8_to_ucs2_t)(const char* utf8_txt, char* ucs2_character, qosa_size_t buffer_size);

extern int qosa_start_voice_call(qosa_uint8_t simid, const char *phone_number, ims_callback_t cb, void *ctx);
typedef int (* _api_qosa_start_voice_call_t)(qosa_uint8_t simid, const char *phone_number, ims_callback_t cb, void *ctx);

extern int qosa_stop_voice_call(qosa_uint8_t simid, ims_callback_t cb, void *ctx);
typedef int (* _api_qosa_stop_voice_call_t)(qosa_uint8_t simid, ims_callback_t cb, void *ctx);

extern int qosa_answer_call(qosa_uint8_t simid, ims_callback_t cb, void *ctx);
typedef int (* _api_qosa_answer_call_t)(qosa_uint8_t simid, ims_callback_t cb, void *ctx);

extern qosa_int32_t qosa_usb_hid_send_proc(qosa_uint8_t hid_num, qosa_uint8_t* sndbuf_ptr, qosa_uint32_t size);
typedef qosa_int32_t (*_api_qosa_usb_hid_send_proc_t)(qosa_uint8_t hid_num, qosa_uint8_t* sndbuf_ptr, qosa_uint32_t size);

extern qosa_int32_t qosa_usb_hidx_isconnect(qosa_uint8_t hid_num);
typedef qosa_int32_t (*_api_qosa_usb_hidx_isconnect_t)(qosa_uint8_t hid_num);

extern qosa_int32_t qosa_usb_config_hid(qosa_usb_hid_mode_e mode);
typedef qosa_int32_t (*_api_qosa_usb_config_hid_t)(qosa_usb_hid_mode_e mode);

extern qosa_clat_errcode_e qosa_clat_set_cfg(int profile_id, qosa_clat_config_t *cfg);
typedef qosa_clat_errcode_e (* _api_qosa_clat_set_cfg_t)(int profile_id, qosa_clat_config_t *cfg);

extern qosa_clat_errcode_e qosa_clat_get_cfg(int profile_id, qosa_clat_config_t *cfg);
typedef qosa_clat_errcode_e (* _api_qosa_clat_get_cfg_t)(int profile_id, qosa_clat_config_t *cfg);

extern qosa_pinctrl_error_e qosa_pinctrl_set_option(qosa_pin_num_e pin_num, pinctrl_option_t option);
typedef qosa_pinctrl_error_e (* _api_qosa_pinctrl_set_option_t)(qosa_pin_num_e pin_num, pinctrl_option_t option);

extern qosa_pinctrl_error_e qosa_pin_set_func(qosa_pin_num_e pin_num, qosa_uint8_t func_sel);
typedef qosa_pinctrl_error_e (* _api_qosa_pin_set_func_t)(qosa_pin_num_e pin_num, qosa_uint8_t func_sel);

extern qosa_pinctrl_error_e qosa_pin_get_func(qosa_pin_num_e pin_num, qosa_uint8_t *func_sel);
typedef qosa_pinctrl_error_e (* _api_qosa_pin_get_func_t)(qosa_pin_num_e pin_num, qosa_uint8_t *func_sel);
extern void qosa_mp3dec_vpu_init(void);
typedef void (* _api_qosa_mp3dec_vpu_init_t)(void);
extern void qosa_mp3_synth_full_vpu(struct mad_synth *synth, struct mad_frame const *frame, unsigned int nch, unsigned int ns);
typedef void (* _api_qosa_mp3_synth_full_vpu_t)(struct mad_synth *synth, struct mad_frame const *frame, unsigned int nch, unsigned int ns);

extern qosa_int32_t qosa_cpu_usage_start(qosa_uint32_t period_ms, qosa_cpu_usage_cb_t cb);
typedef qosa_int32_t (*_api_qosa_cpu_usage_start_t)(qosa_uint32_t period_ms, qosa_cpu_usage_cb_t cb);

extern qosa_int32_t qosa_cpu_usage_stop(void);
typedef qosa_int32_t (*_api_qosa_cpu_usage_stop_t)(void);

#ifdef __cplusplus
}
#endif

#endif /* _UNIRTOS_SYS_H_ */
