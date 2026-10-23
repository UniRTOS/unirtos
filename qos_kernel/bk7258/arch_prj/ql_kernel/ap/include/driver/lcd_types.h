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

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "driver/lcd_qspi_types.h"

#define  USE_LCD_REGISTER_CALLBACKS  1

typedef void (*lcd_isr_t)(void * args);

typedef enum {
	LCD_DEVICE_UNKNOW,
	LCD_DEVICE_ST7282,  /**< 480X270 RGB  */
	LCD_DEVICE_HX8282,  /**< 1024X600 RGB  */
	LCD_DEVICE_GC9503V, /**< 480X800 RGB  */
	LCD_DEVICE_NT35510, /**< 480X854 RGB  */
	LCD_DEVICE_H050IWV, /**< 800X480 RGB  */
	LCD_DEVICE_MD0430R, /**< 800X480 RGB  */
	LCD_DEVICE_MD0700R, /**< 1024X600 RGB  */
	LCD_DEVICE_ST7701S_LY, /**< 480X480 RGB  */
	LCD_DEVICE_ST7701S, /**< 480X480 RGB  */
	LCD_DEVICE_ST7701SN, /**< 480X480 RGB  */
	LCD_DEVICE_AML01,   /**< 720X1280 RGB  */

	LCD_DEVICE_ST7796S, /**< 320X480 MCU  */
	LCD_DEVICE_NT35512, /**< 480X800 MCU  */
	LCD_DEVICE_NT35510_MCU, /**< 480X800 MCU  */
	LCD_DEVICE_ST7789V, /**< 170X320 MCU  */
	LCD_DEVICE_ST7789T3, /**< 240X320 MCU  */

	LCD_DEVICE_SH8601A, /**< 454X454 QSPI  */
	LCD_DEVICE_ST77903_WX20114, /**< 400X400 QSPI  */
	LCD_DEVICE_ST77903_SAT61478M, /**< 400X400 QSPI  */
	LCD_DEVICE_ST77903_H0165Y008T, /**< 360X480 QSPI  */
	LCD_DEVICE_SPD2010, /**< 412X412 QSPI  */
	LCD_DEVICE_GC9C01, /**< 360X360 QSPI  */
	LCD_DEVICE_JD9855, /**< 360X360 QSPI  */
	LCD_DEVICE_JD9855_K18XJ15, /**< 360X360 QSPI  */
	LCD_DEVICE_ST77916, /**< 360X360 QSPI  */
	LCD_DEVICE_JD9853A, /**< 240X320 QSPI  */

	LCD_DEVICE_ST7796U, /**< 320X480 SPI */
	LCD_DEVICE_GC9D01, /**< 160X160 SPI */
	LCD_DEVICE_ST7789V2, /**< 240X320 SPI */
} lcd_device_id_t;

typedef enum {
	LCD_TYPE_RGB,     /**< lcd hardware interface is parallel RGB interface */
	LCD_TYPE_RGB565 = LCD_TYPE_RGB,  /**< lcd device output data hardware interface is RGB565 format */
	LCD_TYPE_MCU8080, /**< lcd device output data hardware interface is MCU 8BIT format */
	LCD_TYPE_QSPI,    /**< lcd device hardware interface is QSPI interface */
	LCD_TYPE_SPI,     /**< lcd device hardware interface is SPI interface */
} lcd_type_t;

typedef enum {
	RGB_OUTPUT_EOF =1 << 5 ,	/**< reg end of frame int status*/
	RGB_OUTPUT_SOF =1 << 4, 	 /**< reg display output start of frame status*/
	I8080_OUTPUT_SOF =1 << 6,	/**< 8080 display output start of frame status */
	I8080_OUTPUT_EOF = 1 << 7,	 /**< 8080 display output end of frame status */
	DE_INT = 1 << 8,             /* de signal discontious interrupt status */
	FRAME_INTERVAL_INT = 1 <<10,
} lcd_int_type_t;


typedef enum {
	DCLK_UNIT = 0,
	HSYNC_UNIT,
	VSYNC_UNIT,
	NONE,
} frame_delay_unit_t;

/** rgb lcd clk select, infulence pfs, user should select according to lcd device spec*/
typedef enum {
    LCD_80M,
    LCD_60M,
    LCD_54M,
    LCD_40M,
    LCD_32M,
    LCD_30M,
    LCD_26M, //26.6M
    LCD_22M, //22.85M
    LCD_20M,
    LCD_17M, //17.1M
    LCD_15M,
    LCD_12M,
    LCD_10M,
    LCD_9M,  //9.2M
    LCD_8M,
} lcd_clk_t;

typedef enum {
    LCD_QSPI_80M = 80,
    LCD_QSPI_64M = 64,
    LCD_QSPI_60M = 60,
    LCD_QSPI_53M = 53, //53.3M
    LCD_QSPI_48M = 48,
    LCD_QSPI_40M = 40,
    LCD_QSPI_32M = 32,
    LCD_QSPI_30M = 30,
} lcd_qspi_clk_t;

/** rgb data output in clk rising or falling */
typedef enum {
	POSEDGE_OUTPUT = 0,    /**< output in clk falling*/
	NEGEDGE_OUTPUT,        /**< output in clk rising*/
} rgb_out_clk_edge_t;

typedef enum {
	ARGB8888 = 0, /**< ARGB8888 DMA2D color mode */
	RGB888,       /**< RGB888 DMA2D color mode   */
	RGB565,       /**< RGB565 DMA2D color mode   */
	YUYV,
} data_format_t;

/** rgb interface config param */
typedef struct
{
	lcd_clk_t clk;                         /**< config lcd clk */
	rgb_out_clk_edge_t data_out_clk_edge;  /**< rgb data output in clk edge, should refer lcd device spec*/

	uint16_t hsync_back_porch;            /**< rang 0~0x3FF (0~1023), should refer lcd device spec*/
	uint16_t hsync_front_porch;           /**< rang 0~0x3FF (0~1023), should refer lcd device spec*/
	uint16_t vsync_back_porch;            /**< rang 0~0xFF (0~255), should refer lcd device spec*/
	uint16_t vsync_front_porch;           /**< rang 0~0xFF (0~255), should refer lcd device spec*/
	uint8_t hsync_pulse_width;            /**< rang 0~0x3F (0~7), should refer lcd device spec*/
	uint8_t vsync_pulse_width;            /**< rang 0~0x3F (0~7), should refer lcd device spec*/
} lcd_rgb_t;

/** mcu interface config param */
typedef struct
{
	lcd_clk_t clk; /**< config lcd clk */
	bk_err_t (*set_xy_swap)(const void *handle, bool swap_axes); 
	bk_err_t (*set_mirror)(const void *handle, bool mirror_x, bool mirror_y);
	void (*set_display_area)(const void *handle, uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye);
	void (*start_transfer)(const void *handle); 
	void (*continue_transfer)(const void *handle); 
} lcd_mcu_t; 

/** qspi interface config param */
typedef struct
{
	lcd_qspi_clk_t clk;
	lcd_qspi_refresh_method_t refresh_method;
	uint8_t reg_write_cmd;
	uint8_t reg_read_cmd;
	lcd_qspi_write_config_t pixel_write_config;
	lcd_qspi_reg_read_config_t reg_read_config;
	const lcd_qspi_init_cmd_t *init_cmd;
	uint32_t device_init_cmd_len;

	lcd_qspi_refresh_config_by_line_t refresh_config;
	uint32_t frame_len;
} lcd_qspi_t;

/** spi interface config param */
typedef struct
{
	lcd_qspi_clk_t clk;
	const lcd_qspi_init_cmd_t *init_cmd;
	uint32_t device_init_cmd_len;
	uint32_t frame_len;
} lcd_spi_t;

typedef struct
{
    int id;           /**< lcd device type, user can add if you want to add another lcd device */
    char *name;       /**< lcd device name */
    uint8_t type;     /**< lcd device hw interface */
    uint16_t width;   /**< lcd device width */
    uint16_t height;  /**< lcd device height */
    uint8_t src_fmt;  /**< source data format: input to display module data format(rgb565/rgb888/yuv) */
    uint8_t out_fmt;  /**< display module output data format(rgb565/rgb666/rgb888), input to lcd device, 
                                Color_depth, 24bit/pixel:RGB888,18bit/pixel:RGB666,16bit/pixel:RGB565 */
    union {
        const lcd_rgb_t *rgb;  /**< RGB interface lcd device config */
        const lcd_mcu_t *mcu;  /**< MCU interface lcd device config */
        const lcd_qspi_t *qspi;/**< QSPI interface lcd device config */
        const lcd_spi_t *spi;  /**< SPI interface lcd device config */
    };

    bk_err_t (*init)(const void *handle);   /**< lcd device initial function by handle */
    bk_err_t (*off)(const void *handle);    /**< lcd off by handle */
} lcd_device_t;

typedef struct {
    uint16_t x_start;
    uint16_t y_start;
    uint16_t x_end;
    uint16_t y_end;
} lcd_display_area_t;



typedef struct bk_lcd_i80_handle
{
    bk_err_t (*write_cmd)(uint32_t command, uint32_t *param, uint8_t param_count);
    bk_err_t (*delete)(struct bk_lcd_i80_handle *handle);
}bk_lcd_i80_handle_t;

bk_lcd_i80_handle_t * lcd_i80_bus_io_register(void *io);


/*
 * @}
 */

#ifdef __cplusplus
}
#endif


