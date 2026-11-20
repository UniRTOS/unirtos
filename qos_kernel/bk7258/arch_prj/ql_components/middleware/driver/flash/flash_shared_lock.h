#pragma once

#include <common/bk_include.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_SHARED_CPU_CP   0
#define FLASH_SHARED_CPU_AP   1
#define FLASH_SHARED_CPU_APX  2
#define FLASH_SHARED_CPU_MAX  3

bk_err_t bk_flash_shared_lock_init(bool primary);
bk_err_t bk_flash_shared_wait_ready(uint32_t timeout_ms);
void bk_flash_shared_set_flash_init_done(void);
bk_err_t bk_flash_shared_wait_flash_init_done(uint32_t timeout_ms);
uint32_t bk_flash_shared_get_cpu_id(void);
bk_err_t bk_flash_shared_acquire(uint32_t cpu_id);
void bk_flash_shared_release(uint32_t cpu_id);
uint32_t bk_aspl_flash_enter_critical(void);
void bk_aspl_flash_exit_critical(uint32_t flags);
void bk_flash_shared_dump(void);

#ifdef __cplusplus
}
#endif
