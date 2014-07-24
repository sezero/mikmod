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

  $Id: marchive.h,v 1.1.1.1 2004/01/16 02:07:32 raph Exp $

  Archive support

==============================================================================*/

#ifndef MARCHIVE_H
#define MARCHIVE_H

#include "mlist.h"

#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)&&!defined(_mikmod_amiga)
/* Drop all root privileges we might have. */
BOOL DropPrivileges (void);
#endif

/* Extracts the file 'file' from the archive 'arc'. Return a file
   descriptor to the extracted file. If the file could not be unlinked
   (e.g. under Windows an open file can not be unlinked), return its
   name in 'extracted'. */
int MA_dearchive (const CHAR *arc, const CHAR *file, CHAR **extracted);

/* Test if filename looks like a module or an archive
   playlist==1: also test against a playlist
   deep==1    : use Player_LoadTitle() for testing against a module,
		        otherwise test based on the filename */
#if LIBMIKMOD_VERSION < 0x030302
BOOL MA_TestName (char *filename, BOOL playlist, BOOL deep);
#else
BOOL MA_TestName (const char *filename, BOOL playlist, BOOL deep);
#endif

/* Examines file 'filename' to add modules to the playlist 'pl'. */
void MA_FindFiles (PLAYLIST * pl, const CHAR *filename);

#endif

/* ex:set ts=4: */
