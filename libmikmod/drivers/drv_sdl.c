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

/*
  Initially written by Paul Spark <sparkynz74@gmail.com>
  Rewrite/major fixes by O. Sezer <sezero@users.sourceforge.net>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_SDL

#include <string.h>
#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#include <SDL/SDL.h>
#else
#include "SDL.h"
#endif

static SDL_AudioSpec g_AudioSpec;
static BOOL g_Playing = 0;

static void SDLSoundCallback(void *userdata, Uint8 *stream, int len)
{
    MUTEX_LOCK(vars);
    if (Player_Paused_internal() || !g_Playing) {
        VC_SilenceBytes((SBYTE *) stream, (ULONG)len);
    }
    else
    {
        /* PDS: len bytes of samples: _not_ the number of samples */
        int got = (int) VC_WriteBytes((SBYTE *) stream, (ULONG)len);
        if (got < len) {	/* fill the rest with silence, then */
            VC_SilenceBytes((SBYTE *) &stream[got], (ULONG)(len-got));
        }
    }
    MUTEX_UNLOCK(vars);
}

static BOOL SetupSDLAudio(void)
{
    g_AudioSpec.freq     = md_mixfreq;
    g_AudioSpec.format   =
#if (SDL_MAJOR_VERSION >= 2)
                           (md_mode & DMODE_FLOAT)  ? AUDIO_F32SYS :
#endif
                           (md_mode & DMODE_16BITS) ? AUDIO_S16SYS : AUDIO_U8;
    g_AudioSpec.channels = (md_mode & DMODE_STEREO) ? 2 : 1;
    g_AudioSpec.samples = 2048; /* a fair default */
    if (g_AudioSpec.freq > 44100) g_AudioSpec.samples = 4096; /* 48-96 kHz? */
    g_AudioSpec.userdata = NULL;
    g_AudioSpec.callback = SDLSoundCallback;

    if (SDL_OpenAudio(&g_AudioSpec, NULL) < 0) {
        _mm_errno=MMERR_OPENING_AUDIO;
        return 0;
    }

    return 1;
}

static void SDLDrv_CommandLine(const CHAR *cmdline)
{
	/* no options */
}

static BOOL SDLDrv_IsPresent(void)
{
    return 1;
}

static int SDLDrv_Init(void)
{
#if (SDL_MAJOR_VERSION < 2)
    if (md_mode & DMODE_FLOAT) {
        _mm_errno=MMERR_NO_FLOAT32;
        return 1;
    }
#endif
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        _mm_errno=MMERR_OPENING_AUDIO;
        return 1;
    }
    if (!SetupSDLAudio()) {
        return 1;
    }

    md_mode |= DMODE_SOFT_MUSIC;

    SDL_PauseAudio(0);
    return VC_Init();
}

static void SDLDrv_Exit(void)
{
    g_Playing = 0;
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    VC_Exit();
}

static int SDLDrv_Reset(void)
{
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    VC_Exit();

    if (!SetupSDLAudio())
        return 1;

    SDL_PauseAudio(0);
    return VC_Init();
}

static void SDLDrv_Update( void )
{
/* do nothing */
}

static void SDLDrv_PlayStop(void)
{
    g_Playing = 0;
    VC_PlayStop();
}

static int SDLDrv_PlayStart(void)
{
    g_Playing = 1;
    return VC_PlayStart();
}

MIKMODAPI struct MDRIVER drv_sdl =
{
    NULL,
    "SDL",
    "SDL Driver v1.2",
    0,
    255,
    "sdl",
    NULL,
    SDLDrv_CommandLine,
    SDLDrv_IsPresent,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    SDLDrv_Init,
    SDLDrv_Exit,
    SDLDrv_Reset,
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

