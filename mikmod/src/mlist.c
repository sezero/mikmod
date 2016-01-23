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

  $Id: mlist.c,v 1.1.1.1 2004/01/16 02:07:37 raph Exp $

  Playlist management functions

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifndef HAVE_FNMATCH_H
#include "mfnmatch.h"
#else
#include <fnmatch.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef SRANDOM_IN_MATH_H
#include <math.h>
#endif

#include "mlist.h"
#include "marchive.h"
#include "mutilities.h"

static int mikmod_random(int limit)
{
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)||defined(_mikmod_amiga)
	return rand() % limit;
#else
	return random() % limit;
#endif
}

/* Mark all the modules in the playlist as not played */
static void PL_ClearPlayed(PLAYLIST * pl)
{
	int i;

	for (i = 0; i < pl->length; i++)
		pl->entry[i].played = 0;
}

BOOL PL_isPlaylistFilename(const CHAR *filename)
{
	char *cfg_name = NULL;
	if (!fnmatch("*.mpl", filename, 0))
		return 1;
	if ((cfg_name = PL_GetFilename())) {
		const char *p1 = FIND_LAST_DIRSEP(cfg_name);
		const char *p2 = FIND_LAST_DIRSEP(filename);
		if (!p1) p1 = cfg_name;
		if (!p2) p2 = filename;
		if (!filecmp(p1, p2)) {
			free(cfg_name);
			return 1;
		}
		free(cfg_name);
	}
	return 0;
}

void PL_InitList(PLAYLIST * pl)
{
	pl->entry = NULL;
	pl->length = 0;
	pl->current = -1;
	pl->curr_deleted = 0;
	pl->add_pos = -1;
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)||defined(_mikmod_amiga)
	srand(time(NULL));
#else
	{
		const char * s = getenv("MIKMOD_SRAND_CONSTANT");
		if (s)
		{
			srandom((unsigned int)atoi(s));
		}
		else
		{
			srandom(time(NULL));
		}
	}
#endif
}

/* Choose the first non-played module */
void PL_InitCurrent(PLAYLIST * pl)
{
	pl->current = 0;
	while ((pl->current < pl->length) && (pl->entry[pl->current].played))
		pl->current++;
	if (pl->current >= pl->length) {
		PL_ClearPlayed(pl);
		pl->current = 0;
	}
	pl->current--;
}

void PL_ClearList(PLAYLIST * pl)
{
	int i;

	for (i = 0; i < pl->length; i++) {
		if (pl->entry[i].file)
			free(pl->entry[i].file);
		if (pl->entry[i].archive)
			free(pl->entry[i].archive);
	}
	if (pl->entry) {
		free(pl->entry);
		pl->entry = NULL;
	}
	pl->current = -1;
	pl->curr_deleted = 0;
	pl->length = 0;
}

BOOL PL_CurrentDeleted(PLAYLIST * pl)
{
	return pl->curr_deleted;
}

PLAYENTRY *PL_GetCurrent(PLAYLIST * pl)
{
	if (pl->current < 0 || !pl->length)
		return NULL;
	return &pl->entry[pl->current];
}

int PL_GetCurrentPos(PLAYLIST * pl)
{
	if (pl->current < 0 || !pl->length)
		return -1;
	return pl->current;
}

PLAYENTRY *PL_GetEntry(PLAYLIST * pl, int number)
{
	if ((number < 0) || (number >= pl->length))
		return NULL;
	return &pl->entry[number];
}

int PL_GetLength(PLAYLIST * pl)
{
	return pl->length;
}

void PL_SetTimeCurrent(PLAYLIST * pl, long sngtime)
{
	if (!pl->curr_deleted && pl->current >= 0 && pl->current < pl->length)
		pl->entry[pl->current].time = sngtime >> 10;
}

void PL_SetPlayedCurrent(PLAYLIST * pl)
{
	if (!pl->curr_deleted && pl->current >= 0 && pl->current < pl->length)
		pl->entry[pl->current].played = 1;
}

BOOL PL_DelEntry(PLAYLIST * pl, int number)
{
	int i;

	if (!pl->length)
		return 0;

	if (pl->entry[number].file)
		free(pl->entry[number].file);
	if (pl->entry[number].archive)
		free(pl->entry[number].archive);

	pl->length--;
	if (number <= pl->current) {
		if (number == pl->current)
			pl->curr_deleted = 1;
		pl->current--;
	}
	for (i = number; i < pl->length; i++)
		pl->entry[i] = pl->entry[i + 1];

	pl->entry = (PLAYENTRY *) realloc(pl->entry, pl->length * sizeof(PLAYENTRY));

	return 1;
}

BOOL PL_DelDouble(PLAYLIST * pl)
{
	int i, j;

	if (!pl->length)
		return 0;

	for (i = pl->length - 2; i >= 0; i--)
		for (j = i + 1; j < pl->length; j++)
			if (!filecmp(pl->entry[i].file, pl->entry[j].file) &&
				(!(pl->entry[i].archive || pl->entry[j].archive) ||
				 (pl->entry[i].archive && pl->entry[j].archive &&
				  !filecmp(pl->entry[i].archive, pl->entry[j].archive)))) {

				/* keep the time and played information whenever possible */
				if (!pl->entry[i].time)
					pl->entry[i].time = pl->entry[j].time;
				if (!pl->entry[i].played)
					pl->entry[i].played = pl->entry[j].played;
				PL_DelEntry(pl, j);
			}
	return 1;
}

/* Following PL_Add will insert at pos */
void PL_StartInsert(PLAYLIST * pl, int pos)
{
	pl->add_pos = pos;
}

/* Following PL_Add will append at end of playlist */
void PL_StopInsert(PLAYLIST * pl)
{
	pl->add_pos = -1;
}

static void PL_Insert(PLAYLIST * pl, int pos, const CHAR *file, const CHAR *arc,
					 int time, BOOL played)
{
	int i;

	pl->length++;
	pl->entry = (PLAYENTRY *) realloc(pl->entry, pl->length * sizeof(PLAYENTRY));

	for (i = pl->length - 1; i > pos; i--)
		pl->entry[i] = pl->entry[i - 1];
	if (pos <= pl->current)
		pl->current++;

	pl->entry[pos].file = strdup(file);

	if (arc) {
		pl->entry[pos].archive = strdup(arc);
	} else
		pl->entry[pos].archive = NULL;

	pl->entry[pos].time = time;
	pl->entry[pos].played = played;
}

/* pl->add_pos < 0 => Append entry at end of playlist
   pl->add_pos >= 0 => Insert entry at pl->add_pos and increment pl->add_pos */
void PL_Add(PLAYLIST * pl, const CHAR *file, const CHAR *arc, int time, BOOL played)
{
	if (pl->add_pos >= 0) {
		PL_Insert(pl, pl->add_pos, file, arc, time, played);
		pl->add_pos++;
	} else
		PL_Insert(pl, pl->length, file, arc, time, played);
}

#define LINE_LEN (PATH_MAX*2+20)	/* "file" "arc" time played */
/* Loads a playlist */
BOOL PL_Load(PLAYLIST * pl, const CHAR *filename)
{
	FILE *file;
	CHAR line[LINE_LEN];
	CHAR *mod, *arc, *pos, *slash;
	int time, played;
	CHAR *ok = NULL;

	if (!(file = fopen(path_conv_sys(filename), "r")))
		return 0;

	while ((ok = fgets(line, LINE_LEN, file)) &&
		   (strcasecmp(line, PL_IDENT)));
	if (!ok) {
		fclose(file);
		return 0;				/* file is not a playlist */
	}

	slash = FIND_LAST_DIRSEP(filename);
	while (fgets(line, LINE_LEN, file)) {
		if (*line != '"') continue;			/* line == '"file" "arc" time played' */

		mod = line + 1;						/* file */
		pos = mod;
		while (*pos != '"' && *pos)
			pos++;
		if (*pos != '"' || pos == mod) continue;
		*pos = '\0';

		pos++;								/* archive */
		while (*pos != '"' && *pos)
			pos++;
		if (*pos == '"') pos++;
		arc = pos;
		while (*pos != '"' && *pos)
			pos++;

		time = played = 0;
		if (*pos) {
			*pos = '\0';
			if (arc == pos)
				arc = NULL;

			pos += 2;							/* time played */
			sscanf(pos, "%d %d", &time, &played);
		} else
			arc = NULL;
		path_conv (arc);
		path_conv (mod);
		if (!arc && !time && !played)
			MA_FindFiles(pl, mod);
		else {
			/* we're loading a playlist, so it might be necessary to convert
			   playlist paths to relative paths from cwd */
			if (slash && path_relative(arc ? arc : mod)) {
				CHAR *dummy;

				dummy = (CHAR *)
				  malloc(slash + 1 - filename + strlen(arc ? arc : mod) + 1);
				strncpy(dummy, filename, slash + 1 - filename);
				dummy[slash + 1 - filename] = '\0';
				strcat(dummy, arc ? arc : mod);

				PL_Add(pl, arc ? mod : dummy, arc ? dummy : NULL, time,
					   (BOOL)played);

				free (dummy);
			} else
				PL_Add(pl, mod, arc, time, (BOOL)played);
		}
	}

	fclose(file);
	return 1;
}

BOOL PL_Save(PLAYLIST * pl, const CHAR *filename)
{
	FILE *file;
	int i;
	PLAYENTRY *entry;

	if (!(file = fopen(path_conv_sys(filename), "w")))
		return 0;

	if (fputs(PL_IDENT, file) != EOF) {
		for (i = 0; i < pl->length; i++) {
			entry = &pl->entry[i];
			if (entry->archive)
				fprintf(file, "\"%s\" \"%s\" %d %d\n",
						entry->file, entry->archive, entry->time,
						(int)entry->played);
			else
				fprintf(file, "\"%s\" \"\" %d %d\n",
						entry->file, entry->time, (int)entry->played);
		}
		fclose(file);
		return 1;
	}
	fclose(file);
	return 0;
}

char *PL_GetFilename(void)
{
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_mikmod_amiga)
	return get_cfg_name("mikmodpl.cfg");
#elif defined(_WIN32)
	return get_cfg_name("mikmod_playlist.mpl");
#else
	return get_cfg_name(".mikmod_playlist");
#endif
}

BOOL PL_LoadDefault(PLAYLIST * pl)
{
	char *name = PL_GetFilename();
	BOOL ret = 0;

	if (name) {
		ret = PL_Load(pl, name);
		free(name);
	}
	return ret;
}

BOOL PL_SaveDefault(PLAYLIST * pl)
{
	char *name = PL_GetFilename();
	BOOL ret = 0;

	if (name) {
		ret = PL_Save(pl, name);
		free(name);
	}
	return ret;
}

/* check if selected file is a playlist and exchange it with the
   playlist */
static BOOL PL_CheckPlaylist(PLAYLIST * pl, BOOL *ok, int old_current,
							 int cont, CHAR **retfile, CHAR **retarc, int arg)
{
	/* check if selected file is a playlist */
	if ((pl->entry[pl->current].file) && (!pl->entry[pl->current].archive)) {
		pl->add_pos = pl->current + 1;
		if (PL_Load(pl, pl->entry[pl->current].file)) {
			/* Yes -> del playlist-entry and get next entry in now modified
			   list */
			pl->add_pos = -1;
			PL_DelEntry(pl, pl->current);
			pl->current = old_current;
			switch (cont) {
			  case PL_CONT_NEXT:
				*ok = PL_ContNext(pl, retfile, retarc, arg);
				return 1;
			  case PL_CONT_PREV:
				*ok = PL_ContPrev(pl, retfile, retarc);
				return 1;
			  case PL_CONT_POS:
				*ok = PL_ContPos(pl, retfile, retarc, arg);
				return 1;
			}
		}
		pl->add_pos = -1;
	}
	return 0;
}

/* get next module to play
   mode: PM_MODULE, PM_MULTI, PM_SHUFFLE, or PM_RANDOM
   return: was there a module? */
BOOL PL_ContNext(PLAYLIST * pl, CHAR **retfile, CHAR **retarc, int mode)
{
	int num, i, not_played, old_current = pl->current;
	BOOL ok = 1;

	pl->curr_deleted = 0;
	if (!pl->length)
		return 0;

	if (BTST(mode, PM_RANDOM)) {
		not_played = 0;
		for (i = 0; i < pl->length; i++)
			if (!pl->entry[i].played)
				not_played++;

		if (!not_played) {
			PL_ClearPlayed(pl);
			not_played = pl->length;
			if (BTST(mode, PM_SHUFFLE))
				PL_Randomize(pl);
			if (!BTST(mode, PM_MULTI))
				return 0;
		}

		num = mikmod_random(not_played) + 1;
		while (num > 0) {
			pl->current++;
			if (pl->current == pl->length)
				pl->current = 0;
			if (!pl->entry[pl->current].played)
				num--;
		}
	} else {
		pl->current++;
		if (pl->current >= pl->length) {
			not_played = 0;
			for (i = 0; i < pl->length; i++)
				if (!pl->entry[i].played)
					not_played++;
			if (!not_played) {
				PL_ClearPlayed(pl);
				if (BTST(mode, PM_SHUFFLE))
					PL_Randomize(pl);
			}
			pl->current = 0;
			if (!BTST(mode, PM_MULTI))
				return 0;
		}
	}

	/* check if selected file is a playlist and load it */
	if (PL_CheckPlaylist(pl, &ok, old_current, PL_CONT_NEXT,
						 retfile, retarc, mode))
		return ok;

	if (retfile)
		*retfile = pl->entry[pl->current].file;
	if (retarc)
		*retarc = pl->entry[pl->current].archive;

	return 1;
}

BOOL PL_ContPrev(PLAYLIST * pl, CHAR **retfile, CHAR **retarc)
{
	int old_current = pl->current;
	BOOL ok = 1;

	pl->curr_deleted = 0;
	if (!pl->length)
		return 0;

	pl->current--;
	if (pl->current < 0)
		pl->current = pl->length - 1;

	/* check if selected file is a playlist and load it */
	if (PL_CheckPlaylist(pl, &ok, old_current, PL_CONT_PREV,
						 retfile, retarc, 0))
		return ok;

	if (retfile)
		*retfile = pl->entry[pl->current].file;
	if (retarc)
		*retarc = pl->entry[pl->current].archive;

	return 1;
}

BOOL PL_ContPos(PLAYLIST * pl, CHAR **retfile, CHAR **retarc, int number)
{
	int old_current = pl->current;
	BOOL ok = 1;

	pl->curr_deleted = 0;
	if ((number < 0) || (number >= pl->length))
		return 0;

	pl->current = number;

	/* check if selected file is a playlist and load it */
	if (PL_CheckPlaylist(pl, &ok, old_current, PL_CONT_POS,
						 retfile, retarc, number))
		return ok;

	if (retfile)
		*retfile = pl->entry[pl->current].file;
	if (retarc)
		*retarc = pl->entry[pl->current].archive;

	return 1;
}

void PL_Sort(PLAYLIST * pl,
			 int (*compar) (PLAYENTRY * small, PLAYENTRY * big))
{
	int i, j;
	BOOL end = 0;
	PLAYENTRY tmp;

	for (i = 0; i < pl->length && !end; i++) {
		end = 1;
		for (j = pl->length - 1; j > i; j--)
			if (compar(&pl->entry[j - 1], &pl->entry[j]) > 0) {
				tmp = pl->entry[j];
				pl->entry[j] = pl->entry[j - 1];
				pl->entry[j - 1] = tmp;

				if (pl->current == j)
					pl->current = j - 1;
				else if (pl->current == j - 1)
					pl->current = j;
				end = 0;
			}
	}
}

void PL_Randomize(PLAYLIST * pl)
{
	if (pl->length > 1) {
		int i, target;

		for (i = 0; i < pl->length - 1; i++) {
			target = mikmod_random(pl->length - i) + i;
			if (target != i) {
				PLAYENTRY temp;

				temp = pl->entry[i];
				pl->entry[i] = pl->entry[target];
				pl->entry[target] = temp;

				/* track selection */
				if (pl->current == i)
					pl->current = target;
				else if (pl->current == target)
					pl->current = i;
			}
		}
	}
}

/* ex:set ts=4: */
