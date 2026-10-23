// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "components/bk_jpeg_decode/bk_jpeg_decode_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get JPEG data information
 * @details This function can be called independently to obtain image information
 * @param[in] img_info Image information structure pointer
 * @return AVDK_ERR_OK on success, others on failure
 */
avdk_err_t bk_get_jpeg_data_info(bk_jpeg_decode_img_info_t *img_info);

#ifdef __cplusplus
}
#endif
