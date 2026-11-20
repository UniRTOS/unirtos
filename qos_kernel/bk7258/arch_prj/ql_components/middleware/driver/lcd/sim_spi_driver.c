#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <stdlib.h>
#include <driver/sim_spi.h>
#include <driver/gpio.h>
#include "bk_misc.h"
#include <driver/hal/hal_gpio_types.h>
#include "gpio_driver.h"

#define TAG "st7796s"

#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#if 1
static void SPI_SendData(uint8_t data)
{
	uint8_t n;

	//in while loop, to avoid disable IRQ too much time, release it if finish one byte.
    uint32_t irq_level = rtos_disable_int();
    for (n = 0; n < 8; n++)
	{
		if (data & 0x80)
		{
			bk_gpio_set_output_high(LCD_SPI_SDA_GPIO);
		}
		else
		{
			bk_gpio_set_output_low(LCD_SPI_SDA_GPIO);
		}

		bk_delay_us(LCD_SPI_DELAY);
		data <<= 1;

		bk_gpio_set_output_low(LCD_SPI_CLK_GPIO);
		bk_delay_us(LCD_SPI_DELAY);
		bk_gpio_set_output_high(LCD_SPI_CLK_GPIO);
		bk_delay_us(LCD_SPI_DELAY);

	}
    rtos_enable_int(irq_level);
}

void lcd_spi_write_cmd(uint8_t cmd)
{
	bk_gpio_set_output_low(LCD_SPI_CSX_GPIO);
	bk_delay_us(LCD_SPI_DELAY);
	bk_gpio_set_output_low(LCD_SPI_SDA_GPIO);
	bk_delay_us(LCD_SPI_DELAY);

	bk_gpio_set_output_low(LCD_SPI_CLK_GPIO);
	bk_delay_us(LCD_SPI_DELAY);
	bk_gpio_set_output_high(LCD_SPI_CLK_GPIO);
	bk_delay_us(LCD_SPI_DELAY);

	SPI_SendData(cmd);

	bk_gpio_set_output_high(LCD_SPI_CSX_GPIO);
	bk_delay_us(LCD_SPI_DELAY);
}
void lcd_spi_write_data(uint8_t data)
{
	bk_gpio_set_output_low(LCD_SPI_CSX_GPIO);
	bk_delay_us(LCD_SPI_DELAY);
	bk_gpio_set_output_high(LCD_SPI_SDA_GPIO);
	bk_delay_us(LCD_SPI_DELAY);

	bk_gpio_set_output_low(LCD_SPI_CLK_GPIO);
	bk_delay_us(LCD_SPI_DELAY);
	bk_gpio_set_output_high(LCD_SPI_CLK_GPIO);
	bk_delay_us(LCD_SPI_DELAY);

	SPI_SendData(data);

	bk_gpio_set_output_high(LCD_SPI_CSX_GPIO);
	bk_delay_us(LCD_SPI_DELAY);
}

void lcd_spi_write_hf_word_data(unsigned int data)
{
	bk_gpio_set_output_low(LCD_SPI_CSX_GPIO);
	bk_delay_us(LCD_SPI_DELAY);


	SPI_SendData(0x40);
	SPI_SendData(data);


	bk_gpio_set_output_high(LCD_SPI_CSX_GPIO);
	bk_delay_us(LCD_SPI_DELAY);
}
void lcd_spi_write_hf_word_cmd(unsigned int cmd)
{
	bk_gpio_set_output_low(LCD_SPI_CSX_GPIO);
	bk_delay_us(LCD_SPI_DELAY);

	SPI_SendData(0x20);
	SPI_SendData(cmd >> 8); //high 8bit

	SPI_SendData(0x00);	  //low 8bit
	SPI_SendData(cmd);

	bk_gpio_set_output_high(LCD_SPI_CSX_GPIO);
	bk_delay_us(LCD_SPI_DELAY);
}

void lcd_spi_init_gpio(void)
{
}
static bk_lcd_spi_handle_t *g_spi_handle = NULL;
bk_lcd_spi_handle_t *get_spi_handle(void)
{
    return g_spi_handle;
}
int32_t lcd_driver_get_spi_gpio(LCD_SPI_GPIO_TYPE_E gpio_type)
{
    int32_t gpio = 0;
	bk_lcd_spi_handle_t *handle = get_spi_handle();
	if (!handle)
	{
		LOGE("%s: NULL handle \n", __func__);
		return -1;
	}
    switch (gpio_type)
    {
		case SPI_GPIO_CLK:
			gpio = handle->io.clk;
			break;
		case SPI_GPIO_CSX:
			gpio = handle->io.cs;
			break;
		case SPI_GPIO_SDA:
			gpio = handle->io.sda;
			break;
		case SPI_GPIO_RST:
			gpio = handle->io.rst;
			break;
		default:
			break;
    }
    return gpio;
}
#endif

static bk_err_t lcd_spi_io_init_by_handle(const bk_lcd_spi_handle_t *handle)
{
	if (!handle)
	{
		LOGE("%s: NULL handle \n", __func__);
		return BK_ERR_NULL_PARAM;
	}
	if (handle->io.clk == -1 || handle->io.cs == -1 || handle->io.sda == -1 || handle->io.rst == -1)
	{
		LOGE("%s clk: %d, cs: %d, sda: %d, rst: %d \n", __func__, handle->io.clk, handle->io.cs, handle->io.sda, handle->io.rst);
		return BK_ERR_PARAM;
	}

	const lcd_spi_io_t *io = &handle->io;
	gpio_dev_unmap(io->rst);
    BK_LOG_ON_ERR(bk_gpio_disable_input(io->rst));
    BK_LOG_ON_ERR(bk_gpio_enable_output(io->rst));

    gpio_dev_unmap(io->clk);
    BK_LOG_ON_ERR(bk_gpio_disable_input(io->clk));
    BK_LOG_ON_ERR(bk_gpio_enable_output(io->clk));

    gpio_dev_unmap(io->cs);
    BK_LOG_ON_ERR(bk_gpio_disable_input(io->cs));
    BK_LOG_ON_ERR(bk_gpio_enable_output(io->cs));

    gpio_dev_unmap(io->sda);
    BK_LOG_ON_ERR(bk_gpio_disable_input(io->sda));
    BK_LOG_ON_ERR(bk_gpio_enable_output(io->sda));

	bk_delay_us(200);
	LOGD("%s clk: %d, cs: %d, sda: %d, rst: %d \n", __func__, io->clk, io->cs, io->sda, io->rst);
	bk_gpio_set_output_high(io->clk);
	bk_gpio_set_output_high(io->cs);
	bk_gpio_set_output_high(io->sda);
	bk_gpio_set_output_high(io->rst);
	return BK_OK;
}

static void spi_send_data(const lcd_spi_io_t *io, uint8_t data)
{
	uint8_t n;

	//in while loop, to avoid disable IRQ too much time, release it if finish one byte.
    uint32_t irq_level = rtos_disable_int();
    for (n = 0; n < 8; n++)
	{
		if (data & 0x80)
		{
            bk_gpio_set_output_high(io->sda);
		}
		else
		{
            bk_gpio_set_output_low(io->sda);
		}

		bk_delay_us(LCD_SPI_DELAY);
		data <<= 1;

        bk_gpio_set_output_low(io->clk);
		bk_delay_us(LCD_SPI_DELAY);
        bk_gpio_set_output_high(io->clk);
		bk_delay_us(LCD_SPI_DELAY);

	}
    rtos_enable_int(irq_level);
}

static bk_err_t lcd_spi_write_cmd_by_handle(const bk_lcd_spi_handle_t *handle, uint8_t cmd)
{
	if (!handle)
	{
		LOGE("%s: NULL handl \n", __func__);
		return BK_ERR_NULL_PARAM;
	}

	const lcd_spi_io_t *io = &handle->io;
	bk_gpio_set_output_low(io->cs);
	bk_delay_us(LCD_SPI_DELAY);
	bk_gpio_set_output_low(io->sda);
	bk_delay_us(LCD_SPI_DELAY);

	bk_gpio_set_output_low(io->clk);
	bk_delay_us(LCD_SPI_DELAY);
	bk_gpio_set_output_high(io->clk);
	bk_delay_us(LCD_SPI_DELAY);

	spi_send_data(io, cmd);

	bk_gpio_set_output_high(io->cs);
	bk_delay_us(LCD_SPI_DELAY);
	return BK_OK;
}
static bk_err_t lcd_spi_write_data_by_handle(const bk_lcd_spi_handle_t *handle, uint8_t data)
{
	if (!handle)
	{
		LOGE("%s: NULL handle \n", __func__);
		return BK_ERR_NULL_PARAM;

	}
	const lcd_spi_io_t *io = &handle->io;
	bk_gpio_set_output_low(io->cs);
	bk_delay_us(LCD_SPI_DELAY);
	bk_gpio_set_output_high(io->sda);
	bk_delay_us(LCD_SPI_DELAY);

	bk_gpio_set_output_low(io->clk);
	bk_delay_us(LCD_SPI_DELAY);
	bk_gpio_set_output_high(io->clk);
	bk_delay_us(LCD_SPI_DELAY);

	spi_send_data(io, data);

	bk_gpio_set_output_high(io->cs);
	bk_delay_us(LCD_SPI_DELAY);
	return BK_OK;
}

static bk_err_t lcd_spi_write_hf_word_data_by_handle(const bk_lcd_spi_handle_t *handle, unsigned int data)
{
	if (!handle)
	{
		LOGE("%s: NULL handle \n", __func__);
		return BK_ERR_NULL_PARAM;
	}

 	const lcd_spi_io_t *io = &handle->io;
	bk_gpio_set_output_low(io->cs);
	bk_delay_us(LCD_SPI_DELAY);

    spi_send_data(io, 0x40);
	spi_send_data(io, data);

	bk_gpio_set_output_high(io->cs);
	bk_delay_us(LCD_SPI_DELAY);
	return BK_OK;

}
static bk_err_t lcd_spi_write_hf_word_cmd_by_handle(const bk_lcd_spi_handle_t *handle, unsigned int cmd)
{
	if (!handle)
	{
		LOGE("%s: NULL handle\n", __func__);
		return BK_ERR_NULL_PARAM;
	}

	const lcd_spi_io_t *io = &handle->io;
	bk_gpio_set_output_low(io->cs);
	bk_delay_us(LCD_SPI_DELAY);

	spi_send_data(io, 0x20);
	spi_send_data(io, cmd >> 8); //high 8bit

	spi_send_data(io, 0x00);	  //low 8bit
	spi_send_data(io, cmd);

	bk_gpio_set_output_high(io->cs);
	bk_delay_us(LCD_SPI_DELAY);
	return BK_OK;

}

static bk_err_t lcd_spi_deinit_by_handle(bk_lcd_spi_handle_t *handle)
{
	if (!handle)
	{
		LOGE("%s: NULL handle \n", __func__);
		return BK_ERR_NULL_PARAM;
	}

    if (&handle->io)
    {
        gpio_dev_unmap(handle->io.rst);
		gpio_dev_unmap(handle->io.clk);
		gpio_dev_unmap(handle->io.cs);
        gpio_dev_unmap(handle->io.sda);
    }
    if (handle)
    {
		if (handle == g_spi_handle)
        {
            g_spi_handle = NULL;
        }
        os_free(handle);
    }
	return BK_OK;
}

bk_lcd_spi_handle_t * lcd_spi_bus_io_register(const lcd_spi_io_t *io)
{
    bk_lcd_spi_handle_t *handle = (bk_lcd_spi_handle_t *)os_malloc(sizeof(bk_lcd_spi_handle_t));
    if (!handle)
    {
		LOGE("%s: malloc handle fail \n", __func__);
        return NULL;
    }
	if ((io->clk < 0 || io->cs < 0 || io->sda < 0 || io->rst < 0) ||
	 (io->clk == 0 && io->cs == 0 && io->sda == 0 && io->rst == 0))
    {
		LOGW("%s: invalid io clk %d cs %d sda %d rst %d \n", __func__, io->clk, io->cs, io->sda, io->rst);
        return NULL;
    }
    handle->io = *io;
	handle->init = lcd_spi_io_init_by_handle;
    handle->write_cmd = lcd_spi_write_cmd_by_handle;
    handle->write_data = lcd_spi_write_data_by_handle;
    handle->write_hf_word_data = lcd_spi_write_hf_word_data_by_handle;
    handle->write_hf_word_cmd = lcd_spi_write_hf_word_cmd_by_handle;
    handle->deinit = lcd_spi_deinit_by_handle;
	g_spi_handle = handle;
	return handle;
}


