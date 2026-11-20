set(TP_DEVICE_FILES "")
set(TP_PATH src/tp)

if (CONFIG_TP)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_sensor_devices.c)
endif()

if (CONFIG_TP_FT6336)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_ft6336.c)
	list(APPEND GLOBAL_FUNCTION_SYMBOLS "ft6336_detect_sensor")
endif()

if (CONFIG_TP_GT911)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_gt911.c)
	list(APPEND GLOBAL_FUNCTION_SYMBOLS "gt911_detect_sensor")
endif()

if (CONFIG_TP_GT1151)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_gt1151.c)
	list(APPEND GLOBAL_FUNCTION_SYMBOLS "gt1151_detect_sensor")
endif()

if (CONFIG_TP_HY4633)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_hy4633.c)
	list(APPEND GLOBAL_FUNCTION_SYMBOLS "hy4633_detect_sensor")
endif()

if (CONFIG_TP_CST816D)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_cst816d.c)
	list(APPEND GLOBAL_FUNCTION_SYMBOLS "cst816d_detect_sensor")
endif()
