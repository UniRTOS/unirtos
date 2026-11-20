#include <os/os.h>

#include <common/bk_include.h>
#include <components/bk_audio_asr_service.h>
#include <components/bk_asr_service_types.h>
#include <components/bk_asr_service.h>

#include "asr.h"

#if(CONFIG_WANSON_ASR_GROUP_VERSION)
/* 分组相关全局变量 */
Fst fst_1;
Fst fst_2;
static unsigned char asr_curr_group_id; // 当前使用的分组ID
static uint8_t __maybe_unused wanson_fst_group_select = 0;

/**
 * @brief 分组设置 
 * 
 * 当前设备没有播放音乐时，切换成分组1
 * 当设备需要播放音乐前，将其切换成分组2
 * 
 * @param group_id 分组ID
 */
void wanson_fst_group_change(unsigned char group_id)
{
    /* 如果需要设置的分组和当前分组ID不一致，则进行切换 */
    if(asr_curr_group_id != group_id) {
        asr_curr_group_id = group_id;

        if (group_id == 1) {
            Wanson_ASR_Set_Fst(&fst_1);
        } else if (group_id == 2) {
            Wanson_ASR_Set_Fst(&fst_2);
        }
        os_printf("fst_group_change_to: %d\n", group_id);
    }
}
#endif

/**
 * @brief ASR通用初始化函数
 * 
 * @param with_group 是否启用分组功能
 * @return int 初始化结果
 */
int bk_wanson_asr_common_init(void)
{
	int res = Wanson_ASR_Init();
	if (res < 0)
	{
		os_printf("Wanson_ASR_Init Failed!\n");
		return res;
	}

#if (CONFIG_WANSON_ASR_GROUP_VERSION)
	/* 指令分组初始化 */
	fst_1.states = fst01_states;
	fst_1.num_states = fst01_num_states;
	fst_1.finals = fst01_finals;
	fst_1.num_finals = fst01_num_finals;
	fst_1.words = fst01_words;

	fst_2.states = fst02_states;
	fst_2.num_states = fst02_num_states;
	fst_2.finals = fst02_finals;
	fst_2.num_finals = fst02_num_finals;
	fst_2.words = fst02_words;

	/* 设置默认分组 */
	wanson_fst_group_change(2);
	os_printf("Wanson_ASR_Init GRP OK!\n");
	return res;
#else
	Wanson_ASR_Reset();
#endif
	return res;
}

/**
 * @brief ASR反初始化
 * 
 * 释放ASR资源
 */
void bk_wanson_asr_common_deinit(void)
{
	Wanson_ASR_Release();
}

/**
 * @brief ASR识别函数
 * 
 * @param read_buf 音频数据缓冲区
 * @param read_size 音频数据大小
 * @param p1 用户参数1
 * @param p2 用户参数2
 * @return int 识别结果
 */
int bk_wanson_asr_recog(void *read_buf, uint32_t read_size, void *p1, void *p2)
{
	return Wanson_ASR_Recog((short*)read_buf, read_size>>1, p1, p2);
}

