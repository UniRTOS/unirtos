#ifndef __CAMERA_CTRL_TYPES_H__
#define __CAMERA_CTRL_TYPES_H__

#include <components/avdk_utils/avdk_error.h>
#include <components/dvp_camera_types.h>
#include <components/uvc_camera_types.h>

/**
 * @brief DVP camera controller configuration structure
 * 
 * This structure holds the configuration for a DVP (Digital Video Port) camera controller.
 */
typedef struct 
{
    bk_dvp_config_t config;      /**< DVP configuration parameters */
    const bk_dvp_callback_t *cbs;  /**< DVP callback functions */
} bk_dvp_ctlr_config_t;

/**
 * @brief UVC camera controller configuration structure
 * 
 * This structure holds the configuration for a UVC (USB Video Class) camera controller.
 */
typedef struct
{
    bk_cam_uvc_config_t config;  /**< UVC configuration parameters */
    const bk_uvc_callback_t *cbs;  /**< UVC callback functions */
} bk_uvc_ctlr_config_t;

/**
 * @brief Camera controller handle type
 * 
 * This is an opaque handle to a camera controller instance.
 */
typedef struct bk_camera_ctlr *bk_camera_ctlr_handle_t;

/**
 * @brief Camera controller structure
 * 
 * This structure defines the interface for camera controllers, providing
 * function pointers for various camera operations.
 */
typedef struct bk_camera_ctlr bk_camera_ctlr_t;

/**
 * @brief Camera controller interface
 * 
 * This structure contains function pointers that define the operations
 * available on a camera controller.
 */
struct bk_camera_ctlr
{
    /**
     * @brief Open the camera
     * @param controller Pointer to the camera controller
     * @return AVDK error code
     */
    avdk_err_t (*open)(bk_camera_ctlr_t *controller);

    /**
     * @brief Close the camera
     * @param controller Pointer to the camera controller
     * @return AVDK error code
     */
    avdk_err_t (*close)(bk_camera_ctlr_t *controller);

    /**
     * @brief Suspend the camera
     * @param controller Pointer to the camera controller
     * @return AVDK error code
     */
    avdk_err_t (*suspend)(bk_camera_ctlr_t *controller);

    /**
     * @brief Resume the camera
     * @param controller Pointer to the camera controller
     * @return AVDK error code
     */
    avdk_err_t (*resume)(bk_camera_ctlr_t *controller);

    /**
     * @brief Delete the camera controller
     * @param controller Pointer to the camera controller
     * @return AVDK error code
     */
    avdk_err_t (*del)(bk_camera_ctlr_t *controller);

    /**
     * @brief IOCTL operation for the camera
     * @param controller Pointer to the camera controller
     * @param cmd Command to execute
     * @param arg Argument for the command
     * @return AVDK error code
     */
    avdk_err_t (*ioctlr)(bk_camera_ctlr_t *controller, uint32_t cmd, void *arg);
} ;


#endif