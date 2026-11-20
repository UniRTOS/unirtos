cmake_minimum_required(VERSION 3.6.3)

if(POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()

project(VolcEngineRTCHal LANGUAGES C)

# set(CMAKE_C_VISIBILITY_PRESET hidden)
# set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_definitions(-D_GNU_SOURCE)

set(CMAKE_C_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/configs -DMBEDTLS_USER_CONFIG_FILE='<config_mbedtls.h>' ${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden -std=c99")
set(CMAKE_CXX_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/configs -DMBEDTLS_USER_CONFIG_FILE='<config_mbedtls.h>' ${CMAKE_CXX_FLAGS}")

aux_source_directory(${PROJECT_SOURCE_DIR}/src/common VOLC_HAL_COMMON_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/platform/bk7258 VOLC_HAL_PLATFORM_FILES)

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/third_party/include)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/inc ${PROJECT_BINARY_DIR}/inc)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/VolcEngineRTCLite/lib)

add_library(VolcEngineRTCHal STATIC ${VOLC_HAL_COMMON_FILES} ${VOLC_HAL_PLATFORM_FILES})

install(TARGETS VolcEngineRTCHal
        LIBRARY DESTINATION VolcEngineRTCLite/lib
        ARCHIVE DESTINATION VolcEngineRTCLite/lib
)
