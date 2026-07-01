cmake_minimum_required(VERSION 3.0.0)

# Decouple platform selection.
if(NOT DEFINED QURL_PLATFORM_CMAKE_FILE)
    set(QURL_PLATFORM_CMAKE_FILE "${CMAKE_CURRENT_LIST_DIR}/platform/lzm_linux/platform.cmake")
endif()

message("QURL_PLATFORM_CMAKE_FILE:${QURL_PLATFORM_CMAKE_FILE}")
include(${QURL_PLATFORM_CMAKE_FILE})

if(QURL_COMPILE_CFG_EXE)
    # Fixed SDK output directories.
    # Executable output path.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    set(CMAKE_FW_OUTPUT_DIRECTORY      "${CMAKE_BINARY_DIR}/out")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/libs")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/libs")

    # Generate configuration header input.
    message("QUEL_TARGET_NAME:${QURL_TARGET_NAME}")
    set(target ${QURL_TARGET_NAME})

    project(${target} VERSION 0.0.1 LANGUAGES C CXX ASM)

    # Sources.
    set(src_qurl
        ${QURL_USER_SRC}
    )

    add_executable(${target}.elf ${src_qurl})
    add_custom_command(TARGET ${target}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${target}.elf ${target}.bin
        COMMAND ${CMAKE_OBJCOPY} ${target}.elf ${CMAKE_FW_OUTPUT_DIRECTORY}/${target}.elf
        # COMMAND ${CMAKE_SIZE} ${target}.elf
        VERBATIM)

    if(QURL_COMPILE_CFG_SDK)
        add_custom_command(TARGET ${target}.elf POST_BUILD
            COMMAND ${QURL_COMPILE_CFG_SDK_CMD}
            VERBATIM)
        message(QURL_COMPILE_CFG_SDK_CMD:${QURL_COMPILE_CFG_SDK_CMD})
    endif()

    # Auto-resolve link libs from the shared SDK menuconfig mapping.
    set(QURL_AUTO_LINK_LIBS)
    if(COMMAND unirtos_get_auto_link_component_targets)
        unirtos_get_auto_link_component_targets(QURL_AUTO_LINK_LIBS)
    elseif(COMMAND unirtos_collect_auto_link_component_targets)
        unirtos_collect_auto_link_component_targets(QURL_AUTO_LINK_LIBS)
    endif()

    if(QURL_AUTO_LINK_LIBS)
        message(STATUS "QURL auto link libs from menuconfig: ${QURL_AUTO_LINK_LIBS}")
    endif()

    # Keep QURL_USER_LIBS as optional compatibility fallback.
    target_link_libraries(${target}.elf PRIVATE qurl ${QURL_AUTO_LINK_LIBS} ${QURL_USER_LIBS})
    if(QURL_COMPILE_CFG_APPS)
        target_link_libraries(${target}.elf PRIVATE qurl_apps)
    endif()
    target_include_directories(${target}.elf PUBLIC ${QURL_USER_INC} include core/include)
    target_include_directories(${target}.elf PUBLIC include)
endif() # QURL_COMPILE_CFG_EXE

add_subdirectory(core)

if(QURL_COMPILE_CFG_EXE)
    if(QURL_COMPILE_CFG_APPS)
        add_subdirectory(apps)
    endif()
endif() # QURL_COMPILE_CFG_EXE
