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

#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <math.h>
#include "arch_interrupt.h"
#include "lcd_disp_hal.h"
#include <driver/lcd.h>
#include <driver/gpio.h>
#include "gpio_map.h"
#include <driver/int.h>
#include "sys_driver.h"
#include <driver/hal/hal_gpio_types.h>
#include <driver/hal/hal_lcd_types.h>
#include <driver/flash.h>
#include "driver/lcd.h"
#include "driver/pwr_clk.h"
#include "cpu_id.h"
#include "bk_misc.h"
#define TAG "lcd_drv"

#if CONFIG_SOC_BK7256XX
#define MINOOR_ITCM __attribute__((section(".itcm_sec_code ")))
#else
#define MINOOR_ITCM
#endif

#define IO_FUNCTION_ENABLE(pin, func)   \
	do {                                \
		gpio_dev_unmap(pin);            \
		gpio_dev_map(pin, func);        \
		bk_gpio_enable_output(pin); 	\
		bk_gpio_set_capacity(pin,GPIO_DRIVER_CAPACITY_1);	\
	} while (0)

#define IO_FUNCTION_ENABLE_I8080(pin, func)   \
	do {                                \
		gpio_dev_unmap(pin);            \
		gpio_dev_map(pin, func);        \
		bk_gpio_enable_output(pin);     \
		bk_gpio_set_capacity(pin,GPIO_DRIVER_CAPACITY_3);    \
	} while (0)

//set high impedance
#define IO_FUNCTION_UNMAP(pin)   \
        do {                                \
            gpio_dev_unmap(pin);            \
        } while (0)

#define LCD_RETURN_ON_NOT_INIT() do {\
		if (!s_lcd_driver_is_init) {\
			return BK_ERR_LCD_NOT_INIT;\
		}\
	} while(0)
#define BK_RETURN_ON_NULL(_x) do {\
	if (!(_x)) {\
		BK_LOGE(ERR_TAG, "Null %s\n", __FUNCTION__);\
		return BK_ERR_NULL_PARAM;\
	}\
} while(0)

static bool s_lcd_driver_is_init = false;

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


typedef struct
{
	lcd_isr_t lcd_8080_frame_start_handler;
	void *lcd_8080_start_arg;
	lcd_isr_t lcd_8080_frame_end_handler;
	void *lcd_8080_end_arg;
	lcd_isr_t lcd_rgb_frame_end_handler;
	void *lcd_rgb_end_arg;
	lcd_isr_t lcd_rgb_frame_start_handler;
	void *lcd_rgb_start_arg;
	lcd_isr_t lcd_rgb_de_handler;
	void *lcd_rgb_de_arg;
	lcd_isr_t lcd_rgb_frame_interval_handler;
	void *lcd_rgb_interval_arg;
	const lcd_device_t device;  /**< lcd device config */
} lcd_driver_t;

static lcd_driver_t s_lcd = {0};

static const lcd_device_t **devices_list = NULL;
static uint16_t devices_size = 0;
static bk_lcd_i80_handle_t *s_lcd_i80_handle = NULL;  // Global handle to prevent multiple registrations
uint32_t get_lcd_devices_num(void)
{
	return devices_size;
}
const lcd_device_t **get_lcd_devices_list(void)
{
	return devices_list;
}

const lcd_device_t * get_lcd_device_by_name(char * name)
{
	uint32_t i;

	LOGD("%s, devices: %d\n", __func__, devices_size);

	for (i = 0; i < devices_size; i++)
	{
		if (os_strcmp(devices_list[i]->name, name) == 0)
		{
			LOGD("%s, name: %s\n", __func__, devices_list[i]->name);
			return devices_list[i];
		}
	}
	return NULL;
}

const lcd_device_t *get_lcd_device_by_ppi(media_ppi_t ppi)
{
	return NULL;
}

const lcd_device_t *get_lcd_device_by_id(lcd_device_id_t id)
{
	uint32_t i;

	for (i = 0; i < devices_size; i++)
	{
		if (devices_list[i]->id == id)
		{
			return devices_list[i];
		}
	}

	return NULL;
}

void bk_lcd_set_devices_list(const lcd_device_t **list, uint16_t size)
{
	devices_list = list;
	devices_size = size;
}


bk_err_t lcd_mcu_gpio_init(void)
{
	LOGV("%s\n", __func__);
#if 0
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D0_PIN, LCD_MCU_D0_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D1_PIN, LCD_MCU_D1_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D2_PIN, LCD_MCU_D2_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D3_PIN, LCD_MCU_D3_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D4_PIN, LCD_MCU_D4_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D5_PIN, LCD_MCU_D5_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D6_PIN, LCD_MCU_D6_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D7_PIN, LCD_MCU_D7_FUNC);
#if CONFIG_SOC_BK7236XX
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D8_PIN , LCD_MCU_D8_FUNC );
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D9_PIN , LCD_MCU_D9_FUNC );
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D10_PIN, LCD_MCU_D10_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D11_PIN, LCD_MCU_D11_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D12_PIN, LCD_MCU_D12_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D13_PIN, LCD_MCU_D13_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D14_PIN, LCD_MCU_D14_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D15_PIN, LCD_MCU_D15_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D16_PIN, LCD_MCU_D16_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_D17_PIN, LCD_MCU_D17_FUNC);
#endif
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_RDX_PIN, LCD_MCU_RDX_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_WRX_PIN, LCD_MCU_WRX_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_RSX_PIN, LCD_MCU_RSX_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_RESET_PIN, LCD_MCU_RESET_FUNC);
	IO_FUNCTION_ENABLE_I8080(LCD_MCU_CSX_PIN, LCD_MCU_CSX_FUNC);
#endif
	bk_gpio_set_capacity(LCD_MCU_RDX_PIN,GPIO_DRIVER_CAPACITY_3);
	bk_gpio_set_capacity(LCD_MCU_WRX_PIN,GPIO_DRIVER_CAPACITY_3);
	bk_gpio_set_capacity(LCD_MCU_RESET_PIN,GPIO_DRIVER_CAPACITY_3);
	bk_gpio_set_capacity(LCD_MCU_RSX_PIN,GPIO_DRIVER_CAPACITY_3);
	bk_gpio_set_capacity(LCD_MCU_CSX_PIN,GPIO_DRIVER_CAPACITY_3);
	return BK_OK;
}


static bk_err_t lcd_rgb_gpio_init(void)
{
	LOGV("%s\n", __func__);
#if 0
#if CONFIG_SOC_BK7236XX
	IO_FUNCTION_ENABLE(LCD_RGB_R0_PIN, LCD_RGB_R0_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_R1_PIN, LCD_RGB_R1_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_R2_PIN, LCD_RGB_R2_FUNC);
#endif
	IO_FUNCTION_ENABLE(LCD_RGB_R3_PIN, LCD_RGB_R3_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_R4_PIN, LCD_RGB_R4_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_R5_PIN, LCD_RGB_R5_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_R6_PIN, LCD_RGB_R6_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_R7_PIN, LCD_RGB_R7_FUNC);
	
#if CONFIG_SOC_BK7236XX
	IO_FUNCTION_ENABLE(LCD_RGB_G0_PIN, LCD_RGB_G0_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G1_PIN, LCD_RGB_G1_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G2_PIN, LCD_RGB_G2_FUNC);
#endif
	IO_FUNCTION_ENABLE(LCD_RGB_G3_PIN, LCD_RGB_G3_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G4_PIN, LCD_RGB_G4_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G5_PIN, LCD_RGB_G5_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G6_PIN, LCD_RGB_G6_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G7_PIN, LCD_RGB_G7_FUNC);

#if CONFIG_SOC_BK7236XX
	IO_FUNCTION_ENABLE(LCD_RGB_B0_PIN, LCD_RGB_B0_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B1_PIN, LCD_RGB_B1_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B2_PIN, LCD_RGB_B2_FUNC);
	
#endif
	IO_FUNCTION_ENABLE(LCD_RGB_B3_PIN, LCD_RGB_B3_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B4_PIN, LCD_RGB_B4_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B5_PIN, LCD_RGB_B5_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B6_PIN, LCD_RGB_B6_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B7_PIN, LCD_RGB_B7_FUNC);

	IO_FUNCTION_ENABLE(LCD_RGB_CLK_PIN, LCD_RGB_CLK_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_DISP_PIN, LCD_RGB_DISP_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_HSYNC_PIN, LCD_RGB_HSYNC_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_VSYNC_PIN, LCD_RGB_VSYNC_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_DE_PIN, LCD_RGB_DE_FUNC);
#endif
	return BK_OK;
}

#if (CONFIG_SOC_BK7236XX)
__attribute__((section(".iram"))) void lcd_isr();
#else
__attribute__((section(".itcm_sec_code"))) void lcd_isr();
#endif

bk_err_t bk_lcd_isr_register(lcd_int_type_t int_type, lcd_isr_t isr, void *args)
{
	// Add parameter validation
	if (isr == NULL)
	{
		LOGE("%s: ISR handler is NULL\n", __func__);
		return BK_ERR_NULL_PARAM;
	}

	// Validate interrupt type
	if (int_type != I8080_OUTPUT_SOF && int_type != I8080_OUTPUT_EOF &&
		int_type != RGB_OUTPUT_SOF && int_type != RGB_OUTPUT_EOF &&
		int_type != DE_INT && int_type != FRAME_INTERVAL_INT)
	{
		LOGE("%s: invalid interrupt type: %d\n", __func__, int_type);
		return BK_FAIL;
	}

	if (int_type == I8080_OUTPUT_SOF)
	{
		s_lcd.lcd_8080_frame_start_handler = isr;
		s_lcd.lcd_8080_start_arg = args;
	}
	else if (int_type == I8080_OUTPUT_EOF)
	{
		s_lcd.lcd_8080_frame_end_handler = isr;
		s_lcd.lcd_8080_end_arg = args;
	}
	else if (int_type == RGB_OUTPUT_SOF)
	{
		s_lcd.lcd_rgb_frame_start_handler = isr;
		s_lcd.lcd_rgb_start_arg = args;
	}
	else if (int_type == RGB_OUTPUT_EOF)
	{
		s_lcd.lcd_rgb_frame_end_handler = isr;
		s_lcd.lcd_rgb_end_arg = args;
	}
	else if (int_type == DE_INT)
	{
		s_lcd.lcd_rgb_de_handler = isr;
		s_lcd.lcd_rgb_de_arg = args;
	}
	else if (int_type == FRAME_INTERVAL_INT)
	{
		s_lcd.lcd_rgb_frame_interval_handler = isr;
		s_lcd.lcd_rgb_interval_arg = args;
	}

	LOGD("%s: ISR registered for type %d\n", __func__, int_type);
	return BK_OK;
}
static bk_err_t lcd_i80_bus_delete(bk_lcd_i80_handle_t *handle)
{
	if (handle == NULL)
	{
		LOGE("%s: handle is NULL\n", __func__);
		return BK_FAIL;
	}

	// Clear the global handle if it matches
	if (s_lcd_i80_handle == handle)
	{
		s_lcd_i80_handle = NULL;
		LOGD("%s: I80 bus handle deinitialized\n", __func__);
	}

	os_free(handle);
	return BK_OK;
}


bk_lcd_i80_handle_t * lcd_i80_bus_io_register(void *io)
{
	// Check if handle already exists to prevent multiple registrations
	if (s_lcd_i80_handle != NULL)
	{
		LOGW("%s: I80 bus already registered, returning existing handle\n", __func__);
		return s_lcd_i80_handle;
	}

	bk_lcd_i80_handle_t *handle = os_malloc(sizeof(bk_lcd_i80_handle_t));
	if (handle == NULL)
	{
		LOGE("%s: malloc handle fail \n", __func__);
		return NULL;
	}

	os_memset(handle, 0, sizeof(bk_lcd_i80_handle_t));
	handle->write_cmd = bk_lcd_8080_send_cmd;
	handle->delete = lcd_i80_bus_delete;

	// Store the handle globally to prevent multiple registrations
	s_lcd_i80_handle = handle;

	LOGD("%s: I80 bus handle registered successfully\n", __func__);
	return handle;
}

#if (CONFIG_SOC_BK7236XX)
__attribute__((section(".iram"))) void lcd_isr(void)
#else
__attribute__((section(".itcm_sec_code"))) void lcd_isr(void)
#endif
{
	uint32_t int_status = lcd_hal_int_status_get();

	if (int_status & RGB_OUTPUT_SOF)
	{
		if (s_lcd.lcd_rgb_frame_start_handler)
		{
			s_lcd.lcd_rgb_frame_start_handler(s_lcd.lcd_rgb_start_arg);
		}
		lcd_hal_int_status_clear(RGB_OUTPUT_SOF);
	}
	if (int_status & RGB_OUTPUT_EOF)
	{
		if (s_lcd.lcd_rgb_frame_end_handler)
		{
			s_lcd.lcd_rgb_frame_end_handler(s_lcd.lcd_rgb_end_arg);
		}
		lcd_hal_int_status_clear(RGB_OUTPUT_EOF);
	}

	if (int_status & DE_INT)
	{
		if (s_lcd.lcd_rgb_de_handler)
		{
			s_lcd.lcd_rgb_de_handler(s_lcd.lcd_rgb_de_arg);
		}
		//lcd_hal_soft_reset();
		lcd_hal_int_status_clear(DE_INT);
	}
	if (int_status & FRAME_INTERVAL_INT)
	{
		if (s_lcd.lcd_rgb_frame_interval_handler)
		{
			s_lcd.lcd_rgb_frame_interval_handler(s_lcd.lcd_rgb_interval_arg);
		}
		lcd_hal_int_status_clear(FRAME_INTERVAL_INT);
	}

	if (int_status & I8080_OUTPUT_SOF)
	{
		if (s_lcd.lcd_8080_frame_start_handler)
		{
			s_lcd.lcd_8080_frame_start_handler(s_lcd.lcd_8080_start_arg);
		}
		lcd_hal_int_status_clear(I8080_OUTPUT_SOF);
	}

	if (int_status & I8080_OUTPUT_EOF)
	{
		if (s_lcd.lcd_8080_frame_end_handler)
		{
			s_lcd.lcd_8080_frame_end_handler(s_lcd.lcd_8080_end_arg);
		}
		lcd_hal_int_status_clear(I8080_OUTPUT_EOF);
	}
}


bk_err_t bk_lcd_driver_deinit(void)
{
	if (!s_lcd_driver_is_init) {
		LOGD("%s, lcd already deinit. \n", __func__);
		return BK_OK;
	}
	
	// Clear ISR handlers to prevent stale callbacks
	os_memset(&s_lcd, 0, sizeof(s_lcd));
	
	// Clean up I80 bus handle if it exists
	if (s_lcd_i80_handle != NULL)
	{
		lcd_i80_bus_delete(s_lcd_i80_handle);
		s_lcd_i80_handle = NULL;
	}

	sys_ll_set_cpu_device_clk_enable_disp_cken(0);
	lcd_disp_ll_set_module_control_soft_reset(0);

	bk_int_isr_unregister(INT_SRC_LCD);
	sys_drv_core_intr_group1_disable(CPU2_CORE_ID, LCD_INTERRUPT_CTRL_BIT);

	s_lcd_driver_is_init = false;
	LOGD("%s: LCD driver deinitialized successfully\n", __func__);
	return BK_OK;
}

bk_err_t bk_lcd_driver_init(lcd_clk_t clk)
{
	bk_err_t ret = BK_OK;
	if (s_lcd_driver_is_init) {
		LOGE("%s already init. \n", __func__);
		return BK_OK;
	}

	sys_drv_core_intr_group1_enable(CPU2_CORE_ID, LCD_INTERRUPT_CTRL_BIT);
	sys_ll_set_cpu_device_clk_enable_disp_cken(0);
	switch (clk)
	{
		case LCD_80M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_0, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_54M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_0, DISP_DIV_H_1, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_60M:
			ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_0, DISP_DIV_H_0, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_32M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_0, DISP_DIV_H_2, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_30M:
			ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_1, DISP_DIV_H_0, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_40M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_1, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_26M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_2, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_22M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_3, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_20M:
			ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_0, DISP_DIV_H_1, DSIP_DISCLK_ALWAYS_ON);
			//ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_3, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_17M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_0, DISP_DIV_H_4, DSIP_DISCLK_ALWAYS_ON);
			//ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_3, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_15M:
				ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_1, DISP_DIV_H_1, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_12M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_0, DISP_DIV_H_6, DSIP_DISCLK_ALWAYS_ON);
		break;
		case LCD_10M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_7, DSIP_DISCLK_ALWAYS_ON);
			//ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_1, DISP_DIV_H_2, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_8M:
			ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_0, DISP_DIV_H_3, DSIP_DISCLK_ALWAYS_ON);
			break;
		default:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_0, DISP_DIV_H_4, DSIP_DISCLK_ALWAYS_ON);
			break;
	}

	bk_int_isr_register(INT_SRC_LCD, lcd_isr, NULL);
	rtos_delay_milliseconds(1);
	lcd_disp_ll_set_module_control_soft_reset(0);
	rtos_delay_milliseconds(1);
	lcd_disp_ll_set_module_control_soft_reset(1);
	sys_ll_set_cpu_device_clk_enable_disp_cken(1);
	s_lcd_driver_is_init = true;
	return ret;
}
/**
 * @brief This API config lcd display x size and y size

 * @param
 *     - width lcd display width
 *     - height lcd display height
 *
 * attention 1. int the next version, the width and height deside the transfer number of lcd display.
 *              will config with another two register x offset and y offset
 *
 * attention 2. in this sdk version width/height only set once in 8080_init,if you want set twice,should
                set bk_lcd_8080_display_enable(0)
 */
bk_err_t bk_lcd_pixel_config(uint16_t x_pixel, uint16_t y_pixel)
{
	LCD_RETURN_ON_NOT_INIT();

	lcd_hal_pixel_config(x_pixel, y_pixel);
	return BK_OK;
}



bk_err_t bk_lcd_8080_send_cmd(uint32_t command, uint32_t *param, uint8_t param_count)
{
	LCD_RETURN_ON_NOT_INIT();

	// Add parameter validation
	if (param_count > 0 && param == NULL)
	{
		LOGE("%s: param is NULL but param_count is %d\n", __func__, param_count);
		return BK_ERR_NULL_PARAM;
	}

	lcd_hal_8080_cmd_send(param_count, command, param);
	return BK_OK;
}

bk_err_t bk_lcd_input_pixel_hf_reverse(bool hf_reverse)
{
	lcd_hal_set_pixel_reverse(hf_reverse);
	return BK_OK;
}

/*
 * @brief 
 * rgb input RGB565, rgb888. yuv format
 */
bk_err_t bk_lcd_set_yuv_mode(pixel_format_t input_data_format)
{
	lcd_hal_set_yuv_mode(input_data_format);
	return BK_OK;
}

pixel_format_t bk_lcd_get_yuv_mode(void)
{
	uint32_t yuv_mode = lcd_hal_get_display_yuv_sel();
	uint32_t reverse = lcd_hal_get_pixel_reverse();
	pixel_format_t output_data_format = PIXEL_FMT_UNKNOW;

	if (reverse == 0)
	{
		if (yuv_mode == 0)
		{
			output_data_format = PIXEL_FMT_RGB565_LE;
		}
		else if (yuv_mode == 1)
		{
			output_data_format = PIXEL_FMT_YUYV;
		}
		else if (yuv_mode == 2)
		{
			output_data_format = PIXEL_FMT_UYVY;
		}
		else if (yuv_mode == 3)
		{
			output_data_format = PIXEL_FMT_YYUV;
		}
		else if (yuv_mode == 4)
		{
			output_data_format = PIXEL_FMT_UVYY;
		}
		else if (yuv_mode == 5)
		{
			output_data_format = PIXEL_FMT_VUYY;
		}
	}
	else if (reverse == 1)
	{
		if (yuv_mode == 0)
		{
			output_data_format = PIXEL_FMT_RGB565;
		}
	}

	return output_data_format;
}

bk_err_t bk_lcd_set_partical_display(bool en, uint16_t partial_clum_l, uint16_t partial_clum_r, uint16_t partial_line_l, uint16_t partial_line_r)
{
	LCD_RETURN_ON_NOT_INIT();
	lcd_hal_set_partical_display(en, partial_clum_l, partial_clum_r, partial_line_l, partial_line_r);
	return BK_OK;
}
uint32_t bk_lcd_rgb_ver_cnt_get(void)
{
    return lcd_hal_get_status_ver_cnt_status();
}

#if CONFIG_FLASH
void lcd_flash_disable_int(uint32_t enable)
{
    if (lcd_hal_get_rbg_dispay_en() == 0)
        return;

    if(enable)
    {
        //check display flush status (vsync cnt) is flushing or not, when vsync_pulse_width timing cnt==0
        if (bk_lcd_rgb_ver_cnt_get() == 0)
        {
            //if not flush, delay 90us (need bigger then vsync_pulse_width times),recheck
            delay(20);
            //recheck flush status ,display is also not workking, to reset display
            if (bk_lcd_rgb_ver_cnt_get() == 0)
            {
                LOGD(" %s softreset display %d\n", __func__, bk_lcd_rgb_ver_cnt_get());
                lcd_disp_ll_set_module_control_soft_reset(0);
                delay(10);
                lcd_disp_ll_set_module_control_soft_reset(1);
            }
        }
#if (CONFIG_RGB_FLUSH_BY_SOF)
		lcd_hal_rgb_int_enable(1, 0);
#else
		lcd_hal_rgb_int_enable(0, 1);
#endif
    }
    else
    {
        lcd_hal_rgb_int_enable(0, 0);
    }
}
#endif

bk_err_t bk_lcd_rgb_init(const lcd_device_t *device)
{
	BK_RETURN_ON_NULL(device);

	const lcd_rgb_t *rgb = device->rgb;
	uint16_t x = device->width;  //lcd size x
	uint16_t y = device->height;  //lcd size y
	LOGV("%s\n", __func__);

	lcd_hal_rgb_display_sel(1);  //RGB display enable, and select rgb module
	if (rgb->hsync_pulse_width > 7 || rgb->vsync_pulse_width > 7)
	{
		LOGW("%s hsync_pulse_width or vsync_pulse_width is overflow %d %d, MAX 7, change to 2\n", __func__, rgb->hsync_pulse_width, rgb->vsync_pulse_width);
		lcd_hal_set_sync_low(2, 2);
	}
	else
	{
		lcd_hal_set_sync_low(rgb->hsync_pulse_width, rgb->vsync_pulse_width);
	}
#if (CONFIG_RGB_FLUSH_BY_SOF)
	lcd_hal_rgb_int_enable(1, 0);
#else
	lcd_hal_rgb_int_enable(0, 1);
#endif
#if CONFIG_FLASH
	mb_flash_register_op_notify(lcd_flash_disable_int);
#endif
	if (rgb->hsync_back_porch > 0x3FF || rgb->hsync_front_porch > 0x3FF ||
		rgb->vsync_back_porch > 0xFF || rgb->vsync_front_porch > 0xFF)
	{
		LOGW("%s porch value is overflow %d %d %d %d \n", __func__, rgb->hsync_back_porch, rgb->hsync_front_porch, rgb->vsync_back_porch, rgb->vsync_front_porch);
	}
	lcd_hal_rgb_sync_config(rgb->hsync_back_porch,
	                        rgb->hsync_front_porch,
	                        rgb->vsync_back_porch,
	                        rgb->vsync_front_porch);

	lcd_hal_set_rgb_clk_rev_edge(rgb->data_out_clk_edge);//output data is in clk doen edge or up adge

	lcd_hal_disconti_mode(DISCONTINUE_MODE);

	bk_lcd_pixel_config(x, y); //image xpixel ypixel
	// bk_lcd_set_yuv_mode(device->fmt);
	lcd_hal_set_data_fifo_thrd(DATA_FIFO_WR_THRD, DATA_FIFO_RD_THRD);
	return BK_OK;
}

bk_err_t bk_lcd_8080_init(const lcd_device_t *device)
{
	BK_RETURN_ON_NULL(device);

	uint16_t x = device->width ;
	uint16_t y = device->height;

	lcd_hal_rgb_display_sel(0); //25bit - rgb_on = 0 select 8080 mode
	lcd_hal_disconti_mode(DISCONTINUE_MODE);
	lcd_hal_8080_verify_1ms_count(VERIFY_1MS_COUNT);
	lcd_hal_8080_set_tik(TIK_CNT);
	lcd_hal_set_data_fifo_thrd(DATA_FIFO_WR_THRD, DATA_FIFO_RD_THRD);
	lcd_hal_8080_set_fifo_data_thrd(CMD_FIFO_WR_THRD, CMD_FIFO_RD_THRD);
	lcd_hal_pixel_config(x, y);
	lcd_hal_8080_display_enable(1);
	lcd_hal_8080_int_enable(0, 1); //set eof int enable
	// bk_lcd_set_yuv_mode(config->fmt);
	return BK_OK;
}

bk_err_t bk_lcd_8080_deinit(void)
{
	LCD_RETURN_ON_NOT_INIT();
	lcd_hal_8080_int_enable(0, 0);
	lcd_hal_8080_display_enable(0);
	lcd_hal_8080_start_transfer(0);
	return BK_OK;
}

bk_err_t bk_lcd_rgb_deinit(void)
{
	LCD_RETURN_ON_NOT_INIT();
	lcd_hal_rgb_int_enable(0, 0);
	lcd_hal_rgb_display_en(0);
	lcd_hal_rgb_display_sel(0);

	return BK_OK;
}

bk_err_t bk_lcd_8080_start_transfer(bool start)
{
	LCD_RETURN_ON_NOT_INIT();
	lcd_hal_8080_start_transfer(start);
	return BK_OK;
}

bk_err_t bk_lcd_rgb_display_en(bool en)
{
	lcd_hal_rgb_display_en(en);
	return BK_OK;
}

bk_err_t lcd_driver_display_disable(void)
{
	lcd_type_t type;
	type = s_lcd.device.type;

	if ((type == LCD_TYPE_RGB565) || (type == LCD_TYPE_RGB))
	{
		lcd_hal_rgb_display_en(0);
	} else if (type == LCD_TYPE_MCU8080) {
		lcd_hal_8080_cmd_param_count(0);
		lcd_hal_8080_start_transfer(0);
	}

	return BK_OK;
}

bk_err_t lcd_driver_display_enable(void)
{
	lcd_type_t type;
	type = s_lcd.device.type;
	if ((type == LCD_TYPE_RGB565) || (type == LCD_TYPE_RGB))
	{
		lcd_hal_rgb_display_en(1);
	}
	else if (type == LCD_TYPE_MCU8080)
	{
		lcd_hal_8080_start_transfer(1);
	}

	return BK_OK;
}

bk_err_t lcd_driver_display_continue(void)
{
	lcd_type_t type;
	type = s_lcd.device.type;

	if (type == LCD_TYPE_MCU8080)
	{
		lcd_hal_8080_start_transfer(1);
	}

	return BK_OK;
}

bk_err_t lcd_driver_set_display_base_addr(uint32_t disp_base_addr)
{
	lcd_hal_set_display_read_base_addr(disp_base_addr);

	return BK_OK;
}

void lcd_driver_ppi_set(uint16_t width, uint16_t height)
{
	uint16_t x = s_lcd.device.width;
	uint16_t y = s_lcd.device.height;

	//bk_lcd_set_partical_display(0, 0, 0, 0, 0);
	lcd_hal_pixel_config(width, height);
	uint16_t start_x = 1;
	uint16_t start_y = 1;
	uint16_t end_x = x;
	uint16_t end_y = y;

	if (x < width || y < height)
	{
		if (x < width)
		{
			start_x = (width - x) / 2 + 1;
			end_x = start_x + x - 1;
		}
		if (y < height)
		{
			start_y = (height - y) / 2 + 1;
			end_y = start_y + y - 1;
		}

		LOGV("%s, offset %d, %d, %d, %d\n", __func__, start_x, end_x, start_y, end_y);
		bk_lcd_set_partical_display(1, start_x, end_x, start_y, end_y);
	}
	else
	{
		bk_lcd_set_partical_display(0, 0, 0, 0, 0);
	}
}

bk_err_t lcd_driver_init(const lcd_device_t *device)
{
	BK_RETURN_ON_NULL(device);

	int ret = BK_OK;
	LOGD("%s  \n", __func__);

	/// LCD module power
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
	bk_pm_clock_ctrl(PM_CLK_ID_DISP, CLK_PWR_CTRL_PWR_UP);

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_480M);

	os_memset(&s_lcd, 0, sizeof(s_lcd));

	if ((device->type == LCD_TYPE_RGB565) || (device->type == LCD_TYPE_RGB))
	{
		bk_lcd_driver_init(device->rgb->clk);
		// lcd_rgb_gpio_init();
		bk_lcd_rgb_init(device);
		lcd_hal_rgb_set_in_out_format(device->src_fmt, device->out_fmt);
		// lcd_hal_int_enable(DE_INT);

		// lcd_hal_int_enable(FRAME_INTERVAL_INT);
		// lcd_hal_frame_interval_config(1, VSYNC_UNIT, 2);
	}
	else if (device->type == LCD_TYPE_MCU8080)
	{
		bk_lcd_driver_init(device->mcu->clk);
		lcd_mcu_gpio_init();

		bk_lcd_8080_init(device);

		lcd_hal_mcu_set_in_out_format(device->src_fmt, device->out_fmt);
	}

	os_memcpy((void*)&s_lcd.device, device, sizeof(lcd_device_t));

	return ret;
}

bk_err_t lcd_driver_deinit(void)
{
	bk_err_t ret = BK_OK;

	// Check if driver is initialized
	if (!s_lcd_driver_is_init)
	{
		LOGD("%s: LCD driver not initialized\n", __func__);
		return BK_OK;
	}

	if (s_lcd.device.type == LCD_TYPE_RGB565)
	{
		ret = bk_lcd_rgb_deinit();
		if (ret != BK_OK)
		{
			LOGE("lcd system deinit reg config error \r\n");
			ret = BK_FAIL;
			goto out;
		}
	}
	else if (s_lcd.device.type == LCD_TYPE_MCU8080)
	{
		ret = bk_lcd_8080_deinit();
		if (ret != BK_OK)
		{
			LOGE("lcd system deinit reg config error \r\n");
			ret = BK_FAIL;
			goto out;
		}
	}

out:
	bk_lcd_driver_deinit();
	bk_pm_clock_ctrl(PM_CLK_ID_DISP, CLK_PWR_CTRL_PWR_DOWN);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);
	LOGD("%s: LCD driver deinitialized\n", __func__);
	return ret;
}


