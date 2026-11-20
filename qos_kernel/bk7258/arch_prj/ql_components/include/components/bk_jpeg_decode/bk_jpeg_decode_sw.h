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

#include "components/bk_jpeg_decode/bk_jpeg_decode_utils.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_types_sw.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Software JPEG Decoder API
 * 
 * This file provides the external interface for the software-based JPEG decoder module.
 * All functions in this file implement JPEG decoding using software algorithms
 * rather than dedicated hardware.
 */

/**
 * @brief Create a new software JPEG decoder instance
 *
 * This function initializes and creates a new software-based JPEG decoder instance
 * with the specified configuration parameters. It allocates memory for the decoder
 * controller structure and initializes it with the provided configuration.
 *
 * @param handle [out] Pointer to store the decoder handle, which will be used for subsequent operations
 * @param config [in] Decoder configuration parameters, including callback functions, output format, etc.
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_software_jpeg_decode_new(bk_jpeg_decode_sw_handle_t *handle, bk_jpeg_decode_sw_config_t *config);

/**
 * @brief Create a new software JPEG decoder instance optimized for multi-core execution
 *
 * This function creates a software JPEG decoder instance specifically optimized
 * for execution across multiple cores, which can provide better performance by
 * distributing the decoding workload.
 *
 * @param handle [out] Pointer to store the decoder handle, which will be used for subsequent operations
 * @param config [in] Decoder configuration parameters, including callback functions, output format, etc.
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_software_jpeg_decode_on_multi_core_new(bk_jpeg_decode_sw_handle_t *handle, bk_jpeg_decode_sw_config_t *config);

/**
 * @brief Open the software JPEG decoder
 *
 * This function prepares the software JPEG decoder for operation by
 * initializing internal resources, setting up the decoding environment,
 * and calling the underlying JPEG decoder's initialization function.
 * The decoder must be opened before starting any decoding operations.
 *
 * @param handle [in] Software decoder handle obtained from bk_software_jpeg_decode_new or bk_software_jpeg_decode_on_multi_core_new
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_sw_open(bk_jpeg_decode_sw_handle_t handle);

/**
 * @brief Close the software JPEG decoder
 *
 * This function closes the software JPEG decoder, releasing any resources
 * that were allocated during the open operation and calling the underlying
 * JPEG decoder's deinitialization function. After closing, the decoder
 * can be reopened or deleted.
 *
 * @param handle [in] Software decoder handle
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_sw_close(bk_jpeg_decode_sw_handle_t handle);

/**
 * @brief Start JPEG decoding process
 *
 * This function initiates the decoding of a JPEG frame using the software decoder.
 * It configures rotation, output format, and other parameters, then calls the
 * underlying JPEG decoder's start function to begin the decoding process.
 * The decoder must be opened before calling this function.
 *
 * @param handle [in] Software decoder handle
 * @param frame [in] Input JPEG frame buffer containing the encoded JPEG data
 * @param out_frame [out] Output decoded frame buffer where the decoded image will be stored
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_sw_decode(bk_jpeg_decode_sw_handle_t handle, frame_buffer_t *frame, frame_buffer_t *out_frame);

/**
 * @brief Start JPEG decoding process asynchronously
 *
 * This function initiates the decoding of a JPEG frame using the software decoder
 * in an asynchronous manner. It configures rotation, output format, and other parameters,
 * then calls the underlying JPEG decoder's start function to begin the decoding process.
 * The decoder must be opened before calling this function.
 *
 * @param handle [in] Software decoder handle
 * @param frame [in] Input JPEG frame buffer containing the encoded JPEG data
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_sw_decode_async(bk_jpeg_decode_sw_handle_t handle, frame_buffer_t *frame);

/**
 * @brief Delete the software JPEG decoder instance
 *
 * This function deletes the software JPEG decoder instance, releasing all
 * associated resources including the decoder controller structure memory.
 * After deletion, the handle is no longer valid for any operation.
 *
 * @param handle [in] Software decoder handle to be deleted
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_sw_delete(bk_jpeg_decode_sw_handle_t handle);

/**
 * @brief Set output frame configuration for software JPEG decoder
 *
 * This function allows setting output frame configuration parameters for the software JPEG decoder,
 * specifically output format and byte order.
 *
 * @param handle [in] Software decoder handle
 * @param config [in] Output frame configuration parameters to be set
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_sw_set_config(bk_jpeg_decode_sw_handle_t handle, bk_jpeg_decode_sw_out_frame_info_t *config);

/**
 * @brief Get image information before decoding
 *
 * This function retrieves image information from the input JPEG frame before actual decoding.
 * It provides details such as image dimensions, color format, and other relevant parameters
 * by calling the bk_get_jpeg_data_info function to parse the JPEG header.
 *
 * @param handle [in] Software decoder handle
 * @param info [out] Pointer to store the retrieved image information
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_sw_get_img_info(bk_jpeg_decode_sw_handle_t handle, bk_jpeg_decode_img_info_t *info);

/**
 * @brief Extended control interface for software JPEG decoder
 *
 * This function provides an extended interface for controlling the software JPEG decoder
 * and accessing additional functionalities beyond basic decoding operations. Currently,
 * this function is a placeholder that always returns success without performing any operations.
 *
 * @param handle [in] Software decoder handle
 * @param cmd [in] Command code specifying the operation to perform (defined in jpeg_decode_sw_ioctl_cmd_t)
 * @param param [in/out] Command parameters specific to the operation being performed
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_sw_ioctl(bk_jpeg_decode_sw_handle_t handle, bk_jpeg_decode_sw_ioctl_cmd_t cmd, void *param);

#ifdef __cplusplus
}
#endif
