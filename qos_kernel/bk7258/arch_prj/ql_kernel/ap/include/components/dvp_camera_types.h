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

#include <common/bk_err.h>
#include <components/media_types.h>
#include <driver/hal/hal_yuv_buf_types.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Size of H.264 self-defined SEI data
 */
#define H264_SELF_DEFINE_SEI_SIZE (96)

/**
 * @brief DVP sensor ID types
 * @details Enumeration of supported DVP sensor IDs
 */
typedef enum
{
    ID_UNKNOW = 0,    /**< Unknown sensor ID */
    ID_PAS6329,       /**< PAS6329 sensor */
    ID_OV7670,        /**< OV7670 sensor */
    ID_PAS6375,       /**< PAS6375 sensor */
    ID_GC0328C,       /**< GC0328C sensor */
    ID_BF2013,        /**< BF2013 sensor */
    ID_GC0308C,       /**< GC0308C sensor */
    ID_HM1055,        /**< HM1055 sensor */
    ID_GC2145,        /**< GC2145 sensor */
    ID_OV2640,        /**< OV2640 sensor */
    ID_GC0308,        /**< GC0308 sensor */
    ID_TVP5150,       /**< TVP5150 sensor */
    ID_SC101          /**< SC101 sensor */
} sensor_id_t;

/**
 * @brief Sensor data bit width types
 * @details Enumeration of supported data bit widths for DVP sensors
 */
typedef enum
{
    SENSOR_BITS_WIDTH_8BIT = 8,   /**< 8-bit data width */
    SENSOR_BITS_WIDTH_10BIT = 10, /**< 10-bit data width */
    SENSOR_BITS_WIDTH_12BIT = 12, /**< 12-bit data width */
    SENSOR_BITS_WIDTH_16BIT = 16, /**< 16-bit data width */
} sensor_bits_width_t;

typedef enum
{
    DVP_IOCTL_CMD_H264_IDR_RESET = 0, /**< Regenerate idr frame */
    DVP_IOCTL_CMD_SENSOR_WRITE_REGISTER = 1,
    DVP_IOCTL_CMD_SENSOR_READ_REGISTER = 2, /**< Read sensor register */
} dvp_ioctl_cmd_t;

/**
 * @brief DVP IO configuration structure
 * @details Configuration parameters for DVP interface IO pins
 */
typedef struct
{
    sensor_bits_width_t data_width; /**< Data bus width */
    uint8_t data_pin[16];             /**< Data IO pins (up to 16 pins) */
    uint8_t vsync_pin;                /**< Vertical sync IO pin */
    uint8_t hsync_pin;                /**< Horizontal sync IO pin */
    uint8_t xclk_pin;                 /**< Core input clock to sensor */
    uint8_t pclk_pin;                 /**< Sensor output clock to core */
} bk_dvp_io_config_t;

/**
 * @brief DVP I2C configuration structure
 * @details Configuration parameters for I2C interface used by DVP sensor
 */
typedef struct
{
    uint8_t id;          /**< I2C bus ID */
    uint8_t scl_pin;       /**< I2C clock line IO pin */
    uint8_t sda_pin;       /**< I2C data line IO pin */
    uint32_t baud_rate;  /**< I2C bus baud rate */
    uint32_t flags;      /**< I2C configuration flags */
} bk_dvp_i2c_config_t;

/**
 * @brief DVP camera configuration structure
 * @details Complete configuration parameters for DVP camera
 */
typedef struct
{
    bk_dvp_i2c_config_t i2c_config; /**< I2C configuration */
    uint8_t reset_pin;                /**< Reset IO pin */
    uint8_t pwdn_pin;                 /**< Power down IO pin */
    bk_dvp_io_config_t io_config;   /**< IO configuration */
    mclk_freq_t clk_source;         /**< Core input clock to sensor */
    uint16_t img_format;            /**< Image format */
    uint16_t width;                 /**< Image width */
    uint16_t height;                /**< Image height */
    uint32_t fps;                   /**< Frames per second */
    void *user_data;                /**< User data pointer */
} bk_dvp_config_t;

/**
 * @brief DVP camera callback functions
 * @details Callback functions for DVP camera operations
 * @attention In the callback functions, no blocking operations or long operations should be performed, otherwise the hardware interrupt will be delayed, leading to abnormal image data.
 */
typedef struct
{
    /**
     * @brief Allocate frame buffer
     * @param format Image format
     * @param size Buffer size
     * @return Pointer to allocated frame buffer
     */
    frame_buffer_t *(*malloc)(image_format_t format, uint32_t size);

    /**
     * @brief Frame completion callback
     * @param format Image format
     * @param frame Pointer to frame buffer
     * @param result Operation result
     */
    void (*complete)(image_format_t format, frame_buffer_t *frame, int result);
} bk_dvp_callback_t;

/**
 * @brief DVP sensor register value structure
 * @details Structure to hold sensor register address and value pairs
 */
typedef struct
{
    uint32_t reg; /**< Register address */
    uint32_t val; /**< Register value */
} dvp_sensor_reg_val_t;

/**
 * @brief DVP sensor configuration structure
 * @details Configuration parameters and function pointers for DVP sensor
 */
typedef struct
{
    char *name;                     /**< Sensor name */
    media_ppi_t def_ppi;            /**< Sensor default resolution */
    frame_fps_t def_fps;            /**< Sensor default FPS */
    mclk_freq_t  clk;               /**< Sensor working clock for configured FPS and PPI */
    pixel_format_t fmt;             /**< Sensor input data format */
    sync_level_t vsync;             /**< Sensor VSYNC active level */
    sync_level_t hsync;             /**< Sensor HSYNC active level */
    uint16_t id;                    /**< Sensor type (sensor_id_t) */
    uint16_t address;               /**< Sensor write register address by I2C */
    uint16_t fps_cap;               /**< Sensor supported FPS */
    uint16_t ppi_cap;               /**< Sensor supported resolutions */

    /**
     * @brief Auto-detect used DVP sensor
     * @return true if sensor detected, false otherwise
     */
    bool (*detect)(void);

    /**
     * @brief Initialize DVP sensor
     * @return 0 on success, error code on failure
     */
    int (*init)(void);

    /**
     * @brief Set resolution of sensor
     * @param ppi Resolution to set
     * @return 0 on success, error code on failure
     */
    int (*set_ppi)(media_ppi_t ppi);

    /**
     * @brief Set FPS of sensor
     * @param fps FPS to set
     * @return 0 on success, error code on failure
     */
    int (*set_fps)(frame_fps_t fps);

    /**
     * @brief Power down or reset sensor
     * @return 0 on success, error code on failure
     */
    int (*power_down)(void);

    /**
     * @brief Read sensor register
     * @param reg Register address
     * @param val Pointer to store register value
     * @return 0 on success, error code on failure
     */
    int (*read_register)(uint32_t reg, uint32_t *val);

    /**
     * @brief Write sensor register
     * @param reg Register address
     * @param val Register value to write
     * @return 0 on success, error code on failure
     */
    int (*write_register)(uint32_t reg, uint32_t val);
} dvp_sensor_config_t;

/**
 * @brief Camera sensor detection function structure
 */
typedef struct
{
    const dvp_sensor_config_t *(*detect)(void);  /**< Detection function pointer */
} dvp_sensor_detect_func_t;

/**
 * @brief Section attribute macro implementation
 * @param SECTION Section name
 * @param COUNTER Counter
 */
#define _SECTION_ATTR_IMPL(SECTION, COUNTER)    __attribute__((section(SECTION "." _CPIMTER_STRINGIFY(COUNTER))))

/**
 * @brief Stringify macro
 * @param COUNTER Counter
 */
#define _CPIMTER_STRINGIFY(COUNTER) #COUNTER

/**
 * @brief Camera sensor detection function section registration macro
 * @param f Detection function name
 *
 * This macro registers a sensor detection function into a dedicated section,
 * enabling the system to automatically discover and run detection routines.
 */
#define BK_CAMERA_SENSOR_DETECT_SECTION(f)                                                        \
    const dvp_sensor_config_t * __bk_sensor_##f(void);                                            \
    __attribute__((used)) _SECTION_ATTR_IMPL(".camera_sensor_detect_function_list", __COUNTER__)  \
    const dvp_sensor_detect_func_t dvp_sensor_##f = {                                             \
        .detect = __bk_sensor_##f,                                                          \
    } ;                                                                                           \
    const dvp_sensor_config_t * __bk_sensor_##f(void)                                             \
    { return f(); }

extern dvp_sensor_detect_func_t __camera_sensor_detect_array_start;
extern dvp_sensor_detect_func_t __camera_sensor_detect_array_end;


/**
 * @brief Flag for QVGA subsample usage
 */
#define GC_QVGA_USE_SUBSAMPLE          1

/**
 * @brief DVP camera configuration for 864x480 resolution at 30 FPS with MJPEG format
 * @details Predefined configuration macro for common DVP camera settings
 */
#define BK_DVP_864X480_30FPS_MJPEG_CONFIG()	\
{	\
	.i2c_config = {	\
		.id = 2,	\
		.scl_pin = 0,	\
		.sda_pin = 1,	\
		.baud_rate = 100000,	\
		.flags = 0,	\
	},	\
	.reset_pin = 0xFF,	\
	.pwdn_pin = 0xFF,	\
	.io_config = {	\
		.data_width = SENSOR_BITS_WIDTH_8BIT,	\
		.data_pin = {32, 33, 34, 35, 36, 37, 38, 39},	\
		.vsync_pin = 31,	\
		.hsync_pin = 30,	\
		.xclk_pin = 27,	\
		.pclk_pin = 29,	\
	},	\
	.clk_source = MCLK_24M,	\
	.width = 864,	\
	.height = 480,	\
	.fps = 30,	\
	.img_format = IMAGE_MJPEG,	\
	.user_data = NULL,	\
}

/*
 * @}
 */

#ifdef __cplusplus
}
#endif
