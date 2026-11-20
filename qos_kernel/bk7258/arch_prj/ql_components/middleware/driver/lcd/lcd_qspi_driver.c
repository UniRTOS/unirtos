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
#include <os/mem.h>
#include <os/str.h>
#include <driver/qspi.h>
#include <driver/qspi_types.h>
#include <driver/lcd_types.h>
#include <driver/lcd_qspi.h>
#include <driver/lcd_qspi_types.h>
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "bk_misc.h"
#include <driver/dma.h>
#include "bk_general_dma.h"
 

#define LCD_QSPI_TAG "lcd_qspi_drv"

#define LCD_QSPI_LOGI(...) BK_LOGI(LCD_QSPI_TAG, ##__VA_ARGS__)
#define LCD_QSPI_LOGW(...) BK_LOGW(LCD_QSPI_TAG, ##__VA_ARGS__)
#define LCD_QSPI_LOGE(...) BK_LOGE(LCD_QSPI_TAG, ##__VA_ARGS__)
#define LCD_QSPI_LOGD(...) BK_LOGD(LCD_QSPI_TAG, ##__VA_ARGS__)

#define LCD_QSPI_DEVICE_CASET        0x2A
#define LCD_QSPI_DEVICE_RASET        0x2B

static void lcd_qspi0_dma_finish_isr(dma_id_t dma_id);
static void lcd_qspi1_dma_finish_isr(dma_id_t dma_id);

static qspi_driver_t s_lcd_qspi[SOC_QSPI_UNIT_NUM] = {
    {
        .hal.hw = (qspi_hw_t *)(SOC_QSPI0_REG_BASE),
    },
#if (SOC_QSPI_UNIT_NUM > 1)
    {
        .hal.hw = (qspi_hw_t *)(SOC_QSPI1_REG_BASE),
    }
#endif
};

static lcd_qspi_disp_t s_qspi_disp[SOC_QSPI_UNIT_NUM] = {
    {
        .dma_finish_isr = lcd_qspi0_dma_finish_isr,
        .dma_is_repeat_mode = false,
        .lcd_qspi_is_init = false,
        .qspi_data = LCD_QSPI0_DATA_ADDR,
    },
    {
        .dma_finish_isr = lcd_qspi1_dma_finish_isr,
        .dma_is_repeat_mode = false,
        .lcd_qspi_is_init = false,
        .qspi_data = LCD_QSPI1_DATA_ADDR,
    },
};

static void lcd_qspi_dma_finish_handler(qspi_id_t qspi_id, dma_id_t dma_id, bool dma_is_repeat_mode)
{
    bk_err_t ret = BK_OK;
    uint32_t value = 0;

    if (dma_is_repeat_mode) {
        value = bk_dma_get_repeat_wr_pause(dma_id);
        if (value) {
            bk_dma_stop(dma_id);

            ret = rtos_set_semaphore(&s_qspi_disp[qspi_id].dma_sema);
            if (ret != BK_OK) {
                LCD_QSPI_LOGE("lcd qspi dma semaphore set failed\r\n");
                return;
            }
        }
    } else {
        ret = rtos_set_semaphore(&s_qspi_disp[qspi_id].dma_sema);
        if (ret != BK_OK) {
            LCD_QSPI_LOGE("lcd qspi dma semaphore set failed\r\n");
            return;
        }
    }
}

static void lcd_qspi0_dma_finish_isr(dma_id_t dma_id)
{
    lcd_qspi_dma_finish_handler(QSPI_ID_0, dma_id, s_qspi_disp[0].dma_is_repeat_mode);
}

static void lcd_qspi1_dma_finish_isr(dma_id_t dma_id)
{
    lcd_qspi_dma_finish_handler(QSPI_ID_1, dma_id, s_qspi_disp[1].dma_is_repeat_mode);
}

static bk_err_t lcd_qspi_driver_init(qspi_id_t qspi_id, lcd_qspi_clk_t clk)
{
    qspi_config_t lcd_qspi_config;
    os_memset(&lcd_qspi_config, 0, sizeof(lcd_qspi_config));

    s_lcd_qspi[qspi_id].hal.id = qspi_id;
    qspi_hal_init(&s_lcd_qspi[qspi_id].hal);

    switch (clk) {
        case LCD_QSPI_80M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 5;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_64M:
            lcd_qspi_config.src_clk = QSPI_SCLK_320M;
            lcd_qspi_config.src_clk_div = 4;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_60M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 7;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_53M:
        lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 8;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_48M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 9;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_40M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 11;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_32M:
            lcd_qspi_config.src_clk = QSPI_SCLK_320M;
            lcd_qspi_config.src_clk_div = 9;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_30M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 15;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        default:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 11;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;
    }

    qspi_hal_set_cmd_a_cfg2(&s_lcd_qspi[qspi_id].hal, 0x80008000);

    return BK_OK;
}

static bk_err_t lcd_qspi_hardware_reset(gpio_id_t reset_pin)
{
    gpio_config_t cfg = {0};
    gpio_id_t gpio_id = reset_pin;

    BK_LOG_ON_ERR(gpio_dev_unmap(gpio_id));
    cfg.io_mode = GPIO_OUTPUT_ENABLE;
    cfg.pull_mode = GPIO_PULL_DISABLE;
    cfg.func_mode = GPIO_SECOND_FUNC_DISABLE;
    BK_LOG_ON_ERR(bk_gpio_set_config(gpio_id, &cfg));
    BK_LOG_ON_ERR(bk_gpio_set_output_high(gpio_id));
    rtos_delay_milliseconds(10);
    BK_LOG_ON_ERR(bk_gpio_set_output_low(gpio_id));
    rtos_delay_milliseconds(10);
    BK_LOG_ON_ERR(bk_gpio_set_output_high(gpio_id));
    rtos_delay_milliseconds(120);

    return BK_OK;
}

static bk_err_t lcd_qspi_reg_data_convert(qspi_id_t qspi_id, uint8_t *data, uint32_t data_len)
{
    uint32_t data_buffer[data_len];
    uint8_t i, j, data_tmp1;
    uint8_t data_tmp2[4];

    bk_qspi_read(qspi_id, data_buffer, data_len * 4);
    for (i = 0; i < data_len; i++) {
        for (j = 0; j < 4; j++) {
            data_tmp1 = (data_buffer[i] >> ((j * 8) + 4)) & 0x1;
            data_tmp2[j] = (data_tmp1 << 1) | ((data_buffer[i] >> (j * 8)) & 0x1);
        }

        data[i] = (data_tmp2[0] << 6) | (data_tmp2[1] << 4) | (data_tmp2[2] << 2) | (data_tmp2[3]);
    }

    return BK_OK;
}

static bk_err_t lcd_qspi_refresh_by_line_lcd_head_config(qspi_id_t qspi_id, const lcd_device_t *device)
{
    uint8_t *cmd = NULL;
    uint32_t head_cmd[4];
    uint8_t i;

    cmd = device->qspi->pixel_write_config.cmd;
    for (i = 0; i < device->qspi->pixel_write_config.cmd_len; i++) {
        if (0 == cmd[i]) {
            head_cmd[i] = 0x0;
            continue;
        } else {
            uint8_t cmd_temp = cmd[i];
            cmd_temp = ((cmd_temp >> 4) & 0x0F) | ((cmd_temp << 4) & 0xF0);
            cmd_temp = ((cmd_temp >> 2) & 0x33) | ((cmd_temp << 2) & 0xCC);
            cmd_temp = ((cmd_temp >> 1) & 0x55) | ((cmd_temp << 1) & 0xAA);

            head_cmd[i] = ((cmd_temp << 21) & 0x10000000) | ((cmd_temp << 18) & 0x01000000) |
                        ((cmd_temp << 15) & 0x00100000) | ((cmd_temp << 12) & 0x00010000) |
                        ((cmd_temp << 9) & 0x00001000) | ((cmd_temp << 6) & 0x00000100) |
                        ((cmd_temp << 3) & 0x00000010) | (cmd_temp & 0x00000001);
        }
    }

    qspi_hal_set_lcd_head_cmd0(&s_lcd_qspi[qspi_id].hal, head_cmd[0]);
    qspi_hal_set_lcd_head_cmd1(&s_lcd_qspi[qspi_id].hal, head_cmd[1]);
    qspi_hal_set_lcd_head_cmd2(&s_lcd_qspi[qspi_id].hal, head_cmd[2]);
    qspi_hal_set_lcd_head_cmd3(&s_lcd_qspi[qspi_id].hal, head_cmd[3]);
    qspi_hal_set_lcd_head_resolution(&s_lcd_qspi[qspi_id].hal, device->qspi->refresh_config.line_len * 2, device->height);

    qspi_hal_enable_lcd_head_selection_without_ram(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_set_lcd_head_len(&s_lcd_qspi[qspi_id].hal, 0x20);
    qspi_hal_set_lcd_head_dly(&s_lcd_qspi[qspi_id].hal, (device->qspi->clk >> 1) + 6);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_read_data(qspi_id_t qspi_id, uint8_t *data, const lcd_device_t *device, uint8_t regist_addr, uint8_t data_len)
{
    qspi_hal_set_cmd_d_h(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_d_cfg1(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_d_cfg2(&s_lcd_qspi[qspi_id].hal, 0);

    qspi_hal_set_cmd_d_h(&s_lcd_qspi[qspi_id].hal, (regist_addr << 16 | device->qspi->reg_read_cmd) & 0xFF00FF);
    qspi_hal_set_cmd_d_cfg1(&s_lcd_qspi[qspi_id].hal, 0x300);

    qspi_hal_set_cmd_d_data_line(&s_lcd_qspi[qspi_id].hal, 2);
    qspi_hal_set_cmd_d_data_length(&s_lcd_qspi[qspi_id].hal, data_len * 4);
    qspi_hal_set_cmd_d_dummy_clock(&s_lcd_qspi[qspi_id].hal, device->qspi->reg_read_config.dummy_clk);
    qspi_hal_set_cmd_d_dummy_mode(&s_lcd_qspi[qspi_id].hal, device->qspi->reg_read_config.dummy_mode);

    qspi_hal_cmd_d_start(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_lcd_qspi[qspi_id].hal);

    lcd_qspi_reg_data_convert(qspi_id, data, data_len);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_send_cmd(qspi_id_t qspi_id, uint8_t write_cmd, uint8_t cmd, const uint8_t *data, uint8_t data_len)
{
    qspi_hal_set_cmd_c_l(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_lcd_qspi[qspi_id].hal, 0);

    if (data_len == 0) {
        qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, (cmd << 16 | write_cmd) & 0xFF00FF);
    } else if (data_len > 0 && data_len <= 4) {
        uint32_t value = 0;
        for (uint8_t i = 0; i < data_len; i++) {
            value = value | (data[i] << (i * 8));
        }
        qspi_hal_set_cmd_c_l(&s_lcd_qspi[qspi_id].hal, value);
        qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, (cmd << 16 | write_cmd) & 0xFF00FF);
    } else if (data_len > 4 && data_len != 0xFF) {
        qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, (cmd << 16 | write_cmd) & 0xFF00FF);
        qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0x300);
        qspi_hal_set_cmd_c_cfg2(&s_lcd_qspi[qspi_id].hal, data_len << 2);
        bk_qspi_write(qspi_id, data, data_len);
        qspi_hal_cmd_c_start(&s_lcd_qspi[qspi_id].hal);
        qspi_hal_wait_cmd_done(&s_lcd_qspi[qspi_id].hal);
        qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0);
        qspi_hal_set_cmd_c_cfg2(&s_lcd_qspi[qspi_id].hal, 0);
        return BK_OK;
    }

    qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0x3 << ((data_len + 4) * 2));
    qspi_hal_cmd_c_start(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_lcd_qspi[qspi_id].hal);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_quad_write_start(qspi_id_t qspi_id, lcd_qspi_write_config_t reg_config, bool addr_is_4wire)
{
    uint32_t cmd_c_h = 0;

    qspi_hal_force_spi_cs_low_enable(&s_lcd_qspi[qspi_id].hal);

    qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, 0);

    for (uint8_t i = 0; i < reg_config.cmd_len; i++) {
        cmd_c_h = qspi_hal_get_cmd_c_h(&s_lcd_qspi[qspi_id].hal);
        cmd_c_h |= ((reg_config.cmd[i]) << i * 8);
        qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, cmd_c_h);
    }

    if (addr_is_4wire) {
        qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0x3A8);
    } else {
        qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0x300);
    }
    qspi_hal_cmd_c_start(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_lcd_qspi[qspi_id].hal);

    qspi_hal_io_cpu_mem_select(&s_lcd_qspi[qspi_id].hal, 1);
    qspi_hal_disable_cmd_sck_enable(&s_lcd_qspi[qspi_id].hal);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_quad_write_stop(qspi_id_t qspi_id)
{
    qspi_hal_disable_cmd_sck_disable(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_force_spi_cs_low_disable(&s_lcd_qspi[qspi_id].hal);

    qspi_hal_io_cpu_mem_select(&s_lcd_qspi[qspi_id].hal, 0);

    return BK_OK;
}

static void lcd_qspi_disp_area_config(qspi_id_t qspi_id, const lcd_device_t *device, lcd_display_area_t *area)
{
    uint8_t column_value[4] = {0};
    uint8_t row_value[4] = {0};

    column_value[0] = (area->x_start >> 8) & 0xFF;
    column_value[1] = area->x_start & 0xFF;
    column_value[2] = (area->x_end >> 8) & 0xFF;
    column_value[3] = area->x_end & 0xFF;
    row_value[0] = (area->y_start >> 8) & 0xFF;
    row_value[1] = area->y_start & 0xFF;
    row_value[2] = (area->y_end >> 8) & 0xFF;
    row_value[3] = area->y_end & 0xFF;

    bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, LCD_QSPI_DEVICE_CASET, column_value, 4);
    bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, LCD_QSPI_DEVICE_RASET, row_value, 4);
}

static bk_err_t lcd_qspi_get_dma_repeat_once_len(uint32_t data_len)
{
    uint32_t len = 0;
    uint32_t value = 0;
    uint8_t i = 0;

    for (i = 2; i < 25; i++) {
        len = data_len / i;
        if (len <= 0x10000) {
            value = data_len % i;
            if (!value) {
                return len;
            }
        }
    }
    LCD_QSPI_LOGE("%s Error dma length, please check the data_len parameter\r\n", __func__);

    return len;
}

static void lcd_qspi_dma_init(qspi_id_t qspi_id)
{
    bk_err_t ret = BK_OK;

    ret = bk_dma_driver_init();
    if (ret != BK_OK) {
        LCD_QSPI_LOGE("dma driver init failed!\r\n");
        return;
    }

    ret = rtos_init_semaphore(&s_qspi_disp[qspi_id].dma_sema, 1);
    if (ret != kNoErr) {
        LCD_QSPI_LOGE("lcd qspi dma semaphore init failed.\r\n");
        return;
    }

    s_qspi_disp[qspi_id].dma_id = bk_dma_alloc(DMA_DEV_DTCM);
    if ((s_qspi_disp[qspi_id].dma_id < DMA_ID_0) || (s_qspi_disp[qspi_id].dma_id >= DMA_ID_MAX)) {
        LCD_QSPI_LOGE("%s dma id malloc failed!\r\n", __func__);
        return;
    }

#if (CONFIG_SPE)
    bk_dma_set_src_sec_attr(s_qspi_disp[qspi_id].dma_id, DMA_ATTR_SEC);
    bk_dma_set_dest_sec_attr(s_qspi_disp[qspi_id].dma_id, DMA_ATTR_SEC);
    bk_dma_set_dest_burst_len(s_qspi_disp[qspi_id].dma_id, BURST_LEN_INC16);
    bk_dma_set_src_burst_len(s_qspi_disp[qspi_id].dma_id, BURST_LEN_INC16);
#endif
}

static void lcd_qspi_dma_deinit(qspi_id_t qspi_id)
{
    bk_err_t ret = BK_OK;

    bk_dma_stop(s_qspi_disp[qspi_id].dma_id);

    bk_dma_disable_finish_interrupt(s_qspi_disp[qspi_id].dma_id);

    ret = bk_dma_free(DMA_DEV_DTCM, s_qspi_disp[qspi_id].dma_id);
    if (ret != BK_OK) {
        LCD_QSPI_LOGE("%s dma id free failed.\r\n", __func__);
        return;
    }

    ret = rtos_deinit_semaphore(&s_qspi_disp[qspi_id].dma_sema);
    if (ret != kNoErr) {
        LCD_QSPI_LOGE("%s dma semaphore deinit failed.\r\n", __func__);
        return;
    }
}

static void lcd_qspi_dma_single_mode_config(qspi_id_t qspi_id, dma_id_t dma_id, uint32_t *data, uint32_t data_len)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};

    dma_config.mode = DMA_WORK_MODE_SINGLE;
    dma_config.chan_prio = 0;

    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.start_addr = (uint32_t)data;
    dma_config.src.end_addr = (uint32_t)(data + data_len);

    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;

    dma_config.dst.start_addr = s_qspi_disp[qspi_id].qspi_data;
    dma_config.dst.end_addr = s_qspi_disp[qspi_id].qspi_data + data_len;

    ret = bk_dma_init(dma_id, &dma_config);
    if (ret != BK_OK) {
        LCD_QSPI_LOGE("bk_dma_init failed!\r\n");
        return;
    }

    bk_dma_set_transfer_len(dma_id, data_len);
    bk_dma_register_isr(s_qspi_disp[qspi_id].dma_id, NULL, s_qspi_disp[qspi_id].dma_finish_isr);
    bk_dma_enable_finish_interrupt(s_qspi_disp[qspi_id].dma_id);
}

static void lcd_qspi_dma_repeat_mode_config(qspi_id_t qspi_id, dma_id_t dma_id, uint32_t *data, uint32_t data_len)
{
    uint32_t dma_repeat_once_len = 0;

    bk_dma_stateless_judgment_configuration((void *)s_qspi_disp[qspi_id].qspi_data, (void *)data, data_len, dma_id, s_qspi_disp[qspi_id].dma_finish_isr);

    dma_set_src_pause_addr(dma_id, (uint32_t)data + data_len);
    dma_set_dst_pause_addr(dma_id, s_qspi_disp[qspi_id].qspi_data + data_len);

    dma_repeat_once_len = lcd_qspi_get_dma_repeat_once_len(data_len);
    bk_dma_set_transfer_len(dma_id, dma_repeat_once_len);
}

bk_err_t bk_lcd_qspi_init(qspi_id_t qspi_id, const lcd_device_t *device, uint8_t reset_pin)
{
    bk_err_t ret = BK_OK;

    if (device == NULL) {
        LCD_QSPI_LOGE("lcd qspi device not found\r\n");
        return BK_FAIL;
    }

    if (s_qspi_disp[qspi_id].lcd_qspi_is_init) {
        LCD_QSPI_LOGE("lcd qspi init already complete\r\n");
        return BK_OK;
    }

    lcd_qspi_dma_init(qspi_id);

    qspi_hal_disable_soft_reset(&s_lcd_qspi[qspi_id].hal);
    bk_delay_us(10);
    qspi_hal_enable_soft_reset(&s_lcd_qspi[qspi_id].hal);

    lcd_qspi_hardware_reset(reset_pin);

    ret = lcd_qspi_driver_init(qspi_id, device->qspi->clk);
    if (ret != BK_OK) {
        LCD_QSPI_LOGE("lcd qspi driver init failed!\r\n");
        return ret;
    }

    if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_LINE) {
        lcd_qspi_refresh_by_line_lcd_head_config(qspi_id, device);
    } else {
        lcd_display_area_t disp_area = {0};
        disp_area.x_start = 0;
        disp_area.y_start = 0;
        disp_area.x_end = device->width - 1;
        disp_area.y_end = device->height - 1;
        lcd_qspi_disp_area_config(qspi_id, device, &disp_area);
    }

    if (device->qspi->init_cmd != NULL) {
        const lcd_qspi_init_cmd_t *init = device->qspi->init_cmd;
        for (uint32_t i = 0; i < device->qspi->device_init_cmd_len; i++) {
            if (init->data_len == 0xff) {
                rtos_delay_milliseconds(init->data[0]);
            } else {
                bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, init->cmd, init->data, init->data_len);
            }
            init++;
        }
    } else {
        LCD_QSPI_LOGE("lcd qspi device don't init\r\n");
        return BK_FAIL;
    }

    s_qspi_disp[qspi_id].lcd_qspi_is_init = true;

    return BK_OK;
}

bk_err_t bk_lcd_qspi_deinit(qspi_id_t qspi_id, uint8_t reset_pin)
{
    if (s_qspi_disp[qspi_id].lcd_qspi_is_init == false) {
        LCD_QSPI_LOGE("lcd qspi deinit already complete\r\n");
        return BK_OK;
    }

    lcd_qspi_dma_deinit(qspi_id);

    BK_LOG_ON_ERR(bk_qspi_deinit(qspi_id));

    gpio_dev_unmap(reset_pin);

    s_qspi_disp[qspi_id].lcd_qspi_is_init = false;

    return BK_OK;
}

bk_err_t bk_lcd_qspi_wait_display_complete(qspi_id_t qspi_id, const lcd_device_t *device)
{
    bk_err_t ret = BK_OK;

    if (s_qspi_disp[qspi_id].dma_sema) {
        ret = rtos_get_semaphore(&s_qspi_disp[qspi_id].dma_sema, 5000);
        if (ret != kNoErr) {
            LCD_QSPI_LOGE("ret = %d, lcd qspi get semaphore failed!\r\n", ret);
            return BK_FAIL;
        }
        bk_delay_us(10);
        bk_lcd_qspi_quad_write_stop(qspi_id);

        if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_LINE) {
            for (uint16_t i = 0; i < device->qspi->refresh_config.hbp; i++) {
                bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, device->qspi->refresh_config.hsync_cmd, NULL, 0);
                bk_delay_us(40);
            }
        }
    }

    return ret;
}

bk_err_t bk_lcd_qspi_frame_display(qspi_id_t qspi_id, const lcd_device_t *device, uint32_t *data, uint32_t data_len)
{
    if (data_len <= 0x10000) {
        s_qspi_disp[qspi_id].dma_is_repeat_mode = false;
        lcd_qspi_dma_single_mode_config(qspi_id, s_qspi_disp[qspi_id].dma_id, data, data_len);
    } else {
        s_qspi_disp[qspi_id].dma_is_repeat_mode = true;
        lcd_qspi_dma_repeat_mode_config(qspi_id, s_qspi_disp[qspi_id].dma_id, data, data_len);
    }

    if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_LINE) {
        for (uint16_t i = 0; i < device->qspi->refresh_config.vsw; i++) {
            bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, device->qspi->refresh_config.vsync_cmd, NULL, 0);
            bk_delay_us(40);
        }

        for (uint16_t i = 0; i < device->qspi->refresh_config.hfp; i++) {
            bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, device->qspi->refresh_config.hsync_cmd, NULL, 0);
            bk_delay_us(40);
        }

        qspi_hal_clear_lcd_head(&s_lcd_qspi[qspi_id].hal, 1);
        qspi_hal_clear_lcd_head(&s_lcd_qspi[qspi_id].hal, 0);
        bk_lcd_qspi_quad_write_start(qspi_id, device->qspi->pixel_write_config, 0);
        bk_dma_start(s_qspi_disp[qspi_id].dma_id);
    } else if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_FRAME) {
        bk_lcd_qspi_quad_write_start(qspi_id, device->qspi->pixel_write_config, 0);
        bk_dma_start(s_qspi_disp[qspi_id].dma_id);
    } else {
        LCD_QSPI_LOGE("invalid lcd qspi refresh method\r\n");
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_lcd_qspi_partial_display(qspi_id_t qspi_id, const lcd_device_t *device, lcd_display_area_t *area, uint32_t *data)
{
    if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_FRAME) {
        lcd_qspi_disp_area_config(qspi_id, device, area);
        lcd_qspi_dma_single_mode_config(qspi_id, s_qspi_disp[qspi_id].dma_id, data, (area->x_end - area->x_start + 1) * (area->y_end - area->y_start + 1) * CONFIG_LCD_QSPI_COLOR_DEPTH_BYTE);
        bk_lcd_qspi_quad_write_start(qspi_id, device->qspi->pixel_write_config, 0);
        bk_dma_start(s_qspi_disp[qspi_id].dma_id);
    } else {
        LCD_QSPI_LOGE("Partial display just support qspi lcd with ram\r\n");
        return BK_FAIL;
    }

    return BK_OK;
}