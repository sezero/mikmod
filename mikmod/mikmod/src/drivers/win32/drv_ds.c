/*           

Name:
DRV_DS.C

Author: Amoon/vanic a.k.a Steffen Rusitschka

Description:
Mikmod driver for output via DirectSound

Portability:

MSDOS:  BC(n)   Watcom(n)       DJGPP(n)
Win95:  (?)     y               n
Os2:    n
Linux:  n

(y) - yes
(n) - no (not possible or not useful)
(?) - may be possible, but not tested

*/

#include "mikmod.h"
#include <wtypes.h>
#include <windowsx.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <cguid.h>
#include "dsound.h"

char *TranslateDSError( HRESULT hr )
    {
    switch( hr )
    {
    case DSERR_ALLOCATED:
        return "DSERR_ALLOCATED";

    case DSERR_CONTROLUNAVAIL:
        return "DSERR_CONTROLUNAVAIL";

    case DSERR_INVALIDPARAM:
        return "DSERR_INVALIDPARAM";

    case DSERR_INVALIDCALL:
        return "DSERR_INVALIDCALL";

    case DSERR_GENERIC:
        return "DSERR_GENERIC";

    case DSERR_PRIOLEVELNEEDED:
        return "DSERR_PRIOLEVELNEEDED";

    case DSERR_OUTOFMEMORY:
        return "DSERR_OUTOFMEMORY";

    case DSERR_BADFORMAT:
        return "DSERR_BADFORMAT";

    case DSERR_UNSUPPORTED:
        return "DSERR_UNSUPPORTED";

    case DSERR_NODRIVER:
        return "DSERR_NODRIVER";

    case DSERR_ALREADYINITIALIZED:
        return "DSERR_ALREADYINITIALIZED";

    case DSERR_NOAGGREGATION:
        return "DSERR_NOAGGREGATION";

    case DSERR_BUFFERLOST:
        return "DSERR_BUFFERLOST";

    case DSERR_OTHERAPPHASPRIO:
        return "DSERR_OTHERAPPHASPRIO";

    case DSERR_UNINITIALIZED:
        return "DSERR_UNINITIALIZED";

    default:
        return "Unknown HRESULT";
    }
    }


/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>> "lolevel" DIRECTSOUND stuff <<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

typedef struct{
        UBYTE kick;                     /* =1 -> sample has to be restarted */
        UBYTE active;                   /* =1 -> sample is playing */
        UWORD flags;                    /* 16/8 bits looping/one-shot */
        SWORD handle;                   /* identifies the sample */
        ULONG start;                    /* start index */
        ULONG size;                     /* samplesize */
        ULONG reppos;                   /* loop start */
        ULONG repend;                   /* loop end */
        ULONG frq;                      /* current frequency */
        SLONG frqo;
        UBYTE vol;                      /* current volume */
        SWORD volo;
        UBYTE pan;                                              /* current panning position */
        SWORD pano;
        LPDIRECTSOUNDBUFFER samp;
        UBYTE myflags;
} GHOLD;

#define MF_ISDUPL 0x0001
#define MF_LOOPRESET 0x0002
#define MF_ISSWBUF 0x0004

GHOLD *ghld = NULL;
int   ds_voices;

UBYTE DS_BPM;
float RateInt;
float RateFloat;
float RateCount=0;


typedef struct {
  UINT wTimerID;
} MYDATA;
typedef MYDATA *LPMYDATA;

HWND hwnd;

MYDATA myData;

LPDIRECTSOUND lpDirectSound;
LPDIRECTSOUNDBUFFER lpDirectSoundBuffer;
LPDIRECTSOUNDBUFFER lpHwSamp[MAXSAMPLEHANDLES]={NULL};
LPDIRECTSOUNDBUFFER lpSwSamp[MAXSAMPLEHANDLES]={NULL};

#define TARGET_RESOLUTION 1  /* Try for 1-millisecond accuracy. */

TIMECAPS tc;
UINT     wTimerRes;

int freeHwBuffer()
{
  DSCAPS dscaps;
  HRESULT hr;
  dscaps.dwSize = sizeof(DSCAPS);
  hr = lpDirectSound->lpVtbl->GetCaps(lpDirectSound,&dscaps);
  if(DS_OK == hr) {
    return dscaps.dwFreeHwMixingAllBuffers;
  } else return 0;
}

BOOL AppCreateWritePrimaryBuffer(
    LPDIRECTSOUND lpDirectSound,
    LPDIRECTSOUNDBUFFER *lplpDsb,
    HWND hwnd)
{
    DSBUFFERDESC dsbdesc;
    HRESULT hr;
    WAVEFORMATEX pcmwf;

    // Set up wave format structure.
    memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
    pcmwf.wFormatTag = WAVE_FORMAT_PCM;
    pcmwf.nChannels = 2;
    pcmwf.nSamplesPerSec = 44100;
    pcmwf.nBlockAlign = 4;
    pcmwf.nAvgBytesPerSec =
        pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
    pcmwf.wBitsPerSample = 16;
    // Set up DSBUFFERDESC structure.
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbdesc.dwBufferBytes = 0; // Buffer size is determined
                               // by sound hardware.
    dsbdesc.lpwfxFormat = NULL; // Must be NULL for primary buffers.
    // Obtain write-primary cooperative level.
    hr = lpDirectSound->lpVtbl->SetCooperativeLevel(lpDirectSound,
        hwnd, DSSCL_PRIORITY);
    if(DS_OK == hr) {
        // Succeeded! Try to create buffer.
        hr = lpDirectSound->lpVtbl->CreateSoundBuffer(lpDirectSound,
            &dsbdesc, lplpDsb, NULL);
        if(DS_OK == hr) {
            // Succeeded! Set primary buffer to desired format.
            hr = (*lplpDsb)->lpVtbl->SetFormat(*lplpDsb, &pcmwf);
            (*lplpDsb)->lpVtbl->Play(*lplpDsb,0,0,DSBPLAY_LOOPING);
            return TRUE;
        }
    }
    // If we got here, then we failed SetCooperativeLevel.
    // CreateSoundBuffer, or SetFormat.
    *lplpDsb = NULL;
    return FALSE;
}

BOOL WriteDataToBuffer(
    LPDIRECTSOUNDBUFFER lpDsb,
    LPBYTE lpbSoundData,
    DWORD dwSoundBytes)
{
    LPVOID lpvPtr1;
    DWORD dwBytes1;
    LPVOID lpvPtr2;
    DWORD dwBytes2;
    HRESULT hr;
    // Obtain write pointer.
    hr = lpDsb->lpVtbl->Lock(lpDsb, 0, dwSoundBytes, &lpvPtr1,
        &dwBytes1, &lpvPtr2, &dwBytes2, 0);

    // If we got DSERR_BUFFERLOST, restore and retry lock.
    if(DSERR_BUFFERLOST == hr) {
        lpDsb->lpVtbl->Restore(lpDsb);
        hr = lpDsb->lpVtbl->Lock(lpDsb, 0, dwSoundBytes, &lpvPtr1,
             &dwBytes1, &lpvPtr2, &dwBytes2, 0);
    }
    if(DS_OK == hr) {
        // Write to pointers.
        CopyMemory(lpvPtr1, lpbSoundData, dwBytes1);
        if(NULL != lpvPtr2) {
            CopyMemory(lpvPtr2, lpbSoundData+dwBytes1, dwBytes2);
        }
        // Release the data back to DirectSound.
        hr = lpDsb->lpVtbl->Unlock(lpDsb, lpvPtr1, dwBytes1, lpvPtr2,
            dwBytes2);
        if(DS_OK == hr) {
            // Success!
            return TRUE;
        }
    }
    // If we got here, then we failed Lock, Unlock, or Restore.
    return FALSE;
}

BOOL LoadSamp(LPDIRECTSOUND lpDirectSound,
              LPDIRECTSOUNDBUFFER *lplpDsb,
              LPBYTE samp, UINT length, UINT flags)
{
    DSBUFFERDESC dsbdesc;
    HRESULT hr;
    WAVEFORMATEX pcmwf;
    //UINT temp;

    // Set up wave format structure.
    memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
    pcmwf.wFormatTag = WAVE_FORMAT_PCM;
    pcmwf.nChannels = 1;
    pcmwf.nSamplesPerSec = 44100;
    pcmwf.nBlockAlign = 2;
    pcmwf.nAvgBytesPerSec =
        pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
    pcmwf.wBitsPerSample = 16;
    // Set up DSBUFFERDESC structure.
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_CTRLPAN|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLFREQUENCY|
                      DSBCAPS_GLOBALFOCUS|DSBCAPS_GETCURRENTPOSITION2|flags;
    dsbdesc.dwBufferBytes = length;
    dsbdesc.lpwfxFormat = &pcmwf;
    hr = lpDirectSound->lpVtbl->CreateSoundBuffer(lpDirectSound,
        &dsbdesc, lplpDsb, NULL);
    if(hr == DS_OK) {
//        lpDirectSound->lpVtbl->SetCooperativeLevel(
//                lpDirectSound,hwnd, DSSCL_EXCLUSIVE);
        // Succeeded! Valid interface is in *lplpDsb.
        WriteDataToBuffer(*lplpDsb,samp,length);
//        lpDirectSound->lpVtbl->SetCooperativeLevel(
//                lpDirectSound,hwnd, DSSCL_NORMAL);
    } else {
//      printf("%s\n",TranslateDSError(hr));
//      getch();
      *lplpDsb=NULL;
    }
    return 1;
}

void DS_Update(void);

void PASCAL TimerHandler(UINT wTimerID, UINT msg,
    DWORD dwUser, DWORD dw1, DWORD dw2)
{
    RateCount+=RateInt;
    if (RateCount > RateFloat)
    {   DS_Update();
        RateCount -= RateFloat;
    }

    UNREFERENCED_PARAMETER(dw1);
    UNREFERENCED_PARAMETER(dw2);
    UNREFERENCED_PARAMETER(dwUser);
    UNREFERENCED_PARAMETER(msg);
    UNREFERENCED_PARAMETER(wTimerID);
}


void myTimerInit(void)
{
    if(timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
    {   // Error; application can't continue.
    }

    wTimerRes = min(max(tc.wPeriodMin, TARGET_RESOLUTION), tc.wPeriodMax);

    timeBeginPeriod(wTimerRes);
}

UINT myTimerSet(LPMYDATA lpmyData,         // Sequencer data
                float msInterval)          // Event interval
{
    RateFloat=msInterval;
    RateInt=(int)msInterval;
    timeKillEvent(lpmyData->wTimerID);
    lpmyData->wTimerID = timeSetEvent(
         RateInt,                         // Delay
         wTimerRes,                       // Resolution (global variable)
         (LPTIMECALLBACK) TimerHandler,   // Callback function
         (DWORD) lpmyData,                // User data
         TIME_PERIODIC);                  // Event type


    if (lpmyData->wTimerID != 0)
        return TIMERR_NOCANDO;
    else
        return TIMERR_NOERROR;
}

VOID myTimerDone(LPMYDATA lpmyData)
{
    timeKillEvent(lpmyData->wTimerID);
    timeEndPeriod(wTimerRes);
}


/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>> The actual DIRECTSOUND driver <<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

static UINT     wUpdCount=0;
static UINT     wUpdSkip=3;
static UINT     LoopSamples=6*1024;

static SWORD voltab[65];
static SWORD pantab[256];

SWORD lin2log(double i)
{
    /* 1..64 -> 0..10000 logarithmic */
    return (10000.0/6.0)*log(i)/log(2.0);
}

SWORD volconv(SWORD vol)
{
    if (!vol) return -10000; else
      return -2000+lin2log(vol)*2000/10000-1000;
//      return -1000-(64-vol)*32;
}

SWORD panconv(SWORD pan)
{
    pan-=128;
    if (pan==0) return 0; else
    if (pan>100) return 10000; else
    if (pan<-100) return -10000; else
    if (pan>0) return 2000-lin2log((99-pan)*63/99+1)*2000/10000; else
        return -2000+lin2log((100+pan)*63/100+1)*2000/10000;
}

void DSSetBPM(UWORD bpm)
{
    /* The player routine has to be called (bpm*50)/125 times a second,
       so the interval between calls takes 125/(bpm*50) seconds (amazing!).
    */
    float rate=125.0*1000.0/(bpm*50*wUpdSkip);

    myTimerSet(&myData,rate);
}



void DS_Update(void)
{
    UBYTE t;
    GHOLD *aud;
    LPDIRECTSOUNDBUFFER lps;

    DWORD status;
    DWORD pos,posp;

    wUpdCount = (wUpdCount+1)%wUpdSkip;

    if (wUpdCount==0)
    {   md_player();

        for(t=0; t<ds_voices; t++)
        {   aud = &ghld[t];
            if ((aud->samp)&&(aud->flags&SF_LOOP))
            {   aud->samp->lpVtbl->GetCurrentPosition(aud->samp,&pos,&posp);
                pos>>=1;
                if (pos>aud->repend+(LoopSamples>>1))
                {   if (!(aud->myflags&MF_ISSWBUF))
                        aud->samp->lpVtbl->SetVolume(aud->samp,-10000);
                    aud->myflags|=MF_LOOPRESET;
                }
            }

            if((aud->samp)&&(aud->kick)&&(!(aud->myflags&MF_ISSWBUF)))
            {   aud->samp->lpVtbl->SetVolume(aud->samp,-10000);
            }
        }
        return;
    } else if (wUpdCount==1)
    {   if(DS_BPM != md_bpm)
        {    DSSetBPM(md_bpm);
             DS_BPM = md_bpm;
        }

        for(t=0; t<ds_voices; t++)
        {   aud = &ghld[t];

            if((aud->samp) && (aud->flags & SF_LOOP))
            {   if (aud->myflags & MF_LOOPRESET)
                {   aud->samp->lpVtbl->GetCurrentPosition(aud->samp,&pos,&posp);
                    pos>>=1;
                    pos=((pos-aud->reppos) % (aud->repend-aud->reppos)) + aud->reppos;
                    aud->samp->lpVtbl->SetCurrentPosition(aud->samp,pos<<1);
                    if (!(aud->myflags&MF_ISSWBUF))
                        aud->volo = -1;  // set volume
                }
            }

            if(aud->samp)
            {   aud->samp->lpVtbl->GetStatus(aud->samp,&status);
                if (!(status&DSBSTATUS_PLAYING))
                {   if (aud->myflags&MF_ISDUPL)
                    {   aud->samp->lpVtbl->Release(aud->samp);
                    }
                    aud->samp = NULL;
                    aud->myflags &= ~MF_ISSWBUF;
                 }
            }

            if(aud->kick)
            {   if (aud->samp)
                {   aud->samp->lpVtbl->Stop(aud->samp);
                    if (aud->myflags & MF_ISDUPL) aud->samp->lpVtbl->Release(aud->samp);
                    aud->samp = NULL;
                    aud->myflags &= ~MF_ISSWBUF;
                }

                if(lpHwSamp[aud->handle])
                {   lpHwSamp[aud->handle]->lpVtbl->GetStatus(lpHwSamp[aud->handle],&status);
                    if(status&DSBSTATUS_PLAYING)
                    {   if (freeHwBuffer() > 0)
                        {   lpDirectSound->lpVtbl->DuplicateSoundBuffer(lpDirectSound,lpHwSamp[aud->handle],&aud->samp);
                            aud->myflags|=MF_ISDUPL;
                        }
                    } else
                    {   aud->samp = lpHwSamp[aud->handle];
                        aud->myflags &= ~MF_ISDUPL;
                    }
                }

                if((!aud->samp) && (lpSwSamp[aud->handle]))
                {   lpSwSamp[aud->handle]->lpVtbl->GetStatus(lpSwSamp[aud->handle],&status);
                    if(status&DSBSTATUS_PLAYING)
                    {   lpDirectSound->lpVtbl->DuplicateSoundBuffer(lpDirectSound,lpSwSamp[aud->handle],&aud->samp);
                        aud->myflags|=MF_ISDUPL;
                    } else
                    {   aud->samp = lpSwSamp[aud->handle];
                        aud->myflags &= ~MF_ISDUPL;
                    }
                    aud->myflags |= MF_ISSWBUF;
                }

                lps = aud->samp;

                if(lps)
                {   lps->lpVtbl->SetCurrentPosition(lps,(aud->flags&SF_16BITS)?aud->start:aud->start<<1);
                }
                aud->frqo = aud->pano = aud->volo = -1;
            }

            lps = aud->samp;
            if(lps)
            {   if (aud->frq!=aud->frqo)
                    lps->lpVtbl->SetFrequency(lps,aud->frq);
                if (aud->vol!=aud->volo)
                    lps->lpVtbl->SetVolume(lps,voltab[aud->vol]);
                if (aud->pan!=aud->pano)
                    lps->lpVtbl->SetPan(lps,pantab[aud->pan]);
                if (aud->kick)
                    lps->lpVtbl->Play(lps,0,0,0);
            }

            aud->kick = 0;
            aud->myflags &= ~MF_LOOPRESET;
            aud->pano = aud->pan;
            aud->volo = aud->vol;
            aud->frqo = aud->frq;
//            printf("%i ",aud->myflags);
        }
    }
}


SWORD DS_SampleLoad(SAMPLOAD *sload, int type)
{
    SAMPLE *s = sload->sample;
    int    handle, t;
    long   length, loopstart, loopend;
    SWORD  *lpSampMem;
    ULONG  lengthAlloc;
    ULONG  pos;

    // Find empty slot to put sample address in

    for(handle=0; handle<MAXSAMPLEHANDLES; handle++)
    {   if(lpSwSamp[handle]==NULL) break;
    }

    if(handle==MAXSAMPLEHANDLES)
    {   _mm_errno = MMERR_OUT_OF_HANDLES;
        return -1;
    }

    length    = s->length;
    loopstart = s->loopstart;
    loopend   = s->loopend;

    lengthAlloc = (length<<1) + 4;
    if(s->flags & SF_LOOP)
      lengthAlloc += LoopSamples<<1;  // for copying the loop behind the sample

    if(!(lpSampMem=_mm_malloc(lengthAlloc)))
    {   _mm_errno = MMERR_SAMPLE_TOO_BIG;
        return -1;
    }

//    printf("Handle=%i Free=%i\n",handle,freeHwBuffer());

    // Load the sample

    if(!(s->flags & SF_LOOP)) loopstart = 0;

    SL_SampleSigned(sload);
    SL_Sample8to16(sload);
    SL_Load(lpSampMem,sload,length);

    if (!(s->flags & SF_LOOP))
    {   lpSampMem[length]   = 0;
        lpSampMem[length+1] = 0;
    } else
    {   pos = loopstart;
        for (t=loopend; t<lengthAlloc/2; t++)
        {   lpSampMem[t] = lpSampMem[pos];
            pos++;
            if (pos == loopend)
                pos = loopstart;
        }
    }

    if(freeHwBuffer() > 0)
    {   LoadSamp(lpDirectSound, &lpHwSamp[handle],(LPBYTE)lpSampMem, lengthAlloc, DSBCAPS_LOCHARDWARE);
    } else
        lpHwSamp[handle] = 0;

    LoadSamp(lpDirectSound, &lpSwSamp[handle],(LPBYTE)lpSampMem, lengthAlloc, DSBCAPS_LOCSOFTWARE);

    free(lpSampMem);
    return handle;
}


void DS_UnLoad(SWORD handle)
/*
    callback routine to unload samples

    smp                     :sampleinfo of sample that is being freed
*/
{
    if (lpHwSamp[handle]) {
      lpHwSamp[handle]->lpVtbl->Stop(lpHwSamp[handle]);
      lpHwSamp[handle]->lpVtbl->Release(lpHwSamp[handle]);
    }
    if (lpSwSamp[handle]) {
      lpSwSamp[handle]->lpVtbl->Stop(lpSwSamp[handle]);
      lpSwSamp[handle]->lpVtbl->Release(lpSwSamp[handle]);
    }
    lpHwSamp[handle]=NULL;
    lpSwSamp[handle]=NULL;
}



BOOL DS_Init(void)
{
    int t;
    HRESULT hr;
    DSCAPS  dscaps;
/*    for (t=0; t<256; t+=16) printf("%i:%i\n",t,panconv(t));*/

    hwnd = GetForegroundWindow();
    if (FAILED(CoInitialize(NULL))) return 1;
    hr = CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDirectSound, (void**)&lpDirectSound);
    if(!FAILED(hr))
    {   hr = IDirectSound_Initialize(lpDirectSound, NULL);
        if(DS_OK == hr)
        {   AppCreateWritePrimaryBuffer(lpDirectSound, &lpDirectSoundBuffer,hwnd);

//        lpDirectSound->lpVtbl->SetCooperativeLevel(
//                lpDirectSound,hwnd, DSSCL_PRIORITY);

        dscaps.dwSize = sizeof(DSCAPS);
        hr = lpDirectSound->lpVtbl->GetCaps(lpDirectSound,&dscaps);
        if(DS_OK == hr) {
            printf("Hardware Buffers:      %i\n",dscaps.dwMaxHwMixingAllBuffers);
            printf("Hardware Memory:       %iK\n",dscaps.dwTotalHwMemBytes/1024);
            printf("Hardware Free Memory:  %iK\n",dscaps.dwFreeHwMemBytes/1024);
        } 
      }
    }

    for (t=0; t<256; t++) pantab[t] = panconv(t);
    for (t=0; t<65; t++) voltab[t]  = volconv(t);

    return 0;
}



void DS_Exit(void)
{
    if (lpDirectSoundBuffer) {
      lpDirectSoundBuffer->lpVtbl->Stop(lpDirectSoundBuffer);
      lpDirectSoundBuffer->lpVtbl->Release(lpDirectSoundBuffer);
    }
    if (lpDirectSound) {
      lpDirectSound->lpVtbl->Release(lpDirectSound);
    }
    CoUninitialize();
    lpDirectSound=NULL;
}


BOOL DS_SetNumVoices(void)
{
    if(ghld) free(ghld);
    if((ds_voices = md_hardchn) != 0)
    {   if((ghld = _mm_calloc(sizeof(GHOLD),ds_voices)) == NULL) return 1;
    } else
    {   ghld = NULL;
    }    

    return 0;
}


void DS_PlayStart(void)
{
    int t;

    for(t=0; t<ds_voices; t++)
    {   ghld[t].flags   = 0;
        ghld[t].handle  = 0;
        ghld[t].kick    = 0;
        ghld[t].active  = 1;
        ghld[t].frq     = 10000;
        ghld[t].vol     = 0;
        ghld[t].pan     = (t&1) ? 0:255;
        ghld[t].samp    = NULL;
    }

    myTimerInit();
    DS_BPM = 125;
    DSSetBPM(125);
}



void DS_PlayStop(void)
{
    UINT t;
    GHOLD *aud;
    myTimerDone(&myData);
    for (t=0; t<ds_voices; t++)
    {   aud = &ghld[t];
        if(aud->samp)
        {   aud->samp->lpVtbl->Stop(aud->samp);
            if(aud->myflags & MF_ISDUPL)
                aud->samp->lpVtbl->Release(aud->samp);
            aud->samp = NULL;
        }
    }
}


BOOL DS_IsThere(void)
{
    if(DS_OK == DirectSoundCreate(NULL, &lpDirectSound, NULL))
    {   lpDirectSound->lpVtbl->Release(lpDirectSound);
        return 1;
    } else return 0;
}


void DS_VoiceSetVolume(int voice,int vol)
{
    ghld[voice].vol = vol;
}


void DS_VoiceSetFrequency(int voice,ULONG frq)
{
    ghld[voice].frq = frq;
}

void DS_VoiceSetPanning(int voice, int pan)
{
    if(pan == PAN_SURROUND) pan = 128;
    ghld[voice].pan = ((pan/2) + 128) & 0xff;
}

void DS_VoicePlay(int voice, SWORD handle, int start, ULONG size, ULONG reppos, ULONG repend, UWORD flags)
{
    if(start >= size) return;

    if(flags & SF_LOOP)
    {   if(repend > size) repend = size;
    }

    ghld[voice].flags  = flags;
    ghld[voice].handle = handle;
    ghld[voice].start  = start;
    ghld[voice].size   = size;
    ghld[voice].reppos = reppos;
    ghld[voice].repend = repend;
    ghld[voice].active = 1;
    ghld[voice].kick   = 1;
}

ULONG DS_SampleSpace(int type)
{
    return  1024; //(type==MD_HARDWARE) ? (UltraMemTotal()>>10)-16 : VC_SampleSpace(type);
}


ULONG DS_SampleLength(int type, SAMPLE *s)
{
    return (s->length * 2) + ((s->loopend - s->loopstart) * 2) + 32;
}


void DS_VoiceStop(int voice)
{
    ghld[voice].active = 0;
}


BOOL DS_VoiceStopped(int voice)
{
    return ghld[voice].active;
}


void DS_VoiceReleaseSustain(int voice)
{

}


ULONG DS_VoiceGetPosition(int voice)
{
    return 0;
}


ULONG DS_VoiceRealVolume(int voice)
{
    return 0;
}


void DS_Dummy(void)
{
}




MDRIVER drv_ds =
{   "DirectSound",
    "DirectSound Driver v0.30 - by Amoon, rusi@mathematik.uni-marburg.de",
    30,
    0,
    NULL,

    DS_IsThere,
    DS_SampleLoad,
    DS_UnLoad,
    DS_SampleSpace,
    DS_SampleLength,
    DS_Init,
    DS_Exit,
    NULL,
    DS_SetNumVoices,
    NULL,
    DS_PlayStart,
    DS_PlayStop,
    DS_Dummy,
    DS_VoiceSetVolume,
    DS_VoiceSetFrequency,
    DS_VoiceSetPanning,
    DS_VoicePlay,
    DS_VoiceStop,
    DS_VoiceStopped,
    DS_VoiceReleaseSustain,
    DS_VoiceGetPosition,
    DS_VoiceRealVolume    
};
