#ifndef __BK_WANSON_ASR_INTF_H__
#define __BK_WANSON_ASR_INTF_H__

#include <os/os.h>

#if(CONFIG_WANSON_ASR_GROUP_VERSION)
/**
 * @brief 分组设置 
 * 
 * 当前设备没有播放音乐时，切换成分组1
 * 当设备需要播放音乐前，将其切换成分组2
 * 
 * @param group_id 分组ID
 */
void wanson_fst_group_change(unsigned char group_id);
#endif

/**
 * @brief ASR通用初始化函数
 * 
 * @return int 初始化结果
 */
int bk_wanson_asr_common_init(void);

/**
 * @brief ASR反初始化
 * 
 * 释放ASR资源
 */
void bk_wanson_asr_common_deinit(void);

/**
 * @brief ASR识别函数
 * 
 * @param read_buf 音频数据缓冲区
 * @param read_size 音频数据大小
 * @param p1 用户参数1
 * @param p2 用户参数2
 * @return int 识别结果
 */
int bk_wanson_asr_recog(void *read_buf, uint32_t read_size, void *p1, void *p2);

#endif /* __BK_WANSON_ASR_INTF_H__ */