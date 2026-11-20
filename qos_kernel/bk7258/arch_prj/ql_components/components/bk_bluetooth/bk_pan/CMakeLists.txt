set(incs)
set(srcs)

if (CONFIG_NET_PAN)
list(APPEND incs
    .
    storage
)

list(APPEND srcs
    bt_manager.c
    pan_service.c
    pan_demo_cli.c
    bt_comm_list.c
    hidd_service.c
    storage/bluetooth_storage.c
)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" 
    PRIV_REQUIRES lwip_intf_v2_1)

