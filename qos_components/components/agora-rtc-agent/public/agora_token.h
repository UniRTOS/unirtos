#ifndef AGORA_TOKEN_H
#define AGORA_TOKEN_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 版本常量
#define AGORA_TOKEN_VERSION "007"

// 服务类型
#define SERVICE_RTC  1
#define SERVICE_RTM  2
#define SERVICE_FPA  4
#define SERVICE_CHAT 5
#define SERVICE_APAAS 7

// RTC 特权
#define PRIV_RTC_JOIN_CHANNEL     1
#define PRIV_RTC_PUBLISH_AUDIO    2
#define PRIV_RTC_PUBLISH_VIDEO    3
#define PRIV_RTC_PUBLISH_DATA     4

// RTM 特权
#define PRIV_RTM_LOGIN            1

// 用户角色（用于简单 API）
typedef enum {
    ROLE_PUBLISHER  = 1,
    ROLE_SUBSCRIBER = 2
} user_role_t;

// 前向声明
typedef struct access_token2_s access_token2_t;

/**
 * 创建 AccessToken2 实例
 * @param app_id 声网 App ID
 * @param app_cert 声网 App Certificate
 * @param issue_ts 签发时间戳（0 表示当前时间）
 * @param expire 凭证有效期（秒）
 * @return 返回新创建的 token 对象，使用完毕后需调用 access_token2_destroy 释放
 */
access_token2_t* access_token2_create(const char* app_id, const char* app_cert,
                                      uint32_t issue_ts, uint32_t expire);

/**
 * 销毁 token 对象
 */
void access_token2_destroy(access_token2_t* token);

/**
 * 添加 RTC 服务（使用字符串 account）
 * @param token token 对象
 * @param channel_name 频道名
 * @param account 用户 account（字符串形式，若 uid 为整数需自行转为字符串）
 * @param privileges 特权数组，每个元素为 (privilege_id, expire_timestamp)
 * @param privilege_count 特权个数
 * @return 0 成功，-1 失败
 */
int access_token2_add_service_rtc(access_token2_t* token,
                                  const char* channel_name,
                                  const char* account,
                                  const uint16_t* privilege_ids,
                                  const uint32_t* privilege_expires,
                                  size_t privilege_count);

/**
 * 添加 RTM 服务
 */
int access_token2_add_service_rtm(access_token2_t* token,
                                  const char* user_id,
                                  uint16_t privilege_id,
                                  uint32_t privilege_expire);

/**
 * 构建最终的 Token 字符串
 * @param token token 对象
 * @return 新分配的 Token 字符串，调用者需 free；失败返回 NULL
 */
char* access_token2_build(access_token2_t* token);

// ========== RtcTokenBuilder2 风格的便捷函数 ==========

/**
 * 使用整数 UID 构建 RTC Token（简单角色模式）
 * @param app_id
 * @param app_cert
 * @param channel_name
 * @param uid 用户 ID（0 表示不校验）
 * @param role ROLE_PUBLISHER 或 ROLE_SUBSCRIBER
 * @param token_expire 凭证有效期（秒）
 * @param privilege_expire 权限有效期（秒，通常与 token_expire 相同）
 * @return 新分配的 Token 字符串，调用者 free
 */
char* rtc_token_builder_build_with_uid(const char* app_id,
                                       const char* app_cert,
                                       const char* channel_name,
                                       uint32_t uid,
                                       user_role_t role,
                                       uint32_t token_expire,
                                       uint32_t privilege_expire);

/**
 * 使用字符串 account 构建 RTC Token（简单角色模式）
 */
char* rtc_token_builder_build_with_account(const char* app_id,
                                           const char* app_cert,
                                           const char* channel_name,
                                           const char* account,
                                           user_role_t role,
                                           uint32_t token_expire,
                                           uint32_t privilege_expire);

/**
 * 使用整数 UID 构建 RTC Token（精细权限模式）
 * 各权限过期时间独立
 */
char* rtc_token_builder_build_with_uid_ex(const char* app_id,
                                          const char* app_cert,
                                          const char* channel_name,
                                          uint32_t uid,
                                          uint32_t token_expire,
                                          uint32_t join_channel_expire,
                                          uint32_t pub_audio_expire,
                                          uint32_t pub_video_expire,
                                          uint32_t pub_data_expire);

/**
 * 使用字符串 account 构建 RTC Token（精细权限模式）
 */
char* rtc_token_builder_build_with_account_ex(const char* app_id,
                                              const char* app_cert,
                                              const char* channel_name,
                                              const char* account,
                                              uint32_t token_expire,
                                              uint32_t join_channel_expire,
                                              uint32_t pub_audio_expire,
                                              uint32_t pub_video_expire,
                                              uint32_t pub_data_expire);

/**
 * 构建同时包含 RTC 和 RTM 权限的 Token
 * @param rtm_user_id RTM 用户 ID
 * @param rtm_expire RTM 权限有效期（秒）
 */
char* rtc_token_builder_build_with_rtm(const char* app_id,
                                       const char* app_cert,
                                       const char* channel_name,
                                       const char* account,
                                       user_role_t role,
                                       uint32_t token_expire,
                                       uint32_t privilege_expire,
                                       const char* rtm_user_id,
                                       uint32_t rtm_expire);

#ifdef __cplusplus
}
#endif

#endif // AGORA_TOKEN_C_H