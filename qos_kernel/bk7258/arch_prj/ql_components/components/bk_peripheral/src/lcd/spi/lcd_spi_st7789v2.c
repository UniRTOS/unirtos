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

#include <common/bk_include.h>
#include <driver/lcd_types.h>


static const lcd_qspi_init_cmd_t st7789v2_init_cmds[] =
{
    {0x00, {0x78}, 0xFF},
    {0x11, {0x00}, 0},
    {0x00, {0x78}, 0xFF},
    {0x36, {0x00}, 1},
    {0x3A, {0x05}, 1},
    {0xB2, {0x3F, 0x3F, 0x00, 0x33, 0x33}, 5},
    {0xB7, {0x35}, 1},
    {0xBB, {0x2A}, 1},
    {0xC0, {0x2C}, 1},
    {0xC2, {0x01}, 1},
    {0xC3, {0x0B}, 1},
    {0xC4, {0x20}, 1},
    {0xC6, {0x1F}, 1},
    {0XD0, {0xA4, 0xA1}, 2},
    {0xE0, {0xD0, 0x01, 0x08, 0x0F, 0x11, 0x2A, 0x36, 0x55, 0x44, 0x3A, 0x0B, 0x06, 0x11, 0x20}, 14},
    {0xE1, {0xD0, 0x02, 0x07, 0x0A, 0x0B, 0x18, 0x34, 0x43, 0x4A, 0x2B, 0x1B, 0x1C, 0x22, 0x1F}, 14},
    // {0xE7, {0x10}, 1},
    {0x35, {0x00}, 0},
    {0x29, {0x00}, 0},
};

static const lcd_spi_t lcd_spi_st7789v2_config =
{
    .clk = LCD_QSPI_64M,
    .init_cmd = st7789v2_init_cmds,
    .device_init_cmd_len = sizeof(st7789v2_init_cmds) / sizeof (lcd_qspi_init_cmd_t),
    .frame_len = 240 * 320 * CONFIG_LCD_SPI_COLOR_DEPTH_BYTE,
};

const lcd_device_t lcd_device_st7789v2 =
{
    .id = LCD_DEVICE_ST7789V2,
    .name = "st7789v2",
    .type = LCD_TYPE_SPI,
    .width = 240,
    .height = 320,
    .spi = &lcd_spi_st7789v2_config,
    .init = NULL,
    .off = NULL,
};
