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

  $Id: mlist.h,v 1.1.1.1 2004/01/16 02:07:37 raph Exp $

  Playlist management functions

==============================================================================*/

#ifndef MLIST_H
#define MLIST_H

#include <mikmod.h> /* for BOOL and CHAR */

#define PL_CONT_NEXT (1)
#define PL_CONT_PREV (2)
#define PL_CONT_POS  (3)

#define PM_MODULE  (1)			/* Module repeatly */
#define PM_MULTI   (2)			/* PlayList repeatly */
#define PM_SHUFFLE (4)			/* shuffle PlayList */
#define PM_RANDOM  (8)			/* PlayList in random order */

#define PL_IDENT "MikMod playlist\n"

typedef struct {
	CHAR *file;
	CHAR *archive;
	int time;
	BOOL played;
} PLAYENTRY;

typedef struct {
	PLAYENTRY *entry;
	int length;
	int current;
	BOOL curr_deleted;
	int add_pos;
} PLAYLIST;

extern PLAYLIST playlist;

BOOL PL_isPlaylistFilename(const CHAR *filename);
void PL_InitList(PLAYLIST * pl);
void PL_InitCurrent(PLAYLIST * pl);
void PL_ClearList(PLAYLIST * pl);

BOOL PL_CurrentDeleted(PLAYLIST * pl);

int PL_GetCurrentPos(PLAYLIST * pl);
PLAYENTRY *PL_GetCurrent(PLAYLIST * pl);
PLAYENTRY *PL_GetEntry(PLAYLIST * pl, int number);
int PL_GetLength(PLAYLIST * pl);
void PL_SetTimeCurrent(PLAYLIST * pl, long sngtime);
void PL_SetPlayedCurrent(PLAYLIST * pl);

BOOL PL_DelEntry(PLAYLIST * pl, int number);
BOOL PL_DelDouble(PLAYLIST * pl);
void PL_Add(PLAYLIST * pl, const CHAR *file, const CHAR *arc, int time, BOOL played);
void PL_StartInsert(PLAYLIST * pl, int pos);
void PL_StopInsert(PLAYLIST * pl);

BOOL PL_Load(PLAYLIST * pl, const CHAR *filename);
BOOL PL_Save(PLAYLIST * pl, const CHAR *filename);
char *PL_GetFilename(void);
BOOL PL_LoadDefault(PLAYLIST * pl);
BOOL PL_SaveDefault(PLAYLIST * pl);

/* Get new playlist entry and change current accordingly */
BOOL PL_ContNext(PLAYLIST * pl, CHAR **retfile, CHAR **retarc, int mode);
BOOL PL_ContPrev(PLAYLIST * pl, CHAR **retfile, CHAR **retarc);
BOOL PL_ContPos(PLAYLIST * pl, CHAR **retfile, CHAR **retarc, int number);

void PL_Sort(PLAYLIST * pl,
			 int (*compar) (PLAYENTRY * small, PLAYENTRY * big));
void PL_Randomize(PLAYLIST * pl);

#endif

/* ex:set ts=4: */
