/*********************************************************************
 * INCLUDES
 */
#include "os/os.h"
#include "os/mem.h"
#include <string.h>
#include "board_mfrc522.h"
#include "components/bk_nfc.h"
#include "cli.h"
#include <driver/gpio.h>

extern bk_err_t bk_pm_module_vote_ctrl_external_ldo(gpio_ctrl_ldo_module_e module,gpio_id_t gpio_id,gpio_output_state_e value);
/**
 @brief 测试MFRC522的功能， Search for card --防碰撞---选定卡---验证卡片密码--写数据---读数据
*/
static int nfc_test_main(void)
{
	uint8_t i;
	uint8_t Card_Type1[2];
	uint8_t Card_ID[4];
	uint8_t Card_KEY[6] = {0xff,0xff,0xff,0xff,0xff,0xff};    //{0x11,0x11,0x11,0x11,0x11,0x11};
	uint8_t Card_Data[16];
	uint8_t status;

	MFRC522_LOGI("enter test !!!!!!!\n\r",__FUNCTION__);
	bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_NFC, CONFIG_LDO3V3_CTRL_GPIO, GPIO_OUTPUT_STATE_HIGH);
	bk_nfc_init();
	while(1)
	{
		rtos_delay_milliseconds(10);
		if(MI_OK==bk_mfrc522_request(MFRC522_PICC_REQALL, Card_Type1))
		{
			uint16_t cardType = (Card_Type1[0]<<8)|Card_Type1[1];
			MFRC522_LOGI("Card Type(0x%04X):",cardType);
			switch(cardType){
			case 0x4400:
					MFRC522_LOGI("Mifare UltraLight\n\r");
					break;
			case 0x0400:
					MFRC522_LOGI("Mifare One(S50)\n\r");
					break;
			case 0x0200:
					MFRC522_LOGI("Mifare One(S70)\n\r");
					break;
			case 0x0800:
					MFRC522_LOGI("Mifare Pro(X)\n\r");
					break;
			case 0x4403:
					MFRC522_LOGI("Mifare DESFire\n\r");
					break;
			default:
					MFRC522_LOGI("Unknown Card\n\r");
					continue;
			}
			//rtos_delay_milliseconds(10);
			status = bk_mfrc522_anticoll(Card_ID);//����ײ
			if(status != MI_OK){
				MFRC522_LOGE("Anticoll Error\n\r");
				continue;
			}else{
					MFRC522_LOGI("Serial Number:%02X%02X%02X%02X\n\r",Card_ID[0],Card_ID[1],Card_ID[2],Card_ID[3]);
			}
			status = bk_mfrc522_select(Card_ID);  //ѡ��
			if(status != MI_OK){
				MFRC522_LOGE("Select Card Error\n\r");
				continue;
			}
			MFRC522_LOGI("Select Card ok\n\r");
			status = bk_mfrc522_authState(MFRC522_PICC_AUTHENT1A,5,Card_KEY,Card_ID);
			if(status != MI_OK){
				MFRC522_LOGE("Auth State Error\n\r");
				continue;
			}
			MFRC522_LOGI("Auth State ok\n\r");
			memset(Card_ID,1,4);
			memset(Card_Data,1,16);
			Card_Data[0]=0xaa;
			status = bk_mfrc522_write(5,Card_Data);                   //д��0XAA,0X01,0X01����
			if(status != MI_OK){
				MFRC522_LOGE("Card Write Error\n\r");
				continue;
			}
			MFRC522_LOGI("Card Write ok\n\r");
			memset(Card_Data,0,16);
			rtos_delay_milliseconds(8);
			
			status = bk_mfrc522_read(5,Card_Data);                    //��һ�ΰ�����ȡ����16�ֽڵĿ�Ƭ����
			if(status != MI_OK){
				MFRC522_LOGE("Card Read Error\n\r");
				continue;
			}else{
				MFRC522_LOGI("Card Read ok\n\r");
				for(i=0;i<16;i++){
					MFRC522_LOGI("%02X ",Card_Data[i]);
				}
				MFRC522_LOGI("\n\r");
			}
			
			memset(Card_Data,2,16);
			Card_Data[0]=0xbb;
			rtos_delay_milliseconds(8);
			status = bk_mfrc522_write(5,Card_Data);                   //д��0Xbb,0X02,0X02����
			if(status != MI_OK){
				MFRC522_LOGE("Card Write Error\n\r");
				continue;
			}
			rtos_delay_milliseconds(8);
			MFRC522_LOGI("Card Write ok2\n\r");
			status = bk_mfrc522_read(5,Card_Data);                    //��һ�ΰ�����ȡ����16�ֽڵĿ�Ƭ����
			if(status != MI_OK){
				MFRC522_LOGE("Card Read Error\n\r");
				continue;
			}else{
				MFRC522_LOGI("Card read ok2\n\r");
				for(i=0;i<16;i++){
					MFRC522_LOGI("%02X ",Card_Data[i]);
				}
				MFRC522_LOGI("\n\r");
			}
			memset(Card_Data,0,16);
			bk_nfc_deinit();
			bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_NFC, CONFIG_LDO3V3_CTRL_GPIO, GPIO_OUTPUT_STATE_LOW);
		}
		return 0 ;
	}
}


static void cli_nfc_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	nfc_test_main();
}

static uint8_t s_card_id[4];

static void cli_nfc_cmd_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t Card_Type1[2];
	uint8_t Card_KEY[6] = {0xff,0xff,0xff,0xff,0xff,0xff};    //{0x11,0x11,0x11,0x11,0x11,0x11};
	uint8_t status;

	MFRC522_LOGI("enter cmd test !!!!!!!\n\r",__FUNCTION__);

	if (argc < 2)
	{
		MFRC522_LOGI("param is invaild !!!!!!!\n\r",__FUNCTION__);
		return;
	}
	bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_NFC, CONFIG_LDO3V3_CTRL_GPIO, GPIO_OUTPUT_STATE_HIGH);
	if (os_strcmp(argv[1], "init") == 0) {
		bk_nfc_init();
		MFRC522_LOGE("init ok\n\r");
	}else if (os_strcmp(argv[1], "request") == 0) {
		if(MI_OK==bk_mfrc522_request(MFRC522_PICC_REQALL, Card_Type1))
		{
			uint16_t cardType = (Card_Type1[0]<<8)|Card_Type1[1];
			MFRC522_LOGI("Card Type(0x%04X):",cardType);
			switch(cardType){
			case 0x4400:
					MFRC522_LOGI("Mifare UltraLight\n\r");
					break;
			case 0x0400:
					MFRC522_LOGI("Mifare One(S50)\n\r");
					MFRC522_LOGI("request ok\n\r");
					break;
			case 0x0200:
					MFRC522_LOGI("Mifare One(S70)\n\r");
					break;
			case 0x0800:
					MFRC522_LOGI("Mifare Pro(X)\n\r");
					break;
			case 0x4403:
					MFRC522_LOGI("Mifare DESFire\n\r");
					break;
			default:
					MFRC522_LOGI("Unknown Card\n\r");
			}
		}
	} else if (os_strcmp(argv[1], "anticoll") == 0) {
		status = bk_mfrc522_anticoll(s_card_id);
		if(status != MI_OK){
			MFRC522_LOGE("Anticoll Error\n\r");
		}else{
			MFRC522_LOGI("Serial Number:%02X%02X%02X%02X\n\r",s_card_id[0],s_card_id[1],s_card_id[2],s_card_id[3]);
			MFRC522_LOGI("Anticoll ok\n\r");
		}
	} else if(os_strcmp(argv[1], "select") == 0) {
		status = bk_mfrc522_select(s_card_id);
		if(status != MI_OK){
			MFRC522_LOGE("Select Card Error\n\r");
		}
		else{
			MFRC522_LOGI("Select Card ok\n\r");
	    }
	} else if(os_strcmp(argv[1], "authState") == 0) {
		status = bk_mfrc522_authState(MFRC522_PICC_AUTHENT1A,5,Card_KEY,s_card_id);
		if(status != MI_OK){
			MFRC522_LOGE("Auth State Error\n\r");
		}
		else{
			MFRC522_LOGI("Auth State ok\n\r");
		}
	} else if(os_strcmp(argv[1], "deinit") == 0){
		extern void nfc_delete_get_id_task(void);
		nfc_delete_get_id_task();
		bk_nfc_deinit();
		bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_NFC, CONFIG_LDO3V3_CTRL_GPIO, GPIO_OUTPUT_STATE_LOW);
	}else {

		return;
	}

}

static void cli_nfc_write_read_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t status;
	uint8_t i;
	uint8_t card_Data[16];
	uint8_t write_Data_val = 0;

	if(os_strcmp(argv[1], "write") == 0)
		write_Data_val = os_strtoul(argv[2], NULL, 10);

	MFRC522_LOGI("enter write_Data_val :0x%x !!!!!!!\n\r",__FUNCTION__,write_Data_val);
	if(os_strcmp(argv[1], "write") == 0)
	{
		memset(card_Data, write_Data_val, 16);
		//card_Data[0]=0xaa;
		status = bk_mfrc522_write(5,card_Data);
		if(status != MI_OK)
		{
			MFRC522_LOGE("Card Write Error\n\r");
		}
		MFRC522_LOGI("Card Write ok\n\r");
	}
	else if(os_strcmp(argv[1], "read") == 0)
	{
		memset(card_Data,0,16);
		rtos_delay_milliseconds(8);
		status = bk_mfrc522_read(5,card_Data);
		if(status != MI_OK)
		{
			MFRC522_LOGE("Card Read Error\n\r");
		}
		else
		{
			MFRC522_LOGI("Card Read ok\n\r");
			for(i=0;i<16;i++)
			{
				MFRC522_LOGI("%02X ",card_Data[i]);
			}
			MFRC522_LOGI("\n\r");
		}
   }
}



#define NFC_CMD_CNT (sizeof(s_nfc_commands) / sizeof(struct cli_command))
static const struct cli_command s_nfc_commands[] = {
	{"nfc_test", "nfc_test", cli_nfc_test},
	{"nfc_cmd_test", "nfc_cmd_test[init/request/anticoll/select/authState/write/read/deinit]", cli_nfc_cmd_test},
	{"nfc_write_test", "nfc_write_test[mode][value]", cli_nfc_write_read_test},
};

int cli_nfc_init(void)
{
	return cli_register_commands(s_nfc_commands, NFC_CMD_CNT);
}

