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
#define OLD_ALSA
#endif

#endif
#ifdef DRV_ALSA

#ifdef MIKMOD_DYNAMIC
/* runtime link with libasound */
#ifdef OLD_ALSA
static unsigned int(*alsa_cards_mask)(void);
#else
static int (*alsa_pcm_subformat_mask_malloc)(snd_pcm_subformat_mask_t **ptr);
static int (*alsa_pcm_hw_params_any)(snd_pcm_t *, snd_pcm_hw_params_t *);
static int (*alsa_pcm_get_params)(snd_pcm_t *, snd_pcm_uframes_t *, snd_pcm_uframes_t *);
static const char * (*alsa_strerror)(int);
static int (*alsa_pcm_sw_params_current)(snd_pcm_t *, snd_pcm_sw_params_t *);
static int (*alsa_pcm_sw_params_set_start_threshold)(snd_pcm_t *, snd_pcm_sw_params_t *,snd_pcm_uframes_t val);
static int (*alsa_pcm_sw_params_set_avail_min)(snd_pcm_t *, snd_pcm_sw_params_t *, snd_pcm_uframes_t);
static int (*alsa_pcm_sw_params)(snd_pcm_t *, snd_pcm_sw_params_t *);
static int (*alsa_pcm_resume)(snd_pcm_t *);
static int (*alsa_pcm_prepare)(snd_pcm_t *);
static int (*alsa_pcm_sw_params_sizeof)(void);
static int (*alsa_pcm_hw_params_sizeof)(void);
#endif
static int(*alsa_ctl_close)(snd_ctl_t*);
#ifdef OLD_ALSA
static int(*alsa_ctl_hw_info)(snd_ctl_t*,struct snd_ctl_hw_info*);
static int(*alsa_ctl_open)(snd_ctl_t**,int);
#else
static int(*alsa_pcm_open)(snd_pcm_t**, const char *, int, int);
static int (*alsa_pcm_set_params)(snd_pcm_t *pcm,
                       snd_pcm_format_t format,
                       snd_pcm_access_t access,
                       unsigned int channels,
                       unsigned int rate,
                       int soft_resample,
                       unsigned int latency);
#endif
static int(*alsa_ctl_pcm_info)(snd_ctl_t*,int,snd_pcm_info_t*);
#ifdef OLD_ALSA
#if defined(SND_LIB_VERSION) && (SND_LIB_VERSION >= 0x400)
static int(*alsa_ctl_pcm_playback_info)(snd_ctl_t*,int,int,snd_pcm_playback_info_t*);
#else
static int(*alsa_ctl_pcm_playback_info)(snd_ctl_t*,int,snd_pcm_playback_info_t*);
#endif
#endif
static int(*alsa_pcm_close)(snd_pcm_t*);
#ifdef OLD_ALSA
static int(*alsa_pcm_drain_playback)(snd_pcm_t*);
#else
static int(*alsa_pcm_drain)(snd_pcm_t*);
#endif
#ifdef OLD_ALSA
static int(*alsa_pcm_flush_playback)(snd_pcm_t*);
#else
static int(*alsa_pcm_pause)(snd_pcm_t*, int);
#endif
#ifdef OLD_ALSA
static int(*alsa_pcm_open)(snd_pcm_t**,int,int,int);
#endif
#ifdef OLD_ALSA
static int(*alsa_pcm_playback_format)(snd_pcm_t*,snd_pcm_format_t*);
static int(*alsa_pcm_playback_info)(snd_pcm_t*,snd_pcm_playback_info_t*);
static int(*alsa_pcm_playback_params)(snd_pcm_t*,snd_pcm_playback_params_t*);
#endif 
#ifdef OLD_ALSA
static int(*alsa_pcm_playback_status)(snd_pcm_t*,snd_pcm_playback_status_t*);
#else
snd_pcm_sframes_t (*alsa_pcm_avail_update)(snd_pcm_t*);
#endif 

#ifdef OLD_ALSA
static int(*alsa_pcm_write)(snd_pcm_t*,const void*,size_t);
#else
static snd_pcm_sframes_t(*alsa_pcm_writei)(snd_pcm_t*,const void*,snd_pcm_uframes_t);
#endif

static void* libasound=NULL;
#ifndef HAVE_RTLD_GLOBAL
#define RTLD_GLOBAL (0)
#endif
#else
/* compile-time link with libasound */
#ifdef OLD_ALSA
#define alsa_cards_mask				snd_cards_mask
#else
#define alsa_pcm_subformat_mask_malloc snd_pcm_subformat_mask_malloc
#define alsa_strerror snd_strerror
#define alsa_pcm_get_params snd_pcm_get_params
#define alsa_pcm_hw_params_any snd_pcm_hw_params_any
#define alsa_pcm_set_params snd_pcm_set_params
#define alsa_pcm_sw_params_current snd_pcm_sw_params_current
#define alsa_pcm_sw_params_set_start_threshold snd_pcm_sw_params_set_start_threshold
#define alsa_pcm_sw_params_set_avail_min snd_pcm_sw_params_set_avail_min
#define alsa_pcm_sw_params snd_pcm_sw_params
#define alsa_pcm_resume snd_pcm_resume
#define alsa_pcm_prepare snd_pcm_prepare
#endif
#define alsa_ctl_close				snd_ctl_close
#ifdef OLD_ALSA
#define alsa_ctl_hw_info			snd_ctl_hw_info
#define alsa_ctl_open				snd_ctl_open
#endif
#define alsa_ctl_pcm_info			snd_ctl_pcm_info
#ifdef OLD_ALSA
#define alsa_ctl_pcm_playback_info	snd_ctl_pcm_playback_info
#endif
#define alsa_pcm_close				snd_pcm_close
#ifdef OLD_ALSA
#define alsa_pcm_drain_playback		snd_pcm_drain_playback
#else
#define alsa_pcm_drain		snd_pcm_drain
#endif
#ifdef OLD_ALSA
#define alsa_pcm_flush_playback		snd_pcm_flush_playback
#else
#define alsa_pcm_pause		snd_pcm_pause
#endif
#define alsa_pcm_open				snd_pcm_open
#ifdef OLD_ALSA
#define alsa_pcm_playback_format	snd_pcm_playback_format
#define alsa_pcm_playback_info		snd_pcm_playback_info
#define alsa_pcm_playback_params	snd_pcm_playback_params
#endif
#ifdef OLD_ALSA
#define alsa_pcm_playback_status	snd_pcm_playback_status
#else
#define alsa_pcm_avail_update	snd_pcm_avail_update
#endif
#ifdef OLD_ALSA
#define alsa_pcm_write				snd_pcm_write
#else
#define alsa_pcm_writei				snd_pcm_writei
#endif
#endif /* MIKMOD_DYNAMIC */

#define DEFAULT_NUMFRAGS 4

static	snd_pcm_t *pcm_h=NULL;
static	int numfrags=DEFAULT_NUMFRAGS;
static	SBYTE *audiobuffer=NULL;
#ifdef OLD_ALSA
static int fragmentsize;
static	int cardmin=0,cardmax=SND_CARDS;
#else
static snd_pcm_sframes_t period_size;
static snd_pcm_sframes_t buffer_size_in_frames;
static int bytes_written = 0, bytes_played = 0;
#endif
static int global_frame_size;
static	int device=-1;

#ifdef MIKMOD_DYNAMIC
static BOOL ALSA_Link(void)
{
	if(libasound) return 0;

	/* load libasound.so */
	libasound=dlopen("libasound.so",RTLD_LAZY|RTLD_GLOBAL);
	if(!libasound) return 1;

#ifdef OLD_ALSA
	/* resolve function references */
	if (!(alsa_cards_mask           =dlsym(libasound,"snd_cards_mask"))) return 1;
#else
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
#endif
	if(!(alsa_ctl_close            =dlsym(libasound,"snd_ctl_close"))) return 1;
#ifdef OLD_ALSA
	if(!(alsa_ctl_hw_info          =dlsym(libasound,"snd_ctl_hw_info"))) return 1;
	if(!(alsa_ctl_open             =dlsym(libasound,"snd_ctl_open"))) return 1;
#else
	if(!(alsa_pcm_open             =dlsym(libasound,"snd_pcm_open"))) return 1;
#endif
	if(!(alsa_ctl_pcm_info         =dlsym(libasound,"snd_ctl_pcm_info"))) return 1;
#ifdef OLD_ALSA
	if(!(alsa_ctl_pcm_playback_info=dlsym(libasound,"snd_ctl_pcm_playback_info"))) return 1;
#endif
	if(!(alsa_pcm_close            =dlsym(libasound,"snd_pcm_close"))) return 1;
#ifdef OLD_ALSA
	if(!(alsa_pcm_drain_playback   =dlsym(libasound,"snd_pcm_drain_playback"))) return 1;
#else
	if(!(alsa_pcm_drain   =dlsym(libasound,"snd_pcm_drain"))) return 1;
#endif
#ifdef OLD_ALSA
	if(!(alsa_pcm_flush_playback   =dlsym(libasound,"snd_pcm_flush_playback"))) return 1;
#else
	if(!(alsa_pcm_pause   =dlsym(libasound,"snd_pcm_pause"))) return 1;
#endif
	if(!(alsa_pcm_open             =dlsym(libasound,"snd_pcm_open"))) return 1;
#ifdef OLD_ALSA
	if(!(alsa_pcm_playback_format  =dlsym(libasound,"snd_pcm_playback_format"))) return 1;
	if(!(alsa_pcm_playback_info    =dlsym(libasound,"snd_pcm_playback_info"))) return 1;
	if(!(alsa_pcm_playback_params  =dlsym(libasound,"snd_pcm_playback_params"))) return 1;
#endif
#ifdef OLD_ALSA
	if(!(alsa_pcm_playback_status  =dlsym(libasound,"snd_pcm_playback_status"))) return 1;
#else
	if(!(alsa_pcm_avail_update  =dlsym(libasound,"snd_pcm_avail_update"))) return 1;
#endif
#ifdef OLD_ALSA
	if(!(alsa_pcm_write            =dlsym(libasound,"snd_pcm_write"))) return 1;
#else
	if(!(alsa_pcm_writei            =dlsym(libasound,"snd_pcm_writei"))) return 1;
#endif

	return 0;
}

static void ALSA_Unlink(void)
{
#ifdef OLD_ALSA
	alsa_cards_mask           =NULL;
#else
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
#endif
	alsa_ctl_close            =NULL;
#ifdef OLD_ALSA
	alsa_ctl_hw_info          =NULL;
	alsa_ctl_open             =NULL;
#endif
	alsa_ctl_pcm_info         =NULL;
#ifdef OLD_ALSA
	alsa_ctl_pcm_playback_info=NULL;
#else
	alsa_ctl_pcm_info = NULL;
#endif
	alsa_pcm_close            =NULL;
#ifdef OLD_ALSA
	alsa_pcm_drain_playback   =NULL;
#else
    alsa_pcm_drain            = NULL;
#endif
#ifdef OLD_ALSA
	alsa_pcm_flush_playback   =NULL;
#else
	alsa_pcm_pause   =NULL;
#endif
	alsa_pcm_open             =NULL;
#ifdef OLD_ALSA
	alsa_pcm_playback_format  =NULL;
	alsa_pcm_playback_info    =NULL;
	alsa_pcm_playback_params  =NULL;
	alsa_pcm_playback_status  =NULL;
#else
	alsa_pcm_avail_update  =NULL;
#endif
#ifdef OLD_ALSA
	alsa_pcm_write            =NULL;
#else
	alsa_pcm_writei            =NULL;
#endif

	if(libasound) {
		dlclose(libasound);
		libasound=NULL;
	}
}
#endif /* MIKMOD_DYNAMIC */

#ifdef MIKMOD_DYNAMIC
/* This is done to override the identifiers expanded
 * in the macros provided by the ALSA includes which are
 * not available.
 * */
#define snd_strerror alsa_strerror
#define snd_pcm_sw_params_sizeof alsa_pcm_sw_params_sizeof
#define snd_pcm_hw_params_sizeof alsa_pcm_hw_params_sizeof
#endif

static void ALSA_CommandLine(CHAR *cmdline)
{
	CHAR *ptr;

#ifdef OLD_ALSA
	if((ptr=MD_GetAtom("card",cmdline,0))) {
		cardmin=atoi(ptr);cardmax=cardmin+1;
		MikMod_free(ptr);
	} else {
		cardmin=0;cardmax=SND_CARDS;
	}
#endif
	if((ptr=MD_GetAtom("pcm",cmdline,0))) {
		device=atoi(ptr);
		MikMod_free(ptr);
	} else device=-1;
	if((ptr=MD_GetAtom("buffer",cmdline,0))) {
		numfrags=atoi(ptr);
		if ((numfrags<2)||(numfrags>16)) numfrags=DEFAULT_NUMFRAGS;
		MikMod_free(ptr);
	} else numfrags=DEFAULT_NUMFRAGS;
}

static BOOL ALSA_IsThere(void)
{
	int retval;

#ifdef MIKMOD_DYNAMIC
	if (ALSA_Link()) return 0;
#endif
#ifdef OLD_ALSA
	retval=(alsa_cards_mask())?1:0;
#else
    {
       snd_pcm_subformat_mask_t * ptr;
       retval = alsa_pcm_subformat_mask_malloc(&ptr);
       retval = retval || ptr;
       free(ptr);
       ptr = NULL;
    }
#endif
#ifdef MIKMOD_DYNAMIC
	ALSA_Unlink();
#endif
	return retval;
}

#ifndef OLD_ALSA
static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams)
{
	int err;

	/* get the current swparams */
	err = alsa_pcm_sw_params_current(handle, swparams);
	if (err < 0) {
		printf("Unable to determine current swparams for playback: %s\n", alsa_strerror(err));
		return err;
	}
	/* start the transfer when the buffer is almost full: */
	/* (buffer_size / avail_min) * avail_min */
	err = alsa_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size_in_frames / period_size) * period_size);
	if (err < 0) {
		printf("Unable to set start threshold mode for playback: %s\n", alsa_strerror(err));
		return err;
	}
	/* allow the transfer when at least period_size samples can be processed */
	/* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
	err = alsa_pcm_sw_params_set_avail_min(handle, swparams, period_size);
	if (err < 0) {
		printf("Unable to set avail min for playback: %s\n", alsa_strerror(err));
		return err;
	}
	/* write the parameters to the playback device */
	err = alsa_pcm_sw_params(handle, swparams);
	if (err < 0) {
		printf("Unable to set sw params for playback: %s\n", alsa_strerror(err));
		return err;
	}
	return 0;
}
#endif

static BOOL ALSA_Init_internal(void)
{
	snd_pcm_format_t pformat;
#ifdef OLD_ALSA
#define channels pformat.channels
#define rate pformat.rate
	int mask,card;
#else
    int rate;
    int channels;
    int err;
    snd_pcm_hw_params_t * hwparams;
	snd_pcm_sw_params_t * swparams;
#endif

	/* adjust user-configurable settings */
	if((getenv("MM_NUMFRAGS"))&&(numfrags==DEFAULT_NUMFRAGS)) {
		numfrags=atoi(getenv("MM_NUMFRAGS"));
		if ((numfrags<2)||(numfrags>16)) numfrags=DEFAULT_NUMFRAGS;
	}
#ifdef OLD_ALSA
	if((getenv("ALSA_CARD"))&&(!cardmin)&&(cardmax==SND_CARDS)) {
		cardmin=atoi(getenv("ALSA_CARD"));
		cardmax=cardmin+1;
#endif
		if(getenv("ALSA_PCM"))
			device=atoi(getenv("ALSA_PCM"));
#ifdef OLD_ALSA
	}
#endif

	/* setup playback format structure */
#define NUM_CHANNELS() ((md_mode&DMODE_STEREO)?2:1)

#ifdef OLD_ALSA
	memset(&pformat,0,sizeof(pformat));
#ifdef SND_LITTLE_ENDIAN
	pformat.format=(md_mode&DMODE_16BITS)?SND_PCM_SFMT_S16_LE:SND_PCM_SFMT_U8;
#else
	pformat.format=(md_mode&DMODE_16BITS)?SND_PCM_SFMT_S16_BE:SND_PCM_SFMT_U8;
#endif
#else
    pformat = (md_mode&DMODE_16BITS)?SND_PCM_FORMAT_S16_LE:SND_PCM_FORMAT_U8;
    snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);
#endif
	channels = NUM_CHANNELS();
	rate=md_mixfreq;

	/* scan for appropriate sound card */
#ifdef OLD_ALSA
	mask=alsa_cards_mask();
#endif
	_mm_errno=MMERR_OPENING_AUDIO;
#ifdef OLD_ALSA
	for (card=cardmin;card<cardmax;card++)
#endif
    {
#ifdef OLD_ALSA
		struct snd_ctl_hw_info info;
		snd_ctl_t *ctl_h;
		int dev,devmin,devmax;
#endif

#ifdef OLD_ALSA
		/* no card here, onto the next */
		if (!(mask&(1<<card))) continue;
#endif

#ifdef OLD_ALSA
		/* try to open the card in query mode */
		if (alsa_ctl_open(&ctl_h,card)<0)
			continue;
#else

#define MIKMOD_ALSA_DEVICE "default"

        if ((err = alsa_pcm_open(&pcm_h, MIKMOD_ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
        {
            printf("snd_pcm_open() call failed: %s\n", alsa_strerror(err));
            goto END;
        }
#endif

#ifdef OLD_ALSA
		/* get hardware information */
		if(alsa_ctl_hw_info(ctl_h,&info)<0) {
			alsa_ctl_close(ctl_h);
			continue;
		}

		/* scan subdevices */
		if(device==-1) {
			devmin=0;devmax=info.pcmdevs;
		} else
			devmin=devmax=device;
#endif

#ifdef OLD_ALSA
		for(dev=devmin;dev<devmax;dev++)
#endif
        {
#ifdef OLD_ALSA
			int size,bps;
			snd_pcm_info_t pcminfo;
			snd_pcm_playback_info_t ctlinfo;
			struct snd_pcm_playback_info pinfo;
			struct snd_pcm_playback_params pparams;

			/* get PCM capabilities */
			if(alsa_ctl_pcm_info(ctl_h,dev,&pcminfo)<0)
				continue;

			/* look for playback capability */
			if(!(pcminfo.flags&SND_PCM_INFO_PLAYBACK))
				continue;

			/* get playback information */
#if defined(SND_LIB_VERSION) && (SND_LIB_VERSION >= 0x400)
			if(alsa_ctl_pcm_playback_info(ctl_h,dev,0,&ctlinfo)<0)
				continue;
#else
			if(alsa_ctl_pcm_playback_info(ctl_h,dev,&ctlinfo)<0)
				continue;
#endif

	/*
	   If control goes here, we have found a sound device able to play PCM data.
	   Let's open in in playback mode and see if we have compatible playback
	   settings.
	*/

			if (alsa_pcm_open(&pcm_h,card,dev,SND_PCM_OPEN_PLAYBACK)<0)
				continue;

			if (alsa_pcm_playback_info(pcm_h,&pinfo)<0) {
				alsa_pcm_close(pcm_h);
				pcm_h=NULL;
				continue;
			}

			/* check we have compatible settings */
			if((pinfo.min_rate>rate)||(pinfo.max_rate<rate)||
			   (!(pinfo.formats&(1<<pformat.format)))) {
				alsa_pcm_close(pcm_h);
				pcm_h=NULL;
				continue;
			}

			fragmentsize=pinfo.buffer_size/numfrags;
#ifdef MIKMOD_DEBUG
			if ((fragmentsize<512)||(fragmentsize>16777216L))
				fprintf(stderr,"\rweird pinfo.buffer_size:%d\n",pinfo.buffer_size);
#endif

			alsa_pcm_flush_playback(pcm_h);

			/* set new parameters */
			if(alsa_pcm_playback_format(pcm_h,&pformat)<0)
#else
            if( alsa_pcm_set_params(pcm_h, pformat, 
                SND_PCM_ACCESS_RW_INTERLEAVED,
                channels,
                rate,
                1,
                500000 /* 0.5sec */
                ) < 0)
#endif
            {
				alsa_pcm_close(pcm_h);
				pcm_h=NULL;
#ifdef OLD_ALSA
				continue;
#else
                goto END;
#endif
			}

            global_frame_size = channels*(md_mode&DMODE_16BITS?2:1);
#ifdef OLD_ALSA
			/* compute a fragmentsize hint
			   each fragment should be shorter than, but close to, half a
			   second of playback */
			bps=(rate*global_frame_size)>>1;
			size=fragmentsize;while (size>bps) size>>=1;
#endif
#ifdef MIKMOD_DEBUG
			if (size < 16) {
				fprintf(stderr,"\rweird hint result:%d from %d, bps=%d\n",size,fragmentsize,bps);
				size=16;
			}
#endif
#ifdef OLD_ALSA
         buffer_size = size;
			memset(&pparams,0,sizeof(pparams));
			pparams.fragment_size=size;
			pparams.fragments_max=-1; /* choose the best */
			pparams.fragments_room=-1;
			if(alsa_pcm_playback_params(pcm_h,&pparams)<0)
            {
				alsa_pcm_close(pcm_h);
				pcm_h=NULL;
				continue;
			}
#else
            /* choose all parameters */
            err = alsa_pcm_hw_params_any(pcm_h, hwparams);
            if (err < 0) {
                printf("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
                goto END;
            }

            {
                snd_pcm_uframes_t temp_u_buffer_size, temp_u_period_size;
                err = alsa_pcm_get_params(pcm_h, &temp_u_buffer_size, &temp_u_period_size);
                if (err < 0) {
                    alsa_pcm_close(pcm_h);
                    pcm_h=NULL;
                    printf("Unable to get buffer size for playback: %s\n", alsa_strerror(err));
                    goto END;
                }
                buffer_size_in_frames = 1200;
                period_size = temp_u_period_size;
            }

            /* The set_swparams function was taken from test/pcm.c
             * in the alsa-lib distribution*/
            if ((err = set_swparams(pcm_h, swparams)) < 0) {
                printf("Setting of swparams failed: %s\n", snd_strerror(err));
                goto END;
            }
#endif

			if (!(audiobuffer=(SBYTE*)MikMod_malloc(
#ifdef OLD_ALSA
                        fragmentsize
#else
                        buffer_size_in_frames * global_frame_size
#endif
                        ))) {
#ifdef OLD_ALSA
				alsa_ctl_close(ctl_h);
#else
                alsa_pcm_close(pcm_h);
#endif
				return 1;
			}

			/* sound device is ready to work */
			if (VC_Init()) {
#ifdef OLD_ALSA
				alsa_ctl_close(ctl_h);
#else
                alsa_pcm_close(pcm_h);
#endif
				return 1;
			} else
			  return 0;
		}

#ifdef OLD_ALSA
		alsa_ctl_close(ctl_h);
#else
        alsa_pcm_close(pcm_h);
#endif
	}
END:
	return 1;
}

static BOOL ALSA_Init(void)
{
#ifdef HAVE_SSE2
	// TODO : Detect SSE2, then set
	// md_mode |= DMODE_SIMDMIXER; 
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
#ifdef OLD_ALSA
		alsa_pcm_drain_playback(pcm_h);
#else
		alsa_pcm_drain(pcm_h);
#endif
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

#ifdef OLD_ALSA
static void ALSA_Update(void)
{
	snd_pcm_playback_status_t status;
	int total, count;

	if (alsa_pcm_playback_status(pcm_h, &status) >= 0) {
		/* Update md_mixfreq if necessary */
		if (md_mixfreq != status.rate)
			md_mixfreq = status.rate;

		/* Using status.count would cause clicks, as this is always less than
		   the freespace  in the buffer - so compute how many bytes we can
		   afford */
		total = status.fragments * status.fragment_size - status.queue;
		if (total < fragmentsize)
			total = fragmentsize;
	} else
		total = fragmentsize;
	
	/* Don't send data if ALSA is too busy */
	while (total) {
		count = fragmentsize > total ? total : fragmentsize;
		total -= count;
		alsa_pcm_write(pcm_h,audiobuffer,VC_WriteBytes(audiobuffer,count));
	}
}
#else

/*
 *   Underrun and suspend recovery .
 *   This was copied from test/pcm.c in the alsa-lib distribution.
 */
 
static int xrun_recovery(snd_pcm_t *handle, int err)
{
	if (err == -EPIPE) {	/* under-run */
		err = alsa_pcm_prepare(handle);
		if (err < 0)
			printf("Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
		return 0;
	} else if (err == -ESTRPIPE) {
		while ((err = alsa_pcm_resume(handle)) == -EAGAIN)
			sleep(1);	/* wait until the suspend flag is released */
		if (err < 0) {
			err = alsa_pcm_prepare(handle);
			if (err < 0)
				printf("Can't recover from suspend, prepare failed: %s\n", snd_strerror(err));
		}
		return 0;
	}
	return err;
}

static void ALSA_Update(void)
{
    int err;

    {
        if (bytes_written == 0 || bytes_played == bytes_written)
        {
            bytes_written = VC_WriteBytes(audiobuffer,buffer_size_in_frames * global_frame_size);
            bytes_played = 0;
        }

        while (bytes_played < bytes_written)
        {
            err = alsa_pcm_writei(pcm_h, &audiobuffer[bytes_played], (bytes_written - bytes_played) / global_frame_size);
            if (err == -EAGAIN)
            {
                continue;
            }
            if (err < 0) {
                if ((err = xrun_recovery(pcm_h, err)) < 0) {
                    printf("Write error: %s\n", alsa_strerror(err));
                    exit(-1);
                }
                break;
            }
            bytes_played += err * global_frame_size;
        }
    }
}
#endif

static void ALSA_PlayStop(void)
{
	VC_PlayStop();
	
#ifdef OLD_ALSA
	alsa_pcm_flush_playback(pcm_h);
#else
    alsa_pcm_pause(pcm_h, 1);
#endif
}

static BOOL ALSA_Reset(void)
{
	ALSA_Exit_internal();
	return ALSA_Init_internal();
}

MIKMODAPI MDRIVER drv_alsa={
	NULL,
	"ALSA",
	"Advanced Linux Sound Architecture (ALSA) driver v0.4",
	0,255,
	"alsa",
	"card:r:0,31,0:Soundcard number\n"
        "pcm:r:0,3,0:PCM device number\n"
        "buffer:r:2,16,4:Number of buffer fragments\n",	
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
	VC_PlayStart,
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
