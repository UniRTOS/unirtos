/*********************************************************************
 * INCLUDES
 */
#include "os/os.h"
#include "os/mem.h"
#include <string.h>
#include "uart_hal.h"
#include <driver/uart.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/log.h>
#include "board_uart.h"
#include "board_mfrc522.h"
#include "components/bk_nfc.h"

void RC522_Config(unsigned char Card_Type)
{
	bk_mfrc522_clear_bit_mask(MFRC522_REG_STATUS2,MFRC522_REG_STATUS2_VAL);
	bk_mfrc522_write_rawRc(MFRC522_REG_MODE,MFRC522_REG_MODE_VAL);
	bk_mfrc522_write_rawRc(MFRC522_REG_RX_SEL,MFRC522_REG_RX_SEL_VAL);
	bk_mfrc522_write_rawRc(MFRC522_REG_RF_CFG,MFRC522_REG_RF_CFG_VAL);
	bk_mfrc522_write_rawRc(MFRC522_REG_TRELOAD_L,MFRC522_REG_TRELOAD_L_VAL);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec)
	bk_mfrc522_write_rawRc(MFRC522_REG_TRELOAD_H,MFRC522_REG_TRELOAD_H_VAL);
	bk_mfrc522_write_rawRc(MFRC522_REG_TMODE,MFRC522_REG_TMODE_VAL);
	bk_mfrc522_write_rawRc(MFRC522_REG_TPRESCALER,MFRC522_REG_TPRESCALER_VAL);
}

char PcdHalt(void)
{
    uint32_t  unLen;
    uint32_t  status;
    unsigned char ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = MFRC522_PICC_HALT;
    ucComMF522Buf[1] = 0;
    bk_mfrc522_calulate_crc(ucComMF522Buf,2,&ucComMF522Buf[2]);
	status = bk_mfrc522_com_transceive(MFRC522_PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf, &unLen);
    MFRC522_LOGI("status :0x%x  \r\n",status);

    return MI_OK;
}

/**
 @brief MFRC522的初始化函数
 @param 无
 @return 无
*/
void mfrc522_init(void)
{
    bk_mfrc522_reset();         // 复位
    bk_mfrc522_antenna_off();   //关闭天线发射
    rtos_delay_milliseconds(2);
    bk_mfrc522_antenna_on();    // 开启天线发射
}

/**
 @brief MFRC522的初始化函数
 @param 无
 @return 无
*/
void mfrc522_deinit(void)
{
    bk_mfrc522_antenna_off();   //关闭天线发射
    rtos_delay_milliseconds(5);	
    PcdHalt();                  // 复位
}

/**
 @brief  Search for card
 @param reqCode -[in] Search method for cards，0x52Search method for cards, 0x52 searches for all cards compliant with the 1443A standard ，0x26 searches for cards not in a sleep state
 @param pTagType -[out] Tag type code
                        0x4400 = Mifare_UltraLight
                        0x0400 = Mifare_One(S50)
                        0x0200 = Mifare_One(S70)
                        0x0800 = Mifare_Pro(X)
                        0x4403 = Mifare_DESFire
 @return state value，MI OK - successful；MI_ERR - failed
*/
char bk_mfrc522_request(uint8_t reqCode, uint8_t *pTagType)
{
    char status;
    uint32_t len;
    uint8_t comMF522Buf[MAXRLEN];

    bk_mfrc522_clear_bit_mask(MFRC522_REG_STATUS2, 0x08);
    bk_mfrc522_write_rawRc(MFRC522_REG_BIT_FRAMING, 0x07);
    bk_mfrc522_set_bit_mask(MFRC522_REG_TX_CONTROL, 0x03);

    comMF522Buf[0] = reqCode;

    status = bk_mfrc522_com_transceive(MFRC522_PCD_TRANSCEIVE, comMF522Buf, 1, comMF522Buf, &len);    // 发送并接收数据
    if((status == MI_OK) && (len == 0x10))
    {
        MFRC522_LOGI("mi_ok \r\n");
        *pTagType = comMF522Buf[0];
        *(pTagType+1) = comMF522Buf[1];
    }
    else
    {
        MFRC522_LOGE("mi_err \r\n");
        status = MI_ERR;
    }

    return status;
}

/**
 @brief Anti-collision
 @param pSnr -[out] Card serial number, 4 bytes
 @return state value，MI OK - successful；MI_ERR - failed
*/
char bk_mfrc522_anticoll(uint8_t *pSnr)
{
    char status;
    uint8_t i, snrCheck = 0;
    uint32_t len;
    uint8_t comMF522Buf[MAXRLEN];

    bk_mfrc522_clear_bit_mask(MFRC522_REG_STATUS2, 0x08);             // 寄存器包含接收器和发送器和数据模式检测器的状态标志
    bk_mfrc522_write_rawRc(MFRC522_REG_BIT_FRAMING, 0x00);            // 不启动数据发送，接收的LSB位存放在位0，接收到的第二位放在位1，定义发送的最后一个字节位数为8
    bk_mfrc522_clear_bit_mask(MFRC522_REG_COLL, 0x80);                // 所有接收的位在冲突后将被清除

    comMF522Buf[0] = MFRC522_PICC_ANTICOLL1;
    comMF522Buf[1] = 0x20;

    status = bk_mfrc522_com_transceive(MFRC522_PCD_TRANSCEIVE, comMF522Buf, 2, comMF522Buf, &len);

    if(status == MI_OK)
    {
        for(i = 0; i < 4; i++)
        {
            *(pSnr + i) = comMF522Buf[i];
            snrCheck ^= comMF522Buf[i];
        }
        if(snrCheck != comMF522Buf[i])          // 返回四个字节，最后一个字节为校验位
        {
            status = MI_ERR;
        }
    }

    bk_mfrc522_set_bit_mask(MFRC522_REG_COLL, 0x80);

    return status;
}

/**
 @brief Select Card
 @param pSnr -[in] Card serial number, 4 bytes
 @return state value，MI OK - successful；MI_ERR - failed
*/
char bk_mfrc522_select(uint8_t *pSnr)
{
    char status;
    uint8_t i;
    uint8_t comMF522Buf[MAXRLEN];
    uint32_t len;

    comMF522Buf[0] = MFRC522_PICC_ANTICOLL1;
    comMF522Buf[1] = 0x70;
    comMF522Buf[6] = 0;

    for(i = 0; i < 4; i++)
    {
        comMF522Buf[i + 2] = *(pSnr + i);
        comMF522Buf[6] ^= *(pSnr + i);
    }

    bk_mfrc522_calulate_crc(comMF522Buf, 7, &comMF522Buf[7]);

    bk_mfrc522_clear_bit_mask(MFRC522_REG_STATUS2, 0x08);

    status = bk_mfrc522_com_transceive(MFRC522_PCD_TRANSCEIVE, comMF522Buf, 9, comMF522Buf, &len);

    if((status == MI_OK ) && (len == 0x18))
    {
        status = MI_OK;
    }
    else
    {
        status = MI_ERR;
    }

    return status;
}

/**
 @brief Verify card password
 @param authMode -[in] Password verification mode， 0x60 verifies A key, 0x61 verifies B key
 @param addr -[in] Block address
 @param pKey -[in] Password
 @param pSnr -[in] Card serial number, 4 bytes
 @return state value，MI OK - successful；MI_ERR - failed
*/
char bk_mfrc522_authState(uint8_t authMode, uint8_t addr, uint8_t *pKey, uint8_t *pSnr)
{
    char status;
    uint8_t comMF522Buf[MAXRLEN] = {0};
    uint32_t len;

    comMF522Buf[0] = authMode;
    comMF522Buf[1] = addr;
    os_memcpy(&comMF522Buf[2], pKey, 6);
    os_memcpy(&comMF522Buf[8], pSnr, 6);
    status = bk_mfrc522_com_transceive(MFRC522_PCD_AUTHENT, comMF522Buf, 12, comMF522Buf, &len);

    if((status != MI_OK ) || ( ! (bk_mfrc522_read_rawRc(MFRC522_REG_STATUS2) & 0x08)))
    {
        status = MI_ERR;
    }

    return status;
}

/**
 @brief Read one block of data from an M1 card
 @param addr -[in] Block address
 @param pData -[out] The read data (16 bytes)
 @return Status value, MI_OK - success; MI_ERR - failure
*/
char bk_mfrc522_read(uint8_t addr, uint8_t *pData)
{
    char status;
    uint8_t i, comMF522Buf[MAXRLEN];
    uint32_t len;

    comMF522Buf[0] = MFRC522_PICC_READ;
    comMF522Buf[1] = addr;

    bk_mfrc522_calulate_crc(comMF522Buf, 2, &comMF522Buf[2]);

    status = bk_mfrc522_com_transceive(MFRC522_PCD_TRANSCEIVE, comMF522Buf, 4, comMF522Buf, &len);

    if((status == MI_OK) && (len == 0x90))
    {
        for(i = 0; i < 16; i++)
        {
            *(pData + i) = comMF522Buf[i];
        }
    }
    else
    {
        status = MI_ERR;
    }

    return status;
}

/**
 @brief Write a block of data to the M1 card.
 @param addr -[in] Block address
 @param pData -[out] Data to write, 16 bytes
 @return Status value, MI_OK - success; MI_ERR - failure
 */
char bk_mfrc522_write(uint8_t addr, uint8_t *pData)
{
    char status;
    uint8_t i, comMF522Buf[MAXRLEN];
    uint32_t len;

    comMF522Buf[0] = MFRC522_PICC_WRITE;
    comMF522Buf[1] = addr;

    bk_mfrc522_calulate_crc(comMF522Buf, 2, &comMF522Buf[2]);

    status = bk_mfrc522_com_transceive(MFRC522_PCD_TRANSCEIVE, comMF522Buf, 4, comMF522Buf, &len);
    if((status != MI_OK) || (len != 4) || ((comMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }

    if(status == MI_OK)
    {
        for(i = 0; i < 16; i++)
        {
            comMF522Buf[i] = *(pData + i);
        }
        bk_mfrc522_calulate_crc(comMF522Buf, 16, &comMF522Buf[16]);

        status = bk_mfrc522_com_transceive(MFRC522_PCD_TRANSCEIVE, comMF522Buf, 18, comMF522Buf, &len);
        if((status != MI_OK) || (len != 4) || ((comMF522Buf[0] & 0x0F) != 0x0A))
        {
            status = MI_ERR;
        }
    }

    return status;
}

/**
 @brief reset RC522
 @return None
*/
void bk_mfrc522_reset(void)
{
#if NFC_DEBUG_CODE
    // 需先保持高电平，后给个下降沿
    mfrc522_gpio_write(MFRC522_RST_LOW);
    rtos_delay_milliseconds(5);
    mfrc522_gpio_write(MFRC522_RST_HIGH);
    rtos_delay_milliseconds(100);
#endif
    uint32_t speed = 0;
    for(int i =0 ;i<2;i++) //loops twice to read all the dirty data in fifo.
    {
        speed = bk_mfrc522_read_rawRc(MFRC522_REG_SERIAL_SPEED);  
        MFRC522_LOGD("speed :0x%x  \r\n",speed);
    }
	
    bk_mfrc522_write_rawRc(MFRC522_REG_COMMAND, MFRC522_PCD_RESETPHASE);     // 和MI卡通讯，CRC初始值0x6363
    rtos_delay_milliseconds(2);

    bk_mfrc522_write_rawRc(MFRC522_REG_MODE, 0x3D);
    bk_mfrc522_write_rawRc(MFRC522_REG_TRELOAD_L, 30);
    bk_mfrc522_write_rawRc(MFRC522_REG_TRELOAD_H, 0);
    bk_mfrc522_write_rawRc(MFRC522_REG_TMODE, 0x8D);
    bk_mfrc522_write_rawRc(MFRC522_REG_TPRESCALER, 0x3E);
    bk_mfrc522_write_rawRc(MFRC522_REG_TX_ASK, 0x40);
    bk_mfrc522_clear_bit_mask(MFRC522_REG_TEST_PIN_EN, 0x80);//off MX and DTRQ out
    bk_mfrc522_write_rawRc(MFRC522_REG_TX_ASK, 0x40);
}


/** 
@brief Calculate CRC16 using MF522
@param pInData -[in] Array to calculate CRC16
@param len -[in] Byte length of the array to calculate CRC16
@param pOutData -[out] First address where the result is stored
@return None
*/
void bk_mfrc522_calulate_crc(uint8_t *pInData, uint8_t len, uint8_t *pOutData)
{
    uint8_t i, n;

    bk_mfrc522_clear_bit_mask(MFRC522_REG_DIVIRQ, 0x04);
    bk_mfrc522_write_rawRc(MFRC522_REG_COMMAND, MFRC522_PCD_IDLE);
    bk_mfrc522_set_bit_mask(MFRC522_REG_FIFO_LEVEL, 0x80);

    for(i = 0; i < len; i++)
    {
        bk_mfrc522_write_rawRc(MFRC522_REG_FIFO_DATA, *(pInData + i));
    }

    bk_mfrc522_write_rawRc(MFRC522_REG_COMMAND, MFRC522_PCD_CALCCRC);

    i = 0xFF;

    do
    {
        n = bk_mfrc522_read_rawRc(MFRC522_REG_DIVIRQ);
        i--;
    }
    while((i != 0) && ! (n & 0x04));

    pOutData[0] = bk_mfrc522_read_rawRc(MFRC522_REG_CRC_RESULT_L);
    pOutData[1] = bk_mfrc522_read_rawRc(MFRC522_REG_CRC_RESULT_H);
}


/** @brief Communicates with an ISO14443 card using the MFRC522 module
@param command -[in] RC522 command word
@param pInData -[in] Data sent to the card via RC522
@param inLenByte -[in] Length of the sent data in bytes
@param pOutData -[out] Data received from the card
@param pOutLenBit -[out] Length of the received data in bits
@return Status value, MI_OK - Success; MI_ERR - Failure
*/
char bk_mfrc522_com_transceive(uint8_t command, uint8_t *pInData, uint8_t inLenByte, uint8_t *pOutData, uint32_t *pOutLenBit)
{
    char status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitFor = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint32_t i;
    uint8_t j;

    switch(command)
    {
    case MFRC522_PCD_AUTHENT:
        irqEn = 0x12;
        waitFor = 0x10;
        break;
    case MFRC522_PCD_TRANSCEIVE:
        irqEn = 0x77;
        waitFor = 0x30;
        break;
    default:
        break;
    }

    bk_mfrc522_write_rawRc(MFRC522_REG_COMIEN, irqEn | 0x80);
    bk_mfrc522_clear_bit_mask(MFRC522_REG_COMIRQ, 0x80);
    bk_mfrc522_write_rawRc(MFRC522_REG_COMMAND, MFRC522_PCD_IDLE);
    bk_mfrc522_write_rawRc(MFRC522_REG_FIFO_LEVEL, 0x80);             // 清空FIFO

    for(i = 0; i < inLenByte; i++)
    {
        bk_mfrc522_write_rawRc(MFRC522_REG_FIFO_DATA, pInData[i]);    // 数据写入FIFO
    }
    bk_mfrc522_write_rawRc(MFRC522_REG_COMMAND, command);            // 命令写入命令寄存器

    if(command == MFRC522_PCD_TRANSCEIVE)
    {
        bk_mfrc522_set_bit_mask(MFRC522_REG_BIT_FRAMING, 0x80);        // 开始发送
    }

    i = 600;                                   // 根据时钟频率调整，操作M1卡最大等待时间25ms 2000?
    do
    {
        n = bk_mfrc522_read_rawRc(MFRC522_REG_COMIRQ);
        i--;
    }
    while((i != 0) && !(n & 0x01) && !(n & waitFor));
    bk_mfrc522_clear_bit_mask(MFRC522_REG_BIT_FRAMING, 0x80);
	
    MFRC522_LOGD("%s , %d ,i :0x%x, n :0x%x\r\n", __FUNCTION__,__LINE__,i, n);
	
    if(i != 0)
    {
        j = bk_mfrc522_read_rawRc(MFRC522_REG_ERROR);
        if(!(j & 0x1B))
        {
            status = MI_OK;
            if(n & irqEn & 0x01)
            {
                status = MI_NOTAGERR;
            }
            if(command == MFRC522_PCD_TRANSCEIVE)
            {
                n = bk_mfrc522_read_rawRc(MFRC522_REG_FIFO_LEVEL);
                lastBits = bk_mfrc522_read_rawRc(MFRC522_REG_CONTROL) & 0x07;
                if(lastBits)
                {
                    *pOutLenBit = (n - 1) * 8 + lastBits;
                }
                else
                {
                    *pOutLenBit = n * 8;
                }
                if(n == 0)
                {
                    n = 1;
                }
                if(n > MAXRLEN)
                {
                    n = MAXRLEN;
                }
                for(i = 0; i < n; i++)
                {
                    pOutData[i] = bk_mfrc522_read_rawRc(MFRC522_REG_FIFO_DATA);
                }
            }
        }
        else
        {
          status = MI_ERR;
        }
    }
    MFRC522_LOGD("%s , %d ,status:0x%x \r\n", __FUNCTION__,__LINE__,status);
    bk_mfrc522_set_bit_mask(MFRC522_REG_CONTROL, 0x80);               // stop timer now
    bk_mfrc522_write_rawRc(MFRC522_REG_COMMAND, MFRC522_PCD_IDLE);

    return status;
}

/**
@brief Enable the antenna (There must be at least a 1ms interval between each antenna transmission start or stop)
@return None
*/
void bk_mfrc522_antenna_on(void)
{
    uint8_t temp;
    temp = bk_mfrc522_read_rawRc(MFRC522_REG_TX_CONTROL);
    if(!(temp & 0x03))
    {
        bk_mfrc522_set_bit_mask(MFRC522_REG_TX_CONTROL, 0x03);
    }
}

/**
@brief Disable the antenna
@return None
*/
void bk_mfrc522_antenna_off(void)
{
    bk_mfrc522_clear_bit_mask(MFRC522_REG_TX_CONTROL, 0x03);
}

/**
@brief Set bits in the RC522 register
@param reg -[in] Register address
@param mask -[in] Bit pattern to set
@return None
*/
void bk_mfrc522_set_bit_mask(uint8_t reg, uint8_t mask)
{
    char temp = 0x00;
    temp = bk_mfrc522_read_rawRc(reg) | mask;
    bk_mfrc522_write_rawRc(reg, temp | mask);               // set bit mask
}

/**
@brief Clear bits in the RC522 register
@param reg -[in] Register address
@param mask -[in] Bit pattern to clear
@return None
*/
void bk_mfrc522_clear_bit_mask(uint8_t reg, uint8_t mask)
{
    char temp = 0x00;
    temp = bk_mfrc522_read_rawRc(reg) & (~mask);
    bk_mfrc522_write_rawRc(reg, temp);                      // clear bit mask
}

/**
 @brief NFC module initialization
 @param No parameters
 @return No return value
 */
void bk_nfc_init(void)
{ 
	mfrc522_gpio_write(MFRC522_RST_HIGH);
    mfrc522_uart_init();
    mfrc522_init();
	RC522_Config('A');
}

/**
 @brief NFC module initialization
 @param No parameters
 @return No return value
 */
void bk_nfc_deinit(void)
{ 
    mfrc522_deinit();
    mfrc522_uart_deinit();

}
/****************************************************END OF FILE****************************************************/