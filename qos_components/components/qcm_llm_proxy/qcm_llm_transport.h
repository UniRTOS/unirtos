
#ifndef  QCM_LLM_TRANSPORT_H
#define  QCM_LLM_TRANSPORT_H

#include <stdbool.h>

#include "qcm_llm_types.h"

qcm_llm_error_e qcm_llm_http_post_json(const qcm_llm_http_json_request_t *request,
                                  qcm_llm_http_response_t *out_response,
                                  char **out_error_message);

void qcm_llm_http_response_free(qcm_llm_http_response_t *response);

void qcm_llm_http_arm_abort(volatile bool *flag);

void qcm_llm_http_disarm_abort(void);

#endif