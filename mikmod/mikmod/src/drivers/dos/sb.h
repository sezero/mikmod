
#ifndef _SB_H_
#define _SB_H_

#include "mdma.h"

#ifdef __cplusplus
extern "C" {
#endif

void  SB_MixerMono(void);
void  SB_MixerStereo(void);
BOOL  SB_IsThere(void);
BOOL  SB_Ping(void);
void  SB_ResetDSP(void);
UWORD SB_ReadDSP(void);
BOOL  SB_WriteDSP(UBYTE data);
void  SB_SpeakerOn(void);
void  SB_SpeakerOff(void);
BOOL  SB_Ping(void);

BOOL  SB_Init(void);
void  SB_Exit(void);
BOOL  SB_Reset(void);
void  SB_Update(void);
void  SB_PlayStop(void);
BOOL  SB_CommonPlayStart(void);
BOOL  SB_DMAreset(void);


extern DMAMEM *SB_DMAMEM;
extern SBYTE  *SB_DMABUF;

extern UBYTE SB_TIMECONSTANT;

extern UWORD sb_port;          // sb base port
extern UWORD sb_int;           // interrupt vector that belongs to sb_irq
extern UWORD sb_ver;           // DSP version number
extern UBYTE sb_irq;           // sb irq
extern UBYTE sb_lodma;         // 8 bit dma channel (1.0/2.0/pro)
extern UBYTE sb_hidma;         // 16 bit dma channel (16/16asp)
extern UBYTE sb_dma;           // current dma channel
extern UBYTE sb_mode;          // internal copy of md_mode

extern unsigned int AWE32Base;

#ifdef __cplusplus
};
#endif


#endif
