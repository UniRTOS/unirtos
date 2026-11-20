// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file i2c_unified.c
 * @brief Unified I2C API routing layer
 *
 * This file provides a unified interface for all I2C operations.
 * It automatically routes calls to either hardware I2C or simulated I2C
 * based on the I2C ID parameter:
 *   - id < SOC_I2C_UNIT_NUM: Routes to hardware I2C (i2c_driver.c)
 *   - id >= SOC_I2C_UNIT_NUM: Routes to simulated I2C (sim_i2c_driver.c)
 */

#include <driver/i2c.h>
#include <driver/i2c_types.h>
#include <common/bk_err.h>
#include <stdint.h>
#include <sdkconfig.h>
#include "i2c_driver.h"  // For I2C logging macros
#include <soc/soc.h>

// Define SOC_I2C_UNIT_NUM default value first (before including headers)
// This ensures it's always available even if header inclusion fails
#ifndef SOC_I2C_UNIT_NUM
#define SOC_I2C_UNIT_NUM  2  // Default to 2 hardware I2C units
#endif

// Define SIM_I2C_START_ID if not defined (for backward compatibility)
#ifndef SIM_I2C_START_ID
#define SIM_I2C_START_ID  SOC_I2C_UNIT_NUM
#endif

// Declare hardware I2C implementation functions (from i2c_driver.c)
// These are always available regardless of CONFIG_SIM_I2C
extern bk_err_t i2c_hardware_driver_init(void);
extern bk_err_t i2c_hardware_driver_deinit(void);
extern bk_err_t i2c_hardware_init(i2c_id_t id, const i2c_config_t *cfg);
extern bk_err_t i2c_hardware_deinit(i2c_id_t id);
extern bk_err_t i2c_hardware_memory_write(i2c_id_t id, const i2c_mem_param_t *mem_param);
extern bk_err_t i2c_hardware_memory_read(i2c_id_t id, const i2c_mem_param_t *mem_param);

// Declare simulated I2C implementation functions (from sim_i2c_driver.c)
#if CONFIG_SIM_I2C
#if CONFIG_SIM_I2C_HW_BOARD_V3
// V3 version exports _v2 functions
extern bk_err_t bk_i2c_driver_init_v2(void);
extern bk_err_t bk_i2c_driver_deinit_v2(void);
extern bk_err_t bk_i2c_init_v2(i2c_id_t id, const i2c_config_t *cfg);
extern bk_err_t bk_i2c_deinit_v2(i2c_id_t id);
extern bk_err_t bk_i2c_memory_write_v2(i2c_id_t id, const i2c_mem_param_t *mem_param);
extern bk_err_t bk_i2c_memory_read_v2(i2c_id_t id, const i2c_mem_param_t *mem_param);
#else
// Non-V3 version exports sim_i2c_* functions
extern bk_err_t sim_i2c_init(i2c_id_t id, const i2c_config_t *cfg);
extern bk_err_t sim_i2c_deinit(i2c_id_t id);
extern bk_err_t sim_i2c_memory_write(i2c_id_t id, const i2c_mem_param_t *mem_param);
extern bk_err_t sim_i2c_memory_read(i2c_id_t id, const i2c_mem_param_t *mem_param);
#endif
#endif

// ============================================================================
// ID Validation Macros and Helpers
// ============================================================================

/**
 * @brief Validate I2C ID based on CONFIG_SIM_I2C configuration
 *
 * Validation rules:
 * - When CONFIG_SIM_I2C is disabled: Only hardware I2C IDs (0 to SOC_I2C_UNIT_NUM-1) are valid
 * - When CONFIG_SIM_I2C is enabled:
 *   - Hardware I2C IDs: 0 to SOC_I2C_UNIT_NUM-1
 *   - Simulated I2C IDs: SIM_I2C_START_ID (typically SOC_I2C_UNIT_NUM) and above
 *
 * @param id I2C ID to validate
 * @return BK_OK if valid, BK_ERR_I2C_INVALID_ID if invalid
 */
static inline bk_err_t i2c_validate_id(i2c_id_t id)
{
#if CONFIG_SIM_I2C
	// When SIM_I2C is enabled, accept both hardware and simulated I2C IDs
	if ((uint8_t)id < SOC_I2C_UNIT_NUM) {
		// Hardware I2C ID (0 to SOC_I2C_UNIT_NUM-1): valid
		return BK_OK;
	} else if ((uint8_t)id >= SIM_I2C_START_ID) {
		// Simulated I2C ID (>= SIM_I2C_START_ID): valid
		// For BK7258/BK7257: SIM_I2C_START_ID == SOC_I2C_UNIT_NUM == 2
		// So simulated I2C IDs start from 2
		return BK_OK;
	} else {
		// ID between SOC_I2C_UNIT_NUM and SIM_I2C_START_ID: invalid gap
		// This should not happen if SIM_I2C_START_ID == SOC_I2C_UNIT_NUM (normal case)
		return BK_ERR_I2C_INVALID_ID;
	}
#else
	// When SIM_I2C is disabled, only hardware I2C IDs (0 to SOC_I2C_UNIT_NUM-1) are valid
	if ((uint8_t)id < SOC_I2C_UNIT_NUM) {
		return BK_OK;
	} else {
		// Simulated I2C IDs are not allowed when CONFIG_SIM_I2C is disabled
		// This is an error condition - user tried to use simulated I2C ID without enabling CONFIG_SIM_I2C
		return BK_ERR_I2C_INVALID_ID;
	}
#endif
}

// ============================================================================
// Unified Standard API Implementation
// These functions are always available and route to the appropriate implementation
// ============================================================================

bk_err_t bk_i2c_driver_init(void)
{
#if CONFIG_SIM_I2C
	// Initialize hardware I2C driver infrastructure (for id < SOC_I2C_UNIT_NUM)
	bk_err_t ret = i2c_hardware_driver_init();
	if (ret != BK_OK) {
		return ret;
	}
	// Initialize simulated I2C driver infrastructure (for id >= SOC_I2C_UNIT_NUM)
#if CONFIG_SIM_I2C_HW_BOARD_V3
	return bk_i2c_driver_init_v2();
#else
	// Non-V3 version: init happens in sim_i2c_init(), driver_init just returns OK
	return BK_OK;
#endif
#else
	// Only hardware I2C - call hardware implementation directly
	extern bk_err_t i2c_hardware_driver_init(void);
	return i2c_hardware_driver_init();
#endif
}

bk_err_t bk_i2c_driver_deinit(void)
{
#if CONFIG_SIM_I2C
	// Deinitialize hardware I2C driver
	bk_err_t ret = i2c_hardware_driver_deinit();
#if CONFIG_SIM_I2C_HW_BOARD_V3
	// Deinitialize simulated I2C driver
	bk_err_t ret2 = bk_i2c_driver_deinit_v2();
	return (ret == BK_OK) ? ret2 : ret;
#else
	return ret;
#endif
#else
	// Only hardware I2C - call hardware implementation directly
	extern bk_err_t i2c_hardware_driver_deinit(void);
	return i2c_hardware_driver_deinit();
#endif
}

bk_err_t bk_i2c_init(i2c_id_t id, const i2c_config_t *cfg)
{
	// Validate I2C ID first
	bk_err_t ret = i2c_validate_id(id);
	if (ret != BK_OK) {
#if CONFIG_SIM_I2C
		I2C_LOGE("Invalid I2C ID %d: When CONFIG_SIM_I2C is enabled, hardware I2C IDs are 0-%d, simulated I2C IDs start from %d\r\n",
		         id, SOC_I2C_UNIT_NUM - 1, SIM_I2C_START_ID);
#else
		I2C_LOGE("Invalid I2C ID %d: When CONFIG_SIM_I2C is disabled, only hardware I2C IDs 0-%d are valid\r\n",
		         id, SOC_I2C_UNIT_NUM - 1);
#endif
		return ret;
	}

#if CONFIG_SIM_I2C
	// Route based on id:
	// - id < SOC_I2C_UNIT_NUM: Uses hardware I2C (I2C_ID_0, I2C_ID_1, etc.)
	// - id >= SIM_I2C_START_ID: Uses GPIO simulated I2C (starting from I2C_ID_2)
	if ((uint8_t)id < SOC_I2C_UNIT_NUM) {
		return i2c_hardware_init(id, cfg);
	} else {
		// Simulated I2C
#if CONFIG_SIM_I2C_HW_BOARD_V3
		return bk_i2c_init_v2(id, cfg);
#else
		return sim_i2c_init(id, cfg);
#endif
	}
#else
	// Only hardware I2C - call hardware implementation directly
	return i2c_hardware_init(id, cfg);
#endif
}

bk_err_t bk_i2c_deinit(i2c_id_t id)
{
	// Validate I2C ID first
	bk_err_t ret = i2c_validate_id(id);
	if (ret != BK_OK) {
		return ret;
	}

#if CONFIG_SIM_I2C
	// Route based on id
	if ((uint8_t)id < SOC_I2C_UNIT_NUM) {
		return i2c_hardware_deinit(id);
	} else {
		// Simulated I2C
#if CONFIG_SIM_I2C_HW_BOARD_V3
		return bk_i2c_deinit_v2(id);
#else
		return sim_i2c_deinit(id);
#endif
	}
#else
	// Only hardware I2C - call hardware implementation directly
	return i2c_hardware_deinit(id);
#endif
}

bk_err_t bk_i2c_memory_write(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	// Validate I2C ID first
	bk_err_t ret = i2c_validate_id(id);
	if (ret != BK_OK) {
		return ret;
	}

#if CONFIG_SIM_I2C
	// Route based on id
	if ((uint8_t)id < SOC_I2C_UNIT_NUM) {
		return i2c_hardware_memory_write(id, mem_param);
	} else {
		// Simulated I2C
#if CONFIG_SIM_I2C_HW_BOARD_V3
		return bk_i2c_memory_write_v2(id, mem_param);
#else
		return sim_i2c_memory_write(id, mem_param);
#endif
	}
#else
	// Only hardware I2C - call hardware implementation directly
	return i2c_hardware_memory_write(id, mem_param);
#endif
}

bk_err_t bk_i2c_memory_read(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	// Validate I2C ID first
	bk_err_t ret = i2c_validate_id(id);
	if (ret != BK_OK) {
		return ret;
	}

#if CONFIG_SIM_I2C
	// Route based on id
	if ((uint8_t)id < SOC_I2C_UNIT_NUM) {
		return i2c_hardware_memory_read(id, mem_param);
	} else {
		// Simulated I2C
#if CONFIG_SIM_I2C_HW_BOARD_V3
		return bk_i2c_memory_read_v2(id, mem_param);
#else
		return sim_i2c_memory_read(id, mem_param);
#endif
	}
#else
	// Only hardware I2C - call hardware implementation directly
	return i2c_hardware_memory_read(id, mem_param);
#endif
}

