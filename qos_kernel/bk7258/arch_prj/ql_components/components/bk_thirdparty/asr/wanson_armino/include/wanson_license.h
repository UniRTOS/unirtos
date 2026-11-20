#ifndef __WANSON_LICENSE_H__
#define __WANSON_LICENSE_H__

#include <stdint.h>

/**
 * @brief Authorize the user with Wanson.
 * @brief 开始获取上海华镇中文语音识别授权凭证.
 *
 * This function checks if the Wanson OTP authorization is ready. If not, it retrieves
 * the user ID, queries the authorizations, and applies for them.
 *
 * @param user_id The user ID to be authorized.
 * @return BK_OK on success, BK_FAIL on failure.
 */
#if(CONFIG_WANSON_CN_LICENSE && CONFIG_SYS_CPU0)
int wanson_authorization(const char* user_id);
#endif
#endif // __WANSON_LICENSE_H__