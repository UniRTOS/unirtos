/*************************************************************
 *
 * This is a part of the key Framework Library.
 * Copyright (C) 2021 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#ifndef __KEY_ADAPTER_H__
#define __KEY_ADAPTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <driver/hal/hal_gpio_types.h>

#define KEY_GPIO_13   GPIO_13
#define KEY_GPIO_12   GPIO_12
#define KEY_GPIO_8    GPIO_8

#define LONG_RRESS_TIMR 3000  //long press wake up time

typedef enum {
    /* ========== BK Internal event definitions (Range: 0-99) ========== */
    EVENT_NONE = 0,
    VOLUME_UP,
    VOLUME_DOWN,
    SHUT_DOWN,
    POWER_ON,
    IR_MODE_SWITCH,	// image recognition mode switch
    CONFIG_NETWORK,
    BRIGHTNESS_ADD,
    AI_AGENT_CONFIG,
    FACTORY_RESET,
    AUDIO_BUF_APPEND,
    
    /* BK Internal event range maximum */
    INTERNAL_EVENT_MAX = 99,
    
    /* ========== User-defined event area (Range: 100-255) ========== */
    /* 
     * Customers can define custom events using macros in their own header files, for example:
     * 
     *   #define USER_LED_TOGGLE      (USER_EVENT_START + 0)   // = 100
     *   ...
     * 
     * Note: Event values cannot exceed 255
     */
    USER_EVENT_START = 100,    // User event start value
    USER_EVENT_MAX = 255       // User event maximum value (cannot exceed)
} key_event_t;

typedef enum{
    SHORT_PRESS = 0,
    DOUBLE_PRESS = 1,
    LONG_PRESS =2,
    LONG_PRESS_UP =3
} key_action_t;

typedef struct {
    uint8_t gpio_id;
    uint8_t active_level;
    key_event_t short_event;          // Short press event
    key_event_t double_event;         // Double press event
    key_event_t long_event;           // Long press event
    key_event_t long_press_up_event;  // Long press release event
} KeyConfig_t;

typedef struct
{
    uint8_t gpio_id;
    key_action_t action;
} KeyEventMsg_t;

#define KEY_EVENT_USER_MAX    USER_EVENT_MAX

/* Event validation macro */
#define IS_INVALID_EVENT(event)  ((event) > USER_EVENT_MAX)    // Check if event is invalid (>255)

/* Event handler function type */
typedef void (*key_handler_t)(uint8_t event);

/* API functions */
// Initialize key driver
void bk_key_driver_init(KeyConfig_t* configs, uint8_t num_keys);

// Deinitialize key driver
void bk_key_driver_deinit(KeyConfig_t* configs, uint8_t num_keys);

// Register global event handler (implemented by application layer)
void bk_key_register_event_handler(key_handler_t handler);


#ifdef __cplusplus
}
#endif
#endif 