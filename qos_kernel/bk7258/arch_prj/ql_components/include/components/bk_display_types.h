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

#include <driver/lcd_types.h>
#include <components/avdk_utils/avdk_types.h>
#include <components/avdk_utils/avdk_check.h>
#include <components/avdk_utils/avdk_error.h>

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************
 * component name: bk_display
 * description: Public API (open interface)
 *******************************************************************/


/**
 * @brief RGB display controller configuration structure
 * 
 * Used to configure LCD display controller with RGB interface,
 * including device name, LCD device information and SPI IO configuration
 */
typedef struct
{
    const lcd_device_t * lcd_device;  /**< LCD device configuration structure pointer, containing display parameters and initialization commands */
    int8_t clk_pin;                /**<  spi clk io pin */
    int8_t cs_pin;                 /**< cs io pin */
    int8_t sda_pin;                /**<  sda io pin */
    int8_t rst_pin;                /**<  lcd reset io pin */
} bk_display_rgb_ctlr_config_t;

/**
 * @brief MCU display controller configuration structure
 * 
 * Used to configure LCD display controller with MCU interface,
 * including device name and LCD device information
 */
typedef struct
{
    const lcd_device_t * lcd_device;  /**< LCD device configuration structure pointer, containing display parameters and initialization commands */
    int8_t te_pin;                /**<  lcd te io pin */
} bk_display_mcu_ctlr_config_t;

typedef struct
{
    const lcd_device_t *lcd_device; /**< LCD device configuration structure pointer, containing display parameters and initialization commands */
    uint8_t qspi_id;                /**< qspi_id */
    uint8_t reset_pin;              /**< qspi reset io pin */
    uint8_t te_pin;                 /**< qspi te io pin */
} bk_display_qspi_ctlr_config_t;

typedef struct
{
    const lcd_device_t *lcd_device; /**<  LCD device configuration structure pointer, containing display parameters and initialization commands */
    uint8_t spi_id;                 /**< spi_id */
    uint8_t dc_pin;                 /**< spi dc io pin */
    uint8_t reset_pin;              /**< spi reset io pin */
    uint8_t te_pin;                 /**< spi te io pin */
} bk_display_spi_ctlr_config_t;

typedef struct
{
    bk_display_qspi_ctlr_config_t lcd0_config;
    bk_display_qspi_ctlr_config_t lcd1_config;
} bk_display_dual_qspi_ctlr_config_t;

typedef struct
{
    bk_display_spi_ctlr_config_t lcd0_config;
    bk_display_spi_ctlr_config_t lcd1_config;
} bk_display_dual_spi_ctlr_config_t;

/**
 * @brief Display controller interface structure
 * 
 * Defines the interface functions that the display controller must implement,
 * including open, close, flush, delete and IO control
 */
typedef struct bk_display_ctlr *bk_display_ctlr_handle_t;
typedef struct bk_display_ctlr
{
    avdk_err_t (*open)(bk_display_ctlr_handle_t controller);
    avdk_err_t (*close)(bk_display_ctlr_handle_t controller);
    avdk_err_t (*flush)(bk_display_ctlr_handle_t controller, frame_buffer_t *frame, bk_err_t (*free_t)(void *args));
    avdk_err_t (*delete)(bk_display_ctlr_handle_t controller);
    avdk_err_t (*ioctl)(bk_display_ctlr_handle_t controller, uint32_t cmd, void *arg); 
} bk_display_ctlr_t;

#ifdef __cplusplus
}
#endif


