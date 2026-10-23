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

#ifndef INTERF_ENC_H
#define INTERF_ENC_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AMRNB_WRAPPER_INTERNAL
/* Copied from enc/src/gsmamr_enc.h */
enum Mode {
	MR475 = 0,/* 4.75 kbps */
	MR515,    /* 5.15 kbps */
	MR59,     /* 5.90 kbps */
	MR67,     /* 6.70 kbps */
	MR74,     /* 7.40 kbps */
	MR795,    /* 7.95 kbps */
	MR102,    /* 10.2 kbps */
	MR122,    /* 12.2 kbps */
	MRDTX,    /* DTX       */
	N_MODES   /* Not Used  */
};
#endif

/**
 * @brief     Initialize the AMR-NB encoder interface.
 *
 * This function allocates and configures the encoder state. The caller must
 * check the return value and release the state with `Encoder_Interface_exit`
 * when encoding is no longer required.
 *
 * @param dtx Discontinuous transmission flag; non-zero enables DTX mode.
 *
 * @return Pointer to an opaque encoder state on success, NULL on memory
 *         allocation failure.
 */
void* Encoder_Interface_init(int dtx);

/**
 * @brief     Release an AMR-NB encoder state.
 *
 * This function frees internal resources associated with the encoder state
 * previously created by `Encoder_Interface_init`. Passing NULL is safe and
 * results in no operation.
 *
 * @param state Encoder state handle obtained from `Encoder_Interface_init`.
 */
void Encoder_Interface_exit(void* state);

/**
 * @brief     Encode one frame of 16-bit PCM samples.
 *
 * This function converts a single frame of linear PCM samples to a compressed
 * AMR-NB bitstream. The caller must ensure that buffers are valid; the function
 * does not perform buffer size checks.
 *
 * @param state Encoder state handle.
 * @param mode Target bitrate mode.
 * @param speech Pointer to 160-sample PCM input buffer.
 * @param out Pointer to the output buffer receiving the encoded frame.
 * @param forceSpeech Non-zero forces encoding in speech mode even when DTX is enabled.
 *
 * @return Size in bytes of the encoded frame on success, negative value on failure.
 */
int Encoder_Interface_Encode(void* state, enum Mode mode, const short* speech, unsigned char* out, int forceSpeech);

#ifdef __cplusplus
}
#endif

#endif

