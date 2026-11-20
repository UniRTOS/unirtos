#include <common/bk_include.h>
#include <common/bk_typedef.h>
#include "bk_arm_arch.h"
#include "bk_gpio.h"
#include "multi_button.h"
#include <os/os.h>
#include <os/mem.h>
#include <common/bk_kernel_err.h>
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_driver.h"
#include "adc_hal.h"
#include "adc_statis.h"
#include "adc_driver.h"
#include <driver/adc.h>
#include "sys_driver.h"
#include "iot_adc.h"
#include "bk_saradc.h"
#include <bat_monitor.h>
// #include "app_event.h"

#if CONFIG_PM_ENABLE
#include <modules/pm.h>
#endif


/* ADC using params */
#define BAT_DETECT_ONESHOT_TIMER          1
#define BAT_DETEC_ADC_CLK                 203125
#define BAT_DETEC_ADC_SAMPLE_RATE         0
#define BAT_DETEC_ADC_STEADY_CTRL         7

#define ADC_VOL_BUFFER_SIZE               (5 + 5)   /* The first 5 samples can be skipped */
#define ADC_READ_SEMAPHORE_WAIT_TIME      1000      /* ms */
#define BATTERY_STATE_MONITORING_PERIOD   60 * 1000  /* ms */


/* On/Off toggle for configuration */
#define HARDWARE_SUPPORT_CURRENT          0
#define HARDWARE_SUPPORT_VOLTAGE          1
#define HARDWARE_SUPPORT_CHARGE_LVL       0
#define HARDWARE_BATTERY_PRESENT          1

/* Battery capacity threshold example (percentage) for simple determination */
#define SHUTDOWN_CAPACITY_THRESHOLD       2
#define LOW_CAPACITY_THRESHOLD            20
#define FULL_CAPACITY_THRESHOLD           95

#define GPIO_CHARGE      GPIO_51  // GPIO for charging state
#define GPIO_FULL        GPIO_26  // GPIO for fully charged state

/**
 * @brief Battery lookup table structure
 *  - voltageMV: Voltage value(unit: mV)
 *  - percent:   Corresponding battery percentage(unit: %)
 */
typedef struct
{
    uint16_t voltageMV;  /*!< Voltage points（mV） */
    uint8_t  percent;    /*!< Remaining battery percentage corresponding to the voltage point(0~100) */
} BatteryLUT_t;

/*
 * Example: Define several sampling points between 3.00V (3000mV) and 4.10V (4100mV).
 * !!!!The percentages mentioned here are for demonstration purposes only and may not represent real curves.!!!!
 * !!!!Please adjust according to your specific battery characteristics!!!!
 * !!!!Full charge state is determined by the external GPIO of the charging module.!!!!
 * !!!!Here, it is suggested to set the maximum percentage in the table to 99, while the fully charged state
 * is determined by the charging module.!!!!
 *
 */

static const BatteryLUT_t s_chargeLUT[] =
{
    {3000,   0},   /* 3.00V ->   0% */
    {3400,  10},   /* 3.40V ->  10% */
    {3450,  20},   /* 3.45V ->  20% */
    {3500,  30},   /* 3.50V ->  30% */
    {3550,  40},   /* 3.55V ->  40% */
    {3590,  50},   /* 3.59V ->  50% */
    {3650,  60},   /* 3.65V ->  60% */
    {3750,  70},   /* 3.75V ->  70% */
    {3880,  80},   /* 3.88V ->  80% */
    {3980,  90},   /* 3.98V ->  90% */
    {4100,  99},   /* 4.10V ->  99% */
};

#if CONFIG_BAT_MONITOR

static bool s_charging_init_status_flag = false;
static beken_thread_t battery_monitor_thread_hdl = NULL;
static IotBatteryHandle_t xGlobalHandle = NULL;
IotBatteryDescriptor_t gxBatteryDescriptor[BATTERY_MAX_INSTANCE] = { 0 };

static uint16_t * s_raw_voltage_data = NULL;

static uint16_t  prvCalculateVoltage( void );
static bk_err_t  prvStartBatteryAdcOneTime( uint16_t * vol );
static void      prvCheckChargeStatus( IotBatteryHandle_t xHandle );
static void      prvBatteryMonitorTaskMain( void );
static bk_err_t  prvBatteryMonitorTaskInit( void );

static battery_event_callback_t s_battery_event_callback = NULL;
int battery_event_callback_register(battery_event_callback_t callback)
{
	s_battery_event_callback = callback;
	return 0;
}

int32_t battery_get_voltage(uint16_t *pVoltage)
{
    if (xGlobalHandle == NULL || pVoltage == NULL)
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    return iot_battery_voltage(xGlobalHandle, pVoltage);
}

int32_t battery_get_current(uint16_t *pCurrent)
{
    if (xGlobalHandle == NULL || pCurrent == NULL)
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    return iot_battery_current(xGlobalHandle, pCurrent);
}

int32_t battery_get_charge_level(uint8_t *pLevel)
{
    if (xGlobalHandle == NULL || pLevel == NULL)
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    return iot_battery_chargeLevel(xGlobalHandle, pLevel);
}

static inline IotBatteryStatus_t battery_get_status_from_gpio(void)
{
    if(!xGlobalHandle)
    {
        return eBatteryUnknown;
    }

    int charge_state = bk_gpio_get_input(GPIO_CHARGE);
    int full_state   = bk_gpio_get_input(GPIO_FULL);

    if (charge_state == 1)
    {
        if (full_state == 1)
        {
            return eBatteryCharging;
        }
        else
        {
            return eBatteryChargeFull;
        }
    }
    else
    {
        return eBatteryDischarging;
    }
}

bool battery_if_is_charging(void)
{
    if (!xGlobalHandle)
    {
        return false;
    }

    IotBatteryStatus_t status = battery_get_status_from_gpio();
    return (status == eBatteryCharging);
}

IotBatteryInfo_t * battery_if_get_info(void)
{
    if (!xGlobalHandle)
    {
        return NULL;
    }
    return iot_battery_getInfo(xGlobalHandle);
}

static int hardware_read_voltage( uint16_t * pusVoltage )
{
    if( !HARDWARE_SUPPORT_VOLTAGE )
    {
        return -1;
    }
    /* for test:3800mV */
    //*pusVoltage = 3800;

	if (pusVoltage == NULL) {
        BAT_MONITOR_WPRT("Error: pusVoltage pointer is NULL\r\n");
        return -1;
    }

	bk_err_t ret = prvStartBatteryAdcOneTime(pusVoltage);
    return (ret == BK_OK) ? 0 : -1;

}

static int hardware_read_current( uint16_t * pusCurrent )
{
    if( !HARDWARE_SUPPORT_CURRENT )
    {
        return -1;
    }
    /* for test: 500mA */
    *pusCurrent = 500;

    /*TODO: user needs to implement it themselves  */
    return 0;
}

static int hardware_read_charge_level( uint8_t * pucChargeLevel )
{
    if( !HARDWARE_SUPPORT_CHARGE_LVL )
    {
        return -1;
    }
    /* for test: 50% */
    *pucChargeLevel = 50;

    /*TODO: user needs to implement it themselves  */
    return 0;
}

static bool hardware_battery_present( void )
{
    return (HARDWARE_BATTERY_PRESENT != 0);
}


/**
 * @brief Open and initialize battery/power management system
 */
IotBatteryHandle_t iot_battery_open( int32_t lBatteryInstance )
{

    if( lBatteryInstance < 0 || lBatteryInstance >= BATTERY_MAX_INSTANCE )
    {
        return NULL;
    }

    IotBatteryDescriptor_t * pxDesc = &gxBatteryDescriptor[lBatteryInstance];
    if( pxDesc->bIsOpen )
    {
        return NULL;
    }

    os_memset( pxDesc, 0, sizeof(*pxDesc) );
    pxDesc->bIsOpen = true;
    pxDesc->lInstance = lBatteryInstance;

    /* Simulate to determine if hardware has a battery */
    pxDesc->bBatteryPresent = hardware_battery_present();

    /* Set default battery information */
    pxDesc->xBatteryInfo.xBatteryType     = eBatteryChargeable;
    pxDesc->xBatteryInfo.usMinVoltage     = 3000;   /* mV */
    pxDesc->xBatteryInfo.usMaxVoltage     = 4100;   /* mV */
    pxDesc->xBatteryInfo.sMinTemperature  = 0;
    pxDesc->xBatteryInfo.lMaxTemperature  = 50;
    pxDesc->xBatteryInfo.usMaxCapacity    = 100;    /* Calculate based on 100% */
    pxDesc->xBatteryInfo.ucAsyncSupported = 1;      /* support for asynchronous Initialize measurement data */

    /* Initialize measurement data */
    pxDesc->usCurrentVoltage = 0;
    pxDesc->usCurrent        = 0;
    pxDesc->ucChargeLevel    = 0;
	pxDesc->xBatteryInfo.xBatteryStatus    = eBatteryUnknown;

    return (IotBatteryHandle_t) pxDesc;
}



/**
 * @brief Get battery information pointer
 */
IotBatteryInfo_t * iot_battery_getInfo( IotBatteryHandle_t const pxBatteryHandle )
{
    if( pxBatteryHandle == NULL )
    {
        return NULL;
    }

    IotBatteryDescriptor_t * pxDesc = (IotBatteryDescriptor_t *) pxBatteryHandle;
    if( !pxDesc->bIsOpen )
    {
        return NULL;
    }

    return &( pxDesc->xBatteryInfo );
}

/**
 * @brief Get battery current (mA)
 */
int32_t iot_battery_current( IotBatteryHandle_t const pxBatteryHandle,
                             uint16_t * pusCurrent )
{
    if( ( pxBatteryHandle == NULL ) || ( pusCurrent == NULL ) )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    IotBatteryDescriptor_t * pxDesc = (IotBatteryDescriptor_t *) pxBatteryHandle;
    if( !pxDesc->bIsOpen )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    if( !pxDesc->bBatteryPresent )
    {
        return IOT_BATTERY_NOT_EXIST;
    }

    if( !HARDWARE_SUPPORT_CURRENT )
    {
        return IOT_BATTERY_FUNCTION_NOT_SUPPORTED;
    }

    if( hardware_read_current( pusCurrent ) < 0 )
    {
        return IOT_BATTERY_READ_FAILED;
    }

    pxDesc->usCurrent = *pusCurrent;
    return IOT_BATTERY_SUCCESS;
}

/**
 * @brief Get battery voltage (mV)
 */
int32_t iot_battery_voltage( IotBatteryHandle_t const pxBatteryHandle,
                             uint16_t * pusVoltage )
{
    if( ( pxBatteryHandle == NULL ) || ( pusVoltage == NULL ) )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    IotBatteryDescriptor_t * pxDesc = (IotBatteryDescriptor_t *) pxBatteryHandle;
    if( !pxDesc->bIsOpen )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    if( !pxDesc->bBatteryPresent )
    {
        return IOT_BATTERY_NOT_EXIST;
    }

    if( !HARDWARE_SUPPORT_VOLTAGE )
    {
        return IOT_BATTERY_FUNCTION_NOT_SUPPORTED;
    }

    if( hardware_read_voltage( pusVoltage ) < 0 )
    {
        return IOT_BATTERY_READ_FAILED;
    }

	//CONVERT TO REAL VOL
	#if 1
    uint32_t temp = (uint32_t)(*pusVoltage) * 667;
	uint16_t practic_voltage = (uint16_t)((temp / 1000) + 40);
	#else
	float practic_voltage = (float)(s_raw_voltage_data[0] - saradc_val.low);
    practic_voltage = (practic_voltage / (float)(saradc_val.high - saradc_val.low)) + 1;
	#endif
    //BAT_MONITOR_PRT("pusVoltage = %d, practic_voltage = %d.\r\n",*pusVoltage, practic_voltage);

    *pusVoltage = practic_voltage;

    pxDesc->usCurrentVoltage = practic_voltage;

    return IOT_BATTERY_SUCCESS;
}

static uint8_t battery_voltage_to_percent(uint16_t voltageMV)
{
    const int LUT_SIZE = sizeof(s_chargeLUT) / sizeof(s_chargeLUT[0]);

    /* If it is below the minimum value, directly return the minimum percentage in the table*/
    if(voltageMV <= s_chargeLUT[0].voltageMV)
    {
        return s_chargeLUT[0].percent;
    }

    /* If the value exceeds the maximum value, return the maximum percentage */
    if(voltageMV >= s_chargeLUT[LUT_SIZE - 1].voltageMV)
    {
        return s_chargeLUT[LUT_SIZE - 1].percent;
    }

    /* Perform linear interpolation within the interval */
    for(int i = 0; i < LUT_SIZE - 1; i++)
    {
        uint16_t v1 = s_chargeLUT[i].voltageMV;
        uint16_t v2 = s_chargeLUT[i+1].voltageMV;

        if(voltageMV >= v1 && voltageMV <= v2)
        {
            uint8_t p1 = s_chargeLUT[i].percent;
            uint8_t p2 = s_chargeLUT[i+1].percent;

            uint16_t dist  = (v2 - v1);
            uint16_t delta = (voltageMV - v1);

            /* ratio: 0.0 ~ 1.0 */
            float ratio = (float)delta / (float)dist;
            float pf    = p1 + ratio * (p2 - p1);

            /* Round to the nearest integer */
            if(pf < 0)   pf = 0;
            if(pf > 100) pf = 100;

            return (uint8_t)(pf + 0.5f);
        }
    }

    /* According to theory, it wouldn't be here for safety. */
    return s_chargeLUT[LUT_SIZE - 1].percent;
}

/**
 * @brief Get battery remaining charge (%)，range 1~100
 */
int32_t iot_battery_chargeLevel( IotBatteryHandle_t const pxBatteryHandle,
                                 uint8_t * pucChargeLevel )
{
    if( ( pxBatteryHandle == NULL ) || ( pucChargeLevel == NULL ) )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    IotBatteryDescriptor_t * pxDesc = (IotBatteryDescriptor_t *) pxBatteryHandle;
    if( !pxDesc->bIsOpen )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    if( !pxDesc->bBatteryPresent )
    {
        return IOT_BATTERY_NOT_EXIST;
    }

    if (HARDWARE_SUPPORT_CHARGE_LVL)
    {
        if (hardware_read_charge_level( pucChargeLevel ) < 0 )
        {
            return IOT_BATTERY_READ_FAILED;
        }
        pxDesc->ucChargeLevel = *pucChargeLevel;
        return IOT_BATTERY_SUCCESS;
    }
    else if (HARDWARE_SUPPORT_VOLTAGE)
    {
        // Using voltage interpolation
        /* get vlotage ( mV ) */
        uint16_t voltageMV = 0;
        int32_t ret = iot_battery_voltage(pxBatteryHandle, &voltageMV);
        if(ret != IOT_BATTERY_SUCCESS)
        {
            return ret;
        }

        /* Call the interpolation function to convert voltageMV to percentage. */
        uint8_t batteryPercent = battery_voltage_to_percent(voltageMV);

        IotBatteryDescriptor_t *pxDesc = (IotBatteryDescriptor_t *) pxBatteryHandle;
        if (pxDesc->xBatteryInfo.xBatteryStatus == eBatteryCharging) {
            bool isReallyFull = (pxDesc->xBatteryInfo.xBatteryStatus == eBatteryChargeFull);
            if (!isReallyFull) {
                if (batteryPercent >= 99) {
                    batteryPercent = 99;
                }
            }
        }

        if (pxDesc->xBatteryInfo.xBatteryStatus == eBatteryChargeFull) {
            batteryPercent = 100;
        }

        /* update chargeLevel */
        pxDesc->ucChargeLevel = batteryPercent;

        *pucChargeLevel = batteryPercent;

        return IOT_BATTERY_SUCCESS;
    }
    else
    {
        return IOT_BATTERY_FUNCTION_NOT_SUPPORTED;
    }

}


/*
 * Calculate the average of the filtered list
 * Skip the first 5 samples，Filter out any values that are 0 or 2048
 */
static uint16_t prvCalculateVoltage( void )
{
    if( s_raw_voltage_data == NULL )
    {
        BAT_MONITOR_WPRT("s_raw_voltage_data is NULL.\r\n");
        return 0;
    }

    uint32_t sum   = 0;
    uint32_t count = 0;

    for( uint32_t i = 5; i < ADC_VOL_BUFFER_SIZE; i++ )
    {
        if( ( s_raw_voltage_data[i] != 0 ) &&
            ( s_raw_voltage_data[i] != 2048 ) )
        {
            sum += s_raw_voltage_data[i];
            count++;
        }
    }

    if( count == 0 )
    {
        s_raw_voltage_data[0] = 0;
    }
    else
    {
        s_raw_voltage_data[0] = (uint16_t)( sum / count );
    }

    return s_raw_voltage_data[0];
}

/*
 * Simultaneous sampling and calculation of voltage
 */
static bk_err_t prvStartBatteryAdcOneTime( uint16_t * vol )
{
	if (vol == NULL) {
        BAT_MONITOR_WPRT("Error: vol pointer is NULL\r\n");
        return BK_FAIL;
    }

    BK_LOG_ON_ERR( bk_adc_acquire() );
    BK_LOG_ON_ERR( bk_adc_init( ADC_0 ) );

    adc_config_t config;
    os_memset( &config, 0, sizeof(config) );

    config.chan          = ADC_0;
    config.adc_mode      = ADC_CONTINUOUS_MODE;
    config.src_clk       = ADC_SCLK_XTAL_26M;
    config.clk           = BAT_DETEC_ADC_CLK;
    config.saturate_mode = 4;
    config.steady_ctrl   = BAT_DETEC_ADC_STEADY_CTRL;
    config.adc_filter    = 0;
    config.sample_rate   = BAT_DETEC_ADC_SAMPLE_RATE;

    if( config.adc_mode == ADC_CONTINUOUS_MODE )
    {
        config.sample_rate = 0;
    }

    BK_LOG_ON_ERR( bk_adc_set_config( &config ) );
    BK_LOG_ON_ERR( bk_adc_enable_bypass_clalibration() );
    BK_LOG_ON_ERR( bk_adc_start() );

    bk_err_t ret = bk_adc_read_raw( s_raw_voltage_data,
                                    ADC_VOL_BUFFER_SIZE,
                                    ADC_READ_SEMAPHORE_WAIT_TIME );
    if( ret != BK_OK )
    {
        BAT_MONITOR_WPRT("Failed to read ADC data, err: %d\r\n", ret );
		*vol = 0;
        goto ADC_EXIT;
    }

    *vol = prvCalculateVoltage();
    //BAT_MONITOR_PRT("ADC VALUE: %d .\r\n", *vol);

ADC_EXIT:
    BK_LOG_ON_ERR( bk_adc_stop() );
    BK_LOG_ON_ERR( bk_adc_deinit( ADC_0 ) );
    BK_LOG_ON_ERR( bk_adc_release() );

	return ret;
}

/*
 * Check the GPIO pin to determine whether it is charging, fully charged, or without an external power source
 */
static void prvCheckChargeStatus( IotBatteryHandle_t xHandle )
{

    IotBatteryDescriptor_t * pxDesc = (IotBatteryDescriptor_t *) xHandle;
    if(pxDesc == NULL)
    {
        BAT_MONITOR_WPRT("Invalid battery handle in prvCheckChargeStatus\r\n");
        return;
    }

    int charge_state = bk_gpio_get_input( GPIO_CHARGE );
    int full_state   = bk_gpio_get_input( GPIO_FULL );

    //printf("charge_state = %d,full_state = %d.\r\n",charge_state,full_state);

    if( charge_state == 1 )
    {
        if(full_state == 1)
        {
            pxDesc->xBatteryInfo.xBatteryStatus = eBatteryCharging;
            BAT_MONITOR_PRT("Device is charging...\r\n");
        }
        else
        {
            pxDesc->xBatteryInfo.xBatteryStatus = eBatteryChargeFull;
            BAT_MONITOR_PRT("Battery is full.\r\n");
        }
    }
    else
    {
        pxDesc->xBatteryInfo.xBatteryStatus = eBatteryDischarging;
        BAT_MONITOR_PRT("Battery powered.\r\n");
    }

}

int32_t iot_battery_close(IotBatteryHandle_t pxBatteryHandle)
{
    if(pxBatteryHandle == NULL)
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    IotBatteryDescriptor_t *pxDesc = (IotBatteryDescriptor_t *)pxBatteryHandle;

    if(!pxDesc->bIsOpen)
    {
        // Already closed or not opened
        return IOT_BATTERY_INVALID_VALUE;
    }

    pxDesc->bIsOpen = false;
    return IOT_BATTERY_SUCCESS;
}


/*
 * Monitoring Thread
 */
static void prvBatteryMonitorTaskMain( void )
{
    static bool bLowVoltageTriggered = false;  // Low Battery Status Indicator
    static bool bShutdownTriggered = false;

    xGlobalHandle = iot_battery_open( 0 );
    if( xGlobalHandle == NULL )
    {
        BAT_MONITOR_WPRT("Failed to open battery driver!\r\n");
        goto TASK_EXIT;
    }

    /* Get basic information and print it */
    IotBatteryInfo_t * pxInfo = iot_battery_getInfo( xGlobalHandle );

    if( pxInfo )
    {
        BAT_MONITOR_PRT("Battery info: Type=%d, MinVolt=%d, MaxVolt=%d\r\n",
             pxInfo->xBatteryType,
             pxInfo->usMinVoltage,
             pxInfo->usMaxVoltage );
    }

    /* Every BATTERY_STATE_MONITORING_PERIOD, check the charging state, sample the voltage, evaluate the state */
    while( s_charging_init_status_flag )
    {
        /* Check charging status */
        prvCheckChargeStatus( xGlobalHandle );
        if (pxInfo->xBatteryStatus == eBatteryCharging)
        {
            if (s_battery_event_callback) {
                s_battery_event_callback(EVT_BATTERY_CHARGING);
            }
            bLowVoltageTriggered = false;
            bShutdownTriggered = false;
        }

        {
            uint16_t usVoltage   = 0;
            uint16_t usCurrent   = 0;
            uint8_t  ucCharge    = 0;

            if( iot_battery_voltage( xGlobalHandle, &usVoltage ) == IOT_BATTERY_SUCCESS )
            {
                if(pxInfo->xBatteryStatus == eBatteryCharging)
                    BAT_MONITOR_PRT("Supply voltage: %u mV\r\n", usVoltage);
                else
                    BAT_MONITOR_PRT("Battery voltage: %u mV\r\n", usVoltage);
            }
            if( iot_battery_current( xGlobalHandle, &usCurrent ) == IOT_BATTERY_SUCCESS )
            {
                BAT_MONITOR_PRT("Battery current: %u mA\r\n", usCurrent);
            }
            if( iot_battery_chargeLevel( xGlobalHandle, &ucCharge ) == IOT_BATTERY_SUCCESS )
            {
                /* Low battery detection logic */
                if ((ucCharge <= SHUTDOWN_CAPACITY_THRESHOLD) && (pxInfo->xBatteryStatus != eBatteryCharging))
                {
                    if (!bShutdownTriggered)
                    {
                        if (s_battery_event_callback) {
                            s_battery_event_callback(EVT_SHUTDOWN_LOW_BATTERY);
                        }
                        BAT_MONITOR_WPRT("Shutdown due to critical battery level!\r\n");
                        bShutdownTriggered = true;

                        // if you want to shutdown immdiately,can runnning this fake function here：
                        // system_shutdown();
                    }
                }
                else if ((ucCharge <= LOW_CAPACITY_THRESHOLD) && (pxInfo->xBatteryStatus != eBatteryCharging))
                {
                    if (!bLowVoltageTriggered)
                    {
                        if (s_battery_event_callback) {
                            s_battery_event_callback(EVT_BATTERY_LOW_VOLTAGE);
                        }
                        BAT_MONITOR_WPRT("Low voltage event triggered!\r\n");
                        bLowVoltageTriggered = true;
                    }
                    bShutdownTriggered = false;
                }
                else
                {
                    bLowVoltageTriggered = false;  // When charging resumes, reset the flag
                    bShutdownTriggered = false;
                }
                if(pxInfo->xBatteryStatus != eBatteryCharging)
                    BAT_MONITOR_PRT("Battery level: %u%%\r\n", ucCharge);
            }
        }

        rtos_delay_milliseconds( BATTERY_STATE_MONITORING_PERIOD );
    }

TASK_EXIT:

    if (xGlobalHandle) {
        iot_battery_close(xGlobalHandle);
        xGlobalHandle = NULL;
    }

    if( s_raw_voltage_data )
    {
        os_free( s_raw_voltage_data );
        s_raw_voltage_data = NULL;
    }

    battery_monitor_thread_hdl = NULL;
    rtos_delete_thread( NULL );
}

/**
 * @brief Create a battery monitoring thread and allocate a buffer
 */

static bk_err_t prvBatteryMonitorTaskInit( void )
{
    if( battery_monitor_thread_hdl != NULL )
    {
        BAT_MONITOR_PRT("Battery monitor task already running.\r\n");
        return BK_OK;
    }

    s_raw_voltage_data = (uint16_t *) os_malloc( ADC_VOL_BUFFER_SIZE * sizeof(uint16_t) );
    if( s_raw_voltage_data == NULL )
    {
        BAT_MONITOR_WPRT("Failed to allocate memory for s_raw_voltage_data\r\n");
        return BK_ERR_NO_MEM;
    }

#if CONFIG_PSRAM_AS_SYS_MEMORY
    bk_err_t ret = rtos_create_psram_thread( &battery_monitor_thread_hdl,
                                       4,
                                       "battery_monitor",
                                       (beken_thread_function_t)prvBatteryMonitorTaskMain,
                                       1536,
                                       (beken_thread_arg_t)NULL );
#else
    bk_err_t ret = rtos_create_thread( &battery_monitor_thread_hdl,
                                       4,
                                       "battery_monitor",
                                       (beken_thread_function_t)prvBatteryMonitorTaskMain,
                                       1536,
                                       (beken_thread_arg_t)NULL );
#endif

    if( ret != BK_OK )
    {
        battery_monitor_thread_hdl = NULL;
        os_free( s_raw_voltage_data );
        s_raw_voltage_data = NULL;
        BAT_MONITOR_WPRT("Failed to create battery_monitor task, err=%d\r\n", ret );
        return BK_ERR_NOT_INIT;
    }

    return BK_OK;
}

void battery_monitor_init( void )
{
    if( s_charging_init_status_flag )
    {
        BAT_MONITOR_PRT("Battery monitor has already been initialized.\n");
        return;
    }

    bk_err_t ret = prvBatteryMonitorTaskInit();
    if( ret != BK_OK )
    {
        BAT_MONITOR_PRT("Battery monitor task create failed!\n");
        return;
    }

    s_charging_init_status_flag = true;
    BAT_MONITOR_PRT("Battery monitor initialized.\n");
}

void battery_monitor_deinit(void)
{
    if (!s_charging_init_status_flag) {
        BAT_MONITOR_PRT("Battery monitor already deinitialized.\n");
        return;
    }

    s_charging_init_status_flag = false;

    if (battery_monitor_thread_hdl) {
        rtos_delete_thread(&battery_monitor_thread_hdl);
        battery_monitor_thread_hdl = NULL;
    }

    if (xGlobalHandle) {
        iot_battery_close(xGlobalHandle);
        xGlobalHandle = NULL;
    }

    if (s_raw_voltage_data) {
        os_free(s_raw_voltage_data);
        s_raw_voltage_data = NULL;
    }

    BAT_MONITOR_PRT("Battery monitor deinitialized.\n");
}

#endif /* CONFIG_BAT_MONITOR */
