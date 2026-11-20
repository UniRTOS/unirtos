#ifndef ASR_FN_ASR_H_
#define ASR_FN_ASR_H_

#ifdef __cplusplus
extern "C" {
#endif

#if(CONFIG_WANSON_ASR_GROUP_VERSION)
#include "fst_01.h"
#include "fst_02.h"
#include "fst_types.h"
#else
#include "fst_graph.h"
#include "fst_words.h"
#endif
// Return value   : 0 - OK, -1 - Error
#if (CONFIG_WANSON_CN_LICENSE)
int  Wanson_ASR_Init(uint8_t userid[8]); 
#else
int  Wanson_ASR_Init();
#endif

#if(CONFIG_WANSON_ASR_GROUP_VERSION)
void Wanson_ASR_Set_Fst(Fst *fst);
#endif
void Wanson_ASR_Reset();

/*****************************************
* Input:
*      - buf      : Audio data (16k, 16bit, mono)
*      - buf_len  : Now must be 480 (30ms)
*
* Output:
*      - text     : The text of ASR
*      - score    : The confidence of ASR (Now not used)
*
* Return value    :  0 - No result
*                    1 - Has result
*                   -1 - Error
******************************************/
int  Wanson_ASR_Recog(short *buf, int buf_len, const char **text, float *score);

void Wanson_ASR_Release();

#ifdef __cplusplus
}
#endif

#endif
