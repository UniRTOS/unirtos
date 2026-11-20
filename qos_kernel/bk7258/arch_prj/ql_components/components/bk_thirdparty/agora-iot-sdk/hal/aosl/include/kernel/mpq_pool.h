#ifndef __MPQ_POOL_H__
#define __MPQ_POOL_H__

#include <api/aosl_mpqp.h>


extern aosl_mpq_t genp_queue_no_fail_argv (aosl_mpq_t dq, aosl_ref_t ref, const char *f_name, aosl_mpq_func_argv_t f, uintptr_t argc, uintptr_t *argv);

/**
 * Please be very careful when using the best q get/put
 * functions, because if you forgot to put a best q,
 * system would run into a dead loop state due to the
 * mpq load calculating mechanism.
 * -- Lionfore Hao Oct 27th, 2019
 **/
extern aosl_mpq_t genp_best_q_get (void);
extern int mpqp_best_q_put (aosl_mpq_t qid);


#endif /* __MPQ_POOL_H__ */