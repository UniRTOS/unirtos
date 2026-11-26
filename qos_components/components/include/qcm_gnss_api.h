/*****************************************************************/ /**
* @file qcm_gnss_api.h
* @brief
* @author kruskal.zhu@quectel.com
* @date   2025-10
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date    <th>Version <th>Author       <th>Description"
* <tr><td>2025-10 <td>1.0     <td>kruskal.zhu  <td> Init
* </table>
**********************************************************************/

#ifndef __QCM_GNSS_API_H__
#define __QCM_GNSS_API_H__

#include "qosa_def.h"
#include "qosa_rtc.h"

#define QCM_GNSS_GSV_NUM            8
#define QCM_GNSS_GSA_NUM            8
#define QCM_GNSS_NMEA_LEN           256 //128

//GNSS AGPS Default Parameter Configuration
#define QCM_GNSS_AGPS_URL_LEN       256
#define QCM_GNSS_AGPS_VENDOR_ID_LEN 32
#define QCM_GNSS_AGPS_MODEL_ID_LEN  32
#define QCM_GNSS_AGPS_PASSWORD_LEN  32

//GNSS AGPS default AGPS server address URL
#ifdef CONFIG_QOSA_QCX216_PLATFORM_FUNC
#define QCM_GNSS_AGPS_URL_DEF       "http://elpo.airoha.com/EPO_GR_3_1.DAT?vendor=QUECTEL-TEST&project=4kn_0DxhWopmK6uRyZHcmsvxGg9sfuPXa3w2StXMDLO&device_id=1"
//GNSS AGPS default AGPS server manufacturer ID, model ID, and password
#define QCM_GNSS_AGPS_VENDOR_ID_DEF ""
#define QCM_GNSS_AGPS_MODEL_ID_DEF  ""
#define QCM_GNSS_AGPS_PASSWORD_DEF  ""
#else
#define QCM_GNSS_AGPS_URL_DEF       "http://quectel-api1.rx-networks.cn/rxn-api/locationApi/rtcm"
//GNSS AGPS default AGPS server manufacturer ID, model ID, and password
#define QCM_GNSS_AGPS_VENDOR_ID_DEF "wLgWwv6JQt"
#define QCM_GNSS_AGPS_MODEL_ID_DEF  "Quectel"
#define QCM_GNSS_AGPS_PASSWORD_DEF  "aFltUERDZzZxeTY5cEp2eA=="
#endif
// GNSS NTP
#define QCM_GNSS_NTP_URL_LEN        256
//GNSS NTP Default Server URL
#define QCM_GNSS_NTP_URL_DEF        "ntp.aliyun.com"
//GNSS NTP server default port number
#define QCM_GNSS_NTP_PORT_DEF       123

//GNSS uses SIM card 0 by default
#define QCM_GNSS_DEF_SIM_ID    0
//GNSS uses PDP_CID=1 by default
#define QCM_GNSS_DEF_PDP_CID   1

typedef enum
{
    QCM_GNSS_ERR_OK         = 0,  /*!< Operation successful */

    QCM_GNSS_ERR_UNKNOW     = 1,  /*!< Unknown error */
    QCM_GNSS_ERR_INVAL_PARM = 2,  /*!< Invalid parameter error */
    QCM_GNSS_ERR_NO_MEMORY  = 3,  /*!< Insufficient memory error */
    QCM_GNSS_ERR_NOT_INIT   = 4,  /*!< not initialized error */
    QCM_GNSS_ERR_BUSY       = 5,  /*!< Busy error */
    QCM_GNSS_ERR_EXECUTE    = 6,  /*!< Execution failed */
    QCM_GNSS_ERR_NOT_START  = 7,  /*!< The GNSS module is not powered on */
    QCM_GNSS_ERR_NOT_RUN    = 8,  /*!< GNSS is not positioned */

    // Firmware related errors
    QCM_GNSS_ERR_FIRMWARE   = 9,  /*!< Firmware loading failed */
    // AGPS related errors
    QCM_GNSS_ERR_PDP        = 10, /*!< pdp activation failed */
    QCM_GNSS_ERR_SOCKET     = 11, /*!< Socket connection error */
} qcm_gnss_err_e;

/**
 * @brief Event notification for status result return during API asynchronous operations
 */
typedef enum
{
    //Event notification for status result return during API asynchronous operations
    QCM_GNSS_NOTIFY_CFG_EVT,             /*!< Configuration parameters set successfully, this notification will be received after executing the qcm_gnss_cfg function.*/
    QCM_GNSS_NOTIFY_START_EVT,           /*!< GNSS service startup status notification, this notification will be received after executing the qcm_gnss_start function.*/
    QCM_GNSS_NOTIFY_STOP_EVT,            /*!< GNSS service stop status notification, this notification will be received after executing the qcm_gnss_stop function.*/
    QCM_GNSS_NOTIFY_SEND_RAW_DATA_EVT,   /*!< Raw data transmission notification, this notification will be received after executing the qcm_gnss_send_raw_data function.*/
    QCM_GNSS_NOTIFY_DEL_ASSIST_DATA_EVT, /*!< Delete assistance data notification, this notification will be received after executing the qcm_gnss_delete_assistance_data function.*/

    //GNSS Module Status Notification
    QCM_GNSS_NOTIFY_FIRMWARE_LOAD_EVT, /*!< Firmware loading result notification*/
    QCM_GNSS_NOTIFY_TRACK,             /*!< GNSS service has started a tracking session*/
    QCM_GNSS_NOTIFY_FIX_ACQUIRED,      /*!< Valid position fix (2D or 3D) obtained*/
    QCM_GNSS_NOTIFY_FIX_LOST,          /*!< Position location lost*/
    QCM_GNSS_NOTIFY_SYSTEM_TIME_UPDATE,/*!< System time update status notification*/
    QCM_GNSS_NOTIFY_AGPS_UPDATE_EVT,   /*!< AGPS assistance data update status notification*/
    QCM_GNSS_NOTIFY_ERROR_RECOVERABLE, /*!< Recoverable error occurred (e.g., AGPS download failure)*/
    QCM_GNSS_NOTIFY_ERROR_FATAL,       /*!< Fatal unrecoverable error occurred*/
    QCM_GNSS_NOTIFY_FIX_SESSION_DONE,  /*!< Tracking session completed*/
} qcm_gnss_notify_e;

/**
 * @brief Main status of GNSS
 */
typedef enum
{
    QCM_GNSS_STATE_IDLE,      /*!< Default state: not started, not initialized*/
    QCM_GNSS_STATE_SYS_INIT,  /*!< System initialization completed, resource allocation finished.*/
    QCM_GNSS_STATE_GNSS_INIT, /*!< Power on and initialize the GNSS module*/
    QCM_GNSS_STATE_TRACK,     /*!< The GNSS module is tracking satellites and parsing data*/
    QCM_GNSS_STATE_STOP,      /*!< Closing module*/
    QCM_GNSS_STATE_SYS_DEINIT /*!< Closing the module and releasing system resources*/
} qcm_gnss_state_e;

//GNSS chip startup type (ephemeris data deletion level)
typedef enum
{
    QCM_GNSS_RESET_COLD = 0, /*!< Cold start (full deletion)*/
    QCM_GNSS_RESET_HOT  = 1, /*!< Hot  start (no deletion)*/
    QCM_GNSS_RESET_WARM = 2, /*!< Warm Start (Partial Deletion)*/
} qcm_gnss_reset_type_e;

//This enumeration defines the prefix for BeiDou satellite NMEA sentence output.
typedef enum
{
    QCM_GNSS_BDS_PREFIX_PQ = 0,    /*!< BDS NMEA sentence prefix: PQ*/
    QCM_GNSS_BDS_PREFIX_GB = 1,    /*!< BDS NMEA sentence prefix: GB*/
    QCM_GNSS_BDS_PREFIX_BD = 2,    /*!< BDS NMEA sentence prefix: BD*/
} qcm_gnss_bds_prefix_e;

//Location Information Structure
typedef struct
{
    /* RMC */
    int             valid;              /*!< Positioning validity (1=valid, 0=invalid)*/
    double          latitude;           /*!< Latitude*/
    unsigned char   latitude_cardinal;  /*!< Latitude N (North) or S (South)*/
    double          longitude;          /*!< Longitude*/
    unsigned char   longitude_cardinal; /*!< Longitude E (East) or W (West)*/
    double          speed;              /*!< Ground speed (knots)*/
    double          course;             /*!< Ground heading (degrees)*/
    qosa_rtc_time_t time;               /*!< Time information (including year, month, day, hour, minute, second)*/

    /* GSA */
    unsigned char mode; /*!<   Mode (M=Manual, A=Auto)*/
    int navmode; /*!<  Navigation mode (1=unpositioned, 2=2D, 3=3D) (1-Unpositioned: unable to obtain a valid position solution; 2-2D positioning: obtains latitude and longitude only [requires at least 3 satellites]; 3-3D positioning: obtains latitude, longitude, and altitude [requires at least 4 satellites])*/
    double pdop; /*!< Position Dilution of Precision*/
    double hdop; /*!< Horizontal Dilution of Precision*/
    double vdop; /*!< Vertical Dilution of Precision*/

    /* GGA */
    int   satellites_tracked; /*!< Number of tracked satellites*/
    float altitude;           /*!< Altitude*/

    /* VTG */
    double        cogt;      /*!< True Heading (degrees) ,degrees (0-359) ,ground heading angle relative to true north*/
    unsigned char cogt_flag; /*!< True heading identifier flag, value: 'T' indicates true heading is valid, used to verify if the field is valid*/
    double        cogm;      /*!< Magnetic heading (degrees), degrees (0-359), ground heading angle relative to magnetic north*/
    unsigned char cogm_flag; /*!< Magnetic heading identifier flag, value: 'M' indicates magnetic heading is valid, used to verify if the field is valid*/
    double        spkn;      /*!< Speed - Knots, unit: knots (Knots)  1 knot = 1 nautical mile/hour = 1.852 km/hour*/
    unsigned char spkn_flag; /*!< Section identifier flag, value: 'N' indicates section speed is valid, used to verify if the field is valid*/
    double        spkm;      /*!< Speed - km/h, Unit: km/h, 1 km/h = 0.54 knots*/
    unsigned char spkm_flag; /*!< Kilometer per hour identifier flag, value: 'K' indicates kilometer per hour speed is valid, used to verify if the field is valid*/
    unsigned char
        mode_indicator_flag; /*!< Mode indicator flag, possible values: 'A' = Autonomous mode, 'D' = Differential mode, 'E' = Estimated mode, 'M' = Manual input mode, 'S' = Simulator mode, 'N' = Data invalid*/
} qcm_gnss_location_t;

//This enumeration defines the types of satellite navigation systems.
typedef enum
{
    QCM_CONSTEL_GPS = 0,      /*!< GPS System        United States Global Positioning System*/
    QCM_CONSTEL_GLONASS = 1,  /*!< GLONASS system    Russian Global Navigation Satellite System*/
    QCM_CONSTEL_GALILEO = 2,  /*!< GALILEO system    European Union Galileo positioning system*/
    QCM_CONSTEL_BDS = 3,      /*!< BDS system        China's BeiDou Navigation Satellite System*/
    QCM_CONSTEL_MULTISYS = 4, /*!< Multi-system          Hybrid positioning mode*/
    QCM_CONSTEL_MAX = 5,      /*!< Number of enumeration items*/
} qcm_gnss_constel_e;

//This enumeration defines the types of NMEA sentences.
typedef enum
{
    QCM_NMEA_GGA = 0, /*!< Global Positioning System Fix Data               GPS positioning data                  Contains information such as time, location, fix quality, number of satellites, altitude, etc.*/
    QCM_NMEA_RMC = 1, /*!< Recommended Minimum Specific GPS/Transit Data    Recommended Minimum Positioning Information    Contains basic information such as time, date, position, speed, and heading.*/
    QCM_NMEA_GSA = 2, /*!< GPS DOP and Active Satellites                    GPS DOP and Active Satellites       Contains positioning mode, used satellite PRN numbers, Position Dilution of Precision (DOP), etc.*/
    QCM_NMEA_GSV = 3, /*!< GPS Satellites in View                           Visible Satellite Information                Includes total number of visible satellites, satellite ID, elevation, azimuth, signal-to-noise ratio, etc.*/
    QCM_NMEA_VTG = 4, /*!< Track Made Good and Ground Speed                 Ground Speed Information                Contains true bearing, magnetic bearing, ground speed (knots and km/h)*/
    QCM_NMEA_GLL = 5, /*!< Geographic Position - Latitude/Longitude         Geographic Location Information (Latitude/Longitude)         Contains latitude, longitude, and time*/
    QCM_NMEA_TXT = 6, /*!< Text Transmission                                Text Information Transmission                Contains text messages, typically used for device status or error information*/
    QCM_NMEA_ZDA = 7, /*!< Time and Date                                    Time and Date Information              Contains UTC time, date, and timezone information*/
    QCM_NMEA_GNS = 8, /*!< GNSS Fix Data                                    Global Navigation Satellite System Fix Data    GNSS version of the GGA sentence, supports multiple satellite systems (GPS, GLONASS, Galileo, etc.)*/
    QCM_NMEA_GST = 9, /*!< GNSS Pseudorange Signal Tracking                 Report pseudorange noise statistics for all constellations, including RMS, standard deviation, and ellipse error components.*/
    QCM_NMEA_MAX = 10,/*!< Number of NMEA enumerations*/
} qcm_gnss_nmea_type_e;

//This structure defines the source data for NMEA sentences
typedef struct
{
    char gga[QCM_GNSS_NMEA_LEN];
    char rmc[QCM_GNSS_NMEA_LEN];

    char gsv[QCM_CONSTEL_MAX][QCM_GNSS_GSV_NUM][QCM_GNSS_NMEA_LEN];
    char gsa[QCM_GNSS_GSA_NUM][QCM_GNSS_NMEA_LEN];

    char vtg[QCM_GNSS_NMEA_LEN];
    char gll[QCM_GNSS_NMEA_LEN];
    char gst[QCM_GNSS_NMEA_LEN];
} qcm_gnss_nmeasrc_sentences_t;

//Set the positioning satellite combination for the GNSS chip (expandable in the future)
typedef enum
{
    QCM_MULTI_GP = 0,          /*!< GPS */
    QCM_MULTI_GP_GB_GL = 1,    /*!< GPS+BDS+GLONASS */
    QCM_MULTI_GP_GB_GA = 2,    /*!< GPS+BDS+GALILEO */
    QCM_MULTI_GP_GL_GA = 3,    /*!< GPS+GLONASS+GALILEO */
    QCM_MULTI_GP_GL = 4,       /*!< GPS+GLONASS */
    QCM_MULTI_GP_GB = 5,       /*!< GPS+BDS */
    QCM_MULTI_GP_GA = 6,       /*!< GPS+GALILEO */
    QCM_MULTI_GB = 7,          /*!< BDS */
    QCM_MULTI_GP_GB_GA_GL = 8, /*!< GPS+BDS+GALILEO+GLONASS */
} qcm_gnss_multi_constel_e;

//GNSS AGPS configuration structure
typedef struct
{
    qosa_bool_t  enable;                                 /*!< Enable/Disable AGPS function*/
    char         url[QCM_GNSS_AGPS_URL_LEN];             /*!< AGPS server address*/
    char         vendor_id[QCM_GNSS_AGPS_VENDOR_ID_LEN]; /*!< Username*/
    char         model_id[QCM_GNSS_AGPS_MODEL_ID_LEN];   /*!< Client identifier*/
    char         password[QCM_GNSS_AGPS_PASSWORD_LEN];   /*!< Password*/
} qcm_gnss_agps_cfg_t;

//GNSS NTP configuration structure, NTP can be enabled for network time calibration after successful network registration
typedef struct
{
    qosa_bool_t  enable;                       /*!< Enable/Disable NTP function*/
    char         url[QCM_GNSS_NTP_URL_LEN];    /*!< NTP server address, used for NTP time calibration*/
    int          port;                         /*!< NTP port number*/
} qcm_gnss_ntp_cfg_t;

//GNSS Configuration Structure
typedef struct
{
    /*!< User specifies the galaxy and outputs the corresponding NMEA type statement.*/
    /*!< gps/glonass/galileo/beidou/gnss  meatype */
    qosa_bool_t constel_nmea[QCM_CONSTEL_MAX][QCM_NMEA_MAX];

    /*!< Set the positioning satellite combination for the GNSS chip*/
    qcm_gnss_multi_constel_e multi_constel;

    /*!< Number of positioning attempts, 0 indicates continuous positioning*/
    qosa_uint32_t fix_count;

    /*!< AGPS related configuration*/
    qcm_gnss_agps_cfg_t agps_cfg;

    /*!< NTP related configuration*/
    qcm_gnss_ntp_cfg_t ntp_cfg;

     /*!< NTP AGPS General Configuration*/
    qosa_uint8_t simid;  /*!< SIM card ID*/
    qosa_uint8_t pdpcid; /*!< PDP Index ID*/

    /*!< Set GNSS chip cold/warm/hot start*/
    /*!< The reset mode is now set through qcm_gnss_set_reset_mode.
    The members here have no practical significance and are merely retained to ensure consistency between different versions of gnss nv cfg.*/
    qcm_gnss_reset_type_e reset_mode;

    //APFLASH and backup power cannot be enabled simultaneously.
    /*!< APFLASH enable (mutually exclusive with backup power)*/
    qosa_bool_t apflash_enable;
    /*!< Backup power enable (mutually exclusive with APFLASH)*/
    qosa_bool_t backup_power_enable;

    /*!< Configure BDS NMEA sentence prefix*/
    qcm_gnss_bds_prefix_e bds_prefix;

    /*!< Turn on the GNSS chip DEBUG mode */
    qosa_bool_t debug_enable;
} qcm_gnss_cfg_t;

typedef void (*qcm_gnss_notify_cb_t)(qcm_gnss_notify_e notify, qcm_gnss_err_e error_code);

typedef void (*qcm_gnss_nmea_cb_t)(const char *nmea, int nmea_len);

/**
 * @brief GNSS Initialization
 *
 * @param[in] qcm_gnss_notify_cb status_cb
 *          - GNSS asynchronous status notification
 *
 * @param[in] qcm_gnss_nmea_cb nmea_cb
 *          - GNSS NMEA Sentence Notification
 *
 * @return qcm_gnss_err_e
 *       - GNSS initialization successful returns QCM_GNSS_ERR_OK
 *       - GNSS initialization failed and returned other errors
 */
qcm_gnss_err_e qcm_gnss_init(qcm_gnss_notify_cb_t status_cb, qcm_gnss_nmea_cb_t nmea_cb);

/**
 * @brief GNSS Deinitialization
 */
qcm_gnss_err_e qcm_gnss_deinit(void);

/**
 * @brief Start GNSS, it is recommended to execute the defined config configuration before starting.
 *
 * @note Asynchronous interface, the success or failure of execution requires waiting for the QCM_GNSS_NOTIFY_START_EVT event notification from qcm_gnss_notify_cb
 *
 */
qcm_gnss_err_e qcm_gnss_start(void);

/**
 * @brief Stop the current GNSS session, retaining initialization resources and configuration.
 *
 * @return QCM_GNSS_ERR_OK means the request was queued; wait for
 *         QCM_GNSS_NOTIFY_STOP_EVT to determine whether shutdown succeeded.
 * @note On successful completion, the chip is powered off, the sleep vote is
 *       released, and location/NMEA/session state is cleared. Call start again
 *       without init or set_cfg; each run restarts the configured fix count.
 *       Background time synchronization/downloads may finish independently;
 *       hardware injection is only allowed in an active session.
 * @note Completion runs in the GNSS core task. Do not block or call deinit or
 *       set_cfg from that callback. Queue control requests from an owner task.
 *       Repeated stop is allowed. A failed stop remains retryable; use deinit
 *       from the owner task when full resource destruction is required.
 */
qcm_gnss_err_e qcm_gnss_stop(void);

/**
 * @brief Obtain GNSS positioning information
 *
 * @param[out] qcm_gnss_location_t *location
 *          - Positioning information after successful GNSS positioning
 *
 * @note This is a synchronous interface that directly retrieves the corresponding positioning information after a successful fix.
 */
qcm_gnss_err_e qcm_gnss_get_location(qcm_gnss_location_t *location);

/**
 * @brief @brief Get NMEA sentence
 *
 * @param[out] qcm_gnss_nmeasrc_sentences_t * nmeasrc_sentences
 *           - NMEA Primitive
 *
 * @return qcm_gnss_err_e
 *
 * @note This is a synchronous interface used to directly retrieve configured specific NMEA sentences.
 */
qcm_gnss_err_e qcm_gnss_get_nmeasrc_sentences(qcm_gnss_nmeasrc_sentences_t *nmeasrc_sentences);

/**
 * @brief Set GNSS configuration
 *
 * @param[in] qcm_gnss_cfg_t * cfg
 *          - GNSS Configuration Parameters
 *
 * @return qcm_gnss_err_e
 *       - GNSS configuration successful returns QCM_GNSS_ERR_OK
 *       - GNSS configuration failure returns other errors
 *
 * @note Asynchronous interface, the success or failure of execution requires waiting for the QCM_GNSS_NOTIFY_CFG_EVT event notification from qcm_gnss_notify_cb.
 */
qcm_gnss_err_e qcm_gnss_set_cfg(qcm_gnss_cfg_t *cfg);

/**
 * @brief Get GNSS Configuration
 *
 * @param[out] qcm_gnss_cfg_t * cfg
 *           - GNSS Configuration Parameters
 *
 * @return qcm_gnss_err_e
 *       - GNSS configuration successful returns QCM_GNSS_ERR_OK
 *       - GNSS configuration failure returns other errors
 *
 * @note This function is a synchronous interface that directly retrieves the corresponding CONFIG configuration information.
 */
qcm_gnss_err_e qcm_gnss_get_cfg(qcm_gnss_cfg_t *cfg);

/**
 * @brief Set the GNSS reset mode
 *
 * @param[in] qcm_gnss_reset_type_e reset_mode
 *            The reset type of GNSS
 *
 * @return qcm_gnss_err_e
 *       - GNSS configuration successful returns QCM_GNSS_ERR_OK
 *       - GNSS configuration failure returns other errors
 *
 * @note This function is an asynchronous interface; Used to set the reset mode of GNSS
 */
qcm_gnss_err_e qcm_gnss_set_reset_mode(qcm_gnss_reset_type_e reset_mode);

/**
 * @brief Get GNSS status
 */
qcm_gnss_state_e qcm_gnss_get_state(void);

/**
 * @brief Directly send the original GNSS CMD commands of the chip platform
 *
 * @param[in] char * cmd
 *          - GNSS raw CMD commands
 *
 * @param[in] int cmd_len
 *          - GNSS raw CMD command length
 *
 * @return qcm_gnss_err_e
 *       - GNSS raw CMD command sent successfully returns QCM_GNSS_ERR_OK
 *       - GNSS raw CMD command sending failed, returning other errors
 *
 * @note This function is an asynchronous interface; the success or failure of its execution requires waiting for the QCM_GNSS_NOTIFY_SEND_RAW_DATA_EVT event notification from qcm_gnss_notify_cb.
 */
qcm_gnss_err_e qcm_gnss_send_raw_data(char *cmd, int cmd_len);

/**
 * @brief Delete auxiliary positioning data
 */
qcm_gnss_err_e qcm_gnss_delete_assistance_data(void);

/**
 * @brief Directly obtain GNSS version information
 */
qcm_gnss_err_e qcm_gnss_get_version(char *version, int len);

#endif /* __QCM_GNSS_API_H__ */
