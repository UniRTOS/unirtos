/*****************************************************************/ /**
* @file qosa_mifi_storage.h
* @brief MIFI KV and Blob storage public interface
* @author
* @date 2026-07-22
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par Module revision history
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-07-22 <td>1.0 <td>Initial <td>Initial version
* </table>
**********************************************************************/

#ifndef __QOSA_MIFI_STORAGE_H__
#define __QOSA_MIFI_STORAGE_H__

#include "qosa_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QOSA_MIFI_STORAGE_SSID_MAX_LEN          32U  /*!< Maximum SSID length in bytes */
#define QOSA_MIFI_STORAGE_PASSWORD_MAX_LEN      64U  /*!< Maximum WLAN password length in bytes */
#define QOSA_MIFI_STORAGE_LANGUAGE_MAX_LEN      15U  /*!< Maximum language tag length in bytes */
#define QOSA_MIFI_STORAGE_PDP_PROFILE_MAX_LEN   576U /*!< Maximum PDP Profile Blob payload length in bytes */
#define QOSA_MIFI_STORAGE_ACCOUNT_MAX_LEN       256U /*!< Maximum serialized account config length in bytes */
#define QOSA_MIFI_STORAGE_ACCOUNT_MAX_COUNT     4U   /*!< Maximum number of persistent accounts */
#define QOSA_MIFI_STORAGE_WIFI_DUAL_BAND_BLOB_MAX_LEN 256U /*!< Maximum dual-band WiFi Blob payload length in bytes */
#define QOSA_MIFI_STORAGE_ACCOUNTS_BLOB_MAX_LEN \
    (4U + (QOSA_MIFI_STORAGE_ACCOUNT_MAX_COUNT * QOSA_MIFI_STORAGE_ACCOUNT_MAX_LEN)) /*!< Maximum accounts table Blob payload length in bytes */
#define QOSA_MIFI_WEB_ACCOUNTS_BLOB_VERSION     1U   /*!< WebUI accounts table Blob structure version */
#define QOSA_MIFI_STORAGE_SIM_SLOT_MAX_COUNT    3U   /*!< Maximum SIM slot count */
#define QOSA_MIFI_STORAGE_SIM_INFO_BLOB_MAX_LEN  256U /*!< Maximum SIM info Blob payload length in bytes */

#define MIFI_KV_CHECK(expr)                                  \
    do                                                       \
    {                                                        \
        int __ret = (expr);                                  \
        if (__ret != QOSA_MIFI_STORAGE_OK)                   \
        {                                                    \
            QLOGE(0, "kv err:%d", __ret);                 \
        }                                                    \
    } while (0)

#define MIFI_KV_CHECK_EX(expr, is_ok)                        \
    do                                                       \
    {                                                        \
        if (is_ok)                                           \
        {                                                    \
            int __ret = (expr);                              \
            if (__ret != QOSA_MIFI_STORAGE_OK)               \
            {                                                \
                QLOGE(0, "kv err:%d", __ret);                \
                (is_ok) = QOSA_FALSE;                             \
            }                                                \
        }                                                    \
    } while (0)


/** Base error code value for MIFI storage module */
#define QOSA_ERRCODE_MIFI_STORAGE_BASE (QOSA_COMPONENT_MIFI << 16)

typedef qosa_uint16_t qosa_mifi_kv_key_t;  /*!< MIFI KV numeric key type */
typedef qosa_uint16_t qosa_mifi_blob_id_t; /*!< MIFI Blob numeric ID type */

/**
 * @brief Payload structure of the WebUI accounts table Blob.
 *
 * Each element of value holds a serialized account text:
 * index|username|password|permission|version|timestamp|is_invalid.
 * This struct defines the on-disk format protocol; when fields change,
 * the structure version must be bumped and old versions treated as invalid.
 */
typedef struct
{
    qosa_uint8_t version;  /*!< Structure version, currently QOSA_MIFI_WEB_ACCOUNTS_BLOB_VERSION */
    qosa_uint8_t reserved[3]; /*!< Reserved bytes, zeroed on write */
    char value[QOSA_MIFI_STORAGE_ACCOUNT_MAX_COUNT][QOSA_MIFI_STORAGE_ACCOUNT_MAX_LEN]; /*!< Account slots */
} qosa_mifi_web_accounts_blob_t;

// clang-format off
/**
 * @enum qosa_mifi_storage_err_e
 * @brief MIFI storage operation return codes.
 */
typedef enum mifi_storage_err
{
    QOSA_MIFI_STORAGE_OK                   = QOSA_OK,

    QOSA_MIFI_STORAGE_ERR_PARAM            = 101 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< Invalid parameter */
    QOSA_MIFI_STORAGE_ERR_NOT_INIT         = 102 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< Storage module not initialized */
    QOSA_MIFI_STORAGE_ERR_KEY_NOT_FOUND    = 103 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< KV key not defined */
    QOSA_MIFI_STORAGE_ERR_ID               = 104 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< Blob ID not defined */
    QOSA_MIFI_STORAGE_ERR_LENGTH           = 105 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< KV value length invalid */
    QOSA_MIFI_STORAGE_ERR_BUFFER_TOO_SMALL = 106 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< Output buffer too small */
    QOSA_MIFI_STORAGE_ERR_BUSY             = 107 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< Storage mutex operation failed */
    QOSA_MIFI_STORAGE_ERR_NOT_FOUND        = 108 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< Persistent object not found */
    QOSA_MIFI_STORAGE_ERR_TOO_LARGE        = 109 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< Persistent object exceeds capacity */
    QOSA_MIFI_STORAGE_ERR_STORAGE          = 110 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< Underlying storage operation failed */
    QOSA_MIFI_STORAGE_ERR_FORMAT           = 111 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< Persistent object format invalid */
    QOSA_MIFI_STORAGE_ERR_CRC              = 112 | QOSA_ERRCODE_MIFI_STORAGE_BASE, /*!< Blob payload CRC check failed */
} qosa_mifi_storage_err_e;
// clang-format on

/**
 * @enum qosa_mifi_kv_key_id_e
 * @brief Permanent numeric identifiers for MIFI KV configuration items.
 *
 * Numeric keys are permanent protocol identifiers; once published, a value
 * must never be reused even if the corresponding field is deprecated.
 * Variable-length data is stored as raw bytes with an explicit length;
 * the storage layer does not append a trailing NUL.
 * Each key's comment specifies a fixed scalar type, or the maximum/fixed
 * length for variable-length / raw-bytes configuration items.
 */
typedef enum
{
    QOSA_MIFI_KV_KEY_INVALID = 0x0000, /*!< Invalid KV key */

    QOSA_MIFI_KV_WLAN_ENABLE        = 0x0101, /*!< WLAN enable; type: qosa_uint8_t */
    QOSA_MIFI_KV_WLAN_BAND          = 0x0102, /*!< WLAN band; type: qosa_uint8_t */
    QOSA_MIFI_KV_WLAN_SSID          = 0x0103, /*!< WLAN SSID; max length: 32 bytes */
    QOSA_MIFI_KV_WLAN_PASSWORD      = 0x0104, /*!< WLAN password; max length: 64 bytes */
    QOSA_MIFI_KV_WLAN_BROADCAST     = 0x0105, /*!< SSID broadcast switch; type: qosa_uint8_t */
    QOSA_MIFI_KV_WLAN_ISOLATION     = 0x0106, /*!< AP isolation switch; type: qosa_uint8_t */
    QOSA_MIFI_KV_WLAN_SECURITY      = 0x0107, /*!< WLAN security mode; type: qosa_uint8_t */
    QOSA_MIFI_KV_WLAN_MAX_CLIENTS   = 0x0108, /*!< Maximum station count; type: qosa_uint32_t */
    QOSA_MIFI_KV_WLAN_WIRELESS_MODE = 0x0109, /*!< Wireless mode; type: qosa_uint8_t */
    QOSA_MIFI_KV_WLAN_CHANNEL       = 0x010A, /*!< WLAN channel; type: qosa_uint32_t */
    QOSA_MIFI_KV_WLAN_WIDTH         = 0x010B, /*!< WLAN bandwidth; type: qosa_uint8_t */

    QOSA_MIFI_KV_LAN_GATEWAY    = 0x0301, /*!< LAN gateway; type: qosa_uint32_t (IPv4 network byte order) */
    QOSA_MIFI_KV_LAN_DNS_ENABLE = 0x0302, /*!< LAN DNS switch; type: qosa_uint8_t */
    QOSA_MIFI_KV_LAN_DNS1       = 0x0303, /*!< Primary DNS; type: qosa_uint32_t (IPv4 network byte order) */
    QOSA_MIFI_KV_LAN_DNS2       = 0x0304, /*!< Secondary DNS; type: qosa_uint32_t (IPv4 network byte order) */
    QOSA_MIFI_KV_LAN_SUBNET_MASK = 0x0305, /*!< LAN subnet mask; type: qosa_uint32_t (IPv4 network byte order) */

    QOSA_MIFI_KV_DHCP_ENABLE     = 0x0401, /*!< DHCP switch; type: qosa_uint8_t */
    QOSA_MIFI_KV_DHCP_START      = 0x0402, /*!< DHCP start address; type: qosa_uint32_t (IPv4 network byte order) */
    QOSA_MIFI_KV_DHCP_END        = 0x0403, /*!< DHCP end address; type: qosa_uint32_t (IPv4 network byte order) */
    QOSA_MIFI_KV_DHCP_LEASE_TIME = 0x0404, /*!< DHCP lease time; type: qosa_uint32_t */

    QOSA_MIFI_KV_WAN_ROAM         = 0x0501, /*!< WAN roaming switch; type: qosa_uint8_t */
    QOSA_MIFI_KV_WAN_NETWORK_MODE = 0x0502, /*!< WAN network mode; type: qosa_uint8_t */
    QOSA_MIFI_KV_WAN_APN_MODE     = 0x0503, /*!< APN mode; type: qosa_uint8_t */

    QOSA_MIFI_KV_SIM_DEFAULT_SLOT      = 0x0601, /*!< Default SIM slot; type: qosa_uint8_t */
    QOSA_MIFI_KV_SIM_LAST_SLOT         = 0x0602, /*!< Last used SIM slot; type: qosa_uint8_t */
    QOSA_MIFI_KV_SIM_FORBID_MAP        = 0x0603, /*!< SIM slot disabled bitmap; type: qosa_uint8_t */
    QOSA_MIFI_KV_SIM_POLL_ORDER        = 0x0604, /*!< SIM slot polling order; fixed length: 3 bytes */
    QOSA_MIFI_KV_SIM_POLL_ENABLE       = 0x0605, /*!< SIM slot polling switch; type: qosa_uint8_t */
    QOSA_MIFI_KV_SIM_SCAN_ENABLE       = 0x0606, /*!< SIM slot scanning switch; type: qosa_uint8_t */
    QOSA_MIFI_KV_SIM_POLL_ROUNDS       = 0x0607, /*!< Polling rounds; type: qosa_uint32_t */
    QOSA_MIFI_KV_SIM_POLL_INTERVAL_SEC = 0x0608, /*!< Polling interval; type: qosa_uint32_t */
    QOSA_MIFI_KV_SIM_POLL_PERIOD_SEC   = 0x0609, /*!< Polling period; type: qosa_uint32_t */

    QOSA_MIFI_KV_POWER_MODE         = 0x0701, /*!< Power mode; type: qosa_uint8_t */
    QOSA_MIFI_KV_POWER_SHUTDOWN_MIN = 0x0702, /*!< Shutdown timeout; type: qosa_uint16_t */
    QOSA_MIFI_KV_POWER_SLEEP_MIN    = 0x0703, /*!< Sleep timeout; type: qosa_uint16_t */

    QOSA_MIFI_KV_UI_LANGUAGE         = 0x0801, /*!< WebUI language; max length: 15 bytes */
    QOSA_MIFI_KV_UI_QUICK_SETUP_DONE = 0x0802, /*!< Quick setup status; type: qosa_uint8_t */

} qosa_mifi_kv_key_id_e;

/**
 * @struct qosa_mifi_kv_cache_t
 * @brief Customer-editable MIFI KV default cache.
 *
 * Platform implementations fill business defaults only.  The storage module
 * owns schema_version and overwrites it after this callback returns.
 * Member comments describe business values and storage capacities. The KV
 * layer checks the schema version and value lengths only; callers must meet
 * business ranges and platform capability limits.
 * Byte-array lengths exclude a string terminator; KV does not append a NUL.
 */
typedef struct
{
    qosa_uint16_t schema_version; /*!< Storage-owned schema version; must equal QOSA_MIFI_KV_SCHEMA_VERSION
                                   * (currently 1). */

    qosa_uint8_t wlan_enable; /*!< WLAN switch: 0 = disabled, 1 = enabled. */
    qosa_uint8_t wlan_band; /*!< Radio band: 1 = 2.4 GHz, 2 = 5 GHz; must be supported by the platform. */
    qosa_uint8_t wlan_ssid_len; /*!< Stored SSID length: 0..32 bytes; an active AP requires 1..32 bytes. */
    qosa_uint8_t wlan_ssid[QOSA_MIFI_STORAGE_SSID_MAX_LEN]; /*!< Raw SSID bytes; first wlan_ssid_len bytes are
                                                             * valid, without a trailing NUL. */
    qosa_uint8_t wlan_password_len; /*!< Stored password length: 0..64 bytes; usable length depends on security mode
                                     * and platform. */
    qosa_uint8_t wlan_password[QOSA_MIFI_STORAGE_PASSWORD_MAX_LEN]; /*!< Raw password bytes; first wlan_password_len
                                                                     * bytes are valid; 8360S secured APs require
                                                                     * 8..63 bytes. */
    qosa_uint8_t wlan_broadcast; /*!< SSID advertisement: 0 = hidden, 1 = broadcast. */
    qosa_uint8_t wlan_isolation; /*!< AP client isolation: 0 = disabled, 1 = enabled. */
    qosa_uint8_t wlan_security; /*!< qosa_wifi_protocol_e: 0 = open, 1 = WPA2, 2 = WPA3, 3 = WPA/WPA2 mixed, 4 =
                                 * WPA2/WPA3 mixed. */
    qosa_uint32_t wlan_max_clients; /*!< Maximum associated clients: 1..platform limit (8360S:
                                     * QOSA_WIFI_MAX_NUM_CLIENTS, default 16). */
    qosa_uint8_t wlan_wireless_mode; /*!< qosa_wifi_wireless_mode_e: 0 = b/g, 1 = n, 2 = ac, 3 = ax; platform/band
                                      * support applies. */
    qosa_uint32_t wlan_channel; /*!< Channel: 0 = automatic; nonzero values must be supported by the selected
                                 * band/platform (2.4 GHz: 1..14). */
    qosa_uint8_t wlan_width; /*!< Channel width: 1 = 20 MHz, 2 = 40 MHz; legacy b/g mode requires 20 MHz. */

    qosa_uint32_t lan_gateway; /*!< LAN gateway IPv4 in network byte order; valid host address in the LAN subnet,
                                * excluding network/broadcast addresses. */
    qosa_uint32_t lan_subnet_mask; /*!< LAN subnet mask in network byte order; current LAN API accepts only
                                    * 255.255.255.0 (/24). */
    qosa_uint8_t lan_dns_enable; /*!< Custom LAN DNS: 0 = use upstream/fallback DNS, 1 = use configured DNS
                                  * addresses. */
    qosa_uint32_t lan_dns1; /*!< Primary DNS IPv4 in network byte order; 0 = unset, otherwise a valid address
                             * excluding 255.255.255.255. */
    qosa_uint32_t lan_dns2; /*!< Secondary DNS IPv4 in network byte order; 0 = unset, otherwise a valid address
                             * excluding 255.255.255.255. */

    qosa_uint8_t dhcp_enable; /*!< DHCP pool editing permission: 0 = disabled, 1 = enabled (current LAN API
                               * semantics). */
    qosa_uint32_t dhcp_start; /*!< DHCP pool first IPv4 in network byte order; valid LAN host address, no greater
                               * than dhcp_end in host order. */
    qosa_uint32_t dhcp_end; /*!< DHCP pool last IPv4 in network byte order; valid LAN host address, no less than
                             * dhcp_start in host order. */
    qosa_uint32_t dhcp_lease_time; /*!< Lease duration: 1..4294967295 seconds before platform limits; 1805 accepts
                                    * 120..86400 seconds. */

    qosa_uint8_t wan_roam; /*!< Cellular roaming permission: 0 = disallowed, 1 = allowed. */
    qosa_uint8_t wan_network_mode; /*!< Current WebUI modes: 2 = 4G only, 3 = 5G only, 4 = automatic; platform RAT
                                    * support applies. */
    qosa_uint8_t wan_apn_mode; /*!< APN selection: 0 = manual, 1 = automatic. */

    qosa_uint8_t sim_default_slot; /*!< Fallback SIM slot after polling failure: 0..2, restricted to
                                    * platform-supported slots. */
    qosa_uint8_t sim_last_slot; /*!< Last active SIM slot restored at boot: 0..2, restricted to platform-supported
                                 * slots. */
    qosa_uint8_t sim_forbid_map; /*!< Disabled-slot bitmap: 0..0x07; bit n = 1 forbids slot n (n = 0..2), upper bits
                                  * must be zero. */
    qosa_uint8_t sim_poll_order[QOSA_MIFI_STORAGE_SIM_SLOT_MAX_COUNT]; /*!< Fixed 3-byte polling order; a
                                                                        * permutation of slot IDs 0, 1, 2;
                                                                        * unsupported slots are skipped. */
    qosa_uint8_t sim_poll_enable; /*!< Polling mode: 0 = off, 1 = allowed supported slots, 2 = also require cached
                                   * SIM presence; modes 1/2 honor sim_forbid_map. */
    qosa_uint8_t sim_scan_enable; /*!< Boot SIM-info scan: 0 = off, 1 = every boot, 2 = only when SIM-info cache is
                                   * absent. */
    qosa_uint32_t sim_poll_rounds; /*!< Maximum polling rounds: 0 = unlimited, 1..4294967295 = finite round count. */
    qosa_uint32_t sim_poll_interval_sec; /*!< Wait between rounds: stored range 0..4294967295 seconds, 0 = no delay;
                                          * keep <=4294967 for uint32 millisecond conversion. */
    qosa_uint32_t sim_poll_period_sec; /*!< Periodic connectivity check: stored range 0..4294967295 seconds, 0 =
                                        * disabled; keep <=4294967 for uint32 millisecond conversion. */

    qosa_uint8_t power_mode; /*!< Idle power-saving action: 0 = SUPER (power off), 1 = STANDARD (recoverable low
                              * power). */
    qosa_uint16_t power_shutdown_min; /*!< Continuous idle time before power-off in SUPER mode: 0..1440 minutes; 0
                                       * disables automatic power-off. */
    qosa_uint16_t power_sleep_min; /*!< Continuous idle time before low power in STANDARD mode: 0..1440 minutes; 0
                                    * disables automatic sleep. */

    qosa_uint8_t ui_language_len; /*!< Stored language-tag length: 0..15 bytes; WebUI setters accept 1..15 bytes. */
    qosa_uint8_t ui_language[QOSA_MIFI_STORAGE_LANGUAGE_MAX_LEN]; /*!< Raw language-tag bytes (e.g. "cn"); first
                                                                   * ui_language_len bytes are valid, without a
                                                                   * trailing NUL. */
    qosa_uint8_t ui_quick_setup_done; /*!< Quick-setup completion flag: 0 = incomplete, 1 = complete. */
} qosa_mifi_kv_cache_t;

/**
 * @brief Load customer-editable platform defaults into the MIFI KV cache.
 */
void qosa_mifi_kv_load_platform_defaults(qosa_mifi_kv_cache_t *cache);

/**
 * @enum qosa_mifi_blob_id_e
 * @brief Permanent numeric identifiers for MIFI Blob objects.
 */
typedef enum
{
    QOSA_MIFI_BLOB_ID_INVALID = 0,             /*!< Invalid Blob ID */
    QOSA_MIFI_BLOB_ID_LAN_STATIC_BINDINGS = 1, /*!< LAN static address binding table */
    QOSA_MIFI_BLOB_ID_WIFI_MAC_FILTER = 2,     /*!< WLAN MAC filter table */
    QOSA_MIFI_BLOB_ID_PORTAL_PAGE = 3,         /*!< Portal page content */
    QOSA_MIFI_BLOB_ID_CA_CERT = 4,             /*!< CA certificate content */
    QOSA_MIFI_BLOB_ID_CLIENT_CERT = 5,         /*!< Client certificate content */
    QOSA_MIFI_BLOB_ID_OPERATOR_CONFIG = 6,     /*!< Operator custom configuration */
    QOSA_MIFI_BLOB_ID_WAN_PDP_PROFILE = 7,     /*!< WAN PDP Profile */
    QOSA_MIFI_BLOB_ID_WEB_ACCOUNTS = 8,        /*!< WebUI accounts table */
    QOSA_MIFI_BLOB_ID_WIFI_DUAL_BAND_CONFIG = 9, /*!< WiFi dual-band configuration */
    QOSA_MIFI_BLOB_ID_SIM_INFO = 10,             /*!< SIM card information cache (ICCID/IMSI per slot) */
    QOSA_MIFI_BLOB_ID_MAX                      /*!< Blob ID max boundary */
} qosa_mifi_blob_id_e;

/**
 * @brief Initialize the MIFI KV and Blob storage modules.
 *
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_storage_init(void);

/**
 * @brief Restore KV defaults and delete all MIFI persistent objects.
 *
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_storage_reset(void);

/**
 * @brief Check whether the complete WLAN KV profile is available in persistent storage.
 *
 * A QOSA_FALSE result means the WLAN fields currently exposed by qosa_mifi_kv_get()
 * are storage defaults because the persistent file or one of its WLAN fields was absent.
 *
 * @return qosa_bool_t
 *          - QOSA_TRUE: all WLAN fields were loaded from or written to persistent storage.
 *          - QOSA_FALSE: WLAN fields are defaults or the KV module is unavailable.
 */
qosa_bool_t qosa_mifi_kv_wlan_is_persisted(void);

/**
 * @brief Read a config value from the KV RAM cache.
 *
 * When buf is QOSA_NULL and buf_size is 0, only the current value length is queried.
 *
 * @param[in] key
 *          - KV numeric key.
 * @param[out] buf
 *          - Output buffer; pass QOSA_NULL when querying length only.
 * @param[in] buf_size
 *          - Output buffer size in bytes.
 * @param[out] actual_len
 *          - Actual value length in bytes.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - QOSA_MIFI_STORAGE_ERR_BUFFER_TOO_SMALL: buffer too small.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_kv_get(qosa_mifi_kv_key_t key,
                                         void *buf,
                                         qosa_uint16_t buf_size,
                                         qosa_uint16_t *actual_len);

/**
 * @brief Update a config value in the KV RAM cache.
 *
 * This API does not write to storage. After a group of related config changes
 * is complete, call qosa_mifi_kv_commit() to persist the full cache. This
 * module only checks storage length and capacity; business-level validation
 * rules are the caller's responsibility.
 *
 * @param[in] key
 *          - KV numeric key.
 * @param[in] value
 *          - Input value buffer.
 * @param[in] value_len
 *          - Input value length in bytes.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_kv_set(qosa_mifi_kv_key_t key,
                                         const void *value,
                                         qosa_uint16_t value_len);

/**
 * @brief Read a fixed-length KV config of type qosa_uint8_t.
 *
 * @param[in] key
 *          - KV numeric key.
 * @param[out] value
 *          - Read 8-bit unsigned value.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - QOSA_MIFI_STORAGE_ERR_LENGTH: key is not a qosa_uint8_t fixed-length config.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_kv_get_u8(qosa_mifi_kv_key_t key,
                                             qosa_uint8_t *value);

/**
 * @brief Update a fixed-length KV config of type qosa_uint8_t.
 *
 * This API only modifies the RAM cache; call qosa_mifi_kv_commit() to persist.
 *
 * @param[in] key
 *          - KV numeric key.
 * @param[in] value
 *          - 8-bit unsigned value to write.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - QOSA_MIFI_STORAGE_ERR_LENGTH: key is not a qosa_uint8_t fixed-length config.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_kv_set_u8(qosa_mifi_kv_key_t key,
                                             qosa_uint8_t value);

/**
 * @brief Read a fixed-length KV config of type qosa_uint16_t.
 *
 * @param[in] key
 *          - KV numeric key.
 * @param[out] value
 *          - Read 16-bit unsigned value.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - QOSA_MIFI_STORAGE_ERR_LENGTH: key is not a qosa_uint16_t fixed-length config.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_kv_get_u16(qosa_mifi_kv_key_t key,
                                              qosa_uint16_t *value);

/**
 * @brief Update a fixed-length KV config of type qosa_uint16_t.
 *
 * This API only modifies the RAM cache; call qosa_mifi_kv_commit() to persist.
 *
 * @param[in] key
 *          - KV numeric key.
 * @param[in] value
 *          - 16-bit unsigned value to write.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - QOSA_MIFI_STORAGE_ERR_LENGTH: key is not a qosa_uint16_t fixed-length config.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_kv_set_u16(qosa_mifi_kv_key_t key,
                                              qosa_uint16_t value);

/**
 * @brief Read a fixed-length KV config of type qosa_uint32_t.
 *
 * @param[in] key
 *          - KV numeric key.
 * @param[out] value
 *          - Read 32-bit unsigned value.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - QOSA_MIFI_STORAGE_ERR_LENGTH: key is not a qosa_uint32_t fixed-length config.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_kv_get_u32(qosa_mifi_kv_key_t key,
                                              qosa_uint32_t *value);

/**
 * @brief Update a fixed-length KV config of type qosa_uint32_t.
 *
 * This API only modifies the RAM cache; call qosa_mifi_kv_commit() to persist.
 *
 * @param[in] key
 *          - KV numeric key.
 * @param[in] value
 *          - 32-bit unsigned value to write.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - QOSA_MIFI_STORAGE_ERR_LENGTH: key is not a qosa_uint32_t fixed-length config.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_kv_set_u32(qosa_mifi_kv_key_t key,
                                              qosa_uint32_t value);

/**
 * @brief Validate the KV storage layout and persist the full RAM cache as a JSON node.
 *
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_kv_commit(void);

/**
 * @brief Get the Blob payload length after integrity checks.
 *
 * @param[in] id
 *          - Blob numeric ID.
 * @param[out] payload_size
 *          - Verified payload length in bytes.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_blob_get_size(qosa_mifi_blob_id_t id,
                                                 qosa_uint32_t *payload_size);

/**
 * @brief Read and verify a complete Blob object.
 *
 * Partial Blob reads are not supported.
 *
 * @param[in] id
 *          - Blob numeric ID.
 * @param[out] buf
 *          - Payload output buffer.
 * @param[in] buf_size
 *          - Output buffer size in bytes.
 * @param[out] actual_size
 *          - Actual payload length in bytes.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - QOSA_MIFI_STORAGE_ERR_BUFFER_TOO_SMALL: buffer too small.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_blob_read(qosa_mifi_blob_id_t id,
                                             void *buf,
                                             qosa_uint32_t buf_size,
                                             qosa_uint32_t *actual_size);

/**
 * @brief Persist a complete Blob object.
 *
 * The payload is written to a temporary file first, then synced and verified,
 * and finally renamed to the canonical file name.
 *
 * @param[in] id
 *          - Blob numeric ID.
 * @param[in] data
 *          - Payload input buffer.
 * @param[in] data_size
 *          - Input payload length in bytes.
 * @return qosa_mifi_storage_err_e
 *          - QOSA_MIFI_STORAGE_OK: success.
 *          - Other values: failure.
 */
qosa_mifi_storage_err_e qosa_mifi_blob_write(qosa_mifi_blob_id_t id,
                                              const void *data,
                                              qosa_uint32_t data_size);

#ifdef __cplusplus
}
#endif

#endif /* __QOSA_MIFI_STORAGE_H__ */
