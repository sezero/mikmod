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

  $Id: mplayer.h,v 1.1.1.1 2004/01/16 02:07:36 raph Exp $

  Threaded player functions

==============================================================================*/

#ifndef MPLAYER_H
#define MPLAYER_H

#include <mikmod.h>

#if LIBMIKMOD_VERSION >= 0x030200
#define MAXVOICES 256
#endif

#if LIBMIKMOD_VERSION >= 0x030200
typedef struct {
	VOICEINFO vinfo[MAXVOICES];		/* Current status for all module voices */
	struct {
		unsigned long time;			/* Last time this structure was updated */
		UBYTE volamp;				/* Volume meter amplitude */
	} vstatus[MAXVOICES];			/* Dynamic voice status */
} MP_DATA;

/* Returns a copy of the actual playdata */
void MP_GetData (MP_DATA *data);
#endif

/* Initialise the threads. Returns if threads are used. */
BOOL MP_Init (void);

/* Inits a new thread for a new song to be played */
void MP_Start (void);
/* MikMod_Update(), if threads are not used */
void MP_Update (void);
/* Removes the thread started by MP_Start() */
void MP_End (void);

/* Wrapper for Player_Active() */
BOOL MP_Active (void);
/* Wrapper for Player_TogglePause() */
void MP_TogglePause (void);
/* Wrapper for Player_Paused() */
BOOL MP_Paused (void);
/* Wrapper for Player_SetVolume() */
void MP_Volume (int vol);

#endif /* MPLAYER_H */
