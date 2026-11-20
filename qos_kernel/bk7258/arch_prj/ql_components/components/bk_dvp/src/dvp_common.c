// Copyright 2023-2024 Beken
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

#include <driver/gpio.h>
#include "sys_driver.h"
#include "gpio_driver.h"

#include <components/dvp_camera_types.h>

#define TAG "dvp_common"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

void dvp_camera_mclk_enable(mclk_freq_t mclk)
{
    gpio_dev_unmap(GPIO_27);
    gpio_dev_map(GPIO_27, GPIO_DEV_CLK_AUXS_CIS);

    switch (mclk)
    {
        case MCLK_15M:
            sys_drv_set_auxs_cis(3, 31);
            break;

        case MCLK_16M:
            sys_drv_set_auxs_cis(3, 29);
            break;

        case MCLK_20M:
            sys_drv_set_auxs_cis(3, 23);
            break;

        default:
        case MCLK_24M:
            sys_drv_set_auxs_cis(3, 19);
            break;

        case MCLK_30M:
            sys_drv_set_auxs_cis(3, 15);
            break;

        case MCLK_32M:
            sys_drv_set_auxs_cis(3, 14);
            break;

        case MCLK_40M:
            sys_drv_set_auxs_cis(3, 11);
            break;

        case MCLK_48M:
            sys_drv_set_auxs_cis(3, 9);
            break;
    }

    sys_drv_set_cis_auxs_clk_en(1);
}

void dvp_camera_mclk_disable(void)
{
    sys_drv_set_cis_auxs_clk_en(0); // ausx_clk disable
}

void dvp_camera_io_init(bk_dvp_io_config_t *io_config)
{
    if (io_config->xclk_pin != 0xFF)
    {
        gpio_dev_unmap(io_config->xclk_pin);
        gpio_dev_map(io_config->xclk_pin, GPIO_DEV_JPEG_MCLK);
    }

    if (io_config->pclk_pin != 0xFF)
    {
        gpio_dev_unmap(io_config->pclk_pin);
        gpio_dev_map(io_config->pclk_pin, GPIO_DEV_JPEG_PCLK);
    }

    if (io_config->vsync_pin != 0xFF)
    {
        gpio_dev_unmap(io_config->vsync_pin);
        gpio_dev_map(io_config->vsync_pin, GPIO_DEV_JPEG_VSYNC);
    }

    if (io_config->hsync_pin != 0xFF)
    {
        gpio_dev_unmap(io_config->hsync_pin);
        gpio_dev_map(io_config->hsync_pin, GPIO_DEV_JPEG_HSYNC);
    }

    for (uint8_t i = 0; i < io_config->data_width; i++)
    {
        if (io_config->data_pin[i] != 0xFF)
        {
            gpio_dev_unmap(io_config->data_pin[i]);
            gpio_dev_map(io_config->data_pin[i], GPIO_DEV_JPEG_PXDATA0 + i);
        }
    }
}

void dvp_camera_io_deinit(bk_dvp_io_config_t *io_config)
{
    if (io_config->xclk_pin != 0xFF)
    {
        gpio_dev_unmap(io_config->xclk_pin);
    }

    if (io_config->pclk_pin != 0xFF)
    {
        gpio_dev_unmap(io_config->pclk_pin);
    }

    if (io_config->vsync_pin != 0xFF)
    {
        gpio_dev_unmap(io_config->vsync_pin);
    }

    if (io_config->hsync_pin != 0xFF)
    {
        gpio_dev_unmap(io_config->hsync_pin);
    }

    for (uint8_t i = 0; i < io_config->data_width; i++)
    {
        if (io_config->data_pin[i] != 0xFF)
        {
            gpio_dev_unmap(io_config->data_pin[i]);
        }
    }
}