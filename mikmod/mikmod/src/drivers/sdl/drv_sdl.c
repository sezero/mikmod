/*
  
Name:
drv_sdl.c
by Jan Lönnberg
lonnberg@mbnet.fi

Adapted from drv_dx6.c
by matthew gambrell
zeromus@verge-rpg.com

Description:
Mikmod driver for output via SDL (Simple DirectMedia Layer)

Portability:

MSDOS:  N/A
Win95:  BC(?) Watcom(?) MSVC6(?) GCC(y)
Linux:  GCC(y)
BeOS:   GCC(?)
MacOS:  GCC(?)
Unix:   GCC(?)

(y) - yes
(n) - no (not possible or not useful)
(?) - may be possible, but not tested

Notes: SDL's sound handling is based on a callback function that fills the sound
buffer, while MikMod seems to use a (more or less) regular interrupt that fills the
sound buffer. For this reason, this driver has its own buffer, which it fills just
like the DirectSound driver, and sends to SDL when requested.

Not ideal, but it works. When I get the bugs out of this version I'll see if I can
do something more elegant, such as having the callback call VC_WriteBytes (which
ought to decrease latency further).

*/

#include "mikmod.h"
#include "virtch.h"

#include "SDL.h"

#include <stdio.h>

static void sdl_PlayStop(MDRIVER *md);
static void sdl_WipeBuffers(MDRIVER *md);
void sdl_FillAudio(void *userdata, Uint8 *stream, int len);

typedef struct MMSDL_LOCALINFO
{
	uint  sdlMode;
	uint  sdlMixspeed;
	uint  sdlChannels;

	uint requested_latency;

	uint dorefresh;

	Uint8 *sdlBuffer;
	uint sdlBufferLength;

	volatile uint CurrBufPlayPos,CurrBufWritePos,CurrBufContent;
} MMSDL_LOCALINFO;

// =====================================================================================
static BOOL sdl_IsPresent(void)
// =====================================================================================
{
  /* SDL doesn't have a decent way of checking whether it's available (doing so in
     a portable fashion is non-trivial, especially when you don't know what linking
     method has been used).
  */
  return 1;
}

// =====================================================================================
static BOOL sdl_Init(MDRIVER *md, uint latency, void *optstr)
// =====================================================================================
{
  MMSDL_LOCALINFO *hwdata;

  hwdata = (MMSDL_LOCALINFO *)_mm_calloc(md->allochandle, 1,sizeof(MMSDL_LOCALINFO));

  hwdata->sdlMode = DMODE_16BITS | DMODE_INTERP | DMODE_NOCLICK;
  hwdata->sdlMixspeed = 44100;
  hwdata->sdlChannels = 2;

  hwdata->dorefresh=0;

  hwdata->sdlBuffer=NULL;

  if((SDL_Init(SDL_INIT_AUDIO|SDL_INIT_NOPARACHUTE)==-1)) {
    _mmlog("Mikmod > drv_sdl > Failed to initialise SDL.");
    return 1;
  }

  /* Audio callback setup in sdl_SetMode. */

  hwdata->requested_latency=latency;

  if(!md->device.vc)
    {   md->device.vc = VC_Init();
        if(!md->device.vc) return 1;
    }

  md->device.local = hwdata;

  return 0;
}


// =====================================================================================
static void sdl_Exit(MDRIVER *md)
// =====================================================================================
{
  MMSDL_LOCALINFO  *hwdata = md->device.local;

  if (hwdata==NULL) return;

  hwdata->dorefresh = 0;

  free(hwdata->sdlBuffer);
    
  VC_Exit(md->device.vc);

  SDL_CloseAudio();

  _mm_free(md->allochandle, hwdata);
}

// =====================================================================================
void sdl_FillAudio(void *userdata, Uint8 *stream, int len)
// =====================================================================================
{
  MMSDL_LOCALINFO  *hwdata = ((MDRIVER *)userdata)->device.local;

  /* We require that sdl_Update is called at least every requested_latency
     milliseconds. If this fails, we end up playing silence. */

  if (hwdata->dorefresh==0) return;

  /* Only play if we have data left */
  if (hwdata->CurrBufContent == 0 )
    return;

  /* Mix as much data as possible */
  len = ( len > hwdata->CurrBufContent ? hwdata->CurrBufContent : len );
  /*  SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME); */
  if (len+hwdata->CurrBufPlayPos<hwdata->sdlBufferLength)
    memcpy(stream,hwdata->sdlBuffer+hwdata->CurrBufPlayPos,len); /* Much faster... */
  else
    {
      memcpy(stream,hwdata->sdlBuffer+hwdata->CurrBufPlayPos,hwdata->sdlBufferLength-hwdata->CurrBufPlayPos);
      memcpy(stream,hwdata->sdlBuffer,len-(hwdata->sdlBufferLength-hwdata->CurrBufPlayPos));
    }
  hwdata->CurrBufPlayPos += len;
  hwdata->CurrBufPlayPos&=(hwdata->sdlBufferLength-1); /* Power of two, remember? */
  hwdata->CurrBufContent -= len;
}

// =====================================================================================
static void sdl_Update(MDRIVER *md)
// =====================================================================================
{
  MMSDL_LOCALINFO  *hwdata = md->device.local;

  /* Feed data into a DirectSound-style buffer... */

  if(hwdata->dorefresh)
    {
      int      diff;
      uint     ptr1len, ptr2len;
      Uint8    *ptr1, *ptr2;
      uint     written;

      diff = hwdata->sdlBufferLength-hwdata->CurrBufContent;
      
      if(diff!=0)
        {
	  ptr1=hwdata->sdlBuffer+hwdata->CurrBufWritePos;
	  if (hwdata->CurrBufWritePos+diff>=hwdata->sdlBufferLength)
	    {
	      ptr1len=hwdata->sdlBufferLength-hwdata->CurrBufWritePos;
	      ptr2=hwdata->sdlBuffer;
	      ptr2len=diff-ptr1len;
	      hwdata->CurrBufWritePos+=ptr1len;
	    }
	  else
	    {
	      ptr1len=diff;
	      ptr2=NULL;
	      ptr2len=0;
	      hwdata->CurrBufWritePos=0;
	    }

	  written=VC_WriteBytes(md, ptr1,ptr1len);

	  SDL_LockAudio(); /* Just to be thread-safe... */
	  hwdata->CurrBufContent+=written;
	  SDL_UnlockAudio();

	  if ((ptr2!=NULL)&&(written==ptr1len))
	    {	      
	      written=VC_WriteBytes(md, ptr2,ptr2len);

	      SDL_LockAudio(); /* Just to be thread-safe... */
	      hwdata->CurrBufContent+=written;
	      SDL_UnlockAudio();

	      hwdata->CurrBufWritePos=ptr2len;
	    }
	}
      else return;
    }
}


// =====================================================================================
    static void sdl_WipeBuffers(MDRIVER *md)
// =====================================================================================
{
  MMSDL_LOCALINFO  *hwdata = md->device.local;

    if(hwdata->dorefresh)
    {
      /* We can't do anything about the data that's been sent to SDL, so we
	 wipe the driver's buffer. */
      if (hwdata->sdlMode&DMODE_16BITS)
	memset(hwdata->sdlBuffer,0,hwdata->sdlBufferLength); /* Always signed. */
      else
	memset(hwdata->sdlBuffer,0x80,hwdata->sdlBufferLength); /* Always unsigned. */
    }
}

// =====================================================================================
static BOOL sdl_SetMode(MDRIVER *md, uint mixspeed, uint mode, uint channels, uint cpumode)
// =====================================================================================
{
  MMSDL_LOCALINFO  *hwdata = md->device.local;

  SDL_AudioSpec wanted;

  uint requested_samples;
  uint bits=0;

  /* SDL can happily emulate any sound format we like, so we simply force the
     user's selection. */
  
  if(!(mode & DMODE_DEFAULT)) hwdata->sdlMode = mode;
  if (mixspeed>0) hwdata->sdlMixspeed=mixspeed;

  switch(channels)
    {
    case MD_MONO:
      hwdata->sdlChannels = 1;
      break;
      
    default:
      hwdata->sdlChannels = 2;
      channels   = MD_STEREO;
      break;

      /* SDL doesn't support 4 channel sound; defaulting to 2. */
    }
  
  /* Set the audio format... */
  wanted.freq = (int)hwdata->sdlMixspeed;
  /* MikMod uses unsigned 8 bits or signed 16 bits (system byte order). */
  wanted.format = (mode&DMODE_16BITS)?AUDIO_S16SYS:AUDIO_U8;
  wanted.channels = hwdata->sdlChannels;
  /* Adjust buffer size to requested latency (must be a power of two, so
     we round upwards to prevent sound from breaking up)... */
  requested_samples = hwdata->requested_latency*hwdata->sdlMixspeed/1000;
  while(requested_samples>0)
    {
      requested_samples>>=1;
      bits++;
    }
  if (bits>15) bits=15;
  if (bits<6) bits=6;
  wanted.samples=1<<bits;
  wanted.callback = sdl_FillAudio;
  wanted.userdata = md;

  hwdata->sdlBuffer=malloc(hwdata->sdlBufferLength=
		   sizeof(Uint8)*wanted.samples*
		   hwdata->sdlChannels*((mode&DMODE_16BITS)?2:1));

  if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
    _mmlog("Mikmod > drv_sdl > Failed to set SDL sound format.");
    return(1);
  }
  
  VC_SetMode(md->device.vc,hwdata->sdlMixspeed, hwdata->sdlMode, hwdata->sdlChannels, cpumode);
    
  // Finally, start playing the buffer (and wipe it first)...

  hwdata->dorefresh=1;

  sdl_WipeBuffers(md);
  
  hwdata->CurrBufPlayPos = hwdata->CurrBufWritePos = hwdata->CurrBufContent=0;
  
  SDL_PauseAudio(0);

  return 0;
}

// =====================================================================================
    static BOOL sdl_SetSoftVoices(MDRIVER *md, uint num)
// =====================================================================================
{
    return VC_SetSoftVoices(md->device.vc, num);
}

// =====================================================================================
    static void sdl_GetMode(MDRIVER *md, uint *mixspeed, uint *mode, uint *channels, uint *cpumode)
// =====================================================================================
{
    VC_GetMode(md->device.vc, mixspeed, mode, channels, cpumode);
}

MD_DEVICE drv_sdl =
{
    "SDL",
    "SDL 1.2 Driver v0.2",
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
    sdl_IsPresent,
    sdl_Init,
    sdl_Exit,
    sdl_Update,
    VC_Preempt,

    NULL,
    sdl_SetSoftVoices,

    sdl_SetMode,
    sdl_GetMode,

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

    sdl_WipeBuffers
};
