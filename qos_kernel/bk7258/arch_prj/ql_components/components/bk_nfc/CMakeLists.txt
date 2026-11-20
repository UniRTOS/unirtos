set(incs)
set(srcs)

if(CONFIG_NFC_ENABLE)

if (CONFIG_NFC_TEST_EN)
list(APPEND srcs
		mfrc522_test.c
	)
endif()

if (CONFIG_NFC_MFRC522_ENABLE)
list(APPEND srcs
		bk_mfrc522.c
		board_uart.c
		bk_mfrc522_main.c
		bk_mfrc522_adapt.c
	)
endif()
endif()
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

