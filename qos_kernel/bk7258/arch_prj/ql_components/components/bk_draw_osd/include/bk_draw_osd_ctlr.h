#ifndef __BK_DRAW_OSD_CTLR_H__
#define __BK_DRAW_OSD_CTLR_H__

#include "components/bk_draw_osd_types.h"
#include "bk_draw_icon.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************
 * component name: bk_draw_osd
 * description: Private API (internal interface)
 ******************************************************************/


typedef struct
{
    uint32_t event;
    uint32_t param;
    uint32_t param2;
} osd_ctlr_msg_t;

typedef enum
{
    OSD_ADD,
    OSD_REMOVE,
    OSD_EXIT
} osd_ctlr_msg_type_t;
/**
 * @brief OSD控制器上下文结构体
 */
typedef struct {
    dynamic_array_t dyn_array;           /* 动态数组，用于存储混合信息 */
    const blend_info_t *blend_assets;    /* 混合资源数组 */
    const blend_info_t *blend_info;      /* 默认混合数组 */
    uint32_t blend_assets_size;          /* 混合资源数组大小 */
    bk_draw_icon_ctlr_handle_t icon_handle;   /* 图标句柄 */
    uint8_t draw_in_psram;                  /* 是否在PSRAM中绘制 */
    beken_semaphore_t task_sem;           /* 任务信号量 */
    beken_thread_t task;
    beken_queue_t queue;
    bool task_running;
} osd_ctlr_context_t;

/**
 * @brief OSD控制器私有结构体
 */
typedef struct {
    osd_ctlr_context_t context;          /* OSD控制器上下文 */
    bk_draw_osd_ctlr_t ops;              /* OSD控制器操作函数 */
} private_draw_osd_ctlr_t;

/* 控制器操作函数 */
avdk_err_t osd_ctlr_new(bk_draw_osd_ctlr_handle_t *handle, osd_ctlr_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* __BK_DRAW_OSD_CTLR_H__ */