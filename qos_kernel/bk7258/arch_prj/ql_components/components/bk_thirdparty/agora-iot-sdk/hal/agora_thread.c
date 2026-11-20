#include <os/os.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"

#ifndef RTOS_HIGHEST_PRIORITY
#define RTOS_HIGHEST_PRIORITY            (configMAX_PRIORITIES-1)
#endif


bk_err_t agora_create_thread(beken_thread_t *thread, uint8_t priority, const char *name, 
                             beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg)
{
    bk_err_t ret;
    
    /* Limit priority to default lib priority */
    if (priority > RTOS_HIGHEST_PRIORITY) {
        priority = RTOS_HIGHEST_PRIORITY;
    }
    
#if CONFIG_AGORA_RTC_THREAD_BIND_CPU
    #if CONFIG_AGORA_RTC_THREAD_BIND_CPU_ID == 0
    #if CONFIG_TASK_STACK_IN_PSRAM && CONFIG_PSRAM_AS_SYS_MEMORY
        ret = rtos_core0_create_psram_thread(thread,
                                    priority,
                                    name,
                                    function,
                                    stack_size,
                                    arg);
    #else
        ret = rtos_core0_create_thread(thread,
                                    priority,
                                    name,
                                    function,
                                    stack_size,
                                    arg);
    #endif
    #elif CONFIG_AGORA_RTC_THREAD_BIND_CPU_ID == 1
    #if CONFIG_TASK_STACK_IN_PSRAM && CONFIG_PSRAM_AS_SYS_MEMORY
        ret = rtos_core1_create_psram_thread(thread,
                                    priority,
                                    name,
                                    function,
                                    stack_size,
                                    arg);
    #else
        ret = rtos_core1_create_thread(thread,
                                    priority,
                                    name,
                                    function,
                                    stack_size,
                                    arg);
    #endif
    #endif
#else
    /* Create thread without CPU binding */
    ret = rtos_create_thread(thread,
                            priority,
                            name,
                            function,
                            stack_size,
                            arg);
#endif

    return ret;
}