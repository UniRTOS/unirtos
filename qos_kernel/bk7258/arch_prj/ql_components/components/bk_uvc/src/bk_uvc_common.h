#ifndef _BK_UVC_COMMON_H_
#define _BK_UVC_COMMON_H_

/**
 * @file bk_uvc_common.h
 * @brief Common definitions and data structures for UVC (USB Video Class) driver
 * 
 * This header file contains shared macros, type definitions, and data structures
 * used by the UVC camera driver implementation.
 */

#include <common/bk_err.h>
//#include <common/bk_include.h>
#include <bk_list.h>

//#include <os/os.h>
//#include <os/mem.h>
//#include <stdio.h>

//#include <components/usbh_hub_multiple_classes_api.h>
#include <components/uvc_camera_types.h>
//#include <driver/dma.h>

#include "FreeRTOS.h"
#include "event_groups.h"

/**
 * @brief Create a bitmask for a specific bit position
 * @param bit The bit position to set in the mask
 * @return A bitmask with the specified bit set to 1
 */
#define INDEX_MASK(bit)   (1U << bit)

/**
 * @brief Create a bitmask with a specific bit cleared
 * @param bit The bit position to clear in the mask
 * @return A bitmask with the specified bit set to 0
 */
#define INDEX_UNMASK(bit) (~(1U << bit))

/**
 * @brief Event bits for controlling UVC stream tasks
 * These bits are used in the event group to control the state of UVC stream and process tasks
 */
#define UVC_STREAM_TASK_ENABLE_BIT   INDEX_MASK(0)    /**< Enable UVC stream task bit */
#define UVC_STREAM_TASK_DISABLE_BIT  INDEX_MASK(1)    /**< Disable UVC stream task bit */
#define UVC_PROCESS_TASK_ENABLE_BIT  INDEX_MASK(2)    /**< Enable UVC processing task bit */
#define UVC_PROCESS_TASK_DISABLE_BIT INDEX_MASK(3)    /**< Disable UVC processing task bit */
#define UVC_STREAM_START_BIT         INDEX_MASK(4)    /**< Start UVC stream bit */
#define UVC_PROCESS_TASK_START_BIT   INDEX_MASK(5)    /**< Start UVC processing task bit */

#define UVC_CONNECT_BIT              INDEX_MASK(6)    /**< UVC device connection bit */
#define UVC_CLOSE_BIT                INDEX_MASK(7)    /**< UVC device disconnection bit */

/**
 * @brief Event bits for controlling H26x encoded stream tasks
 * These bits are used for managing H.264/H.265 encoded video streams
 */
#define UVC_STREAM_H26X_TASK_ENABLE_BIT   INDEX_MASK(8)     /**< Enable H26x stream task bit */
#define UVC_STREAM_H26X_TASK_DISABLE_BIT  INDEX_MASK(9)     /**< Disable H26x stream task bit */
#define UVC_H26X_PROCESS_TASK_ENABLE_BIT  INDEX_MASK(10)    /**< Enable H26x processing task bit */
#define UVC_H26X_PROCESS_TASK_DISABLE_BIT INDEX_MASK(11)    /**< Disable H26x processing task bit */
#define UVC_H26X_CONNECT_BIT              INDEX_MASK(12)    /**< H26x device connection bit */
#define UVC_H26X_CLOSE_BIT                INDEX_MASK(13)    /**< H26x device disconnection bit */

/**
 * @brief UVC timing and status definitions
 */
#define UVC_TIME_INTERVAL       (4)      /**< Time interval in milliseconds for UVC operations */
#define UVC_FRAME_OK            (0)      /**< Frame processing success status code */
#define UVC_FRAME_ERR           (-1)     /**< Frame processing error status code */

/**
 * @brief UVC packet size definition
 */
#define UVC_MAX_PACKET_SIZE (1024)  /**< Maximum size of UVC data packets in bytes */

//#define UVC_DEBUG_TIME  /**< Uncomment to enable debug timing GPIO signals */

/**
 * @brief Debug GPIO macros for timing analysis
 * These macros are used to toggle GPIO pins at specific points in the code
 * to allow timing analysis with an oscilloscope or logic analyzer.
 */
#ifdef UVC_DEBUG_TIME
#define UVC_POWER_ON_START()            GPIO_UP(32);
#define UVC_POWER_ON_END()              GPIO_DOWN(32);

#define UVC_INIT_START()                GPIO_UP(33);
#define UVC_INIT_END()                  GPIO_DOWN(33);

#define UVC_PACKET_PUSH_START()         GPIO_UP(32);
#define UVC_PACKET_PUSH_END()           GPIO_DOWN(32);

#define UVC_EOF_START()                 GPIO_UP(34);
#define UVC_EOF_END()                   GPIO_DOWN(34);

#define UVC_PACKET_START()              GPIO_UP(35);
#define UVC_PACKET_END()                GPIO_DOWN(35);

#define UVC_PACKET_HEAD_START()         GPIO_UP(36);
#define UVC_PACKET_HEAD_END()           GPIO_DOWN(36);

#define UVC_PACKET_COPY_START()         GPIO_UP(37);
#define UVC_PACKET_COPY_END()           GPIO_DOWN(37);

#define UVC_EOF_BIT_START()             GPIO_UP(38);
#define UVC_EOF_BIT_END()               GPIO_DOWN(38);
#else
#define UVC_POWER_ON_START()
#define UVC_POWER_ON_END()

#define UVC_INIT_START()
#define UVC_INIT_END()

#define UVC_PACKET_PUSH_START()
#define UVC_PACKET_PUSH_END()

#define UVC_EOF_START()
#define UVC_EOF_END()

#define UVC_PACKET_START()
#define UVC_PACKET_END()

#define UVC_PACKET_HEAD_START()
#define UVC_PACKET_HEAD_END()

#define UVC_PACKET_COPY_START()
#define UVC_PACKET_COPY_END()

#define UVC_EOF_BIT_START()
#define UVC_EOF_BIT_END()
#endif

/**
 * @brief UVC Connection States
 * 
 * Represents the different states of a UVC device connection lifecycle.
 */
typedef enum
{
    UVC_CLOSED_STATE,          /**< UVC device is closed and inactive */
    UVC_CLOSING_STATE,         /**< UVC device is in the process of closing */
    UVC_CONNECT_STATE,         /**< UVC device is connected but not yet configured */
    UVC_CONFIGING_STATE,       /**< UVC device is being configured */
    UVC_STREAMING_STATE,       /**< UVC device is actively streaming data */
    UVC_DISCONNECT_STATE,      /**< UVC device is disconnected */
    UVC_STATE_UNKNOW,          /**< UVC device state is unknown or undefined */
} uvc_state_t;

/**
 * @brief UVC event types enumeration
 * This enumeration defines all possible events that can occur in the UVC system
 */
typedef enum
{
    UVC_CONNECT_IND = 0,         /**< UVC device connection indication */
    UVC_DISCONNECT_IND,          /**< UVC device disconnection indication */
    UVC_START_IND,               /**< UVC streaming start indication */
    UVC_STOP_IND,                /**< UVC streaming stop indication */
    UVC_SUSPEND_IND,             /**< UVC device suspend indication */
    UVC_RESUME_IND,              /**< UVC device resume indication */
    UVC_SET_PARAM_IND,           /**< UVC parameter setting indication */
    UVC_DATA_REQUEST_IND,        /**< UVC data request indication */
    UVC_DATA_CLEAR_IND,          /**< UVC data clear indication */
    UVC_EXIT_IND,                /**< UVC exit indication */
    UVC_UNKNOW_IND,              /**< Unknown UVC event */
} uvc_event_t;

/**
 * @brief UVC stream event structure
 * This structure represents an event in the UVC stream processing system
 */
typedef struct
{
    uvc_event_t event;    /**< Type of UVC event */
    uint32_t  param;      /**< Additional parameter associated with the event */
} uvc_stream_event_t;

/**
 * @brief Camera parameter structure
 * This structure holds configuration and state information for a UVC camera device
 */
typedef struct
{
    uint8_t index;                     /**< Camera index/identifier */
    uint8_t camera_state;              /**< Current state of the camera */
    beken_semaphore_t sem;             /**< Semaphore for synchronization */
    bk_cam_uvc_config_t *info;         /**< Camera configuration information */
    struct usbh_urb *urb;              /**< USB Request Block for data transfer */
    frame_buffer_t *frame;             /**< Pointer to frame buffer for image data */
    bk_usb_hub_port_info *port_info;   /**< USB hub port information */
} camera_param_t;

/**
 * @brief UVC processing configuration structure
 * This structure contains configuration parameters for UVC stream processing
 */
typedef struct
{
    uvc_stream_state_t stream_state;           /**< Current state of the UVC stream */
    uint8_t transfer_bulk[UVC_PORT_MAX];       /**< Transfer mode per port: 1 for bulk, 0 for isochronous */
    uint8_t packet_error[UVC_PORT_MAX];        /**< Packet error flag per port */
    uint8_t head_bit0[UVC_PORT_MAX];           /**< Header bit flag per port */
    uint8_t dma[UVC_PORT_MAX];                 /**< DMA usage flag per port */
    uint16_t max_packet_size[UVC_PORT_MAX];    /**< Maximum packet size per port */
    uint32_t frame_id[UVC_PORT_MAX];           /**< Current frame ID per port */

#if (MEDIA_DEBUG_TIMER_ENABLE)
    beken_timer_t timer;                       /**< Timer for debugging */
    uint32_t later_id[UVC_PORT_MAX];           /**< Later frame ID for debugging */
    uint32_t curr_length[UVC_PORT_MAX];        /**< Current frame length for debugging */
    uint32_t all_packet_num;                   /**< Total packet count for debugging */
    uint32_t packet_err_num;                   /**< Error packet count for debugging */
#endif
} uvc_pro_config_t;

/**
 * @brief UVC node structure for linked list
 * This structure represents a node in the linked list of UVC camera devices
 */
typedef struct {
    LIST_HEADER_T list;         /**< Linked list header */
    camera_param_t *param;      /**< Pointer to camera parameters */
} uvc_node_t;

/**
 * @brief UVC stream handle structure
 * This structure represents a handle to a UVC stream processing instance
 */
typedef struct
{
    uint8_t pro_enable;                /**< Processing enable flag */
    uint8_t stream_num;                /**< Number of streams */
    beken_event_t handle;              /**< Event handle for synchronization */
    beken_thread_t stream_thread;      /**< Stream processing thread handle */
    beken_queue_t stream_queue;        /**< Queue for stream events */
    beken_thread_t pro_thread;         /**< Processing thread handle */
    beken_mutex_t mutex;               /**< Mutex for thread synchronization */
    uint8_t connect_camera_count;      /**< Number of connected cameras */
    uvc_pro_config_t *pro_config;      /**< UVC processing configuration information */
    void (*packet_cb)(struct usbh_urb *urb); /**< Packet processing callback function */
    const bk_uvc_callback_t *callback; /**< UVC callback function set */
    LIST_HEADER_T list;                /**< Linked list of UVC devices */
    void *user_data;                   /**< User data pointer for application-specific data */
} uvc_stream_handle_t;

#endif
