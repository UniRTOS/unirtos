/*****************************************************************/ /**
* @file qosa_sys.h
* @brief System operation interface for UniRTOS
* @author larson.li@quectel.com
* @date 2023-04-19
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-04-19 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_Q_SYS_H__
#define __QOSA_Q_SYS_H__

#include "qosa_def.h"

#define QOSA_FLAG_INVALID_BIT 0x80000000U

/** osa error code base */
#define QOSA_ERRCODE_OS_BASE (QOSA_COMPONENT_OSI << 16)

/*! osa base component error */
typedef enum
{
    QOSA_ERROR_OK = 0,                                        /*!< success equal QOSA_OK*/

    QOSA_ERROR_MSGQ_CREATE_ERR = 1 | QOSA_ERRCODE_OS_BASE,    /*!< Message queue creation failed */
    QOSA_ERROR_MSGQ_FULL_ERR,                                 /*!< Message queue full error */
    QOSA_ERROR_MSGQ_RECV_ERR,                                 /*!< Message queue failed to receive */
    QOSA_ERROR_MSGQ_TIMEOUT_ERR,                              /*!< Message queue failed for timeout */
    QOSA_ERROR_MSGQ_INVALID_ERR,                              /*!< Message queue invalid parameter error */
    QOSA_ERROR_MSGQ_DELETE_ERR,                               /*!< Message queue delete error */
    QOSA_ERROR_MSGQ_RESET_ERR,                                /*!< Message queue reset error */
    QOSA_ERROR_MSGQ_SIZE_ERR,                                 /*!< Message queue size is not the same error */

    QOSA_ERROR_MUTEX_CREATE_ERR = 100 | QOSA_ERRCODE_OS_BASE, /*!< Failed to create mutex lock */
    QOSA_ERROR_MUTEX_LOCK_ERR,                                /*!< Failed to lock the mutex */
    QOSA_ERROR_MUTEX_EBUSY_ERR,                               /*!< other task to use this mutex  */
    QOSA_ERROR_MUTEX_INVALID_ERR,                             /*!< Mutex invalid parameter error */
    QOSA_ERROR_MUTEX_UNLOCK_ERR,                              /*!< Mutex unlock error */
    QOSA_ERROR_MUTEX_DELETE_ERR,                              /*!< Mutex delete error */

    QOSA_ERROR_SEMA_CREATE_ERR = 200 | QOSA_ERRCODE_OS_BASE,  /*!< Semaphore creation failed */
    QOSA_ERROR_SEMA_TIMEOUT_ERR,                              /*!< Timeout did not receive a valid semaphore */
    QOSA_ERROR_SEMA_INVALID_ERR,                              /*!< Semaphore invalid parameter error */
    QOSA_ERROR_SEMA_DELETE_ERR,                               /*!< Semaphore delete error */
    QOSA_ERROR_SEMA_RELEASE_ERR,                              /*!< Semaphore release error */

    QOSA_ERROR_TASK_CREATE_ERR = 300 | QOSA_ERRCODE_OS_BASE,  /*!< Task creation failed */
    QOSA_ERROR_TASK_INVALID_ERR,                              /*!< Task invalid parameter error */
    QOSA_ERROR_TASK_DELETE_ERR,                               /*!< Task delete error */
    QOSA_ERROR_TASK_SUSPEND_ERR,                              /*!< Task suspend error */
    QOSA_ERROR_TASK_RESUME_ERR,                               /*!< Task resume error */

    QOSA_ERROR_TIMER_CREATE_ERR = 400 | QOSA_ERRCODE_OS_BASE, /*!< Timer creation failed */
    QOSA_ERROR_TIMER_START_ERR,                               /*!< Timer start failed */
    QOSA_ERROR_TIMER_DELETE_ERR,                              /*!< Timer deletion failed */
    QOSA_ERROR_TIMER_INVALID_ERR,                             /*!< Timer invalid parameter error */
    QOSA_ERROR_TIMER_STOP_ERR,                                /*!< Timer stop error */

    QOSA_ERROR_FLAG_INVALID_ERR = 600 | QOSA_ERRCODE_OS_BASE, /*!< FLAG invalid param */
    QOSA_ERROR_FLAG_CREATE_ERR,                               /*!< FLAG create err */
    QOSA_ERROR_FLAG_DELETE_ERR,                               /*!< FLAG delete err */
    QOSA_ERROR_FLAG_SET_ERR,                                  /*!< FLAG set err */
    QOSA_ERROR_FLAG_WAIT_ERR,                                 /*!< FLAG wait err */
    QOSA_ERROR_FLAG_CLEAR_ERR,                                /*!< FLAG clear err */
    QOSA_ERROR_FLAG_GET_ERR,                                  /*!< FLAG get err */

    QOSA_ERROR_MAX
} qosa_errcode_os_e;

/**
 * @enum qosa_flag_clear_e
 * @brief flag auto clear flag
 */
typedef enum
{
    QOSA_FLAG_CLEAR_ENABLE = 0,
    QOSA_FLAG_CLEAR_DISABLE = 1,
} qosa_flag_clear_e;

/**
 * @enum qosa_flag_wait_e
 * @brief Wait for all flag triggers or any flag trigger
 */
typedef enum
{
    QOSA_FLAG_WAIT_ANY = 0,
    QOSA_FLAG_WAIT_ALL = 1,
} qosa_flag_wait_e;

typedef void* qosa_msgq_t;
typedef void* qosa_mutex_t;
typedef void* qosa_sem_t;
typedef void* qosa_task_t;
typedef void* qosa_timer_t;
typedef void* qosa_event_t;
typedef void* qosa_flag_t;

/**
 * @brief Used to create a message queue and initialize it
 *
 * @param[in] qosa_msgq_t * msgQRef
 *          - The message queue pointer handle
 *
 * @param[in] qosa_uint32_t size
 *          - The length of the data type to be stored in the message queue,
 *            usually the length of the data type pointer.
 *
 * @param[in] qosa_uint32_t maxNumber
 *          - The maximum number of messages in the message queue
 *
 * @return int
 *        - The function returns QOSA_ERROR_OK if the function executes successfully,
 *          otherwise it returns a negative number
 */
int qosa_msgq_create(qosa_msgq_t* msgQRef, qosa_uint32_t size, qosa_uint32_t maxNumber);

/**
* @brief Create a message queue in custom memory
*
* @param[in] qosa_msgq_t * msgQRef
*           - Pointer handle to the message queue
*
* @param[in] qosa_uint32_t size
*           - Size of each message, in bytes
*
* @param[in] qosa_uint32_t maxNumber
*           - Maximum number of messages the message queue can store
*
* @param[in] qosa_uint32_t * cb_mem
*           - Memory address of the control block
*
* @param[in] void * q_name
*           - Queue name
*
* @param[in] qosa_uint32_t * mq_mem
*           - Memory address of the message queue
*
* @param[in] qosa_uint32_t * mq_size
*           - Size of the message queue, in bytes
*
* @return int
*           - Function returns QOSA_OK on success, otherwise returns a negative number
*
* @note This function is used to create a message queue for inter-thread communication
*/
int qosa_msgq_create_static(qosa_msgq_t *msgQRef, qosa_uint32_t size, qosa_uint32_t maxNumber, void *cb_mem, void *q_name, qosa_uint8_t *mq_mem, qosa_uint32_t mq_size);

/**
 * @brief Used to delete the created message queue
 *
 * @param[in] qosa_msgq_t msgQRef
 *          - Message queue pointer handle
 *
 * @return int
 *        -  Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 */
int qosa_msgq_delete(qosa_msgq_t msgQRef);

/**
 * @brief Release (send) a message
 *
 * @param[in] qosa_msgq_t msgQRef
 *          - Message queue pointer handle
 *
 * @param[in] qosa_uint32_t size
 *          - Length of data type to be stored in message queue, must be consistent with size parameter when creating
 *
 * @param[in] qosa_uint8_t * value
 *          - First address of message data to be released
 *
 * @param[in] qosa_uint32_t timeout
 *          - QOSA_WAIT_FOREVER, QOSA_NO_WAIT, or timeout
 *
 * @return int
 *       - Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 *
 * @note
 *     - Please note that publishing may fail and return a negative value when the message queue is full
 */
int qosa_msgq_release(qosa_msgq_t msgQRef, qosa_uint32_t size, qosa_uint8_t* value, qosa_uint32_t timeout);

/**
 * @brief Wait for a message to arrive, when it is OSA_WAIT_FOREVER, if the message queue is full after
 *			  it needs to wait
 *
 * @param[in] qosa_task_t task_ref
 *          - Thread pointer handle
 *
 * @param[out] qosa_uint8_t * recvMsg
 *          - First address of data ready to receive
 *
 * @param[in] qosa_uint32_t size
 *          - Length of data type to be stored in message queue, must be consistent with size parameter when creating
 *
 * @param[in] qosa_uint32_t timeout
 *          - QOSA_WAIT_FOREVER, QOSA_NO_WAIT, or timeout
 *
 * @return int
 *       - 0 Return execution successful
 *       - Other indicates execution failed
 */
int qosa_msgq_wait(qosa_msgq_t msgQRef, qosa_uint8_t* value, qosa_uint32_t size, qosa_uint32_t timeout);

/**
 * @brief Get the number of messages currently stored in the message queue
 *
 * @param[in] qosa_msgq_t msgQRef
 *          - Message queue pointer handle
 *
 * @param[in] qosa_uint32_t * cnt_ptr
 *          - Return the number of items currently in the queue
 *
 * @return int
 *       - Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 */
int qosa_msgq_get_cnt(qosa_msgq_t msgQRef, qosa_uint32_t* cnt_ptr);

/**
  * @brief Get the size of the message queue
  *
  * @param[in] qosa_msgq_t msgQRef
  *       - Message queue handle
  *
  * @param[out] qosa_uint32_t * space
  *       - A pointer used to store the size of the queue
  *
  * @return int
  *       - The function returns QOSA_OK if executed successfully, otherwise it returns a negative number
  *
  */
int qosa_msgq_get_space(qosa_msgq_t msgQRef, qosa_uint32_t *space);

/**
 * @brief Reset message queue, clear all messages in the queue
 *
 * @param[in] qosa_msgq_t msgQRef
 *          - Message queue pointer handle
 *
 * @return int
 *       - Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 */
int qosa_msgq_reset(qosa_msgq_t msgQRef);

/**
 * @brief Create system timer
 *
 * @param[in] qosa_timer_t * timerRef
 *          - System timer pointer handle
 *
 * @param[in] void * callBackRoutine
 *          - Function used to actively notify user when timer reaches time
 *
 * @param[in] void * argv
 *          - User-defined callBackRoutine function parameter
 *
 * @return int
 *        - Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 */
int qosa_timer_create(qosa_timer_t* timerRef, void (*callBackRoutine)(void*), void* argv);

/**
 * @brief Control system timer start
 *
 * @param[in] qosa_timer_t timerRef
 *          - System timer pointer handle
 *
 * @param[in] qosa_uint32_t set_Time
 *          - Set timer waiting time interval, unit ms
 *
 * @param[in] qosa_bool_t cyclicalEn
 *          - When OSA_TRUE, it means cyclic timer, OSA_FALSE means single timer
 *
 * @return int
 *       - Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 */
int qosa_timer_start(qosa_timer_t timerRef, qosa_uint32_t set_Time, qosa_bool_t cyclicalEn);

/**
 * @brief Control system timer stop running
 *
 * @param[in] qosa_timer_t timerRef
 *          - System timer pointer handle
 *
 * @return int
 *       - Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 */
int qosa_timer_stop(qosa_timer_t timerRef);

/**
 * @brief Determine if system timer is working
 *
 * @param[in] qosa_timer_t timerRef
 *          - System timer pointer handle
 *
 * @return qosa_bool_t
 *        - OSA_TRUE means it is working, OSA_FALSE means it is not working
 */
qosa_bool_t qosa_timer_is_running(qosa_timer_t timerRef);

/**
 * @brief Used to delete timer, and release timer occupied system resources
 *
 * @param[in] qosa_timer_t timerRef
 *          - System timer pointer handle
 *
 * @return int
 *       - Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 *
 * @note
 *     - Before use, please use osa_osa_timerStop to stop timer from running
 *
 * @see  qosa_timer_stop , qosa_timer_create
 */
int qosa_timer_delete(qosa_timer_t timerRef);

/**
 * @brief Query the minimum required size of task control block (TCB) memory.
 *
 * This API returns the minimum number of bytes required for the task control
 * block (TCB) when creating a task using qosa_task_create_static().
 *
 * Behavior differences by OS:
 * - Linux (pthread):
 *   - No user-provided TCB memory is required.
 *   - This function returns 0.
 * - FreeRTOS:
 *   - Returns the minimum size required to hold a StaticTask_t structure.
 *   - The caller must ensure that tcbSize passed to
 *     qosa_task_create_static() is greater than or equal to this value.
 *
 * Usage notes:
 * - The returned value represents the minimum size only.
 * - The caller may allocate a larger buffer for future compatibility
 *   or alignment convenience.
 *
 * @return qosa_uint32_t
 *         - Minimum required TCB memory size in bytes.
 *         - 0 if the current OS does not require user-provided TCB memory.
 */
qosa_uint32_t qosa_task_get_tcb_min_size(void);

/**
 * @brief Query the alignment requirement for task control block (TCB) memory.
 *
 * This API returns the required alignment (in bytes) for the base address
 * of the task control block (TCB) memory when creating a task using
 * qosa_task_create_static().
 *
 * Behavior differences by OS:
 * - Linux (pthread):
 *   - TCB memory is managed internally by the OS.
 *   - This function returns a placeholder alignment (typically sizeof(void*))
 *     for API compatibility.
 * - FreeRTOS:
 *   - Returns the required alignment for StaticTask_t.
 *   - The caller must ensure that tcbMem is aligned to this value, otherwise
 *     undefined behavior may occur.
 *
 * Usage notes:
 * - If the returned value is non-zero, the caller should ensure:
 *       (uintptr_t)tcbMem % alignment == 0
 * - This function allows upper layers to validate memory alignment at runtime
 *   and fail fast if the requirement is not met.
 *
 * @return qosa_uint32_t
 *         - Required alignment in bytes for tcbMem.
 *         - A non-zero value indicates a strict alignment requirement.
 */
qosa_uint32_t qosa_task_get_tcb_align(void);

/**
 * @brief Create a task/thread with user-provided stack and control memory.
 *
 * This API creates a task (RTOS) or thread (Linux) using memory supplied
 * by the caller. It is designed as a unified interface across different
 * operating systems.
 *
 * Behavior differences by OS:
 * - Linux (pthread):
 *   - @p stackMem / @p stackSize are used via pthread_attr_setstack().
 *   - @p tcbMem / @p tcbSize are NOT used and will be ignored.
 * - FreeRTOS:
 *   - @p stackMem / @p stackSize are used as the task stack.
 *   - @p tcbMem / @p tcbSize are used as the static task control block
 *     (must satisfy size and alignment requirements).
 *
 * Memory ownership:
 * - The memory provided by @p stackMem and @p tcbMem is owned by the caller.
 * - This function will NOT allocate or free these memory regions.
 * - The caller must guarantee that the memory remains valid for the entire
 *   lifetime of the task/thread.
 *
 * @param[out] qosa_task_t *taskRef
 *        Pointer to receive the created task/thread handle.
 *
 * @param[in] void *stackMem
 *        Base address of the user-provided stack memory.
 *        - Must be non-NULL.
 *        - Must remain valid until the task/thread exits.
 *
 * @param[in] qosa_uint32_t stackSize
 *        Size of the stack memory in bytes.
 *        - Linux: must be >= PTHREAD_STACK_MIN.
 *        - FreeRTOS: size will be converted to StackType_t units internally.
 *
 * @param[in] void *tcbMem
 *        Base address of the user-provided task control block (TCB) memory.
 *        - Linux: ignored.
 *        - FreeRTOS: must be non-NULL and properly aligned.
 *
 * @param[in] qosa_uint32_t tcbSize
 *        Size of the task control block memory in bytes.
 *        - Linux: ignored.
 *        - FreeRTOS: must be >= qosa_task_get_tcb_min_size().
 *
 * @param[in] qosa_uint8_t priority
 *        Priority of the created task/thread.
 *        - Mapped to OS-specific priority range internally.
 *
 * @param[in] char *taskName
 *        Name of the task/thread (used for debugging and diagnostics).
 *
 * @param[in] void (*taskStart)(void *)
 *        Entry function of the task/thread.
 *
 * @param[in] void *argv
 *        User-defined argument passed to @p taskStart.
 *
 * @return int
 *         - QOSA_ERROR_OK on success.
 *         - A negative error code on failure.
 *
 * @note For FreeRTOS, the alignment requirement of @p tcbMem can be queried
 *       by calling qosa_task_get_tcb_align().
 * @note For Linux, @p tcbMem and @p tcbSize exist only for API compatibility
 *       and may be set to NULL / 0.
 */
int qosa_task_create_static(qosa_task_t* taskRef, void* stackMem, qosa_uint32_t stackSize, void* tcbMem, qosa_uint32_t tcbSize, qosa_uint8_t priority, char* taskName, void (*taskStart)(void*), void* argv, ...);

/**
 * @brief Thread create and initialize function
 *
 * @param[in] qosa_task_t * taskRef
 *          - Thread pointer handle
 *
 * @param[in] qosa_uint32_t stackSize
 *          - Stack space size for creating thread
 *
 * @param[in] qosa_uint8_t priority
 *          - Priority of creating thread
 *
 * @param[in] char * taskName
 *          - Name of creating thread
 *
 * @param[in] void * taskStart
 *          - New thread's entry function after thread is successfully created
 *
 * @param[in] void * argv
 *          - Custom parameter to be passed to new thread's entry function
 *
 * @return int
 *       - Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 */
int qosa_task_create(qosa_task_t* taskRef, qosa_uint32_t stackSize, qosa_uint8_t priority, char* taskName, void (*taskStart)(void*), void* argv, ...);

/**
 * @brief Thread stop and destroy function
 *
 * @param[in] qosa_task_t taskRef
 *          - Thread pointer handle
 *
 * @return int
 *        - Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 */
int qosa_task_delete(qosa_task_t taskRef);

/**
 * @brief Get current thread running status
 *
 * @param[in] qosa_task_t task_ref
 *          - Thread pointer handle
 *
 * @param[out] qosa_int32_t * status
 *           - Return 1 means it is running, return 0 means it is stopped
 *
 * @return int
 *       - Function executes successfully returns QOSA_ERROR_OK, otherwise returns a negative number
 */
int qosa_task_get_status(qosa_task_t task_ref, qosa_int32_t* status);

/**
* @brief Get the remaining stack space of the specified thread
*
* @param[in] qosa_task_t task_ref
*           - Thread pointer handle
*
* @param[out] qosa_int32_t * space
*           - Returns the remaining stack space
*
* @return int
*           - Function executes successfully returns OSA_OK, otherwise returns a negative number
*/
int qosa_task_get_stack_space(qosa_task_t task_ref, qosa_int32_t* space);

/**
 * @brief Thread millisecond timer
 *
 * @param[in] qosa_uint32_t ms
 *          - Millisecond time to sleep
 *
 */
void qosa_task_sleep_ms(qosa_uint32_t ms);

/**
 * @brief Thread second timer
 *
 * @param[in] qosa_uint32_t s
 *          - Time to sleep, unit seconds
 *
 */
void qosa_task_sleep_sec(qosa_uint32_t s);

#if 1
/**
 * @brief Suspend specified task thread from running
 *
 * Not actually used
 *
 * @param[in] qosa_task_t taskRef
 *          - Thread pointer handle
 *
 * @return int
 *       - 0 Return execution successful
 *       - Other indicates execution failed
 */
int qosa_task_suspend(qosa_task_t taskRef);

/**
 * @brief Resume specified task thread to run, appears in pair with suspend
 *
 * Not actually used
 *
 * @param[in] qosa_task_t taskRef
 *          - Thread pointer handle
 *
 * @return int
 *       - 0 Return execution successful
 *       - Other indicates execution failed
 */
int qosa_task_resume(qosa_task_t taskRef);

/**
 * @brief Get current running task pointer handle
 *
 * @param[out] qosa_task_t * taskRef
 *           - Returned current thread pointer handle
 *
 * @return int
 *       - 0 Return execution successful
 *       - Other indicates execution failed
 */
int qosa_task_get_current_ref(qosa_task_t* taskRef);

/**
 * @brief Enter critical section, disable task scheduling and preemption
 *
 * This function disables task scheduling to protect shared resources from
 * concurrent access between tasks. Must be paired with qosa_exit_critical().
 *
 * @note Must not be called from an interrupt service routine (ISR).
 *       For ISR context, use qosa_enter_critical_from_isr() instead.
 *
 * @see qosa_exit_critical
 */
void qosa_enter_critical(void);

/**
 * @brief Enter critical section from an interrupt service routine (ISR)
 *
 * This function saves the current interrupt mask state and disables interrupts
 * to protect shared resources accessed from ISR context. Must be paired with
 * qosa_exit_critical_from_isr().
 *
 * @return qosa_uint32_t
 *       - Saved interrupt mask state, must be passed to
 *         qosa_exit_critical_from_isr() to restore the previous state
 *
 * @see qosa_exit_critical_from_isr
 */
qosa_uint32_t qosa_enter_critical_from_isr(void);

/**
 * @brief Exit critical section, re-enable task scheduling and preemption
 *
 * This function re-enables task scheduling after a critical section entered
 * via qosa_enter_critical(). Must be called after qosa_enter_critical().
 *
 * @note Must not be called from an interrupt service routine (ISR).
 *       For ISR context, use qosa_exit_critical_from_isr() instead.
 *
 * @see qosa_enter_critical
 */
void qosa_exit_critical(void);

/**
 * @brief Exit critical section from an interrupt service routine (ISR)
 *
 * This function restores the interrupt mask state saved by
 * qosa_enter_critical_from_isr(), re-enabling interrupts as appropriate.
 * Must be paired with qosa_enter_critical_from_isr().
 *
 * @param[in] qosa_uint32_t isrm
 *          - Saved interrupt mask state returned by qosa_enter_critical_from_isr()
 *
 * @see qosa_enter_critical_from_isr
 */
void qosa_exit_critical_from_isr(qosa_uint32_t isrm);

#endif

/**
 * @brief Create a semaphore
 *
 * @param[in] qosa_sem_t * semaRef
 *          - Semaphore pointer handle
 *
 * @param[in] qosa_uint32_t initialCount
 *          - Semaphore initialization, usually 0.
 *
 * @return int
 *        - The function returns QOSA_ERROR_OK if the function executes successfully,
 *          otherwise it returns a negative number
 */
int qosa_sem_create(qosa_sem_t* semaRef, qosa_uint32_t initialCount);

/**
 * @brief Create a semaphore, you can configure the value of the maximum count of the semaphore.
 *
 * @param[in] qosa_sem_t * semaRef
 *          - Semaphore pointer handle
 *
 * @param[in] qosa_uint32_t initialCount
 *          - Semaphore initialization, usually 0
 *
 * @param[in] qosa_uint32_t max_cnt
 *          - Semaphore maximum count value
 *
 * @return int
 *       - The function returns QOSA_ERROR_OK if the function executes successfully,
 *         otherwise it returns a negative number
 */
int qosa_sem_create_ex(qosa_sem_t* semaRef, qosa_uint32_t initialCount, qosa_uint32_t max_cnt);

/**
 * @brief Waiting for the semaphore to arrive
 *
 * @param[in] qosa_sem_t semaRef
 *          - Semaphore pointer handle
 *
 * @param[in] qosa_uint32_t timeout
 *          - QOSA_WAIT_FOREVER, QOSA_NO_WAIT, or timeout
 *
 * @return int
 */
int qosa_sem_wait(qosa_sem_t semaRef, qosa_uint32_t timeout);

/**
 * @brief Release a new semaphore, and the semaphore counts after execution++
 *
 * @param[in] qosa_sem_t semaRef
 *          - Semaphore pointer handle
 *
 * @return int
 *       - The function returns QOSA_ERROR_OK if the function executes successfully,
 *         otherwise it returns a negative number
 */
int qosa_sem_release(qosa_sem_t semaRef);

/**
 * @brief Get the current count value of the semaphore
 *
 * @param[in] qosa_sem_t semaRef
 *          - Semaphore pointer handle
 *
 * @param[out] qosa_uint32_t * cnt_ptr
 *           - Returns the actual semaphore count
 *
 * @return int
 *       - The function returns QOSA_ERROR_OK if the function executes successfully,
 *         otherwise it returns a negative number
 */
int qosa_sem_get_cnt(qosa_sem_t semaRef, qosa_uint32_t* cnt_ptr);

/**
 * @brief Delete the semaphore and free resources
 *
 * @param[in] qosa_sem_t semaRef
 *          - Semaphore pointer handle
 *
 * @return int
 *       - The function returns QOSA_ERROR_OK if the function executes successfully,
 *         otherwise it returns a negative number
 */
int qosa_sem_delete(qosa_sem_t semaRef);

/**
 * @brief Create a mutex and initialize it
 *
 * @param[in] qosa_mutex_t * mutexRef
 *          - Mutex pointer handle
 *
 * @return int
 *       - The function returns QOSA_ERROR_OK if the function executes successfully,
 *         otherwise it returns a negative number
 */
int qosa_mutex_create(qosa_mutex_t* mutexRef);

/**
 * @brief If the current same thread has already acquired the lock, it can
 *        continue to add the lock. When other threads acquire the lock,
 *        it cannot add and return an error.
 *
 * @param[in] qosa_mutex_t mutexRef
 *          - Mutex pointer handle
 *
 * @return int
 *       - The function returns QOSA_ERROR_OK if the function executes successfully,
 *         otherwise it returns a negative number
 */
int qosa_mutex_try_lock(qosa_mutex_t mutexRef);

/**
 * @brief lock mutex
 *
 * @param[in] qosa_mutex_t mutexRef
 *          - Mutex pointer handle
 *
 * @param[in] qosa_uint32_t timeout
 *          - QOSA_WAIT_FOREVER, QOSA_NO_WAIT, or timeout
 *
 * @return int
 *       - The function returns QOSA_ERROR_OK if the function executes successfully,
 *         otherwise it returns a negative number
 */
int qosa_mutex_lock(qosa_mutex_t mutexRef, qosa_uint32_t timeout);

/**
 * @brief unlock mutex
 *
 * @param[in] qosa_mutex_t mutexRef
 *          - Mutex pointer handle
 *
 * @return int
 *       - The function returns QOSA_ERROR_OK if the function executes successfully,
 *         otherwise it returns a negative number
 */
int qosa_mutex_unlock(qosa_mutex_t mutexRef);

/**
 * @brief Delete the mutex and free the resource
 *
 * @param[in] qosa_mutex_t mutexRef
 *          - Mutex pointer handle
 *
 * @return int
 *       - The function returns QOSA_ERROR_OK if the function executes successfully,
 *         otherwise it returns a negative number
 */
int qosa_mutex_delete(qosa_mutex_t mutexRef);

/**
 * @brief Used to create flag object
 *
 * @param[in] qosa_flag_t *flag_ref
 *          - flag pointer
 *
 * @return qosa_errcode_os_e
 *        -  Function executes successfully returns QOSA_ERROR_OK, otherwise returns corresponding error code
 */
qosa_errcode_os_e qosa_flag_create(qosa_flag_t* flag_ref);

/**
 * @brief Used to delete flag object
 *
 * @param[in] qosa_flag_t flag_ref
 *          - flag pointer
 *
 * @return qosa_errcode_os_e
 *        -  Function executes successfully returns QOSA_ERROR_OK, otherwise returns corresponding error code
 */
qosa_errcode_os_e qosa_flag_delete(qosa_flag_t flag_ref);

/**
 * @brief Used to set flag
 *
 * @param[in] qosa_flag_t flag_ref
 *          - flag pointer
 *
 * @param[in] qosa_uint32_t flag
 *          - Flag value to be set, bit31(0x80000000) is reserved as error indicator and cannot be used
 *
 * @return qosa_errcode_os_e
 *        -  Function executes successfully returns QOSA_ERROR_OK, otherwise returns corresponding error code
 */
qosa_errcode_os_e qosa_flag_set(qosa_flag_t flag_ref, qosa_uint32_t flag);

/**
 * @brief Used to wait to receive flag
 *
 * @param[in] qosa_flag_t flag_ref
 *          - flag pointer
 *
 * @param[in] qosa_uint32_t flag
 *          - Flag value to wait for, bit31(0x80000000) is reserved as error indicator and cannot be used
 *
 * @param[in] qosa_uint32_t clear_exit
 *          - Whether to automatically clear flag after receiving flag
 *
 * @param[in] qosa_uint32_t wait_all
 *          - Wait for all flag triggers or any flag trigger
 *
 * @param[in] qosa_uint32_t timeout
 *          - Timeout for waiting flag, unit ms
 *
 * @return qosa_uint32_t
 *        -  Function executes successfully returns received flag, fails to return QOSA_ERROR_FLAG_WAIT_ERR
 */
qosa_uint32_t qosa_flag_wait(qosa_flag_t flag_ref, qosa_uint32_t flag, qosa_uint32_t clear_exit, qosa_uint32_t wait_all, qosa_uint32_t timeout);

/**
 * @brief Used to actively clear specified flag
 *
 * @param[in] qosa_flag_t flag_ref
 *          - flag pointer
 *
 * @param[in] qosa_uint32_t flag
 *          - Flag value to clear, bit31(0x80000000) is reserved as error indicator and cannot be used
 *
 * @return qosa_errcode_os_e
 *        -  Function executes successfully returns QOSA_ERROR_OK, otherwise returns corresponding error code
 */
qosa_errcode_os_e qosa_flag_clear(qosa_flag_t flag_ref, qosa_uint32_t flag);

/**
 * @brief Used to get current flag value
 *
 * @param[in] qosa_flag_t flag_ref
 *          - flag pointer
 *
 * @return qosa_uint32_t
 *        -  Function executes successfully returns current flag value, fails to return QOSA_ERROR_FLAG_GET_ERR
 */
qosa_uint32_t qosa_flag_get(qosa_flag_t flag_ref);

#endif /* __QOSA_Q_SYS_H__ */
