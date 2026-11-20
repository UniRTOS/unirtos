#ifndef __BK_DRAW_ICON_CTLR_H__
#define __BK_DRAW_ICON_CTLR_H__


#include <components/avdk_utils/avdk_error.h>
#include "bk_draw_icon.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************
 * component name: bk_draw_icon
 * description: Private API (internal interface)
 ******************************************************************/

/* 内存分配结构体 */
typedef struct {
    uint8_t *addr;
    uint32_t size;
    //bool is_malloc_psram; /* 0:sram, 1:psram */
} icon_malloc_t;

/* 私有控制器结构体 */
typedef struct {
    icon_malloc_t buf1; /* 缓冲区1 */
    icon_malloc_t buf2; /* 缓冲区2 */
    uint8_t draw_in_psram;     /* 0；sram, 1:psram */
} draw_icon_context_t;

/* 控制器句柄结构体 */
typedef struct {
    draw_icon_context_t context; /* 上下文 */
    bk_draw_icon_ctlr_t ops; /* 操作函数 */
} private_draw_icon_ctlr_t;


/* 控制器操作函数 */
avdk_err_t bk_draw_icon_ctlr_new(bk_draw_icon_ctlr_handle_t *handle, icon_ctlr_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* __BK_DRAW_ICON_CTLR_H__ */