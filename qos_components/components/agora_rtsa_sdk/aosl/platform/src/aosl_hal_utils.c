#include "qosa_def.h"
#include "aosl_hal_utils.h"
#include "aosl_hal_time.h"

#ifndef RAND_MAX
#define RAND_MAX 0x7fff
#endif

int aosl_hal_get_uuid (char buf [], int buf_sz)
{
	uint64_t ts_now = (uint64_t) aosl_hal_get_tick_ms ();
	unsigned int s, r1, r2;

	s = (unsigned int)ts_now;
	while (s > RAND_MAX) {
		/* s *= 7 */
		s = (s << 2) + (s << 1) + s;
	}

	srand (s);
	r1 = (unsigned int)rand ();
	r2 = (unsigned int)rand ();
	qosa_snprintf (buf, buf_sz, "%016llx%08x%08x", (unsigned long long)ts_now, r1, r2);
	return 0;
}

/**
 * @brief get OS version string
 * @param [out] buf buffer to store OS version string
 * @param [in] buf_sz size of buffer
 * @return 0 on success, < 0 on error
 */
int aosl_hal_os_version (char buf [], int buf_sz)
{
    qosa_snprintf(buf, buf_sz, "%s", " ");
    return 0;
}
