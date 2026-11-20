#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "qosa_rtc.h"
#include "agora_token.h"

#include <driver/trng.h>
#include <driver/aon_rtc.h>
// 引入 OpenSSL 的 HMAC-SHA256
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/md.h"
// 引入 zlib
#include <zlib.h>


#define QOS_LOG_TAG LOG_TAG_DEMO

// ==================== 辅助函数声明 ====================

static uint32_t get_current_timestamp(void);
static uint32_t generate_salt(void);
static int is_uuid(const char* str);
static void hmac_sha256(const uint8_t* msg, size_t msg_len,
                        const uint8_t* key, size_t key_len,
                        uint8_t out[32]);
static char* base64_encode(const uint8_t* data, size_t len);
static uint8_t* base64_decode(const char* str, size_t* out_len);
static uint8_t* compress_data(const uint8_t* data, size_t len, size_t* comp_len);
static uint8_t* decompress_data(const uint8_t* data, size_t len, size_t* decomp_len);

// ==================== Packer / Unpacker ====================

typedef struct {
    uint8_t* buf;
    size_t size;
    size_t pos;
} packer_t;

static void packer_init(packer_t* p, uint8_t* buf, size_t size) {
    p->buf = buf;
    p->size = size;
    p->pos = 0;
}

static void pack_uint16(packer_t* p, uint16_t v) {
    if (p->pos + 2 > p->size) return;
    qosa_memcpy(p->buf+p->pos, &v,sizeof(v));
    p->pos += sizeof(v);
    // p->buf[p->pos++] = v & 0xFF;
    // p->buf[p->pos++] = (v >> 8) & 0xFF;
}

static void pack_uint32(packer_t* p, uint32_t v) {
    if (p->pos + 4 > p->size) return;
    qosa_memcpy(p->buf+p->pos, &v,sizeof(v));
    p->pos += sizeof(v);
    // p->buf[p->pos++] = v & 0xFF;
    // p->buf[p->pos++] = (v >> 8) & 0xFF;
    // p->buf[p->pos++] = (v >> 16) & 0xFF;
    // p->buf[p->pos++] = (v >> 24) & 0xFF;
}

static void pack_string(packer_t* p, const char* s) {
    uint16_t len = (uint16_t)strlen(s);
    pack_uint16(p, len);
    if (p->pos + len > p->size) return;
    qosa_memcpy(p->buf + p->pos, s, len);
    p->pos += len;
}

// 类似地，可以定义 unpacker 函数，但构建 Token 只需要 pack，故省略

// ==================== Service 结构体 ====================

typedef struct service_s {
    uint16_t type;
    uint16_t privilege_count;
    uint16_t* privilege_ids;      // 动态数组
    uint32_t* privilege_expires;  // 动态数组
    // 派生类特有字段，通过 union 或单独结构体管理
    // 为简化，我们直接在函数中处理不同服务类型的序列化
} service_t;

static service_t* service_create(uint16_t type) {
    service_t* s = (service_t*)qosa_calloc(1, sizeof(service_t));
    s->type = type;
    return s;
}

static void service_destroy(service_t* s) {
    if (s) {
        qosa_free(s->privilege_ids);
        qosa_free(s->privilege_expires);
        qosa_free(s);
    }
}

static void service_add_privilege(service_t* s, uint16_t priv_id, uint32_t expire) {
    s->privilege_count++;
    s->privilege_ids = qosa_realloc(s->privilege_ids, s->privilege_count * sizeof(uint16_t));
    s->privilege_expires = qosa_realloc(s->privilege_expires, s->privilege_count * sizeof(uint32_t));
    s->privilege_ids[s->privilege_count - 1] = priv_id;
    s->privilege_expires[s->privilege_count - 1] = expire;
}

// 序列化 service 到 packer（不包含 service 类型，因为外层已写）
static void service_pack(packer_t* p, service_t* s) {
    pack_uint16(p, s->privilege_count);
    for (size_t i = 0; i < s->privilege_count; i++) {
        pack_uint16(p, s->privilege_ids[i]);
        pack_uint32(p, s->privilege_expires[i]);
    }
    // 根据类型写入额外字段
    switch (s->type) {
        case SERVICE_RTC: {
            // 额外字段需要从外部传入，我们使用一个全局变量暂存？实际调用时会通过特定函数设置。
            // 简化：在添加 RTC 服务时，我们单独保存 channel 和 account，在序列化时通过 service 结构体中的指针获取。
            // 但为了代码清晰，我们使用一个扩展结构体。
            // 这里仅作示意，实际实现中应使用专门的 RTC 服务结构体。
            break;
        }
        default:
            break;
    }
}

// ==================== RTC 服务扩展结构体 ====================

typedef struct service_rtc_s {
    service_t base;
    char channel_name[256];
    char account[256];
} service_rtc_t;

static service_rtc_t* service_rtc_create(const char* channel, const char* account) {
    service_rtc_t* s = (service_rtc_t*)qosa_calloc(1, sizeof(service_rtc_t));
    s->base.type = SERVICE_RTC;
    qosa_strncpy(s->channel_name, channel, sizeof(s->channel_name) - 1);
    qosa_strncpy(s->account, account, sizeof(s->account) - 1);
    return s;
}

static void service_rtc_destroy(service_rtc_t* s) {
    if (s) {
        qosa_free(s->base.privilege_ids);
        qosa_free(s->base.privilege_expires);
        qosa_free(s);
    }
}

static void service_rtc_pack(packer_t* p, service_rtc_t* s) {
    // 先打包基类的特权
    pack_uint16(p, s->base.privilege_count);
    for (size_t i = 0; i < s->base.privilege_count; i++) {
        pack_uint16(p, s->base.privilege_ids[i]);
        pack_uint32(p, s->base.privilege_expires[i]);
    }
    // 再打包 RTC 特有字段
    pack_string(p, s->channel_name);
    pack_string(p, s->account);
}

// ==================== RTM 服务扩展结构体 ====================

typedef struct service_rtm_s {
    service_t base;
    char user_id[256];
} service_rtm_t;

static service_rtm_t* service_rtm_create(const char* user_id) {
    service_rtm_t* s = (service_rtm_t*)qosa_calloc(1, sizeof(service_rtm_t));
    s->base.type = SERVICE_RTM;
    qosa_strncpy(s->user_id, user_id, sizeof(s->user_id) - 1);
    return s;
}

static void service_rtm_destroy(service_rtm_t* s) {
    if (s) {
        qosa_free(s->base.privilege_ids);
        qosa_free(s->base.privilege_expires);
        qosa_free(s);
    }
}

static void service_rtm_pack(packer_t* p, service_rtm_t* s) {
    pack_uint16(p, s->base.privilege_count);
    for (size_t i = 0; i < s->base.privilege_count; i++) {
        pack_uint16(p, s->base.privilege_ids[i]);
        pack_uint32(p, s->base.privilege_expires[i]);
    }
    pack_string(p, s->user_id);
}

// ==================== AccessToken2 结构体 ====================

struct access_token2_s {
    uint32_t issue_ts;
    uint32_t expire;
    uint32_t salt;
    char app_id[128];
    char app_cert[128];
    uint8_t signature[32];   // 实际构建时填充
    // 服务列表：使用 void* 数组，每个元素指向一个服务对象（如 service_rtc_t*）
    void** services;
    size_t service_count;
};

access_token2_t* access_token2_create(const char* app_id, const char* app_cert,
                                      uint32_t issue_ts, uint32_t expire) {
    access_token2_t* token = (access_token2_t*)qosa_calloc(1, sizeof(access_token2_t));
    qosa_strncpy(token->app_id, app_id, sizeof(token->app_id) - 1);
    qosa_strncpy(token->app_cert, app_cert, sizeof(token->app_cert) - 1);
    token->issue_ts = issue_ts ? issue_ts : get_current_timestamp();
    token->expire = expire;
    token->salt = generate_salt();
    token->services = NULL;
    token->service_count = 0;
    return token;
}

void access_token2_destroy(access_token2_t* token) {
    if (!token) return;
    for (size_t i = 0; i < token->service_count; i++) {
        service_t* s = (service_t*)token->services[i];
        // 根据类型释放不同的结构体（简化：这里假设都是 service_rtc_t 或 service_rtm_t，实际需判断）
        // 为简化示例，我们只释放基类部分，完整实现需要根据类型调用对应析构。
        qosa_free(s->privilege_ids);
        qosa_free(s->privilege_expires);
        qosa_free(s);
    }
    qosa_free(token->services);
    qosa_free(token);
}

int access_token2_add_service_rtc(access_token2_t* token,
                                  const char* channel_name,
                                  const char* account,
                                  const uint16_t* privilege_ids,
                                  const uint32_t* privilege_expires,
                                  size_t privilege_count) {
    QLOGI("channel_name:%s\n",channel_name);
    service_rtc_t* s = service_rtc_create(channel_name, account);
    for (size_t i = 0; i < privilege_count; i++) {
        service_add_privilege((service_t*)s, privilege_ids[i], privilege_expires[i]);
    }
    token->service_count++;
    token->services = qosa_realloc(token->services, token->service_count * sizeof(void*));
    token->services[token->service_count - 1] = s;
    return 0;
}

int access_token2_add_service_rtm(access_token2_t* token,
                                  const char* user_id,
                                  uint16_t privilege_id,
                                  uint32_t privilege_expire) {
    service_rtm_t* s = service_rtm_create(user_id);
    service_add_privilege((service_t*)s, privilege_id, privilege_expire);
    token->service_count++;
    token->services = qosa_realloc(token->services, token->service_count * sizeof(void*));
    token->services[token->service_count - 1] = s;
    return 0;
}

// 计算 signing（与 C++ 版本一致）
static void compute_signing(access_token2_t* token, uint8_t out[32]) {
    uint8_t step1[32] = {0};
    uint8_t buf[4] = {0};
    packer_t p;
    packer_init(&p, buf, 4);
    QLOGI("issue_ts:%d\n", token->issue_ts);
    pack_uint32(&p, token->issue_ts);
    for(int k = 0; k < p.pos; k++){
        QLOGI("%02x", buf[k]);
    }
    QLOGI("\n");
    QLOGI("appCert:%s\n", token->app_cert);
    hmac_sha256(buf, 4, (uint8_t*)token->app_cert, strlen(token->app_cert), step1);
    // printf("step1:\n");
    // for(int i = 0; i <32; i++)
    //     printf("%02x", step1[i]);
    // printf("\n");
    packer_init(&p, buf, 4);
    QLOGI("salt:%d\n", token->salt);
    pack_uint32(&p, token->salt);
    // for(int k = 0; k < p.pos; k++){
    //     printf("%02x", buf[k]);
    // }
    // printf("\n");
    hmac_sha256(buf, 4, step1, 32, out);
    // printf("out:\n");
    // for(int i = 0; i <32; i++)
    //     printf("%02x", out[i]);
    // printf("\n");
}

// 构建 signing_info 并返回其二进制数据和长度
static uint8_t* build_signing_info(access_token2_t* token, size_t* out_len) {
    // 估算大小：app_id + 3个uint32 + 服务个数 + 各服务序列化
    size_t buf_size = 1024;
    uint8_t* buf = (uint8_t*)qosa_malloc(buf_size);
    packer_t p;
    packer_init(&p, buf, buf_size);

    pack_string(&p, token->app_id);
    pack_uint32(&p, token->issue_ts);
    pack_uint32(&p, token->expire);
    pack_uint32(&p, token->salt);
    // 写入服务数量
    pack_uint16(&p, (uint16_t)token->service_count);
    for (size_t i = 0; i < token->service_count; i++) {
        service_t* s = (service_t*)token->services[i];
        // 先写服务类型
        pack_uint16(&p, s->type);
        // 根据类型调用对应的 pack 函数
        if (s->type == SERVICE_RTC) {
            service_rtc_pack(&p, (service_rtc_t*)s);
        } else if (s->type == SERVICE_RTM) {
            service_rtm_pack(&p, (service_rtm_t*)s);
        } else {
            // 其他类型暂不支持，直接跳过
            pack_uint16(&p, 0); // privilege_count = 0
        }
    }
    *out_len = p.pos;
    // printf("sign_info:\n");
    // for(int k = 0; k < p.pos; k++){
    //     printf("%02x", buf[k]);
    // }
    // printf("\n");
    return buf;
}

char* access_token2_build(access_token2_t* token) {
    // 1. 检查
    if (!is_uuid(token->app_id) || !is_uuid(token->app_cert) || token->service_count == 0) {
        return NULL;
    }

    // 2. 计算 signing
    uint8_t signing[32];
    compute_signing(token, signing);

    // 3. 构建 signing_info
    size_t info_len;
    uint8_t* info = build_signing_info(token, &info_len);
    if (!info) return QOSA_NULL;

    // 4. 计算最终签名
    uint8_t final_sig[32];
    hmac_sha256(signing, 32, info, info_len, final_sig);
    // for(int i = 0; i <32;i++)
    //     printf("%02x",final_sig[i]);
    // printf("\n");
    // 5. 组合 signature + signing_info
    size_t combined_len = 2+32 + info_len;
    uint8_t* combined = (uint8_t*)malloc(combined_len);
    uint16_t final_sig_len = 32;
    memcpy(combined, &final_sig_len, 2);
    memcpy(combined+2, final_sig, 32);
    memcpy(combined + 2+32, info, info_len);
    qosa_free(info);
    // printf("combined:\n");
    // for(int i = 0; i < combined_len; i++)
    //     printf("%02x",combined[i]);
    // printf("\n");
    // 6. 压缩
    size_t comp_len;
    uint8_t* compressed = compress_data(combined, combined_len, &comp_len);
    qosa_free(combined);
    if (!compressed) return QOSA_NULL;
    // printf("compressed:\n");
    // for(int i = 0; i < comp_len; i++)
    //     printf("%02x",compressed[i]);
    // printf("\n");
    // 7. Base64 编码
    char* b64 = base64_encode(compressed, comp_len);
    qosa_free(compressed);
    if (!b64) return QOSA_NULL;
    // printf("b64:%s\n",b64);
    // 8. 拼接版本号
    char* token_str = (char*)qosa_malloc(strlen(AGORA_TOKEN_VERSION) + strlen(b64) + 1);
    qosa_sprintf(token_str, "%s%s", AGORA_TOKEN_VERSION, b64);
    qosa_free(b64);
    return token_str;
}

// ==================== 便捷函数实现 ====================

char* rtc_token_builder_build_with_uid(const char* app_id,
                                       const char* app_cert,
                                       const char* channel_name,
                                       uint32_t uid,
                                       user_role_t role,
                                       uint32_t token_expire,
                                       uint32_t privilege_expire) {
    char account[32] = {0};
    if (uid != 0) {
        qosa_sprintf(account, "%u", uid);
    }
    return rtc_token_builder_build_with_account(app_id, app_cert, channel_name,
                                                uid == 0 ? "" : account,
                                                role, token_expire, privilege_expire);
}

char* rtc_token_builder_build_with_account(const char* app_id,
                                           const char* app_cert,
                                           const char* channel_name,
                                           const char* account,
                                           user_role_t role,
                                           uint32_t token_expire,
                                           uint32_t privilege_expire) {
    access_token2_t* token = access_token2_create(app_id, app_cert, 0, token_expire);
    // 准备特权数组
    uint16_t priv_ids[4];
    uint32_t priv_exp[4];
    size_t count = 1;
    priv_ids[0] = PRIV_RTC_JOIN_CHANNEL;
    priv_exp[0] = privilege_expire;
    if (role == ROLE_PUBLISHER) {
        priv_ids[1] = PRIV_RTC_PUBLISH_AUDIO;
        priv_exp[1] = privilege_expire;
        priv_ids[2] = PRIV_RTC_PUBLISH_VIDEO;
        priv_exp[2] = privilege_expire;
        priv_ids[3] = PRIV_RTC_PUBLISH_DATA;
        priv_exp[3] = privilege_expire;
        count = 4;
    }
    access_token2_add_service_rtc(token, channel_name, account, priv_ids, priv_exp, count);
    char* result = access_token2_build(token);
    access_token2_destroy(token);
    return result;
}

char* rtc_token_builder_build_with_uid_ex(const char* app_id,
                                          const char* app_cert,
                                          const char* channel_name,
                                          uint32_t uid,
                                          uint32_t token_expire,
                                          uint32_t join_channel_expire,
                                          uint32_t pub_audio_expire,
                                          uint32_t pub_video_expire,
                                          uint32_t pub_data_expire) {
    char account[32] = {0};
    if (uid != 0) {
        qosa_sprintf(account, "%u", uid);
    }
    return rtc_token_builder_build_with_account_ex(app_id, app_cert, channel_name,
                                                   uid == 0 ? "" : account,
                                                   token_expire, join_channel_expire,
                                                   pub_audio_expire, pub_video_expire,
                                                   pub_data_expire);
}

char* rtc_token_builder_build_with_account_ex(const char* app_id,
                                              const char* app_cert,
                                              const char* channel_name,
                                              const char* account,
                                              uint32_t token_expire,
                                              uint32_t join_channel_expire,
                                              uint32_t pub_audio_expire,
                                              uint32_t pub_video_expire,
                                              uint32_t pub_data_expire) {
    access_token2_t* token = access_token2_create(app_id, app_cert, 0, token_expire);
    uint16_t priv_ids[4] = {PRIV_RTC_JOIN_CHANNEL, PRIV_RTC_PUBLISH_AUDIO,
                            PRIV_RTC_PUBLISH_VIDEO, PRIV_RTC_PUBLISH_DATA};
    uint32_t priv_exp[4] = {join_channel_expire, pub_audio_expire,
                            pub_video_expire, pub_data_expire};
    access_token2_add_service_rtc(token, channel_name, account, priv_ids, priv_exp, 4);
    char* result = access_token2_build(token);
    access_token2_destroy(token);
    return result;
}

char* rtc_token_builder_build_with_rtm(const char* app_id,
                                       const char* app_cert,
                                       const char* channel_name,
                                       const char* account,
                                       user_role_t role,
                                       uint32_t token_expire,
                                       uint32_t privilege_expire,
                                       const char* rtm_user_id,
                                       uint32_t rtm_expire) {
    access_token2_t* token = access_token2_create(app_id, app_cert, 0, token_expire);
    // RTC 部分
    uint16_t rtc_priv_ids[4];
    uint32_t rtc_priv_exp[4];
    size_t rtc_count = 1;
    rtc_priv_ids[0] = PRIV_RTC_JOIN_CHANNEL;
    rtc_priv_exp[0] = privilege_expire;
    if (role == ROLE_PUBLISHER) {
        rtc_priv_ids[1] = PRIV_RTC_PUBLISH_AUDIO;
        rtc_priv_exp[1] = privilege_expire;
        rtc_priv_ids[2] = PRIV_RTC_PUBLISH_VIDEO;
        rtc_priv_exp[2] = privilege_expire;
        rtc_priv_ids[3] = PRIV_RTC_PUBLISH_DATA;
        rtc_priv_exp[3] = privilege_expire;
        rtc_count = 4;
    }
    access_token2_add_service_rtc(token, channel_name, account, rtc_priv_ids, rtc_priv_exp, rtc_count);
    // RTM 部分
    access_token2_add_service_rtm(token, rtm_user_id, PRIV_RTM_LOGIN, rtm_expire);
    char* result = access_token2_build(token);
    access_token2_destroy(token);
    return result;
}

// ==================== 辅助函数实现（需要外部库） ====================

static uint32_t get_current_timestamp(void) {
    // struct timeval get_time = {0};

	// bk_rtc_gettimeofday(&get_time,NULL);
    // return get_time.tv_sec;
    return qosa_get_system_time_seconds();
}

static uint32_t generate_salt(void) {
    // 简单随机数，实际应使用更安全的随机源
    return  ((uint32_t)qosa_rand()) ^ ((uint32_t)qosa_get_system_time_seconds());
}

static int is_uuid(const char* str) {
    // 简单校验：长度 32 或 36，仅包含十六进制字符和连字符
    if (!str) return 0;
    size_t len = qosa_strlen(str);
    if (len != 32 && len != 36) return 0;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == '-'))
            return 0;
    }
    return 1;
}

static void hmac_sha256(const uint8_t* key, size_t key_len,
                        const uint8_t* msg, size_t msg_len,                        
                        uint8_t out[32]) {
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    QLOGI("key_len:%d,msg_len:%d\n", key_len, msg_len);
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, md_info, 1); // 1 表示使用 HMAC
    mbedtls_md_hmac_starts(&ctx, key, key_len);
    mbedtls_md_hmac_update(&ctx, msg, msg_len);
    mbedtls_md_hmac_finish(&ctx, out);
    mbedtls_md_free(&ctx);
}

static char* base64_encode(const uint8_t* data, size_t len) {
    size_t out_len = 0;
    // 计算所需缓冲区大小：每3字节编码为4字符，加上结尾'\0'
    size_t max_len = ((len + 2) / 3) * 4 + 1;
    char* out = (char*)qosa_malloc(max_len);
    if (!out) return QOSA_NULL;

    int ret = mbedtls_base64_encode((unsigned char*)out, max_len, &out_len,
                                    data, len);
    if (ret != 0) {
        qosa_free(out);
        return QOSA_NULL;
    }
    out[out_len] = '\0';
    return out;
}

static uint8_t* base64_decode(const char* str, size_t* out_len) {
    size_t len = qosa_strlen(str);
    uint8_t* out = (uint8_t*)qosa_malloc(len); // 解码后长度不超过原长度
    if (!out) return QOSA_NULL;

    size_t out_len_tmp = 0;
    int ret = mbedtls_base64_decode(out, len, &out_len_tmp,
                                    (const unsigned char*)str, len);
    if (ret != 0) {
        qosa_free(out);
        return QOSA_NULL;
    }
    *out_len = out_len_tmp;
    return out;
}

static uint8_t* compress_data(const uint8_t* data, size_t len, size_t* comp_len) {
    uLongf dest_len = compressBound((uLong)len);
    uint8_t* dest = (uint8_t*)qosa_malloc(dest_len);
    int ret = compress2(dest, &dest_len, data, (uLong)len, Z_BEST_COMPRESSION);
    if (ret != Z_OK) {
        qosa_free(dest);
        return QOSA_NULL;
    }
    *comp_len = dest_len;
    return dest;
}

static uint8_t* decompress_data(const uint8_t* data, size_t len, size_t* decomp_len) {
    // 解压需要已知原始大小，此函数在构建时不需要，故简单实现占位
    return QOSA_NULL;
}