#include "os/os.h"
#include <components/cherryusb/usb_log.h>
#if (CONFIG_VFS)
#include "driver/flash_partition.h"
#include "bk_posix.h"
#endif

#if (CONFIG_FATFS)
static int _fs_mount(void)
{
    struct bk_fatfs_partition partition;
    char *fs_name = NULL;
    int ret;

    fs_name = "fatfs";
    partition.part_type = FATFS_DEVICE;
#if (CONFIG_SDCARD)
    partition.part_dev.device_name = FATFS_DEV_SDCARD;
#else
    partition.part_dev.device_name = FATFS_DEV_FLASH;
#endif
    partition.mount_path = VFS_SD_0_PATITION_0;

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

    return ret;
}
#endif

bk_err_t usb_vfs_init(void)
{
    bk_err_t ret = BK_FAIL;

    do {
        ret = _fs_mount();
        if (BK_OK != ret)
        {
            USB_LOG_WRN("[%s][%d] mount fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }

        USB_LOG_INFO("[%s][%d] mount success\r\n", __FUNCTION__, __LINE__);
    } while(0);

    return ret;
}

bk_err_t usb_vfs_deinit(void)
{
    bk_err_t ret = BK_FAIL;

    ret = umount(VFS_SD_0_PATITION_0);
    if (BK_OK != ret) {
        USB_LOG_WRN("[%s][%d] unmount fail:%d\r\n", __FUNCTION__, __LINE__, ret);
    }

    USB_LOG_INFO("[%s][%d] unmount success\r\n", __FUNCTION__, __LINE__);

    return ret;
}

