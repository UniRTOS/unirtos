message(STATUS "cmake config ${CMAKE_CURRENT_SOURCE_DIR}")

set(target mongoose-7.10)
add_components_libraries($<TARGET_FILE:${target}>)
add_library(${target} STATIC)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_unir_lib_dir})
target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR})
target_include_directories(${target} PRIVATE ${SOURCE_SYS_DEBUG_DIR})
target_include_directories(${target} PRIVATE ${SOURCE_SYS_OS_DIR})
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)
target_include_directories(${target} PRIVATE ${SOURCE_TOP_DIR}/components/include)

target_include_directories(${target} PRIVATE ${arch_prj_dir}/ql_components/unirtos/generated)

target_add_sources(${target} PRIVATE
    mongoose.c
 )

target_link_libraries_if(CONFIG_QOSA_LINK_SYSTEM_LIBRARIES_FUNC THEN ${target} system_os)

