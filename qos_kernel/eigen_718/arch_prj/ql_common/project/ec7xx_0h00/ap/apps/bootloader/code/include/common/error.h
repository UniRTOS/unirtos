#ifndef __ERROR_H__
#define __ERROR_H__

#define NoError 0x0
#define IllegalAddressError 0x1

#define EfusePorTimeOutError 0x20
#define EfusePorFailError 0x21


#define EfuseSwReadTimeOutError 0x24
#define EFuseSwReadFailEror 0x25
#define EFuseSwReadLocError 0x26
#define SpiReadErr 0x30
#define SpiWriteErr 0x31
#define SpiEraseErr 0x32 

#define GPP_PackageNotFound 0x50
#define GPP_UnknownOperation 0x51
#define INSTR_Timeout 0x52
#define INSTR_InvalidMaskOrValue 0x53
#define INSTR_InvalidAddress 0x54
#define INSTR_UnknownInstruction 0x5f


#define InvalidHeadKeyHashError 0x6a
#define InvalidBodyKeyHashError 0x6b
#define InvalidEcdsaVerifyError 0x6c
#define IHNotFound 0x6d
#define ImageToBigError 0x6e
#define ImageHeadToBigError 0x6f
#define InvalidSecVerMagicError 0x70
#define InvalidSecVerError 0x71
#define InvalidPubkeyError 0x72

#define CmdMisMatchError 0x80
#define CommandIDError 0x81

#define DownloadImageTooBigError 0x94
#define TimeOutError 0x96


#define UnknownImageError 0x98
#define DownloadBreakError 0x99
#define CrcCheckError 0x9a
#define SequenceCheckError 0x9c

#define UartLNSError 0x9d
#define UartReadWriteTimeOutError 0x9e

#define SHABusyError 0xa0
#define SHATimeOutError 0xa1
#define AESBusyError 0xa2
#define AESTimeOutError 0xa3

#define TestError 0xff000000
#endif
