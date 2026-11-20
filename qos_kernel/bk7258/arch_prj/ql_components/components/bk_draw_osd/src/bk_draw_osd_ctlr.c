#include <os/str.h>
#include <components/media_types.h>
#include "components/bk_draw_osd_types.h"
#include "bk_draw_osd_ctlr.h"
#include "bk_draw_icon.h"

#define TAG "draw_osd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)



/**
 * @brief Calculate the length of blend array
 */
static inline size_t osd_ctlr_get_array_length(const blend_info_t *array)
{
    size_t length = 0;
    if(array != NULL) {
        while(array[length].addr != NULL) {
            length++;
        }
    }
    return length; // only return the length of the array, not the length of the string
}


/**
 * @brief Initialize dynamic array
 */
static bk_err_t dynamic_array_init(dynamic_array_t *dyn_array, size_t initial_capacity)
{
    uint32_t len = initial_capacity * sizeof(blend_info_t);
    dyn_array->entry = os_malloc(len);
    if (dyn_array->entry == NULL)
    {
        return BK_FAIL;
    }

    os_memset((void *)dyn_array->entry, 0, len);

    for(int i = 0; i < initial_capacity; i++)
    {
        dyn_array->entry[i].name[0] = '\0';
        dyn_array->entry[i].addr = NULL;
    }
    dyn_array->size = 0;
    dyn_array->capacity = initial_capacity;
    return BK_OK;
}

/**
 * @brief Copy existing blend info to dynamic array
 */
static void copy_existing_blend_info_to_dynamic_array(dynamic_array_t *dyn_array, const blend_info_t *blend_info)
{
    if (blend_info == NULL)
    {
        return;
    }

    size_t length = osd_ctlr_get_array_length(blend_info);

    if (dyn_array->size + length > dyn_array->capacity)
    {
        dyn_array->capacity = dyn_array->size + length;
        dyn_array->entry = os_realloc(dyn_array->entry, dyn_array->capacity * sizeof(blend_info_t));
        if (dyn_array->entry == NULL)
        {
            LOGI("%s realloc fail \n", __func__);
            return;
        }
    }

    for (size_t i = 0; i < length; i++)
    {
        dyn_array->entry[dyn_array->size] = blend_info[i];
        dyn_array->size++;
    }
}

/**
 * @brief Find blend info in assets by name
 */
static const blend_info_t *find_blend_info_in_assets_by_name(const blend_info_t *blend_assets, uint32_t blend_assets_size, const char *name)
{
    if (name == NULL || blend_assets == NULL)
    {
        return NULL;
    }

    for(int i = 0; i < blend_assets_size; i++)
    {
        if (strcmp((char *)blend_assets[i].name, name) == 0)
        {
            if (blend_assets[i].addr != NULL)
            {
                LOGI("find blend_info_in_assets_by_name name = %s, addr = %p, content = %s \n", name, blend_assets[i].addr, blend_assets[i].content);
                return &blend_assets[i];
            }
        }
    }
    return NULL;
}

/**
 * @brief Find blend info in assets by content
 */
static const blend_info_t *find_blend_info_in_assets_by_content(const blend_info_t *blend_assets, uint32_t blend_assets_size, const char *content)
{
    if (content == NULL || content[0] == '\0' || blend_assets == NULL)
    {
        return NULL;
    }
    for(int i = 0; i < blend_assets_size; i++)
    {
        if (strcmp((char *)blend_assets[i].content, content) == 0)
        {
            if (blend_assets[i].addr != NULL)
            {
                return &blend_assets[i];
            }
        }
    }
    return NULL;
}

/**
 * @brief Find blend info in dynamic array
 */
static blend_info_t *find_blend_info_in_dynamic_array(dynamic_array_t *dyn_array, const char *name)
{
    if (name == NULL || name[0] == '\0')
    {
        return NULL;
    }

    for(int i = 0; i < dyn_array->size; i++)
    {
        if (strcmp(dyn_array->entry[i].name, name) == 0)
        {
            return &dyn_array->entry[i];
        }
    }
    return NULL;
}

/**
 * @brief Add or update blend info in dynamic array
 */
static avdk_err_t add_or_update_blend_info_to_dynamic_array(dynamic_array_t *dyn_array, 
                                                      const blend_info_t *blend_assets, 
                                                      uint32_t blend_assets_size, 
                                                      const char *name, 
                                                      const char* content)
{
    size_t dyn_array_size = dyn_array->size;
    if (name == NULL || name[0] == '\0')
    {
        return AVDK_ERR_INVAL;
    }

    blend_info_t *exiting_info = find_blend_info_in_dynamic_array(dyn_array, name);
    if (exiting_info != NULL)
    {
        os_strncpy(exiting_info->name, name, sizeof(exiting_info->name) - 1);
        exiting_info->name[sizeof(exiting_info->name) - 1] = '\0';
        if (content != NULL)
        {
            os_strncpy(exiting_info->content, content, sizeof(exiting_info->content) - 1);
            exiting_info->content[sizeof(exiting_info->content) - 1] = '\0';
            if (content[0] != '\0')
            {
                if (exiting_info->addr->blend_type == BLEND_TYPE_IMAGE)
                {
                    const blend_info_t *update_img = find_blend_info_in_assets_by_content(blend_assets, blend_assets_size, content);
                    if (update_img != NULL)
                    {
                        exiting_info->addr = update_img->addr;
                    }
                    else
                    {
                        LOGW("(%d)warring!!!, input no content \n", __LINE__);
                        exiting_info->content[0] = '\0';
                    }
                }
            }
            else
            {
                LOGW("(%d)warring!!!, input no content \n", __LINE__);
                return AVDK_ERR_INVAL;
            }
        }
        return BK_OK;
    }

    const blend_info_t *assets_info = find_blend_info_in_assets_by_name(blend_assets, blend_assets_size, name);
    if (assets_info == NULL)
    {
        LOGW("%s, not fint assets %s\n", __func__, name);
        return AVDK_ERR_INVAL;
    }

    if (dyn_array_size >= dyn_array->capacity)
    {
        dyn_array->capacity *= 2;
        dyn_array->entry = os_realloc(dyn_array->entry, dyn_array->capacity * sizeof(blend_info_t));
        if (dyn_array->entry == NULL)
        {
            LOGI("%s realloc fail \n", __func__);
            return AVDK_ERR_INVAL;
        }
        for(int i = dyn_array_size; i < dyn_array->capacity; i++)
        {
            dyn_array->entry[i].name[0] = '\0';
            dyn_array->entry[i].addr = NULL;
        }
        LOGI("%s extend dyn_array capacity * 2\n", __func__);
    }
    dyn_array->entry[dyn_array_size] = *assets_info;

    strncpy(dyn_array->entry[dyn_array_size].name, name, sizeof(dyn_array->entry[dyn_array_size].name) - 1);
    dyn_array->entry[dyn_array_size].name[sizeof(dyn_array->entry[dyn_array_size].name) - 1] = '\0';
    if (content != NULL)
    {
        strncpy(dyn_array->entry[dyn_array_size].content, content, sizeof(dyn_array->entry[dyn_array_size].content) - 1);
        dyn_array->entry[dyn_array_size].content[sizeof(dyn_array->entry[dyn_array_size].content) - 1] = '\0';
        if (content[0] != '\0')
        {
            if (dyn_array->entry[dyn_array_size].addr->blend_type == BLEND_TYPE_IMAGE)
            {
                const blend_info_t *update_img = find_blend_info_in_assets_by_content(blend_assets, blend_assets_size, content);
                if (update_img != NULL)
                {
                    dyn_array->entry[dyn_array_size].addr = update_img->addr;
                }
                else
                {
                    LOGW("(%d)warring!!!, input no content \n", __LINE__);
                    dyn_array->entry[dyn_array_size].content[0] = '\0';
                }
            }
        }
        else
        {
            LOGW("(%d)warring!!!, input no content \n", __LINE__);
        }
    }

    dyn_array->size++;
    // make sure the last element is NULL terminated
    dyn_array->entry[dyn_array->size].addr = NULL;
    return AVDK_ERR_OK;
}


static bk_err_t osd_ctlr_task_send_msg(osd_ctlr_context_t *context, uint8_t type, uint32_t param, uint32_t param2)
{
    int ret = BK_FAIL;
    osd_ctlr_msg_t msg;

    if (context && context->task_running)
    {
        msg.event = type;
        msg.param = param;
        msg.param2 = param2;

        if (context && context->task_running)
        {
            ret = rtos_push_to_queue(&context->queue, &msg, BEKEN_WAIT_FOREVER);

            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s push failed\n", __func__);
            }
        }
    }

    return ret;
}


/**
 * @brief Stop osd background task and deinit queue/semaphore
 */
static void osd_ctlr_task_stop(osd_ctlr_context_t *context)
{
    if (context == NULL) {
        return;
    }

    if (context->task_running) {
        if (osd_ctlr_task_send_msg(context, OSD_EXIT, 0, 0) == BK_OK) {
            rtos_get_semaphore(&context->task_sem, BEKEN_NEVER_TIMEOUT);
        } else {
            LOGE("%s send OSD_EXIT failed\n", __func__);
        }
        context->task_running = false;
    }

    if (context->queue) {
        rtos_deinit_queue(&context->queue);
        context->queue = NULL;
    }

    rtos_deinit_semaphore(&context->task_sem);
    context->task = NULL;
}

/**
 * @brief Release all resources allocated in osd_ctlr_new
 */
static void osd_ctlr_destroy(private_draw_osd_ctlr_t *priv_ctl, bool stop_task)
{
    if (priv_ctl == NULL) {
        return;
    }

    if (stop_task) {
        osd_ctlr_task_stop(&priv_ctl->context);
    }

    if (priv_ctl->context.icon_handle) {
        bk_draw_icon_delete(priv_ctl->context.icon_handle);
        priv_ctl->context.icon_handle = NULL;
    }

    if (priv_ctl->context.dyn_array.entry) {
        os_free(priv_ctl->context.dyn_array.entry);
        priv_ctl->context.dyn_array.entry = NULL;
        priv_ctl->context.dyn_array.size = 0;
        priv_ctl->context.dyn_array.capacity = 0;
    }

    os_free(priv_ctl);
}

/**
 * @brief delete osd controller
 */
static avdk_err_t osd_ctlr_delete(bk_draw_osd_ctlr_handle_t handle)
{
    private_draw_osd_ctlr_t *priv_ctl = __containerof(handle, private_draw_osd_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(priv_ctl, AVDK_ERR_INVAL, TAG, "control is NULL");

    osd_ctlr_destroy(priv_ctl, true);

    LOGI("OSD controller deleted successfully \n");
    return AVDK_ERR_OK;
}

avdk_err_t osd_ctlr_add_or_updata(bk_draw_osd_ctlr_handle_t handle, const char *name, const char* content)
{
    private_draw_osd_ctlr_t *priv_ctl = __containerof(handle, private_draw_osd_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(priv_ctl, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(name, AVDK_ERR_INVAL, TAG, "name is NULL");

    blend_info_t *info = os_malloc(sizeof(blend_info_t));
    AVDK_RETURN_ON_FALSE(info, AVDK_ERR_INVAL, TAG, "malloc blend_info_t failed");

    os_strncpy(info->name, name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    if (content != NULL) {
        os_strncpy(info->content, content, sizeof(info->content) - 1);
        info->content[sizeof(info->content) - 1] = '\0';
    } else {
        info->content[0] = '\0';
    }
    
    LOGI("%s, name = %s, content = %s \n", __func__, info->name, content);

    int ret = osd_ctlr_task_send_msg(&priv_ctl->context, OSD_ADD, (uint32_t)info, 0);

    if (ret != AVDK_ERR_OK) {
        LOGW("%s, send msg failed, name = %s, content = %s \n", __func__, info->name, content);
        os_free(info);
    }

    return ret;
}

avdk_err_t osd_ctlr_remove(bk_draw_osd_ctlr_handle_t handle, const char *name)
{
    private_draw_osd_ctlr_t *priv_ctl = __containerof(handle, private_draw_osd_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(priv_ctl, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(name, AVDK_ERR_INVAL, TAG, "name is NULL");
    return osd_ctlr_task_send_msg(&priv_ctl->context, OSD_REMOVE, (uint32_t)name, 0);
}


/**
 * @brief Print all available OSD resources,return resources array and size
 * @param handle osd controller handle
 * @param resources resources array
 * @param size resources size
 * @param is_printf 1:print resources info,0:no print
 * @return avdk_err_t AVDK_ERR_OK:success,other:fail
 */
avdk_err_t osd_ctlr_print_all_resources(bk_draw_osd_ctlr_handle_t handle, const blend_info_t **resources, uint32_t *size, uint32_t is_printf)
{
    private_draw_osd_ctlr_t *priv_ctl = __containerof(handle, private_draw_osd_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(priv_ctl, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (resources) {
        *resources = priv_ctl->context.blend_assets;
    }
    if (size) {
        *size = priv_ctl->context.blend_assets_size;
    }

    if (is_printf) {
        LOGI("===== All Available OSD Resources =====\n");
        LOGI("Total resources: %u\n", priv_ctl->context.blend_assets_size);
        
        if (priv_ctl->context.blend_assets && priv_ctl->context.blend_assets_size > 0) {
            for (uint32_t i = 0; i < priv_ctl->context.blend_assets_size; i++) {
                const blend_info_t *asset = &priv_ctl->context.blend_assets[i];
                if (asset->addr != NULL) {
                    const bk_blend_t *blend = (bk_blend_t *)asset->addr;
                    
                    LOGI("Resource %u:\n", i + 1);
                    LOGI("  Name: %s\n", asset->name);
                    LOGI("  Content: %s\n", asset->content);
                    LOGI("  Type: %s\n", (blend->blend_type == BLEND_TYPE_IMAGE) ? "Image" : 
                                        (blend->blend_type == BLEND_TYPE_FONT) ? "Font" : "Unknown");
                    LOGI("  Position: X=%d, Y=%d\n", blend->xpos, blend->ypos);
                    LOGI("  Size: Width=%d, Height=%d\n", blend->width, blend->height);
                    LOGI("----------------------------------------\n");
                }
            }
        } else {
            LOGI("No resources available\n");
        }
        
        LOGI("========================================\n");
    }
    
    return AVDK_ERR_OK;
}


avdk_err_t osd_ctlr_print_current_info(bk_draw_osd_ctlr_handle_t handle, const blend_info_t **resources, uint32_t *size, uint32_t is_printf)
{
    private_draw_osd_ctlr_t *priv_ctl = __containerof(handle, private_draw_osd_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(priv_ctl, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (resources) {
        *resources = priv_ctl->context.dyn_array.entry;
    }
    if (size) {
        *size = priv_ctl->context.dyn_array.size;
    }

    if (is_printf) {
        LOGI("Dynamic array elements: %u\n", priv_ctl->context.dyn_array.size);
        if (priv_ctl->context.dyn_array.size > 0) {
            for (uint32_t i = 0; i < priv_ctl->context.dyn_array.size; i++) {
                const blend_info_t *element = &priv_ctl->context.dyn_array.entry[i];
                if (element->addr != NULL) {
                    const bk_blend_t *blend = (bk_blend_t *)element->addr;
                    
                    LOGI("Element %u:\n", i + 1);
                    LOGI("  Name: %s\n", element->name);
                    LOGI("  Content: %s\n", element->content);
                    LOGI("  Type: %s\n", (blend->blend_type == BLEND_TYPE_IMAGE) ? "Image" : 
                                        (blend->blend_type == BLEND_TYPE_FONT) ? "Font" : "Unknown");
                    LOGI("  Position: X=%d, Y=%d\n", blend->xpos, blend->ypos);
                    LOGI("  Size: Width=%d, Height=%d\n", blend->width, blend->height);
                    LOGI("----------------------------------------\n");
                }
                else
                {
                    LOGI("Element %u:\n", i + 1);
                    LOGI("  addr: %p\n", element->addr);
                }
            }
        }
    }
    
    return AVDK_ERR_OK;
}

/**
 * @brief Draw image on osd
 * @param handle osd controller handle
 * @param bg_info osd background info
 * @param img_info osd image info
 * @return avdk_err_t AVDK_ERR_OK:success,other:fail
 */
static avdk_err_t osd_ctlr_draw_image(bk_draw_osd_ctlr_handle_t handle, osd_bg_info_t *bg_info, const blend_info_t *img_info)
{
    private_draw_osd_ctlr_t *priv_ctl = __containerof(handle, private_draw_osd_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(priv_ctl, AVDK_ERR_INVAL, TAG, "control is NULL");

    AVDK_RETURN_ON_FALSE(bg_info, AVDK_ERR_INVAL, TAG, "bg_info is NULL");
    AVDK_RETURN_ON_FALSE(img_info, AVDK_ERR_INVAL, TAG, "img_info is NULL");
    AVDK_RETURN_ON_FALSE(bg_info->frame, AVDK_ERR_INVAL, TAG, "bg_info->frame is NULL");
    AVDK_RETURN_ON_FALSE(bg_info->frame->frame, AVDK_ERR_INVAL, TAG, "bg_info->frame->frame is NULL");
    AVDK_RETURN_ON_FALSE(bg_info->width, AVDK_ERR_INVAL, TAG, "bg_info->width is NULL");
    AVDK_RETURN_ON_FALSE(bg_info->height, AVDK_ERR_INVAL, TAG, "bg_info->height is NULL");

    icon_image_blend_cfg_t cfg = {0};
    const bk_blend_t *img_dsc = (bk_blend_t *)img_info->addr;
    AVDK_RETURN_ON_FALSE(!(img_dsc->width + img_dsc->xpos > bg_info->width), AVDK_ERR_INVAL, TAG, "img_dsc->width + img_dsc->xpos > bg_info->width");
    AVDK_RETURN_ON_FALSE(!(img_dsc->height + img_dsc->ypos > bg_info->height), AVDK_ERR_INVAL, TAG, "img_dsc->height + img_dsc->ypos > bg_info->height");

    cfg.pfg_addr = (uint8_t *)img_dsc->image.data;
    cfg.pbg_addr = (uint8_t *)(bg_info->frame->frame);
    cfg.xpos = img_dsc->xpos;
    cfg.ypos = img_dsc->ypos;
    cfg.xsize = img_dsc->width;
    cfg.ysize = img_dsc->height;
    cfg.bg_data_format = bg_info->frame->fmt;
    cfg.bg_width = bg_info->frame->width;
    cfg.bg_height = bg_info->frame->height;
    cfg.visible_width = bg_info->width;
    cfg.visible_height = bg_info->height;
    bk_draw_icon_image(priv_ctl->context.icon_handle, &cfg);

    return AVDK_ERR_OK;
}

/**
 * @brief Draw font on osd
 * @param handle osd controller handle
 * @param bg_info osd background info
 * @param font_info osd font info
 * @return avdk_err_t AVDK_ERR_OK:success,other:fail
 */
static avdk_err_t osd_ctlr_draw_font(bk_draw_osd_ctlr_handle_t handle, osd_bg_info_t *bg_info,  const blend_info_t *font_info)
{
    private_draw_osd_ctlr_t *priv_ctl = __containerof(handle, private_draw_osd_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(priv_ctl, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(bg_info, AVDK_ERR_INVAL, TAG, "bg_info is NULL");
    AVDK_RETURN_ON_FALSE(font_info, AVDK_ERR_INVAL, TAG, "font_info is NULL");

    const bk_blend_t *font_strings = font_info->addr;
    icon_font_blend_cfg_t cfg = {0};
    cfg.pbg_addr = (uint8_t *)(bg_info->frame->frame);
    cfg.xsize = font_strings->width;
    cfg.ysize = font_strings->height;

    if ((font_strings->width + font_strings->xpos > bg_info->width) || (font_strings->height + font_strings->ypos > bg_info->height))
    {
        if (font_strings->xpos + font_strings->width > bg_info->width)
        {
            LOGD("content: %s, xpos %d + width %d > width %d\n", __func__, font_strings->xpos, font_strings->width, bg_info->width);
            cfg.xsize = bg_info->width - font_strings->xpos;
        }
        if (font_strings->ypos + font_strings->height > bg_info->height)
        {
            LOGD("content: %s, ypos %d + height %d > height %d\n", __func__, font_strings->ypos, font_strings->height, bg_info->height);
            cfg.ysize = bg_info->height - font_strings->ypos;
        }
    }

    cfg.xpos = font_strings->xpos;
    cfg.ypos = font_strings->ypos;
    cfg.str_num = 1;
    if (bg_info->frame->fmt == PIXEL_FMT_VUYY)
    {
        cfg.font_format = FONT_VUYY;
    }
    else if (bg_info->frame->fmt == PIXEL_FMT_YUYV)
    {
        cfg.font_format = FONT_YUYV;
    }
    else
    {
        cfg.font_format = FONT_RGB565;
    }
    cfg.str[0] = (icon_font_str_cfg_t)
    {
        (const char *)font_info->content, font_strings->font.color, font_strings->font.font_digit_type, 0, 0
    };
    cfg.bg_data_format = bg_info->frame->fmt;
    cfg.bg_width = bg_info->frame->width;
    cfg.bg_height = bg_info->frame->height;
    cfg.visible_width = bg_info->width;
    cfg.visible_height = bg_info->height;
    bk_draw_icon_font(priv_ctl->context.icon_handle, &cfg);
    return AVDK_ERR_OK;
}

/**
 * @brief Draw array on osd
 * @param handle osd controller handle
 * @param bg_info osd background info
 * @param array_ptr osd array info
 * @return avdk_err_t AVDK_ERR_OK:success,other:fail
 */
static avdk_err_t osd_ctlr_draw_array(bk_draw_osd_ctlr_handle_t handle, osd_bg_info_t *bg_info,  const blend_info_t *array_ptr)
{
    AVDK_RETURN_ON_FALSE(bg_info, AVDK_ERR_INVAL, TAG, "bg_info is NULL");
    //AVDK_RETURN_ON_FALSE(array_ptr, AVDK_ERR_INVAL, TAG, "array_ptr is NULL");

    //if array is NULL OSD default by osd_ctlr_new  param(blend_info)
    private_draw_osd_ctlr_t *priv_ctl = __containerof(handle, private_draw_osd_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(priv_ctl, AVDK_ERR_INVAL, TAG, "control is NULL");

   const blend_info_t *parray  = NULL;

    if (array_ptr == NULL)
    {
        parray = priv_ctl->context.dyn_array.entry;
    }
    else  
    {
        //TODO
    }

    uint8_t i = 0;
    while (parray && parray[i].addr != NULL)
    {
        if (parray[i].addr->blend_type == BLEND_TYPE_FONT)
        {
            osd_ctlr_draw_font(handle, bg_info, &parray[i]);
        }
        else if (parray[i].addr->blend_type == BLEND_TYPE_IMAGE)
        {
            osd_ctlr_draw_image(handle, bg_info, &parray[i]);
        }
        i++;
    }

    return AVDK_ERR_OK;
}
/**
 * @brief Draw osd on screen
 * @param handle osd controller handle
 * @param ioctl_cmd ioctl command
 * @param param1 param1
 * @param param2 param2
 * @param param3 param3
 * @return avdk_err_t AVDK_ERR_OK:success,other:fail
 */ 
static avdk_err_t osd_ctlr_ioctl(bk_draw_osd_ctlr_handle_t handle, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3)
{
    private_draw_osd_ctlr_t *priv_ctl = __containerof(handle, private_draw_osd_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(priv_ctl, AVDK_ERR_INVAL, TAG, "control is NULL");

    avdk_err_t ret = AVDK_ERR_OK;

    switch (ioctl_cmd)
    {
        case OSD_CTLR_CMD_SET_PSRAM_USAGE:
            {
                priv_ctl->context.draw_in_psram = param1;
                if (priv_ctl->context.icon_handle) {
                    bk_draw_icon_ioctl(priv_ctl->context.icon_handle, DRAW_ICON_CTLR_CMD_SET_PSRAM_USAGE, param1, 0, 0);
                }
            }
            break;
        
        case OSD_CTLR_CMD_GET_PSRAM_USAGE:
            *((uint8_t *)param1) = priv_ctl->context.draw_in_psram;
            break;

        case OSD_CTLR_CMD_GET_ALL_ASSETS:
            // param1: is_printf, param2: resources指针, param3: size指针
            osd_ctlr_print_all_resources(handle, (const blend_info_t **)param2, (uint32_t *)param3, param1);
        break;

        case OSD_CTLR_CMD_GET_DRAW_INFO:
            // param1: is_printf, param2: resources指针, param3: size指针
            osd_ctlr_print_current_info(handle, (const blend_info_t **)param2, (uint32_t *)param3, param1);
        break;

        default:
            ret = AVDK_ERR_UNSUPPORTED;
            LOGE("%s, unsupported cmd: %d\n", __func__, ioctl_cmd);
            break;
    }
    
    return ret;
}

/**
 * @brief Remove blend info from dynamic array by name
 */
static void remove_blend_info_from_dynamic_array(dynamic_array_t *dyn_array, const char *name)
{
    if (name == NULL || name[0] == '\0' || dyn_array == NULL || dyn_array->size == 0) {
        return;
    }

    // Find the index of the blend info to remove
    int remove_index = -1;
    for (int i = 0; i < dyn_array->size; i++) {
        if (strcmp(dyn_array->entry[i].name, name) == 0) {
            remove_index = i;
            break;
        }
    }

    // If found, remove it by shifting array elements
    if (remove_index >= 0) {
        // Shift elements to fill the gap
        for (int i = remove_index; i < dyn_array->size - 1; i++) {
            dyn_array->entry[i] = dyn_array->entry[i + 1];
        }
        
        // Only need to adjust the size, no need to clear the content of the last element
        dyn_array->size--;
        
        // make sure the last element is NULL
        dyn_array->entry[dyn_array->size].addr = NULL;
        LOGI("Successfully removed blend info: %s\n", name);
    } else {
        LOGW("Failed to find blend info to remove: %s\n", name);
    }
}

static void osd_ctlr_task_entry(beken_thread_arg_t arg)
{
    osd_ctlr_context_t *context = (osd_ctlr_context_t *)arg;
    context->task_running = true;
    rtos_set_semaphore(&context->task_sem);

    while (context->task_running)
    {
        osd_ctlr_msg_t msg;
        int ret = rtos_pop_from_queue(&context->queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == BK_OK)
        {
            switch (msg.event)
            {
                case OSD_ADD:
                {
                    blend_info_t *info = (blend_info_t *)msg.param;
                    ret = add_or_update_blend_info_to_dynamic_array(&context->dyn_array,
                                           context->blend_assets,
                                           context->blend_assets_size,
                                           info->name, 
                                           info->content);
                    os_free(info);
                }
                break;

                case OSD_REMOVE:
                {
                    // Handle OSD_REMOVE message
                    const char *name = (const char *)msg.param;
                    if (name != NULL) {
                        remove_blend_info_from_dynamic_array(&context->dyn_array, name);
                    } else {
                        LOGW("Invalid name parameter for OSD_REMOVE\n");
                    }
                }
                break;

                case OSD_EXIT:
                {
                    context->task_running = false;
                    context->task = NULL;
                    rtos_set_semaphore(&context->task_sem);
                    rtos_delete_thread(NULL);
                }
                break;

                default:
                    break;
            }
        }
    }
}

static bk_err_t osd_ctlr_task_start(osd_ctlr_context_t *context)
{
    int ret = AVDK_ERR_OK;

    ret = rtos_init_queue(&context->queue,
                          "osd_queue",
                          sizeof(osd_ctlr_msg_t),
                          15);
    AVDK_RETURN_ON_ERROR(ret, TAG, "osd_queue init failed\n");
    
    ret = rtos_create_thread(&context->task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "osd_thread",
                             (beken_thread_function_t)osd_ctlr_task_entry,
                             1024 * 5,
                             (beken_thread_arg_t)context);
    AVDK_RETURN_ON_ERROR(ret, TAG, "osd_thread init failed\n");

    ret = rtos_get_semaphore(&context->task_sem, BEKEN_NEVER_TIMEOUT);
    AVDK_RETURN_ON_ERROR(ret, TAG, "task_sem get failed\n");

    return ret;
}

/**
 * @brief Create osd controller
 * @param handle Output parameter, used to store the created osd controller handle
 * @param config osd controller configuration parameters
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t osd_ctlr_new(bk_draw_osd_ctlr_handle_t *handle, osd_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    avdk_err_t ret = AVDK_ERR_OK;
    private_draw_osd_ctlr_t *priv_ctl = NULL;

    // osd controller memory
    priv_ctl = os_malloc(sizeof(private_draw_osd_ctlr_t));
    AVDK_RETURN_ON_FALSE(priv_ctl, AVDK_ERR_NOMEM, TAG, "malloc failed");
    os_memset(priv_ctl, 0, sizeof(private_draw_osd_ctlr_t));

    // set osd controller config
    priv_ctl->context.draw_in_psram = config->draw_in_psram;

    icon_ctlr_config_t icon_config = {
        .draw_in_psram = config->draw_in_psram,
    };
    ret = bk_draw_icon_new(&priv_ctl->context.icon_handle, &icon_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s bk_draw_icon_new failed\n", __func__);
        os_free(priv_ctl);
        return ret;
    }

    priv_ctl->context.blend_assets = config->blend_assets;
    priv_ctl->context.blend_info = config->blend_info;
    priv_ctl->context.blend_assets_size = config->blend_assets ? osd_ctlr_get_array_length(config->blend_assets) : 0;

    ret = dynamic_array_init(&priv_ctl->context.dyn_array, priv_ctl->context.blend_assets_size);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s dynamic_array_init failed\n", __func__);
        osd_ctlr_destroy(priv_ctl, false);
        return ret;
    }
    LOGE("%s dynamic_array_init size = %u\n", __func__, priv_ctl->context.dyn_array.size);
    // copy blend info to dynamic array
    copy_existing_blend_info_to_dynamic_array(&priv_ctl->context.dyn_array, priv_ctl->context.blend_info);

    ret = rtos_init_semaphore(&priv_ctl->context.task_sem, 1);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s task_sem init failed\n", __func__);
        osd_ctlr_destroy(priv_ctl, false);
        return ret;
    }

    ret = osd_ctlr_task_start(&priv_ctl->context);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s osd ctlr task init failed\n", __func__);
        osd_ctlr_destroy(priv_ctl, true);
        return ret;
    }

    // set osd controller ops
    priv_ctl->ops.delete = osd_ctlr_delete;
    priv_ctl->ops.add_or_updata = osd_ctlr_add_or_updata;
    priv_ctl->ops.remove = osd_ctlr_remove;
    priv_ctl->ops.draw_osd_array = osd_ctlr_draw_array;
    priv_ctl->ops.draw_image = osd_ctlr_draw_image;
    priv_ctl->ops.draw_font = osd_ctlr_draw_font;
    priv_ctl->ops.ioctl = osd_ctlr_ioctl;
    
    *handle = &priv_ctl->ops;
    
    LOGI("OSD controller created successfully \n");
    return ret;
}
