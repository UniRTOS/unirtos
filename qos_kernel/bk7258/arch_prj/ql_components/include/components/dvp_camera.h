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

#include <components/dvp_camera_types.h>

#ifdef __cplusplus
extern "C" {
#endif

const dvp_sensor_config_t **get_sensor_config_devices_list(void);
int get_sensor_config_devices_num(void);

const dvp_sensor_config_t *get_sensor_config_interface_by_id(sensor_id_t id);


/**
* @brief This API set dvp camera device list
* @return
*	  - void
*/
void bk_dvp_set_devices_list(const dvp_sensor_config_t **list, uint16_t size);

/**
 * @brief     enumerate dvp camera
 *
 * This API will auto detect dvp sensor, and init sensor, i2c module, psram, dma, ect.
 *
 * @param config sensor config
 *
 * @attation 1. bk_dvp_camera_driver_init api include bk_dvp_camera_enumerate function
 *
 * @return
 *    - dvp_sensor_config: sensor ptr
 *    - NULL: not found
 */
const dvp_sensor_config_t *bk_dvp_detect(bk_dvp_config_t *cfg);

/**
 * @brief     Init the camera
 *
 * This API will auto detect dvp sensor, and init sensor, jpeg module, i2c module, psram, dma, ect.
 *
 * @param handle init handle
 * @param config dvp config
 * @param callback frame callback
 * @param encode_buffer encode buffer
 *
 * @attation 1. you need make sure upper module exist.
 *           2. only work in encode mode(H264 or MJPEG) need to set encode_buffer.
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
bk_err_t bk_dvp_open(camera_handle_t *handle, bk_dvp_config_t *config, const bk_dvp_callback_t *callback, uint8_t *encode_buffer);

/**
 * @brief     close the camera
 *
 * This API will deinit  sensor, jpeg module, i2c module, psram, dma, ect.
 *
* @param handle
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
bk_err_t bk_dvp_close(camera_handle_t handle);

/**
 * @brief     auto detect dvp camera
 *
 * This API called by user, before use dvp camera
 *
 * @attation 1. This api return a pointer for dvp camera sensor config
 *
 * @return
 *    - get current dvp config(type)
 *    - return NULL
 */
const dvp_sensor_config_t *bk_dvp_get_sensor_auto_detect(void);

/**
 * @brief     regenerate idr frame
 *
 * This API called by user, once call this api, will regenerate idr frame
 *
 * @param handle
 *
 * @attation 1. This api only effect in camera is working, and work in h264 mode or h264&yuv mode,
 *            and user need config h264 param by bk_dvp_set_h264_param api.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_dvp_h264_idr_reset(camera_handle_t handle);

/**
 * @brief     suspend dvp
 *
 * This API will suspend dvp stream
 *
 * @param handle the dvp init device handle
 *
 * @return
 *    - BK_OK: set success
 *    - others: other errors.
 */
bk_err_t bk_dvp_suspend(camera_handle_t handle);

/**
 * @brief     resume dvp
 *
 * This API will resume dvp stream
 *
 * @param handle the dvp init device handle
 *
 * @return
 *    - BK_OK: set success
 *    - others: other errors.
 */
bk_err_t bk_dvp_resume(camera_handle_t handle);

/**
 * @brief     write dvp sensor register
 *
 * This API will write dvp sensor register
 *
 * @param handle the dvp init device handle
 * @param reg_val the dvp sensor register value
 *
 * @return
 *    - BK_OK: set success
 *    - others: other errors.
 */
bk_err_t bk_dvp_sensor_write_register(camera_handle_t handle, dvp_sensor_reg_val_t *reg_val);

/**
 * @brief     read dvp sensor register
 *
 * This API will read dvp sensor register
 *
 * @param handle the dvp init device handle
 * @param reg_val the dvp sensor register value
 *
 * @return
 *    - BK_OK: set success
 *    - others: other errors.
 */
bk_err_t bk_dvp_sensor_read_register(camera_handle_t handle, dvp_sensor_reg_val_t *reg_val);

#ifdef __cplusplus
}
#endif
