
#include "mikmod.h"
#include "virtch.h"

#ifdef __GNUC__
#include <sys/types.h>
#else
#include <io.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>

#define WAVBUFFERSIZE 8192

// =====================================================================================
    typedef struct WAV_LOCALINFO
// =====================================================================================
{
    MMSTREAM  *wavout;
    SBYTE     *WAV_DMABUF;
    ULONG      dumpsize;

    // driver mode settings
    uint       mode;
    uint       mixspeed;
    uint       channels;
} WAV_LOCALINFO;


// =====================================================================================
    static BOOL WAV_IsThere(void)
// =====================================================================================
{
    return 1;
}


// =====================================================================================
    static BOOL WAV_Init(MDRIVER *md, uint latency, void *optstr)
// =====================================================================================
{
    WAV_LOCALINFO  *hwdata;

    hwdata = (WAV_LOCALINFO *)_mm_calloc(md->allochandle, 1, sizeof(WAV_LOCALINFO));
    if(NULL == (hwdata->wavout = _mm_fopen("music.wav", "wb"))) return 1;
    if(NULL == (hwdata->WAV_DMABUF = _mm_malloc(md->allochandle, WAVBUFFERSIZE))) return 1;

    md->device.vc = VC_Init();
    if(!md->device.vc) return 1;

    hwdata->mode     = DMODE_16BITS | DMODE_INTERP | DMODE_NOCLICK;
    hwdata->mixspeed = 48000;
    hwdata->channels = 2;

    md->device.local = hwdata;
    
    return 0;
}


// =====================================================================================
    static void WAV_Exit(MDRIVER *md)
// =====================================================================================
{
    WAV_LOCALINFO  *hwdata = md->device.local;

    VC_Exit(md->device.vc);

    // write in the actual sizes now

    if(hwdata->wavout)
    {   _mm_fseek(hwdata->wavout,4,SEEK_SET);
        _mm_write_I_ULONG(hwdata->dumpsize + 34, hwdata->wavout);
        _mm_fseek(hwdata->wavout,42,SEEK_SET);
        _mm_write_I_ULONG(hwdata->dumpsize, hwdata->wavout);

        _mm_fclose(hwdata->wavout);
        _mm_free(md->allochandle, hwdata->WAV_DMABUF);
    }
}


// =====================================================================================
    static void WAV_Update(MDRIVER *md)
// =====================================================================================
{
    WAV_LOCALINFO  *hwdata = md->device.local;

    VC_WriteBytes(md, hwdata->WAV_DMABUF, WAVBUFFERSIZE);
    _mm_write_SBYTES(hwdata->WAV_DMABUF, WAVBUFFERSIZE, hwdata->wavout);
    hwdata->dumpsize += WAVBUFFERSIZE;
}


// =====================================================================================
    static BOOL WAV_SetMode(MDRIVER *md, uint mixspeed, uint mode, uint channels, uint cpumode)
// =====================================================================================
{
    WAV_LOCALINFO  *hwdata = md->device.local;

    //mode = DMODE_16BITS | DMODE_INTERP | DMODE_NOCLICK | DMODE_SAMPLE_DYNAMIC;

    if(mixspeed) hwdata->mixspeed = mixspeed;
    if(!(mode & DMODE_DEFAULT)) hwdata->mode = mode;

    hwdata->mode |= DMODE_SAMPLE_DYNAMIC;   // software mixer only.

    switch(channels)
    {   case MD_MONO:
            hwdata->channels = 1;
        break;

        case MD_STEREO:
            hwdata->channels = 2;
        break;

        case MD_QUADSOUND:
            hwdata->channels = 4;
        break;
    }

    VC_SetMode(md->device.vc, hwdata->mixspeed, hwdata->mode, channels, cpumode);

    _mm_write_string("RIFF    WAVEfmt ",hwdata->wavout);
    _mm_write_I_ULONG(18,hwdata->wavout);     // length of this RIFF block crap

    _mm_write_I_UWORD(1, hwdata->wavout);     // microsoft format type
    _mm_write_I_UWORD(hwdata->channels, hwdata->wavout);
    _mm_write_I_ULONG(hwdata->mixspeed, hwdata->wavout);
    _mm_write_I_ULONG(hwdata->mixspeed * hwdata->channels *
                      ((hwdata->mode & DMODE_16BITS) ? 2 : 1), hwdata->wavout);

    _mm_write_I_UWORD(((hwdata->mode & DMODE_16BITS) ? 2 : 1) * (hwdata->channels), hwdata->wavout);    // block alignment (8/16 bit)

    _mm_write_I_UWORD((hwdata->mode & DMODE_16BITS) ? 16 : 8, hwdata->wavout);
    _mm_write_I_UWORD(0,hwdata->wavout);      // No extra data here.

    _mm_write_string("data    ",hwdata->wavout);

    hwdata->dumpsize = 0;

    return 0;
}


// =====================================================================================
    static BOOL WAV_SetSoftVoices(MDRIVER *md, uint voices)
// =====================================================================================
{
    return VC_SetSoftVoices(md->device.vc, voices);
}


// =====================================================================================
    static void WAV_GetMode(MDRIVER *md, uint *mixspeed, uint *mode, uint *channels, uint *cpumode)
// =====================================================================================
{
    VC_GetMode(md->device.vc, mixspeed, mode, channels, cpumode);
}


MD_DEVICE drv_wav =
{   
    "music.wav file",
    "WAV [music.wav] file output driver v1.0",
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
    WAV_IsThere,
    WAV_Init,
    WAV_Exit,
    WAV_Update,
    VC_Preempt,

    NULL,
    WAV_SetSoftVoices,

    WAV_SetMode,
    WAV_GetMode,

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
    VC_VoiceGetPosition
};

