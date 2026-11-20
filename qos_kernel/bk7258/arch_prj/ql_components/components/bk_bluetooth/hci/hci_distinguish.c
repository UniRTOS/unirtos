#include <stdint.h>
#include <os/mem.h>
#include <os/os.h>

#include "hci_distinguish.h"
#include "hal_hci_internal.h"
#include "hci_parse.h"

typedef struct
{
    uint16_t ocf;
    uint8_t type;// 0 bt 1 ble 2 mix
} hci_cmd_ocf_t;

typedef struct
{
    uint8_t ogf;
    uint8_t all_same;
    uint8_t default_value;
    uint32_t ocf_array_len;
    const hci_cmd_ocf_t *ocf_array;
} hci_cmd_group_map_t;


#define LOG_TAG "hal_hci_p"
#define LOG_LEVEL LOG_LEVEL_INFO

#define HCI_CMD_OCF_MASK ((((uint16_t)1) << 10) - 1)

static const hci_cmd_ocf_t s_hci_cmd_ocf_01[] =
{
    //Link Control Commands 01
    // {.ocf = (APP_HCI_INQ_CMD_OPCODE & HCI_CMD_OCF_MASK),                                    .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_INQ_CANCEL_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_PER_INQ_MODE_CMD_OPCODE & HCI_CMD_OCF_MASK),                           .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_EXIT_PER_INQ_MODE_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_CREATE_CON_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = HCI_CMD_TYPE_BT},
    {.ocf = (APP_HCI_DISCONNECT_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = HCI_CMD_TYPE_NOT_SURE},
    // {.ocf = (APP_HCI_CREATE_CON_CANCEL_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_ACCEPT_CON_REQ_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_REJECT_CON_REQ_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_LK_REQ_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),                           .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_LK_REQ_NEG_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_PIN_CODE_REQ_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),                     .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_PIN_CODE_REQ_NEG_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),                 .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_CHG_CON_PKT_TYPE_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_AUTH_REQ_CMD_OPCODE & HCI_CMD_OCF_MASK),                               .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_SET_CON_ENC_CMD_OPCODE & HCI_CMD_OCF_MASK),                            .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_CHG_CON_LK_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_MASTER_LK_CMD_OPCODE & HCI_CMD_OCF_MASK),                              .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_REM_NAME_REQ_CMD_OPCODE & HCI_CMD_OCF_MASK),                           .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_REM_NAME_REQ_CANCEL_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_RD_REM_SUPP_FEATS_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_RD_REM_EXT_FEATS_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_RD_REM_VER_INFO_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_RD_CLK_OFF_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_RD_LMP_HDL_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_SETUP_SYNC_CON_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_ACCEPT_SYNC_CON_REQ_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_REJECT_SYNC_CON_REQ_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_IO_CAP_REQ_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_USER_CFM_REQ_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),                     .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_USER_CFM_REQ_NEG_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),                 .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_USER_PASSKEY_REQ_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),                 .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_USER_PASSKEY_REQ_NEG_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),             .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_REM_OOB_DATA_REQ_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),                 .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_REM_OOB_DATA_REQ_NEG_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),             .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_IO_CAP_REQ_NEG_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),                   .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_ENH_SETUP_SYNC_CON_CMD_OPCODE & HCI_CMD_OCF_MASK),                     .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_ENH_ACCEPT_SYNC_CON_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_TRUNC_PAGE_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_TRUNC_PAGE_CAN_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_SET_CON_SLV_BCST_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_SET_CON_SLV_BCST_REC_CMD_OPCODE & HCI_CMD_OCF_MASK),                   .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_START_SYNC_TRAIN_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_REC_SYNC_TRAIN_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = HCI_CMD_TYPE_BT},
    // {.ocf = (APP_HCI_REM_OOB_EXT_DATA_REQ_REPLY_CMD_OPCODE & HCI_CMD_OCF_MASK),             .type = HCI_CMD_TYPE_BT},
};

static const hci_cmd_ocf_t s_hci_cmd_ocf_03[] =
{
    //Controller and Baseband Commands 03
    {.ocf = (APP_HCI_SET_EVT_MASK_CMD_OPCODE & HCI_CMD_OCF_MASK),                           .type = HCI_CMD_TYPE_MIX},
    {.ocf = (APP_HCI_RESET_CMD_OPCODE & HCI_CMD_OCF_MASK),                                  .type = HCI_CMD_TYPE_MIX},
    {.ocf = (APP_HCI_SET_EVT_FILTER_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = HCI_CMD_TYPE_MIX},
    {.ocf = (APP_HCI_FLUSH_CMD_OPCODE & HCI_CMD_OCF_MASK),                                  .type = HCI_CMD_TYPE_MIX},
    // {.ocf = (APP_HCI_RD_PIN_TYPE_CMD_OPCODE & HCI_CMD_OCF_MASK),                            .type = 0},
    // {.ocf = (APP_HCI_WR_PIN_TYPE_CMD_OPCODE & HCI_CMD_OCF_MASK),                            .type = 0},
    // {.ocf = (APP_HCI_CREATE_NEW_UNIT_KEY_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = 0},
    // {.ocf = (APP_HCI_RD_STORED_LK_CMD_OPCODE & HCI_CMD_OCF_MASK),                           .type = 0},
    // {.ocf = (APP_HCI_WR_STORED_LK_CMD_OPCODE & HCI_CMD_OCF_MASK),                           .type = 0},
    // {.ocf = (APP_HCI_DEL_STORED_LK_CMD_OPCODE & HCI_CMD_OCF_MASK),                          .type = 0},
    // {.ocf = (APP_HCI_WR_LOCAL_NAME_CMD_OPCODE & HCI_CMD_OCF_MASK),                          .type = 0},
    // {.ocf = (APP_HCI_RD_LOCAL_NAME_CMD_OPCODE & HCI_CMD_OCF_MASK),                          .type = 0},
    // {.ocf = (APP_HCI_RD_CON_ACCEPT_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_WR_CON_ACCEPT_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_RD_PAGE_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = 0},
    // {.ocf = (APP_HCI_WR_PAGE_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = 0},
    // {.ocf = (APP_HCI_RD_SCAN_EN_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = 0},
    // {.ocf = (APP_HCI_WR_SCAN_EN_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = 0},
    // {.ocf = (APP_HCI_RD_PAGE_SCAN_ACT_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_WR_PAGE_SCAN_ACT_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_RD_INQ_SCAN_ACT_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_WR_INQ_SCAN_ACT_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_RD_AUTH_EN_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = 0},
    // {.ocf = (APP_HCI_WR_AUTH_EN_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = 0},
    // {.ocf = (APP_HCI_RD_CLASS_OF_DEV_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_WR_CLASS_OF_DEV_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_RD_VOICE_STG_CMD_OPCODE & HCI_CMD_OCF_MASK),                           .type = 0},
    // {.ocf = (APP_HCI_WR_VOICE_STG_CMD_OPCODE & HCI_CMD_OCF_MASK),                           .type = 0},
    // {.ocf = (APP_HCI_RD_AUTO_FLUSH_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_WR_AUTO_FLUSH_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_RD_NB_BDCST_RETX_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_WR_NB_BDCST_RETX_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_RD_HOLD_MODE_ACTIVITY_CMD_OPCODE & HCI_CMD_OCF_MASK),                  .type = 0},
    // {.ocf = (APP_HCI_WR_HOLD_MODE_ACTIVITY_CMD_OPCODE & HCI_CMD_OCF_MASK),                  .type = 0},
    // {.ocf = (APP_HCI_RD_TX_PWR_LVL_CMD_OPCODE & HCI_CMD_OCF_MASK),                          .type = 0},
    // {.ocf = (APP_HCI_RD_SYNC_FLOW_CTRL_EN_CMD_OPCODE & HCI_CMD_OCF_MASK),                   .type = 0},
    // {.ocf = (APP_HCI_WR_SYNC_FLOW_CTRL_EN_CMD_OPCODE & HCI_CMD_OCF_MASK),                   .type = 0},
    // {.ocf = (APP_HCI_SET_CTRL_TO_HOST_FLOW_CTRL_CMD_OPCODE & HCI_CMD_OCF_MASK),             .type = 0},
    // {.ocf = (APP_HCI_HOST_BUF_SIZE_CMD_OPCODE & HCI_CMD_OCF_MASK),                          .type = 0},
    // {.ocf = (APP_HCI_HOST_NB_CMP_PKTS_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_RD_LINK_SUPV_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_WR_LINK_SUPV_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_RD_NB_SUPP_IAC_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = 0},
    // {.ocf = (APP_HCI_RD_CURR_IAC_LAP_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_WR_CURR_IAC_LAP_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_SET_AFH_HOST_CH_CLASS_CMD_OPCODE & HCI_CMD_OCF_MASK),                  .type = 0},
    // {.ocf = (APP_HCI_RD_INQ_SCAN_TYPE_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_WR_INQ_SCAN_TYPE_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
    // {.ocf = (APP_HCI_RD_INQ_MODE_CMD_OPCODE & HCI_CMD_OCF_MASK),                            .type = 0},
    // {.ocf = (APP_HCI_WR_INQ_MODE_CMD_OPCODE & HCI_CMD_OCF_MASK),                            .type = 0},
    // {.ocf = (APP_HCI_RD_PAGE_SCAN_TYPE_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = 0},
    // {.ocf = (APP_HCI_WR_PAGE_SCAN_TYPE_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = 0},
    // {.ocf = (APP_HCI_RD_AFH_CH_ASSESS_MODE_CMD_OPCODE & HCI_CMD_OCF_MASK),                  .type = 0},
    // {.ocf = (APP_HCI_WR_AFH_CH_ASSESS_MODE_CMD_OPCODE & HCI_CMD_OCF_MASK),                  .type = 0},
    // {.ocf = (APP_HCI_RD_EXT_INQ_RSP_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = 0},
    // {.ocf = (APP_HCI_WR_EXT_INQ_RSP_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = 0},
    // {.ocf = (APP_HCI_REFRESH_ENC_KEY_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_RD_SP_MODE_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = 0},
    // {.ocf = (APP_HCI_WR_SP_MODE_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = 0},
    // {.ocf = (APP_HCI_RD_LOC_OOB_DATA_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_RD_INQ_RSP_TX_PWR_LVL_CMD_OPCODE & HCI_CMD_OCF_MASK),                  .type = 0},
    // {.ocf = (APP_HCI_WR_INQ_TX_PWR_LVL_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = 0},
    // {.ocf = (APP_HCI_RD_DFT_ERR_DATA_REP_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = 0},
    // {.ocf = (APP_HCI_WR_DFT_ERR_DATA_REP_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = 0},
    // {.ocf = (APP_HCI_ENH_FLUSH_CMD_OPCODE & HCI_CMD_OCF_MASK),                              .type = 0},
    // {.ocf = (APP_HCI_SEND_KEYPRESS_NOTIF_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = 0},
    {.ocf = (APP_HCI_SET_EVT_MASK_PAGE_2_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = HCI_CMD_TYPE_MIX},
    // {.ocf = (APP_HCI_RD_FLOW_CNTL_MODE_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = 0},
    // {.ocf = (APP_HCI_WR_FLOW_CNTL_MODE_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = 0},
    // {.ocf = (APP_HCI_RD_ENH_TX_PWR_LVL_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = 0},
    // {.ocf = (APP_HCI_RD_LE_HOST_SUPP_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_WR_LE_HOST_SUPP_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_SET_MWS_CHANNEL_PARAMS_CMD_OPCODE & HCI_CMD_OCF_MASK),                 .type = 0},
    // {.ocf = (APP_HCI_SET_EXTERNAL_FRAME_CONFIG_CMD_OPCODE & HCI_CMD_OCF_MASK),              .type = 0},
    // {.ocf = (APP_HCI_SET_MWS_SIGNALING_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = 0},
    // {.ocf = (APP_HCI_SET_MWS_TRANSPORT_LAYER_CMD_OPCODE & HCI_CMD_OCF_MASK),                .type = 0},
    // {.ocf = (APP_HCI_SET_MWS_SCAN_FREQ_TABLE_CMD_OPCODE & HCI_CMD_OCF_MASK),                .type = 0},
    // {.ocf = (APP_HCI_SET_MWS_PATTERN_CONFIG_CMD_OPCODE & HCI_CMD_OCF_MASK),                 .type = 0},
    // {.ocf = (APP_HCI_SET_RES_LT_ADDR_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_DEL_RES_LT_ADDR_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_SET_CON_SLV_BCST_DATA_CMD_OPCODE & HCI_CMD_OCF_MASK),                  .type = 0},
    // {.ocf = (APP_HCI_RD_SYNC_TRAIN_PARAM_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = 0},
    // {.ocf = (APP_HCI_WR_SYNC_TRAIN_PARAM_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = 0},
    // {.ocf = (APP_HCI_RD_SEC_CON_HOST_SUPP_CMD_OPCODE & HCI_CMD_OCF_MASK),                   .type = 0},
    // {.ocf = (APP_HCI_WR_SEC_CON_HOST_SUPP_CMD_OPCODE & HCI_CMD_OCF_MASK),                   .type = 0},
    // {.ocf = (APP_HCI_RD_AUTH_PAYL_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_WR_AUTH_PAYL_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_RD_LOC_OOB_EXT_DATA_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = 0},
    // {.ocf = (APP_HCI_RD_EXT_PAGE_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = 0},
    // {.ocf = (APP_HCI_WR_EXT_PAGE_TO_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = 0},
    // {.ocf = (APP_HCI_RD_EXT_INQ_LEN_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = 0},
    // {.ocf = (APP_HCI_WR_EXT_INQ_LEN_CMD_OPCODE & HCI_CMD_OCF_MASK),                         .type = 0},
    // {.ocf = (APP_HCI_SET_ECO_BASE_INTV_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = 0},
    // {.ocf = (APP_HCI_CONFIG_DATA_PATH_CMD_OPCODE & HCI_CMD_OCF_MASK),                       .type = 0},
};
static const hci_cmd_ocf_t s_hci_cmd_ocf_04[] =
{
    //Info Params 04
    // {.ocf = (APP_HCI_RD_LOCAL_VER_INFO_CMD_OPCODE & HCI_CMD_OCF_MASK),                      .type = HCI_CMD_TYPE_MIX},
    // {.ocf = (APP_HCI_RD_LOCAL_SUPP_CMDS_CMD_OPCODE & HCI_CMD_OCF_MASK),                     .type = HCI_CMD_TYPE_MIX},
    // {.ocf = (APP_HCI_RD_LOCAL_SUPP_FEATS_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = HCI_CMD_TYPE_MIX},
    // {.ocf = (APP_HCI_RD_LOCAL_EXT_FEATS_CMD_OPCODE & HCI_CMD_OCF_MASK),                     .type = HCI_CMD_TYPE_MIX},
    // {.ocf = (APP_HCI_RD_BUF_SIZE_CMD_OPCODE & HCI_CMD_OCF_MASK),                            .type = 0},
    // {.ocf = (APP_HCI_RD_BD_ADDR_CMD_OPCODE & HCI_CMD_OCF_MASK),                             .type = HCI_CMD_TYPE_MIX},
    // {.ocf = (APP_HCI_RD_DATA_BLOCK_SIZE_CMD_OPCODE & HCI_CMD_OCF_MASK),                     .type = 0},
    // {.ocf = (APP_HCI_RD_LOCAL_SUPP_CODECS_CMD_OPCODE & HCI_CMD_OCF_MASK),                   .type = HCI_CMD_TYPE_MIX},
    // {.ocf = (APP_HCI_RD_LOCAL_SP_OPT_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = 0},
    // {.ocf = (APP_HCI_RD_LOCAL_SUPP_CODECS_V2_CMD_OPCODE & HCI_CMD_OCF_MASK),                .type = 0},
    // {.ocf = (APP_HCI_RD_LOCAL_SUPP_CODEC_CAP_CMD_OPCODE & HCI_CMD_OCF_MASK),                .type = 0},
    // {.ocf = (APP_HCI_RD_LOCAL_SUPP_CTRL_DELAY_CMD_OPCODE & HCI_CMD_OCF_MASK),               .type = 0},
};
static const hci_cmd_ocf_t s_hci_cmd_ocf_05[] =
{
    //Status Params 05
    // {.ocf = (APP_HCI_RD_FAIL_CONTACT_CNT_CMD_OPCODE & HCI_CMD_OCF_MASK),                    .type = HCI_CMD_TYPE_NOT_SURE},
    // {.ocf = (APP_HCI_RST_FAIL_CONTACT_CNT_CMD_OPCODE & HCI_CMD_OCF_MASK),                   .type = HCI_CMD_TYPE_NOT_SURE},
    // {.ocf = (APP_HCI_RD_LINK_QUAL_CMD_OPCODE & HCI_CMD_OCF_MASK),                           .type = HCI_CMD_TYPE_NOT_SURE},
    // {.ocf = (APP_HCI_RD_RSSI_CMD_OPCODE & HCI_CMD_OCF_MASK),                                .type = HCI_CMD_TYPE_NOT_SURE},
    // {.ocf = (APP_HCI_RD_AFH_CH_MAP_CMD_OPCODE & HCI_CMD_OCF_MASK),                          .type = HCI_CMD_TYPE_NOT_SURE},
    // {.ocf = (APP_HCI_RD_CLK_CMD_OPCODE & HCI_CMD_OCF_MASK),                                 .type = HCI_CMD_TYPE_NOT_SURE},
    // {.ocf = (APP_HCI_RD_ENC_KEY_SIZE_CMD_OPCODE & HCI_CMD_OCF_MASK),                        .type = HCI_CMD_TYPE_NOT_SURE},
    {.ocf = (APP_HCI_GET_MWS_TRANSPORT_LAYER_CONFIG_CMD_OPCODE & HCI_CMD_OCF_MASK),         .type = 0},
};

static const hci_cmd_ocf_t s_hci_cmd_ocf_3f[] =
{
    //vendor cmd
    {.ocf = (APP_HCI_VENDOR_SET_ACL_PRIORITY_CMD_OPCODE & HCI_CMD_OCF_MASK),                  .type = HCI_CMD_TYPE_ONLY_ONE},
};

static const hci_cmd_group_map_t s_hci_cmd_group_map[] =
{
    {
        //link control
        .ogf = 0x1,
        .all_same = 0,
        .default_value = HCI_CMD_TYPE_BT,
        .ocf_array_len = sizeof(s_hci_cmd_ocf_01) / sizeof(s_hci_cmd_ocf_01[0]),
        .ocf_array = s_hci_cmd_ocf_01,
    },
    {
        //link policy
        .ogf = 0x2,
        .all_same = 1,
        .default_value = HCI_CMD_TYPE_BT,
    },
    {
        //controller baseband
        .ogf = 0x3,
        .all_same = 0,
        .default_value = HCI_CMD_TYPE_BT,
        .ocf_array_len = sizeof(s_hci_cmd_ocf_03) / sizeof(s_hci_cmd_ocf_03[0]),
        .ocf_array = s_hci_cmd_ocf_03,
    },
    {
        //info
        .ogf = 0x4,
        .all_same = 0,
        .default_value = HCI_CMD_TYPE_ONLY_ONE,
        .ocf_array_len = sizeof(s_hci_cmd_ocf_04) / sizeof(s_hci_cmd_ocf_04[0]),
        .ocf_array = s_hci_cmd_ocf_04,
    },
    {
        //status
        .ogf = 0x5,
        .all_same = 0,
        .default_value = HCI_CMD_TYPE_NOT_SURE,
        .ocf_array_len = sizeof(s_hci_cmd_ocf_05) / sizeof(s_hci_cmd_ocf_05[0]),
        .ocf_array = s_hci_cmd_ocf_05,
    },
    // {
    //     //test
    //     .ogf = 0x6,
    //     .default_value = 2,
    // },
    {
        //ble
        .ogf = 0x8,
        .all_same = 1,
        .default_value = HCI_CMD_TYPE_BLE,
    },
    {
        //vnd
        .ogf = 0x3f,
        .all_same = 0,
        .default_value = HCI_CMD_TYPE_ONLY_ONE,
        .ocf_array_len = sizeof(s_hci_cmd_ocf_3f) / sizeof(s_hci_cmd_ocf_3f[0]),
        .ocf_array = s_hci_cmd_ocf_3f,
    }
};

uint8_t hci_cmd_get_type(uint16_t opcode)
{
    uint8_t ogf = ((opcode & ~(uint16_t)HCI_CMD_OCF_MASK) >> 10);
    uint16_t ocf = (opcode & (uint16_t)HCI_CMD_OCF_MASK);

    LOGV("op 0x%x ogf 0x%x ocf 0x%x", opcode, ogf, ocf);

    for (uint32_t i = 0; i < sizeof(s_hci_cmd_group_map) / sizeof(s_hci_cmd_group_map[0]); i++)
    {
        if (s_hci_cmd_group_map[i].ogf == ogf)
        {
            if (s_hci_cmd_group_map[i].all_same)
            {
                return s_hci_cmd_group_map[i].default_value;
            }
            else
            {
                for (uint32_t j = 0; j < s_hci_cmd_group_map[i].ocf_array_len; j++)
                {
                    if (s_hci_cmd_group_map[i].ocf_array[j].ocf == ocf)
                    {
                        return s_hci_cmd_group_map[i].ocf_array[j].type;
                    }
                }

                return s_hci_cmd_group_map[i].default_value;
            }
        }
    }

    return HCI_CMD_TYPE_ONLY_ONE;
}
