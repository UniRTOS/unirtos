// Copyright 2020-2021 Beken
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Number of hardware I2C units available on this SOC
// Hardware I2C IDs: 0, 1, ..., (SOC_I2C_UNIT_NUM - 1)
#define SOC_I2C_UNIT_NUM              2

// Simulated I2C configuration
// When CONFIG_SIM_I2C is enabled, simulated I2C IDs start from SOC_I2C_UNIT_NUM
// Simulated I2C IDs: SOC_I2C_UNIT_NUM, SOC_I2C_UNIT_NUM + 1, ...
// For BK7257: Hardware I2C uses ID 0-1, Simulated I2C uses ID 2+
#define SIM_I2C_START_ID              SOC_I2C_UNIT_NUM

#ifdef __cplusplus
}
#endif

