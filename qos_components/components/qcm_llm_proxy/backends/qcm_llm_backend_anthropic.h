#ifndef QCM_LLM_BACKEND_ANTHROPIC_H
#define QCM_LLM_BACKEND_ANTHROPIC_H

#include "qcm_llm_runtime.h"

#define QCM_LLM_BACKEND_ANTHROPIC_ID "anthropic_compatible"
#define QCM_LLM_BACKEND_ANTHROPIC_AUTH_TYPE "none"
#define QCM_LLM_BACKEND_ANTHROPIC_CHAT_PATH "/messages"
#define QCM_LLM_BACKEND_ANTHROPIC_DEFAULT_MAX_TOKENS_FIELD "max_tokens"

const qcm_llm_backend_vtable_t *qcm_llm_backend_anthropic_vtable(void);
const qcm_llm_backend_registration_t *qcm_llm_backend_anthropic_registration(void);

#endif
