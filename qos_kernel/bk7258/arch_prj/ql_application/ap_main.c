#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "cli.h"
#include <driver/gpio.h>
#include "gpio_driver.h"
#include "sys_driver.h"
#include "sys_hal.h"
#include <media_service.h>
#include "bk_posix.h"
#include <driver/flash.h>
#include <driver/flash_partition.h>
#define TAG "quectel"


extern uint32_t bk_misc_get_ap_reset_reason(void);
extern uint32_t bk_misc_get_cp_reset_reason(void);
extern void app_main(void);

#if (CONFIG_UNIRTOS_SUPPORT == 1)
extern void qosa_app_main(void * argv);
#endif

static bk_err_t init_littlefs()
{
    struct bk_little_fs_partition partition = {0};
    char *fs_name = NULL;
    bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_LITTLEFS);

    if (pt == NULL) {
        BK_LOGE(TAG,"%s[%d]\n", __func__, __LINE__);
        return BK_FAIL;
    }
    
    fs_name = "littlefs";
	partition.part_type = LFS_FLASH;
	partition.part_flash.start_addr = pt->partition_start_addr;
	partition.part_flash.size = pt->partition_length;
	partition.mount_path = "/";
    int ret = 0;
    BK_LOGI(TAG,"lfs start_addr:%p,size:%d\n", pt->partition_start_addr, pt->partition_length);

    if ((ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition)) != BK_OK) {
        if((ret = mkfs("PART_NONE", fs_name,&partition)) != BK_OK
          || (ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition)) != BK_OK){
            BK_LOGE(TAG, "mount failed!!\n");
            return BK_FAIL;
        }        
    }
    BK_LOGI(TAG,"mount successful!!\n");
    return BK_OK;
}

static void bk_enter_deepsleep(void)
{
    BK_LOGI(TAG,"RESET_SOURCE_FORCE_DEEPSLEEP\r\n");
   
    rtos_delay_milliseconds(100);
    bk_pm_ap_sleep_mode_set(PM_MODE_FORCE_DEEP_SLEEP);
    rtos_delay_milliseconds(10);
}


int main(void)
{
    if (bk_misc_get_ap_reset_reason() != RESET_SOURCE_FORCE_DEEPSLEEP)
    {
        bk_init();

        BK_LOGI(TAG, "reset_reason=AP:0x%02X CP:0x%02X\r\n", bk_misc_get_ap_reset_reason(), bk_misc_get_cp_reset_reason());

        media_service_init();

        init_littlefs();
        
        BK_LOGI(TAG,"quectel custom app start!\r\n");
#if (CONFIG_UNIRTOS_SUPPORT == 1)
#if CONFIG_CLI
        if (cli_mem_init() != 0)
        {
            BK_LOGE(TAG, "register memory CLI failed\r\n");
        }
#endif
        qosa_app_main(NULL);
#endif
    }
    else
    {
        bk_init();
        bk_enter_deepsleep();
    }
    return 0;
}
