#include <stddef.h>

#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "unirtos_apps_config.h"

#ifdef CONFIG_QOSA_OEM_DL_FUNC
#include "unirtos_atcmd_gw_cfg.h"
#endif

#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO
#define UNIRTOS_APP_INIT_MAX_COUNT 96

#ifdef CONFIG_QCM_URC_FUNC
extern void qcm_urc_start_rdy(char *str);
#endif

#ifdef CONFIG_QOSA_NBIOT_CUST_FUNC
extern void qcm_urc_start_nb(void);
#endif

#ifdef CONFIG_QAPP_UNIRTOS_STD_FUNC
extern void unir_atcmd_init(void);

static void unir_unirtos_std_bootstrap(void)
{
    unir_atcmd_init();
#ifdef CONFIG_QCM_URC_FUNC
#ifdef CONFIG_QOSA_NBIOT_CUST_FUNC
    qcm_urc_start_nb();
#else
    qcm_urc_start_rdy("RDY");
#endif
#endif
}

UNIRTOS_APP_EXPORT(100, "unirtos_std", unir_unirtos_std_bootstrap);
#endif

#ifdef CONFIG_QAPP_UNIRTOS_STD_DL_FUNC
extern void unir_gw_atcmd_init(void);

/* GW dispatch entries are now wired into unirtos_std's unir_img_at_desc[] */
static void unir_unirtos_gw_init_bootstrap(void)
{
    unir_gw_atcmd_init();
}

UNIRTOS_APP_EXPORT(110, "unirtos_gw_init",     unir_unirtos_gw_init_bootstrap);
#endif

static void unirtos_app_sort_entries(const unirtos_app_init_entry_t **entries, size_t count)
{
    size_t i;

    for (i = 1; i < count; ++i)
    {
        const unirtos_app_init_entry_t *current = entries[i];
        size_t j = i;

        while (j > 0 && entries[j - 1]->order > current->order)
        {
            entries[j] = entries[j - 1];
            --j;
        }

        entries[j] = current;
    }
}

void unirtos_app_run_registered_init(void)
{
    const unirtos_app_init_entry_t *start = __unirtos_app_init_start;
    const unirtos_app_init_entry_t *end = __unirtos_app_init_end;
    const unirtos_app_init_entry_t *ordered[UNIRTOS_APP_INIT_MAX_COUNT];
    size_t count = (size_t)(end - start);
    size_t i;

    if (count == 0)
    {
        return;
    }

    if (count > UNIRTOS_APP_INIT_MAX_COUNT)
    {
        QLOGW("registered app init count %u exceeds max %u", (unsigned int)count, UNIRTOS_APP_INIT_MAX_COUNT);
        count = UNIRTOS_APP_INIT_MAX_COUNT;
    }

    /* Keep execution order stable even though init handlers are collected from a linker section. */
    for (i = 0; i < count; ++i)
    {
        ordered[i] = &start[i];
    }

    unirtos_app_sort_entries(ordered, count);

    /* Runtime dispatch is centralized here for both built-in apps and external qos_applications projects. */
    for (i = 0; i < count; ++i)
    {
        if (ordered[i]->fn == NULL)
        {
            continue;
        }

        QLOGV("app init: %s", ordered[i]->name);
        ordered[i]->fn();
    }
}

void apps_init(void)
{
    unirtos_app_run_registered_init();
}
