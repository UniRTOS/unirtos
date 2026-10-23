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

#ifndef INTERF_DEC_H
#define INTERF_DEC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Initialize the AMR-NB decoder interface.
 *
 * This function allocates and prepares the internal state required to decode
 * AMR-NB frames. The caller must check the return value and release the state
 * with `Decoder_Interface_exit` after use.
 *
 * @return Pointer to an opaque decoder state on success, NULL on memory
 *         allocation failure.
 */
void* Decoder_Interface_init(void);

/**
 * @brief     Release an AMR-NB decoder state.
 *
 * This function frees resources associated with the decoder state previously
 * created by `Decoder_Interface_init`. The caller must ensure that `state` is a
 * valid pointer; passing NULL is safe and results in no operation.
 *
 * @param state Decoder state handle obtained from `Decoder_Interface_init`.
 */
void Decoder_Interface_exit(void* state);

/**
 * @brief     Decode one AMR-NB frame.
 *
 * This function decodes a single compressed AMR-NB frame to linear PCM samples.
 * The caller must provide valid input and output buffers; no buffer overrun
 * checks are performed internally.
 *
 * @param state Decoder state handle.
 * @param in Pointer to the input buffer containing one encoded frame.
 * @param out Pointer to the output PCM buffer; must hold 160 samples.
 * @param bfi Bad frame indicator; non-zero marks the frame as corrupted.
 */
void Decoder_Interface_Decode(void* state, const unsigned char* in, short* out, int bfi);

#ifdef __cplusplus
}
#endif

#endif

