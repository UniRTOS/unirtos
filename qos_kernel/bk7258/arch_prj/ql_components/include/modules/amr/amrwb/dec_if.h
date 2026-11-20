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

#ifndef DEC_IF_H
#define DEC_IF_H

#ifdef __cplusplus
extern "C" {
#endif

#define _good_frame 0

/**
 * @brief     Initialize the AMR-WB decoder interface.
 *
 * This function allocates and initializes the decoder state required to
 * process AMR-WB frames. The caller must verify the return value and release
 * the state with `D_IF_exit` after use.
 *
 * @return Pointer to an opaque decoder state on success, NULL on memory
 *         allocation failure.
 */
void* D_IF_init(void);

/**
 * @brief     Decode one AMR-WB frame.
 *
 * This function decodes a compressed AMR-WB frame to linear PCM samples. The
 * caller must provide valid input and output buffers; the function does not
 * perform bounds checking internally.
 *
 * @param state Decoder state handle.
 * @param bits Pointer to the input buffer containing one encoded frame.
 * @param synth Pointer to the output PCM buffer; must hold 320 samples.
 * @param bfi Bad frame indicator; non-zero marks the frame as corrupted.
 */
void D_IF_decode(void* state, const unsigned char* bits, short* synth, int bfi);

/**
 * @brief     Release an AMR-WB decoder state.
 *
 * This function frees resources associated with the decoder state previously
 * created by `D_IF_init`. Passing NULL is safe and results in no operation.
 *
 * @param state Decoder state handle obtained from `D_IF_init`.
 */
void D_IF_exit(void* state);

#ifdef __cplusplus
}
#endif

#endif

