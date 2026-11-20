set(incs)
set(srcs)

if (CONFIG_AGORA_IOT_SDK)

list(APPEND incs
            include/bk7258
            hal/aosl/include
            hal/aosl/platform/include
            hal/aosl/platform/src/bk7258/config
)

list(APPEND srcs
            hal/agora_thread.c
            hal/aosl/kernel/file.c
            hal/aosl/kernel/fileobj.c
            hal/aosl/kernel/iofd.c
            hal/aosl/kernel/log.c
            hal/aosl/kernel/mpq.c
            hal/aosl/kernel/mpqp.c
            hal/aosl/kernel/osmp.c
            hal/aosl/kernel/panic.c
            hal/aosl/kernel/refobj.c
            hal/aosl/kernel/task.c
            hal/aosl/kernel/thread.c
            hal/aosl/kernel/time.c
            hal/aosl/kernel/timer.c
            hal/aosl/kernel/version.c
            hal/aosl/kernel/select_mp.c
            hal/aosl/kernel/poll_mp.c
            hal/aosl/kernel/et_mp.c
            hal/aosl/kernel/rt_monitor.c
            hal/aosl/lib/atomic.c
            hal/aosl/lib/bitmap.c
            hal/aosl/lib/byteswap.c
            hal/aosl/lib/cond.c
            hal/aosl/lib/errno.c
            hal/aosl/lib/marshalling-base-obj.c
            hal/aosl/lib/marshalling.c
            hal/aosl/lib/psbuff.c
            hal/aosl/lib/rbtree.c
            hal/aosl/lib/rwlock.c
            hal/aosl/lib/rwlock-xadd.c
            hal/aosl/lib/thread.c
            hal/aosl/lib/tls.c
            hal/aosl/lib/utils.c
            hal/aosl/mm/mm.c
            hal/aosl/net/dns.c
            hal/aosl/net/if.c
            hal/aosl/net/inet_ntop.c
            hal/aosl/net/inet_pton.c
            hal/aosl/net/ipv6.c
            hal/aosl/net/route.c
            hal/aosl/net/sk_utils.c
            hal/aosl/platform/src/bk7258/aosl_hal_atomic.c
            hal/aosl/platform/src/bk7258/aosl_hal_errno.c
            hal/aosl/platform/src/bk7258/aosl_hal_file.c
            hal/aosl/platform/src/bk7258/aosl_hal_iomp.c
            hal/aosl/platform/src/bk7258/aosl_hal_log.c
            hal/aosl/platform/src/bk7258/aosl_hal_memory.c
            hal/aosl/platform/src/bk7258/aosl_hal_socket.c
            hal/aosl/platform/src/bk7258/aosl_hal_thread.c
            hal/aosl/platform/src/bk7258/aosl_hal_time.c
            hal/aosl/platform/src/bk7258/aosl_hal_utils.c
)

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common lwip_intf_v2_1 bk_vfs bk_rtos)

if (CONFIG_AGORA_IOT_SDK)
	file(GLOB agora_libs ${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libs/*.a)
	foreach(lib ${agora_libs})
		get_filename_component(lib_name ${lib} NAME)
		message(STATUS "agora iot sdk:${lib_name}")
		add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libs/${lib_name}")
		target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
	endforeach()
endif()
