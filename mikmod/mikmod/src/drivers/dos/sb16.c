/*

Name: DRV_SB16.C

Description:
 Mikmod driver for output on Creative Labs Soundblaster 16's and
 compatables.

Portability:

 MSDOS:  BC(y)   Watcom(y)   DJGPP(y)
 Win95:  n
 Os2:    n
 Linux:  n

 (y) - yes
 (n) - no (not possible or not useful)
 (?) - may be possible, but not tested

*/


#include <dos.h>
#include <conio.h>
#ifndef __DJGPP__
#include <mem.h>
#endif

#include "mikmod.h"
#include "mirq.h"
#include "sb.h"


static BOOL SB16_Detect(void)
{
    if(!SB_IsThere() || (sb_hidma==0xff) || (sb_ver < 0x400)) return 0;
    return 1;
}


static BOOL SB16_PlayStart(void)
{
    if(SB_CommonPlayStart()) return 1;

    SB_WriteDSP(0x41);
    SB_WriteDSP(md_mixfreq >> 8);
    SB_WriteDSP(md_mixfreq & 0xff);

    if(md_mode & DMODE_16BITS)
    {   SB_WriteDSP(0xb6);
        SB_WriteDSP((md_mode & DMODE_STEREO) ? 0x30 : 0x10);
    } else
    {   SB_WriteDSP(0xc6);
        SB_WriteDSP((md_mode & DMODE_STEREO) ? 0x20 : 0x00);
    }

    SB_WriteDSP(0xff);
    SB_WriteDSP(0xef);

    return 0;
}


static void SB16_Exit(void)
{
    if(md_mode & DMODE_16BITS)
        SB_WriteDSP(0xda);
    else
        SB_WriteDSP(0xd9);

    SB_Exit();
}


MDRIVER drv_sb16 =
{   "Soundblaster 16",
    "Soundblaster 16 Driver v3.0",
    0,255,

    NULL,
    SB16_Detect,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    SB_Init,
    SB16_Exit,
    SB_Reset,
    VC_SetNumVoices,
    VC_GetActiveVoices,
    SB16_PlayStart,
    SB_PlayStop,
    SB_Update,
    VC_VoiceSetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceSetPanning,
    VC_VoicePlay,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceReleaseSustain,
    VC_VoiceGetPosition,
    VC_VoiceRealVolume
};

