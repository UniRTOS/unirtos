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
#include <driver/spi.h>


#ifdef __cplusplus
extern "C" {
#endif

void bk_lcd_spi_send_cmd(uint8_t id, uint8_t cmd);

void bk_lcd_spi_send_data(uint8_t id, uint8_t *data, uint32_t data_len);

#if (CONFIG_LCD_SPI_REFRESH_WITH_QSPI_MAPPING_MODE)
void bk_lcd_spi_send_data_with_qspi_mapping_mode(uint8_t id, uint8_t *data, uint32_t data_len);

bk_err_t bk_lcd_spi_wait_display_complete(qspi_id_t qspi_id);
#endif

void bk_lcd_spi_init(uint8_t id, const lcd_device_t *device, uint8_t reset_pin, uint8_t dc_pin);

void bk_lcd_spi_deinit(uint8_t id, uint8_t reset_pin, uint8_t dc_pin);

bk_err_t bk_lcd_spi_frame_display(uint8_t id, uint8_t *data, uint32_t data_len);

bk_err_t bk_lcd_spi_partial_display(uint8_t id, lcd_display_area_t *area, uint8_t *data);

#ifdef __cplusplus
}
#endif

