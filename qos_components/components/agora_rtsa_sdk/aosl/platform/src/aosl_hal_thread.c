#include "aosl_hal_thread.h"
#include "qosa_def.h"
#include "qosa_sys.h"
int aosl_hal_thread_create(aosl_thread_t *thread, aosl_thread_param_t *param,
                           void *(*entry)(void *), void *args)
{
    qosa_task_t taskref = QOSA_NULL;
    //aosl_thread_proiority_e
    if (qosa_task_create(&taskref, param->stack_size,
                         (qosa_uint8_t)(param->priority + QOSA_PRIORITY_HIGH),
                         (char *)param->name, (void (*)(void *))entry, args) != QOSA_ERROR_OK)
    {
        return -1;
    }
    *thread = (aosl_thread_t)taskref;
    return 0;
}

void aosl_hal_thread_destroy(aosl_thread_t thread)
{
    QOSA_UNUSED(thread);
    //auto delete task resource
}

void aosl_hal_thread_exit(void *retval)
{
    QOSA_UNUSED(retval);
    //auto delete task resource
}

int aosl_hal_thread_set_name(const char *name)
{
    QOSA_UNUSED(name);
    return 0;
}

int aosl_hal_thread_set_priority(aosl_thread_proiority_e priority)
{
    QOSA_UNUSED(priority);
    return 0;
}

int aosl_hal_thread_join(aosl_thread_t thread, void **retval)
{
    QOSA_UNUSED(thread);
    QOSA_UNUSED(retval);
    return 0;
}

void aosl_hal_thread_detach(aosl_thread_t thread)
{
    QOSA_UNUSED(thread);
    return;
}

aosl_thread_t aosl_hal_thread_self(void)
{
    qosa_task_t task_ptr = QOSA_NULL;
    if (qosa_task_get_current_ref(&task_ptr) == QOSA_OK)
    {
        return (aosl_thread_t)task_ptr;
    }
    return QOSA_NULL;
}

aosl_mutex_t aosl_hal_mutex_create(void)
{
    qosa_mutex_t mutexRef = QOSA_NULL;
    if (qosa_mutex_create(&mutexRef) != QOSA_ERROR_OK)
    {
        return QOSA_NULL;
    }
    return (aosl_mutex_t)mutexRef;
}

void aosl_hal_mutex_destroy(aosl_mutex_t mutex)
{
    qosa_mutex_delete((qosa_mutex_t)mutex);
}

int aosl_hal_mutex_lock(aosl_mutex_t mutex)
{
    return qosa_mutex_lock((qosa_mutex_t)mutex, QOSA_WAIT_FOREVER);
}

int aosl_hal_mutex_trylock(aosl_mutex_t mutex)
{
    return qosa_mutex_try_lock((qosa_mutex_t)mutex);
}

int aosl_hal_mutex_unlock(aosl_mutex_t mutex)
{
    return qosa_mutex_unlock((qosa_mutex_t) mutex);
}

typedef struct {
    qosa_sem_t sem;
    qosa_uint8_t waitingTasks;
}pCondId_t;

aosl_cond_t aosl_hal_cond_create(void)
{
    pCondId_t *cond = (pCondId_t *)qosa_malloc(sizeof(pCondId_t));
    QOSA_ASSERT((cond != QOSA_NULL));

    qosa_sem_t semaRef = QOSA_NULL;
    qosa_sem_create(&semaRef, 0);
    cond->sem = semaRef;
    QOSA_ASSERT((cond->sem!= QOSA_NULL));
    cond->waitingTasks = 0;
    return (aosl_cond_t)cond;
}

void aosl_hal_cond_destroy(aosl_cond_t cond)
{
    pCondId_t * condId = (pCondId_t *)cond;
    qosa_sem_delete(condId->sem);
    condId->sem = QOSA_NULL;
    condId->waitingTasks = 0;
    qosa_free(condId);
    condId = NULL;
    return;
}

int aosl_hal_cond_signal(aosl_cond_t cond)
{
    pCondId_t * condId = (pCondId_t *)cond;
    if(condId->waitingTasks > 0)
    qosa_sem_release(condId->sem);
    return 0;
}

int aosl_hal_cond_broadcast(aosl_cond_t cond)
{
  pCondId_t * condId = (pCondId_t *)cond;
  qosa_enter_critical();
  qosa_uint8_t waitCnt = condId->waitingTasks;
  qosa_exit_critical();
  while(waitCnt-- > 0)
  {
    qosa_sem_release(condId->sem);
  }
  return 0;
}

int aosl_hal_cond_wait(aosl_cond_t cond, aosl_mutex_t mutex)
{
  pCondId_t * condId = (pCondId_t *)cond;
  qosa_enter_critical();
  condId->waitingTasks++;
  qosa_exit_critical();
  qosa_mutex_unlock((qosa_mutex_t)mutex);

  qosa_sem_wait(condId->sem, QOSA_WAIT_FOREVER);
  qosa_mutex_lock((qosa_mutex_t)mutex, QOSA_WAIT_FOREVER);
  qosa_enter_critical();
  condId->waitingTasks--;
  qosa_exit_critical();
  return 0;
}

int aosl_hal_cond_timedwait(aosl_cond_t cond, aosl_mutex_t mutex, intptr_t timeout_ms)
{
  pCondId_t * condId = (pCondId_t *)cond;
  qosa_enter_critical();
  condId->waitingTasks++;
  qosa_exit_critical();
  qosa_mutex_unlock((qosa_mutex_t)mutex);

  qosa_sem_wait(condId->sem, timeout_ms);
  qosa_mutex_lock((qosa_mutex_t)mutex, QOSA_WAIT_FOREVER);
  qosa_enter_critical();
  condId->waitingTasks--;
  qosa_exit_critical();
  return 0;
}

aosl_sem_t aosl_hal_sem_create(void)
{
    qosa_sem_t semaRef = QOSA_NULL;
    qosa_sem_create(&semaRef, 0);

    return (aosl_sem_t)semaRef;
}

void aosl_hal_sem_destroy(aosl_sem_t sem)
{
  qosa_sem_delete((qosa_sem_t)sem);
}

int aosl_hal_sem_post(aosl_sem_t sem)
{
  return qosa_sem_release((qosa_sem_t)sem);
}

int aosl_hal_sem_wait(aosl_sem_t sem)
{
  return qosa_sem_wait((qosa_sem_t)sem, QOSA_WAIT_FOREVER);
}

int aosl_hal_sem_timedwait(aosl_sem_t sem, intptr_t timeout_ms)
{
  return qosa_sem_wait((qosa_sem_t)sem,timeout_ms);
}
