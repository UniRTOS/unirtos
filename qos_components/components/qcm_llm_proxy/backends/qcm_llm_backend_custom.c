/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "qcm_llm_backend_custom.h"

#include <stdlib.h>
#include <string.h>
#include "qosa_def.h"
#include "qosa_log.h"
#include "qcm_llm_backend_anthropic.h"
#include "qcm_llm_backend_openai_compatible.h"

typedef struct custom_backend_registration_node {
    qcm_llm_backend_registration_t registration;
    struct custom_backend_registration_node *next;
} custom_backend_registration_node_t;

static custom_backend_registration_node_t *s_registrations = NULL;

typedef const qcm_llm_backend_registration_t *(*builtin_backend_registration_getter_t)(void);

static const qcm_llm_backend_registration_t *find_builtin_backend_registration(const char *id)
{
    static const builtin_backend_registration_getter_t getters[] = {
        qcm_llm_backend_openai_compatible_registration,
        qcm_llm_backend_anthropic_registration,
    };
    size_t i;

    if (!id || !id[0]) {
        return NULL;
    }

    for (i = 0; i < sizeof(getters) / sizeof(getters[0]); i++) {
        const qcm_llm_backend_registration_t *registration = getters[i]();

        if (registration && registration->id && qosa_strcmp(registration->id, id) == 0) {
            return registration;
        }
    }

    return NULL;
}

static const qcm_llm_backend_registration_t *find_custom_backend_registration(const char *id)
{
    custom_backend_registration_node_t *node = s_registrations;

    if (!id || !id[0]) {
        return NULL;
    }

    while (node) {
        if (qosa_strcmp(node->registration.id, id) == 0) {
            return &node->registration;
        }
        node = node->next;
    }

    return NULL;
}

const qcm_llm_backend_registration_t *qcm_llm_find_backend_registration(const char *id)
{
    const qcm_llm_backend_registration_t *registration = find_builtin_backend_registration(id);

    if (registration) {
        return registration;
    }

    return find_custom_backend_registration(id);
}

const qcm_llm_backend_vtable_t *qcm_llm_find_custom_backend(const char *id)
{
    const qcm_llm_backend_registration_t *registration = find_custom_backend_registration(id);

    return registration ? registration->vtable : NULL;
}

qcm_llm_error_e qcm_llm_register_custom_backend(const qcm_llm_backend_registration_t *registration)
{
    custom_backend_registration_node_t *node;

    if (!registration || !registration->id || !registration->vtable) {
        return QCM_LLM_ERR_INVALID_ARG;
    }
    if (qcm_llm_find_backend_registration(registration->id)) {
        return QCM_LLM_ERR_INVALID_STATE;
    }

    node = qosa_calloc(1, sizeof(*node));

    if (!node) {
        return QCM_LLM_ERR_NO_MEM;
    }

    node->registration = *registration;
    node->next = s_registrations;
    s_registrations = node;
    return QCM_LLM_OK;
}
