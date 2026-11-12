/*****************************************************************/ /**
* @file qcm_opauth.h
* @brief QCM OpAuth 公共类型定义与 API 声明
* @author lambert.zhao@quectel.com
* @date 2026-08-06
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2026-08-06 <td>1.0 <td>Lambert.Zhao <td> Init
* </table>
**********************************************************************/
#ifndef __QCM_OPAUTH_H__
#define __QCM_OPAUTH_H__

#include "qosa_def.h"

/**
 * UniRTOS 运营商二次认证公共 API
 *
 * 面向应用层提供的标准化、稳定的二次认证操作接口。
 * 应用层无需了解内部实现细节。
 *
 * 支持两种认证模式：
 *   - QCM_OPAUTH_METHOD_SERVER_H5   : 运营商服务器重定向认证
 *   - QCM_OPAUTH_METHOD_INTERNAL_H5 : 内置局域网H5认证门户
 *
 * 扩展点：
 *   - 通过 qcm_opauth_config_t::custom_param_query 注入自定义参数查询实现
 *   - 通过 qcm_opauth_register_event_cb() 注册事件回调
 */

/* ======================== 错误码 ======================== */

/**
 * @enum qcm_opauth_err_e
 * @brief 认证 API 统一错误码
 */
typedef enum {
    QCM_OPAUTH_RES_SUCCESS               = 0, /** 成功 */
    QCM_OPAUTH_RES_ERROR_GENERAL         = 1, /** 一般错误 */
    QCM_OPAUTH_RES_ERROR_MEMORY          = 2, /** 内存错误 */
    QCM_OPAUTH_RES_ERROR_TIMEOUT         = 3, /** 超时错误 */
    QCM_OPAUTH_RES_ERROR_EXECUTE         = 4, /** 执行错误 */
    QCM_OPAUTH_RES_ERROR_INVALID_PARAM   = 5, /** 无效参数 */
    QCM_OPAUTH_RES_ERROR_NOT_SUPPORTED   = 6, /** 不支持的操作 */
    QCM_OPAUTH_RES_ERROR_INVALID_STATE   = 7, /** 状态不正确 */
    QCM_OPAUTH_RES_ERROR_API_ERROR       = 8, /** API 执行失败 */
    QCM_OPAUTH_RES_ERROR_SIMINFO_MISSING = 9, /** 当前 SIM 未配置认证参数 */
} qcm_opauth_err_e;

/* ======================== 认证使能枚举 ======================== */

/**
 * @enum qcm_opauth_enable_e
 * @brief 认证使能状态
 */
typedef enum {
    QCM_OPAUTH_DISABLE = 0, /** 不使能认证（默认） */
    QCM_OPAUTH_ENABLE  = 1, /** 使能认证 */
} qcm_opauth_enable_e;

/* ======================== 认证模式枚举 ======================== */

/**
 * @enum qcm_opauth_mode_e
 * @brief 运营商认证模式
 */
typedef enum {
    QCM_OPAUTH_MODE_NONE = 0, /** 无效模式 */
    QCM_OPAUTH_MODE_CMCC = 1, /** 中国移动 */
    QCM_OPAUTH_MODE_CTCC = 2, /** 中国电信 */
    QCM_OPAUTH_MODE_CUCC = 3, /** 中国联通 */
} qcm_opauth_mode_e;

/* ======================== 认证方法枚举 ======================== */

/**
 * @enum qcm_opauth_method_e
 * @brief 认证方法
 */
typedef enum {
    QCM_OPAUTH_METHOD_NONE        = 0, /** 未指定 */
    QCM_OPAUTH_METHOD_SERVER_H5   = 1, /** 运营商服务器重定向认证 */
    QCM_OPAUTH_METHOD_INTERNAL_H5 = 2, /** 内置局域网 H5 认证门户 */
} qcm_opauth_method_e;

/* ======================== 状态机状态枚举 ======================== */

/**
 * @enum qcm_opauth_state_e
 * @brief 认证状态机状态
 */
typedef enum {
    QCM_OPAUTH_STATE_NONE          = 0, /** 未启动 */
    QCM_OPAUTH_STATE_CHECK_NETWORK,     /** 检查网络 */
    QCM_OPAUTH_STATE_PARAM_INIT,        /** 参数初始化 */
    QCM_OPAUTH_STATE_SIM_SIGN_QUERY,    /** SIM 签名参数查询 */
    QCM_OPAUTH_STATE_MODAL_PROCESS,     /** 认证初始化处理中 */
    QCM_OPAUTH_STATE_MODAL_DONE,        /** 认证初始化完成 */
    QCM_OPAUTH_STATE_MODAL_CLOSE,       /** 认证关闭 */
} qcm_opauth_state_e;

/** 内部认证支持的 SIM 槽位集合 */
#ifndef QCM_OPAUTH_INTERNAL_SIM_SLOTS
#define QCM_OPAUTH_INTERNAL_SIM_SLOTS {0,1,2}
#endif

/* ======================== 事件类型 ======================== */

/**
 * @enum qcm_opauth_event_e
 * @brief 事件类型，应用层可按事件枚举注册回调
 */
typedef enum {
    QCM_OPAUTH_EVT_STATE_CHANGED = 0,       /** 通用状态变更，每次状态迁移均触发，可用于统一的状态监控 */

    QCM_OPAUTH_EVT_STATE_CHECK_NETWORK,     /** 进入 CHECK_NETWORK 状态 */
    QCM_OPAUTH_EVT_STATE_PARAM_INIT,        /** 进入 PARAM_INIT 状态 */
    QCM_OPAUTH_EVT_STATE_SIM_SIGN_QUERY,    /** 进入 SIM_SIGN_QUERY 状态 */
    QCM_OPAUTH_EVT_STATE_MODAL_PROCESS,     /** 进入 MODAL_PROCESS 状态 */
    QCM_OPAUTH_EVT_STATE_MODAL_DONE,        /** 进入 MODAL_DONE 状态 */
    QCM_OPAUTH_EVT_STATE_MODAL_CLOSE,       /** 进入 MODAL_CLOSE 状态 */

    QCM_OPAUTH_EVT_MAX
} qcm_opauth_event_e;

/* ======================== 回调类型 ======================== */

/**
 * @brief 认证事件回调函数原型
 *
 * @param[in] event
 *          - 触发的事件类型
 * @param[in] event_data
 *          - 事件附加数据，类型取决于 event
 * @param[in] user_ctx
 *          - 注册时传入的用户上下文
 */
typedef void (*qcm_opauth_event_cb_t)(qcm_opauth_event_e event,
                                       void *event_data,
                                       void *user_ctx);

/**
 * @brief 自定义签名参数查询回调函数原型
 *
 * 在 SIM_SIGN_QUERY 阶段被调用，应用层在此回调中动态写入当前卡槽的
 * 运营商认证参数。
 *
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 非零值 失败，状态机将重试或关闭
 */
typedef int (*qcm_opauth_sign_query_fn_t)(void);

/* ======================== 卡槽数量 ======================== */

/** 最大卡槽数量 */
#define QCM_OPAUTH_SLOT_MAX  3

/* ======================== KV 格式凭据配置结构体 ======================== */

/** KV 凭据 key 最大长度 */
#define QCM_OPAUTH_CRED_KEY_LEN    16
/** KV 凭据 value 最大长度 */
#define QCM_OPAUTH_CRED_VAL_LEN    52
/** 单条凭据最多 KV 对数 */
#define QCM_OPAUTH_CRED_MAX_KV      3

/**
 * @struct qcm_opauth_cred_kv_t
 * @brief KV 凭据键值对
 */
typedef struct {
    char key[QCM_OPAUTH_CRED_KEY_LEN];   /*!< 键名 */
    char value[QCM_OPAUTH_CRED_VAL_LEN]; /*!< 键值 */
} qcm_opauth_cred_kv_t;

/**
 * @struct qcm_opauth_slot_credential_t
 * @brief SIM 卡槽 KV 凭据配置
 */
typedef struct {
    int                  slot_id;                              /*!< SIM 卡槽 ID */
    qcm_opauth_enable_e  auth_enable;                          /*!< 认证使能 */
    qcm_opauth_mode_e    mode;                                 /*!< 运营商模式 */
    qosa_uint8_t         kv_num;                               /*!< 有效 KV 对数 */
    qcm_opauth_cred_kv_t kvs[QCM_OPAUTH_CRED_MAX_KV];          /*!< KV 凭据数组 */
} qcm_opauth_slot_credential_t;

/* ======================== 初始化配置结构体 ======================== */

/**
 * @struct qcm_opauth_config_t
 * @brief 初始化配置
 */
typedef struct {
    qcm_opauth_method_e        method;              /*!< 认证方法 */
    qcm_opauth_sign_query_fn_t custom_param_query;  /*!< 自定义参数查询回调，可为 NULL */
} qcm_opauth_config_t;

/* ======================== SIMINFO 凭据存储 ======================== */

/** 最大存储 SIM 记录条数 */
#define QCM_OPAUTH_SIMINFO_MAX_RECORDS   3
/** 每条记录最大 KV 对数 */
#define QCM_OPAUTH_SIMINFO_MAX_KV_NUM    3
/** KV 键字段长度（字节） */
#define QCM_OPAUTH_SIMINFO_KEY_LEN       16
/** KV 值字段长度（字节） */
#define QCM_OPAUTH_SIMINFO_VAL_LEN       52
/** ICCID 字段长度（字节） */
#define QCM_OPAUTH_SIMINFO_ICCID_LEN     21

/**
 * @enum qcm_opauth_siminfo_err_e
 * @brief SIMINFO 存储操作错误码
 */
typedef enum {
    QCM_OPAUTH_SIMINFO_SUCCESS       = 0, /**< 成功 */
    QCM_OPAUTH_SIMINFO_ERR_PARAM     = 1, /**< 参数无效 */
    QCM_OPAUTH_SIMINFO_ERR_NOT_FOUND = 2, /**< 记录不存在 */
    QCM_OPAUTH_SIMINFO_ERR_CRC       = 3, /**< 数据损坏 */
    QCM_OPAUTH_SIMINFO_ERR_STORAGE   = 4, /**< 存储失败 */
    QCM_OPAUTH_SIMINFO_ERR_NOSPACE   = 5, /**< 空间不足 */
    QCM_OPAUTH_SIMINFO_ERR_GENERAL   = 6, /**< 一般错误 */
} qcm_opauth_siminfo_err_e;

/**
 * @enum qcm_opauth_siminfo_carrier_e
 * @brief 运营商类型
 */
typedef enum {
    QCM_OPAUTH_SIMINFO_CARRIER_UNKNOWN = 0, /**< 未知 */
    QCM_OPAUTH_SIMINFO_CARRIER_CMCC    = 1, /**< 中国移动 */
    QCM_OPAUTH_SIMINFO_CARRIER_CTCC    = 2, /**< 中国电信 */
    QCM_OPAUTH_SIMINFO_CARRIER_CUCC    = 3, /**< 中国联通 */
} qcm_opauth_siminfo_carrier_e;

/** KV 键名宏定义 */
#define QCM_OPAUTH_SIMINFO_KEY_APPID       "appid"      /**< 中国移动 */
#define QCM_OPAUTH_SIMINFO_KEY_PASSWORD    "password"   /**< 中国移动 */
#define QCM_OPAUTH_SIMINFO_KEY_APPKEY      "appKey"     /**< 中国电信 */
#define QCM_OPAUTH_SIMINFO_KEY_SECRETKEY   "secretKey"  /**< 中国电信 */

/**
 * @struct qcm_opauth_siminfo_kv_t
 * @brief 单个 KV 键值对
 */
typedef struct __attribute__((packed)) {
    char          key[QCM_OPAUTH_SIMINFO_KEY_LEN];   /**< 键名 */
    qosa_uint16_t value_len;                         /**< 值实际长度 */
    char          value[QCM_OPAUTH_SIMINFO_VAL_LEN]; /**< 值 */
} qcm_opauth_siminfo_kv_t;

/**
 * @struct qcm_opauth_siminfo_record_t
 * @brief 单条 SIM 凭据记录
 */
typedef struct __attribute__((packed)) {
    qosa_uint8_t            valid;                      /**< 1=本槽位有效 */
    qosa_uint8_t            rec_version;                /**< 记录格式版本号 */
    qosa_uint8_t            carrier;                    /**< 运营商枚举值 */
    qosa_uint8_t            kv_num;                     /**< 已使用 KV 槽位数 */
    char                    iccid[QCM_OPAUTH_SIMINFO_ICCID_LEN]; /**< 主键 ICCID */
    qosa_uint8_t            reserved[3];                /**< 保留 */
    qcm_opauth_siminfo_kv_t kv[QCM_OPAUTH_SIMINFO_MAX_KV_NUM];   /**< KV 数组 */
    qosa_uint32_t           seq;                        /**< 写入序号 */
    qosa_uint32_t           crc32;                      /**< 记录级 CRC32 */
} qcm_opauth_siminfo_record_t;

/* ======================== 白名单自动同步 ======================== */
typedef enum {
    QCM_OPAUTH_WHITELIST_SYNC_DISABLE = 0, /**< 关闭白名单同步（默认）*/
    QCM_OPAUTH_WHITELIST_SYNC_ENABLE  = 1, /**< 使能白名单同步 */
} qcm_opauth_whitelist_sync_e;

/* ======================== APP调用接口函数 ======================== */

/**
 * @brief 写入 SIM 卡槽凭据（KV 格式）
 *
 * @note 仅在 qcm_opauth_init() 之后、qcm_opauth_start() 之前可调用。
 *
 * @param[in] cred
 *          - 凭据配置指针
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_set_slot_credential(const qcm_opauth_slot_credential_t *cred);

/**
 * @brief 运行时更新当前卡槽凭据
 *
 * @param[in] cred
 *          - 凭据配置指针
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_update_slot_credential(const qcm_opauth_slot_credential_t *cred);

/**
 * @brief 读取指定卡槽的凭据
 *
 * @param[in] slot_id
 *          - 卡槽索引
 * @param[out] cred
 *          - 凭据输出缓冲区
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_get_slot_credential(int slot_id, qcm_opauth_slot_credential_t *cred);

/**
 * @brief 初始化运营商二次认证模块
 *
 * @param[in] config
 *          - 初始化配置指针
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_init(const qcm_opauth_config_t *config);

/**
 * @brief 启动认证初始化流程
 *
 * @note 调用前调用一次qcm_opauth_set_slot_credential() 预配置卡槽参数
 *       或者运行时在qcm_opauth_sign_query_fn_t中更新当前卡槽凭据。
 *
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_start(void);

/**
 * @brief 注册认证事件回调
 *
 * @param[in] event
 *          - 要监听的事件类型
 * @param[in] cb
 *          - 回调函数指针
 * @param[in] user_ctx
 *          - 用户上下文，回调时透传
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_register_event_cb(qcm_opauth_event_e event,
                                   qcm_opauth_event_cb_t cb,
                                   void *user_ctx);

/**
 * @brief 查询当前 SIM 是否使能认证
 *
 * @return int
 *       - 1 使能认证
 *       - 0 未使能
 */
int qcm_opauth_is_auth_enabled(void);

/**
 * @brief 获取当前认证状态
 *
 * @return qcm_opauth_state_e 当前状态机状态
 */
qcm_opauth_state_e qcm_opauth_get_state(void);

/**
 * @brief 获取mifi设备当前 SIM 卡 ICCID
 *
 * @param[out] iccid
 *          - ICCID 输出缓冲区
 * @param[in] len
 *          - 缓冲区长度
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_get_current_iccid(char *iccid, int len);

/**
 * @brief 获取mifi设备 MAC 地址
 *
 * @param[out] mac
 *          - MAC 地址输出缓冲区
 * @param[in] len
 *          - 缓冲区长度
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_get_device_mac(char *mac, int len);

/**
 * @brief 获取mifi设备 IMEI
 *
 * @param[out] imei
 *          - IMEI 输出缓冲区
 * @param[in] len
 *          - 缓冲区长度
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_get_device_imei(char *imei, int len);

/**
 * @brief 重启认证流程
 *
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_restart(void);

/**
 * @brief 使能/禁用终端列表同步
 *
 * @param[in] enable
 *          - QCM_OPAUTH_WHITELIST_SYNC_ENABLE  使能
 *          - QCM_OPAUTH_WHITELIST_SYNC_DISABLE 禁用
 * @return qcm_opauth_err_e
 *       - QCM_OPAUTH_RES_SUCCESS(0) 成功
 *       - 其他值 失败
 */
int qcm_opauth_terminal_list_sync_set(qcm_opauth_whitelist_sync_e enable);

/**
 * @brief 查询终端列表同步使能状态
 *
 * @return 1 使能，0 未使能，-1 上下文未初始化
 */
int qcm_opauth_terminal_list_sync_get(void);

/**
 * @brief 写入指定 ICCID 的 KV 键值对
 *
 * @param[in] iccid   SIM 卡 ICCID
 * @param[in] carrier 运营商类型
 * @param[in] key     KV 键名（必须使用 QCM_OPAUTH_SIMINFO_KEY_* 宏）
 * @param[in] value   KV 值
 *
 * @return qcm_opauth_siminfo_err_e
 */
int qcm_opauth_siminfo_write_kv(const char *iccid,
                                 qcm_opauth_siminfo_carrier_e carrier,
                                 const char *key,
                                 const char *value);

/**
 * @brief 读取指定 ICCID 的某个 KV 值
 *
 * @param[in]  iccid   SIM 卡 ICCID
 * @param[in]  key     KV 键名
 * @param[out] buf     输出缓冲区
 * @param[in]  buf_len 缓冲区长度
 *
 * @return qcm_opauth_siminfo_err_e
 */
int qcm_opauth_siminfo_read_kv(const char *iccid,
                                const char *key,
                                char *buf,
                                int buf_len);

/**
 * @brief 删除指定 ICCID 的记录
 *
 * @param[in] iccid SIM 卡 ICCID
 *
 * @return qcm_opauth_siminfo_err_e
 */
int qcm_opauth_siminfo_del(const char *iccid);

/**
 * @brief 按 ICCID 读取记录并填充为卡槽凭据（KV + 认证模式）
 *
 * 内部完成：记录查找 -> carrier 映射为 mode -> KV 逐对搬运。
 * @note 不修改 @p cred 的 slot_id 与 auth_enable —— 这两项属于应用层策略。
 * @note 无对应记录时返回 QCM_OPAUTH_SIMINFO_ERR_NOT_FOUND，@p cred 保持不变。
 *
 * @param[in]  iccid SIM 卡 ICCID
 * @param[out] cred  凭据输出（只需预先填好 slot_id / auth_enable）
 *
 * @return qcm_opauth_siminfo_err_e
 */
int qcm_opauth_siminfo_fill_credential(const char *iccid,
                                       qcm_opauth_slot_credential_t *cred);

/**
 * @brief 读取指定 ICCID 的完整记录
 *
 * @param[in]  iccid SIM 卡 ICCID
 * @param[out] out   输出记录
 *
 * @return qcm_opauth_siminfo_err_e
 */
int qcm_opauth_siminfo_get_by_iccid(const char *iccid,
                                     qcm_opauth_siminfo_record_t *out);

/**
 * @brief 枚举所有已保存的记录摘要
 *
 * @param[out] out_array 输出数组
 * @param[in]  max_num   数组容量
 * @param[out] real_num  实际记录数
 *
 * @return qcm_opauth_siminfo_err_e
 */
int qcm_opauth_siminfo_list(qcm_opauth_siminfo_record_t *out_array,
                             qosa_uint32_t max_num,
                             qosa_uint32_t *real_num);

#endif /* __QCM_OPAUTH_H__ */
