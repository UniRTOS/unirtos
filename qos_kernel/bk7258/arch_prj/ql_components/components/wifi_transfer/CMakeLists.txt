set(incs include)
set(srcs)

if (1)
	list(APPEND srcs
		src/wifi_transfer.c
		)
endif()

set(priv_req bk_common multimedia media_utils)

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}"
	)
 