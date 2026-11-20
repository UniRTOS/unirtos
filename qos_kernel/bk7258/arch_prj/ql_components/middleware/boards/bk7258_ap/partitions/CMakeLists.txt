armino_component_register(INCLUDE_DIRS ./)

if (CMAKE_BUILD_EARLY_EXPANSION)
return()
endif()

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/_build)
armino_build_set_property(PARTITIONS_HDR_DIR ${GEN_DIR})
set(GEN_FILE ${GEN_DIR}/security.h)
set(CONFIG_FILE_LISTS
	bl1_control.json
	../csv/ota.csv
	../csv/security.csv
	../csv/ppc.csv
	../csv/mpc.csv
	)

armino_build_get_property(python PYTHON)

if(UNIRTOS_APP_BUILD)
	armino_build_get_property(project_dir PROJECT_DIR)
	armino_build_get_property(project_name PROJECT_NAME)
	armino_build_get_property(armino_soc ARMINO_SOC)
	armino_build_get_property(armino_path ARMINO_PATH)
	string(REGEX REPLACE "_ap$" "" soc_name "${armino_soc}")
endif()

set(GEN_TOOL $ENV{ARMINO_TOOLS_PATH}/build_tools/build_process/bk_sdk/bk_prebuild.py)
if(UNIRTOS_APP_BUILD)
	if(DEFINED ENV{BEKEN_UTILS_DIR} AND NOT "$ENV{BEKEN_UTILS_DIR}" STREQUAL "")
		set(beken_utils_dir "$ENV{BEKEN_UTILS_DIR}")
	else()
		set(beken_utils_dir "$ENV{ARMINO_TOOLS_PATH}/env_tools/beken_utils")
	endif()
	if(CMAKE_HOST_WIN32)
		set(python_path "$ENV{ARMINO_TOOLS_PATH};$ENV{PYTHONPATH}")
	else()
		set(python_path "$ENV{ARMINO_TOOLS_PATH}:$ENV{PYTHONPATH}")
	endif()
endif()
if(UNIRTOS_APP_BUILD)
	add_custom_command(OUTPUT ${GEN_FILE}
		COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${python_path}"
			"PROJECT_DIR=${project_dir}" "PROJECT_NAME=${project_name}"
			"PROJECT_BUILD_DIR=${CMAKE_BINARY_DIR}" "ARMINO_SOC_NAME=${soc_name}"
			"ARMINO_BOARD_CONFIG_DIR=${armino_path}/middleware/boards/${armino_soc}"
			"ARMINO_PARTITIONS_OUTPUT_DIR=${GEN_DIR}"
			"BEKEN_UTILS_DIR=${beken_utils_dir}"
			${python} ${GEN_TOOL} ${armino_soc}
		DEPENDS ${CONFIG_FILE_LISTS} ${GEN_TOOL}
		VERBATIM
	)
else()
	add_custom_command(OUTPUT ${GEN_FILE}
		COMMAND ${python} ${GEN_TOOL} $ENV{ARMINO_SOC}
		DEPENDS ${CONFIG_FILE_LISTS} ${GEN_TOOL}
	)
endif()

add_custom_target(generated_files SOURCES ${GEN_FILE})
add_dependencies(${COMPONENT_LIB} generated_files)
target_include_directories(${COMPONENT_LIB} INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/_build)

