/*  MikMod module player
	(c) 1998 - 2000 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: mplayer.c,v 1.1.1.1 2004/01/16 02:07:36 raph Exp $

  Threaded player functions

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(__OS2__)||defined(__EMX__)
#define INCL_DOS
#include <os2.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "mplayer.h"
#include "mthreads.h"
#include "mconfig.h"
#include "mutilities.h"

extern MODULE *mf;

#if LIBMIKMOD_VERSION >= 0x030200
static MP_DATA playdata;
#endif
static BOOL active = 0, paused = 1, use_threads = 0;
static int volume = -1;

#ifdef USE_THREADS
static DEFINE_MUTEX(data);
#endif
static DEFINE_THREAD(updater,updater_mode);

static void do_update(void)
{
#if LIBMIKMOD_VERSION >= 0x030200
	int i;
	unsigned long cur_time;
#endif
	BOOL locked = 0;

	MikMod_Update();

	if (updater_mode == MTH_RUNNING) {
		MUTEX_LOCK(data);
		locked = 1;
	}
	if (volume>=0) {
		Player_SetVolume (volume);
		volume = -1;
	}

	paused = Player_Paused();
	active = Player_Active();

#if LIBMIKMOD_VERSION >= 0x030200
	if (mf) {
		if (!config.fakevolbars) {
			cur_time = Time1000();
			for (i = 0; i < mf->numchn; i++) {
				playdata.vstatus[i].time = cur_time;
				playdata.vstatus[i].volamp =
					(Voice_RealVolume(Player_GetChannelVoice(i))
					 * playdata.vinfo[i].volume) >> 16;
			}
		}
		/* Query current voice status */
		Player_QueryVoices(mf->numchn, playdata.vinfo);
	}
#endif
	if (locked)
		MUTEX_UNLOCK(data);
}

#ifdef USE_THREADS
#ifdef HAVE_PTHREAD
static void* MP_updater(void *dummy)
#else
static void MP_updater(void *dummy)
#endif
{
	while (active && (updater_mode == MTH_RUNNING)) {
		do_update();
		SLEEP(5);
	}

	updater_mode = MTH_NORUN;
	active = 0;
	paused = 1;
#ifdef HAVE_PTHREAD
	return NULL;
#else
	return;
#endif
}
#endif

/* Initialise the threads. Returns if threads are used. */
BOOL MP_Init (void)
{
#ifdef USE_THREADS
	static int firstcall = 1;

	if (firstcall) {
		firstcall = 0;
		use_threads = 1;
		if (!MikMod_InitThreads() || !INIT_MUTEX(data))
			use_threads = 0;
	}
#endif
	return use_threads;
}

/* Inits a new thread for a new song to be played */
void MP_Start (void)
{
	MP_Init();
	do_update();
#ifdef USE_THREADS
	if (use_threads) {
		updater_mode = MTH_RUNNING;
		use_threads = THREAD_START(updater, MP_updater, NULL);
	}
#endif
}

/* MikMod_Update(), if threads are not used */
void MP_Update (void)
{
	if (!use_threads) {
		do_update();
	}
}

/* Removes the thread started by MP_Start() */
void MP_End (void)
{
	if (updater_mode == MTH_RUNNING)
		THREAD_JOIN(updater,updater_mode);

	active = 0;
	paused = 1;
}

/* Wrapper for Player_Active() */
BOOL MP_Active (void)
{
	return (active != 0);
}

/* Wrapper for Player_TogglePause() */
void MP_TogglePause (void)
{
	Player_TogglePause();
	paused = Player_Paused();
}

/* Wrapper for Player_Paused() */
BOOL MP_Paused (void)
{
	return (paused != 0);
}

/* Wrapper for Player_SetVolume() */
void MP_Volume (int vol)
{
	MUTEX_LOCK(data);
	volume = vol;
	MUTEX_UNLOCK(data);
}

#if LIBMIKMOD_VERSION >= 0x030200
/* Returns a copy of the actual playdata */
void MP_GetData (MP_DATA *data)
{
	MUTEX_LOCK(data);
	*data = playdata;
	MUTEX_UNLOCK(data);
}
#endif
