set(incs)
set(srcs)

if (CONFIG_GSENSOR_ENABLE)
	list(APPEND incs
		.
	)

	list(APPEND srcs
		bk_gsensor.c
	)
endif()

if (CONFIG_GSENSOR_SC7A20_ENABLE)
	list(APPEND srcs
		gsensor_sc7a20.c
	)
endif()

if (CONFIG_GSENSOR_DEMO_EN)
	list(APPEND srcs
		gsensor_demo.c
	)
endif()

if (CONFIG_GSENSOR_ARITHEMTIC_DEMO_EN)
	list(APPEND srcs
		arithmetic_module.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

