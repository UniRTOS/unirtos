/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_THREAD_H__
#define __HAL_VOLC_THREAD_H__

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#if defined(__BUILDING_BYTE_RTC_SDK__)
#define __byte_rtc_api__ __declspec(dllexport)
#else
#define __byte_rtc_api__ __declspec(dllimport)
#endif
#else
#define __byte_rtc_api__ __attribute__((visibility("default")))
#endif

/**
 * @brief 定义线程名称的最大长度。
 * 
 * 该宏定义了线程名称字符串的最大允许长度，设置为 16 个字符。
 * 这有助于确保线程名称不会过长，避免在存储和处理线程名称时出现问题。
 */
#define VOLC_THREAD_NAME_MAX_LENGTH 16


/**
 * @locale zh
 * @type keytype
 * @brief 线程id
 */
typedef void* volc_tid_t;

/**
 * @brief 定义线程本地存储的类型，使用 void* 指针表示。
 */
typedef void* volc_thread_local_t;

/**
 * @brief 定义线程参数的结构体。
 * 
 * 该结构体用于存储创建线程时所需的参数，包括线程名称、优先级、栈大小和绑定的 CPU 核心。
 */
typedef struct {
    /**
     * @brief 线程的名称，最大长度由 VOLC_THREAD_NAME_MAX_LENGTH 定义。
     */
    char name[VOLC_THREAD_NAME_MAX_LENGTH];
    /**
     * @brief 线程的优先级。
     */
    int  priority;
    /**
     * @brief 线程的栈大小。
     */
    int  stack_size;
    /**
     * @brief 线程绑定的 CPU 核心。
     */
    int  bind_cpu;
    int stack_in_ext;
} volc_thread_param_t;

/**
 * @brief 获取当前线程的 ID。
 * 
 * @return 当前线程的 ID。
 */
__byte_rtc_api__ volc_tid_t volc_thread_get_id(void);

/**
 * @brief 获取指定线程的名称。
 * 
 * @param thread 要获取名称的线程的 ID。
 * @param name 用于存储线程名称的字符数组。
 * @param len 字符数组的长度。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_thread_get_name(volc_tid_t thread, char* name, uint32_t len);

/**
 * @brief 设置当前线程的名称。
 * 
 * @param name 要设置的线程名称。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_thread_set_name(const char* name);

/**
 * @brief 创建一个新的线程。
 * 
 * @param thread 指向存储新线程 ID 的变量的指针。
 * @param param 指向线程参数结构体的指针。
 * @param start_routine 线程启动时执行的函数指针。
 * @param args 传递给线程启动函数的参数。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_thread_create(volc_tid_t* thread, const volc_thread_param_t* param, void* (*start_routine)(void *), void* args);

/**
 * @brief 等待指定线程结束并获取其返回值。
 * 
 * @param thread 要等待的线程的 ID。
 * @param ret 指向存储线程返回值的变量的指针。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_thread_join(volc_tid_t thread, void* ret);

/**
 * @brief 取消指定线程的执行。
 * 
 * @param thread 要取消的线程的 ID。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_thread_cancel(volc_tid_t thread);

/**
 * @brief 将指定线程设置为分离状态。
 * 
 * @param thread 要设置为分离状态的线程的 ID。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_thread_detach(volc_tid_t thread);

/**
 * @brief 销毁指定线程。
 * 
 * @param thread 要销毁的线程的 ID。
 */
__byte_rtc_api__ void volc_thread_destroy(volc_tid_t thread);

/**
 * @brief 退出指定线程。
 * 
 * @param thread 要退出的线程的 ID。
 */
__byte_rtc_api__ void volc_thread_exit(volc_tid_t thread);

/**
 * @brief 使当前线程休眠指定的时间。
 * 
 * @param time 要休眠的时间（毫秒）。
 */
__byte_rtc_api__ void volc_thread_sleep(uint64_t time);

/**
 * @brief 使当前线程休眠直到指定的时间。
 * 
 * @param time 要休眠到的时间（毫秒）。
 */
__byte_rtc_api__ void volc_thread_sleep_until(uint64_t time);

/**
 * @brief 外部声明的线程本地存储变量。
 */
extern volc_thread_local_t g_thread_local;

/**
 * @brief 创建一个新的线程本地存储对象。
 * 
 * @return 新创建的线程本地存储对象的句柄。
 */
__byte_rtc_api__ volc_thread_local_t volc_thread_local_create(void);

/**
 * @brief 销毁指定的线程本地存储对象。
 * 
 * @param local 要销毁的线程本地存储对象的句柄。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_thread_local_destroy(volc_thread_local_t local);

/**
 * @brief 设置线程本地存储对象的值。
 * 
 * @param local 要设置值的线程本地存储对象的句柄。
 * @param data 要存储的数据指针。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_thread_local_set(volc_thread_local_t local, void* data);

/**
 * @brief 清理线程本地存储对象的值。
 * 
 * @param local 要清理值的线程本地存储对象的句柄。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_thread_local_clean(volc_thread_local_t local);

/**
 * @brief 获取线程本地存储对象的值。
 * 
 * @param local 要获取值的线程本地存储对象的句柄。
 * @return 存储在该对象中的数据指针。
 */
__byte_rtc_api__ void* volc_thread_local_get(volc_thread_local_t local);



#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_THREAD_H__ */
