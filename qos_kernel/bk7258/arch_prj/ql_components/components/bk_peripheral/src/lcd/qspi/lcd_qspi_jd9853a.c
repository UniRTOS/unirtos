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

#define LCD_QSPI_JD9853A_REGISTER_WRITE_COMMAND        0x02
#define LCD_QSPI_JD9853A_REGISTER_READ_COMMAND         0x03

static const lcd_qspi_init_cmd_t jd9853a_init_cmds[] =
{
    {0x01, {0x00}, 0},
    {0X00, {0X0A}, 0xFF},
    {0xDF, {0x98, 0x53}, 2},
    {0xDE, {0x00}, 1},
    {0xB2, {0x25}, 1},
    {0xB7, {0x00, 0x21, 0x00, 0x49}, 4},
    {0xBB, {0x4F, 0x9A, 0x55, 0x73, 0x63, 0xF0}, 6},
    {0xC0, {0x44, 0xA4}, 2},
    {0xC1, {0x12}, 1},
    {0xC3, {0x7D, 0x07, 0x14, 0x06, 0xC8, 0x71, 0x6C, 0x77}, 8},
    {0xC4, {0x00, 0x00, 0xA0, 0x79, 0x0E, 0x0A, 0x16, 0x79, 0x25, 0x0A, 0x16, 0x82}, 12},
    {0xC8, {0x3F, 0x34, 0x2B, 0x20, 0x2A, 0x2C, 0x24, 0x24, 0x21, 0x22, 0x20, 0x15, 0x10, 0x0B, 0x06, 0x00, 0x3F, 0x34, 0x2B, 0x20, 0x2A, 0x2C, 0x24, 0x24, 0x21, 0x22, 0x20, 0x15, 0x10, 0x0B, 0x06, 0x00}, 32},
    {0xD0, {0x04, 0x06, 0x6B, 0x0F, 0x00}, 5},
    {0xD7, {0x00, 0x30}, 2},
    {0xE6, {0x10}, 1},
    {0xDE, {0x01}, 1},
    {0xB7, {0x03, 0x13, 0xEF, 0x35, 0x35}, 5},

    {0xC1, {0x14, 0x15, 0xC0}, 3},
    {0xC2, {0x06, 0x3A, 0xC7}, 3},
    {0xC4, {0x72, 0x12}, 2},
    {0xBE, {0x00}, 1},
    {0xDE, {0x00}, 1},
    {0x35, {0x00}, 1},
    {0x36, {0x00}, 1},
    {0x3A, {0x05}, 1},
    {0x11, {0X00}, 0},
    {0x00, {0X78}, 0xFF},
    {0x29, {0x00}, 0},
    {0x00, {0X14}, 0xFF},
};

static uint8_t jd9853a_cmd[4] = {0x32, 0x00, 0x2c, 0x00};

static const lcd_qspi_t lcd_qspi_jd9853a_config =
{
    .clk = LCD_QSPI_48M,
    .refresh_method = LCD_QSPI_REFRESH_BY_FRAME,
    .reg_write_cmd = LCD_QSPI_JD9853A_REGISTER_WRITE_COMMAND,
    .reg_read_cmd = LCD_QSPI_JD9853A_REGISTER_READ_COMMAND,
    .reg_read_config.dummy_clk = 0,
    .reg_read_config.dummy_mode = LCD_QSPI_NO_INSERT_DUMMMY_CLK,
    .pixel_write_config.cmd = jd9853a_cmd,
    .pixel_write_config.cmd_len = sizeof(jd9853a_cmd),
    .init_cmd = jd9853a_init_cmds,
    .device_init_cmd_len = sizeof(jd9853a_init_cmds) / sizeof(lcd_qspi_init_cmd_t),
    .refresh_config = {0},
    .frame_len = 240 * 320 * CONFIG_LCD_QSPI_COLOR_DEPTH_BYTE,
};

const lcd_device_t lcd_device_jd9853a =
{
    .id = LCD_DEVICE_JD9853A,
    .name = "jd9853a",
    .type = LCD_TYPE_QSPI,
    .width = 240,
    .height = 320,
    .qspi = &lcd_qspi_jd9853a_config,
    .init = NULL,
    .off = NULL,
};
