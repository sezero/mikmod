/*	MikMod sound library
	(c) 1998-2014 Miodrag Vallat and others - see file AUTHORS for
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

  drv_vita.c -- output data to PlayStation Vita audio device.
  by Matthieu Milan (15 April 2018, mikmod 3.3.11.1 port)

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_VITA

#include <psp2/audioout.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h>

#define DEFAULT_FRAGSIZE 11

static int audio_handle = -1;
static volatile int audio_terminate = 0;
static volatile int playing = 0;
static int fragsize = 1 << DEFAULT_FRAGSIZE;
static SBYTE *audiobuffers[2];
static int whichbuffer = 0;
static SceUID audiothread;

static int audio_thread(int args, void *argp)
{
	SceAudioOutPortType port = (md_mixfreq < 48000) ? SCE_AUDIO_OUT_PORT_TYPE_BGM : SCE_AUDIO_OUT_PORT_TYPE_MAIN;
	SceAudioOutMode mode = (md_mode & DMODE_STEREO) ? SCE_AUDIO_OUT_MODE_STEREO : SCE_AUDIO_OUT_MODE_MONO;

	audio_handle = sceAudioOutOpenPort(port, fragsize / (md_mode & DMODE_STEREO ? 4 : 2), md_mixfreq, mode);
	if (audio_handle < 0) {
		audio_terminate = 1;
		audio_handle = -1;
		_mm_errno = MMERR_OPENING_AUDIO;
		return 1;
	}

	int vols[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
	sceAudioOutSetVolume(audio_handle, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vols);

	while (!audio_terminate) {
		if (playing) {
			whichbuffer = !whichbuffer;
			VC_WriteBytes(audiobuffers[whichbuffer], fragsize);

			sceAudioOutOutput(audio_handle, audiobuffers[whichbuffer]);
		}
	}

	return 1;
}

static void VITA_CommandLine(const CHAR *cmdline)
{
	CHAR *ptr = MD_GetAtom("buffer", cmdline, 0);

	if (!ptr)
		fragsize = 1 << DEFAULT_FRAGSIZE;
	else {
		int n = atoi(ptr);
		if (n < 6 || n > 15) {
			n = DEFAULT_FRAGSIZE;
		}
		fragsize = 1 << n;
		MikMod_free(ptr);
	}
}

static void VITA_Update(void)
{
}

static BOOL VITA_IsPresent(void)
{
	return 1;
}

static int VITA_Init(void)
{
	if (VC_Init())
		return 1;

	for (int i = 0; i < 2; i++) {
		if (!(audiobuffers[i] = (SBYTE *)MikMod_malloc(fragsize))) {
			_mm_errno = MMERR_OUT_OF_MEMORY;
			return 1;
		}
	}

	audiothread = sceKernelCreateThread("Audio Thread", (void*)&audio_thread, 0x10000100, 0x10000, 0, 0, NULL);
	int res = sceKernelStartThread(audiothread, sizeof(audiothread), &audiothread);
	if (res != 0) {
		sceClibPrintf("Failed to init audio thread (0x%x)", res);
		return 0;
	}

	return 0;
}

static void VITA_Exit(void)
{
	audio_terminate = 1;
	sceKernelWaitThreadEnd(audiothread, NULL, NULL);
	sceKernelDeleteThread(audiothread);

	if (audio_handle != -1) {
		sceAudioOutReleasePort(audio_handle);
		audio_handle = -1;
	}

	for (int i = 0; i < 2; i++) {
		MikMod_free(audiobuffers[i]);
		audiobuffers[i] = NULL;
	}

	VC_Exit();
}

static int VITA_Reset(void)
{
	VC_Exit();
	return VC_Init();
}

static int VITA_PlayStart(void)
{
	VC_PlayStart();
	playing = 1;
	return 0;
}

static void VITA_PlayStop(void)
{
	playing = 0;
	VC_PlayStop();
}

MIKMODAPI MDRIVER drv_vita =
{
	NULL,
	"VITA Audio",
	"VITA Output Driver v1.0 - by usineur",
	0,255,
	"vitadrv",
	NULL,
	VITA_CommandLine,

	VITA_IsPresent,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	VITA_Init,
	VITA_Exit,
	VITA_Reset,

	VC_SetNumVoices,
	VITA_PlayStart,
	VITA_PlayStop,
	VITA_Update,
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

MISSING(drv_vita);

#endif

/* ex:set ts=8: */
