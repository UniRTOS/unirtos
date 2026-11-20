#ifndef _BOARD_MFRC522_H_
#define _BOARD_MFRC522_H_
#include "os/os.h"
//#include <string.h>
//#include "uart_hal.h"
//#include <driver/uart.h>

#define MFRC522_TAG "mfrc522"
#define MFRC522_LOGI(...) BK_LOGI(MFRC522_TAG, ##__VA_ARGS__)
#define MFRC522_LOGW(...) BK_LOGW(MFRC522_TAG, ##__VA_ARGS__)
#define MFRC522_LOGE(...) BK_LOGE(MFRC522_TAG, ##__VA_ARGS__)
#define MFRC522_LOGD(...) BK_LOGD(MFRC522_TAG, ##__VA_ARGS__)

#define MAXRLEN                  (64)// 18

//******************************************************************/
// MFRC522命令字
//******************************************************************/
#define MFRC522_PCD_IDLE                  0x00           // 取消当前命令
#define MFRC522_PCD_AUTHENT               0x0E           // 验证密钥
#define MFRC522_PCD_RECEIVE               0x08           // 接收数据
#define MFRC522_PCD_TRANSMIT              0x04           // 发送数据
#define MFRC522_PCD_TRANSCEIVE            0x0C           // 发送并接收数据
#define MFRC522_PCD_RESETPHASE            0x0F           // 复位
#define MFRC522_PCD_CALCCRC               0x03           // CRC计算

//******************************************************************/
// Mifare_One卡片命令字
//******************************************************************/
#define MFRC522_PICC_REQIDL               0x26           // 寻天线区内未进入休眠状态
#define MFRC522_PICC_REQALL               0x52           // 寻天线区内全部卡
#define MFRC522_PICC_ANTICOLL1            0x93           // 防冲撞
#define MFRC522_PICC_ANTICOLL2            0x95           // 防冲撞
#define MFRC522_PICC_AUTHENT1A            0x60           // 验证A密钥
#define MFRC522_PICC_AUTHENT1B            0x61           // 验证B密钥
#define MFRC522_PICC_READ                 0x30           // 读块
#define MFRC522_PICC_WRITE                0xA0           // 写块
#define MFRC522_PICC_DECREMENT            0xC0           // 扣款
#define MFRC522_PICC_INCREMENT            0xC1           // 充值
#define MFRC522_PICC_RESTORE              0xC2           // 调块数据到缓冲区
#define MFRC522_PICC_TRANSFER             0xB0           // 保存缓冲区中数据
#define MFRC522_PICC_HALT                 0x50           // 休眠

#define DEF_FIFO_LENGTH           64             // FIFO size=64byte

/*page0*/
#define MFRC522_REG_PAGE0                 (unsigned char)0x00
#define MFRC522_REG_COMMAND               (unsigned char)0x01        /**< starts and stops command execution register */
#define MFRC522_REG_COMIEN                (unsigned char)0x02        /**< enable and disable interrupt request control bits register */
#define MFRC522_REG_DIVIEN                (unsigned char)0x03        /**< enable and disable interrupt request control bits register */
#define MFRC522_REG_COMIRQ                (unsigned char)0x04        /**< interrupt request bits register */
#define MFRC522_REG_DIVIRQ                (unsigned char)0x05        /**< interrupt request bits register */
#define MFRC522_REG_ERROR                 (unsigned char)0x06        /**< error bits showing the error status of the last command executed register */
#define MFRC522_REG_STATUS1               (unsigned char)0x07        /**< communication status bits register */
#define MFRC522_REG_STATUS2               (unsigned char)0x08        /**< receiver and transmitter status bits register */
#define MFRC522_REG_FIFO_DATA             (unsigned char)0x09        /**< input and output of 64 byte FIFO buffer register */
#define MFRC522_REG_FIFO_LEVEL            (unsigned char)0x0A        /**< number of bytes stored in the FIFO buffer register */
#define MFRC522_REG_WATER_LEVEL           (unsigned char)0x0B        /**< level for FIFO underflow and overflow warning register */
#define MFRC522_REG_CONTROL               (unsigned char)0x0C        /**< miscellaneous control registers register */
#define MFRC522_REG_BIT_FRAMING           (unsigned char)0x0D        /**< adjustments for bit-oriented frames register */
#define MFRC522_REG_COLL                  (unsigned char)0x0E        /**< bit position of the first bit-collision detected on the RF interface register */
#define MFRC522_REG_RFU0F                 (unsigned char)0x0F

/*page1*/
#define MFRC522_REG_PAGE1                 (unsigned char)0x10
#define MFRC522_REG_MODE                  (unsigned char)0x11        /**< defines general modes for transmitting and receiving register */
#define MFRC522_REG_TX_MODE               (unsigned char)0x12        /**< defines transmission data rate and framing register */
#define MFRC522_REG_RX_MODE               (unsigned char)0x13        /**< defines reception data rate and framing register */
#define MFRC522_REG_TX_CONTROL            (unsigned char)0x14        /**< controls the logical behavior of the antenna driver pins TX1 and TX 2 register */
#define MFRC522_REG_TX_ASK                (unsigned char)0x15        /**< controls the setting of the transmission modulation register */
#define MFRC522_REG_TX_SEL                (unsigned char)0x16        /**< selects the internal sources for the antenna driver register */
#define MFRC522_REG_RX_SEL                (unsigned char)0x17        /**< selects internal receiver settings register */
#define MFRC522_REG_RX_THRESHOLD          (unsigned char)0x18        /**< selects thresholds for the bit decoder register */
#define MFRC522_REG_DEMOD                 (unsigned char)0x19        /**< defines demodulator settings register */
#define MFRC522_REG_RFU1A                 (unsigned char)0x1A
#define MFRC522_REG_RFU1B                 (unsigned char)0x1B
#define MFRC522_REG_MIFNFC                (unsigned char)0x1C        /**< controls some MIFARE communication transmit parameters register */
#define MFRC522_REG_MANUALRCV             (unsigned char)0x1D        /**< controls some MIFARE communication receive parameters register */
#define MFRC522_REG_TYPEB                 (unsigned char)0x1E 
#define MFRC522_REG_SERIAL_SPEED          (unsigned char)0x1F        /**< selects the speed of the serial UART interface register */

/*page2*/
#define MFRC522_REG_PAGE2                 (unsigned char)0x20
#define MFRC522_REG_CRC_RESULT_H          (unsigned char)0x21        /**< shows the MSB and LSB values of the CRC calculation high register */
#define MFRC522_REG_CRC_RESULT_L          (unsigned char)0x22        /**< shows the MSB and LSB values of the CRC calculation low register */
#define MFRC522_REG_GSN_OFF               (unsigned char)0x23
#define MFRC522_REG_MOD_WIDTH             (unsigned char)0x24        /**< controls the ModWidth setting register */
#define MFRC522_REG_TXBIT_PHASE           (unsigned char)0x25
#define MFRC522_REG_RF_CFG                (unsigned char)0x26        /**< configures the receiver gain register */
#define MFRC522_REG_GSN_ON                (unsigned char)0x27        /**< selects the conductance of the antenna driver pins TX1 and TX2 for modulation register */
#define MFRC522_REG_CW_GSP                (unsigned char)0x28        /**< defines the conductance of the p-driver output during periods of no modulation register */
#define MFRC522_REG_MOD_GSP               (unsigned char)0x29        /**< defines the conductance of the p-driver output during periods of modulation register */
#define MFRC522_REG_TMODE                 (unsigned char)0x2A        /**< defines settings for the internal timer register */
#define MFRC522_REG_TPRESCALER            (unsigned char)0x2B        /**< defines settings for the internal timer register */
#define MFRC522_REG_TRELOAD_H             (unsigned char)0x2C        /**< defines the 16-bit timer reload value high register */
#define MFRC522_REG_TRELOAD_L             (unsigned char)0x2D        /**< defines the 16-bit timer reload value low register */
#define MFRC522_REG_TCOUNTER_VAL_H        (unsigned char)0x2E        /**< shows the 16-bit timer value high register */
#define MFRC522_REG_TCOUNTER_VAL_L        (unsigned char)0x2F        /**< shows the 16-bit timer value low register */

/*page3*/
#define MFRC522_REG_PAGE3                 (unsigned char)0x30
#define MFRC522_REG_TEST_SEL1             (unsigned char)0x31        /**< general test signal configuration register */
#define MFRC522_REG_TEST_SEL2             (unsigned char)0x32        /**< general test signal configuration and PRBS control register */
#define MFRC522_REG_TEST_PIN_EN           (unsigned char)0x33        /**< enables pin output driver on pins D1 to D7 register */
#define MFRC522_REG_TEST_PIN_VALUE        (unsigned char)0x34        /**< defines the values for D1 to D7 when it is used as an I/O bus register */
#define MFRC522_REG_TEST_BUS              (unsigned char)0x35        /**< shows the status of the internal test bus register */
#define MFRC522_REG_AUTO_TEST             (unsigned char)0x36        /**< controls the digital self test register */
#define MFRC522_REG_VERSION               (unsigned char)0x37        /**< shows the software version register */
#define MFRC522_REG_ANALOG_TEST           (unsigned char)0x38        /**< controls the pins AUX1 and AUX2 register */
#define MFRC522_REG_TEST_DAC1             (unsigned char)0x39        /**< defines the test value for TestDAC1 register */
#define MFRC522_REG_TEST_DAC2             (unsigned char)0x3A        /**< defines the test value for TestDAC2 register */
#define MFRC522_REG_TEST_ADC              (unsigned char)0x3B        /**< shows the value of ADC I and Q channels register */
#define MFRC522_REG_RFU3C                 (unsigned char)0x3C   
#define MFRC522_REG_RFU3D                 (unsigned char)0x3D   
#define MFRC522_REG_RFU3E                 (unsigned char)0x3E   
#define MFRC522_REG_RFU3F                 (unsigned char)0x3F

#define MI_OK                             (char)0
#define MI_NOTAGERR                       (char)(-1)
#define MI_ERR                            (char)(-2)

#define MFRC522_REG_STATUS2_VAL           (0x08)
#define MFRC522_REG_MODE_VAL	          (0x3D)
#define MFRC522_REG_RX_SEL_VAL	          (0x86)
#define MFRC522_REG_RF_CFG_VAL	          (0x7F)
#define MFRC522_REG_TRELOAD_L_VAL         (30)
#define MFRC522_REG_TRELOAD_H_VAL	      (0)
#define MFRC522_REG_TMODE_VAL             (0x8D)
#define MFRC522_REG_TPRESCALER_VAL        (0x3E)

#define bk_mfrc522_read_rawRc(addr)                mfrc522_read_rawRc(addr)
#define bk_mfrc522_write_rawRc(addr, writeData)    mfrc522_write_rawRc(addr, writeData)

typedef enum 
{
    MI_STATUS_OK = 0            ,
    MI_STATUS_ERROR	            ,
    MI_STATUS_COLLISION         ,
    MI_STATUS_TIMEOUT           ,
    MI_STATUS_NO_ROOM           ,
    MI_STATUS_INTERNAL_ERROR    ,
    MI_STATUS_INVALID           ,
    MI_STATUS_CRC_WRONG         ,
    MI_STATUS_MIFARE_NACK = 0xff,
}MI_status_code_t;

// The commands used by the PCD to manage communication with several PICCs (ISO 14443-3, Type A, section 6.4)
typedef enum  {
    MFRC522_PICC_CMD_REQA           = 0x26,		// REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
    MFRC522_PICC_CMD_WUPA           = 0x52,		// Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
    MFRC522_PICC_CMD_CT             = 0x88,		// Cascade Tag. Not really a command, but used during anti collision.
    MFRC522_PICC_CMD_SEL_CL1        = 0x93,		// Anti collision/Select, Cascade Level 1
    MFRC522_PICC_CMD_SEL_CL2        = 0x95,		// Anti collision/Select, Cascade Level 2
    MFRC522_PICC_CMD_SEL_CL3        = 0x97,		// Anti collision/Select, Cascade Level 3
    MFRC522_PICC_CMD_HLTA           = 0x50,		// HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.
    MFRC522_PICC_CMD_RATS           = 0xE0,		// Request command for Answer To Reset.
    MFRC522_PICC_CMD_MF_AUTH_KEY_A  = 0x60,		// Perform authentication with Key A
    MFRC522_PICC_CMD_MF_AUTH_KEY_B  = 0x61,		// Perform authentication with Key B
    MFRC522_PICC_CMD_MF_READ        = 0x30,		// Reads one 16 byte block from the authenticated sector of the PICC. Also used for MIFARE Ultralight.
    MFRC522_PICC_CMD_MF_WRITE       = 0xA0,		// Writes one 16 byte block to the authenticated sector of the PICC. Called "COMPATIBILITY WRITE" for MIFARE Ultralight.
    MFRC522_PICC_CMD_MF_DECREMENT   = 0xC0,		// Decrements the contents of a block and stores the result in the internal data register.
    MFRC522_PICC_CMD_MF_INCREMENT   = 0xC1,		// Increments the contents of a block and stores the result in the internal data register.
    MFRC522_PICC_CMD_MF_RESTORE     = 0xC2,		// Reads the contents of a block into the internal data register.
    MFRC522_PICC_CMD_MF_TRANSFER    = 0xB0,		// Writes the contents of the internal data register to a block.
    MFRC522_PICC_CMD_UL_WRITE       = 0xA2,		// Writes one 4 byte page to the PICC.
}MFRC522_PICC_Command_t;

typedef struct {
	uint8_t		size;			// Number of bytes in the UID. 4, 7 or 10.
	uint8_t		uidByte[10];
	uint8_t		sak;			// The SAK (Select acknowledge) byte returned from the PICC after successful selection.
} uid_num_t;

/**
 @brief reset RC522
 @return None
*/
void bk_mfrc522_reset(void);

/** 
@brief Calculate CRC16 using MF522
@param pInData -[in] Array to calculate CRC16
@param len -[in] Byte length of the array to calculate CRC16
@param pOutData -[out] First address where the result is stored
@return None
*/
void bk_mfrc522_calulate_crc(uint8_t *pInData, uint8_t len, uint8_t *pOutData);

/** @brief Communicates with an ISO14443 card using the MFRC522 module
@param command -[in] RC522 command word
@param pInData -[in] Data sent to the card via RC522
@param inLenByte -[in] Length of the sent data in bytes
@param pOutData -[out] Data received from the card
@param pOutLenBit -[out] Length of the received data in bits
@return Status value, MI_OK - Success; MI_ERR - Failure
*/
char bk_mfrc522_com_transceive(uint8_t command, uint8_t *pInData, uint8_t inLenByte, uint8_t *pOutData, uint32_t *pOutLenBit);

/**
@brief Enable the antenna (There must be at least a 1ms interval between each antenna transmission start or stop)
@return None
*/
void bk_mfrc522_antenna_on(void);

/**
@brief Disable the antenna
@return None
*/
void bk_mfrc522_antenna_off(void);

/**
@brief Set bits in the RC522 register
@param reg -[in] Register address
@param mask -[in] Bit pattern to set
@return None
*/
void bk_mfrc522_set_bit_mask(uint8_t reg, uint8_t mask);

/**
@brief Clear bits in the RC522 register
@param reg -[in] Register address
@param mask -[in] Bit pattern to clear
@return None
*/
void bk_mfrc522_clear_bit_mask(uint8_t reg, uint8_t mask);
void RC522_Config(unsigned char Card_Type);
void mfrc522_init(void);
char PcdHalt(void);
#endif /* _BOARD_MFRC522_H_ */