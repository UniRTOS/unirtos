
#include "aosl_hal_log.h"
#include "qosa_def.h"
#include "qosa_log.h"

#define QOS_LOG_TAG                   LOG_TAG_AGORA_RTC

char outlog[2048] = {0};

int aosl_hal_printf(const char *format, va_list args)
{
    qosa_memset(outlog,0,2048);
    int num = qosa_vsprintf(outlog,format,args);
    QLOGI("%s",outlog);
    return num;
}
