/*  MikMod sound library
    (c) 1998-2005 Miodrag Vallat and others - see file AUTHORS for
    complete list.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
    02111-1307, USA.
    */

/*==============================================================================

  $Id$

  Driver for output on SDL platforms

  ==============================================================================*/

/* Written by Paul Spark <sparkynz74@gmail.com> */

#include <time.h>
#include "mikmod_internals.h"

#ifdef DRV_SDL

#include <memory.h>
#include <string.h>

#include "SDL.h"
#include "SDL_audio.h"


SDL_AudioSpec  g_AudioSpec;

int    g_Samples = 2048;

BYTE  *g_SampleBuffer            = NULL;
BYTE  *g_BigTuneBuffer           = NULL;
ULONG  g_BigTuneAvailW           = 0;
ULONG  g_BigTunePlayed           = 0;
ULONG  g_BigTuneBufferMax        = 512 * 1024 * 2 * 2;
ULONG  g_BigTuneUpdateNumSamples = 65536;
BOOL   g_BigTuneAvailStarted     = FALSE;
BOOL   g_WrapOccurred            = FALSE;
BOOL   g_WrapToZeroPending       = FALSE;
BOOL   g_Playing                 = FALSE;

void SDLSoundCallback( void *userdata, Uint8 *pbStream, int nDataLen );


//--------------------------------------------------------------------------------------------
// SetupSDLAudio()
//--------------------------------------------------------------------------------------------
BOOL SetupSDLAudio( void )
{
    g_AudioSpec.freq     = 44100;
    g_AudioSpec.format   = ( md_mode & DMODE_16BITS ) ? AUDIO_S16SYS : AUDIO_S8;
    g_AudioSpec.channels = ( md_mode & DMODE_STEREO ) ?  2 : 1;
    g_AudioSpec.silence  = 0;
    g_AudioSpec.samples  = g_Samples;
    g_AudioSpec.padding  = 0;
    g_AudioSpec.size     = 0;
    g_AudioSpec.userdata = 0;

    g_AudioSpec.callback = SDLSoundCallback;

    if( SDL_OpenAudio( &g_AudioSpec, NULL ) < 0 )
        return FALSE;

    SDL_PauseAudio( 0 );
    return TRUE;
}

//--------------------------------------------------------------------------------------------
// SDLSoundCallback()
//
// PDS: Requests 'len' bytes of samples - this is NOT the number of samples!!
//--------------------------------------------------------------------------------------------
void SDLSoundCallback( void *userdata, Uint8 *pbStream, int nDataLen )
{
    // PDS: Two channels, and 16 bit = 4x the number of samples..
    // PDS: Looks as though SDL knows how many bytes to ask for for stereo and 16 bit..
    int nSampleDataBytes = nDataLen;

    if( ( ! g_BigTuneAvailStarted ) || ( ! g_Playing ) )
    {
        memset( pbStream, 0, nSampleDataBytes );
        return;
    }

    // PDS: Get current chunk of samples from big buffer generated by MikMod..
    memcpy( pbStream, &g_BigTuneBuffer[ g_BigTunePlayed ], nSampleDataBytes );
    g_BigTunePlayed += nSampleDataBytes;

    if( ( g_WrapOccurred ) &&
        ( g_BigTunePlayed >= ( g_BigTuneBufferMax / 2 ) ) )
    {
        // PDS: Writer will have wrapped before we do.. but we clear its flag when reader reaches end of buffer
        //      and wraps also..
        g_WrapOccurred  = FALSE;
    }

    // PDS: Start playing from beginning of buffer again if end reached..
    if( g_BigTunePlayed >= g_BigTuneBufferMax )
    {
        g_BigTunePlayed = 0;
    }
}

//--------------------------------------------------------------------------------------------
// SDLDrv_CommandLine()
//--------------------------------------------------------------------------------------------
static void SDLDrv_CommandLine(CHAR *cmdline)
{
}

//--------------------------------------------------------------------------------------------
// SDLDrv_IsPresent()
//--------------------------------------------------------------------------------------------
static BOOL SDLDrv_IsPresent(void)
{
    return 1;
}

//--------------------------------------------------------------------------------------------
// SDLDrv_Init()
//--------------------------------------------------------------------------------------------
static BOOL SDLDrv_Init(void)
{
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
    {
        return FALSE;
    }

    // PDS: Set up callback etc
    SetupSDLAudio();

    // PDS: Set up big buffer which will get updated by MikMod Update function..
    g_BigTuneBuffer = (BYTE *) malloc( g_BigTuneBufferMax );
    g_BigTuneAvailW = 0;
    g_BigTunePlayed = 0;

    // initialize the library
    md_mode |= DMODE_SOFT_MUSIC;

    return VC_Init();
}

//--------------------------------------------------------------------------------------------
// SDLDrv_Exit()
//--------------------------------------------------------------------------------------------
static void SDLDrv_Exit( void )
{
    if( g_BigTuneBuffer )
    {
        free( g_BigTuneBuffer );
        g_BigTuneBuffer = NULL;
    }

    VC_Exit();
}

//--------------------------------------------------------------------------------------------
// SDLDrv_Update()
//--------------------------------------------------------------------------------------------
static void SDLDrv_Update( void )
{
    // PDS: Don't get any more data until reader clears our wrap flag..
    if( g_WrapOccurred )
        return;

    // PDS: Write/avail count should always be greater than played count in an ideal linear world..
    //      ..however, we have a circular buffer ..
    if( ( g_WrapOccurred ) &&
        ( g_BigTuneAvailW + g_BigTuneUpdateNumSamples >= g_BigTunePlayed ) &&
        ( g_BigTuneAvailStarted == TRUE ) )
    {
        return;
    }

    if( g_WrapToZeroPending )
    {
        // PDS: Wrap and continue..
        g_BigTuneAvailW     = 0;
        g_WrapToZeroPending = FALSE;
    }

    // PDS: Start writing data at start of buffer again..
    if( g_BigTuneAvailW + g_BigTuneUpdateNumSamples > g_BigTuneBufferMax )
    {
        // PDS: Reader/player will clear wrap flag when it wraps itself..
        g_WrapOccurred = TRUE;
        g_WrapToZeroPending = TRUE;
        return;
    }

    if( Player_Paused_internal() )
    {
        VC_SilenceBytes( (SBYTE *) &g_BigTuneBuffer[ g_BigTuneAvailW ], (ULONG) g_BigTuneUpdateNumSamples );
    }
    else
    {
        VC_WriteBytes( (SBYTE *) &g_BigTuneBuffer[ g_BigTuneAvailW ], (ULONG) g_BigTuneUpdateNumSamples );
    }

    g_BigTuneAvailW += g_BigTuneUpdateNumSamples;

    g_BigTuneAvailStarted = TRUE;
}

static void SDLDrv_PlayStop(void)
{
    g_Playing = FALSE;

    VC_PlayStop();
}

static BOOL SDLDrv_PlayStart(void)
{
    g_Playing = TRUE;
    return VC_PlayStart();
}

MIKMODAPI struct MDRIVER drv_sdl =
{
    NULL,
    "SDL",
    "SDL Driver v1.0",
    0,
    255,
    "sdl",
    "buffer:r:12,19,16:Audio buffer log2 size\n"
    "globalfocus:b:0:Play if window does not have the focus\n",

    SDLDrv_CommandLine,
    SDLDrv_IsPresent,

    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    SDLDrv_Init,
    SDLDrv_Exit,
    NULL,
    VC_SetNumVoices,
    SDLDrv_PlayStart,
    SDLDrv_PlayStop,
    SDLDrv_Update,
    NULL,
    VC_VoiceSetVolume,
    VC_VoiceGetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceGetFrequency,
    VC_VoiceSetPanning,
    VC_VoiceGetPanning,
    VC_VoicePlay,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceGetPosition,
    VC_VoiceRealVolume
};

#else

MISSING(drv_sdl);

#endif
