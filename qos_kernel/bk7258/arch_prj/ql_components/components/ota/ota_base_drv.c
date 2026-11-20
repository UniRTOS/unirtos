#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if (CONFIG_REMOTE_VFS_CLIENT || CONFIG_VFS)
#include "bk_posix.h"
#endif
#include "bk_ota_private.h"
#ifdef CONFIG_TASK_WDT
#include "bk_wdt.h"
#endif
#if CONFIG_INT_WDT
#include <driver/wdt.h>
#endif
#ifdef CONFIG_HTTP_AB_PARTITION
#include "modules/ota.h"
#include "driver/flash_partition.h"
extern part_flag update_part_flag;
#endif

#ifdef CONFIG_HTTP_OTA_WITH_BLE
typedef void (*ble_sleep_state_cb)(uint8_t is_sleeping, uint32_t slp_period);
extern void bk_ble_register_sleep_state_callback(ble_sleep_state_cb cb);
#endif
extern void ble_sleep_cb(uint8_t is_sleeping, uint32_t slp_period);
extern int ble_callback_deal_handler(uint32_t deal_flash_time);

static int ota_do_init(f_ota_t* ota_ptr)
{
    OTA_CHECK_POINTER(ota_ptr);

    OTA_MALLOC(ota_ptr->wr_buf, OTA_FLASH_BUFFER_LENGTH);
    OTA_MALLOC(ota_ptr->wr_tmp_buf, OTA_TEMP_FLASH_BUFFER_LENGTH);
    OTA_MALLOC(ota_ptr->rd_buf, OTA_FLASH_BUFFER_LENGTH);

#ifdef CONFIG_HTTP_AB_PARTITION
    exec_flag ret = bk_ota_get_current_partition();
    OTA_LOGI("ret :0x%x \r\n",ret);
    if(ret == EXEC_B_PART){
        update_part_flag = UPDATE_A_PART;
    } else{
        update_part_flag = UPDATE_B_PART;
    }

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
	ota_ptr->partition_length = http_get_sapp_partition_length(BK_PARTITION_S_APP);
    OTA_LOGI("ota_ptr->partition_lenth :0x%x \r\n",ota_ptr->partition_length);
    if(update_part_flag == UPDATE_B_PART){
        OTA_LOGI("UPDATE_B_PART\r\n");
        ota_ptr->pt = bk_flash_partition_get_info(BK_PARTITION_S_APP); //update B_parition
    }else{
        OTA_LOGI("UPDATE_A_PART\r\n");
        ota_ptr->pt = bk_flash_partition_get_info(BK_PARTITION_APPLICATION);//update A_parition.
    }
#else
    ota_ptr->pt = bk_flash_partition_get_info(BK_PARTITION_S_APP);
#endif
#else
    ota_ptr->pt = bk_flash_partition_get_info(BK_PARTITION_OTA);
#endif

    ota_ptr->new_sequence_number  = 0;
    ota_ptr->curr_sequence_number = 0;
    ota_ptr->received_total_size  = 0;
    ota_ptr->wr_last_len          = 0;
    ota_ptr->wr_flash_flag        = 0;
    ota_ptr->ota_crc.crc          = 0xFFFFFFFF;
    ota_ptr->wr_address           = ota_ptr->pt->partition_start_addr;
    ota_ptr->protect_type         = bk_flash_get_protect_type();
    bk_flash_set_protect_type(FLASH_PROTECT_NONE);
    ota_ptr->fd                   = -1;
    ota_ptr->init_flag            = 1;
    OTA_LOGD("ota write to :0x%x \r\n", ota_ptr->wr_address);

#ifdef CONFIG_HTTP_OTA_WITH_BLE
#if CONFIG_BLUETOOTH
        bk_ble_register_sleep_state_callback(ble_sleep_cb);
#endif
#endif

    return BK_OK;
}

static int ota_do_write_flash(f_ota_t* ota_ptr, uint16_t len)
{
    uint32_t  anchor_time = 0;
    uint32_t  temp_time = 0;
    uint8_t   flash_erase_ready = 0;

    OTA_CHECK_POINTER(ota_ptr);

    if (ota_ptr->wr_address % FLASH_SECTOR_SIZE == 0)
    {
        anchor_time = rtos_get_time();
        while(1)
        {
            flash_erase_ready = ble_callback_deal_handler(ERASE_FLASH_TIMEOUT);

            temp_time = rtos_get_time();
            if(temp_time >= anchor_time)
            {
                temp_time -= anchor_time;
            }
            else
            {
                temp_time += (0xFFFFFFFF - anchor_time);
            }

            if(temp_time >= ERASE_TOUCH_TIMEOUT)
            {
                flash_erase_ready = 1;
            }

            if(flash_erase_ready == 1)
            {
#if CONFIG_OTA_POSITION_INDEPENDENT_AB
                if((len != 0) && (((u32)ota_ptr->wr_address + len) <= (ota_ptr->pt->partition_start_addr + ota_ptr->partition_length)))
#else
                if((len != 0) && (((u32)ota_ptr->wr_address + len) <= (ota_ptr->pt->partition_start_addr + ota_ptr->pt->partition_length)))
#endif
                {
                    bk_flash_erase_sector(ota_ptr->wr_address);
                }
                flash_erase_ready = 0;
                break;
            }
            else
            {
                rtos_delay_milliseconds(2);
            }
       }
    }

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
        if (((u32)ota_ptr->wr_address >= ota_ptr->pt->partition_start_addr)
        && (((u32)ota_ptr->wr_address + len) <= (ota_ptr->pt->partition_start_addr + ota_ptr->partition_length)))
#else
        if (((u32)ota_ptr->wr_address >= ota_ptr->pt->partition_start_addr)
        && (((u32)ota_ptr->wr_address + len) <= (ota_ptr->pt->partition_start_addr + ota_ptr->pt->partition_length)))
#endif
        {
            while(1)
            {
                flash_erase_ready = ble_callback_deal_handler(WRITE_FLASH_TIMEOUT);

                temp_time = rtos_get_time();
                if(temp_time >= anchor_time)
                {
                    temp_time -= anchor_time;
                }
                else
                {
                    temp_time += (0xFFFFFFFF - anchor_time);
                }

                if(temp_time >= ERASE_TOUCH_TIMEOUT)
                    flash_erase_ready = 1;

                if(flash_erase_ready == 1)
                {
                    bk_flash_write_bytes(ota_ptr->wr_address, (uint8_t *)ota_ptr->wr_buf, len);

                    if (ota_ptr->rd_buf) {
                        bk_flash_read_bytes(ota_ptr->wr_address, (uint8_t *)ota_ptr->rd_buf, len);
                        if (!os_memcmp(ota_ptr->wr_buf, ota_ptr->rd_buf, len)) {
                        } else{
                            OTA_LOGE("wr flash write err\n");
                            return BK_FAIL;
                        }
                    }
                    ota_ptr->wr_flash_flag = 1;
                    ota_ptr->wr_address += len;
                    os_memset(ota_ptr->wr_buf, 0, len);
                    flash_erase_ready = 0;
                    break;
                }
                else
        		{
                    rtos_delay_milliseconds(2);
                }
        	}
        }
    return BK_OK;
}

static int ota_do_process_data(f_ota_t* ota_ptr, uint16_t len, ota_update_type_t ota_type, ota_wr_callback wr_callback)
{
    int ret = BK_FAIL;
    uint32_t write_len = 0, i = 0;

    OTA_CHECK_POINTER(ota_ptr);
    OTA_CHECK_POINTER(ota_ptr->wr_tmp_buf);
    OTA_CHECK_POINTER(ota_ptr->wr_buf);

#if (CONFIG_TASK_WDT)
    bk_task_wdt_feed();
#endif
#if CONFIG_TASK_WDT
    extern void bk_task_wdt_feed(void);
    bk_task_wdt_feed();
#endif

    if(ota_type == OTA_TYPE_WIFI)
    {
        OTA_LOGD("wr_addr:0x%x, len :0x%x \r\n", ota_ptr->wr_address,len);
        while (i < len)
        {
            write_len = MIN(len - i, (OTA_FLASH_BUFFER_LENGTH - ota_ptr->wr_last_len));
            os_memcpy((ota_ptr->wr_buf + ota_ptr->wr_last_len), (ota_ptr->wr_tmp_buf + i), write_len);

            i += write_len;
            ota_ptr->wr_last_len += write_len;
            ota_ptr->received_total_size += write_len;
            OTA_LOGD("received_size:0x%x,image_size :0x%x, wr_last_len :0x%x \r\n", ota_ptr->received_total_size, ota_ptr->image_size, ota_ptr->wr_last_len);
            OTA_LOGI("cyg_recvlen_per:(%.2f)%%\r\n",(((float)(ota_ptr->received_total_size))/(ota_ptr->image_size))*100);

            if(ota_ptr->received_total_size == ota_ptr->image_size)    /*the last package*/
            {
                while(ota_ptr->wr_last_len > OTA_FLASH_BUFFER_LENGTH)
                {
                    if(wr_callback(ota_ptr, OTA_FLASH_BUFFER_LENGTH) == BK_OK)
                    {
                        OTA_LOGI("wr the last 1k data \r\n");
                        ota_ptr->wr_last_len -= OTA_FLASH_BUFFER_LENGTH;
                    }
                    else
                    {
                        OTA_LOGE("wr the last 1k data fail \r\n");
                        return BK_FAIL;
                    }
                }

                if(wr_callback(ota_ptr, ota_ptr->wr_last_len) == BK_OK)
                {
                    OTA_LOGI("wr the last remain data \r\n");
                    ret = BK_OK;
                }
                else
                {
                    OTA_LOGE("wr the last remain data fail \r\n");
                    ret = BK_FAIL;
                }
            }
            else    /*not the last package*/
            {
                if (ota_ptr->wr_last_len >= OTA_FLASH_BUFFER_LENGTH)
                {
                    if(wr_callback(ota_ptr, OTA_FLASH_BUFFER_LENGTH) == BK_OK)
                    {
                        OTA_LOGD("wr 1k data \r\n");
                        ota_ptr->wr_last_len = 0;
                        ret = BK_OK;
                    }
                    else
                    {
                        OTA_LOGE("wr 1k data fail \r\n");
                        ret = BK_FAIL;
                    }
                }
                else
                {
                    OTA_LOGD("do adding data \r\n");
                    ret = BK_OK;
                }
            }
        }
    }
    else if(ota_type == OTA_TYPE_BLE)
    {
        OTA_LOGV("wr_addr:0x%x, new_seq:0x%x, curr_seq :0x%x,len :0x%x \r\n", ota_ptr->wr_address,ota_ptr->new_sequence_number, ota_ptr->curr_sequence_number,len);
        if(ota_ptr->curr_sequence_number != ota_ptr->new_sequence_number)
        {
            while (i < len)
            {
                write_len = MIN(len - i, (OTA_FLASH_BUFFER_LENGTH - ota_ptr->wr_last_len));
                OTA_LOGV("write_len:0x%x \r\n", write_len);
                os_memcpy((ota_ptr->wr_buf + ota_ptr->wr_last_len), (ota_ptr->wr_tmp_buf + i), write_len);
                ota_ptr->curr_sequence_number = ota_ptr->new_sequence_number;
                OTA_LOGV("ota_ptr->wr_buf:0x%x--0x%x--0x%x--0x%x--0x%x---0x%x \r\n", ota_ptr->wr_buf[0],ota_ptr->wr_buf[1],ota_ptr->wr_buf[2],ota_ptr->wr_buf[3],ota_ptr->wr_buf[4],ota_ptr->wr_buf[5]);

                i += write_len;
                ota_ptr->wr_last_len += write_len;
                ota_ptr->received_total_size += write_len;
                OTA_LOGV("ota_ptr->received_total_size:0x%x, ota_ptr->wr_last_len :0x%x \r\n", ota_ptr->received_total_size, ota_ptr->wr_last_len);
                OTA_LOGD("cyg_recvlen_per:(%.2f)%%\r\n",(((float)(ota_ptr->received_total_size))/(ota_ptr->image_size))*100);
                if(ota_ptr->received_total_size == ota_ptr->image_size)    /*the last package*/
                {
                    ota_ptr->wr_flash_flag = 0;
                    if(ota_do_write_flash(ota_ptr, ota_ptr->wr_last_len) == BK_OK)
                    {
                        OTA_LOGD("wr the last data \r\n");
                        return  BK_OK;
                    }
                }
                else    /*not the last package*/
                {
                    if (ota_ptr->wr_last_len >= OTA_FLASH_BUFFER_LENGTH)
                    {
                        if(ota_do_write_flash(ota_ptr, OTA_FLASH_BUFFER_LENGTH) == BK_OK)
                        {
                            OTA_LOGV("wr 1k data \r\n");
                            ret = BK_OK;
                        }
                        ota_ptr->wr_last_len = 0;
                    }
                    else
                    {
                        OTA_LOGV("do adding data \r\n");
                        ret = BK_OK;
                    }
                }
            }
        }
        else
        {
            OTA_LOGD("exception logic ota_ptr->wr_last_len :0x%x, len :0x%x ,ota_ptr->wr_address :0x%x\r\n", ota_ptr->wr_last_len, len, ota_ptr->wr_address);
            if(ota_ptr->wr_last_len < len)
            {
                OTA_LOGW("the wr_buf has exceeded 1024 bytes \r\n");
                // /*need read flash out firstly , repackage and write*/
                uint8_t *retry_wr_bufer = NULL;
                uint32_t retry_len = ota_ptr->wr_address % FLASH_SECTOR_SIZE;
                OTA_MALLOC(retry_wr_bufer, FLASH_SECTOR_SIZE);
                OTA_LOGD("ota_ptr->wr_address :0x%x, retry_len :0x%x \r\n", ota_ptr->wr_address, retry_len);

                bk_flash_read_bytes((ota_ptr->wr_address - retry_len), retry_wr_bufer, retry_len);
                if(retry_len >len)
                {
                    os_memcpy(&retry_wr_bufer[retry_len - len], ota_ptr->wr_tmp_buf, len);
                    bk_flash_erase_sector(ota_ptr->wr_address - retry_len);
                    bk_flash_write_bytes((ota_ptr->wr_address - retry_len), retry_wr_bufer, retry_len);
                    ret = BK_OK;
                }
                else
                {
                    ret = BK_FAIL;
                    OTA_LOGE("the input len is exception \r\n");
                }
                OTA_FREE(retry_wr_bufer);

            }
            else /*ota_ptr->wr_last_len >= len*/
            {
                OTA_LOGW("when the wr_buf is not exceed 1024 bytes \r\n");
                ota_ptr->wr_last_len -= len;
                os_memcpy(ota_ptr->wr_buf + ota_ptr->wr_last_len, ota_ptr->wr_tmp_buf, len);
                ota_ptr->wr_last_len += len;
                ret = BK_OK;
            }
        }
    }

    return ret;
}

static int ota_do_check_crc(f_ota_t* ota_ptr,uint32_t in_crc)
{
    OTA_CHECK_POINTER(ota_ptr);
    uint32_t start_addr = ota_ptr->pt->partition_start_addr;
    uint32_t end_addr   = (start_addr + ota_ptr->image_size);
    uint32_t out_crc;
    int i =0;

    OTA_LOGE("in_crc :0x%x, in_crc,start_addr :0x%x,end_addr:0x%x \r\n", in_crc,start_addr,end_addr);
    CRC32_Init(&ota_ptr->ota_crc);
    os_memset(ota_ptr->rd_buf, 0, OTA_CRC_BUFFER_LENGTH);

    for( i =0; i<= (ota_ptr->image_size - OTA_CRC_BUFFER_LENGTH); i+= OTA_CRC_BUFFER_LENGTH)
    {
        bk_flash_read_bytes((start_addr + i), ota_ptr->rd_buf, OTA_CRC_BUFFER_LENGTH);
        CRC32_Update(&ota_ptr->ota_crc, ota_ptr->rd_buf, OTA_CRC_BUFFER_LENGTH);
    }
    OTA_LOGV("i :0x%x \r\n", i);
    if (i != ota_ptr->image_size -OTA_CRC_BUFFER_LENGTH)
    {
        uint32_t remain_size = ota_ptr->image_size - i;
        bk_flash_read_bytes((start_addr + i), ota_ptr->rd_buf, remain_size);
        CRC32_Update(&ota_ptr->ota_crc, ota_ptr->rd_buf, remain_size);
    }

    CRC32_Final(&ota_ptr->ota_crc,&out_crc);
    OTA_LOGD("ota_ptr->ota_crc.crc :0x%x,out_crc :0x%x\r\n", ota_ptr->ota_crc.crc,out_crc);
    if(out_crc != in_crc)
    {
        OTA_LOGE("crc error\r\n");
        return BK_FAIL;
    }
    else
    {   
        OTA_LOGE("crc ok\r\n");
        return BK_OK;
    }
}

#if (CONFIG_REMOTE_VFS_CLIENT || CONFIG_VFS)
static int ota_do_open_file(f_ota_t* ota_ptr)
{
    ota_ptr->fd = open(ota_ptr->dest_path_name, (O_RDWR | O_CREAT ));
    if(ota_ptr->fd < 0)
    {
        OTA_LOGE("%s ota_ptr->fd :%d \r\n", __FUNCTION__, ota_ptr->fd);
        return BK_FAIL;
    }
    else
    {
        return BK_OK;
    }
}

static int ota_do_read_file(f_ota_t* ota_ptr, uint16_t len)
{
    int ret = BK_FAIL;

    ret = read(ota_ptr->fd, ota_ptr->rd_buf, len);
    if(ret == len)
        ret = BK_OK;
    else
        ret = BK_FAIL;

    return ret;
}

static int ota_do_write_file(f_ota_t* ota_ptr, uint16_t len)
{
    int ret = BK_FAIL;

    ret = write(ota_ptr->fd, ota_ptr->wr_buf, len);
    if (ret == len)
    {
        // ret = ota_do_read_file(ota_ptr, len);
        // if (!os_memcmp(ota_ptr->wr_buf, ota_ptr->rd_buf, len))
        // {
        //     os_memset(ota_ptr->wr_buf, 0, len);
        //     ret = BK_OK;
        // } 
        // else
        // {
        //     OTA_LOGE("wr flash write err\n");
        //     ret = BK_FAIL;
        // }
        ret = BK_OK;
    }
    else
    {
        ret = BK_FAIL;
    }

    return ret;
}

static void ota_do_mount(f_ota_t* ota_ptr)
{

}

static void ota_do_umount(f_ota_t* ota_ptr)
{
    umount("/");
}

static int ota_do_seek_file(f_ota_t* ota_ptr, uint16_t len)
{
    return BK_OK;
}

static int ota_do_close_file(f_ota_t* ota_ptr)
{
    close(ota_ptr->fd);

    return BK_OK;
}
#endif

static int ota_do_deinit(f_ota_t* ota_ptr)
{
    OTA_CHECK_POINTER(ota_ptr);

    OTA_FREE(ota_ptr->wr_buf);
    OTA_FREE(ota_ptr->wr_tmp_buf);
    OTA_FREE(ota_ptr->rd_buf);
    bk_flash_set_protect_type(ota_ptr->protect_type);
#if (CONFIG_REMOTE_VFS_CLIENT || CONFIG_VFS)
    if(ota_ptr->fd > 0)
    {
        ota_do_close_file(ota_ptr);
    }
#endif
    ota_ptr->init_flag = 0;

    return BK_OK;
}

static const f_ota_func_t s_ota_fun ={
    .init         = ota_do_init,
    .wr_flash     = ota_do_write_flash,
    .data_process = ota_do_process_data,
    .crc          = ota_do_check_crc,
    .deinit       = ota_do_deinit,
#if (CONFIG_REMOTE_VFS_CLIENT || CONFIG_VFS)
    .mount        = ota_do_mount,
    .open         = ota_do_open_file,
    .seek         = ota_do_seek_file,
    .read         = ota_do_read_file,
    .write        = ota_do_write_file,
    .umount       = ota_do_umount,
    .close        = ota_do_close_file,
#endif
};

const f_ota_func_t *f_ota_fun_ptr = &s_ota_fun;
