// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <common/sys_config.h>
#include <sdkconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_CLI_USER_CONFIG
#include "cli_user_config.h"
#else
#if (CONFIG_WIFI_CLI_ENABLE)
#define CLI_CFG_WIFI        1
#else
#define CLI_CFG_WIFI        0
#endif //#if (CONFIG_WIFI_ENABLE)

#define CLI_CFG_BLE         1

#if (CONFIG_BK_NETIF)
#define CLI_CFG_NETIF       1
#else
#define CLI_CFG_NETIF       0
#endif //#if (CONFIG_LWIP)

#define CLI_CFG_MISC        1
#define CLI_CFG_MEM         1
#define CLI_CFG_DWT         1
#define CLI_CFG_FPB         1

#if (CONFIG_WIFI_CLI_ENABLE)
#define CLI_CFG_PHY         0 //1
#else
#define CLI_CFG_PHY         0
#endif //#if (CONFIG_WIFI_ENABLE)

#if (CONFIG_STA_PS)
#define CLI_CFG_PWR         1
#else
#define CLI_CFG_PWR         0
#endif
#if (CONFIG_TIMER)
#define CLI_CFG_TIMER       1
#else
#define CLI_CFG_TIMER       0
#endif
#if (CONFIG_INT_WDT)
#define CLI_CFG_WDT         1
#endif
#if (CONFIG_TRNG_SUPPORT)
#define CLI_CFG_TRNG        1
#else
#define CLI_CFG_TRNG        0
#endif

#define CLI_CFG_GPIO        1

#define CLI_CFG_OS          1
#if ((CONFIG_OTA_TFTP) || (CONFIG_OTA_HTTP))
#define CLI_CFG_OTA         1
#else
#define CLI_CFG_OTA         0
#endif
#if(CONFIG_KEYVALUE)
#define CLI_CFG_KEYVALUE    1
#else
#define CLI_CFG_KEYVALUE    0
#endif
#if(CONFIG_SUPPORT_MATTER)
#define CLI_CFG_MATTER      1
#else
#define CLI_CFG_MATTER      0
#endif


#define CLI_CFG_UART        1
#define CLI_CFG_ADC         0
#define CLI_CFG_SPI         1
#define CLI_CFG_MICO        1
#define CLI_CFG_REG         1
#define CLI_CFG_EXCEPTION   1


#if(CONFIG_GENERAL_DMA)
#define CLI_CFG_DMA         1
#else
#define CLI_CFG_DMA         0
#endif

#if(CONFIG_PWM)
#define CLI_CFG_PWM         1
#else
#define CLI_CFG_PWM         0
#endif

#if(CONFIG_FLASH)
#define CLI_CFG_FLASH       1
#else
#define CLI_CFG_FLASH       0
#endif

#if(CONFIG_SDIO_HOST)
#define CLI_CFG_SDIO_HOST   1
#else
#define CLI_CFG_SDIO_HOST   0
#endif

#if(CONFIG_SDIO_SLAVE)
#define CLI_CFG_SDIO_SLAVE   1
#else
#define CLI_CFG_SDIO_SLAVE   0
#endif

#if (CONFIG_I2C)
#define CLI_CFG_I2C         1
#else
#define CLI_CFG_I2C         0
#endif

#if (CONFIG_QSPI)
#define CLI_CFG_QSPI        1
#else
#define CLI_CFG_QSPI        0
#endif

#if (CONFIG_AON_RTC)
#define CLI_CFG_AON_RTC     1
#else
#define CLI_CFG_AON_RTC     0
#endif

#if (CONFIG_CALENDAR)
#define CLI_CFG_CALENDAR    1
#else
#define CLI_CFG_CALENDAR    0
#endif

//TODO default to 0
#define CLI_CFG_EVENT       1

#if (CONFIG_TEMP_DETECT)
#define CLI_CFG_TEMP_DETECT 1
#else
#define CLI_CFG_TEMP_DETECT 0
#endif

#if (CONFIG_SDCARD)
#define CLI_CFG_SD          1
#else
#define CLI_CFG_SD          0
#endif

#if (CONFIG_FATFS)
#define CLI_FATFS          1
#else

#if (CONFIG_FATFS && (CONFIG_JPEG_SW_ENCODER_TEST || CONFIG_H264_SW_DECODER_TEST))
#define CLI_FATFS          1
#else
#define CLI_FATFS          0
#endif

#endif

#if (CONFIG_VFS_TEST)
#define CLI_CFG_VFS          1
#else
#define CLI_CFG_VFS          0
#endif

#if (CONFIG_AIRKISS_TEST)
#define CLI_CFG_AIRKISS     1
#else
#define CLI_CFG_AIRKISS     0
#endif

#if (CONFIG_IPERF_TEST)
#define CLI_CFG_IPERF       1
#else
#define CLI_CFG_IPERF       0
#endif

#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
#if (CONFIG_TOUCH && CONFIG_TOUCH_TEST)
#define CLI_CFG_TOUCH    1
#else
#define CLI_CFG_TOUCH    0
#endif
#endif

#if (CONFIG_PSRAM_TEST)
#define CLI_CFG_PSRAM        1
#else
#define CLI_CFG_PSRAM        0
#endif

#if (CONFIG_GET_UID_TEST)
#define CLI_CFG_UID        1
#else
#define CLI_CFG_UID        0
#endif

#if (CONFIG_BUTTON || CONFIG_ADC_KEY)
#define CLI_CFG_KEY_DEMO     1
#else
#define CLI_CFG_KEY_DEMO     0
#endif

#if (CONFIG_LIN)
#define CLI_CFG_LIN        1
#else
#define CLI_CFG_LIN        0
#endif

#if (CONFIG_SCR)
#define CLI_CFG_SCR      1
#else
#define CLI_CFG_SCR      0
#endif

#if (CONFIG_JPEG_SW_ENCODER_TEST)
#define CLI_CFG_JPEG_SW_ENC      1
#else
#define CLI_CFG_JPEG_SW_ENC      0
#endif

#endif
#ifdef __cplusplus
}
#endif
