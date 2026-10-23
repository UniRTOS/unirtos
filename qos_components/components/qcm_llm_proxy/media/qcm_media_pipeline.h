#ifndef QCM_MEDIA_PIPELINE_H
#define QCM_MEDIA_PIPELINE_H

#include "qcm_llm_types.h"

qcm_llm_error_e qcm_media_prepare_asset(const qcm_media_asset_t *asset,
                                   const qcm_llm_model_profile_t *profile,
                                   size_t image_max_bytes,
                                   qcm_media_prepared_t *out_prepared,
                                   char **out_error_message);
                                   
void qcm_media_prepared_free(qcm_media_prepared_t *prepared);

#endif
