set(incs .)
set(srcs)

if (CONFIG_BAT_MONITOR)
	list(APPEND incs .
	)

	list(APPEND srcs
		bat_monitor.c
	)
endif() # CONFIG_BAT_MONITOR
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
