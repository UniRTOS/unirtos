#ifndef _MP3_DEC_COMMON_H_
#define _MP3_DEC_COMMON_H_

//#ifdef FEATURE_MP3_LIB_VPU_ENABLE


#ifndef WIN32
#include "sctdef.h"
#define printf(...)
#define fprintf(...)
#else
#include "FxpDefine.h"
#endif

#define Vpu_int32       int
#define Vpu_int16       short
#define Vpu_int8        signed char

#define Vpu_uint32      unsigned int
#define Vpu_uint16      unsigned short
#define Vpu_uint8       unsigned char


#ifndef WIN32
//#define MP3_DECODER_DECAPI_LOG
//#define MP3_DECODER_DECFRAME_LOG
//#define MP3_DECODER_DECCORE_LOG
//#define MP3_DECODER_PLCUPDATE_LOG
//#define MP3_DECODER_DECPARAM_LOG
//#define MP3_DECODER_DECRANGE_LOG
//#define MP3_DECODER_NLSF_LOG
//#define MP3_DECODER_NLSF2A_LOG
//#define MP3_DECODER_NLSF_MSVQ_LOG
//#define MP3_DECODER_DECPULSE_LOG
//#define MP3_DECODER_MAPREDICT_LOG
//#define MP3_RUNTIME_CONVERT(x) (x*1000/3072)
//#define MP3_RUNTIME_CONVERT2(x) (x/3072)

#define MP3_DECODER_RAMCODE				PLAT_FM_RAMCODE
#define MP3_DECODER_RAMCODE_ASMB			PLAT_FA_RAMCODE
#define MP3_DECODER_PSRAM_RAMDATA			PLAT_FPSRAM_DATA
#define MP3_DECODER_PSRAM_ZIDATA			PLAT_FPSRAM_ZI
#define MP3_DECODER_FORECEINLINE_CODE 		__FORCEINLINE
#else
#define MP3_DECODER_RAMCODE				
#define MP3_DECODER_RAMCODE_ASMB		
#define MP3_DECODER_PSRAM_RAMDATA		
#define MP3_DECODER_PSRAM_ZIDATA		
#define MP3_DECODER_FORECEINLINE_CODE
#endif
#if 1
struct mad_pcm 
{
    unsigned int samplerate;		/* sampling frequency (Hz) */
    unsigned short channels;		/* number of channels */
    unsigned short length;		    /* number of samples per channel */
    Vpu_int32 samples[2][1152];		/* PCM output samples [ch][sample] */
};

struct mad_synth 
{
    Vpu_int32 filter[2][2][2][16][8];	/* polyphase filterbank outputs */
  					                    /* [ch][eo][peo][s][v] */

    unsigned int phase;			        /* current processing phase */
    struct mad_pcm pcm;			        /* PCM output */
};

enum mad_layer 
{
    MAD_LAYER_I             = 1,	/* Layer I */
    MAD_LAYER_II            = 2,	/* Layer II */
    MAD_LAYER_III           = 3		/* Layer III */
};

enum mad_mode 
{
    MAD_MODE_SINGLE_CHANNEL = 0,	/* single channel */
    MAD_MODE_DUAL_CHANNEL	= 1,	/* dual channel */
    MAD_MODE_JOINT_STEREO	= 2,	/* joint (MS/intensity) stereo */
    MAD_MODE_STEREO	        = 3		/* normal LR stereo */
};

enum mad_emphasis 
{
    MAD_EMPHASIS_NONE	    = 0,	/* no emphasis */
    MAD_EMPHASIS_50_15_US	= 1,	/* 50/15 microseconds emphasis */
    MAD_EMPHASIS_CCITT_J_17 = 3,	/* CCITT J.17 emphasis */
    MAD_EMPHASIS_RESERVED   = 2		/* unknown emphasis */
};

typedef struct 
{
    signed long seconds;		    /* whole seconds */
    unsigned long fraction;	        /* 1/MAD_TIMER_RESOLUTION seconds */
} mad_timer_t;


struct mad_header 
{
    enum mad_layer layer;			/* audio layer (1, 2, or 3) */
    enum mad_mode mode;			    /* channel mode (see above) */
    int mode_extension;			    /* additional mode info */
    enum mad_emphasis emphasis;		/* de-emphasis to use (see above) */
    unsigned long bitrate;		    /* stream bitrate (bps) */
    unsigned int samplerate;		/* sampling frequency (Hz) */
    unsigned short crc_check;		/* frame CRC accumulator */
    unsigned short crc_target;		/* final target CRC checksum */
    int flags;				        /* flags (see below) */
    int private_bits;			    /* private bits (see below) */
    mad_timer_t duration;			/* audio playing time of frame */
};

struct mad_frame 
{
  struct mad_header header;		    /* MPEG audio header */
  int options;				        /* decoding options (from stream) */
  Vpu_int32 sbsample[2][36][32];	/* synthesis subband filter samples */
  Vpu_int32 (*overlap)[2][32][18];	/* Layer III block overlap data */
};
#endif



/*use this macro modifier instead of const to change the table position in ram or flash*/
#ifndef WIN32
#define MP3_CONST_TABLE_LOCATE __attribute__((aligned(4)))
#else
#define MP3_CONST_TABLE_LOCATE const
#endif

/*enable for MA prediction error filter, enable for C-level acceleration method */
#define MP3_DECODER_EC_CPUCORE

#ifndef WIN32
void mp3DecVpuInit(void);

void mp3DecVpuClock_On(void);

void mp3DecVpuClock_Off(void);
#endif

void synth_full_vpu(
        struct mad_synth *synth, 
        struct mad_frame const *frame,
		unsigned int nch, 
		unsigned int ns
		);

//#endif

#endif
