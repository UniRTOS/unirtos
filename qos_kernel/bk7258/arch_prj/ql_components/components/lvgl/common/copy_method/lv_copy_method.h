#ifndef __LV_COPY_METHOD_H__
#define __LV_COPY_METHOD_H__


void lv_dma2d_memcpy_init(void);

void lv_dma2d_memcpy_deinit(void);

void lv_dma2d_memcpy_wait_transfer_finish(void);

void lv_dma2d_memcpy_last_frame(void *Psrc, void *Pdst, uint32_t xsize, uint32_t ysize, uint32_t src_offline, uint32_t dest_offline);

void lv_dma2d_stop_memcpy_last_frame(void);

void lv_dma2d_memcpy_double_draw_buffer(void *Psrc, uint32_t src_xsize, uint32_t src_ysize, void *Pdst, uint32_t dst_xpos, uint32_t dst_ypos);

void lv_dma2d_memcpy_single_draw_buffer(void *Psrc, uint32_t src_xsize, uint32_t src_ysize, void *Pdst, uint32_t dst_xpos, uint32_t dst_ypos);


void lv_dma_memcpy_init(void);

void lv_dma_memcpy_deinit(void);

void lv_dma_stop_memcpy_last_frame(void);

void lv_dma_memcpy_wait_transfer_finish(void);

void lv_dma_memcpy_last_frame(void *Psrc, void *Pdst, uint32_t xsize, uint32_t ysize);

#endif /* LV_COPY_METHOD_H */
