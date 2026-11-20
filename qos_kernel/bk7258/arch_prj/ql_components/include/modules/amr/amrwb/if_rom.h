/* ------------------------------------------------------------------
 * Copyright (C) 2009 Martin Storsjo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#ifndef IF_ROM_H
#define IF_ROM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief     Signed 16-bit word type used by the AMR-WB decoder.
 *
 * This typedef documents the fixed-width sample type required by ROM-based
 * tables and routines. It maps directly to the ISO C99 `int16_t`.
 */
typedef int16_t Word16;

#ifdef __cplusplus
}
#endif

#endif

