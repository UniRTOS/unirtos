set(incs inc)

if (CONFIG_BK_BLE_PROVISIONING)
list(APPEND incs
    inc/ble_scheme
)
endif()

set(srcs)
if (CONFIG_BK_NETWORK_PROVISIONING)
list (APPEND srcs
    src/bk_network_provisioning.c
)
endif()
if (CONFIG_BK_BLE_PROVISIONING)
list(APPEND srcs
    src/ble_scheme/ble_provisioning.c
    src/ble_scheme/ble_provisioning_utils.c
)
endif()
if (CONFIG_BK_NETWORK_PROVISIONING_BLE_EXAMPLE)
list(APPEND srcs
    example/ble_network_provisioning_demo.c
)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" 
    PRIV_REQUIRES psa_mbedtls json lwip_intf_v2_1 bk_pan)

