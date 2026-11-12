/*
 * Copyright (C) Quectel Wireless Solutions Co., Ltd.
 * All rights reserved.
 */

#ifndef __QCM_QTH_API_H__
#define __QCM_QTH_API_H__

#include "qosa_def.h"
#include "qcm_qth_conn_config.h"
#ifdef CONFIG_QCM_VTLS_FUNC
#include "qcm_vtls_cfg.h"
#endif /* CONFIG_QCM_VTLS_FUNC */

/** Error code base address for qcm_qth_conn component */
#define OSA_ERRCODE_QTH_CONN_BASE (QOSA_COMPONENT_QTH_CONN << 16)

/*============================================================================
 * Enum Type Definitions
 *============================================================================*/

/**
 * @brief Error codes returned by qcm_qth_conn interfaces
 */
typedef enum
{
    QCM_QTH_RES_OK              = QOSA_OK,                               /*!< Operation successful */
    QCM_QTH_RES_ERR_GENERAL     = 1 | OSA_ERRCODE_QTH_CONN_BASE,       /*!< General error */
    QCM_QTH_RES_ERR_NO_MEMORY,                                         /*!< Memory allocation failed */
    QCM_QTH_RES_ERR_PARAM_INVALID,                                     /*!< Invalid parameter */
    QCM_QTH_RES_ERR_PARAM_NULL,                                        /*!< Parameter is NULL */
    QCM_QTH_RES_ERR_STATE,                                             /*!< Current state does not allow this operation */
    QCM_QTH_RES_ERR_NOT_CONFIG,                                        /*!< Required configuration not completed */
    QCM_QTH_RES_ERR_DATACALL_FAILED,                                 /*!< Data call failed */
    QCM_QTH_RES_ERR_DATACALL_DEACT,                                   /*!< Data call deactivated */
    QCM_QTH_RES_ERR_BOOTSTRAP,                                         /*!< Domain bootstrap failed */
    QCM_QTH_RES_ERR_TLS_CFG,                                         /*!< TLS configuration error */
    QCM_QTH_RES_ERR_MQTT_FAIL,                                         /*!< OSA MQTT operation failed */
    QCM_QTH_RES_ERR_SIGN_FAIL,                                         /*!< Signature/password generation failed */
    QCM_QTH_RES_ERR_URL_PARSE,                                         /*!< URL parsing failed */
    QCM_QTH_RES_ERR_TCP_CONNECT,                                         /*!< TCP connection failed */
    QCM_QTH_RES_ERR_MQTT_CONNECT,                                         /*!< MQTT connect failed */
    QCM_QTH_RES_ERR_MQTT_CLOSE,                                         /*!< MQTT disconnect during MQTT operation */
} qcm_qth_conn_errcode_e;

/**
 * @enum qcm_qth_client_close_reason_e.
 * @brief MQTT client close reasons.
 */
typedef enum
{
    QCM_QTH_CLOSE_BY_ACIVE_CLOSE = 0,      /*!< Active close. */
    QCM_QTH_CLOSE_BY_PEER_RESET = 1,       /*!< Close due to peer reset. */
    QCM_QTH_CLOSE_BY_PING_TIMEOUT = 2,     /*!< Close due to ping timeout. */
    QCM_QTH_CLOSE_BY_CONN_TIMEOUT = 3,     /*!< Close due to connection timeout. */
    QCM_QTH_CLOSE_BY_CONNACK_FAIL = 4,     /*!< Close due to connack response failure. */
    QCM_QTH_CLOSE_BY_DISCONN_PEER_FIN = 5, /*!< Close due to peer DISCONNECT. */
    QCM_QTH_CLOSE_BY_PROTOCOL_FAIL = 8,    /*!< Close due to protocol error. */
    QCM_QTH_CLOSE_BY_DISCONNECT = 9,       /*!< Close due to disconnection. */
    QCM_QTH_CLOSE_BY_SSL_HS_FAILED = 10,   /*!< Close due to SSL handshake failure. */
    QCM_QTH_CLOSE_BY_SEND_FAILED = 11,     /*!< Close due to send data to MQTT server failure. */
    QCM_QTH_CLOSE_BY_PDP_DEACTED = 12,     /*!< Close due to PDP deactivation. */
    QCM_QTH_CLOSE_BY_OTHER_FAIL = 13,      /*!< Close due to MQTT internal error. */
} qcm_qth_client_close_reason_e;

/**
 * @brief qcm_qth event types
 */
typedef enum
{
    QCM_QTH_EVENT_AUTH = 1,            /*!< Device authentication/reset result */
    QCM_QTH_EVENT_CONN,                /*!< Device login result */
    QCM_QTH_EVENT_SUBSCRIBED,          /*!< Subscription result */
    QCM_QTH_EVENT_MSG_SEND,            /*!< Message sent successfully (MQTT ACK) */
    QCM_QTH_EVENT_MSG_RECEIVED,        /*!< Platform message received */
    QCM_QTH_EVENT_LOGOUT,              /*!< Device logout */
    QCM_QTH_EVENT_OTA,                  /*!< OTA Event */
    QCM_QTH_EVENT_SERVER,                /*!< Developer Center related event */
    QCM_QTH_EVENT_DATACOLL,
    QCM_QTH_EVENT_BOOTSTRAP,             /*!< Domain bootstrap event */
    QCM_QTH_EVENT_NETWORK_LTE,             /*!< LTE network event */
} qcm_qth_event_e;
/**
 * @brief qcm_qth connection working states
 */
typedef enum
{
    QCM_QTH_STATE_UNINITIALIZE = 0,        /* Uninitialized */
    QCM_QTH_STATE_INITIALIZED = 1,         /* Initialized */
    QCM_QTH_STATE_AUTHENTICATING = 2,      /* Authenticating */
    QCM_QTH_STATE_AUTHENTICATED = 3,       /* Authenticated */
    QCM_QTH_STATE_AUTHENTICATE_FAILED = 4, /* Authentication failed */
    QCM_QTH_STATE_CONNECTING = 5,          /* Registering */
    QCM_QTH_STATE_CONNECTED = 6,           /* Registered and is waiting for subscribing */
    QCM_QTH_STATE_CONNECT_FAIL = 7,        /* Registration failed */
    QCM_QTH_STATE_SUBSCRIBED = 8,          /* Subscribed and data can be sent */
    QCM_QTH_STATE_SUBSCRIBE_FAIL = 9,      /* Subscription failed */
    QCM_QTH_STATE_DISCONNECTING = 10,      /* De-registering */
    QCM_QTH_STATE_DISCONNECTED = 11,       /* De-registered */
    QCM_QTH_STATE_DISCONNECT_FAIL = 12,    /* De-registration failed */
    QCM_QTH_STATE_BOOTSTAP = 13,           /* domain bootstrap */
} qcm_qth_state_e;

/**
 * @brief qcm authentication type enumeration
 */
typedef enum
{
    QCM_SECTYPE_OTOS = 0,       /*!< One-type-one-secret */
    QCM_SECTYPE_ODOS = 1,       /*!< One-device-one-secret */
} qcm_secret_type_e;

typedef enum
{
    QCM_AUTHTYPE_ODOC = 0,     /* One-device-one-cert */
    QCM_AUTHTYPE_PSK = 1,      /* PSK                */
    QCM_AUTHTYPE_SSL = 2,      /* Server-wide cert    */
    QCM_AUTHTYPE_TCP = 3,      /* Raw TCP            */
    QCM_AUTHTYPE_SSL_SIDE = 4, /* One-way auth        */
} qcm_auth_type_e;
#ifdef CONFIG_QCM_VTLS_FUNC
/**
 * @brief Certificate type for ODOC device-specific X.509 credential operations
 */
typedef enum
{
    QCM_CERT_TYPE_CLIENT = 0, /*!< Device client certificate */
    QCM_CERT_TYPE_PRIVATE_KEY /*!< Device client private key */
} qcm_cert_type_e;
#endif /* CONFIG_QCM_VTLS_FUNC */
/**
 * @brief qcm connection mode enumeration
 */
typedef enum
{
    QCM_CONN_MODE_NONE = -1,
    QCM_CONN_MODE_AUTH,             /*!< Device authentication */
    QCM_CONN_MODE_LOGIN,             /*!< Device login */
    QCM_CONN_MODE_RESET,             /*!< Device reset */
} qcm_conn_mode_e;

/**
 * @brief qcm_qth configuration item enumeration
 * @note Enum values are consistent with the field declaration order in qcm_qth_conn_config_t
 */
typedef enum
{
    QCM_QTH_CFG_PRODUCT_KEY = 1,       /*!< Product PK, char[33] */
    QCM_QTH_CFG_PRODUCT_SECRET,        /*!< Product PS, char[33] */
    QCM_QTH_CFG_DEVICE_KEY,            /*!< Device DK, char[33] */
    QCM_QTH_CFG_DEVICE_SECRET,         /*!< Device DS, char[33] */
    QCM_QTH_CFG_SERVER_URL,            /*!< Server URL, char[256] */
    QCM_QTH_CFG_SERVER_IP,             /*!< Server IP, char[46] */
    QCM_QTH_CFG_SERVER_PORT,           /*!< Server port, qosa_uint16_t */
    QCM_QTH_CFG_KEEPALIVE,             /*!< Keepalive interval (seconds), qosa_uint32_t */
    QCM_QTH_CFG_PDP_CID,               /*!< PDP context ID, qosa_uint8_t */
    QCM_QTH_CFG_SIM_ID,                /*!< SIM card ID, qosa_uint8_t */
    QCM_QTH_CFG_SESSION_ENABLE,        /*!< SessionKey encryption, qosa_bool_t */
    QCM_QTH_CFG_ENCRYPT_SENSITIVE,     /*!< Sensitive info encryption return, qosa_bool_t */
    QCM_QTH_CFG_ENCODING_TYPE,         /*!< Sign encoding method, qcm_qth_encoding_type_e */
    QCM_QTH_CFG_SOFTVERSION,           /*!< SDK version number, char* */
    QCM_QTH_CFG_MCU_VERSION,           /*!< MCU version number, char[65] */
    QCM_QTH_CFG_SECRET_TYPE,           /*!< Authentication type, qosa_uint8_t: 0=ODOS, 1=ODOD */
    QCM_QTH_CFG_RESET_FLAG,            /*!< Device reset flag, qosa_bool_t */
    QCM_QTH_CFG_AUTH_TYPE,             /*!< TLS auth type, qcm_auth_type_e */
} qcm_qth_cfg_item_e;
/*============================================================================
 * Result Code Definitions
 *============================================================================*/
enum
{
    QCM_AUTH_SUCC = 10200,                      /* Successful device authentication */
    QCM_AUTH_ERR_UNKNOWN = 10300,               /* Other errors */
    QCM_AUTH_ERR_DMP_INSIDE = 10404,            /* Error occurs when Developer Center calls the API */
    QCM_AUTH_ERR_DMP_STATIC_DEV = 10421,        /* The device with static authentication as the authentication type cannot be authenticated. */
    QCM_AUTH_ERR_DONE = 10422,                  /* The device is authenticated (Connection failed) */
    QCM_AUTH_ERR_PKPS_INVALID = 10423,          /* Failed to query the product information (Connection failed) */
    QCM_AUTH_ERR_PAYLOAD_INVALID = 10424,       /* Failed to decode PAYLOAD (Connection failed) */
    QCM_AUTH_ERR_SIGN_INVALID = 10425,          /* Signature verification failed (Connection failed) */
    QCM_AUTH_ERR_HASH_INVALID = 10427,          /* The hash information is illegal (Connection failed) */
    QCM_AUTH_ERR_DK_ILLEGAL = 10431,            /* DK is illegal (Connection failed) */
    QCM_AUTH_ERR_FLAG = 10433,                  /* Flag is illegal. */
    QCM_AUTH_ERR_CLIENTID = 10434,              /* ClientID is not matched with password (password contains ClientID related information) */
    QCM_AUTH_ERR_SUBDEV_NO_ASSOCIATION = 10440, /* The gateway is not associated with the sub-device */
    QCM_AUTH_ERR_DEVICE_INSIDE = 10450,         /* Device internal error (Connection failed). */
    QCM_AUTH_ERR_FAIL = 10500,                  /* The device authentication to Developer Center failed (Unknown error in system) */
    QCM_AUTH_ERR_BOOTSTRAP_SERVER = 10479,      /* domain bootstrap failed */
};
enum
{
    QCM_CONN_SUCC = 10200,                 /* Register successfully */
    QCM_CONN_ERR_DMP_INSIDE = 10404,       /* Error occurs when Developer Center calls the API */
    QCM_CONN_ERR_DS_INVALID = 10430,       /* The DeviceSecret is wrong (Connection failed) */
    QCM_CONN_ERR_FLAG = 10433,             /* Flag is illegal. */
    QCM_CONN_ERR_CLIENTID = 10434,         /* ClientID is not matched with password (password contains ClientID related information) */
    QCM_CONN_ERR_DS = 10437,               /* Device DS error */
    QCM_CONN_SUBDEV_ERR_ALREADY = 10441,   /* Sub-device is connected (Connected successfully) */
    QCM_CONN_ERR_DEVICE_INSIDE = 10450,    /* Device internal error (Connection failed). */
    QCM_CONN_ERR_VERSION_NOTFOUND = 10471, /* The implementation version is not supported (Connection failed) */
    QCM_CONN_ERR_PING = 10473,             /* Access heartbeat error (Connection timeout) */
    QCM_CONN_ERR_NET = 10474,              /* Network error (Connection timeout) */
    QCM_CONN_ERR_SERVER_CHANGE = 10475,    /* Server changed. */
    QCM_CONN_ERR_AP = 10476,               /* AP connection error */
    QCM_CONN_ERR_DEVICE_FORBID = 10477,    /* No use*/
    QCM_CONN_ERR_BOOTSTRAP_SERVER = 10479, /* domain bootstrap failed */
    QCM_CONN_ERR_UNKNOW = 10500,           /* Registration failed (Unknown error in system) */
};
enum
{
    QCM_SUBCRIBE_SUCC = 10200, /* Successful subscription */
    QCM_SUBCRIBE_ERR = 10300,  /* Failed subscription. */
};
enum
{
    QCM_SEND_SUCC_TRANS = 10200,    /* Send transparent transmission data successfully */
    QCM_SEND_SUCC_PHYMODEL = 10210, /* Send TSL data successfully*/
    QCM_SEND_SUCC_LOC = 10220,      /* Send positioning data successfully */
    QCM_SEND_SUCC_STATE = 10230,    /* Send device status data successfully */
    QCM_SEND_SUCC_INFO = 10240,     /* Send device info data successfully */
    QCM_SEND_SUCC_NTP = 10250,      /* Send network time successfully */
    QCM_SEND_ERR_TRANS = 10300,     /* Failed to send transparent transmission data */
    QCM_SEND_ERR_PHYMODEL = 10310,  /* Failed to send TSL data */
    QCM_SEND_ERR_FAIL_LOC = 10320,  /* Failed to send positioning data */
    QCM_SEND_ERR_STATE = 10330,     /* Failed to send device status data */
    QCM_SEND_ERR_INFO = 10340,      /* Failed to send device info data */
    QCM_SEND_ERR_NTP = 10350,       /* Failed to send network time */
};
enum
{
    QCM_RECV_SUCC_TRANS = 10200,           /* Receive transparent transmission data */
    QCM_RECV_SUCC_PHYMODEL_RECV = 10210,   /* Receice TSL data */
    QCM_RECV_SUCC_PHYMODEL_REQ = 10211,    /* Receive TSL data request */
    QCM_RECV_SUCC_PHYMODEL_SERVER = 10212, /* Receives TSL service data issued by the platform */
    QCM_RECV_SUCC_LOC_REQ = 10220,         /* Receive device locator data request */
    QCM_RECV_SUCC_STATE_REQ = 10230,       /* Receive sub-device device status data request */
    QCM_RECV_SUCC_INFO_REQ = 10240,        /* Receive sub-device device info data request */
    QCM_RECV_ERR_LIMIT = 10428,            /* Traffic limiting caused by excessive device cache. */
    QCM_RECV_ERR_BUFFER = 10473,           /* Failed to receive data whose length exceeds the device cache limit */
};
enum
{
    QCM_LOGOUT_SUCC = 10200,         /* Successful de-registration (successful disconnection) */
    QCM_LOGOUT_SUCC_PASSIVE = 10201, /* Successful de-registration (successful disconnection from cloud) */
};
enum
{
    QCM_OTA_TASK_NOTIFY = 10700,      /* There is an upgrade task */
    QCM_OTA_START = 10701,            /* the device starts downloading firmware package. */
    QCM_OTA_DOWNLOADING = 10702,      /* Firmware is downloaded in progress. */
    QCM_OTA_DOWNLOADED = 10703,       /* the firmware package is downloaded. */
    QCM_OTA_UPDATING = 10704,         /* Firmware is being upgraded. */
    QCM_OTA_UPDATE_OK = 10705,        /* Firmware is upgraded successfully. */
    QCM_OTA_UPDATE_FAIL = 10706,      /* Failed to upgrade the firmware. */
    QCM_OTA_UPDATE_FLAG = 10707,      /* Advertisement of the first device operation result */
    QCM_OTA_WAIT_TIMEOUT = 10708,     /* wait next action timeout */
    QCM_OTA_SUBDEV_TARGETVER = 10711, /* the device firmware version */
};
enum
{
    QCM_SERVER_ERRCODE_RATE_LIMIT = 10428,     /* Traffic limiting caused by high-frequency messages on the device */
    QCM_SERVER_ERRCODE_QUANTITY_LIMIT = 10429, /* Traffic limiting caused by excessive number of single device activations or daily requests */
    QCM_SERVER_ERRCODE_SUBDEV_UNLOGIN = 10442, /* Device is not logged into Developer Center */
};

enum
{
    QCM_DATA_COLL_SEND_SUCCESS = 0, /* Data sent successfully . */
    QCM_DATA_COLL_SEND_FAIL = 1,    /* Data sent failed. */
    QCM_DATA_COLL_DELAYTIME = 2,    /* Data sent throttling (unit:second). */
    QCM_DATA_COLL_SEND_RETRY = 3,   /* Data sent failed. Please try again. */
    QCM_DATA_COLL_CONN_FAIL = 10,   /* Failed to connect to the data collection server. */
    QCM_DATA_COLL_TRANS_FAIL = 11,  /* The data collection channel is terminated abnormally. */
    QCM_DATA_COLL_DISENBLE = 12,    /* The data collection channel is not enabled.*/
    QCM_DATA_COLL_CONN_RETRY = 13,  /* Server connection failed. Please try again.*/
    QCM_DATA_COLL_CLOSE = 20,       /* The data collection channel has been closed. */
};

/*============================================================================
 * Device Information ID Enumeration
 *============================================================================*/
enum
{
    QCM_DPID_INFO_MODEL_TYPE = 1,      /*!< Module model type */
    QCM_DPID_INFO_MODEL_VER = 2,       /*!< Firmware version */
    QCM_DPID_INFO_MCU_VER = 3,         /*!< MCU version */
    QCM_DPID_INFO_CELLID = 4,          /*!< Cell ID */
    QCM_DPID_INFO_ICCID = 5,           /*!< ICCID */
    QCM_DPID_INFO_MCC = 6,             /*!< MCC */
    QCM_DPID_INFO_MNC = 7,             /*!< MNC */
    QCM_DPID_INFO_LAC = 8,             /*!< LAC */
    QCM_DPID_INFO_PHONE_NUM = 9,       /*!< Phone number */
    QCM_DPID_INFO_SIM_NUM = 10,        /*!< IMSI */
    QCM_DPID_INFO_SDK_VER = 11,        /*!< SDK version */
    QCM_DPID_INFO_LOC_SUPLIST = 12,    /*!< Supported location type list (not yet generating data) */
    QCM_DPIO_INFO_DP_VER = 13,         /*!< Data protocol version */
    QCM_DPIO_INFO_CP_VER = 14,         /*!< Communication protocol version */
    QCM_DPID_INFO_LOG_DURATION = 16,   /*!< Log reporting duration */
    QCM_DPID_INFO_LOG_LEVEL = 17,      /*!< Log reporting level */
    QCM_DPIO_INFO_MODULE_ID = 18,      /*!< Module ID */
    QCM_DPID_INFO_NET_CAPABILITY = 19, /*!< LAN communication capability (not yet generating data) */
    QCM_DPID_INFO_MAX,
};
/*============================================================================
 * Network Capability Type Enumeration
 *============================================================================*/

/**
 * @brief Network capability type identifiers
 */
enum
{
    QCM_NETCAP_ABILITY_WAN     = 0, /*!< LTE / WAN */
    QCM_NETCAP_ABILITY_WIFILAN = 1, /*!< WiFi */
    QCM_NETCAP_ABILITY_BLE     = 2, /*!< BLE */
};

/*============================================================================
 * Device Status ID Enumeration
 *============================================================================*/
enum
{
    QCM_DPID_STATUS_BATTERY = 1,   /*!< Battery level (percentage) */
    QCM_DPID_STATUS_VOLTAGE = 2,   /*!< Voltage (V) */
    QCM_DPID_STATUS_SIGNAL = 3,    /*!< RSSI */
    QCM_DPID_STATUS_FLASHFREE = 4, /*!< Available flash space (bytes) */
    QCM_DPID_STATUS_RSRP = 5,      /*!< RSRP */
    QCM_DPID_STATUS_RSRQ = 6,      /*!< LTE RSRQ */
    QCM_DPID_STATUS_SNR = 7,       /*!< SNR */
    QCM_DPID_STATUS_MAX,
};

enum
{
    QCM_QTH_TTLVID_LOC_TYPE = 1,          // Location identifier
    QCM_QTH_TTLVID_UTC_DATE = 2,          // UTC date
    QCM_QTH_TTLVID_UTC_TIME = 3,          // UTC time
    QCM_QTH_TTLVID_LONGITUDE = 4,         // Longitude
    QCM_QTH_TTLVID_LATITUDE = 5,          // Latitude
    QCM_QTH_TTLVID_ALTITUDE = 6,          // Altitude
    QCM_QTH_TTLVID_ELEVATION = 7,         // Height of the earth ellipsoid relative to the geoid
    QCM_QTH_TTLVID_SAT_NUM = 8,           // Number of satellites
    QCM_QTH_TTLVID_PDOP = 9,              // PDOP position dilution of precision
    QCM_QTH_TTLVID_HDOP = 10,             // HDOP horizontal dilution of precision
    QCM_QTH_TTLVID_VDOP = 11,             // VDOP vertical dilution of precision
    QCM_QTH_TTLVID_MAG_DECLINATION = 12,  // Magnetic declination
    QCM_QTH_TTLVID_GROUND_SPEED = 13,     // Ground speed
    QCM_QTH_TTLVID_GROUND_HEADING = 14,   // Ground heading angle
    QCM_QTH_TTLVID_MODE_INDICATOR = 15,   // Mode indicator
};

/*============================================================================
 * Callback Function Type Declarations
 *============================================================================*/

/**
 * @brief qcm_qth event callback function type
 * @param[in] event     Event type
 * @param[in] errcode   Error code, 0 indicates success
 * @param[in] data      Event-related data (e.g., message data), can be NULL
 * @param[in] data_len  When receiving cloud data, it indicates the number of object model messages received this time.
 * @param[in] user_data User context pointer passed when registering callback
 */
typedef void (*qcm_qth_event_cb_t)(qcm_qth_event_e event, int errcode, const void *data, qosa_uint32_t data_len, void *user_data);

/**
 * @brief qcm_qth connection configuration
 */
typedef struct
{
    char product_key[QCM_QTH_PK_MAX_SIZE + 1];     /*!< Product PK */
    char product_secret[QCM_QTH_PS_MAX_SIZE + 1];  /*!< Product PS */
    char device_key[QCM_QTH_DK_MAX_SIZE + 1];      /*!< Device DK */
    char device_secret[QCM_QTH_DS_MAX_SIZE + 1];   /*!< Device DS, empty string for first-time authentication */
    char server_url[QCM_QTH_SERVER_URL_MAX];       /*!< Server address (domain:port) */
    char server_ip[QCM_QTH_IP_MAX_SIZE];           /*!< Server IP address */
    qosa_uint16_t server_port;                       /*!< Server port */
    qosa_uint32_t keepalive;                         /*!< Keepalive interval in seconds */
    qosa_uint8_t pdp_cid;                            /*!< PDP context ID */
    qosa_uint8_t sim_id;                             /*!< SIM card ID */
    qosa_bool_t session_enable;                      /*!< Enable SessionKey encryption */
    qosa_bool_t encrypt_sensitive;                   /*!< Enable sensitive info encryption */
    char *softversion;                                  /*!< Module firmware version */
    char mcu_version[QCM_QTH_MCU_VERSION_MAX_SIZE + 1];  /*!< MCU version number */
} qcm_qth_conn_config_t;

/**
 * @brief Set default values for qcm_qth_conn_config_t
 * @param[in] config Pointer to configuration struct, must not be NULL
 * @return 0 on success, -1 if config is NULL
 */
int qcm_qth_conf_default(qcm_qth_conn_config_t *config);

/**
 * @brief Set application version appended to firmware version as softversion
 * @param[in] app_ver Application version string to append, NULL to use firmware version only
 * @return QCM_QTH_RES_OK on success, see qcm_qth_conn_errcode_e for others
 */
qcm_qth_conn_errcode_e qcm_qth_set_appid(const char *app_ver);

/**
 * @brief Thing model response instance
 */
typedef struct
{
    qosa_uint16_t pkg_id;
    void        *ttlv_head;
} qcm_tsl_t;

/*============================================================================
 * Interface Declarations - Initialization and Configuration
 *============================================================================*/

/**
 * @brief Initialize qcm_qth connection module, reset internal state to defaults
 */
void qcm_qth_init(void);

/**
 * @brief Register event callback function
 * @param[in] cb        Event callback function pointer
 * @param[in] user_data User context pointer, returned as-is in callback
 */
void qcm_qth_set_event_cb(qcm_qth_event_cb_t cb, void *user_data);

/**
 * @brief Set QTH connection configuration
 * @param[in] config Pointer to configuration struct, must not be NULL
 * @return QCM_QTH_RES_OK on success, see qcm_qth_conn_errcode_e for others
 * @details This function provides a configuration method similar to qosa_mqtt_config_t.
 *          Define a qcm_qth_conn_config_t struct, fill in all required fields, then pass it in.
 */
qcm_qth_conn_errcode_e qcm_qth_set_config(const qcm_qth_conn_config_t *config);


/*============================================================================
 * Interface Declarations - Connection Management
 *============================================================================*/

/**
 * @brief Initiate QTH platform MQTT connection (core interface)
 * @details Internally selects authentication or login mode based on whether DS is empty.
 *          Connection is asynchronous; completion is notified via registered callback.
 * @return QCM_QTH_RES_OK if connection initiated, see qcm_qth_conn_errcode_e for others
 */
qcm_qth_conn_errcode_e qcm_qth_mqtt_connect(void);

/**
 * @brief Disconnect from QTH platform MQTT
 * @return QCM_QTH_RES_OK on success, see qcm_qth_conn_errcode_e for others
 */
qcm_qth_conn_errcode_e qcm_qth_disconnect(void);

/**
 * @brief Get current connection state
 * @return Current state enum value
 */
qcm_qth_state_e qcm_qth_get_state(void);

/**
 * @brief Get current ClientId
 * @param[out] buf     Output buffer
 * @param[in]  buf_len Output buffer length
 * @return Actual length of ClientId (excluding '\0'), -1 on failure
 */
int qcm_qth_get_client_id(char *buf, qosa_uint32_t buf_len);

/**
 * @brief Read single configuration item
 * @param[in]  item       Configuration item type
 * @param[out] value      Output buffer (allocated by caller)
 * @param[in]  value_len  Output buffer size (bytes)
 * @return QCM_QTH_RES_OK on success, see qcm_qth_conn_errcode_e for others
 */
qcm_qth_conn_errcode_e qcm_qth_get_cfg(qcm_qth_cfg_item_e item, void *value, qosa_uint32_t value_len);
/**
 * @brief Set single configuration item
 * @param[in] item       Configuration item type
 * @param[in] value      Pointer to new value (string ends with '\0')
 * @param[in] value_len  Length of value (string includes '\0', numeric is sizeof(type))
 * @return QCM_QTH_RES_OK on success, see qcm_qth_conn_errcode_e for others
 */
qcm_qth_conn_errcode_e qcm_qth_set_cfg(qcm_qth_cfg_item_e item, const void *value, qosa_uint32_t value_len);
#if (QCM_ENABLE_OTA == 1)
qosa_bool_t qcm_qth_ota_request(qosa_uint32_t mode);
qosa_bool_t qcm_qth_ota_action(qosa_uint8_t action);
qosa_uint32_t qcm_qth_ota_mcu_fw_data_read(qosa_uint32_t start_addr, qosa_uint8_t data[], qosa_uint32_t max_len);
void          qcm_qth_ota_mcu_version_modify(const char *component_no, const char *version);
#endif /* QCM_ENABLE_OTA */
/**
 * @brief Clear all device configuration, restore to factory defaults
 * @details Clears user-configurable data such as PK/PS/DK/DS/Server URL/Session info,
 *          resets to default values. Module remains initialized and can be reconfigured.
 *          Only allowed when not in an active connection state (INITIALIZED/DISCONNECTED, etc.).
 * @return QCM_QTH_RES_OK on success, see qcm_qth_conn_errcode_e for others
 */
qcm_qth_conn_errcode_e qcm_qth_clear(void);
/**
 * @brief Connection reset (reserved interface, currently empty implementation)
 * @note Bootstrap and reconnection logic to be implemented later
 */
void qcm_conn_reset(void);

/*============================================================================
 * Interface Declarations - QCM Command Bus (refactored from ql_iotCmdBus)
 *============================================================================*/

#if (1 == QCM_ENABLE_PASSTRANS)
/**
 * @brief Send transparent data to Developer Center
 * @param[in] mode    QoS level: 0/1/2
 * @param[in] payload Data to send
 * @param[in] len     Data length in bytes
 * @return OSA_TRUE on success, OSA_FALSE on failure
 */
qosa_bool_t qcm_cmd_bus_pass_trans_send(qosa_uint16_t mode, const qosa_uint8_t *payload, qosa_uint32_t len);

/**
 * @brief Send transparent data (returns package ID)
 * @param[in] mode    QoS level: 0/1/2
 * @param[in] payload Data to send
 * @param[in] len     Data length in bytes
 * @return -1 on failure, 1~65535 uplink package ID
 */
qosa_int32_t qcm_cmd_bus_pass_trans_send_ex(qosa_uint16_t mode, const qosa_uint8_t *payload, qosa_uint32_t len);
#endif /* QCM_ENABLE_PASSTRANS */

#if (1 == QCM_ENABLE_TSL)
/**
 * @brief Report TSL data to Developer Center
 * @param[in] mode      QoS level: 0/1/2
 * @param[in] ttlv_head TTLV data head
 * @return OSA_TRUE on success, OSA_FALSE on failure
 */
qosa_bool_t qcm_cmd_bus_phymodel_report(qosa_uint16_t mode, const void *ttlv_head);

/**
 * @brief Report TSL data (returns package ID)
 * @param[in] mode      QoS level: 0/1/2
 * @param[in] ttlv_head TTLV data head
 * @return -1 on failure, 1~65535 uplink package ID
 */
qosa_int32_t qcm_cmd_bus_phymodel_report_ex(qosa_uint16_t mode, const void *ttlv_head);

/**
 * @brief Respond to TSL data
 * @param[in] mode      QoS level: 0/1/2
 * @param[in] pkg_id    Downlink package ID
 * @param[in] ttlv_head Response TTLV data head
 * @return OSA_TRUE on success, OSA_FALSE on failure
 */
qosa_bool_t qcm_cmd_bus_phymodel_ack(qosa_uint16_t mode, qosa_uint16_t pkg_id, const void *ttlv_head);
#endif /* QCM_ENABLE_TSL */

/*============================================================================

 * Interface Declarations - QCM Command System (refactored from ql_iotCmdSys)
 *============================================================================*/

/**
 * @brief Report device status to Developer Center
 * @param[in] ids   Device status ID array, NULL for all
 * @param[in] size  Number of status IDs, 0 for all
 * @return OSA_TRUE on success, OSA_FALSE on failure
 */
qosa_bool_t qcm_cmd_sys_status_report(const qosa_uint16_t ids[], qosa_uint32_t size);

/**
 * @brief Report device info to Developer Center
 * @param[in] ids   Device info ID array, NULL for all
 * @param[in] size  Number of info IDs, 0 for all
 * @return OSA_TRUE on success, OSA_FALSE on failure
 */
qosa_bool_t qcm_cmd_sys_dev_info_report(const qosa_uint16_t ids[], qosa_uint32_t size);

/**
 * @brief Get device status TTLV data
 * @param[in] ids   Device status ID array, NULL for all
 * @param[in] size  Number of status IDs, 0 for all
 * @return TTLV data list head, caller must free via @ref qcm_ttlv_free()
 */
void *qcm_cmd_sys_get_dev_status(const qosa_uint16_t ids[], qosa_uint32_t size);

/**
 * @brief Get device info TTLV data
 * @param[in] ids   Device info ID array, NULL for all
 * @param[in] size  Number of info IDs, 0 for all
 * @return TTLV data list head, caller must free via @ref qcm_ttlv_free()
 */
void *qcm_cmd_sys_get_dev_info(const qosa_uint16_t ids[], qosa_uint32_t size);
/*============================================================================
 * Interface Declarations - Location Commands (refactored from ql_iotCmdLoc)
 *============================================================================*/
#if (1 == QCM_ENABLE_LOCIN)
/**
 * @brief Get built-in location data by locator type
 * @param[in] title_ttlv TTLV head containing locator type strings
 * @return TTLV data with location NMEA sentences, caller must free with qcm_ttlv_free()
 *         NULL on failure
 */
void *qcm_cmd_loc_get_data(const void *title_ttlv);
/**
 * @brief Get supported built-in locator type list
 * @note This API may probe runtime capabilities such as serving-cell query or
 *       WiFi scan. Use it for capability reporting only, not as a pure
 *       validator for request title TTLV.
 * @return TTLV data with locator type strings, caller must free with qcm_ttlv_free()
 */
void *qcm_cmd_loc_get_sup_list(void);
/**
 * @brief Report built-in location data to cloud
 * @param[in] title_ttlv TTLV head containing locator types to report
 * @return QOSA_TRUE on success, QOSA_FALSE on failure
 */
qosa_bool_t qcm_cmd_loc_report_inside(const void *title_ttlv);
/**
 * @brief Report built-in location data (returns package ID)
 * @param[in] title_ttlv TTLV head containing locator types to report
 * @return -1 on failure, 1~65535 uplink package ID
 */
qosa_int32_t qcm_cmd_loc_report_inside_ex(const void *title_ttlv);
#endif /* QCM_ENABLE_LOCIN */
/**
 * @brief Report external GNSS location data to cloud
 * @param[in] nmea_ttlv TTLV head containing NMEA sentence strings
 * @return QOSA_TRUE on success, QOSA_FALSE on failure
 */
qosa_bool_t qcm_cmd_loc_report_outside(const void *nmea_ttlv);
/**
 * @brief Report external GNSS location data (returns package ID)
 * @param[in] nmea_ttlv TTLV head containing NMEA sentence strings
 * @return -1 on failure, 1~65535 uplink package ID
 */
qosa_int32_t qcm_cmd_loc_report_outside_ex(const void *nmea_ttlv);
/*============================================================================
 * Interface Declarations - Device Binding (refactored from ql_iotDevBind)
 *============================================================================*/

#if (1 == QCM_ENABLE_BINDCODE)
/**
 * @brief Set device binding code (provisioning code)
 * @param[in] bindcode Binding code string, length must be QCM_BINDCODE_MAXSIZE
 * @param[in] timeout  Binding code validity period in seconds, 0 for default
 * @return OSA_TRUE on success, OSA_FALSE on failure
 */
qosa_bool_t qcm_dev_bindcode_set(const char *bindcode, qosa_uint32_t timeout);

/**
 * @brief Get current device binding code
 * @param[out] bindcode Output parameter, receives binding code string pointer
 * @return Remaining validity seconds, 0 if expired, -1 on failure
 */
qosa_uint32_t qcm_dev_bindcode_get(char **bindcode);

/**
 * @brief Get current device binding secret (BindSecret)
 * @return Binding secret string pointer, NULL on failure
 */
char *qcm_dev_bindscrt_get(void);
#endif /* QCM_ENABLE_BINDCODE */

#ifdef CONFIG_QCM_VTLS_FUNC
/**
 * @brief Write ODOC device-specific X.509 certificate or private key to file system.
 *
 * Can only be called when the device is not actively connecting or connected.
 * Refuses to overwrite an already-written credential (one-time burn semantics).
 * On success updates auth_type config to QCM_AUTHTYPE_ODOC.
 *
 * @param[in] type  QCM_CERT_TYPE_CLIENT or QCM_CERT_TYPE_PRIVATE_KEY
 * @param[in] data  PEM-encoded certificate or key data
 * @param[in] len   Length of data in bytes
 * @return QOSA_TRUE on success, QOSA_FALSE on failure or precondition violation
 */
qosa_bool_t qcm_auth_cert_x509_set(qcm_cert_type_e type, const char *data, qosa_uint32_t len);
/**
 * @brief Read ODOC device-specific X.509 certificate or private key from file system.
 *
 * @param[in]  type  QCM_CERT_TYPE_CLIENT or QCM_CERT_TYPE_PRIVATE_KEY
 * @param[out] data  Pointer receiving malloc'd buffer; caller must free with qcm_qth_free()
 * @return Number of bytes read; 0 if file not found or read error (data is NULL in that case)
 */
qosa_uint32_t qcm_auth_cert_x509_get(qcm_cert_type_e type, char **data);
#endif /* CONFIG_QCM_VTLS_FUNC */
#if (1 == QCM_ENABLE_BINDSECRET)
/**
 * @brief Set device binding secret (BindSecret)
 * @param[in] bindsecret Binding secret string, length must be QCM_BINDSCRT_MAXSIZE
 * @return OSA_TRUE on success, OSA_FALSE on failure
 */
qosa_bool_t qcm_dev_bindsecret_set(const char *bindsecret);
#endif /* QCM_ENABLE_BINDSECRET */

/*============================================================================
 * Interface Declarations - External GNSS Location Report
 *============================================================================*/
/**
 * @brief Report external GNSS location TTLV data to cloud
 * @param[in] loc  GNSS location TTLV data
 * @return 0 on success, negative on failure
 */
qosa_int32_t qcm_qth_loc_gnss_report(void *loc);

/*============================================================================
 * Network Capability APIs
 *============================================================================*/

#if (1 == QCM_ENABLE_NETCAP)
/**
 * @brief Set LAN communication capability
 * @param type Network type: QCM_NETCAP_ABILITY_WAN / WIFILAN / BLE
 * @param flag QOSA_TRUE to enable, QOSA_FALSE to disable
 * @return QOSA_TRUE on success, QOSA_FALSE on failure (invalid type or SFS write error)
 */
qosa_bool_t qcm_qth_net_cap_set(qosa_uint8_t type, qosa_bool_t flag);

/**
 * @brief Get current network capability bitmap
 * @return Bitmap of enabled network capabilities
 */
qosa_uint16_t qcm_qth_net_cap_get(void);
#endif /* QCM_ENABLE_NETCAP */

#endif /* __QCM_QTH_API_H__ */
