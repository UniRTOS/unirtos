#ifndef _QOSA_VIRT_AT_H_
#define _QOSA_VIRT_AT_H_

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    QOSA_VIRT_AT_SUCCESS           = 0,
    QOSA_VIRT_AT_INVALID_PARAM_ERR = -1,
    QOSA_VIRT_AT_OPEN_DEVICE_ERR   = -2,		//虚拟AT通道打开失败

}qosa_virt_at_err_e;

typedef enum
{
    QOSA_VIRT_AT_PORT_0 = 0,
	QOSA_VIRT_AT_PORT_1,

	QOSA_VIRT_AT_PORT_MAX,
}qosa_virt_at_port_e;

/*===========================================================================
 * Functions declaration
 ===========================================================================*/
/*****************************************************************
* Description: VIRT AT 回调函数，用于接收AT命令返回
*
* Parameters:
*   channel           [in]    虚拟通道号
*   cache_size        [in]    当前可读取缓存数据大小
*
*****************************************************************/
typedef void (*qosa_virt_at_callback)(qosa_virt_at_port_e channel, qosa_uint32_t cache_size);

/*****************************************************************
* Function: qosa_virt_at_open
*
* Description: 打开 VIRT AT通道,注册回调函数
*
* Parameters:
*   channel	    [in] 	虚拟AT通道
*   virt_at_cb	[in] 	回调函数
*
* Return:
* 	0			打开成功。
*	other 	    错误码。
*
*****************************************************************/
qosa_virt_at_err_e qosa_virt_at_open(qosa_virt_at_port_e channel, qosa_virt_at_callback virt_at_cb);

/*****************************************************************
* Function: qosa_virt_at_close
*
* Description: 关闭 VIRT AT通道
*
* Parameters:
*   channel	    [in] 	虚拟AT通道
*
* Return:
* 	0			关闭成功。
*	other 	    错误码。
*
*****************************************************************/
qosa_virt_at_err_e qosa_virt_at_close(qosa_virt_at_port_e channel);

/*****************************************************************
* Function: qosa_virt_at_read
*
* Description: VIRT AT读数据
*
* Parameters:
*   channel	    [in] 	虚拟AT通道
* 	data	    [out] 	读取的数据
*   data_len    [in]    要读取的数据长度
*
* Return:
*   < 0         错误码,详见ql_errcode_virt_at_e
*	other 	    实际读取到的数据长度
*
*****************************************************************/
int qosa_virt_at_read(qosa_virt_at_port_e channel, qosa_uint8_t *data, qosa_uint32_t data_len);

/*****************************************************************
* Function: qosa_virt_at_write
*
* Description: VIRT AT写数据
*
* Parameters:
*   channel	    [in] 	虚拟AT通道
* 	data	    [in] 	要写入的数据
*   data_len    [in]    要写入的数据长度
*
* Return:
*   < 0         错误码,详见ql_errcode_virt_at_e
*	other 	    实际写入的数据长度
*
*****************************************************************/
int qosa_virt_at_write(qosa_virt_at_port_e channel, qosa_uint8_t *data, qosa_uint32_t data_len);


/*****************************************************************
* Function: qosa_virt_at_read_aviable
*
* Description: 获取虚拟AT通道中的可读字节数
*
* Parameters:
*   channel	    [in] 	虚拟AT通道
*
* Return:
* 	>=0			可读字节数
*	other 	    错误码。
*
*****************************************************************/

int qosa_virt_at_read_aviable(qosa_virt_at_port_e channel);
/*****************************************************************
* Function: qosa_virt_at_urc_port_set
*
* Description: 设置虚拟AT通道urc输出通道口
*
* Parameters:
*   channel	    [in] 	虚拟AT通道
*
* Return:
* 	0			    设置成功
*	other 	    错误码。
*
*****************************************************************/
qosa_virt_at_err_e qosa_virt_at_urc_port_set(qosa_virt_at_port_e channel);

/*****************************************************************
* Function: qosa_virt_at_urc_port_get
*
* Description: 获取虚拟AT通道urc输出通道口
*
* Parameters:
*   channel	    [out] 	读取的urc输出通道口
*
* Return:
*   0			    获取成功
*	other 	    错误码。
*
*****************************************************************/
qosa_virt_at_err_e qosa_virt_at_urc_port_get(qosa_virt_at_port_e* channel);

/*****************************************************************
* Function: qosa_virt_at_chan_to_dev
*
* Description: 获取虚拟AT通道对应的AT通道口
*
* Parameters:
*   channel	    [int] 	读取的urc输出通道口
*
* Return:
*   qosa_at_dev_type_e	at device
*
*	other 	    错误码。
*
*****************************************************************/
qosa_at_dev_type_e qosa_virt_at_chan_to_dev(qosa_virt_at_port_e channel);

#ifdef __cplusplus
}
#endif

#endif
