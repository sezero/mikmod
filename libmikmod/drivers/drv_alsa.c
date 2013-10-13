/*	MikMod sound library
	(c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS for
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

  Driver for Advanced Linux Sound Architecture (ALSA)

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_ALSA

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#ifdef MIKMOD_DYNAMIC
#include <dlfcn.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <alsa/asoundlib.h>
#if defined(SND_LIB_VERSION) && (SND_LIB_VERSION >= 0x20000)
#undef DRV_ALSA
#endif

#if defined(SND_LIB_VERSION) && (SND_LIB_VERSION < 0x600)
#error ALSA Version too old. Please upgrade your Linux distribution.
#endif
#endif /* DRV_ALSA */

#ifdef DRV_ALSA

#ifdef MIKMOD_DYNAMIC
/* runtime link with libasound */
#ifndef HAVE_RTLD_GLOBAL
#define RTLD_GLOBAL (0)
#endif
static int (*alsa_pcm_subformat_mask_malloc)(snd_pcm_subformat_mask_t **);
static int (*alsa_pcm_hw_params_any)(snd_pcm_t *, snd_pcm_hw_params_t *);
static int (*alsa_pcm_get_params)(snd_pcm_t *, snd_pcm_uframes_t *, snd_pcm_uframes_t *);
static const char * (*alsa_strerror)(int);
static int (*alsa_pcm_sw_params_current)(snd_pcm_t *, snd_pcm_sw_params_t *);
static int (*alsa_pcm_sw_params_set_start_threshold)(snd_pcm_t *, snd_pcm_sw_params_t *,snd_pcm_uframes_t);
static int (*alsa_pcm_sw_params_set_avail_min)(snd_pcm_t *, snd_pcm_sw_params_t *, snd_pcm_uframes_t);
static int (*alsa_pcm_sw_params)(snd_pcm_t *, snd_pcm_sw_params_t *);
static int (*alsa_pcm_resume)(snd_pcm_t *);
static int (*alsa_pcm_prepare)(snd_pcm_t *);
static int (*alsa_pcm_sw_params_sizeof)(void);
static int (*alsa_pcm_hw_params_sizeof)(void);
static int(*alsa_ctl_close)(snd_ctl_t*);
static int(*alsa_pcm_open)(snd_pcm_t**, const char *, int, int);
static int (*alsa_pcm_set_params)(snd_pcm_t *, snd_pcm_format_t, snd_pcm_access_t,
				  unsigned int, unsigned int, int, unsigned int);
static int (*alsa_ctl_pcm_info)(snd_ctl_t*, int, snd_pcm_info_t*);
static int (*alsa_pcm_close)(snd_pcm_t*);
static int (*alsa_pcm_drain)(snd_pcm_t*);
static int (*alsa_pcm_drop)(snd_pcm_t*);
static int (*alsa_pcm_start)(snd_pcm_t *);
static snd_pcm_sframes_t (*alsa_pcm_avail_update)(snd_pcm_t*);
static snd_pcm_sframes_t (*alsa_pcm_writei)(snd_pcm_t*,const void*,snd_pcm_uframes_t);

static void* libasound = NULL;

#else
/* compile-time link with libasound */
#define alsa_pcm_subformat_mask_malloc		snd_pcm_subformat_mask_malloc
#define alsa_strerror				snd_strerror
#define alsa_pcm_get_params			snd_pcm_get_params
#define alsa_pcm_hw_params_any			snd_pcm_hw_params_any
#define alsa_pcm_set_params			snd_pcm_set_params
#define alsa_pcm_sw_params_current		snd_pcm_sw_params_current
#define alsa_pcm_sw_params_set_start_threshold	snd_pcm_sw_params_set_start_threshold
#define alsa_pcm_sw_params_set_avail_min	snd_pcm_sw_params_set_avail_min
#define alsa_pcm_sw_params			snd_pcm_sw_params
#define alsa_pcm_resume				snd_pcm_resume
#define alsa_pcm_prepare			snd_pcm_prepare
#define alsa_ctl_close				snd_ctl_close
#define alsa_ctl_pcm_info			snd_ctl_pcm_info
#define alsa_pcm_close				snd_pcm_close
#define alsa_pcm_drain				snd_pcm_drain
#define alsa_pcm_drop				snd_pcm_drop
#define alsa_pcm_start				snd_pcm_start
#define alsa_pcm_open				snd_pcm_open
#define alsa_pcm_avail_update			snd_pcm_avail_update
#define alsa_pcm_writei				snd_pcm_writei
#endif /* MIKMOD_DYNAMIC */

static snd_pcm_t *pcm_h = NULL;
static SBYTE *audiobuffer = NULL;
static snd_pcm_sframes_t period_size;
static int bytes_written = 0, bytes_played = 0;
static int global_frame_size;

#ifdef MIKMOD_DYNAMIC
static int ALSA_Link(void)
{
	if (libasound) return 0;

	/* load libasound.so */
	libasound = dlopen("libasound.so",RTLD_LAZY|RTLD_GLOBAL);
	if (!libasound) return 1;

	if (!(alsa_pcm_subformat_mask_malloc = dlsym(libasound,"snd_pcm_subformat_mask_malloc"))) return 1;
	if (!(alsa_strerror = dlsym(libasound,"snd_strerror"))) return 1;
	if (!(alsa_pcm_sw_params = dlsym(libasound,"snd_pcm_sw_params"))) return 1;
	if (!(alsa_pcm_prepare = dlsym(libasound,"snd_pcm_prepare"))) return 1;
	if (!(alsa_pcm_sw_params_sizeof = dlsym(libasound,"snd_pcm_sw_params_sizeof"))) return 1;
	if (!(alsa_pcm_hw_params_sizeof = dlsym(libasound,"snd_pcm_hw_params_sizeof"))) return 1;
	if (!(alsa_pcm_resume = dlsym(libasound,"snd_pcm_resume"))) return 1;
	if (!(alsa_pcm_sw_params_set_avail_min = dlsym(libasound,"snd_pcm_sw_params_set_avail_min"))) return 1;
	if (!(alsa_pcm_sw_params_current = dlsym(libasound,"snd_pcm_sw_params_current"))) return 1;
	if (!(alsa_pcm_sw_params_set_start_threshold = dlsym(libasound,"snd_pcm_sw_params_set_start_threshold"))) return 1;
	if (!(alsa_pcm_get_params = dlsym(libasound,"snd_pcm_get_params"))) return 1;
	if (!(alsa_pcm_hw_params_any = dlsym(libasound,"snd_pcm_hw_params_any"))) return 1;
	if (!(alsa_pcm_set_params = dlsym(libasound,"snd_pcm_set_params"))) return 1;
	if (!(alsa_ctl_close = dlsym(libasound,"snd_ctl_close"))) return 1;
	if (!(alsa_pcm_open = dlsym(libasound,"snd_pcm_open"))) return 1;
	if (!(alsa_ctl_pcm_info = dlsym(libasound,"snd_ctl_pcm_info"))) return 1;
	if (!(alsa_pcm_close = dlsym(libasound,"snd_pcm_close"))) return 1;
	if (!(alsa_pcm_drain = dlsym(libasound,"snd_pcm_drain"))) return 1;
	if (!(alsa_pcm_drop = dlsym(libasound,"snd_pcm_drop"))) return 1;
	if (!(alsa_pcm_start = dlsym(libasound,"snd_pcm_start"))) return 1;
	if (!(alsa_pcm_open = dlsym(libasound,"snd_pcm_open"))) return 1;
	if (!(alsa_pcm_avail_update = dlsym(libasound,"snd_pcm_avail_update"))) return 1;
	if (!(alsa_pcm_writei = dlsym(libasound,"snd_pcm_writei"))) return 1;

	return 0;
}

static void ALSA_Unlink(void)
{
	alsa_pcm_subformat_mask_malloc = NULL;
	alsa_strerror = NULL;
	alsa_pcm_sw_params_set_start_threshold = NULL;
	alsa_pcm_sw_params_current = NULL;
	alsa_pcm_sw_params_set_avail_min = NULL;
	alsa_pcm_sw_params = NULL;
	alsa_pcm_resume = NULL;
	alsa_pcm_prepare = NULL;
	alsa_pcm_set_params = NULL;
	alsa_pcm_get_params = NULL;
	alsa_pcm_hw_params_any = NULL;
	alsa_ctl_close = NULL;
	alsa_ctl_pcm_info = NULL;
	alsa_ctl_pcm_info = NULL;
	alsa_pcm_close = NULL;
	alsa_pcm_drain = NULL;
	alsa_pcm_drop = NULL;
	alsa_pcm_start = NULL;
	alsa_pcm_open = NULL;
	alsa_pcm_avail_update = NULL;
	alsa_pcm_writei = NULL;

	if (libasound) {
		dlclose(libasound);
		libasound = NULL;
	}
}

/* This is done to override the identifiers expanded
 * in the macros provided by the ALSA includes which are
 * not available.
 * */
#define snd_strerror			alsa_strerror
#define snd_pcm_sw_params_sizeof	alsa_pcm_sw_params_sizeof
#define snd_pcm_hw_params_sizeof	alsa_pcm_hw_params_sizeof
#endif /* MIKMOD_DYNAMIC */

static void ALSA_CommandLine(const CHAR *cmdline)
{
		/* no options */
}

static BOOL ALSA_IsThere(void)
{
	snd_pcm_subformat_mask_t *ptr = NULL;
	BOOL retval;

#ifdef MIKMOD_DYNAMIC
	if (ALSA_Link()) return 0;
#endif
	retval = (alsa_pcm_subformat_mask_malloc(&ptr) == 0) && (ptr != NULL);
	free(ptr);
#ifdef MIKMOD_DYNAMIC
	ALSA_Unlink();
#endif
	return retval;
}

static int ALSA_Init_internal(void)
{
	snd_pcm_format_t pformat;
	int rate, channels, err;
	snd_pcm_hw_params_t * hwparams;
	snd_pcm_sw_params_t * swparams;
	snd_pcm_uframes_t temp_u_buffer_size,
			  temp_u_period_size;

	/* setup playback format structure */
	pformat = (md_mode&DMODE_FLOAT)? SND_PCM_FORMAT_FLOAT :
			(md_mode&DMODE_16BITS)? SND_PCM_FORMAT_S16 : SND_PCM_FORMAT_U8;
	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);
	channels = (md_mode&DMODE_STEREO)?2:1;
	rate = md_mixfreq;

	/* scan for appropriate sound card */
	_mm_errno = MMERR_OPENING_AUDIO;

#define MIKMOD_ALSA_DEVICE "default"
	if ((err = alsa_pcm_open(&pcm_h, MIKMOD_ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
		fprintf(stderr, "snd_pcm_open() call failed: %s\n", alsa_strerror(err));
		goto END;
	}

	if (alsa_pcm_set_params(pcm_h, pformat, SND_PCM_ACCESS_RW_INTERLEAVED,
				channels, rate, 1, 500000 /* 0.5sec */) < 0) {
		goto END;
	}

	global_frame_size = channels *
				((md_mode&DMODE_FLOAT)? 4 : (md_mode&DMODE_16BITS)? 2 : 1);

	/* choose all parameters */
	err = alsa_pcm_hw_params_any(pcm_h, hwparams);
	if (err < 0) {
		fprintf(stderr, "Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
		goto END;
	}

	err = alsa_pcm_get_params(pcm_h, &temp_u_buffer_size, &temp_u_period_size);
	if (err < 0) {
		fprintf(stderr, "Unable to get buffer size for playback: %s\n", alsa_strerror(err));
		goto END;
	}
	period_size = temp_u_period_size;

	if (!(audiobuffer=(SBYTE*)MikMod_malloc(period_size * global_frame_size))) {
		fprintf(stderr, "Out of memory for ALSA buffer\n");
		goto END;
	}

	/* sound device is ready to work */
	if (!VC_Init())
		return 0;
END:
	alsa_pcm_close(pcm_h);
	pcm_h = NULL;
	return 1;
}

static int ALSA_Init(void)
{
#ifdef HAVE_SSE2
/* TODO : Detect SSE2, then set  md_mode |= DMODE_SIMDMIXER;*/
#endif
#ifdef MIKMOD_DYNAMIC
	if (ALSA_Link()) {
		_mm_errno=MMERR_DYNAMIC_LINKING;
		return 1;
	}
#endif
	return ALSA_Init_internal();
}

static void ALSA_Exit_internal(void)
{
	VC_Exit();
	if (pcm_h) {
		alsa_pcm_drain(pcm_h);
		alsa_pcm_close(pcm_h);
		pcm_h=NULL;
	}
	MikMod_free(audiobuffer);
}

static void ALSA_Exit(void)
{
	ALSA_Exit_internal();
#ifdef MIKMOD_DYNAMIC
	ALSA_Unlink();
#endif
}

/*
 *   Underrun and suspend recovery .
 *   This was copied from test/pcm.c in the alsa-lib distribution.
 */

static int xrun_recovery(snd_pcm_t *handle, int err)
{
	if (err == -EPIPE) {	/* under-run */
		err = alsa_pcm_prepare(handle);
		if (err < 0)
			fprintf(stderr, "Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
		return 0;
	}
	else if (err == -ESTRPIPE) {
		while ((err = alsa_pcm_resume(handle)) == -EAGAIN)
			sleep(1);	/* wait until the suspend flag is released */
		if (err < 0) {
			err = alsa_pcm_prepare(handle);
			if (err < 0)
				fprintf(stderr, "Can't recover from suspend, prepare failed: %s\n", snd_strerror(err));
		}
		return 0;
	}
	return err;
}

static void ALSA_Update(void)
{
	int err;

	if (bytes_written == 0 || bytes_played == bytes_written) {
		bytes_written = VC_WriteBytes(audiobuffer,period_size * global_frame_size);
		bytes_played = 0;
	}

	while (bytes_played < bytes_written)
	{
		err = alsa_pcm_writei(pcm_h, &audiobuffer[bytes_played], (bytes_written - bytes_played) / global_frame_size);
		if (err == -EAGAIN)
			continue;
		if (err < 0) {
			if ((err = xrun_recovery(pcm_h, err)) < 0) {
				fprintf(stderr, "Write error: %s\n", alsa_strerror(err));
				exit(-1);
			}
			break;
		}
		bytes_played += err * global_frame_size;
	}
}

static int ALSA_PlayStart(void)
{
	int err = alsa_pcm_prepare(pcm_h);
	if (err == 0)
	    err = alsa_pcm_start(pcm_h);
	if (err < 0) {
		fprintf(stderr, "PCM start error: %s\n", alsa_strerror(err));
		return 1;
	}

	return VC_PlayStart();
}

static void ALSA_PlayStop(void)
{
	VC_PlayStop();
	alsa_pcm_drop(pcm_h);
}

static int ALSA_Reset(void)
{
	ALSA_Exit_internal();
	return ALSA_Init_internal();
}

MIKMODAPI MDRIVER drv_alsa = {
	NULL,
	"ALSA",
	"Advanced Linux Sound Architecture (ALSA) driver v1.0",
	0,255,
	"alsa",
	NULL,
	ALSA_CommandLine,
	ALSA_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	ALSA_Init,
	ALSA_Exit,
	ALSA_Reset,
	VC_SetNumVoices,
	ALSA_PlayStart,
	ALSA_PlayStop,
	ALSA_Update,
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

MISSING(drv_alsa);

#endif

/* ex:set ts=4: */
