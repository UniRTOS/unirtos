
KERNEL_COMMON    :=$(TOP)/ql_common/

CFLAGS_INC      +=\
-I $(KERNEL_COMMON)prebuild/PLAT/inc\
-I $(KERNEL_COMMON)device/target/board/common/ARMCM3/inc\
-I $(KERNEL_COMMON)middleware/developed/fota/custom/inc\
-I $(KERNEL_COMMON)os/freertos/CMSIS/ap/inc\
-I $(KERNEL_COMMON)prebuild/PS/inc\
-I $(KERNEL_COMMON)middleware/developed/nvram/ec7xx/inc\
-I $(KERNEL_COMMON)middleware/thirdparty/mbedtls/configs\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include/lwip\
-I $(KERNEL_COMMON)unirtos_adapter/dynamic_create\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include/lwip/priv\
-I $(KERNEL_COMMON)middleware/quectel/cust/inc\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc\
-I $(KERNEL_COMMON)middleware/thirdparty/mbedtls/include/psa\
-I $(KERNEL_COMMON)middleware/developed/debug/inc\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include/lwip/prot\
-I $(KERNEL_COMMON)middleware/thirdparty/mbedtls/library\
-I $(KERNEL_COMMON)os/freertos/portable/gcc\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include/arch\
-I $(KERNEL_COMMON)driver/hal/common/inc\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc_cmsis\
-I $(KERNEL_COMMON)unirtos_adapter/system_source/plat_source/include\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc/ec718\
-I $(KERNEL_COMMON)middleware/developed/fota/pub\
-I $(KERNEL_COMMON)os/freertos/inc\
-I $(KERNEL_COMMON)os/freertos/CMSIS/common/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/common/inc\
-I $(KERNEL_COMMON)middleware/thirdparty/mbedtls/include/mbedtls\
-I $(KERNEL_COMMON)unirtos_adapter/system_source/plat_source/vfs\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/ap/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/common/pkginc\
-I $(KERNEL_COMMON)middleware/quectel/open_service/ql_service/inc\
-I $(KERNEL_COMMON)middleware/developed/cms/sockmgr/inc\
-I $(KERNEL_COMMON)middleware/developed/cms/cms/inc\
-I $(KERNEL_COMMON)prebuild/PLAT/inc\
-I $(KERNEL_COMMON)device/target/board/common/ARMCM3/inc\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include/netif\
-I $(KERNEL_COMMON)middleware/developed/at/atentity/inc\
-I $(KERNEL_COMMON)os/freertos/CMSIS/ap/inc\
-I $(KERNEL_COMMON)driver/board/ec7xx_0h00/inc/camera/cameraDev\
-I $(KERNEL_COMMON)prebuild/PS/inc\
-I $(KERNEL_COMMON)driver/board/ec7xx_0h00/inc/lcd\
-I $(KERNEL_COMMON)middleware/developed/nvram/ec7xx/inc\
-I $(KERNEL_COMMON)middleware/developed/at/atdecoder/inc\
-I $(KERNEL_COMMON)middleware/developed/common/inc\
-I $(KERNEL_COMMON)project/ec7xx_0h00/ap/apps/at_command/inc\
-I $(KERNEL_COMMON)driver/board/ec7xx_0h00/inc\
-I $(KERNEL_COMMON)unirtos_adapter/dynamic_create\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include/lwip\
-I $(KERNEL_COMMON)middleware/developed/ccio/device/inc\
-I $(KERNEL_COMMON)ver_tag\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include/lwip/priv\
-I $(KERNEL_COMMON)middleware/thirdparty/lzma2201/C\
-I $(KERNEL_COMMON)middleware/thirdparty/lzma2201/C/wrapper\
-I $(KERNEL_COMMON)middleware/quectel/cust/inc\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc\
-I $(KERNEL_COMMON)middleware/quectel/ql_app/include\
-I $(KERNEL_COMMON)middleware/developed/debug/inc\
-I $(KERNEL_COMMON)middleware/quectel/open_service/ql_service/bridge/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/ap/gcc\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include/lwip/prot\
-I $(KERNEL_COMMON)middleware/developed/cms/psdial/inc\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc_cmsis\
-I $(KERNEL_COMMON)os/freertos/portable/gcc\
-I $(KERNEL_COMMON)middleware/thirdparty/lwip/src/include/arch\
-I $(KERNEL_COMMON)driver/hal/common/inc\
-I $(KERNEL_COMMON)middleware/quectel/ql_api/include\
-I $(KERNEL_COMMON)unirtos_adapter/system_source/plat_source/include\
-I $(KERNEL_COMMON)driver/board/ec7xx_0h00/inc/codec\
-I $(KERNEL_COMMON)middleware/developed/ccio/custom/inc\
-I $(KERNEL_COMMON)driver/hal/ec7xx/ap/inc/ec718\
-I $(KERNEL_COMMON)driver/board/ec7xx_0h00/inc/camera\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc/ec718\
-I $(KERNEL_COMMON)middleware/developed/at/atreply/inc\
-I $(KERNEL_COMMON)os/freertos/inc\
-I $(KERNEL_COMMON)os/freertos/CMSIS/common/inc\
-I $(KERNEL_COMMON)prebuild/PLAT/inc/usb\
-I $(KERNEL_COMMON)driver/hal/ec7xx/ap/inc\
-I $(KERNEL_COMMON)middleware/developed/fota/pub\
-I $(KERNEL_COMMON)middleware/developed/ccio/pub\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/common/inc\
-I $(KERNEL_COMMON)middleware/developed/ecapi/aal/inc\
-I $(KERNEL_COMMON)middleware/developed/at/atps/inc\
-I $(KERNEL_COMMON)unirtos_adapter/system_source/plat_source/vfs\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/ap/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/common/pkginc\
-I $(KERNEL_COMMON)middleware/quectel/open_service/ql_service/inc\
-I $(KERNEL_COMMON)middleware/developed/cms/sockmgr/inc\
-I $(KERNEL_COMMON)middleware/developed/debug/inc\
-I $(KERNEL_COMMON)middleware/developed/fota/custom/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/common/inc\
-I $(KERNEL_COMMON)middleware/developed/nvram/ec7xx/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/common/pkginc\
-I $(KERNEL_COMMON)middleware/developed/fota/pub\
-I $(KERNEL_COMMON)prebuild/PLAT/inc\
-I $(KERNEL_COMMON)device/target/board/common/ARMCM3/inc\
-I $(KERNEL_COMMON)os/freertos/CMSIS/ap/inc\
-I $(KERNEL_COMMON)middleware/developed/nvram/ec7xx/inc\
-I $(KERNEL_COMMON)driver/board/ec7xx_0h00/inc\
-I $(KERNEL_COMMON)middleware/thirdparty/ecc/inc\
-I $(KERNEL_COMMON)middleware/thirdparty/lzma2201/C/wrapper\
-I $(KERNEL_COMMON)middleware/quectel/cust/inc\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/ap/gcc\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc_cmsis\
-I $(KERNEL_COMMON)middleware/quectel/ql_api/include\
-I $(KERNEL_COMMON)driver/hal/common/inc\
-I $(KERNEL_COMMON)project/ec7xx_0h00/ap/apps/bootloader/code/include\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc/ec718\
-I $(KERNEL_COMMON)project/ec7xx_0h00/ap/apps/bootloader/code/include/common\
-I $(KERNEL_COMMON)middleware/developed/fota/pub\
-I $(KERNEL_COMMON)driver/hal/ec7xx/ap/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/common/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/ap/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/common/pkginc\
-I $(KERNEL_COMMON)prebuild/PLAT/inc\
-I $(KERNEL_COMMON)device/target/board/common/ARMCM3/inc\
-I $(KERNEL_COMMON)middleware/developed/fota/custom/inc\
-I $(KERNEL_COMMON)os/freertos/CMSIS/ap/inc\
-I $(KERNEL_COMMON)middleware/developed/nvram/ec7xx/inc\
-I $(KERNEL_COMMON)driver/board/ec7xx_0h00/inc\
-I $(KERNEL_COMMON)project/ec7xx_0h00/ap/apps/quec_updater/code/lcd_driver\
-I $(KERNEL_COMMON)middleware/thirdparty/lzma2201/C/wrapper\
-I $(KERNEL_COMMON)middleware/quectel/cust/inc\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/ap/gcc\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc_cmsis\
-I $(KERNEL_COMMON)middleware/quectel/ql_api/include\
-I $(KERNEL_COMMON)driver/hal/common/inc\
-I $(KERNEL_COMMON)driver/chip/ec7xx/ap/inc/ec718\
-I $(KERNEL_COMMON)middleware/developed/fota/pub\
-I $(KERNEL_COMMON)driver/hal/ec7xx/ap/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/common/inc\
-I $(KERNEL_COMMON)project/ec7xx_0h00/ap/apps/quec_updater/code/include\
-I $(KERNEL_COMMON)project/ec7xx_0h00/ap/apps/quec_updater/code/include/common\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/ap/inc\
-I $(KERNEL_COMMON)device/target/board/ec7xx_0h00/common/pkginc\
-I $(KERNEL_COMMON)/middleware/thirdparty/mbedtls/include\
-I $(KERNEL_COMMON)/middleware/thirdparty/mbedtls/configs\
-I $(KERNEL_COMMON)/middleware/thirdparty/lwip/src/include\

CFLAGS += -DUNIRTOS_OPEN_MODE_SUPPORT -DHAVE_STRUCT_TIMESPEC -DSOFTPACK_VERSION="" -DUNIRTOS_LWIP_MODIFY -DTYPE_EC718M -DQL_OEM_VERSION_ -DUNIRTOS_MODEM_MODIFY -DTRACE_LEVEL=5 -DSDK_REL_BUILD -DQL_AP_LIB_OC -DQL_SSL_SUPPORT   -DQL_CP_OC_BIN_MODE -DUNIRTOS_PLATFORM_MODIFY  -DCHIP_EC718 -DQL_IS_APPLICATION -DTYPE_EC718HM -D__EC718 -DUNIRTOS_COMMON_MODIFY -DCORE_IS_AP

KERNEL_SRC_DIRS +=\
$(KERNEL_COMMON)/middleware/quectel/open_service/ql_service/bridge/src/qos_app_api_map.c\

KERNEL_COBJSTEMP := $(patsubst %.c, %.o, $(KERNEL_SRC_DIRS))
KERNEL_COBJSTEMP := $(subst $(KERNEL_COMMON),ql_common,$(KERNEL_COBJSTEMP))
KERNEL_COBJS := $(addprefix $(BUILDDIR)/, $(KERNEL_COBJSTEMP))
KERNEL_PPFILES := $(patsubst %.o, %.pp, $(KERNEL_COBJS))
CUSTLIBPPFILES += $(KERNEL_PPFILES)

ifneq ($(MAKECMDGOALS), unilog)
-include $(KERNEL_COBJS:.o=.d)
endif
-include $(KERNEL_COBJS:.o=.dd)
obj-y          += $(KERNEL_COBJSTEMP)	

