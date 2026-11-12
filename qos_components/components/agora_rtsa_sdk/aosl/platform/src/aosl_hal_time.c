#include "qosa_def.h"
#include "aosl_hal_time.h"
#include "qosa_rtc.h"
uint64_t aosl_hal_get_tick_ms (void)
{
    return (uint64_t)qosa_get_system_tick_cnt();
}

uint64_t aosl_hal_get_time_ms (void)
{
     return (uint64_t)qosa_get_system_tick_cnt();
}

int aosl_hal_get_time_str(char *buf, int len)
{
	uint32_t now;

	if (NULL == buf || 0 == len) {
		return -1;
	}

	now = qosa_get_system_tick_cnt();
	qosa_snprintf(buf, len, "%u", now);
	return 0;
}

void aosl_hal_msleep(uint64_t ms)
{
    qosa_task_sleep_ms(ms);
}
