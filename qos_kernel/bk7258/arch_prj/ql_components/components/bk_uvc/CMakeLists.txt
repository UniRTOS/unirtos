set(incs include)
set(srcs "")
set(priv_req bk_common)

if (CONFIG_USB_CAMERA)
    list(APPEND srcs
        src/bk_uvc.c
        src/uvc_urb_list.c
        src/uvc_stream_list.c
    )
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" media_utils)
