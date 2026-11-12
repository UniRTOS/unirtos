/*****************************************************************/ /**
* @file qcm_vnet.h
* @brief SPI net public service and application API definitions
* @author evin.zhu
* @date 2026-07-24
*
* @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
**********************************************************************/

#ifndef __QCM_VNET_H__
#define __QCM_VNET_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_queue_list.h"
#include "qcm_vnet_spi_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed protocol frame and payload layout limits. */
#define QCM_VNET_HEADER_SIZE          (8U)
#define QCM_VNET_TLV_HEADER_SIZE      (4U)
#define QCM_VNET_DEFAULT_MAX_LEN      (1600U)
#define QCM_VNET_DEFAULT_BOOT_CLK_HZ  (26000000U)
#define QCM_VNET_IF_STATUS_DOWN       (0U)
#define QCM_VNET_IF_STATUS_UP         (1U)
#define QCM_VNET_MIN_XFER_LEN         (512U)
#define QCM_VNET_MAX_XFER_LEN         (4096U)
#define QCM_VNET_MAC_ADDR_LEN         (6U)

/* Built-in string descriptor identifiers. */
#define QCM_VNET_STRING_ID_LANGUAGE   (0U)
#define QCM_VNET_STRING_ID_VENDOR     (1U)
#define QCM_VNET_STRING_ID_PRODUCT    (2U)
#define QCM_VNET_STRING_ID_NETDEV     (3U)
#define QCM_VNET_STRING_ID_SERIAL     (4U)
#define QCM_VNET_STRING_ID_HW_VERSION (5U)
#define QCM_VNET_STRING_ID_SW_VERSION (6U)
#define QCM_VNET_STRING_ID_IMEI1      (7U)
#define QCM_VNET_STRING_ID_IMEI2      (8U)
#define QCM_VNET_MAX_STRING_DESC_LEN  (64U)
#define QCM_VNET_MAX_STRING_TEXT_LEN  ((QCM_VNET_MAX_STRING_DESC_LEN / QCM_VNET_U16_SIZE) + 1U)

/* Default descriptor and network capability values. */
#define QCM_VNET_DEFAULT_VID          (0x2C7CU)
#define QCM_VNET_DEFAULT_PID          (0x0901U)
#define QCM_VNET_DEFAULT_MTU          (1500U)
#define QCM_VNET_DEFAULT_SPEED_BPS    (100000000U)
#define QCM_VNET_DEFAULT_SERIAL_IF    (2U)

/* Interface map sizing, frame continuation flags, and control timing. */
#define QCM_VNET_INTERFACE_MAP_SIZE   (255U)
#define QCM_VNET_MF_SINGLE_FRAME      (0U)
#define QCM_VNET_MF_MORE_FRAME        (1U)
#define QCM_VNET_WAIT_READY_TIMEOUT_MS (100U)
#define QCM_VNET_ERRCODE_BASE         (QOSA_COMPONENT_USBNET << 16)

typedef enum
{
    QCM_VNET_OK = 0,
    QCM_VNET_ERR_GENERAL = 1 | QCM_VNET_ERRCODE_BASE,
    QCM_VNET_ERR_INVALID_PARAM,
    QCM_VNET_ERR_NO_MEMORY,
    QCM_VNET_ERR_BAD_FRAME,
    QCM_VNET_ERR_BAD_MAGIC,
    QCM_VNET_ERR_BAD_LENGTH,
    QCM_VNET_ERR_TIMEOUT,
    QCM_VNET_ERR_IO,
} qcm_vnet_err_e;

typedef enum
{
    QCM_VNET_INTERFACE_CONTROL = 0,
    QCM_VNET_INTERFACE_ETH = 1,
    QCM_VNET_INTERFACE_MAX = (QCM_VNET_INTERFACE_MAP_SIZE - 1),
} qcm_vnet_interface_e;

typedef enum
{
    QCM_VNET_TRANSPORT_SPI = 0,
} qcm_vnet_transport_type_e;

typedef struct
{
    qcm_vnet_transport_type_e type;
    union
    {
        qcm_vnet_spi_transport_cfg_t spi;
    } u;
} qcm_vnet_transport_cfg_t;

typedef enum
{
    QCM_VNET_CMD_GET_STATUS = 0,     /* get current interface state */
    QCM_VNET_CMD_SET_MAX_LEN = 1,    /* set fixed SPI transfer length */
    QCM_VNET_CMD_GET_MAX_LEN = 2,    /* get fixed SPI transfer length */
    QCM_VNET_CMD_SET_WAKEUP = 3,     /* set slave wakeup host enable state */
    QCM_VNET_CMD_GET_WAKEUP = 4,     /* get slave wakeup host enable state */
    QCM_VNET_CMD_IND_STATE = 5,      /* indicate interface state */
    QCM_VNET_CMD_GET_DESCRIPTOR = 6, /* get device/config/string descriptor */
    QCM_VNET_CMD_SET_INTERFACE = 7,  /* set interface state */
} qcm_vnet_cmd_e;

typedef enum
{
    QCM_VNET_DESC_TYPE_DEVICE = 1,
    QCM_VNET_DESC_TYPE_CONFIG = 2,
    QCM_VNET_DESC_TYPE_NETDEV = QCM_VNET_DESC_TYPE_CONFIG,
    QCM_VNET_DESC_TYPE_STRING = 3,
} qcm_vnet_descriptor_type_e;

typedef enum
{
    QCM_VNET_INTERFACE_CLASS_NETDEV = 0,
    QCM_VNET_INTERFACE_CLASS_SERIAL = 1,
    QCM_VNET_INTERFACE_CLASS_CONTROL = 255,
} qcm_vnet_interface_class_e;

/**
 * @brief VNET frame header carried ahead of each protocol payload.
 */
typedef struct
{
    /* Payload length in bytes. */
    qosa_uint16_t data_len;
    /* Frame sequence number. */
    qosa_uint16_t seq;
    /* More-frame flag for fragmented payloads. */
    qosa_uint8_t  mf;
    /* Logical interface identifier carried by the frame. */
    qosa_uint8_t  interface_id;
} qcm_vnet_frame_header_t;

/**
 * @brief Parsed VNET frame view containing header and payload pointer.
 */
typedef struct
{
    /* Parsed header fields for the current frame. */
    qcm_vnet_frame_header_t header;
    /* Payload bytes owned by the VNET service; do not free. */
    const qosa_uint8_t    *payload;
} qcm_vnet_frame_t;

/**
 * @brief User callback invoked when a frame is received on an interface.
 *
 * @param[in] arg
 *       - User context pointer registered with the callback.
 *       - The callback does not own this pointer and must not release it.
 * @param[in] frame
 *       - Parsed frame view owned by the VNET service for the duration of the callback.
 *       - The callback must not modify or release this pointer.
 *       - If the payload needs to be used after the callback returns, copy it first.
 */
typedef void (*qcm_vnet_rx_frame_cb_t)(void *arg, const qcm_vnet_frame_t *frame);

/**
 * @brief Runtime state reported for a single interface.
 */
typedef struct
{
    /* Interface whose state is being reported. */
    qosa_uint8_t interface_id;
    /* Current interface status, such as up or down. */
    qosa_uint8_t status;
} qcm_vnet_interface_state_t;

typedef enum
{
    QCM_VNET_IOCI_EVENT_OPEN = 0,
    QCM_VNET_IOCI_EVENT_CLOSE,
    QCM_VNET_IOCI_EVENT_CONFIG,
    QCM_VNET_IOCI_EVENT_STATE_IND,
} qcm_vnet_ioci_event_e;

/**
 * @brief User callback invoked when the service reports an IOCI event.
 *
 * @param[in] arg
 *       - User context pointer registered with the callback.
 *       - The callback does not own this pointer and must not release it.
 * @param[in] event
 *       - IOCI event type being reported.
 * @param[in] state
 *       - Optional interface state payload owned by the VNET service for the duration of the callback.
 *       - The callback must not modify or release this pointer.
 *       - If the state data needs to be retained after the callback returns, copy it first.
 */
typedef void (*qcm_vnet_ioci_cb_t)(void *arg, qcm_vnet_ioci_event_e event,
    const qcm_vnet_interface_state_t *state);

/**
 * @brief Descriptor entry describing one logical VNET interface.
 */
typedef struct
{
    /* Interface identifier exposed to the peer. */
    qosa_uint8_t  interface_id;
    /* Interface class used by the protocol dispatcher. */
    qosa_uint8_t  interface_class;
    /* String descriptor ID describing the interface name. */
    qosa_uint8_t  interface_string;
    /* Optional IOCI event callback for this interface. */
    qcm_vnet_ioci_cb_t ioci_cb;
    /* User context passed back to ioci_cb; do not free by service. */
    void                  *ioci_cb_arg;
    /* Optional RX callback for raw frames on this interface. */
    qcm_vnet_rx_frame_cb_t rx_frame_cb;
    /* User context passed back to rx_frame_cb; do not free by service. */
    void                      *rx_frame_cb_arg;
} qcm_vnet_interface_desc_t;

/**
 * @brief Top-level configuration descriptor for all exported interfaces.
 */
typedef struct
{
    /* Total encoded descriptor length in bytes. */
    qosa_uint16_t total_len;
    /* Number of valid entries in the interfaces table. */
    qosa_uint8_t  interface_count;
    /* Interface descriptor table indexed by interface ID. */
    qcm_vnet_interface_desc_t interfaces[QCM_VNET_INTERFACE_MAP_SIZE];
} qcm_vnet_config_desc_t;

/**
 * @brief Descriptor payload for the default Ethernet-like network interface.
 */
typedef struct
{
    /* Interface identifier of the network device endpoint. */
    qosa_uint8_t  interface_id;
    /* Interface class, normally netdev for Ethernet payloads. */
    qosa_uint8_t  interface_class;
    /* Maximum transmission unit in bytes. */
    qosa_uint16_t mtu;
    /* Reported link speed in bits per second. */
    qosa_uint32_t speed_bps;
    /* MAC address exposed by the virtual net device. */
    qosa_uint8_t  mac[QCM_VNET_MAC_ADDR_LEN];
    /* String descriptor ID describing the net device. */
    qosa_uint8_t  interface_string;
} qcm_vnet_netdev_desc_t;

/**
 * @brief Static string descriptor entry exposed by the VNET device.
 */
typedef struct
{
    /* String descriptor identifier. */
    qosa_uint8_t id;
    /* NUL-terminated string content; owned by caller configuration. */
    const char  *text;
} qcm_vnet_string_desc_t;

typedef struct qcm_vnet_s qcm_vnet_t;

/**
 * @brief Service startup configuration for a VNET instance.
 */
typedef struct
{
    /* Fixed SPI transfer length, including qcm_vnet_frame_header_t bytes. */
    qosa_uint16_t             negotiated_len;
    /* Selected transport backend and its transport-specific parameters. */
    qcm_vnet_transport_cfg_t  transport;
    /* Optional override for the exported interface configuration descriptor. */
    const qcm_vnet_config_desc_t *config_desc;
    /* Optional override for the default netdev descriptor. */
    const qcm_vnet_netdev_desc_t *netdev_desc;
    /* Optional string descriptor table provided by the upper layer. */
    const qcm_vnet_string_desc_t *string_descs;
    /* Number of valid entries in string_descs. */
    qosa_uint8_t              string_desc_count;
    /* Interface ID whose IOCI events should be reported through ioci_cb. */
    qosa_uint8_t              ioci_interface_id;
    /* Optional IOCI callback registered at service startup. */
    qcm_vnet_ioci_cb_t        ioci_cb;
    /* User context passed back to ioci_cb; not owned by the service. */
    void                     *ioci_cb_arg;
} qcm_vnet_cfg_t;

/**
 * @brief Opaque public handle for the VNET service lifecycle.
 */
typedef struct
{
    /* Opaque internal service context; owned by the VNET service. */
    void                      *priv_ctx;
} qcm_vnet_service_t;

/**
 * @brief Build a TLV field in the specified buffer.
 *
 * @param[out] buf
 *       - Output buffer used to store the TLV bytes.
 * @param[in] buf_size
 *       - Total size of the output buffer in bytes.
 * @param[in] type
 *       - TLV type field.
 * @param[in] value
 *       - TLV value payload.
 * @param[in] value_len
 *       - Length of the TLV value payload in bytes.
 * @param[out] tlv_len
 *       - Actual TLV length written to the output buffer.
 *
 * @return qcm_vnet_err_e
 *        - Returns QCM_VNET_OK when the TLV is built successfully.
 *        - Returns other values when parameters or buffer space are invalid.
 */
qcm_vnet_err_e qcm_vnet_build_tlv(qosa_uint8_t *buf, qosa_uint32_t buf_size, qosa_uint16_t type,
    const qosa_uint8_t *value, qosa_uint16_t value_len, qosa_uint16_t *tlv_len);

/**
 * @brief Register an IOCI event callback for the specified interface.
 *
 * @param[in] vnet
 *       - VNET instance handle.
 * @param[in] interface_id
 *       - Target interface identifier.
 * @param[in] ioci_cb
 *       - Callback invoked when IOCI events are reported.
 * @param[in] ioci_cb_arg
 *       - User argument passed back to the IOCI callback.
 *
 * @return qcm_vnet_err_e
 *        - Returns QCM_VNET_OK when the callback is registered successfully.
 *        - Returns other values when registration fails.
 */
qcm_vnet_err_e qcm_vnet_register_ioci_cb(qcm_vnet_t *vnet, qosa_uint8_t interface_id,
    qcm_vnet_ioci_cb_t ioci_cb, void *ioci_cb_arg);

/**
 * @brief Register a user receive callback for the specified interface.
 *
 * @param[in] vnet
 *       - VNET instance handle.
 * @param[in] interface_id
 *       - Target interface identifier.
 * @param[in] rx_frame_cb
 *       - Callback invoked when a frame is received on the interface.
 * @param[in] rx_frame_cb_arg
 *       - User argument passed back to the receive callback.
 *
 * @return qcm_vnet_err_e
 *        - Returns QCM_VNET_OK when the callback is registered successfully.
 *        - Returns other values when registration fails.
 */
qcm_vnet_err_e qcm_vnet_register_user_rx_cb(qcm_vnet_t *vnet, qosa_uint8_t interface_id,
    qcm_vnet_rx_frame_cb_t rx_frame_cb, void *rx_frame_cb_arg);

/**
 * @brief Send a VNET frame with the specified header and payload.
 *
 * @param[in] vnet
 *       - VNET instance handle.
 * @param[in] header
 *       - Frame header to be sent.
 * @param[in] tx_payload
 *       - Pointer to the frame payload buffer.
 * @param[in] tx_payload_len
 *       - Length of the frame payload in bytes.
 *
 * @return qcm_vnet_err_e
 *        - Returns QCM_VNET_OK when the frame is queued or sent successfully.
 *        - Returns other values when sending fails.
 */
qcm_vnet_err_e qcm_vnet_send_frame(qcm_vnet_t *vnet, const qcm_vnet_frame_header_t *header,
    const qosa_uint8_t *tx_payload, qosa_uint16_t tx_payload_len);

/**
 * @brief Send a control interface TLV command.
 *
 * @param[in] vnet
 *       - VNET instance handle.
 * @param[in] type
 *       - Control TLV type.
 * @param[in] value
 *       - Pointer to the TLV value payload.
 * @param[in] value_len
 *       - Length of the TLV value payload in bytes.
 *
 * @return qcm_vnet_err_e
 *        - Returns QCM_VNET_OK when the control TLV is sent successfully.
 *        - Returns other values when sending fails.
 */
qcm_vnet_err_e qcm_vnet_send_control_tlv(qcm_vnet_t *vnet, qosa_uint16_t type,
    const qosa_uint8_t *value, qosa_uint16_t value_len);

/**
 * @brief Initialize the VNET service with the specified configuration.
 *
 * @param[out] service
 *       - Service instance to initialize.
 * @param[in] cfg
 *       - Service configuration.
 *       - cfg->transport.type selects the transport backend used by the VNET service.
 *       - SPI transport parameters are provided through cfg->transport.u.spi.
 *
 * @return qcm_vnet_err_e
 *        - Returns QCM_VNET_OK when initialization completes successfully.
 *        - Returns other values when initialization fails.
 */
qcm_vnet_err_e qcm_vnet_service_init(qcm_vnet_service_t *service, const qcm_vnet_cfg_t *cfg);

/**
 * @brief Deinitialize the VNET service and release transport resources.
 *
 * @param[in,out] service
 *       - Service instance to deinitialize.
 *
 * @return qcm_vnet_err_e
 *        - Returns QCM_VNET_OK when deinitialization completes successfully.
 *        - Returns other values when deinitialization fails.
 */
qcm_vnet_err_e qcm_vnet_service_deinit(qcm_vnet_service_t *service);

/**
 * @brief Check whether the VNET service has been initialized.
 *
 * @param[in] service
 *       - Service instance to query.
 *
 * @return qosa_bool_t
 *        - Returns QOSA_TRUE when the service is initialized.
 *        - Returns QOSA_FALSE when the service is not initialized.
 */
qosa_bool_t qcm_vnet_service_is_inited(const qcm_vnet_service_t *service);

/**
 * @brief Get the VNET instance associated with the service.
 *
 * @param[in] service
 *       - Service instance to query.
 *
 * @return qcm_vnet_t *
 *        - Returns the bound VNET instance.
 *        - Returns QOSA_NULL when the service is invalid or not initialized.
 */
qcm_vnet_t *qcm_vnet_service_get_vnet(qcm_vnet_service_t *service);

/**
 * @brief Get the active configuration associated with the service.
 *
 * @param[in] service
 *       - Service instance to query.
 *
 * @return const qcm_vnet_cfg_t *
 *        - Returns the active service configuration.
 *        - Returns QOSA_NULL when the service is invalid or not initialized.
 */
const qcm_vnet_cfg_t *qcm_vnet_service_get_cfg(const qcm_vnet_service_t *service);

/**
 * @brief Register an IOCI event callback for the specified interface.
 *
 * @param[in] vnet
 *       - VNET instance handle.
 * @param[in] interface_id
 *       - Target interface identifier.
 * @param[in] ioci_cb
 *       - Callback invoked when IOCI events are reported.
 * @param[in] ioci_cb_arg
 *       - User argument passed back to the IOCI callback.
 *
 * @return qcm_vnet_err_e
 *        - Returns QCM_VNET_OK when the callback is registered successfully.
 *        - Returns other values when registration fails.
 */
qcm_vnet_err_e qcm_vnet_service_register_ioci_cb(qcm_vnet_t *vnet,
    qosa_uint8_t interface_id, qcm_vnet_ioci_cb_t ioci_cb, void *ioci_cb_arg);

/**
 * @brief Register a user receive callback for the specified interface.
 *
 * @param[in] vnet
 *       - VNET instance handle.
 * @param[in] interface_id
 *       - Target interface identifier.
 * @param[in] rx_frame_cb
 *       - Callback invoked when a frame is received on the interface.
 * @param[in] rx_frame_cb_arg
 *       - User argument passed back to the receive callback.
 *
 * @return qcm_vnet_err_e
 *        - Returns QCM_VNET_OK when the callback is registered successfully.
 *        - Returns other values when registration fails.
 */
qcm_vnet_err_e qcm_vnet_service_register_user_rx_cb(qcm_vnet_t *vnet,
    qosa_uint8_t interface_id, qcm_vnet_rx_frame_cb_t rx_frame_cb, void *rx_frame_cb_arg);

#ifdef __cplusplus
}
#endif

#endif /* __QCM_VNET_H__ */