#include <string.h>
#include <os/os.h>
#include <driver/pwr_clk.h>
#include "flash_shared_lock.h"

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile spinlock_t s_flash_aspl_spin_lock = SPIN_LOCK_INIT;
#endif

#define FLASH_SHARED_LOCK_MAGIC        0x46534C4BU /* FSLK */
#define FLASH_SHARED_LOCK_READY        0x52454144U /* READ */
#define FLASH_SHARED_LOCK_WAIT_US      100U
#define FLASH_SHARED_LOCK_SPIN_REPORT  0x100000U
#define FLASH_SHARED_LOCK_TIMEOUT_US   (5U * 1000U * 1000U)

extern void bk_delay_us(uint32_t us);

typedef struct {
	volatile uint32_t magic;
	volatile uint32_t ready;
	volatile uint32_t owner_cpu;
	volatile uint32_t seq;
	volatile uint32_t timeout_count;
	volatile uint32_t flash_init_done;
	volatile uint32_t choosing[FLASH_SHARED_CPU_MAX];
	volatile uint32_t number[FLASH_SHARED_CPU_MAX];
} flash_shared_lock_t;

#define FLASH_SHARED_STATIC_ASSERT(cond, name) typedef char flash_shared_static_assert_##name[(cond) ? 1 : -1]
FLASH_SHARED_STATIC_ASSERT(sizeof(flash_shared_lock_t) <= PWR_MNG_FLASH_SHARED_SIZE, lock_area_too_small);

static volatile uint8_t s_flash_shared_rec_count[FLASH_SHARED_CPU_MAX];

static inline flash_shared_lock_t *flash_shared_lock_get(void)
{
	return (flash_shared_lock_t *)PWR_MNG_FLASH_SHARED_ADDR;
}

static inline void flash_shared_barrier(void)
{
	__asm volatile("dmb sy" ::: "memory");
}

static inline void flash_shared_wait_once(void)
{
	bk_delay_us(FLASH_SHARED_LOCK_WAIT_US);
}

/* s_flash_shared_rec_count[] is indexed by the caller's own cpu_id, so each
 * core only ever touches its own slot and there is no cross-core RMW on the
 * same element. All hot-path callers (bk_aspl_flash_enter/exit_critical) and
 * the crash-dump path already run with local interrupts disabled, so the
 * counter is manipulated without any extra lock here - this keeps the
 * per-32-byte read/write hot path as cheap as the original driver. */
uint32_t bk_flash_shared_get_cpu_id(void)
{
	/* rtos_get_core_id() maps AP/APX to global shared-lock slots 1/2. */
	uint32_t cpu_id = (uint32_t)rtos_get_core_id();

	if (cpu_id >= FLASH_SHARED_CPU_MAX)
		return FLASH_SHARED_CPU_AP;

	return cpu_id;
}

bk_err_t bk_flash_shared_lock_init(bool primary)
{
	flash_shared_lock_t *lock = flash_shared_lock_get();

	if (primary) {
		if ((lock->magic == FLASH_SHARED_LOCK_MAGIC) &&
			(lock->ready == FLASH_SHARED_LOCK_READY) &&
			(lock->owner_cpu != FLASH_SHARED_CPU_MAX)) {
			BK_LOGE("flash_shm", "shared lock busy during primary init, owner=%u\r\n", lock->owner_cpu);
			bk_flash_shared_dump();
			return BK_FAIL;
		}

		memset((void *)lock, 0, sizeof(*lock));
		flash_shared_barrier();
		lock->magic = FLASH_SHARED_LOCK_MAGIC;
		lock->ready = FLASH_SHARED_LOCK_READY;
		lock->owner_cpu = FLASH_SHARED_CPU_MAX;
		flash_shared_barrier();
		return BK_OK;
	}

	bk_err_t ret = bk_flash_shared_wait_ready(5000);
	if (ret != BK_OK) {
		BK_LOGE("flash_shm", "wait shared lock ready timeout, check CP/AP FLASH_CP_AP_DIRECT_ACCESS config\r\n");
		return ret;
	}

	return BK_OK;
}

bk_err_t bk_flash_shared_wait_ready(uint32_t timeout_ms)
{
	flash_shared_lock_t *lock = flash_shared_lock_get();
	uint32_t waited_us = 0;
	uint32_t timeout_us = timeout_ms * 1000U;

	while ((lock->magic != FLASH_SHARED_LOCK_MAGIC) || (lock->ready != FLASH_SHARED_LOCK_READY)) {
		if (waited_us >= timeout_us)
			return BK_FAIL;

		flash_shared_wait_once();
		waited_us += FLASH_SHARED_LOCK_WAIT_US;
	}

	return BK_OK;
}

void bk_flash_shared_set_flash_init_done(void)
{
	flash_shared_lock_t *lock = flash_shared_lock_get();

	lock->flash_init_done = 1;
	flash_shared_barrier();
}

bk_err_t bk_flash_shared_wait_flash_init_done(uint32_t timeout_ms)
{
	flash_shared_lock_t *lock = flash_shared_lock_get();
	uint32_t waited_us = 0;
	uint32_t timeout_us = timeout_ms * 1000U;

	while (lock->flash_init_done != 1) {
		if (waited_us >= timeout_us) {
			BK_LOGE("flash_shm", "wait CP flash init timeout\r\n");
			bk_flash_shared_dump();
			return BK_FAIL;
		}

		flash_shared_wait_once();
		waited_us += FLASH_SHARED_LOCK_WAIT_US;
	}

	return BK_OK;
}

static uint32_t flash_shared_max_number(flash_shared_lock_t *lock)
{
	uint32_t max = 0;

	for (uint32_t i = 0; i < FLASH_SHARED_CPU_MAX; i++) {
		if (lock->number[i] > max)
			max = lock->number[i];
	}

	return max;
}

static bool flash_shared_rec_try_get(uint32_t cpu_id)
{
	if (s_flash_shared_rec_count[cpu_id] > 0) {
		s_flash_shared_rec_count[cpu_id]++;
		return true;
	}

	return false;
}

static void flash_shared_rec_set_first(uint32_t cpu_id)
{
	s_flash_shared_rec_count[cpu_id] = 1;
}

bk_err_t bk_flash_shared_acquire(uint32_t cpu_id)
{
	flash_shared_lock_t *lock = flash_shared_lock_get();
	uint32_t spin_count = 0;
	uint32_t waited_us = 0;

	if (cpu_id >= FLASH_SHARED_CPU_MAX)
		return BK_ERR_PARAM;

	if (flash_shared_rec_try_get(cpu_id))
		return BK_OK;

	lock->choosing[cpu_id] = 1;
	flash_shared_barrier();
	lock->number[cpu_id] = flash_shared_max_number(lock) + 1;
	flash_shared_barrier();
	lock->choosing[cpu_id] = 0;
	flash_shared_barrier();

	for (uint32_t peer = 0; peer < FLASH_SHARED_CPU_MAX; peer++) {
		if (peer == cpu_id)
			continue;

		while (lock->choosing[peer]) {
			if ((++spin_count & (FLASH_SHARED_LOCK_SPIN_REPORT - 1)) == 0) {
				lock->timeout_count++;
				flash_shared_wait_once();
				waited_us += FLASH_SHARED_LOCK_WAIT_US;
				if (waited_us >= FLASH_SHARED_LOCK_TIMEOUT_US)
					goto timeout;
			}
		}

		while ((lock->number[peer] != 0) &&
			((lock->number[peer] < lock->number[cpu_id]) ||
			((lock->number[peer] == lock->number[cpu_id]) && (peer < cpu_id)))) {
			if ((++spin_count & (FLASH_SHARED_LOCK_SPIN_REPORT - 1)) == 0) {
				lock->timeout_count++;
				flash_shared_wait_once();
				waited_us += FLASH_SHARED_LOCK_WAIT_US;
				if (waited_us >= FLASH_SHARED_LOCK_TIMEOUT_US)
					goto timeout;
			}
		}
	}

	lock->owner_cpu = cpu_id;
	lock->seq++;
	flash_shared_barrier();
	flash_shared_rec_set_first(cpu_id);
	return BK_OK;

timeout:
	lock->timeout_count++;
	lock->number[cpu_id] = 0;
	flash_shared_barrier();
	BK_LOGE("flash_shm", "cpu%u acquire timeout\r\n", cpu_id);
	bk_flash_shared_dump();
	return BK_ERR_TIMEOUT;
}

void bk_flash_shared_release(uint32_t cpu_id)
{
	flash_shared_lock_t *lock = flash_shared_lock_get();
	bool final_release = false;

	if (cpu_id >= FLASH_SHARED_CPU_MAX)
		return;

	if (s_flash_shared_rec_count[cpu_id] == 0) {
		BK_LOGE("flash_shm", "cpu%u release without acquire\r\n", cpu_id);
		BK_ASSERT(0);
		return;
	}

	s_flash_shared_rec_count[cpu_id]--;
	final_release = (s_flash_shared_rec_count[cpu_id] == 0);
	if (final_release) {
		lock->owner_cpu = FLASH_SHARED_CPU_MAX;
		flash_shared_barrier();
		lock->number[cpu_id] = 0;
		flash_shared_barrier();
	}
}

uint32_t bk_aspl_flash_enter_critical(void)
{
	/* Disabling local interrupts first pins the running task to this core
	 * (it cannot be preempted/migrated), so the cpu_id captured below stays
	 * stable through the whole critical section. This avoids the costly
	 * vTaskSuspendAll()/xTaskResumeAll() pair on this per-32-byte hot path. */
	uint32_t flags = rtos_disable_int();
#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&s_flash_aspl_spin_lock);
#endif
	uint32_t cpu_id = bk_flash_shared_get_cpu_id();

	if (bk_flash_shared_acquire(cpu_id) != BK_OK) {
#ifdef CONFIG_FREERTOS_SMP
		spin_unlock(&s_flash_aspl_spin_lock);
#endif
		rtos_enable_int(flags);
		BK_ASSERT(0);
	}

	return flags;
}

void bk_aspl_flash_exit_critical(uint32_t flags)
{
	uint32_t cpu_id = bk_flash_shared_get_cpu_id();

	bk_flash_shared_release(cpu_id);
#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&s_flash_aspl_spin_lock);
#endif
	rtos_enable_int(flags);
}

void bk_flash_shared_dump(void)
{
	flash_shared_lock_t *lock = flash_shared_lock_get();

	BK_LOGI("flash_shm", "magic=0x%x ready=0x%x owner=%u seq=%u timeout=%u init_done=%u\r\n",
		lock->magic, lock->ready, lock->owner_cpu, lock->seq,
		lock->timeout_count, lock->flash_init_done);
	for (uint32_t i = 0; i < FLASH_SHARED_CPU_MAX; i++) {
		BK_LOGI("flash_shm", "cpu%u choosing=%u number=%u rec=%u\r\n",
			i, lock->choosing[i], lock->number[i], s_flash_shared_rec_count[i]);
	}
}
