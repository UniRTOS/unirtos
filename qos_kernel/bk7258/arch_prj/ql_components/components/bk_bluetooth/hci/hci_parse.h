#pragma once

#include <stdint.h>

typedef enum
{
    DATA_TYPE_COMMAND = 1,
    DATA_TYPE_ACL     = 2,
    DATA_TYPE_SCO     = 3,
    DATA_TYPE_EVENT   = 4,
} serial_data_type_t;

enum
{
    HCI_PARSER_ENCODE_RET_NO_NEED,
    HCI_PARSER_ENCODE_RET_PENDING,
    HCI_PARSER_ENCODE_RET_ERROR,
};


enum
{
    //Link Control Commands
    APP_HCI_INQ_CMD_OPCODE                        = 0x0401,
    APP_HCI_INQ_CANCEL_CMD_OPCODE                 = 0x0402,
    APP_HCI_PER_INQ_MODE_CMD_OPCODE               = 0x0403,
    APP_HCI_EXIT_PER_INQ_MODE_CMD_OPCODE          = 0x0404,
    APP_HCI_CREATE_CON_CMD_OPCODE                 = 0x0405,
    APP_HCI_DISCONNECT_CMD_OPCODE                 = 0x0406,
    APP_HCI_CREATE_CON_CANCEL_CMD_OPCODE          = 0x0408,
    APP_HCI_ACCEPT_CON_REQ_CMD_OPCODE             = 0x0409,
    APP_HCI_REJECT_CON_REQ_CMD_OPCODE             = 0x040A,
    APP_HCI_LK_REQ_REPLY_CMD_OPCODE               = 0x040B,
    APP_HCI_LK_REQ_NEG_REPLY_CMD_OPCODE           = 0x040C,
    APP_HCI_PIN_CODE_REQ_REPLY_CMD_OPCODE         = 0x040D,
    APP_HCI_PIN_CODE_REQ_NEG_REPLY_CMD_OPCODE     = 0x040E,
    APP_HCI_CHG_CON_PKT_TYPE_CMD_OPCODE           = 0x040F,
    APP_HCI_AUTH_REQ_CMD_OPCODE                   = 0x0411,
    APP_HCI_SET_CON_ENC_CMD_OPCODE                = 0x0413,
    APP_HCI_CHG_CON_LK_CMD_OPCODE                 = 0x0415,
    APP_HCI_MASTER_LK_CMD_OPCODE                  = 0x0417,
    APP_HCI_REM_NAME_REQ_CMD_OPCODE               = 0x0419,
    APP_HCI_REM_NAME_REQ_CANCEL_CMD_OPCODE        = 0x041A,
    APP_HCI_RD_REM_SUPP_FEATS_CMD_OPCODE          = 0x041B,
    APP_HCI_RD_REM_EXT_FEATS_CMD_OPCODE           = 0x041C,
    APP_HCI_RD_REM_VER_INFO_CMD_OPCODE            = 0x041D,
    APP_HCI_RD_CLK_OFF_CMD_OPCODE                 = 0x041F,
    APP_HCI_RD_LMP_HDL_CMD_OPCODE                 = 0x0420,
    APP_HCI_SETUP_SYNC_CON_CMD_OPCODE             = 0x0428,
    APP_HCI_ACCEPT_SYNC_CON_REQ_CMD_OPCODE        = 0x0429,
    APP_HCI_REJECT_SYNC_CON_REQ_CMD_OPCODE        = 0x042A,
    APP_HCI_IO_CAP_REQ_REPLY_CMD_OPCODE           = 0x042B,
    APP_HCI_USER_CFM_REQ_REPLY_CMD_OPCODE         = 0x042C,
    APP_HCI_USER_CFM_REQ_NEG_REPLY_CMD_OPCODE     = 0x042D,
    APP_HCI_USER_PASSKEY_REQ_REPLY_CMD_OPCODE     = 0x042E,
    APP_HCI_USER_PASSKEY_REQ_NEG_REPLY_CMD_OPCODE = 0x042F,
    APP_HCI_REM_OOB_DATA_REQ_REPLY_CMD_OPCODE     = 0x0430,
    APP_HCI_REM_OOB_DATA_REQ_NEG_REPLY_CMD_OPCODE = 0x0433,
    APP_HCI_IO_CAP_REQ_NEG_REPLY_CMD_OPCODE       = 0x0434,
    APP_HCI_ENH_SETUP_SYNC_CON_CMD_OPCODE         = 0x043D,
    APP_HCI_ENH_ACCEPT_SYNC_CON_CMD_OPCODE        = 0x043E,
    APP_HCI_TRUNC_PAGE_CMD_OPCODE                 = 0x043F,
    APP_HCI_TRUNC_PAGE_CAN_CMD_OPCODE             = 0x0440,
    APP_HCI_SET_CON_SLV_BCST_CMD_OPCODE           = 0x0441,
    APP_HCI_SET_CON_SLV_BCST_REC_CMD_OPCODE       = 0x0442,
    APP_HCI_START_SYNC_TRAIN_CMD_OPCODE           = 0x0443,
    APP_HCI_REC_SYNC_TRAIN_CMD_OPCODE             = 0x0444,
    APP_HCI_REM_OOB_EXT_DATA_REQ_REPLY_CMD_OPCODE = 0x0445,

    //Link Policy Commands
    APP_HCI_HOLD_MODE_CMD_OPCODE                  = 0x0801,
    APP_HCI_SNIFF_MODE_CMD_OPCODE                 = 0x0803,
    APP_HCI_EXIT_SNIFF_MODE_CMD_OPCODE            = 0x0804,
    APP_HCI_PARK_STATE_CMD_OPCODE                 = 0x0805,
    APP_HCI_EXIT_PARK_STATE_CMD_OPCODE            = 0x0806,
    APP_HCI_QOS_SETUP_CMD_OPCODE                  = 0x0807,
    APP_HCI_ROLE_DISCOVERY_CMD_OPCODE             = 0x0809,
    APP_HCI_SWITCH_ROLE_CMD_OPCODE                = 0x080B,
    APP_HCI_RD_LINK_POL_STG_CMD_OPCODE            = 0x080C,
    APP_HCI_WR_LINK_POL_STG_CMD_OPCODE            = 0x080D,
    APP_HCI_RD_DFT_LINK_POL_STG_CMD_OPCODE        = 0x080E,
    APP_HCI_WR_DFT_LINK_POL_STG_CMD_OPCODE        = 0x080F,
    APP_HCI_FLOW_SPEC_CMD_OPCODE                  = 0x0810,
    APP_HCI_SNIFF_SUB_CMD_OPCODE                  = 0x0811,

    //Controller and Baseband Commands
    APP_HCI_SET_EVT_MASK_CMD_OPCODE               = 0x0C01,
    APP_HCI_RESET_CMD_OPCODE                      = 0x0C03,
    APP_HCI_SET_EVT_FILTER_CMD_OPCODE             = 0x0C05,
    APP_HCI_FLUSH_CMD_OPCODE                      = 0x0C08,
    APP_HCI_RD_PIN_TYPE_CMD_OPCODE                = 0x0C09,
    APP_HCI_WR_PIN_TYPE_CMD_OPCODE                = 0x0C0A,
    APP_HCI_CREATE_NEW_UNIT_KEY_CMD_OPCODE        = 0x0C0B,
    APP_HCI_RD_STORED_LK_CMD_OPCODE               = 0x0C0D,
    APP_HCI_WR_STORED_LK_CMD_OPCODE               = 0x0C11,
    APP_HCI_DEL_STORED_LK_CMD_OPCODE              = 0x0C12,
    APP_HCI_WR_LOCAL_NAME_CMD_OPCODE              = 0x0C13,
    APP_HCI_RD_LOCAL_NAME_CMD_OPCODE              = 0x0C14,
    APP_HCI_RD_CON_ACCEPT_TO_CMD_OPCODE           = 0x0C15,
    APP_HCI_WR_CON_ACCEPT_TO_CMD_OPCODE           = 0x0C16,
    APP_HCI_RD_PAGE_TO_CMD_OPCODE                 = 0x0C17,
    APP_HCI_WR_PAGE_TO_CMD_OPCODE                 = 0x0C18,
    APP_HCI_RD_SCAN_EN_CMD_OPCODE                 = 0x0C19,
    APP_HCI_WR_SCAN_EN_CMD_OPCODE                 = 0x0C1A,
    APP_HCI_RD_PAGE_SCAN_ACT_CMD_OPCODE           = 0x0C1B,
    APP_HCI_WR_PAGE_SCAN_ACT_CMD_OPCODE           = 0x0C1C,
    APP_HCI_RD_INQ_SCAN_ACT_CMD_OPCODE            = 0x0C1D,
    APP_HCI_WR_INQ_SCAN_ACT_CMD_OPCODE            = 0x0C1E,
    APP_HCI_RD_AUTH_EN_CMD_OPCODE                 = 0x0C1F,
    APP_HCI_WR_AUTH_EN_CMD_OPCODE                 = 0x0C20,
    APP_HCI_RD_CLASS_OF_DEV_CMD_OPCODE            = 0x0C23,
    APP_HCI_WR_CLASS_OF_DEV_CMD_OPCODE            = 0x0C24,
    APP_HCI_RD_VOICE_STG_CMD_OPCODE               = 0x0C25,
    APP_HCI_WR_VOICE_STG_CMD_OPCODE               = 0x0C26,
    APP_HCI_RD_AUTO_FLUSH_TO_CMD_OPCODE           = 0x0C27,
    APP_HCI_WR_AUTO_FLUSH_TO_CMD_OPCODE           = 0x0C28,
    APP_HCI_RD_NB_BDCST_RETX_CMD_OPCODE           = 0x0C29,
    APP_HCI_WR_NB_BDCST_RETX_CMD_OPCODE           = 0x0C2A,
    APP_HCI_RD_HOLD_MODE_ACTIVITY_CMD_OPCODE      = 0x0C2B,
    APP_HCI_WR_HOLD_MODE_ACTIVITY_CMD_OPCODE      = 0x0C2C,
    APP_HCI_RD_TX_PWR_LVL_CMD_OPCODE              = 0x0C2D,
    APP_HCI_RD_SYNC_FLOW_CTRL_EN_CMD_OPCODE       = 0x0C2E,
    APP_HCI_WR_SYNC_FLOW_CTRL_EN_CMD_OPCODE       = 0x0C2F,
    APP_HCI_SET_CTRL_TO_HOST_FLOW_CTRL_CMD_OPCODE = 0x0C31,
    APP_HCI_HOST_BUF_SIZE_CMD_OPCODE              = 0x0C33,
    APP_HCI_HOST_NB_CMP_PKTS_CMD_OPCODE           = 0x0C35,
    APP_HCI_RD_LINK_SUPV_TO_CMD_OPCODE            = 0x0C36,
    APP_HCI_WR_LINK_SUPV_TO_CMD_OPCODE            = 0x0C37,
    APP_HCI_RD_NB_SUPP_IAC_CMD_OPCODE             = 0x0C38,
    APP_HCI_RD_CURR_IAC_LAP_CMD_OPCODE            = 0x0C39,
    APP_HCI_WR_CURR_IAC_LAP_CMD_OPCODE            = 0x0C3A,
    APP_HCI_SET_AFH_HOST_CH_CLASS_CMD_OPCODE      = 0x0C3F,
    APP_HCI_RD_INQ_SCAN_TYPE_CMD_OPCODE           = 0x0C42,
    APP_HCI_WR_INQ_SCAN_TYPE_CMD_OPCODE           = 0x0C43,
    APP_HCI_RD_INQ_MODE_CMD_OPCODE                = 0x0C44,
    APP_HCI_WR_INQ_MODE_CMD_OPCODE                = 0x0C45,
    APP_HCI_RD_PAGE_SCAN_TYPE_CMD_OPCODE          = 0x0C46,
    APP_HCI_WR_PAGE_SCAN_TYPE_CMD_OPCODE          = 0x0C47,
    APP_HCI_RD_AFH_CH_ASSESS_MODE_CMD_OPCODE      = 0x0C48,
    APP_HCI_WR_AFH_CH_ASSESS_MODE_CMD_OPCODE      = 0x0C49,
    APP_HCI_RD_EXT_INQ_RSP_CMD_OPCODE             = 0x0C51,
    APP_HCI_WR_EXT_INQ_RSP_CMD_OPCODE             = 0x0C52,
    APP_HCI_REFRESH_ENC_KEY_CMD_OPCODE            = 0x0C53,
    APP_HCI_RD_SP_MODE_CMD_OPCODE                 = 0x0C55,
    APP_HCI_WR_SP_MODE_CMD_OPCODE                 = 0x0C56,
    APP_HCI_RD_LOC_OOB_DATA_CMD_OPCODE            = 0x0C57,
    APP_HCI_RD_INQ_RSP_TX_PWR_LVL_CMD_OPCODE      = 0x0C58,
    APP_HCI_WR_INQ_TX_PWR_LVL_CMD_OPCODE          = 0x0C59,
    APP_HCI_RD_DFT_ERR_DATA_REP_CMD_OPCODE        = 0x0C5A,
    APP_HCI_WR_DFT_ERR_DATA_REP_CMD_OPCODE        = 0x0C5B,
    APP_HCI_ENH_FLUSH_CMD_OPCODE                  = 0x0C5F,
    APP_HCI_SEND_KEYPRESS_NOTIF_CMD_OPCODE        = 0x0C60,
    APP_HCI_SET_EVT_MASK_PAGE_2_CMD_OPCODE        = 0x0C63,
    APP_HCI_RD_FLOW_CNTL_MODE_CMD_OPCODE          = 0x0C66,
    APP_HCI_WR_FLOW_CNTL_MODE_CMD_OPCODE          = 0x0C67,
    APP_HCI_RD_ENH_TX_PWR_LVL_CMD_OPCODE          = 0x0C68,
    APP_HCI_RD_LE_HOST_SUPP_CMD_OPCODE            = 0x0C6C,
    APP_HCI_WR_LE_HOST_SUPP_CMD_OPCODE            = 0x0C6D,
    APP_HCI_SET_MWS_CHANNEL_PARAMS_CMD_OPCODE     = 0x0C6E,
    APP_HCI_SET_EXTERNAL_FRAME_CONFIG_CMD_OPCODE  = 0x0C6F,
    APP_HCI_SET_MWS_SIGNALING_CMD_OPCODE          = 0x0C70,
    APP_HCI_SET_MWS_TRANSPORT_LAYER_CMD_OPCODE    = 0x0C71,
    APP_HCI_SET_MWS_SCAN_FREQ_TABLE_CMD_OPCODE    = 0x0C72,
    APP_HCI_SET_MWS_PATTERN_CONFIG_CMD_OPCODE     = 0x0C73,
    APP_HCI_SET_RES_LT_ADDR_CMD_OPCODE            = 0x0C74,
    APP_HCI_DEL_RES_LT_ADDR_CMD_OPCODE            = 0x0C75,
    APP_HCI_SET_CON_SLV_BCST_DATA_CMD_OPCODE      = 0x0C76,
    APP_HCI_RD_SYNC_TRAIN_PARAM_CMD_OPCODE        = 0x0C77,
    APP_HCI_WR_SYNC_TRAIN_PARAM_CMD_OPCODE        = 0x0C78,
    APP_HCI_RD_SEC_CON_HOST_SUPP_CMD_OPCODE       = 0x0C79,
    APP_HCI_WR_SEC_CON_HOST_SUPP_CMD_OPCODE       = 0x0C7A,
    APP_HCI_RD_AUTH_PAYL_TO_CMD_OPCODE            = 0x0C7B,
    APP_HCI_WR_AUTH_PAYL_TO_CMD_OPCODE            = 0x0C7C,
    APP_HCI_RD_LOC_OOB_EXT_DATA_CMD_OPCODE        = 0x0C7D,
    APP_HCI_RD_EXT_PAGE_TO_CMD_OPCODE             = 0x0C7E,
    APP_HCI_WR_EXT_PAGE_TO_CMD_OPCODE             = 0x0C7F,
    APP_HCI_RD_EXT_INQ_LEN_CMD_OPCODE             = 0x0C80,
    APP_HCI_WR_EXT_INQ_LEN_CMD_OPCODE             = 0x0C81,
    APP_HCI_SET_ECO_BASE_INTV_CMD_OPCODE          = 0x0C82,
    APP_HCI_CONFIG_DATA_PATH_CMD_OPCODE           = 0x0C83,

    //Info Params
    APP_HCI_RD_LOCAL_VER_INFO_CMD_OPCODE               = 0x1001,
    APP_HCI_RD_LOCAL_SUPP_CMDS_CMD_OPCODE              = 0x1002,
    APP_HCI_RD_LOCAL_SUPP_FEATS_CMD_OPCODE             = 0x1003,
    APP_HCI_RD_LOCAL_EXT_FEATS_CMD_OPCODE              = 0x1004,
    APP_HCI_RD_BUF_SIZE_CMD_OPCODE                     = 0x1005,
    APP_HCI_RD_BD_ADDR_CMD_OPCODE                      = 0x1009,
    APP_HCI_RD_DATA_BLOCK_SIZE_CMD_OPCODE              = 0x100A,
    APP_HCI_RD_LOCAL_SUPP_CODECS_CMD_OPCODE            = 0x100B,
    APP_HCI_RD_LOCAL_SP_OPT_CMD_OPCODE                 = 0x100C,
    APP_HCI_RD_LOCAL_SUPP_CODECS_V2_CMD_OPCODE         = 0x100D,
    APP_HCI_RD_LOCAL_SUPP_CODEC_CAP_CMD_OPCODE         = 0x100E,
    APP_HCI_RD_LOCAL_SUPP_CTRL_DELAY_CMD_OPCODE        = 0x100F,

    //Status Params
    APP_HCI_RD_FAIL_CONTACT_CNT_CMD_OPCODE             = 0x1401,
    APP_HCI_RST_FAIL_CONTACT_CNT_CMD_OPCODE            = 0x1402,
    APP_HCI_RD_LINK_QUAL_CMD_OPCODE                    = 0x1403,
    APP_HCI_RD_RSSI_CMD_OPCODE                         = 0x1405,
    APP_HCI_RD_AFH_CH_MAP_CMD_OPCODE                   = 0x1406,
    APP_HCI_RD_CLK_CMD_OPCODE                          = 0x1407,
    APP_HCI_RD_ENC_KEY_SIZE_CMD_OPCODE                 = 0x1408,
    APP_HCI_GET_MWS_TRANSPORT_LAYER_CONFIG_CMD_OPCODE  = 0x140C,

    //vnd
    APP_HCI_VENDOR_SET_ACL_PRIORITY_CMD_OPCODE         = 0xfc57,
    APP_HCI_VENDOR_H5_INIT_CMD_OPCODE                  = 0xfca7,
};

typedef struct
{
    void (*notify_parse_packet_ready_cb)(uint8_t type, void *data, uint16_t len);
    void (*notify_parse_packet_ready_ext_cb)(void *data, uint16_t len);
    void (*notify_encode_packet_ready_cb)(void *data, uint16_t len);
    void (*notify_encode_packet_ready_ext_cb)(uint8_t type, void *data, uint16_t len);
    // void (*notify_uart_change_cb)(uint32_t baud);
    // void (*notify_uart_reset_cb)(void);
    // int (*read_byte)(uint8_t *ch);
} hci_parser_callbacks_t;

typedef struct
{
    int32_t (*init)(const hci_parser_callbacks_t *cb);
    int32_t (*deinit)(void);
    int32_t (*do_parse)(uint8_t *data, uint32_t len);
    int32_t (*do_encode)(uint8_t *data, uint32_t len);
    int32_t (*set_h5_enable)(uint8_t enable);
    uint8_t (*get_h5_enable)(void);
} hci_parser_t;

const hci_parser_t *hci_parser_get_interface(void);
