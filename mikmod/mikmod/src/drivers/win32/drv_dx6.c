/*
this documentation is all out of date as of 9/4/02
*/
/*

Name:
DRV_dx6.C
by matthew gambrell
zeromus@verge-rpg.com

Description:
Mikmod driver for output via directsound

Portability:

MSDOS:  N/A
Win95:  BC(?)   Watcom(y) MSVC6(y)
Linux:  WINE(?)

(y) - yes
(n) - no (not possible or not useful)
(?) - may be possible, but not tested


Notes about Directsound:

 Directsound is kinda funny about working in cooperative and exclusive
 modes, since it doesn't really behae how the docs would indicate (at
 least by how I read them).

 Our primary buffer always has to be DSSCL_PRIMARY, otherwise we can-
 not alter the mixer settings.
   
 In exclusive mode, we set the primary buffer to DSSCL_EXCLUSIVE,
 which keeps the mixer from continuing while the game is in the back-
 ground.  Without it, some drivers will become unstable and crash,
 and the music will play but not be heard (usually undesired).

 In Cooperative mode, the secondary buffer has to be flagged with
 DSCAPS_GLOBALFOCUS, which works perfectly fine for any soundcard
 (or driver which supports playing multiple waveforms.  This will
 not work on SB16s and other like soundcards, but well, they're
 used to getting shafted.

 NOTE:
    Currently the primary buffer is *forced* to 44100 khz.  This should
    be changed to be smarter: to set itself to the max output of the
    driver's capabilities.

*/

/* This driver had a nasty habit of overwriting the playing sound
   data causing horrible glitches in the sound output. This was
   fixed by changing the driver to synchronise its buffer writes
   with playback instead of just writing more or less to any old
   part of the buffer. Also fixed problem with fixed latency.
     - Jan Lönnberg. */

//#ifdef HAVE_CONFIG_H
//#include "config.h"
//#endif

#define NO_STRICT

#include "mikmod.h"
#include "virtch.h"

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <stdio.h>

#define MINIMUM_LATENCY     (60)        // was 10
#define BUFFER_SIZE         (2000)
#define LAG_INCREMENT       (64)        // was 16
#define ERROR_TOLERANCE     (2500)

static void DS_WipeBuffers(MDRIVER *md);

// =====================================================================================
    typedef struct DS_LOCALINFO
// =====================================================================================
// New and improved instance-ized directsound localinfo crap, which is allocated and
// attached to the MD_DEVICE info block during the call to DS_INIT (called from Mikmod_Init).
{
    HINSTANCE           dll;

    LPDIRECTSOUND       ds;
    LPDIRECTSOUNDBUFFER pb;
    LPDIRECTSOUNDBUFFER bb;
    DSCAPS              DSCaps;

    uint   mode;
    uint   mixspeed;
    uint   channels;
    int    dorefresh;
	uint	bufsize;
	uint	latency;
	int		currpos;
	char *nullbuf;
	int lag;
	int lastwrite;
	int lastwritecursor;
	int samplesize;
	int lasterror;

	int filterwindow[7];


} DS_LOCALINFO;


// =====================================================================================
    static void logerror(const CHAR *function, int code)
// =====================================================================================
{
	static CHAR *error;

	switch (code)
    {   case E_NOINTERFACE:
			error = "Unsupported interface (DirectX version incompatability)";
		break;

        case DSERR_ALLOCATED:
			// This one gets a special action since the user may want to be
            // informed that their sound is tied up.
            error = "Audio device in use.";
            _mmerr_set(MMERR_INITIALIZING_DEVICE, "DirectSound Initialization Failure", "The audio device is already in use and I'm not allowed to use it.  Woops!");
		break;

        case DSERR_BADFORMAT:
			error = "Unsupported audio format";
		break;

        case DSERR_BUFFERLOST:
			error = "Mixing buffer was lost";
		break;

        case DSERR_INVALIDPARAM:
			error = "Invalid parameter";
		break;

        case DSERR_NODRIVER:
			error = "No audio device found";
        break;

		case DSERR_OUTOFMEMORY:
			error = "DirectSound ran out of memory";
		break;

		case DSERR_PRIOLEVELNEEDED:
			error = "Caller doesn't have priority";
		break;

		case DSERR_UNSUPPORTED:
			error = "Function not supported (DirectX driver inferiority)";
		break;
	}
    _mmlog("Mikmod > drv_ds > %s : %s", function, error);
	return;
}


// =====================================================================================
    static BOOL DS_IsPresent(void)
// =====================================================================================
{
	HINSTANCE dsdll;
	int       ok;

	// Version check DSOUND.DLL
	ok = 0;
	dsdll = LoadLibrary("DSOUND.DLL");
	
    if (dsdll)
    {
		// DirectSound available.  Good.
        
        ok = 1;
        
        /*  Note: NT has latency problems, it seems.  We may want to uncomment
            this code and force NT users to drop back to winmm audio.
        
        OSVERSIONINFO ver;
		ver.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		GetVersionEx(&ver);
		switch (ver.dwPlatformId)
        {	case VER_PLATFORM_WIN32_NT:
                if (ver.dwMajorVersion > 4)
                {   // Win2K
                    ok = 1;
				} else
                {   // WinNT
                	ok = 0;
                }
		    break;

			default:
                // Win95 or Win98
                dsound_ok = 1;
		    break;
		}
        */

		FreeLibrary(dsdll);
	}
	
    return ok;
}


// =====================================================================================
    static BOOL DS_Init(MDRIVER *md, uint latency, void *optstr)
// =====================================================================================
{
    HRESULT       (WINAPI *DSoundCreate)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
    HRESULT       hres;
    DS_LOCALINFO *hwdata;
	//int i;
    
    hwdata = (DS_LOCALINFO *)_mm_calloc(md->allochandle, 1,sizeof(DS_LOCALINFO));

	// Load the dsound DLL
    
    hwdata->dll = LoadLibrary("DSOUND.DLL");
	
    if(!hwdata->dll)
    {   _mmlog("Mikmod > drv_ds > Failed loading DSOUND.DLL!");
        return 1;
    }

    // we safely assume that the user detected the dsound first.
    DSoundCreate = (void *)GetProcAddress(hwdata->dll, "DirectSoundCreate");

#ifdef _DEBUG
	AllocConsole();
#endif

    //SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS); //HIGH_PRIORITY_CLASS);
    //SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	
    hres = DSoundCreate(NULL,&hwdata->ds,NULL);
    if(hres != DS_OK)
    {   logerror("DSoundCreate",hres);
        return 1;
    }

	hwdata->latency=latency;

    if(!md->device.vc)
    {   md->device.vc = VC_Init();
        if(!md->device.vc) return 1;
    }

    md->device.local = hwdata;

	hwdata->nullbuf = 0;

    return 0;
}


// =====================================================================================
    static void DS_Exit(MDRIVER *md)
// =====================================================================================
{
	//int i;
    DS_LOCALINFO  *hwdata = md->device.local;
	

    if(hwdata)
    {   
		if(hwdata->dll)
        {   if(hwdata->bb) IDirectSoundBuffer_Stop(hwdata->bb);
    
            VC_Exit(md->device.vc);

            if(hwdata->bb) IDirectSoundBuffer_Release(hwdata->bb);
            if(hwdata->pb) IDirectSoundBuffer_Release(hwdata->pb);
            if(hwdata->ds) IDirectSound_Release(hwdata->ds);

            FreeLibrary(hwdata->dll);
        }

		if(hwdata->nullbuf)
			free(hwdata->nullbuf);

        _mm_free(md->allochandle, hwdata);
    }
}

static DWORD last;

// =====================================================================================
    static void DS_Update(MDRIVER *md)
// =====================================================================================
{
    DS_LOCALINFO  *hwdata = md->device.local;
    HRESULT        h;
    DWORD          play, write;
    DWORD          ptr1len, ptr2len;
    void          *ptr1, *ptr2;
    int            diff, errordiff, thistime;

    if(hwdata->dorefresh)
    {
	    IDirectSoundBuffer_GetCurrentPosition(hwdata->bb, &play, &write);

	    diff = (int)write-hwdata->lastwritecursor;
	    if(diff < 0) diff += (int)hwdata->bufsize;

	    hwdata->lastwritecursor = (int)write;

	    if(diff > hwdata->lag)
	    {
		    thistime = (int)timeGetTime();
		    errordiff = thistime - hwdata->lasterror;

            #ifdef _DEBUG
            {
            char xx[100];
	        DWORD crap;
            sprintf(xx,"%d>%d\n", diff, hwdata->lag);
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), xx, strlen(xx), &crap, 0);
            }
            #endif

		    if(errordiff < ERROR_TOLERANCE)
		    {
			    hwdata->lag += hwdata->samplesize * (LAG_INCREMENT);
			    if(hwdata->lag > (int)hwdata->bufsize)
			    {
                    // we needed more samples worth of lag than BUFFER_SIZE
                    // things are never going to work without a larger buffer
                    hwdata->lag = hwdata->bufsize;
			    }

		    }

		    hwdata->lasterror = thistime;

		    diff = (int)write-hwdata->lastwrite;
		    if(diff < 0)
			    diff += (int)hwdata->bufsize;

		    VC_WriteBytes(md,hwdata->nullbuf,diff);

		    diff = hwdata->lag;
		    hwdata->lastwrite = write;

	    }

	    if(diff>0)
	    {
		    h = IDirectSoundBuffer_Lock(hwdata->bb,hwdata->lastwrite, diff, &ptr1, &ptr1len, &ptr2, &ptr2len, 0);
            if(h==DSERR_BUFFERLOST)
            {
			    IDirectSoundBuffer_Restore(hwdata->bb);
			    h = IDirectSoundBuffer_Lock(hwdata->bb, hwdata->lastwrite, diff, &ptr1, &ptr1len, &ptr2, &ptr2len, 0);
		    }

            if(h==DS_OK)
            {   
			    
                VC_WriteBytes(md, ptr1,ptr1len);
                if(ptr2) VC_WriteBytes(md, ptr2,ptr2len);

                IDirectSoundBuffer_Unlock(hwdata->bb,ptr1,ptr1len,ptr2,ptr2len);
            }

		    hwdata->lastwrite += diff;
		    if(hwdata->lastwrite >= (int)hwdata->bufsize)
			    hwdata->lastwrite -= (int)hwdata->bufsize;
	    }
    }
}


// =====================================================================================
    static void DS_WipeBuffers(MDRIVER *md)
// =====================================================================================
{
    DS_LOCALINFO  *hwdata = md->device.local;

    if(hwdata->dorefresh)
    {   DWORD    ptr1len;
        void    *ptr1;
        HRESULT  h;

        h = IDirectSoundBuffer_Lock(hwdata->bb,0,0,&ptr1,&ptr1len, NULL, NULL, DSBLOCK_ENTIREBUFFER);
        if(h==DSERR_BUFFERLOST)
        {   IDirectSoundBuffer_Restore(hwdata->bb);
            h = IDirectSoundBuffer_Lock(hwdata->bb,0,0,&ptr1,&ptr1len, NULL, NULL, DSBLOCK_ENTIREBUFFER);
        }

        if(h==DS_OK)
        {   VC_SilenceBytes(md, ptr1, ptr1len);
            IDirectSoundBuffer_Unlock(hwdata->bb,ptr1,ptr1len,0,0);
        }
    }
}

// These need code!

// =====================================================================================
    static BOOL DS_SetMode(MDRIVER *md, uint mixspeed, uint mode, uint channels, uint cpumode)
// =====================================================================================
{
    DSBUFFERDESC   bf;
    WAVEFORMATEX   pcmwf;
    HRESULT        hres;
    uint           outrate;
	//HANDLE  thread;
	//int i;

    DS_LOCALINFO  *hwdata = md->device.local;

    if(!hwdata->ds) return 0;

    if(mixspeed)                 hwdata->mixspeed = mixspeed;
    if(hwdata->mixspeed > 44100) hwdata->mixspeed = 44100;

    if(!(mode & DMODE_DEFAULT)) hwdata->mode = mode;

    switch(channels)
    {
        case MD_MONO:
            hwdata->channels = 1;
        break;

        default:
            hwdata->channels = 2;
            channels   = MD_STEREO;
        break;
    }

    hres = hwdata->ds->lpVtbl->SetCooperativeLevel(hwdata->ds,GetForegroundWindow(),DSSCL_PRIORITY | ((hwdata->mode & DMODE_EXCLUSIVE) ? DSSCL_EXCLUSIVE : 0));

    if(hres != DS_OK)
    {   logerror("SetCooperativeLevel", hres);
        return 1;
    }

    // Weird code: Some dsound drivers don't like non-44100/22050/11025 khz settings,
    // so for now I force the primary to match one fo those three.

    if(hwdata->mixspeed <= 11025) outrate = 11025;
    else if(hwdata->mixspeed <= 22050) outrate = 22050;
    else if(hwdata->mixspeed <= 44100) outrate = 44100;

    memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
    pcmwf.wFormatTag      = WAVE_FORMAT_PCM;
    pcmwf.nChannels       = hwdata->channels;
    pcmwf.nSamplesPerSec  = outrate;
    pcmwf.wBitsPerSample  = (hwdata->mode&DMODE_16BITS) ? 16 : 8;
    pcmwf.nBlockAlign     = (pcmwf.wBitsPerSample*pcmwf.nChannels)/8;
    pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec*pcmwf.nBlockAlign;


	hwdata->samplesize = pcmwf.nBlockAlign;

	// set bufsize and round to nearest samplesize
	hwdata->bufsize  = ((BUFFER_SIZE) * hwdata->mixspeed * hwdata->samplesize) / 1000;
	hwdata->bufsize += (hwdata->samplesize - (hwdata->bufsize % hwdata->samplesize)) % hwdata->samplesize;

	// allocate scratch buffer for dumping lost samples
	if(hwdata->nullbuf) free(hwdata->nullbuf);
	hwdata->nullbuf = (UBYTE *)malloc(hwdata->bufsize * sizeof(UBYTE));

	// set write cursor lag and round to nearest samplesize
	hwdata->lag  = (MINIMUM_LATENCY * hwdata->mixspeed*hwdata->samplesize)/1000;
	hwdata->lag += (hwdata->samplesize-(hwdata->samplesize-hwdata->lag%hwdata->samplesize)) % hwdata->samplesize;

	//initial write cursors
	hwdata->currpos = hwdata->lag;
	hwdata->lastwrite = 0;
	hwdata->lastwritecursor = 0;

	//error tolerance timer
	hwdata->lasterror = 0;
	
        memset(&bf,0,sizeof(DSBUFFERDESC));
    bf.dwSize          = sizeof(DSBUFFERDESC);
    bf.dwFlags         = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE | ((hwdata->mode & DMODE_EXCLUSIVE) ? 0 : DSBCAPS_GLOBALFOCUS);
    bf.dwBufferBytes   = hwdata->bufsize;
    bf.lpwfxFormat     = &pcmwf;
    
    hres = IDirectSound_CreateSoundBuffer(hwdata->ds,&bf,&hwdata->bb,NULL);
    if(hres != DS_OK)
    {   logerror("CreateSoundBuffer (Secondary)",hres);
        return 1;
    }

    VC_SetMode(md->device.vc, hwdata->mixspeed, hwdata->mode, channels, cpumode);
   
    // Finally, start playing the buffer (and wipe it first)

	IDirectSoundBuffer_Play(hwdata->bb,0,0,DSBPLAY_LOOPING);
    hwdata->dorefresh = 1;
	DS_WipeBuffers(md);

    return 0;
}



// =====================================================================================
    static BOOL DS_SetSoftVoices(MDRIVER *md, uint num)
// =====================================================================================
{
    return VC_SetSoftVoices(md->device.vc, num);
}

// =====================================================================================
    static void DS_GetMode(MDRIVER *md, uint *mixspeed, uint *mode, uint *channels, uint *cpumode)
// =====================================================================================
{
    VC_GetMode(md->device.vc, mixspeed, mode, channels, cpumode);
}


MD_DEVICE drv_ds =
{
    "DirectSound",
    "DirectSound Driver (DX6) v0.5",
    0,VC_MAXVOICES,

    NULL,       // Linked list!
    NULL,
    NULL,

    // sample Loading
    VC_SampleAlloc,
    VC_SampleGetPtr,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,

    // Detection and initialization
    DS_IsPresent,
    DS_Init,
    DS_Exit,
    DS_Update,
    VC_Preempt,

    NULL,
    DS_SetSoftVoices,

    DS_SetMode,
    DS_GetMode,

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
    VC_VoiceRealVolume,

    DS_WipeBuffers
};


