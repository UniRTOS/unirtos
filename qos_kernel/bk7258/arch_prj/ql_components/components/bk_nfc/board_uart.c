#include "sdkconfig.h"
#include <string.h>
#include "uart_hal.h"
#include <driver/uart.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include "board_uart.h"
#include <bk_private/bk_uart.h>

static uint8_t           s_host_nfc_tx_data;
static volatile uint8_t  s_nfc_host_rx_data;
static volatile uint8_t  s_nfc_host_tx_addr;
static volatile uint8_t  s_nfc_host_state = HOST_STATE_IDLE;
static beken_semaphore_t s_complete_semph = NULL;

static void mfrc522_uart_rx_isr(uart_id_t id, void *param)
{
       uint8_t    rx_data;
       int        ret_val;
       while(1)
       {
              ret_val = uart_read_byte_ex(CONFIG_NFC_MFRC522_COMM_SERIAL_PORT, &rx_data);

              if(ret_val == -1)
                     break;
              if(s_nfc_host_state == HOST_STATE_IDLE)
              {
                     continue;
              }
              if(s_nfc_host_state == HOST_STATE_WAIT_ACK)
              {
                     if(s_nfc_host_tx_addr == rx_data)
                     {
                            bk_uart_write_bytes(CONFIG_NFC_MFRC522_COMM_SERIAL_PORT, &s_host_nfc_tx_data, 1);
                            rtos_set_semaphore(&s_complete_semph);
                     }
                     continue;
              }
              if(s_nfc_host_state == HOST_STATE_WAIT_DATA)
              {
                     s_nfc_host_rx_data = rx_data;
                     rtos_set_semaphore(&s_complete_semph);
                     continue;
              }
       }
}

void nfc_isr(gpio_id_t gpio_id)
{
   // os_printf("get the nfc\r\n");
}

static void mfrc522_gpio_config(gpio_id_t index, gpio_io_mode_t dir, gpio_pull_mode_t pull, gpio_func_mode_t peir)
{
    if(index >= GPIO_NUM)
    {
        return;
    }
    gpio_dev_unmap(index);
    gpio_config_t cfg;
    cfg.io_mode = dir;
    cfg.pull_mode = pull;
    cfg.func_mode = peir;
    bk_gpio_set_config(index, &cfg);
}

/**
 @brief NFC uart驱动初始化
 @param 无
 @return 无
*/
void mfrc522_uart_init(void)
{
    mfrc522_gpio_config(MFRC522_G_INT_PIN,GPIO_INPUT_ENABLE,GPIO_PULL_UP_EN,GPIO_SECOND_FUNC_DISABLE); //config_gpio
    bk_gpio_register_isr(MFRC522_G_INT_PIN , nfc_isr);
    bk_gpio_set_interrupt_type(MFRC522_G_INT_PIN, GPIO_INT_TYPE_FALLING_EDGE); 
    bk_gpio_enable_interrupt(MFRC522_G_INT_PIN);

    uart_config_t uart_cfg = {0};

    uart_cfg.baud_rate = UART_BAUDRATE_9600;
    uart_cfg.data_bits = UART_DATA_8_BITS;
    uart_cfg.parity = UART_PARITY_NONE;
    uart_cfg.stop_bits = UART_STOP_BITS_1;
    uart_cfg.flow_ctrl = UART_FLOWCTRL_DISABLE;
    uart_cfg.src_clk = UART_SCLK_XTAL_26M;

    rtos_init_semaphore(&s_complete_semph, 1);

    bk_uart_init(CONFIG_NFC_MFRC522_COMM_SERIAL_PORT, &uart_cfg);
    bk_uart_disable_sw_fifo(CONFIG_NFC_MFRC522_COMM_SERIAL_PORT);
    bk_uart_register_rx_isr(CONFIG_NFC_MFRC522_COMM_SERIAL_PORT, mfrc522_uart_rx_isr, NULL);
    bk_uart_enable_rx_interrupt(CONFIG_NFC_MFRC522_COMM_SERIAL_PORT);
}

/**
 @brief NFC uart驱动deinit
 @param 无
 @return 无
*/
void mfrc522_uart_deinit(void)
{
    bk_uart_disable_rx_interrupt(CONFIG_NFC_MFRC522_COMM_SERIAL_PORT);
    bk_uart_deinit(CONFIG_NFC_MFRC522_COMM_SERIAL_PORT);
    rtos_deinit_semaphore(&s_complete_semph);
    s_complete_semph = NULL;
    mfrc522_gpio_config(MFRC522_G_INT_PIN,GPIO_IO_DISABLE,GPIO_PULL_DISABLE,GPIO_SECOND_FUNC_DISABLE);
}

/**
 @brief write NFC复位引脚工作模式(for debug)
 @param mode -[in] 工作模式
 @return 无
*/
void mfrc522_gpio_write(uint8_t mode)
{
#if NFC_DEBUG_CODE
   bk_gpio_set_value(MFRC522_RST_GPIO_PIN, mode);
#endif
}

/**
 @brief 读RC522寄存器
 @param addr -[in] 寄存器地址
 @return 读出一字节数据
*/

uint8_t mfrc522_read_rawRc(uint8_t addr)
{
	s_nfc_host_state = HOST_STATE_WAIT_DATA;

       rtos_get_semaphore(&s_complete_semph, 0);  // clear the semaphore state.
	addr |= 0x80;
	bk_uart_write_bytes(CONFIG_NFC_MFRC522_COMM_SERIAL_PORT, &addr, 1);
       rtos_get_semaphore(&s_complete_semph, MFRC522_WAIT_OUT_TIME);
	s_nfc_host_state = HOST_STATE_IDLE;

       return s_nfc_host_rx_data;
}

/**
 @brief 写RC522寄存器
 @param addr -[in] 寄存器地址
 @param writeData -[in] 写入数据
 @return 无
*/
void mfrc522_write_rawRc(uint8_t addr, uint8_t writeData)
{
	s_nfc_host_state = HOST_STATE_WAIT_ACK;

	addr &= 0x7F;
	s_nfc_host_tx_addr = addr;
	s_host_nfc_tx_data = writeData;
	bk_uart_write_bytes(CONFIG_NFC_MFRC522_COMM_SERIAL_PORT, &addr, 1);
       rtos_get_semaphore(&s_complete_semph, MFRC522_WAIT_OUT_TIME);
	s_nfc_host_state = HOST_STATE_IDLE;
}