#include "qosa_log.h"
#include "qosa_def.h"
#include "qcm_proj_config.h"
#include "qosa_system_config.h"

#define QOS_LOG_TAG LOG_TAG_COMPONENT

#ifdef CONFIG_QCM_FILE_API_FUNC
extern void qcm_file_init(void);
#endif /* CONFIG_QCM_FILE_API_FUNC */

#ifdef CONFIG_QCM_WEBUI_FUNC
extern int qcm_webui_init(void);
extern void qcm_webui_httpsrv_start(void);
#endif /* CONFIG_QCM_WEBUI_FUNC */

#ifdef CONFIG_QCM_URC_FUNC
extern void qcm_urc_init(void);
#endif /* CONFIG_QCM_URC_FUNC */

#ifdef CONFIG_QCM_VTLS_FUNC
extern int qcm_ssl_init(void);
#endif /* CONFIG_QCM_VTLS_FUNC */

#ifdef CONFIG_QCM_QVSIM_FUNC
extern void qosa_qvsim_init(void);
#endif /* CONFIG_QCM_QVSIM_FUNC */

#ifdef CONFIG_QCM_MIFI_FUNC
extern void qcm_mifi_init(void);
#endif /* CONFIG_QCM_MIFI_FUNC */

void components_init(void)
{
    QLOGV("components_init");

#ifdef CONFIG_QCM_FILE_API_FUNC
    qcm_file_init();
#endif /* CONFIG_QCM_FILE_API_FUNC */

#ifdef CONFIG_QCM_URC_FUNC
    qcm_urc_init();
#endif /* CONFIG_QCM_URC_FUNC */

#ifdef CONFIG_QCM_VTLS_FUNC
    qcm_ssl_init();
#endif /* CONFIG_QCM_VTLS_FUNC */

#ifdef CONFIG_QCM_QVSIM_FUNC
    qosa_qvsim_init();
#endif /* CONFIG_QCM_QVSIM_FUNC */

#ifdef CONFIG_QCM_MIFI_FUNC
    qcm_mifi_init();
#endif /* CONFIG_QCM_MIFI_FUNC */
}
