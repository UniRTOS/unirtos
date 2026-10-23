/*****************************************************************/ /**
* @file qosa_vsim_adapt.h
* @brief virtual SIM adapter layer header file
* @author joe.tu@quectel.com
* @date 2025-11-19
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-11-19 <td>1.0 <td>joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_VSIM_ADAPT_H__
#define __QOSA_VSIM_ADAPT_H__
#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
/**
 * @enum qosa_vsim_adapt_event_e
 * @brief Virtual SIM adapter event types
 */
typedef enum
{
    QOSA_VSIM_ADAPT_EVENT_QUERY_TYPE, /*!< Query SIM type event */
    QOSA_VSIM_ADAPT_EVENT_MAX
} qosa_vsim_adapt_event_e;

/**
 * @brief  Virtual SIM adapter type
 *
 * Defines the type of adapter for a given SIM slot.
 */
typedef enum
{
    QOSA_VSIM_ADAPT_TYPE_PHYSICAL, /* physical SIM card */
    QOSA_VSIM_ADAPT_TYPE_SOFT,     /* local soft SIM card */
    QOSA_VSIM_ADAPT_TYPE_MAX,
} qosa_vsim_adapt_type_e;

/**
 * @struct qosa_vsim_adapt_event_t
 * @brief Virtual SIM adapter event structure
 */
typedef struct
{
    qosa_vsim_adapt_event_e event; /*!< Event type */
    qosa_uint8_t            simid; /*!< SIM identifier */
    union
    {
        qosa_vsim_adapt_type_e type; /*!< Return type for QUERY_TYPE event */
    } data;
} qosa_vsim_adapt_event_t;

/**
 * @brief Virtual SIM adapter event callback function type
 * @param[in] event Pointer to the event structure
 */
typedef void (*qosa_vsim_adapt_event_cb)(qosa_vsim_adapt_event_t *event);

/**
 * @brief  APDU exchange callback for virtual SIM adaptation layer.
 *
 * This callback is used to handle an APDU exchange between the host and
 * the virtual SIM. The implementation should parse the input APDU request,
 * perform the necessary operation, and fill the APDU response buffer.
 *
 * @param[in] simid
 *          - SIM identifier (e.g., 0 for SIM1, 1 for SIM2).
 * @param[in] req
 *          - Pointer to the input APDU request buffer.
 * @param[in] req_len
 *          - Length of the APDU request buffer.
 * @param[out] rsp
 *          - Pointer to the output APDU response buffer. The buffer
 *            contains SW1SW2 at the end (e.g., "08 29 60 10 08 16
 *            96 19 55 90 00" where "90 00" at the tail is SW1SW2).
 * @param[out] rsp_len
 *          - Pointer to the length of the APDU response buffer. The
 *            implementation should set this to the actual length of
 *            the response data.
 * @return int
 *          - SW1SW2 status word (e.g., 0x9000 for success) on success
 *
 */
// clang-format off
typedef int (*qosa_vsim_adapt_apdu_cb)(
    qosa_uint8_t simid,
    qosa_uint8_t *req,
    qosa_uint16_t req_len,
    qosa_uint8_t *rsp,
    qosa_uint16_t *rsp_len);
// clang-format on

/**
 * @brief Initialize VSIM adapter layer with an event callback.
 *
 * During initialization, the adapter layer will query the SIM type for
 * each slot by triggering QOSA_VSIM_ADAPT_EVENT_QUERY_TYPE events.
 *
 * @param[in] event_cb
 *          - Event callback function implemented by the VSIM application or third-party adapter.
 * @return int
 *          - 0 on success, negative on failure
 */
int qosa_vsim_adapt_init(qosa_vsim_adapt_event_cb event_cb);

/**
 * @brief  Set the adapter type for a specific SIM slot (Adapter Layer API).
 *
 * This function configures the type of adapter for a given SIM slot.
 * It is part of the VSIM adapter layer and can be used by different
 * virtual SIM implementations or third-party adapters.
 *
 * @param[in] simid
 *          - SIM identifier (0 for SIM1, 1 for SIM2)
 * @param[in] type
 *          - Adapter type (physical or soft)
 *
 * @return int
 *          - 0 on success, negative value on failure
 */
int qosa_vsim_adapt_set_type(qosa_uint8_t simid, qosa_vsim_adapt_type_e type);

/**
 * @brief  Register APDU exchange callback for a specific SIM slot (Adapter Layer API).
 *
 * This callback will be invoked whenever an APDU exchange is required
 * for the given SIM slot. It is part of the adapter layer, allowing
 * different virtual SIM implementations or third-party adapters to
 * provide their own APDU handling logic.
 *
 * @param[in] cb
 *          - APDU exchange callback
 *
 * @return int
 *          - 0 on success, negative value on failure
 */
int qosa_vsim_adapt_register_apdu_cb(qosa_vsim_adapt_apdu_cb cb);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#endif /* __QOSA_VSIM_ADAPT_H__ */
