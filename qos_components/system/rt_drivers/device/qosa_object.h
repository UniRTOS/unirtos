/*****************************************************************/ /**
* @file qosa_object.h
* @brief
* @author larson.li@quectel.com
* @date 2023-04-20
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-04-20 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#include "qosa_sys.h"
#include "qosa_queue_list.h"

/**
 * Base structure of Kernel object
 */
struct rt_object
{
    qosa_q_link_type_t list;                              /**< list node of kernel object */
    char               name[CONFIG_QOSA_DEVICE_NAME_MAX]; /**< name of kernel object */
    int                driver_id;
    qosa_uint8_t       type;                              /**< type of kernel object */
    qosa_uint8_t       flag;                              /**< flag of kernel object */
};
typedef struct rt_object* rt_object_t;                    /**< Type for kernel objects. */

/**
 *  The object type can be one of the follows with specific
 *  macros enabled:
 *  - Device
 *  - Static
 */
enum rt_object_class_type
{
    RT_Object_Class_Null = 0x00,   /**< The object is not used. */
    RT_Object_Class_Device = 0x01, /**< The object is a device. */
    RT_Object_Class_Static = 0x80  /**< The object is a static object. */
};

/**
 * The information of the kernel object
 */
struct rt_object_information
{
    qosa_q_type_t             list;        /**< object list */
    enum rt_object_class_type type;        /**< object class type */
    qosa_size_t               object_size; /**< object size */
};

/**
 * This function will find specified name object from object
 * container.
 *
 * @param name the specified name of object.
 * @param type the type of object
 *
 * @return the found object or QOSA_NULL if there is no this object
 * in object container.
 *
 * @note this function shall not be invoked in interrupt status.
 */
rt_object_t rt_object_find(const char* name, qosa_uint8_t type);

/**
 * This function will return the type of object without
 * RT_Object_Class_Static flag.
 *
 * @param object the specified object to be get type.
 *
 * @return the type of object.
 */
qosa_uint8_t rt_object_get_type(rt_object_t object);

/**
 * This function will judge the object is system object or not.
 * Normally, the system object is a static object and the type
 * of object set to RT_Object_Class_Static.
 *
 * @param object the specified object to be judged.
 *
 * @return RT_TRUE if a system object, RT_FALSE for others.
 */
qosa_bool_t rt_object_is_systemobject(rt_object_t object);

/**
 * This function will detach a static object from object system,
 * and the memory of static object is not freed.
 *
 * @param object the specified object to be detached.
 */
void rt_object_detach(rt_object_t object);

/**
 * This function will initialize an object and add it to object system
 * management.
 *
 * @param object the specified object to be initialized.
 * @param type the object type.
 * @param name the object name. In system, the object's name must be unique.
 */
void rt_object_init(struct rt_object* object, enum rt_object_class_type type, const char* name);

/**
 * This function will return the length of object list in object container.
 *
 * @param type the type of object, which can be
 *             RT_Object_Class_Thread/Semaphore/Mutex... etc
 * @return the length of object list
 */
int rt_object_get_length(enum rt_object_class_type type);
/**
 * This function will return the specified type of object information.
 *
 * @param type the type of object, which can be
 *             RT_Object_Class_Thread/Semaphore/Mutex... etc
 *
 * @return the object type information or QOSA_NULL
 */
struct rt_object_information* rt_object_get_information(enum rt_object_class_type type);

rt_object_t rt_object_type_driver_id(int driver_id, qosa_uint8_t type);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#endif /* __OBJECT_H__ */
