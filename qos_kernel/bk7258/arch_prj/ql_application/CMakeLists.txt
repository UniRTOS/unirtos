set(incs
    .
    include
)

set(srcs
    ap_main.c
)

set(priv_req
    bk_common
    bk_event
    bk_wifi
    lwip_intf_v2_1
    bk_cli
    bk_vfs
    json
    easy_flash
    psa_mbedtls
    media_service
    multimedia
    UniRtos
)

armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES "${priv_req}"
)
