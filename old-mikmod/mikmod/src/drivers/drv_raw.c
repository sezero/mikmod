/*

Name:
DRV_RAW.C

Description:
Mikmod driver for output to a file called MUSIC.RAW

MS-DOS Programmers:
 !! DO NOT CALL MD_UPDATE FROM AN INTERRUPT IF YOU USE THIS DRIVER !!

Portability:

MSDOS:  BC(y)   Watcom(y)   DJGPP(y)
Win95:  BC(y)
Linux:  y

(y) - yes
(n) - no (not possible or not useful)
(?) - may be possible, but not tested

*/

#include "mikmod.h"
#include "virtch.h"

#ifdef __GNUC__
#include <sys/types.h>
#else
#include <io.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>

#define RAWBUFFERSIZE 8192

// =====================================================================================
    typedef struct RAW_LOCALINFO
// =====================================================================================
{
    int     rawout;
    SBYTE   RAW_DMABUF[RAWBUFFERSIZE];
} RAW_LOCALINFO;


// =====================================================================================
    static BOOL RAW_IsThere(void)
// =====================================================================================
{
    return 1;
}


// =====================================================================================
    static BOOL RAW_Init(MDRIVER *md, uint latency, void *optstr)
// =====================================================================================
{
    RAW_LOCALINFO  *hwdata;

    hwdata = (RAW_LOCALINFO *)_mm_calloc(md->allochandle, 1, sizeof(RAW_LOCALINFO));

    if(-1 == (hwdata->rawout = open("music.raw", 
#ifndef __GNUC__
                O_BINARY | 
#endif
                O_RDWR | O_TRUNC | O_CREAT, S_IREAD | S_IWRITE)))
        return 1;

    md->device.vc = VC_Init();
    if(!md->device.vc) return 1;

    md->device.local = hwdata;

    return 0;
}


// =====================================================================================
    static void RAW_Exit(MDRIVER *md)
// =====================================================================================
{
    RAW_LOCALINFO  *hwdata = md->device.local;

    VC_Exit(md->device.vc);
    close(hwdata->rawout);
}


// =====================================================================================
    static void RAW_Update(MDRIVER *md)
// =====================================================================================
{
    RAW_LOCALINFO  *hwdata = md->device.local;

    VC_WriteBytes(md, hwdata->RAW_DMABUF, RAWBUFFERSIZE);
    write(hwdata->rawout, hwdata->RAW_DMABUF, RAWBUFFERSIZE);
}


// =====================================================================================
    static BOOL RAW_SetSoftVoices(MDRIVER *md, uint voices)
// =====================================================================================
{
    return VC_SetSoftVoices(md->device.vc, voices);
}


// =====================================================================================
    static BOOL RAW_SetMode(MDRIVER *md, uint mixspeed, uint mode, uint channels, uint cpumode)
// =====================================================================================
{
    return VC_SetMode(md->device.vc, mixspeed, mode, channels, cpumode);
}


// =====================================================================================
    static void RAW_GetMode(MDRIVER *md, uint *mixspeed, uint *mode, uint *channels, uint *cpumode)
// =====================================================================================
{
    VC_GetMode(md->device.vc, mixspeed, mode, channels, cpumode);
}


MD_DEVICE drv_raw =
{   
    "music.raw file",
    "RAW [music.raw] file output driver v1.0",
    0,VC_MAXVOICES,

    NULL,
    NULL,
    NULL,

    // Sample loading
    VC_SampleAlloc,
    VC_SampleGetPtr,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,

    // Detection and initialization
    RAW_IsThere,
    RAW_Init,
    RAW_Exit,
    RAW_Update,
    VC_Preempt,

    NULL,
    RAW_SetSoftVoices,

    RAW_SetMode,
    RAW_GetMode,

    VC_SetVolume,
    VC_GetVolume,

    // Voice control and voice information
    VC_GetActiveVoices,

    VC_VoiceSetVolume,
    VC_VoiceGetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceGetFrequency,
    VC_VoiceSetPosition,
    VC_VoiceGetPosition,
    VC_VoiceSetSurround,
    VC_VoiceSetResonance,

    VC_VoicePlay,
    VC_VoiceResume,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceReleaseSustain,

    VC_VoiceRealVolume
};

