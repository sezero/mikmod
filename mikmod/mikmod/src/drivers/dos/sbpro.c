/*

Name: SBPRO.C

Description:
 Mikmod driver for output on Creative Labs Soundblasters, SB Pros,
 and compatibles (through DSP)

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


static BOOL SBPro_Detect(void)
{
   if(!SB_IsThere() || (sb_ver < 0x300)) return 0;

   sb_ver = 0x301;
   return 1;
}


static BOOL SBMono_Detect(void)
{
   if(!SB_IsThere()) return 0;
   sb_ver = 0x201;
   return 1;
}


static BOOL SBPro_PlayStart(void)
{
    if(sb_ver >= 0x300)
    {   if(md_mode & DMODE_STEREO)
            SB_MixerStereo();
        else
            SB_MixerMono();
    }

    if(SB_CommonPlayStart()) return 1;
    SB_SpeakerOn();

    SB_WriteDSP(0x40);
    SB_WriteDSP(SB_TIMECONSTANT);

    if(sb_ver < 0x200)
    {   SB_WriteDSP(0x14);
        SB_WriteDSP(0xff);
        SB_WriteDSP(0xfe);
    } else if(sb_ver == 0x200)
    {   SB_WriteDSP(0x48);
        SB_WriteDSP(0xff);
        SB_WriteDSP(0xfe);
        SB_WriteDSP(0x1c);
    } else
    {   SB_WriteDSP(0x48);
        SB_WriteDSP(0xff);
        SB_WriteDSP(0xfe);
        SB_WriteDSP(0x90);
    }
    return 0;
}


MDRIVER drv_sb =
{   "Soundblaster & compatibles",
    "Soundblaster Driver v3.0",
    0,255,

    NULL,
    SBMono_Detect,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    SB_Init,
    SB_Exit,
    SB_Reset,
    VC_SetNumVoices,
    VC_GetActiveVoices,
    SBPro_PlayStart,
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


MDRIVER drv_sbpro =
{   "Soundblaster Pro",
    "Soundblaster Pro Driver v3.0",
    0,255,

    NULL,
    SBPro_Detect,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    SB_Init,
    SB_Exit,
    SB_Reset,
    VC_SetNumVoices,
    VC_GetActiveVoices,
    SBPro_PlayStart,
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

