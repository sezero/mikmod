#include "mikmod.h"
#include "virtch.h"

#include <windows.h>
#include "win32au.h"

int config_numbufs=8,config_bufsize=0,config_prebuffer=100,config_prebuffer_as=1;

#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 8

static BOOL RAW_IsThere(void)
{
    return 1;
}


static int block_len, is8bit,ismono;

static uint wvolume   = 128;
static uint wmode     = DMODE_16BITS | DMODE_INTERP | DMODE_NOCLICK | DMODE_SAMPLE_DYNAMIC;
static uint wmixspeed  = 48000;
static uint wChannels = 2;

static BOOL initialized = 0;

static BOOL RAW_Init(void *optstr)
{
    int bufsize = BUFSIZE*1024, cn;
    int bits    = (wmode & DMODE_16BITS) ? 16 : 8;
    int ch      = wChannels;

    if (bits == 8) 
    {   bufsize /= 2;
        is8bit   = 1;
    } else is8bit = 0;

    if (ch == 1) 
    {   bufsize /= 2;
        ismono   = 1;
    } else ismono = 0;

    cn = config_numbufs;

    if (config_bufsize) 
    {   bufsize/=config_bufsize;
        cn *= config_bufsize;
    }

    //cn *= wmixspeed;
    //cn /= 44100;

    block_len = bufsize;

    ch = audioOpen(wmixspeed,ch,bits,bufsize,cn,config_prebuffer,config_prebuffer_as);
    if (ch)
    {   switch (ch)
        {   case MMSYSERR_NOMEM:
                MessageBox(NULL,"Could not initialize audio:\n"
                                "  Insufficient memory\n"
                                " (Try freeing up some memory or\n"
                                "  reducing the number of buffers\n"
                                "  in options->buffering)", "Error",MB_OK|MB_ICONEXCLAMATION);
            break;
            case WAVERR_BADFORMAT:
                MessageBox(NULL,"Could not initialize audio:\n"
                                "  Audio device unable to output this format\n",
                                "Error",MB_OK|MB_ICONEXCLAMATION);  
            break;
            case MMSYSERR_NODRIVER:
            case MMSYSERR_BADDEVICEID:
                    MessageBox(NULL,"Could not initialize audio:\n"
                                    "  No sound driver found\n","Error",MB_OK|MB_ICONEXCLAMATION);   
            break;
            case MMSYSERR_ALLOCATED:
                MessageBox(NULL,"Could not initialize audio:\n"
                                "  Audio device already in use\n",
                                "Error",MB_OK|MB_ICONEXCLAMATION);   
            break;
            case -666:
                    MessageBox(NULL,"  Couldn't create .WAV file\n",
                                    "Error",MB_OK|MB_ICONEXCLAMATION);  
            break;
                default:
                    MessageBox(NULL,"Could not initialize audio:\n"
                                    "  Unknown error\n",
                                    "Error",MB_OK|MB_ICONEXCLAMATION);   
            break;
        }
        return 1;
    }

    if(VC_Init())
    {   audioClose();
        return 1;
    }

    audioSetVolume((wvolume * 51) / 128); // 51=max
//    audioSetPan((wpan * 12) / PAN_RIGHT); // -12 to 12

    initialized = 1;
    return 0;
}


static void Win_Exit(void)
{
    VC_Exit();
    audioClose();
    initialized = 0;
}


static void Win_Update(MDRIVER *md)
{
    if (audioCanWrite())
        audioWrite(block_len);
}

/*static void Win_SetVolume(MDRIVER *volume)
{
    // Volume range is 0 to 51... stupid eh?
    audioSetVolume(((wvolume = volume) * 51) / 128);
}*/

static BOOL Win_SetMode(uint mixspeed, uint mode, uint channels, uint cpumode)
{
    // Check capabilities...
    
    // Set the new mode of play

    if(!initialized)
    {   if(mixspeed) wmixspeed = mixspeed;
        if(wmixspeed > 44100) wmixspeed = 44100;

        if(!(mode & DMODE_DEFAULT)) wmode = mode;

        wmode |= DMODE_SAMPLE_DYNAMIC;   // software mixer only

        switch(channels)
        {   case MD_MONO:
                wChannels = 1;
            break;

            case MD_STEREO:
//            case MD_SURROUND:
                wChannels = 2;
            break;

            case MD_QUADSOUND:
                wChannels = 2;
                channels  = MD_STEREO;
            break;
        }

        VC_SetMode(wmixspeed, wmode, channels, cpumode);
    }

    return 0;
}

MD_DEVICE drv_win =
{   "win32au",
    "Nullsoft win32 output driver v0.666",
    0, VC_MAXVOICES, 
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
    Win_Exit,
    Win_Update,
    VC_Preempt,

    NULL,
    VC_SetSoftVoices,

    Win_SetMode,
    VC_GetMode,

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

    VC_VoicePlay,
    VC_VoiceResume,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceReleaseSustain,
    VC_VoiceRealVolume,
};

