set(incs .
	common/transfer_list
	common/bk_ntwk_pack
	common/bk_video_drop_policy
	common/bk_ntwk_sdp
	bk_network_service/bk_ntwk_socket
	bk_network_service/bk_net_tcp_service
	bk_network_service/bk_net_udp_service
	bk_network_service/bk_net_cs2_service
)

set(srcs)

if (CONFIG_BK_NTWK_TRANSFER)
list(APPEND srcs
	common/transfer_list/trans_list.c
	common/bk_ntwk_pack/ntwk_pack.c
	common/bk_ntwk_pack/ntwk_fragmentation.c
	common/bk_ntwk_sdp/ntwk_sdp.c
	common/bk_video_drop_policy/video_drop.c
	bk_network_service/bk_ntwk_socket/ntwk_socket.c
	bk_network_service/bk_ntwk_tcp_service/ntwk_tcp_service.c
	bk_network_service/bk_ntwk_tcp_service/bk_tcp_service.c
	bk_network_service/bk_ntwk_udp_service/ntwk_udp_service.c
	bk_network_service/bk_ntwk_udp_service/bk_udp_service.c
	network_transfer_internal.c
	network_transfer.c
	)

if (CONFIG_CS2_P2P_SERVER OR CONFIG_CS2_P2P_CLIENT)
list(APPEND srcs
	bk_network_service/bk_ntwk_cs2_service/ntwk_cs2_service.c
	bk_network_service/bk_ntwk_cs2_service/bk_cs2_service.c
)
endif()
endif()

set(req bk_common multimedia media_utils lwip_intf_v2_1 avdk_utils bk_wifi_driver cs2p2p)

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES "${req}"
	)
 