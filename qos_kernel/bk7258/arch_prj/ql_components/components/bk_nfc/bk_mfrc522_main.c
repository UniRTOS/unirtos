/*********************************************************************
 * INCLUDES
 */
#include "os/os.h"
#include "os/mem.h"
#include <string.h>
#include <components/log.h>
#include "board_uart.h"
#include "board_mfrc522.h"
#include "components/bk_nfc.h"
#include <driver/gpio.h>

#define NFC_INPUT_CARD_ID    (1)

extern bk_err_t bk_pm_module_vote_ctrl_external_ldo(gpio_ctrl_ldo_module_e module,gpio_id_t gpio_id,gpio_output_state_e value);
extern uint8_t bk_mfrc522_read_card_id(uid_num_t *uid,uint8_t validBits);
extern void bk_mfrc522_set_low_power(void);

static uint8_t s_fail_times = 0;
static uint8_t s_get_id_times = 0;
static beken_thread_t nfc_test_thread = NULL;

static nfc_event_callback_t s_nfc_event_callback = NULL;

int nfc_event_callback_register(nfc_event_callback_t callback)
{
	s_nfc_event_callback = callback;

	return 0;
}

int nfc_input_event_handler(uint8_t event_param, void *card_id)
{
	if(NULL != s_nfc_event_callback)
	{
		s_nfc_event_callback(event_param, card_id);
	}

	return 0;
}

static void nfc_repuire_card_task(void *arg)
{
    uint8_t Card_Type1[2];
    uint8_t status;
    uid_num_t uid; 

    bk_nfc_init();
    while(1)
    {
        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_NFC, CONFIG_LDO3V3_CTRL_GPIO, GPIO_OUTPUT_STATE_HIGH);
        mfrc522_init();
	    RC522_Config('A');
        if(bk_mfrc522_request(MFRC522_PICC_REQALL, Card_Type1) == 0)
        {
            MFRC522_LOGD("nfc request ok \r\n:");
            status =  bk_mfrc522_read_card_id(&uid, 0) ;
            if(status == MI_OK)
            {
                s_get_id_times ++;
                if(s_get_id_times == 1)
                {
                    nfc_input_event_handler(NFC_INPUT_CARD_ID, uid.uidByte);  // only post one times
                }
                MFRC522_LOGI("nfc post ok \r\n:");
            } 
            bk_mfrc522_set_low_power();
            bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_NFC, CONFIG_LDO3V3_CTRL_GPIO, GPIO_OUTPUT_STATE_LOW);
            rtos_delay_milliseconds(CONFIG_NFC_POLLING_PERIOD_MS);
        }
        else
        {   
            s_fail_times ++;
            if((s_fail_times == 1)&&(s_get_id_times > 0))
            {
                os_memset(uid.uidByte, 0, 7);
                nfc_input_event_handler(NFC_INPUT_CARD_ID, uid.uidByte);
            }
            MFRC522_LOGE("nfc request fail \r\n:");
            bk_mfrc522_set_low_power();
            bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_NFC, CONFIG_LDO3V3_CTRL_GPIO, GPIO_OUTPUT_STATE_LOW);
            s_fail_times   = 0;
            s_get_id_times = 0;
            rtos_delay_milliseconds(CONFIG_NFC_POLLING_PERIOD_MS);
        }
    }
}

void nfc_get_id_task(void)
{
   rtos_create_thread(&nfc_test_thread,
                    5,
                    "nfc_repuire_card_task",
                    nfc_repuire_card_task,
                    2048,
                    NULL);
}

void nfc_delete_get_id_task(void)
{
    if(nfc_test_thread)
    {
        rtos_delete_thread(&nfc_test_thread);
        nfc_test_thread = NULL;
    }
}
/****************************************************END OF FILE****************************************************/