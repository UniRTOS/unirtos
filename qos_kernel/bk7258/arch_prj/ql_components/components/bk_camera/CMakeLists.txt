set(incs include)
set(srcs .)

list(APPEND srcs src/bk_camera.c)

if (CONFIG_DVP_CAMERA)
    list(APPEND srcs
        src/dvp/bk_camera_dvp_ctlr.c
    )
endif()

if (CONFIG_USB_CAMERA)
    list(APPEND srcs
        src/uvc/bk_camera_uvc_ctlr.c
    )
endif()

set(priv_req avdk_utils)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")