/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    psl1commtype.h
 * Description:
 * History:      25/04/2021    Originated
 *
 ****************************************************************************/

#ifndef _PS_L1_COMMON_TYPE_H_
#define _PS_L1_COMMON_TYPE_H_

/*********************************************************************************
* Includes
*********************************************************************************/
#ifdef WIN32
#include "pssys.h"
#else
#include "commontypedef.h"
#endif


/******************************************************************************
 *****************************************************************************
 * MACRO
 *****************************************************************************
******************************************************************************/
#define SUPPORT_MAX_BAND_NUM            32                         //Extend from 16 to 32, 2020-7-28
#define RRC_PHY_SUPPORT_BAND_NUM        SUPPORT_MAX_BAND_NUM       //this value should be the same as PHY_SUPPORT_BAND_NUM



/******************************************************************************
 *****************************************************************************
 * STRUCT
 *****************************************************************************
******************************************************************************/

/******************************************************************************
 * Plmn
 * Numberic PLMN format, 4 bytes,
 * example: CMCC: 46000; mcc = 0x0460, mnc = 0xf000
 *        00101; mcc = 0x0001, mnc = 0xf001
 *        46012; mcc = 0x0460, mnc = 0xf012
 *        460123; mcc = 0x0460, mnc = 0x0123
******************************************************************************/
typedef struct _EPAT_Plmn_Tag
{
    UINT16  mcc;
    UINT16  mncWithAddInfo; // if 2-digit MNC type, the 4 MSB bits should set to 'F',
}Plmn;


/*
 * UE category, ref to 36.306
*/
typedef UINT8 UeCategoryType_t;
typedef enum _EPAT_UeCategoryType_Tag
{
    UeCategory_NB1 = 0x01,
    UeCategory_NB2 = 0x02,
    UeCategory_M1 = 0x11,
    UeCategory_M2 = 0x12,
    UeCategory_Cat1Base = 0x20, //only used for Cat1 start point
    UeCategory_Cat1 = 0x21,
    UeCategory_Cat1bis = 0x22,
}UeCategoryType;

/*
* AT+ECMEASCFG
*/
typedef struct CerrcCcmMeasAdjustConfig_Tag
{
    INT16   rsrpThreshAdjust;       //range (-388..388), unit in 1/4dB
    INT16   rsrqThreshAdjust;       //range (-124..124), unit in 1/4dB
    INT16   rsrpRptAdjust;          //range (-388..388), unit in 1/4dB
    INT16   rsrqRptAdjust;          //range (-124..124), unit in 1/4dB
    INT16   rsrpMeasAdjust;         //range (-388..388), unit in 1/4dB
    INT16   rsrqMeasAdjust;         //range (-124..124), unit in 1/4dB
}CerrcCcmMeasAdjustConfig;

typedef struct CerrcTxPowerThresholdRptCfgTag
{
    //High Tx power report threshold, [-45...23]dbm
    //If value is 127, phy will treat it invalid
    INT8                            highTxTheshold;
    //Low Tx power report threshold, [-45...23]dbm
    //If value is 127, phy will treat it invalid
    INT8                            lowTxTheshold;
}CerrcTxPowerThresholdRptCfg;

/*
* used for AT+ECCFG = 'RrcBarTime', rrcBarEvent, rrcBarTime
*/
typedef struct CerrRrcBarTimeCfgTag
{
    //Used for cell barTime when RRC T300 expire
    UINT16              barTimeT300Expire;
    /*1> If barTimeRLF is 0:
     *   UE will not bar cell if RLF happens in Connection state,
     *   it will still trigger non-standard RLF when PHY unreliable is reported.
     *2> If barTimeRLF is 1 ~ 0xFFFD:
     *   UE will bar cell if RLF happens with configured barTime and trigger non-standard RLF when PHY unreliable is reported.
     *3> If barTimeRLF is 0xFFFE:
     *   UE will ingore PHY unreliable report in Connection state,
     *   it will still bar cell if normal RLF happens with default barTime.
     *4> If barTimeRLF is 0xFFFF:
     *   UE will not bar cell if normal RLF happens and ingore PHY unreliable report in Connection state. */
    UINT16              barTimeRLF;
    //Used for cell barTime where reception of MIB/SIB1 fail(CRC, decode) during cell reselection
    UINT16              barTimeReselCrcErr;
    //Used for cell barTime when receive 'unreliable' indication, and meanwhile UE has not yet collected SIB3 or SIB5 in QualityFirst mode
    UINT16              barTimeIdleUnreliable;
}CerrRrcBarTimeCfg;

/*
 * As Release, start from Rel-13
*/
typedef UINT8 AsReleaseType_t;
typedef enum _EPAT_AsReleaseType_Tag
{
    AsRelease_9 = 9,    //rel 9 is only used in RRC internal code (fill UeCapabilityInformation with rel9 if AT+ECCFG="relVesion",9)
    AsRelease_13 = 13,
    AsRelease_14,
    AsRelease_15,
}AsReleaseType;


/*
 * CERRC_CFG_UE_POWER_CLASS_SETTING
*/
typedef struct PsCfgConfigedUePowerClassTag
{
    //now support to 255
    UINT8                                   freqBandIndicator;
    /*refer to TS36.101 table 6.2.2F-1,
      -- 3 represents Class3(maxpower: 23dBm)
      -- 5 represents Class5 (maxpower: 20dBm)
      -- 6 represents Class6 (maxpower: 14dBm) and only apply for R14 and later */
    UINT8                                   uePowerClass;
}PsCfgConfigedUePowerClass;

typedef struct PsCfgCerrcUePowerClassSetting_Tag
{
    UINT8           numOfBand;
    UINT8           rsvd0;
    UINT16          rsvd1;
    PsCfgConfigedUePowerClass    bandAndPowerClass[RRC_PHY_SUPPORT_BAND_NUM];
}PsCfgCerrcUePowerClassSetting;     // 68 bytes

typedef struct CerrcDuplexPreferCfg_Tag
{
    UINT8           preferMode;     //CmiDuplexPreferMode, 0 - disable, 1 - FDD prefer, 2 - TDD prefer
    UINT8           csScoreAdd;     //range [0-100], PHY will add the score to FDD/TDD frequency point score during cell search procedure
    UINT8           rsrpOffset;     //rsrp offset to all cells for the corresponding duplex mode
                                    //(e.g. if current duplex prefer is FDD, all FDD cells apply with the rsrp offset value).
    UINT8           snrOffset;      //snr offset to all cells for the corresponding duplex mode
                                    //(e.g. if current duplex prefer is FDD, all FDD cells apply with the snr offset value).
}CerrcDuplexPreferCfg;

#endif

