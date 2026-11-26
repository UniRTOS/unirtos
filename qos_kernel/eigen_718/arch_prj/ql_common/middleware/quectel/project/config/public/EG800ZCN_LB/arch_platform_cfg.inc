#BIN_COMPRESS for post bin compress tool
#THIRDPARTY_LZMA_ENABLE LZMA feature compile en
#RAMCODE_COMPRESS_EN ramcode/RW decompress, used in code
#REL_COMPRESS_EN calibration back up compress en, used in code
CFLAGS_DEFS += -DRAMCODE_COMPRESS_EN
CFLAGS_DEFS += -DREL_COMPRESS_EN
THIRDPARTY_LZMA_ENABLE       = y
BIN_COMPRESS                 = y

DRIVER_CPFLASH_ENABLE        = n
DRIVER_VPU_ENABLE            = y
DRIVER_EEPROM_ENABLE         = n
DRIVER_SPI_ENABLE            = y
DRIVER_CAMERA_ENABLE         = y
DRIVER_LCD_ENABLE            = y
DRIVER_I2C_ENABLE            = y
DRIVER_PSRAM_ENABLE          = n
DRIVER_PCACHE_ENABLE         = y
DRIVER_EXSTORAGE_ENABLE      = n
#DRIVER_LPUART_ENABLE         = n
DRIVER_ONEWIRE_ENABLE        = n

THIRDPARTY_MMVIDEO_ENABLE    = y
THIRDPARTY_MMJPEG_ENABLE     = y

MIDDLEWARE_USB_CCID_ENABLE   = n
MIDDLEWARE_FOTAPAR_ENABLE    = y
MIDDLEWARE_FOTA_FS_ENABLE    = n

BUILD_EC_MW                  = n
AT_EXAMPLE_ENABLE            = n

# quec add , should always disable in ap , This macro is designed to select the USB library to be used on the boot side.
DRIVER_USBSMIMG_ENABLE=n

#should always enable, default compiled in lib
DRIVER_ACVOICE_ENG_ENABLE    = y

#Exception Flash Dump enable, default enable
EXCEPTION_FLASH_DUMP_ENABLE    = y


#########################################################################

THIRDPARTY_PING_ENABLE       = n

ifeq ($(IMS_MODE_ENABLE), y)
BUILD_IMS                = y
BUILD_PS_ROHC_ENABLE     = y
THIRDPARTY_CJSON_ENABLE  = y
endif

ifeq ($(IMS_SMSONLY_MODE_ENABLE), y)
BUILD_IMS                = y
THIRDPARTY_CJSON_ENABLE  = y
endif

ifeq ($(AUDIO_MODE_ENABLE), y)
MIDDLEWARE_AMR_ENABLE = y
MIDDLEWARE_VEM_ENABLE = y
DRIVER_VEM_CFG_ENABLE = y
DRIVER_I2C_ENABLE         = y
DRIVER_CODEC_ENABLE       = y
DRIVER_CODEC8311_ENABLE   = y
DRIVER_CODEC8374_ENABLE   = y
BUILD_SUPPORT_APP_PCM_MEM_POOL = n
endif

THIRDPARTY_JPEGCODEC_ENABLE    = n

THIRDPARTY_PPP_ENABLE          = n
THIRDPARTY_IPERF_ENABLE        = n
THIRDPARTY_LIBSNTP_ENABLE      = n
THIRDPARTY_MBEDTLS_ENABLE      = y
THIRDPARTY_HTTPC_ENABLE        = n
MBEDTLS_WITH_HTTP_TLS          = n
THIRDPARTY_MQTT_ENABLE         = n
MBEDTLS_WITH_MQTT_TLS          = n

#cjson is refered by MQTT, cjson defult value: n, as IMS also need it, here remove it
#THIRDPARTY_CJSON_ENABLE        = n
THIRDPARTY_CTCC_DM_ENABLE      = n
THIRDPARTY_CUCC_DM_ENABLE      = n
THIRDPARTY_CMCC_DM_ENABLE      = n
THIRDPARTY_CTWING_CERTI_ENABLE = n

BUILD_PS_TCPIP_API_ENABLE      = n

BUILD_AT                       = y
BUILD_AT_DEBUG                 = n
BUILD_AT_REF                   = n
##################################################################################################################
##Description:
##global option for RIL APIs
##Use method: y is support;n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_AT), y)
BUILD_RIL_AT_API_ENABLE      = n
endif

##################################################################################################################
##Description:
##global option for Plat AT commands
##Use method: y is support;n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_AT), y)

# 平台相关ATC指令 如 ATI AT+CGMR
BUILD_PLAT_CUST_AT_ENABLE    = y

# 内存测试相关ATC指令
BUILD_PLAT_MEM_AT_ENABLE     = n

BUILD_PLAT_PMU_AT_ENABLE     = y
BUILD_PLAT_PER_AT_ENABLE     = y
BUILD_PLAT_MISC_AT_ENABLE    = y
BUILD_PLAT_PROD_AT_ENABLE    = y

BUILD_PLAT_FOTA_AT_ENABLE    = n
BUILD_PLAT_ECOTA_AT_ENABLE   = n
BUILD_PLAT_CMUX_AT_ENABLE    = n
BUILD_PLAT_HTTP_AT_ENABLE    = n
BUILD_PLAT_MQTT_AT_ENABLE    = n
BUILD_PLAT_SSL_AT_ENABLE     = n

BUILD_PLAT_AUDIO_AT_ENABLE   = n
BUILD_PLAT_FS_AT_ENABLE      = n

BUILD_PLAT_ONENET_AT_ENABLE  = n
BUILD_PLAT_CTLWM2M_AT_ENABLE = n
endif


##################################################################################################################
##Description: BUILD_PLAT_MISC_AT_ENABLE
##global option for Plat miscellaneous AT commands
##Use method: y is support;n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_PLAT_MISC_AT_ENABLE), y)
#AT+ECPCFG
BUILD_PLAT_MISC_ECPCFG_ENABLE          = y
#AT+ECRST
BUILD_PLAT_MISC_ECRST_ENABLE           = n
#AT+ECPOWD
BUILD_PLAT_MISC_ECPOWD_ENABLE          = n
#AT^SYSTEST
BUILD_PLAT_MISC_SYSTEST_ENABLE         = n
#AT+ECSYSTEST
BUILD_PLAT_MISC_ECSYSTEST_ENABLE       = n
#AT+ECTASKINFO
BUILD_PLAT_MISC_ECTASKINFO_ENABLE      = n
#AT+ECTASKHISTINFO
BUILD_PLAT_MISC_ECTASKHISTINFO_ENABLE  = n
#AT+ECLOGDBVER
BUILD_PLAT_MISC_ECLOGDBVER_ENABLE      = n
#AT+ECDUMPCHK
BUILD_PLAT_MISC_ECDUMPCHK_ENABLE       = n
#AT+ECBTOFFSETDBG
BUILD_PLAT_MISC_ECBTOFFSETDBG_ENABLE   = n
#AT+ECFUSEMR
BUILD_PLAT_MISC_ECFUSEMR_ENABLE        = n
#AT+ECSIMO
BUILD_PLAT_MISC_ECSIMO_ENABLE          = n
#AT+ECPRFINFO
BUILD_PLAT_MISC_ECPRFINFO_ENABLE       = n
endif

##################################################################################################################
##Description:
##global option for PS AT commands
##Use method: y is support;n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_AT), y)
BUILD_PS_GEN_AT_ENABLE  = y
BUILD_PS_DEV_AT_ENABLE  = y
BUILD_PS_EMM_AT_ENABLE  = n
BUILD_PS_EPS_AT_ENABLE  = y
BUILD_PS_SIM_AT_ENABLE  = y
BUILD_PS_SMS_AT_ENABLE  = y
BUILD_PHY_CONFIG_AT_ENABLE = y
BUILD_PS_TCPIP_ECSOCK_AT_ENABLE  = n
BUILD_PS_TCPIP_SKTSOCK_AT_ENABLE = n
BUILD_PS_TCPIP_NETCONFIG_AT_ENABLE = n
BUILD_PS_TCPIP_IPSEC_AT_ENABLE   = n
endif


##################################################################################################################
##name :BUILD_PS_SMS_AT_ENABLE
##Description:
##BUILD_PS_SMS_AT_ENABLE is to control whether include ps SMS AT commands as: AT+CMGS/CMGR/CMGW/CNMI/...etc
##Use method: y is support; n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_PS_SMS_AT_ENABLE), y)
#AT+CMGS
BUILD_PS_SMS_CMGS_ENABLE   = n
#AT+CMGC
BUILD_PS_SMS_CMGC_ENABLE   = n
#AT+CMGR
BUILD_PS_SMS_CMGR_ENABLE   = n
#AT+CMGW
BUILD_PS_SMS_CMGW_ENABLE   = n
#AT+CSCA
BUILD_PS_SMS_CSCA_ENABLE   = n
#AT+CNMI
BUILD_PS_SMS_CNMI_ENABLE   = n
#AT+CMMS
BUILD_PS_SMS_CMMS_ENABLE   = n
#AT+CMGD
BUILD_PS_SMS_CMGD_ENABLE   = n
#AT+CMGL
BUILD_PS_SMS_CMGL_ENABLE   = n
#AT+CSMS
BUILD_PS_SMS_CSMS_ENABLE   = n
#AT+CPMS
BUILD_PS_SMS_CPMS_ENABLE   = n
#AT+CNMA
BUILD_PS_SMS_CNMA_ENABLE   = n
#AT+CMGF
BUILD_PS_SMS_CMGF_ENABLE   = n
#AT+CSMP
BUILD_PS_SMS_CSMP_ENABLE   = n
#AT+CSCB
BUILD_PS_SMS_CSCB_ENABLE   = n
#AT+CSDH
BUILD_PS_SMS_CSDH_ENABLE   = n
#AT+CMSS
BUILD_PS_SMS_CMSS_ENABLE   = n
#AT+ECSMSSEND
BUILD_PS_SMS_ECSMSSEND_ENABLE   = n
#AT+ECCMGS
BUILD_PS_SMS_ECCMGS_ENABLE   = n
#AT+ECCMGR
BUILD_PS_SMS_ECCMGR_ENABLE   = n
endif

##################################################################################################################
##name :BUILD_PS_EPS_AT_ENABLE
##Description:
##BUILD_PS_EPS_AT_ENABLE is to control whether include PS EPS AT commands
##Use method: y is support;n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_PS_EPS_AT_ENABLE), y)
#AT+CGEREP
BUILD_PS_EPS_CGEREP_ENABLE   = y
endif

##################################################################################################################
##name :BUILD_PHY_CONFIG_AT_ENABLE
##Description:
##BUILD_PHY_CONFIG_AT_ENABLE is to control whether include PHY CONFIG AT commands
##Use method: y is support;n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_PHY_CONFIG_AT_ENABLE), y)
#AT+ECPHYCFG
BUILD_PHY_CONFIG_ECPHYCFG_ENABLE   = y
#AT+ECEDRXSIMU
BUILD_PHY_CONFIG_ECEDRXSIMU_ENABLE = n
endif

##################################################################################################################
##name :BUILD_PS_EPS_AT_ENABLE
##Description:
##BUILD_PS_EPS_AT_ENABLE is to control whether include ESP  AT commands as: AT+CGATT/CGDCONT/CGACT/CGCONTRDP/CGAUTH..etc
##Use method: y is support;n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_PS_EPS_AT_ENABLE), y)
#AT+CGATT
BUILD_PS_EPS_CGATT_ENABLE  = n
#AT+ECEMCATT
BUILD_PS_EPS_ECEMCATT_ENABLE  = n
#AT+CGDATA
BUILD_PS_EPS_CGDATA_ENABLE = n
#AT+CGTFT
BUILD_PS_EPS_CGTFT_ENABLE  = n
#AT+CGCMOD
BUILD_PS_EPS_CGCMOD_ENABLE = n
#AT+CGEQOS
BUILD_PS_EPS_CGEQOS_ENABLE  = n
#AT+CGDSCONT
BUILD_PS_EPS_CGDSCONT_ENABLE = n
#AT+CEMODE
# AT+CEMODE redefined in UniRTOS and no "y". 
BUILD_PS_EPS_CEMODE_ENABLE = y
#AT+CGDCONT
BUILD_PS_EPS_CGDCONT_ENABLE = n
#AT+CGACT
BUILD_PS_EPS_CGACT_ENABLE   = n
#AT+CGEQOSRDP
BUILD_PS_EPS_CGEQOSRDP_ENABLE = n
#AT+CGCONTRDP
BUILD_PS_EPS_CGCONTRDP_ENABLE = n
#AT+CGSCONTRDP
BUILD_PS_EPS_CGSCONTRDP_ENABLE = n
#AT+CGTFTRDP
BUILD_PS_EPS_CGTFTRDP_ENABLE   = n
#AT+CEREG
BUILD_PS_EPS_CEREG_ENABLE = n
#AT+CSCON
BUILD_PS_EPS_CSCON_ENABLE = n
#AT+CSODCP
BUILD_PS_EPS_CSODCP_ENABLE = n
#AT+CRTDCP
BUILD_PS_EPS_CRTDCP_ENABLE = n
#AT+CGAUTH
BUILD_PS_EPS_CGAUTH_ENABLE = n
#AT+CIPCA
BUILD_PS_EPS_CIPCA_ENABLE  = n
#AT+CGAPNRC
BUILD_PS_EPS_CGAPNRC_ENABLE = n
#AT+CGEREP
BUILD_PS_EPS_CGEREP_ENABLE  = n
#AT+CGPADDR
BUILD_PS_EPS_CGPADDR_ENABLE  = n
#AT+CEER
BUILD_PS_EPS_CEER_ENABLE   = n
#AT+CEVDP
BUILD_PS_EPS_CEVDP_ENABLE = n
#AT+CPSDO
BUILD_PS_EPS_CPSDO_ENABLE  = n
#AT+ECCIOTPLANE
BUILD_PS_EPS_ECCIOTPLANE_ENABLE = n
#AT+ECSENDDATA
BUILD_PS_EPS_ECSENDDATA_ENABLE  = n
#AT+ECGDCNT
BUILD_PS_EPS_ECGDCNT_ENABLE     = n
#AT+ECAUGDCNT
BUILD_PS_EPS_ECAUGDCNT_ENABLE   = n
#AT+ECSCLKEX
BUILD_PS_EPS_ECSCLKEX_ENABLE    = n
#AT+ECSMER
BUILD_PS_EPS_ECSMER_ENABLE   = n
#AT+CNEC
BUILD_PS_EPS_CNEC_ENABLE = n
endif

##################################################################################################################
##name :BUILD_PS_SIM_AT_ENABLE
##Description:
##BUILD_PS_SIM_AT_ENABLE is to control whether include SIM AT commands as: AT+CIMI/CPIN/ECICCID/ECSIMCFG..etc
##Use method: y is support;n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_PS_SIM_AT_ENABLE), y)
#AT+CIMI
BUILD_PS_SIM_AT_CIMI_ENABLE         = n
#AT+CPIN
BUILD_PS_SIM_AT_CPIN_ENABLE         = n
#AT+ECICCID
BUILD_PS_SIM_AT_ECICCID_ENABLE      = n
#AT+ECSIMCFG
BUILD_PS_SIM_AT_ECSIMCFG_ENABLE     = y
#AT+CPWD
BUILD_PS_SIM_AT_CPWD_ENABLE         = n
#AT+CPINR
BUILD_PS_SIM_AT_CPINR_ENABLE        = n
#AT+ECSIMSLEEP
BUILD_PS_SIM_AT_ECSIMSLEEP_ENABLE   = y
#AT+CSIM
BUILD_PS_SIM_AT_CSIM_ENABLE         = n
#AT+CRSM
BUILD_PS_SIM_AT_CRSM_ENABLE         = n
#AT+CCHO
BUILD_PS_SIM_AT_CCHO_ENABLE         = n
#AT+CGLA
BUILD_PS_SIM_AT_CGLA_ENABLE         = n
#AT+CCHC
BUILD_PS_SIM_AT_CCHC_ENABLE         = n
#AT+ECSWC
BUILD_PS_SIM_AT_ECSWC_ENABLE        = n
#AT+ECSIMRM
BUILD_PS_SIM_AT_ECSIMRM_ENABLE      = n
#AT+CNUM
BUILD_PS_SIM_AT_CNUM_ENABLE         = n
#AT+ECUSATP
BUILD_PS_SIM_AT_ECUSATP_ENABLE      = n
#AT+CPOL
BUILD_PS_SIM_AT_CPOL_ENABLE         = y
#AT+CPLS
BUILD_PS_SIM_AT_CPLS_ENABLE         = n
endif


##################################################################################################################
##name :BUILD_PS_DEV_AT_ENABLE
##Description:
##BUILD_PS_DEV_AT_ENABLE is to control whether include ps  global AT commands as: AT+CFUN/ECBAND/ECCFG/ECSTATUS/...etc
##Use method: y is support;n is not support.
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_PS_DEV_AT_ENABLE), y)
#AT+CFUN
BUILD_PS_DEV_CFUN_ENABLE    = n
#AT+ECCGSN
BUILD_PS_DEV_ECCGSN_ENABLE  = y
#AT+ECCGSNLOCK
BUILD_PS_DEV_ECCGSNLOCK_ENABLE = n
#AT+ECNETDEVMAC
BUILD_PS_DEV_ECNETDEVMAC_ENABLE = n
#AT+ECSTATUS
BUILD_PS_DEV_ECSTATUS_ENABLE  = n
#AT+ECSTATIS
BUILD_PS_DEV_ECSTATIS_ENABLE  = n
#AT+ECBAND
BUILD_PS_DEV_ECBAND_ENABLE    = y
#AT+ECFREQ
BUILD_PS_DEV_ECFREQ_ENABLE    = n
#AT+ECCFG
BUILD_PS_DEV_ECCFG_ENABLE     = y
#AT+ECMEASCFG
BUILD_PS_DEV_ECMEASCFG_ENABLE = n
#AT+ECRMFPLMN
BUILD_PS_DEV_ECRMFPLMN_ENABLE = n
#AT+ECBLOCKPLMNLIST
FEATURE_PS_DEV_ECBLOCKPLMNLIST_ENABLE = n
#AT+ECBCINFO
BUILD_PS_DEV_ECBCINFO_ENABLE  = n
#AT+ECPSTEST
BUILD_PS_DEV_ECPSTEST_ENABLE  = n
#AT+ECEVENTSTATIS
BUILD_PS_DEV_ECEVENTSTATIS_ENABLE  = n
#AT+ECNASTCFG
BUILD_PS_DEV_ECNASTCFG_ENABLE   = n
#AT+ECWIFISCAN
BUILD_PS_DEV_ECWIFISCAN_ENABLE  = n
#AT+ECBARCELL
BUILD_PS_DEV_ECBARCELL_ENABLE  = n
#AT+ECJDC
BUILD_PS_DEV_ECJDC_ENABLE = n
endif

##################################################################################################################
##Description：BUILD_PLAT_PER_AT_ENABLE
##global option for Plat peripheral related AT commands
##Use method: y is support;n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_PLAT_PER_AT_ENABLE),y)
#AT+IPR
BUILD_PLAT_PER_IPR_ENABLE        = n
#AT+ICF
BUILD_PLAT_PER_ICF_ENABLE        = n
#AT+IFC
BUILD_PLAT_PER_IFC_ENABLE        = n
#AT+ECUSBSYS
BUILD_PLAT_PER_ECUSBSYS_ENABLE   = y
#AT+ECLEDMODE
BUILD_PLAT_PER_ECLEDMODE_ENABLE  = n
#AT+ECPALARM
BUILD_PLAT_PER_ECPALARM_ENABLE   = n
#AT+ECADC/ AT+QTEMP
BUILD_PLAT_PER_ECADC_ENABLE      = y
endif

##################################################################################################################
##Description：BUILD_PLAT_CUST_AT_ENABLE
##global option for Plat product related AT commands
##Use method: y is support;n is not support
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_PLAT_CUST_AT_ENABLE),y)
#AT+CGMI
BUILD_PLAT_CUST_CGMI_ENABLE   = n
#AT+CGMM
BUILD_PLAT_CUST_CGMM_ENABLE   = n
#AT+GMM
BUILD_PLAT_CUST_GMM_ENABLE    = n
#AT+CGMR
BUILD_PLAT_CUST_CGMR_ENABLE   = n
#AT+CGSN
BUILD_PLAT_CUST_CGSN_ENABLE   = n
#ATI
BUILD_PLAT_CUST_ATI_ENABLE    = y
#AT&W
BUILD_PLAT_CUST_ATnW_ENABLE   = y
#AT&F
BUILD_PLAT_CUST_ATnF_ENABLE   = y
#AT&V
BUILD_PLAT_CUST_ATnV_ENABLE   = y
endif


##################################################################################################################
##name :BUILD_PS_GEN_AT_ENABLE
##Description:
##BUILD_PS_GEN_AT_ENABLE is to control whether include ps  global AT commands as: AT/ATQ/ATE/ATT/ATV/ATL/ATM/...etc
##Use method: y is support;n is not support.
##default value is y to support
##################################################################################################################
ifeq ($(BUILD_PS_GEN_AT_ENABLE), y)
#AT via BUILD_PS_GEN_AT_ENABLE
#ATQ
BUILD_PS_GEN_AT_Q_ENABLE      = y
#ATE
BUILD_PS_GEN_AT_E_ENABLE      = y
#ATT
BUILD_PS_GEN_AT_T_ENABLE      = y
#ATV
BUILD_PS_GEN_AT_V_ENABLE      = n
#ATL
BUILD_PS_GEN_AT_L_ENABLE      = y
#ATM
BUILD_PS_GEN_AT_M_ENABLE      = y
#ATS0
BUILD_PS_GEN_AT_S0_ENABLE     = y
#AT&C
BUILD_PS_GEN_AT_nC_ENABLE     = n
#AT&D
BUILD_PS_GEN_AT_nD_ENABLE     = n
#ATD
BUILD_PS_GEN_AT_D_ENABLE      = y
#ATO
BUILD_PS_GEN_AT_O_ENABLE      = n
#ATH
BUILD_PS_GEN_AT_H_ENABLE      = y
#ATA
BUILD_PS_GEN_AT_A_ENABLE      = y
#AT+CSCS
BUILD_PS_GEN_CSCS_ENABLE      = y
#AT+CMEE
BUILD_PS_GEN_CMEE_ENABLE      = y
#AT+ECURC
BUILD_PS_GEN_ECURC_ENABLE     = n
#AT+ECURCCFG
BUILD_PS_GEN_ECURCCFG_ENABLE  = n
#AT+ECPPPHUP
BUILD_PS_GEN_ECPPPHUP_ENABLE  = n
#AT+CLCK
BUILD_PS_GEN_CLCK_ENABLE      = n
endif

# $(error arch_platform_cfg.inc)