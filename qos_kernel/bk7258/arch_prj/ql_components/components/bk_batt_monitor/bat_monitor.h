#ifndef _BAT_MONITOR_H_
#define _BAT_MONITOR_H_

#include "bk_gpio.h"
#include "bk_uart.h"

#define BAT_MONITOR_DEBUG

#ifdef BAT_MONITOR_DEBUG
#define BAT_MONITOR_PRT                 os_printf
#define BAT_MONITOR_WPRT                warning_prf
#else
#define BAT_MONITOR_PRT                 os_null_printf
#define BAT_MONITOR_WPRT                os_null_printf
#endif
#define LOW_VOLTAGE_BLINK_TIME          30000
/* Battery return values */
#define IOT_BATTERY_SUCCESS                   ( 0 )
#define IOT_BATTERY_INVALID_VALUE             ( 1 )
#define IOT_BATTERY_NOT_EXIST                 ( 2 )
#define IOT_BATTERY_READ_FAILED               ( 3 )
#define IOT_BATTERY_FUNCTION_NOT_SUPPORTED    ( 4 )

/* Battery Type */
typedef enum
{
    eBatteryChargeable,
    eBatteryNotChargeable
} IotBatteryType_t;

/* Charger type */
typedef enum
{
    eBatteryChargeNone,
    eBatteryChargeUSB,
    eBatteryChargePowerSupply,
    erBatteryChargeWireless
} IotBatteryChargeSource_t;

/* Battery status */
typedef enum
{
    eBatteryCharging,
    eBatteryDischarging,
    eBatteryNotCharging,
    eBatteryChargeFull,
    eBatteryChargeLow,
    eBatteryOverVoltage,
    eBatteryUnderVoltage,
    eBatteryOverTemp,
    eBatteryUnderTemp,
    eBatteryOverChargingTemp,
    eBatteryUnderhargingTemp,
    eBatteryChargeTimeExpired,
    eBatteryUnknown
} IotBatteryStatus_t;

/* Forward declaration of descriptor structure */
struct IotBatteryDescriptor;

/* Callback type */
typedef void ( * IotBatteryCallback_t )( IotBatteryStatus_t xStatus, void * pvUserContext );

/* Battery handle type */
typedef struct IotBatteryDescriptor * IotBatteryHandle_t;

/* Battery info structure */
typedef struct IotBatteryInfo
{
    IotBatteryType_t     xBatteryType;
    uint16_t             usMinVoltage;
    uint16_t             usMaxVoltage;
    int16_t              sMinTemperature;
    int32_t              lMaxTemperature;
    uint16_t             usMaxCapacity;
    uint8_t              ucAsyncSupported;
    IotBatteryStatus_t   xBatteryStatus;
} IotBatteryInfo_t;

/* Full descriptor structure */
typedef struct IotBatteryDescriptor
{
    bool                 bIsOpen;
    int32_t              lInstance;
    bool                 bBatteryPresent;

    IotBatteryInfo_t     xBatteryInfo;
    IotBatteryCallback_t xCallback;
    void               * pvUserContext;

    uint16_t             usCurrentVoltage;
    uint16_t             usCurrent;
    uint8_t              ucChargeLevel;
} IotBatteryDescriptor_t;

typedef enum{
	EVT_BATTERY_CHARGING = 0,
	EVT_BATTERY_LOW_VOLTAGE,
    EVT_SHUTDOWN_LOW_BATTERY,
}evt_battery;

typedef uint8_t (*battery_event_callback_t)(evt_battery event_param);
int battery_event_callback_register(battery_event_callback_t callback);

/* Public API function prototypes */
/**
 * @brief   Get the current battery voltage (millivolts).
 *
 * This function reads the global battery handle (xGlobalHandle) to retrieve
 * the current voltage (in millivolts). The user should ensure that the global
 * battery handle has been opened and initialized before calling this function.
 *
 * @param[out] pVoltage Pointer to a uint16_t variable in which to store
 *                      the battery voltage (mV).
 *
 * @return
 * - IOT_BATTERY_SUCCESS on success
 * - IOT_BATTERY_INVALID_VALUE if xGlobalHandle == NULL or pVoltage == NULL
 * - IOT_BATTERY_NOT_EXIST if no battery is present
 * - IOT_BATTERY_READ_FAILED on error obtaining valid reading
 * - IOT_BATTERY_FUNCTION_NOT_SUPPORTED if hardware does not support voltage reading
 */
int32_t battery_get_voltage(uint16_t *pVoltage);

/**
 * @brief   Get the current battery output current (milliamps).
 *
 * This function reads the global battery handle (xGlobalHandle) to retrieve
 * the current battery output current (in mA). The user should ensure that the
 * global battery handle has been opened and initialized before calling this function.
 *
 * @param[out] pCurrent Pointer to a uint16_t variable in which to store
 *                      the battery current (mA).
 *
 * @return
 * - IOT_BATTERY_SUCCESS on success
 * - IOT_BATTERY_INVALID_VALUE if xGlobalHandle == NULL or pCurrent == NULL
 * - IOT_BATTERY_NOT_EXIST if no battery is present
 * - IOT_BATTERY_READ_FAILED on error obtaining valid reading
 * - IOT_BATTERY_FUNCTION_NOT_SUPPORTED if hardware does not support current reading
 */
int32_t battery_get_current(uint16_t *pCurrent);

/**
 * @brief   Get the current battery charge level (percentage).
 *
 * This function reads the global battery handle (xGlobalHandle) to retrieve
 * the battery charge level, expressed in percentage (1 to 100).
 * The user should ensure that the global battery handle has been opened and
 * initialized before calling this function.
 *
 * @param[out] pLevel Pointer to a uint8_t variable in which to store
 *                    the battery percentage (1-100%).
 *
 * @return
 * - IOT_BATTERY_SUCCESS on success
 * - IOT_BATTERY_INVALID_VALUE if xGlobalHandle == NULL or pLevel == NULL
 * - IOT_BATTERY_NOT_EXIST if no battery is present
 * - IOT_BATTERY_READ_FAILED on error obtaining valid reading
 * - IOT_BATTERY_FUNCTION_NOT_SUPPORTED if hardware does not support reading charge level
 */
int32_t battery_get_charge_level(uint8_t *pLevel);

/**
 * @brief  Reads the battery charging-related GPIO pins and returns the current battery status.
 *
 * @note   This function internally checks the GPIO_CHARGE and GPIO_FULL pins to determine
 *         whether the battery is charging, full, or discharging.
 *
 * @return
 *   - eBatteryCharging    The battery is charging.
 *   - eBatteryChargeFull  The battery is fully charged.
 *   - eBatteryDischarging The battery is discharging/not charging.
 *   - eBatteryUnknown     If the global handle xGlobalHandle is uninitialized or the status cannot be determined.
 */
static inline IotBatteryStatus_t battery_get_status_from_gpio(void);

/**
 * @brief  Determines if the battery is currently in the "charging" state.
 *
 * @return
 *   - true  The battery is charging.
 *   - false The battery is not charging or the global handle is not initialized.
 */
bool battery_if_is_charging(void);

/**
 * @brief  Retrieves the pointer to the battery information structure.
 *
 * @return
 *   - A valid pointer to IotBatteryInfo_t if available.
 *   - NULL if the global handle xGlobalHandle is not available or uninitialized.
 */
IotBatteryInfo_t * battery_if_get_info(void);

/**
 * @brief Opens the Battery and Charging driver.
 *
 * This function initializes the battery driver for the given instance.
 * Usually, there is only one battery interface, so lBatteryInstance is often 0.
 *
 * @param[in] lBatteryInstance  The instance index of the battery driver to open.
 * @return
 *  - On success, returns a valid battery handle (IotBatteryHandle_t).
 *  - On failure (invalid instance or already opened), returns NULL.
 */
IotBatteryHandle_t iot_battery_open( int32_t lBatteryInstance );

/**
 * @brief Sets the callback function for battery notifications.
 *
 * The callback is only used if the battery driver supports async notifications.
 * Once set, any relevant battery events (e.g., threshold crossing) trigger the callback.
 *
 * @param[in] pxBatteryHandle   Handle to the battery driver (from iot_battery_open()).
 * @param[in] xCallback         Callback function to be registered.
 * @param[in] pvUserContext     User context pointer. Will be passed back to the callback.
 *
 * @note If pxBatteryHandle or xCallback is NULL, this function does nothing.
 */
void iot_battery_set_callback( IotBatteryHandle_t const pxBatteryHandle,
                               IotBatteryCallback_t xCallback,
                               void * pvUserContext );

/**
 * @brief Gets battery design and capability info.
 *
 * This function returns a pointer to the internal structure containing
 * information about battery design (voltage range, capacity, etc.).
 *
 * @param[in] pxBatteryHandle   Handle to the battery driver (from iot_battery_open()).
 *
 * @return
 *  - Pointer to an IotBatteryInfo_t structure on success.
 *  - NULL on error (e.g., invalid handle).
 */
IotBatteryInfo_t * iot_battery_getInfo( IotBatteryHandle_t const pxBatteryHandle );

/**
 * @brief Reads the current battery output current (in milliamps).
 *
 * @param[in]  pxBatteryHandle  Handle to the battery driver.
 * @param[out] pusCurrent       Pointer to a uint16_t variable where the current (mA) will be stored.
 *
 * @return
 *  - IOT_BATTERY_SUCCESS on success.
 *  - IOT_BATTERY_INVALID_VALUE if parameters are invalid.
 *  - IOT_BATTERY_NOT_EXIST if no battery is present.
 *  - IOT_BATTERY_READ_FAILED on hardware read error.
 *  - IOT_BATTERY_FUNCTION_NOT_SUPPORTED if current measurement is not supported.
 */
int32_t iot_battery_current( IotBatteryHandle_t const pxBatteryHandle,
                             uint16_t * pusCurrent );

/**
 * @brief Reads the battery voltage (in millivolts).
 *
 * @param[in]  pxBatteryHandle  Handle to the battery driver.
 * @param[out] pusVoltage       Pointer to a uint16_t variable where the voltage (mV) will be stored.
 *
 * @return
 *  - IOT_BATTERY_SUCCESS on success.
 *  - IOT_BATTERY_INVALID_VALUE if parameters are invalid.
 *  - IOT_BATTERY_NOT_EXIST if no battery is present.
 *  - IOT_BATTERY_READ_FAILED on hardware read error.
 *  - IOT_BATTERY_FUNCTION_NOT_SUPPORTED if voltage measurement is not supported.
 */
int32_t iot_battery_voltage( IotBatteryHandle_t const pxBatteryHandle,
                             uint16_t * pusVoltage );

/**
 * @brief Gets the battery charge level (percentage from 1 to 100).
 *
 * This function may read hardware or compute via voltage look-up to return the
 * battery's remaining capacity in percentage.
 *
 * @param[in]  pxBatteryHandle  Handle to the battery driver.
 * @param[out] pucChargeLevel   Pointer to a uint8_t variable where the remaining battery level will be stored.
 *
 * @return
 *  - IOT_BATTERY_SUCCESS on success.
 *  - IOT_BATTERY_INVALID_VALUE if parameters are invalid.
 *  - IOT_BATTERY_NOT_EXIST if no battery is present.
 *  - IOT_BATTERY_READ_FAILED on hardware read error.
 *  - IOT_BATTERY_FUNCTION_NOT_SUPPORTED if charge level measurement is not supported.
 */
int32_t iot_battery_chargeLevel( IotBatteryHandle_t const pxBatteryHandle,
                                 uint8_t * pucChargeLevel );

/**
 * @brief Initializes the battery monitoring task or logic.
 *
 * This function typically creates and starts a background task (if needed) to
 * periodically read or manage the battery and charging state.
 */
void battery_monitor_init( void );

/**
 * @brief Deinitializes the battery monitoring task or logic.
 *
 * This function stops and cleans up resources (e.g., tasks, buffers) used by
 * the battery monitor.
 */
void battery_monitor_deinit( void );

/* The system only supports one instance globally, but it is scalable. */
#define BATTERY_MAX_INSTANCE   1
extern IotBatteryDescriptor_t gxBatteryDescriptor[BATTERY_MAX_INSTANCE];



#endif //
