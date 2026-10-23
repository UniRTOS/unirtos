#pragma once

#include <components/media_types.h>
#include <components/bk_jpeg_encode_ctlr_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new JPEG encoder controller
 *
 * This function creates and initializes a JPEG encoder controller instance.
 * The returned handle must be used for subsequent operations.
 *
 * @param handle [out] Pointer to store the created controller handle
 * @param config [in] Pointer to the JPEG encoder controller configuration
 *
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_encode_ctlr_new(bk_jpeg_encode_ctlr_handle_t *handle, bk_jpeg_encode_ctlr_config_t *config);

/**
 * @brief Open a JPEG encoder controller
 *
 * This function opens the JPEG encoder associated with the given controller handle.
 * The controller must be opened before calling bk_jpeg_encode_encode().
 *
 * @param handle [in] Handle to the JPEG encoder controller
 *
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_encode_open(bk_jpeg_encode_ctlr_handle_t handle);

/**
 * @brief Close a JPEG encoder controller
 *
 * This function closes the JPEG encoder associated with the given controller handle
 * and releases resources allocated during open/encode.
 *
 * @param handle [in] Handle to the JPEG encoder controller
 *
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_encode_close(bk_jpeg_encode_ctlr_handle_t handle);

/**
 * @brief Encode a frame into JPEG
 *
 * This function encodes an input frame into a JPEG frame.
 * The controller must be opened before calling this function.
 *
 * @param handle [in] Handle to the JPEG encoder controller
 * @param in_frame [in] Input frame buffer (raw image). The frame pointer and size/format must be valid.
 * @param out_frame [in,out] Output frame buffer to store JPEG bitstream.
 *
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_encode_encode(bk_jpeg_encode_ctlr_handle_t handle, frame_buffer_t *in_frame, frame_buffer_t *out_frame);

/**
 * @brief Perform an IOCTL operation on a JPEG encoder controller
 *
 * This function sends a device/controller-specific command to the JPEG encoder controller.
 * depends on the specific command.
 *
 * @param handle [in] Handle to the JPEG encoder controller
 * @param cmd [in] IOCTL command, see JPEG_ENCODE_IOCTL_CMD_BASE, JPEG_ENCODE_IOCTL_CMD_SET_COMPRESS_PARAM, JPEG_ENCODE_IOCTL_CMD_GET_COMPRESS_PARAM
 * @param param [in,out] Command parameter pointer (command-specific)
 *
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_encode_ioctl(bk_jpeg_encode_ctlr_handle_t handle, uint32_t cmd, void *param);

/**
 * @brief Delete a JPEG encoder controller
 *
 * This function deletes and cleans up the given JPEG encoder controller.
 * The controller should be closed before deletion.
 *
 * @param handle [in] Handle to the JPEG encoder controller
 *
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_encode_delete(bk_jpeg_encode_ctlr_handle_t handle);

#ifdef __cplusplus
}
#endif
