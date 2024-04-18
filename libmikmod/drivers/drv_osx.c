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

  Driver for output via CoreAudio [MacOS X and Darwin].
  Based on xmp player:
  Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_OSX

#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreServices/CoreServices.h>

static AudioUnit au;

#if (MAC_OS_X_VERSION_MIN_REQUIRED < 1060) || \
    (!defined(AUDIO_UNIT_VERSION) || ((AUDIO_UNIT_VERSION - 0) < 1060))
#define AudioComponent Component
#define AudioComponentDescription ComponentDescription
#define AudioComponentFindNext FindNextComponent
#define AudioComponentInstanceNew OpenAComponent
#define AudioComponentInstanceDispose CloseComponent
#endif

/*
 * CoreAudio helpers by Timothy J. Wood from mplayer/libao
 * The player fills a ring buffer, OSX retrieves data from the buffer
 */

static SBYTE *buffer;
static int buffer_len;
static int buf_write_pos;
static int buf_read_pos;
static int num_chunks;
static int chunk_size;
static int packet_size;

#define DEFAULT_LATENCY 250
static int latency = DEFAULT_LATENCY;


/* return minimum number of free bytes in buffer, value may change between
 * two immediately following calls, and the real number of free bytes
 * might actually be larger!  */
static int buf_free(void)
{
	int freebuf = buf_read_pos - buf_write_pos - chunk_size;
	if (freebuf < 0) {
	    freebuf += buffer_len;
	}
	return freebuf;
}

/* return minimum number of buffered bytes, value may change between
 * two immediately following calls, and the real number of buffered bytes
 * might actually be larger! */
static int buf_used(void)
{
	int used = buf_write_pos - buf_read_pos;
	if (used < 0) {
	    used += buffer_len;
	}
	return used;
}

/* remove data from ringbuffer */
static int read_buffer(SBYTE *data, int len)
{
	int first_len = buffer_len - buf_read_pos;
	int buffered = buf_used();

	if (len > buffered) {
	    len = buffered;
	}
	if (first_len > len) {
	    first_len = len;
	}

	/* till end of buffer */
	memcpy(data, buffer + buf_read_pos, first_len);
	if (len > first_len) {
	/* wrap around remaining part from beginning of buffer */
	    memcpy(data + first_len, buffer, len - first_len);
	}
	buf_read_pos = (buf_read_pos + len) % buffer_len;

	return len;
}

static OSStatus render_proc(void *inRefCon,
                            AudioUnitRenderActionFlags *inActionFlags,
                            const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                            UInt32 inNumFrames, AudioBufferList *ioData)
{
	int amt = buf_used();
	int req = inNumFrames * packet_size;

	if (amt > req) {
	    amt = req;
	}

	read_buffer((SBYTE *)ioData->mBuffers[0].mData, amt);
	ioData->mBuffers[0].mDataByteSize = amt;

	return noErr;
}

/*
 * end of CoreAudio helpers
 */

static void OSX_CommandLine(const CHAR *cmdline)
{
	CHAR *ptr = MD_GetAtom("latency", cmdline, 0);
	if (ptr) {
	    int n = atoi(ptr);
	    if (n >= 20 && n <= 1024)
		latency = n;
	    MikMod_free(ptr);
	}
}

static BOOL OSX_IsPresent (void)
{
	return 1;
}

static int OSX_Init(void)
{
	AudioStreamBasicDescription ad;
	AudioComponent comp;
	AudioComponentDescription cd;
	AURenderCallbackStruct rc;
	OSStatus status;
	UInt32 size, max_frames;

	if (!(md_mode & DMODE_STEREO)) {
	    _mm_errno = MMERR_OSX_UNSUPPORTED_FORMAT;
	    return 1;
	}
	if (!(md_mode & (DMODE_16BITS|DMODE_FLOAT))) {
	    _mm_errno = MMERR_OSX_UNSUPPORTED_FORMAT;
	    return 1;
	}

	/* set up basic md_mode, just to be secure */
	md_mode |= DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;

	/* Setup for signed 16 bit or float output: */
	ad.mSampleRate = md_mixfreq;
	ad.mFormatID = kAudioFormatLinearPCM;
	ad.mFormatFlags = kAudioFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
	ad.mChannelsPerFrame = 2;

	if (md_mode & DMODE_FLOAT) {
	    ad.mFormatFlags |= kAudioFormatFlagIsFloat;
	    ad.mBitsPerChannel = 32;
	    ad.mBytesPerFrame = 4 * ad.mChannelsPerFrame;
	} else {
	    ad.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
	    ad.mBitsPerChannel = 16;
	    ad.mBytesPerFrame = 2 * ad.mChannelsPerFrame;
	}
	ad.mBytesPerPacket = ad.mBytesPerFrame;
	ad.mFramesPerPacket = 1;

	packet_size = ad.mFramesPerPacket * ad.mChannelsPerFrame * (ad.mBitsPerChannel / 8);

	cd.componentType = kAudioUnitType_Output;
	cd.componentSubType = kAudioUnitSubType_DefaultOutput;
	cd.componentManufacturer = kAudioUnitManufacturer_Apple;
	cd.componentFlags = 0;
	cd.componentFlagsMask = 0;

	_mm_errno = MMERR_OSX_DEVICE_START; /* generic errnum. */

	if ((comp = AudioComponentFindNext(NULL, &cd)) == NULL) {
	    goto err;
	}

	if ((status = AudioComponentInstanceNew(comp, &au))) {
	    goto err1;
	}

	if ((status = AudioUnitInitialize(au))) {
	    goto err1;
	}

	status = AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Input, 0, &ad, sizeof(ad));
	if (status) {
	    goto err1;
	}

	size = sizeof(UInt32);
	status = AudioUnitGetProperty(au, kAudioDevicePropertyBufferSize,
                                      kAudioUnitScope_Input, 0, &max_frames, &size);
	if (status) {
	    goto err1;
	}

	chunk_size = max_frames;
	num_chunks = (md_mixfreq * ad.mBytesPerFrame * latency / 1000 + chunk_size - 1) / chunk_size;
	buffer_len = (num_chunks + 1) * chunk_size;
	if ((buffer = (SBYTE *)MikMod_calloc(num_chunks + 1, chunk_size)) == NULL) {
	    _mm_errno = MMERR_OUT_OF_MEMORY;
	    goto err;
	}

	rc.inputProc = render_proc;
	rc.inputProcRefCon = 0;

	buf_read_pos = 0;
	buf_write_pos = 0;

	status = AudioUnitSetProperty(au,
                                      kAudioUnitProperty_SetRenderCallback,
                                      kAudioUnitScope_Input, 0, &rc, sizeof(rc));
	if (status) {
	    goto err2;
	}

	_mm_errno = 0; /* reset. */

	return VC_Init ();

    err2:
	MikMod_free(buffer);
	buffer = NULL;
    err1:
	fprintf(stderr, "initialization error: %d\n", (int)status);
    err:
	return 1;
}


/* add data to ringbuffer */
static void OSX_Update (void)
{
	int len = buffer_len;
	int first_len = buffer_len - buf_write_pos;
	int freebuf = buf_free();

	if (len > freebuf) { /* shouldn't happen */
	    len = freebuf;
	}
	if (first_len > len) {
	    first_len = len;
	}

	/* till end of buffer */
	VC_WriteBytes(buffer + buf_write_pos, first_len);
	if (len > first_len) {
	/* wrap around remaining part from beginning of buffer */
	    VC_WriteBytes(buffer, len - first_len);
	}
	buf_write_pos = (buf_write_pos + len) % buffer_len;
}

static void OSX_Exit (void)
{
	AudioOutputUnitStop(au);
	AudioUnitUninitialize(au);
	AudioComponentInstanceDispose(au);
	MikMod_free(buffer);
	buffer = NULL;
}

static int OSX_PlayStart (void)
{
	/* start virtch */
	if (VC_PlayStart ()) {
	    return 1;
	}
	AudioOutputUnitStart(au);
	return 0;
}

static void OSX_PlayStop (void)
{
	AudioOutputUnitStop(au);
	VC_PlayStop ();
}

MIKMODAPI MDRIVER drv_osx={
	NULL,
	"CoreAudio Driver",
	"CoreAudio Driver v3.0",
	0,255,
	"osx",
	"latency:r:20,1024,250:CoreAudio latency\n",
	OSX_CommandLine,
	OSX_IsPresent,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	OSX_Init,
	OSX_Exit,
	NULL,
	VC_SetNumVoices,
	OSX_PlayStart,
	OSX_PlayStop,
	OSX_Update,
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

MISSING(drv_osx);

#endif /* DRV_OSX */
