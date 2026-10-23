#ifndef QCM_LLM_BACKEND_OPENAI_COMPATIBLE_H
#define QCM_LLM_BACKEND_OPENAI_COMPATIBLE_H

#include "qcm_llm_runtime.h"

#define QCM_LLM_BACKEND_OPENAI_COMPATIBLE_ID "openai_compatible"
#define QCM_LLM_BACKEND_OPENAI_COMPATIBLE_CHAT_PATH "/chat/completions"
#define QCM_LLM_BACKEND_OPENAI_COMPATIBLE_AUTH_TYPE "bearer"
#define QCM_LLM_BACKEND_OPENAI_COMPATIBLE_DEFAULT_MAX_TOKENS_FIELD "max_tokens"

const qcm_llm_backend_vtable_t *qcm_llm_backend_openai_compatible_vtable(void);
const qcm_llm_backend_registration_t *qcm_llm_backend_openai_compatible_registration(void);

#endif
