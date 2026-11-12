#include "errno.h"
#include "aosl_hal_errno.h"
#include "qosa_def.h"

int aosl_hal_errno_convert(int errnum)
{
    QOSA_UNUSED(errnum);
    int err = qosa_get_errno();
	switch (err) {
		case 0:
			return AOSL_HAL_RET_SUCCESS;
		case EAGAIN:
			return AOSL_HAL_RET_EAGAIN;
		case EINTR:
			return AOSL_HAL_RET_EINTR;
		default:
			return AOSL_HAL_RET_EHAL;
	}
}
