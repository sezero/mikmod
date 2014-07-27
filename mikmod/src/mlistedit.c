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

  $Id: mlistedit.c,v 1.1.1.1 2004/01/16 02:07:43 raph Exp $

  The playlist editor

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(__MINGW32__) || defined(__EMX__) || defined(__DJGPP__)
#include <dirent.h>
#elif defined(__OS2__) /* Watcom */
#include <direct.h>
#elif defined(_WIN32) /* MSVC, etc. */
#include <direct.h>
#else
#include <dirent.h>
#endif

#include <mikmod.h>
#include "mlistedit.h"
#include "mlist.h"
#include "player.h"
#include "mdialog.h"
#include "rcfile.h"
#include "mconfig.h"
#include "mconfedit.h"
#include "marchive.h"
#include "mwidget.h"
#include "keys.h"
#include "display.h"
#include "mutilities.h"

#define FREQ_SEL '*'
#define FREQ_SEL_STR "*"

#define FREQ_UNSEL ' '
#define FREQ_UNSEL_STR " "

/* Function, which is called on Ok/Cancel button select
   button: 0: Ok  1: Cancel
   path: Selected file
   data: user-pointer which was passed to freq_open()
   Return: close fileselector? */
typedef BOOL (*handleFreqFunc) (int button, char *file, void *data);

/* Function, which is called for every new directory during
   directory scanning in scan_dir(). scan_dir() is canceled if
   the function returns 1. */
typedef BOOL (*handleScandirFunc) (char *path, int added, int removed, void *data);

typedef enum {
	FREQ_ADD,
	FREQ_TOGGLE,
	FREQ_REMOVE
} FREQ_MODE;

typedef struct {
	WID_LIST *w;			/* the directory list */
	char path[PATH_MAX<<1];	/* path of currently displayed directory */
	BOOL before_add;		/* TRUE until first call of entry_add() */
	int actline;			/* pos in playlist for insertion */
							/* -1 -> append entries */
	int cnt_list;
	char **searchlist;		/* sorted playlist archives or files */
							/*   (if archives are not available)*/
	handleFreqFunc handle_freq;
	void *data;
} FREQ_DATA;

typedef struct {
	WID_LIST *w;			/* the directory list */
	FREQ_DATA *freq;
} HLIST_DATA;

typedef struct {
	MMENU *menu;
	int *actLine;
} MENU_DATA;

typedef struct {
	WID_LABEL *w;
	BOOL stop;
} FREQ_SCAN_DATA;

/* compare function for qsort on the searchlist */
static int searchlist_cmp (char **key, char **member)
{
	return (filecmp(*key,*member));
}

/* compare function for bsearch on the searchlist */
static int searchlist_search_cmp (char *key, char **member)
{
	return (filecmp(key,*member));
}

/* compare function for qsort on the directory list */
static int dirlist_cmp (char **small, char **big)
{
	if (IS_PATH_SEP((*small)[strlen(*small)-1])) {
		if (IS_PATH_SEP((*big)[strlen(*big)-1]))
			return(filecmp(*small+2,*big+2));
		else
			return -1;
	} else if (IS_PATH_SEP((*big)[strlen(*big)-1]))
		return 1;
	return(filecmp(*small+2,*big+2));
}

/* compare function for bearch on the directory list */
static int dirlist_search_cmp (char *key, char **member)
{
	if (IS_PATH_SEP(key[strlen(key)-1])) {
		if (IS_PATH_SEP((*member)[strlen(*member)-1]))
			return(filecmp(key,*member+2));
		else
			return -1;
	} else if (IS_PATH_SEP((*member)[strlen(*member)-1]))
		return 1;
	return(filecmp(key,*member+2));
}

/* Add/Remove tag marks to the files in entries (count: cnt) from
   directory path according to the searchlist */
static void freq_set_marks (char **entries, int cnt, const char *path, FREQ_DATA *data)
{
	int i;
	char file[PATH_MAX<<1], *fstart;

	strcpy (file,path);
	fstart = file+strlen(file);
	for (i=0; i<cnt; i++) {
		strcpy (fstart,entries[i]+2);
		if (!IS_PATH_SEP(fstart[strlen(fstart)]) &&
			data->cnt_list > 0 &&
			bsearch (file,data->searchlist,data->cnt_list,
					 sizeof(char*),(int(*)())searchlist_search_cmp))
			*(entries[i]) = FREQ_SEL;
		else
			*(entries[i]) = FREQ_UNSEL;
	}
}

/* Check if size of playlist has changed (due to e.g. resolving
   of playlists). If so, rebuild searchlist. */
static void freq_check_searchlist (FREQ_DATA *data)
{
	int i, len = PL_GetLength(&playlist);

	if (len != data->cnt_list) {
		data->searchlist = (char **) realloc (data->searchlist, sizeof(char*) * len);
		data->cnt_list = len;
		if (len) {
			for (i=0; i<len; i++) {
				PLAYENTRY *entry = PL_GetEntry(&playlist, i);
				if (entry->archive)
					data->searchlist[i] = entry->archive;
				else
					data->searchlist[i] = entry->file;
			}
			qsort (data->searchlist, len, sizeof(char*),(int(*)())searchlist_cmp);
		}
		if (data->w) {
			freq_set_marks (data->w->entries,data->w->cnt,data->path,data);
			wid_repaint ((WIDGET*)data->w);
		}
	}
}

/* Insert ins in (already enlarged) searchlist pl.
   pl must be sorted (according to filecmp()).*/
static void entry_insert (int left, int right, char **pl, char *ins)
{
	int pos=0, cmp=0, last = right;

	if (right<0) {
		pl[0] = ins;
	} else {
		while (left<=right) {
			pos = (left+right)/2;
			cmp = filecmp(ins,pl[pos]);
			if (cmp<0)
				right = pos-1;
			else
				left = pos+1;
		}
		if (cmp>0) pos++;
		for (cmp=last; cmp>=pos; cmp--)
			pl[cmp+1] = pl[cmp];
		pl[pos] = ins;
	}
}

/* Insert entry path+file at position data->actline into the playlist and
   update the (before and afterwards sorted) searchlist and
   actline from data.
   Return: Number of added entries */
static int entry_add (char *path, char *file, FREQ_DATA *data)
{
	int len, old_len = PL_GetLength(&playlist);
	char buffer[STORAGELEN];

	strcpy (buffer,path);
	if (file) strcat (buffer,file);

	if (data) {
		if (data->actline < 0 && data->before_add) {
			/* "Load" was selected -> Remove old entries */
			data->before_add = 0;
			PL_ClearList(&playlist);
			old_len = PL_GetLength(&playlist);
			freq_check_searchlist (data);
		} else
			PL_StartInsert(&playlist, data->actline);
	}
	MA_FindFiles(&playlist, buffer);
	PL_StopInsert(&playlist);

	len = PL_GetLength(&playlist);
	if (!old_len && len)
		PL_InitCurrent(&playlist);

	/* Update the searchlist */
	if (len>old_len && data) {
		int i, start, end;

		data->searchlist = (char **) realloc (data->searchlist, sizeof(char*) * len);

		start = data->actline;
		if (start<0) start = old_len;
		end = start+len-old_len;
		for (i=start; i<end; i++) {
			PLAYENTRY *entry = PL_GetEntry(&playlist, i);
			char *ins = entry->archive ? entry->archive:entry->file;
			entry_insert (0,data->cnt_list-1,data->searchlist,ins);
			data->cnt_list++;
		}
		if (data->actline>=0) data->actline += len-old_len;
	}
	return len-old_len;
}

/* remove all entries with archive==path+file (or file==path+file,
   if archive not set) from the playlist and update the (before
   and afterwards sorted) searchlist and actline from data.
   Return: Number of removed entries */
static int entry_remove_by_name(char *path, char *file, FREQ_DATA *data)
{
	int len = PL_GetLength(&playlist);
	char buffer[STORAGELEN];
	int cnt_remove = 0, i;
	char **pos;

	strcpy (buffer,path);
	if (file) strcat (buffer,file);

	/* Update the searchlist */
	while (data->cnt_list>0 &&
		   (pos = (char **) bsearch(buffer,data->searchlist,data->cnt_list,
						 sizeof(char*),(int(*)())searchlist_search_cmp))) {
		while (pos < data->searchlist + data->cnt_list - 1) {
			*pos = *(pos+1);
			pos++;
		}
		data->cnt_list--;
	}
	data->searchlist = (char **) realloc (data->searchlist, sizeof(char*) * data->cnt_list);

	/* Remove the entries from the playlist */
	for (i=len-1; i>=0; i--) {
		PLAYENTRY *entry = PL_GetEntry(&playlist, i);
		if (!filecmp (entry->archive ? entry->archive:entry->file,
					  buffer)) {
			PL_DelEntry(&playlist, i);
			if (i < data->actline) data->actline--;
			cnt_remove++;
		}
	}
	return cnt_remove;
}

/* Scan directory path for modules and add all files to the playlist,
   which are not already in data->searchlist (if data!=NULL).
   recursive: Scan recursively
   links    : Follow links */
static void scan_dir (char *path, BOOL recursive, BOOL links,
					  FREQ_DATA *freq_data, FREQ_MODE mode,
					  handleScandirFunc func, void *data,
					  int *added, int *removed)
{
#define DIR_BLOCK 10
	DIR *dir;
	struct dirent *entry;
	struct stat statbuf;
	char file[PATH_MAX<<1], *pathend, **dirs=NULL;
	int cnt = 0, max = 0, i;

	if (
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)&&!defined(_mikmod_amiga)
		!strcmp (path,"/proc/") ||
		!strcmp (path,"/dev/") ||
#endif
		!(dir = opendir (path_conv_sys(path))))
		return;

	if (func) {
		int add=-1, rem=-1;
		if (added) add = *added;
		if (removed) rem = *removed;
		if (func (path,add,rem,data)) {
			closedir (dir);
			return;
		}
	}

	strcpy (file,path);
	pathend = file+strlen(file);

	while ((entry = readdir (dir))) {
		strcpy (pathend,entry->d_name);
		path_conv(pathend);
		if (!lstat(path_conv_sys(file), &statbuf)) {
			if (S_ISDIR(statbuf.st_mode)) {
				/* if dir, process it after the files */
				if (recursive &&
					(links || !S_ISLNK(statbuf.st_mode)) &&
					strcmp (entry->d_name,"..") &&
					strcmp (entry->d_name,".")) {
					/* FIXME: check for cyclic links is missing */
					if (cnt >= max) {
						max += DIR_BLOCK;
						dirs = (char **) realloc (dirs, sizeof(char*) * max);
					}
					dirs[cnt++] = strdup (entry->d_name);
				}
			} else if (!S_ISCHR(statbuf.st_mode) && !S_ISBLK(statbuf.st_mode) &&
					   !S_ISFIFO(statbuf.st_mode) && !S_ISSOCK(statbuf.st_mode) &&
					   MA_TestName (file, 0 , 0)) {
				/* file of known type: add/remove it */
				char **pos = NULL;
				int j = 0;
				if (freq_data && freq_data->cnt_list > 0)
					pos = (char **) bsearch(file,freq_data->searchlist,freq_data->cnt_list,
								 sizeof(char*),(int(*)())searchlist_search_cmp);
				if (pos) {
					if (mode != FREQ_ADD) {
						j = entry_remove_by_name(file, NULL, freq_data);
						if (removed) *removed += j;
					} else if (freq_data->actline < 0 && freq_data->before_add) {
						j = entry_add(file, NULL, freq_data);
						if (added) *added += j;
					}
				} else {
					if (mode != FREQ_REMOVE) {
						j = entry_add(file, NULL, freq_data);
						if (added) *added += j;
					}
				}
			}
		}
		while (win_main_iteration());
	}
	/* now process dirs after files are already processed */
	for (i=0; i<cnt; i++) {
		strcpy (pathend, dirs[i]);
		path_conv(pathend);
		strcat (pathend,PATH_SEP_STR);
		scan_dir (file, recursive, links, freq_data, mode,
				  func, data, added, removed);
		free (dirs[i]);
	}
	if (dirs) free (dirs);
	closedir (dir);
}

/* read directory path and store the entries in entries */
static void freq_readdir (const char *path, char ***entries, int *cnt, FREQ_DATA *data)
{
#define ENT_BLOCK 10
	int max;
	DIR *dir = opendir (path_conv_sys(path));
	char file[PATH_MAX<<1], *pathend, *help;
	struct dirent *entry;
	struct stat statbuf;

	strcpy (file,path);
	pathend = file+strlen(file);
	*entries = NULL;
	*cnt = 0;
	if (dir) {
		max = *cnt = 0;
#ifdef _mikmod_amiga
		if (pathend != file && pathend[-1] != ':') {
			/* on AmigaOS variants, we won't get a ".." parentdir entry --
			   add a fake one here. */
			max += ENT_BLOCK;
			*entries = (char **) realloc (*entries, sizeof(char*) * max);
			strcpy (pathend,".."PATH_SEP_STR);
			path_conv (pathend);
			help = (char *) malloc (sizeof(char) * (strlen(pathend) + 3));
			strcpy (help,"  ");
			strcat (help,pathend);
			(*entries)[(*cnt)++] = help;
		}
#endif
		while ((entry = readdir (dir))) {
			if (*cnt >= max) {
				max += ENT_BLOCK;
				*entries = (char **) realloc (*entries, sizeof(char*) * max);
			}
			strcpy (pathend,entry->d_name);
			path_conv (pathend);
			if (!stat(path_conv_sys(file), &statbuf))
				if (S_ISDIR(statbuf.st_mode))
					strcat (pathend,PATH_SEP_STR);

			help = (char *) malloc (sizeof(char) * (strlen(pathend) + 3));
			strcpy (help,"  ");
			strcat (help,pathend);
			(*entries)[(*cnt)++] = help;
		}
		freq_set_marks (*entries,*cnt,path,data);
		closedir (dir);
		if (*cnt)
			qsort (*entries, *cnt, sizeof(char*),(int(*)())dirlist_cmp);
	}
}

/* free directory list read with freq_readdir() */
static void freq_freedir (char **entries, int cnt)
{
	int i;
	for (i=0; i<cnt; i++)
		if (entries[i]) free (entries[i]);
	free (entries);
}

static void freq_set_title (FREQ_DATA *data)
{
	int max = data->w->w.width-2;

	if (strlen(data->path) <= max)
		wid_list_set_title (data->w, data->path);
	else {
		char path[MAXWIDTH];
		strcpy (path, "...");
		strcat (path, &data->path[strlen(data->path)-max+3]);
		wid_list_set_title (data->w, path);
	}
	wid_repaint ((WIDGET*)data->w);
}

/* change directory to path (read directory and display it) */
static void freq_changedir (const char *path, FREQ_DATA *data)
{
	char **entries, *last= NULL, *end, **pos = NULL, ch;
	int cnt;

	freq_readdir (path,&entries,&cnt,data);
	if (entries && cnt>0) {
		/* Check if new path is part of the old one and
		   find position in entries where the old path continues
		   to correctly reposition active entry  */
		if (strlen(path) < strlen(data->path)) {
			last = data->path+strlen(path);
			ch = *last;
			*last = '\0';
			if (!filecmp (data->path, path)) {
				*last = ch;
				end = last;
				while (*end && !IS_PATH_SEP(*end))
					end++;
				if (IS_PATH_SEP(*end)) {
					*(end+1) = '\0';
					pos=(char**) bsearch(last, entries, cnt, sizeof(char*),
								 (int(*)())dirlist_search_cmp);
				} else
					pos = NULL;
			}
		}
		if (!pos) pos = entries;
		strcpy (data->path, path);
		wid_list_set_entries (data->w, (const char **)entries, 0, cnt);
		wid_list_set_active (data->w, pos-entries);
		freq_set_title (data);
		freq_freedir (entries,cnt);
	} else
		dlg_error_show ("Unable to read directory \"%s\"!",path);
}

static void hlist_close (HLIST_DATA *data)
{
	dialog_close(data->w->w.d);
	free (data);
}

static int cb_hlist_list_focus(struct WIDGET *w, int focus)
{
	if (focus == FOCUS_ACTIVATE) {
		HLIST_DATA *data = (HLIST_DATA *) w->data;
		int cur = ((WID_LIST*)w)->cur;

		/* return in hotlist -> change to the selected dir */
		freq_check_searchlist (data->freq);
		if (cur < config.cnt_hotlist)
			freq_changedir (config.hotlist[cur],data->freq);
		hlist_close(data);
		return EVENT_HANDLED;
	}
	return focus;
}

static int cb_hlist_button_focus(struct WIDGET *w, int focus)
{
	if (focus == FOCUS_ACTIVATE) {
		HLIST_DATA *data = (HLIST_DATA *) w->data;
		int button = ((WID_BUTTON *) w)->active;
		int cur = data->w->cur;

		freq_check_searchlist (data->freq);
		switch (button) {
			case 0:				/* change To */
				if (cur < config.cnt_hotlist)
					freq_changedir (config.hotlist[cur],data->freq);
				hlist_close(data);
				break;
			case 1:				/* Add current */
				CF_string_array_insert (cur,&config.hotlist,&config.cnt_hotlist,
										data->freq->path,PATH_MAX);
				wid_list_set_entries (data->w,(const char **)config.hotlist,-1,config.cnt_hotlist);
				wid_repaint ((WIDGET*)data->w);
				break;
			case 2:				/* Remove */
				CF_string_array_remove (cur,&config.hotlist,&config.cnt_hotlist);
				wid_list_set_entries (data->w,(const char **)config.hotlist,-1,config.cnt_hotlist);
				wid_repaint ((WIDGET*)data->w);
				break;
			case 3:				/* Cancel */
				hlist_close(data);
				break;
		}
		return EVENT_HANDLED;
	}
	return focus;
}

/* open the directory hotlist editor */
static void freq_hotlist (FREQ_DATA *freq_data)
{
	DIALOG *d = dialog_new();
	WIDGET *w;
	HLIST_DATA *data = (HLIST_DATA *) malloc (sizeof(HLIST_DATA));

	w = wid_list_add(d, 1, (const char **)config.hotlist, config.cnt_hotlist);
	wid_set_size (w, 74, 10);
	data->w = (WID_LIST*)w;
	data->freq = freq_data;
	wid_set_func(w, NULL, cb_hlist_list_focus, data);

	w = wid_button_add(d, 1, "<change &To>|&Add current|&Remove|&Cancel", 0);
	wid_set_func(w, NULL, cb_hlist_button_focus, data);

	dialog_open(d, "Directory hotlist");
}

/* Check if file is a directory and copy the resulting path from
   path and file to dest */
static BOOL path_update (char *dest, char *path, char *file)
{
	char *end;

	if (!strcmp (file,".."PATH_SEP_STR)) {
		strcpy (dest, path);
		end = dest+strlen(dest)-2;
		while (end>dest && !IS_PATH_SEP(*end))
			*end-- = '\0';
	} else if (!strcmp (file,"."PATH_SEP_STR)) {
		strcpy (dest, path);
	} else if (IS_PATH_SEP(file[strlen(file)-1])) {
		strcpy (dest, path);
		strcat (dest, file);
	} else
		return 0;
	return 1;
}

static int cb_scan_dir_stop_focus(struct WIDGET *w, int focus)
{
	if (focus == FOCUS_ACTIVATE) {
		if (((WID_BUTTON *) w)->active == 0)
			((FREQ_SCAN_DATA*)w->data)->stop = 1;

		return EVENT_HANDLED;
	}
	return focus;
}

/* Show progress during directory scanning */
BOOL cb_freq_scan_dir (char *path, int added, int removed, void *data)
{
	FREQ_SCAN_DATA *scan_data = (FREQ_SCAN_DATA*)data;

	if (strlen(path) > 50)
		sprintf (storage,"Scanning ...%s...\n"
				        "%4d entrie(s) added, %4d entrie(s) removed",
				&path[strlen(path)-47], added, removed);
	else
		sprintf (storage,"Scanning %s...\n"
				        "%4d entrie(s) added, %4d entrie(s) removed",
				path, added, removed);

	wid_label_set_label ((WID_LABEL*)(scan_data->w),storage);
	dialog_repaint (scan_data->w->w.d->win);
	win_refresh();
	return scan_data->stop;
}

/* Scan directory path for modules and add/remove them to the playlist
   according to mode */
static void freq_scan_dir (char *path, FREQ_DATA *data, FREQ_MODE mode)
{
	int added=0, removed=0;
	DIALOG *d = dialog_new();
	WIDGET *w;
	FREQ_SCAN_DATA scan_data;

	scan_data.stop = 0;
	if (strlen(path) > 50)
		sprintf (storage,"Scanning ...%-47s...\n"
				         "   0 entrie(s) added,    0 entrie(s) removed",
				&path[strlen(path)-47]);
	else
		sprintf (storage,"Scanning %-50s...\n"
				         "   0 entrie(s) added,    0 entrie(s) removed",path);
	scan_data.w = (WID_LABEL*)wid_label_add(d, 1, storage);
	w = wid_button_add(d, 2, "&Stop", 0);
	wid_set_func(w, NULL, cb_scan_dir_stop_focus, &scan_data);

	dialog_open(d, "Message");
	win_refresh();

	scan_dir (path, 1, 0, data, mode,
			  cb_freq_scan_dir, &scan_data, &added, &removed);
	dialog_close(d);

	freq_set_marks (data->w->entries,data->w->cnt,data->path,data);
	sprintf (storage,"Added %d entrie(s) and removed %d entrie(s).",
			 added,removed);
	dlg_message_open(storage, "&Ok", 0, 0, NULL, NULL);
}

/* Add/Remove entries to/from the playlist */
static void freq_add (FREQ_DATA *data, FREQ_MODE mode)
{
	char *file = data->w->entries[data->w->cur];
	char *path = data->path;
	char help[PATH_MAX<<1];

	if (path_update (help,path,file+2)) {
		freq_scan_dir (help, data, mode);
	} else if (*file == FREQ_SEL) {
		if (mode != FREQ_ADD) {
			if (entry_remove_by_name(path, file+2, data) > 0)
				*file = FREQ_UNSEL;
		} else if (data->actline < 0 && data->before_add)
			if (entry_add(path, file+2, data) > 0)
				*file = FREQ_SEL;
	} else {
		if (mode != FREQ_REMOVE) {
			if (entry_add(path, file+2, data) > 0)
				*file = FREQ_SEL;
		}
	}
	wid_list_set_active (data->w,data->w->cur+1);
	win_panel_repaint();
}

static void freq_close (FREQ_DATA *data)
{
	if (data) {
		if (data->w) dialog_close(data->w->w.d);
		if (data->searchlist) free (data->searchlist);
		free (data);
	}
	PL_DelDouble(&playlist);
}

/* Ok/Back was selected and data->handle_freq() is present -> call function
   Return: close fileselector? */
static BOOL freq_call_func (int button, FREQ_DATA *data)
{
	char file[PATH_MAX<<1];

	strcpy (file, data->path);
	strcat (file, data->w->entries[data->w->cur]+2);
	return data->handle_freq (button,file,data->data);
}

static int cb_freq_list_focus(struct WIDGET *w, int focus)
{
	if (focus == FOCUS_ACTIVATE) {
		FREQ_DATA *data = (FREQ_DATA *) w->data;
		int cur = ((WID_LIST*)w)->cur;
		char path[PATH_MAX<<1], *cur_entry;

		freq_check_searchlist (data);

		path[0] = '\0';
		cur_entry = ((WID_LIST*)w)->entries[cur]+2;

		/* Default action for dirs: change dir
		   For files: call user-function or add entry to playlist */
		if (!path_update(path,data->path,cur_entry)) {
			if (data->handle_freq) {
				if (freq_call_func (0,data))
					freq_close (data);
			} else
				freq_add (data,FREQ_ADD);
		}
		if (path[0] != '\0')
			freq_changedir (path,data);

		return EVENT_HANDLED;
	}
	return focus;
}

static BOOL cb_freq_cd_do (WIDGET *w,int button, void *input, void *data)
{
	if (button<=0) {
		char *pos;
		path_conv((char *)input);
		pos = (char*)input + strlen((char*)input);
		/* Check if path ends with '/' */
		if (!IS_PATH_SEP(*(pos-1))) {
			*pos = PATH_SEP;
			*(pos+1) = '\0';
		}
		freq_check_searchlist ((FREQ_DATA *)data);
		freq_changedir ((char *)input, (FREQ_DATA *)data);
	}
	return 1;
}

static void freq_cd (FREQ_DATA *data)
{
	dlg_input_str ("Change directory to:", "<&Ok>|&Cancel",
				   data->path, PATH_MAX, cb_freq_cd_do, data);
}

static int cb_freq_list_key(WIDGET *w, int ch)
{
	FREQ_DATA *data = (FREQ_DATA *) w->data;

	freq_check_searchlist (data);
	if ((ch < 256) && (isalpha(ch)))
		ch = toupper(ch);
	switch (ch) {
		case KEY_IC:				/* Insert -> Add */
			freq_add (data,FREQ_ADD);
			break;
		default:
			return 0;
	}
	return EVENT_HANDLED;
}

static int cb_freq_button_focus(struct WIDGET *w, int focus)
{
	if (focus == FOCUS_ACTIVATE) {
		FREQ_DATA *data = (FREQ_DATA *) w->data;
		int button = ((WID_BUTTON *) w)->active;

		freq_check_searchlist (data);
		switch (button) {
			case 0:				/* Add */
				freq_add (data,FREQ_ADD);
				break;
			case 1:				/* Toggle */
				freq_add (data,FREQ_TOGGLE);
				break;
			case 2:				/* Cd */
				freq_cd (data);
				break;
			case 3:				/* HotList */
				freq_hotlist (data);
				break;
			case 4:				/* Back / Ok */
				if (!data->handle_freq || freq_call_func (0,data))
					freq_close (data);
				break;
			case 5:				/* Back */
				if (data->handle_freq && freq_call_func (1,data))
					freq_close (data);
				break;
		}
		return EVENT_HANDLED;
	}
	return focus;
}

/* Init initial path and searchlist */
static FREQ_DATA *freq_data_init (const char *path)
{
	struct stat statbuf;
	FREQ_DATA *data = (FREQ_DATA *) malloc(sizeof(FREQ_DATA));
	char *pos;

	data->path[0] = '\0';
	if (path_relative(path)) {
		getcwd (data->path,PATH_MAX);
		path_conv (data->path);
		if (!IS_PATH_SEP(data->path[strlen(data->path)-1]))
			strcat (data->path, PATH_SEP_STR);
	}
	strcat (data->path,path);
	if (stat(path_conv_sys(data->path), &statbuf) || !S_ISDIR(statbuf.st_mode))
		if ((pos = FIND_LAST_DIRSEP(data->path)) != NULL)
			*(pos+1) = '\0';

	pos = data->path+strlen(data->path);
	if (!IS_PATH_SEP(*(pos-1))) {
		*pos = PATH_SEP;
		*(pos+1) = '\0';
	}
	data->w = NULL;
	data->before_add = 1;
	data->actline = -1;
	data->cnt_list = 0;
	data->searchlist = NULL;

	freq_check_searchlist (data);
	return data;
}

/* Open a file requester.
   func!=NULL: func is called if Ok or Cancel is selected
   func==NULL: no Ok button, Add is default */
void freq_open (const char *title, const char *path, int actline,
				handleFreqFunc func, void *data)
{
	FREQ_DATA *freq_data;
	DIALOG *d = dialog_new();
	WIDGET *w;
	char **entries, *path_first = NULL;
	int cnt;

	freq_data = freq_data_init (path);
	freq_data->actline = actline;
	freq_data->handle_freq = func;
	freq_data->data = data;

	freq_readdir(freq_data->path,&entries,&cnt,freq_data);
	if (!entries || !cnt) {
		/* show error after file selector is open */
		path_first = strdup (freq_data->path);

		/* error on initial path -> try root directory */
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
		strcpy (freq_data->path,"c:"PATH_SEP_STR);
#elif defined _mikmod_amiga
		strcpy (freq_data->path,"SYS:"); /* or use ":" instead??? */
#else
		strcpy (freq_data->path,PATH_SEP_STR);
#endif
		freq_readdir(freq_data->path,&entries,&cnt,freq_data);
		if (!entries || !cnt) {
			/* again an error -> give up */
			freq_close (freq_data);
			if (path_first) free (path_first);
			return;
		}
	}

	w = wid_list_add(d, 1, (const char **)entries, cnt);
	freq_data->w = (WID_LIST*)w;
	wid_set_func(w, cb_freq_list_key, cb_freq_list_focus, freq_data);
	freq_freedir(entries, cnt);

	if (func)
		w = wid_button_add(d, 1, "&Add|&Toggle|&Cd|&Hlist|<&Ok>|&Back", 0);
	else
		w = wid_button_add(d, 1, "<&Add>|&Toggle|&Cd|&Hlist|&Back", 0);
	wid_set_func(w, NULL, cb_freq_button_focus, freq_data);

	dialog_open(d, title);
	/* Size of list widget is necessary -> set title after dialog_open() */
	freq_set_title (freq_data);
	if (path_first) {
		dlg_error_show ("Unable to read directory \"%s\"!",path_first);
		free (path_first);
	}
}

static BOOL cb_list_scan_dir (char *path, int added, int removed, void *data)
{
	BOOL quiet = (BOOL)(SINTPTR_T)data;
	char str[70], *pos;
	int i;

	if (!quiet) {
		if (strlen(path) > 43)
			sprintf (str,"\rScanning ...%s... (%d added)",
					 &path[strlen(path)-40],added);
		else
			sprintf (str,"\rScanning %s... (%d added)",path,added);
		pos = str+strlen(str);
		for (i=strlen(str); i<(70-1); i++)
			*pos++ = ' ';
		*pos = '\0';
		printf ("%s", str);
		fflush(stdout);
	}
	return 0;
}

/* test if path is a directory and recursively scan it for modules */
int list_scan_dir (char *path, BOOL quiet)
{
	struct stat statbuf;
	int added = 0;
	char dir[PATH_MAX<<1]="", *pos;

#if defined(__EMX__)||defined(__OS2__)||defined(__DJGPP__)||defined(_WIN32)
	if (*path!=PATH_SEP && *(path+1)!=':')
#else
	if (!IS_PATH_SEP(*path))
#endif
	{
		getcwd (dir,PATH_MAX);
		path_conv (dir);
		if (!IS_PATH_SEP(dir[strlen(dir)-1]))
			strcat (dir, PATH_SEP_STR);
	}
	strcat (dir,path);
	pos = dir+strlen(dir);
	if (!IS_PATH_SEP(*(pos-1))) {
		*pos = PATH_SEP;
		*(pos+1) = '\0';
	}

	if (!stat(path_conv_sys(dir), &statbuf) && S_ISDIR(statbuf.st_mode))
		scan_dir (dir, 1, 0, NULL, FREQ_ADD,
				  cb_list_scan_dir, (void *)(SINTPTR_T)quiet, &added, NULL);
	return added;
}

/* remove an entry from the playlist */
static void entry_remove (int entry)
{
	PL_DelEntry(&playlist, entry);
}

/* remove an entry from the playlist and delete the associated module */
static BOOL cb_delete_entry(WIDGET *w, int button, void *input, void *entry)
{
	if (button<=0) {
		PLAYENTRY *cur = PL_GetEntry(&playlist, (SINTPTR_T)entry);
		if (cur->archive) {
			if (unlink(path_conv_sys(cur->archive)) == -1)
				dlg_error_show("Error deleting archive \"%s\"!",cur->archive);
		} else {
			if (unlink(path_conv_sys(cur->file)) == -1)
				dlg_error_show("Error deleting file \"%s\"!",cur->file);
		}
		entry_remove((SINTPTR_T)entry);
	}
	return 1;
}

/* split a filename into the name and the last extension */
static void split_name(char *file, char **name, char **ext)
{
	*name = FIND_LAST_DIRSEP(file);
	if (!*name)
		*name = file;
	*ext = strrchr(*name, '.');
	if (!*ext)
		*ext = &(*name[strlen(*name)]);
}

static BOOL sort_rev = 0;
/* *INDENT-OFF* */
static enum {
	SORT_NAME,
	SORT_EXT,
	SORT_PATH,
	SORT_TIME
} sort_mode = SORT_NAME;
/* *INDENT-ON* */

static int cb_cmp_sort(PLAYENTRY * small, PLAYENTRY * big)
{
	char ch_s = ' ', ch_b = ' ', *ext_s, *ext_b, *name_s, *name_b;
	int ret = 0;

	switch (sort_mode) {
	  case SORT_NAME:
		split_name(small->file, &name_s, &ext_s);
		split_name(big->file, &name_b, &ext_b);
		ch_s = *ext_s;
		ch_b = *ext_b;
		*ext_s = '\0';
		*ext_b = '\0';
		ret = strcasecmp(name_s, name_b);
		*ext_s = ch_s;
		*ext_b = ch_b;
		break;
	  case SORT_EXT:
		split_name(small->file, &name_s, &ext_s);
		split_name(big->file, &name_b, &ext_b);
		ret = strcasecmp(ext_s, ext_b);
		break;
	  case SORT_PATH:
		ext_s = small->archive;
		if (!ext_s)
			ext_s = small->file;
		name_s = FIND_LAST_DIRSEP(ext_s);
		if (name_s) {
			ch_s = *name_s;
			*name_s = '\0';
		}
		ext_b = big->archive;
		if (!ext_b)
			ext_b = big->file;
		name_b = FIND_LAST_DIRSEP(ext_b);
		if (name_b) {
			ch_b = *name_b;
			*name_b = '\0';
		}
		ret = strcasecmp(ext_s, ext_b);
		if (name_s)
			*name_s = ch_s;
		if (name_b)
			*name_b = ch_b;
		break;
	  case SORT_TIME:
		ret = (small->time == big->time ? 0 :
			   (small->time < big->time ? -1 : 1));
		break;
	}
	return (sort_rev) ? -ret : ret;
}

/* overwrites an existdng playlist */
static BOOL cb_overwrite (WIDGET *w, int button, void *input, void *file)
{
	if (button<=0) {
		path_conv((char *)file);
		if (PL_Save(&playlist, (char *)file))
			rc_set_string(&config.pl_name, (char *)file, PATH_MAX);
		else
			dlg_error_show("Error saving playlist \"%s\"!",file);
	}
	if (file) free(file);
	return 1;
}

static BOOL cb_browse (int button, char *file, void *data)
{
	if (!button) {
		wid_str_set_input ((WID_STR*)data, file, -1);
		wid_repaint ((WIDGET*)data);
	}
	return 1;
}

/* saves a playlist */
static BOOL cb_save_as(WIDGET *w, int button, void *input, void *data)
{
	path_conv((char *)input);
	if (button == 0) {								/* Browse */
		freq_open ("Select directory/file",(char*)input,(SINTPTR_T)data,
				   cb_browse,w);
		return 0;
	} else if (button == 1 || button == -1) {		/* Ok / Str-Widget */
		if (file_exist((char*)input)) {
			char *f_copy = strdup((char*)input);
			char *msg = str_sprintf("File \"%s\" exists.\n"
									"Really overwrite the file?", f_copy);
			dlg_message_open(msg, "&Yes|&No", 1, 1, cb_overwrite, f_copy);
			free(msg);
		} else {
			if (PL_Save(&playlist, (char*)input))
				rc_set_string(&config.pl_name, (char*)input, PATH_MAX);
			else
				dlg_error_show("Error saving playlist \"%s\"!",input);
		}
	}
	return 1;
}

/* playlist menu handler */
static void cb_handle_menu(MMENU * menu)
{
	MENU_DATA *data = (MENU_DATA *) menu->data;
	int actLine = *data->actLine;
	PLAYENTRY *cur;
	char *name, *msg;

	/* main menu */
	if (!menu->id) {
		switch (menu->cur) {
		  case 0:				/* play highlighted module */
			if (actLine >= 0)
				Player_SetNextMod(actLine);
			break;
		  case 1:				/* remove highlighted module */
			if (actLine >= 0)
				entry_remove(actLine);
			break;
		  case 2:				/* delete highlighted module */
			cur = PL_GetEntry(&playlist, actLine);
			if (!cur)
				break;

			if (cur->archive) {
				name = FIND_LAST_DIRSEP(cur->file);
				if (name)
					name++;
				else
					name = cur->file;

				if (strlen(cur->archive) > 60)
					msg = str_sprintf2("File \"%s\" is in an archive!\n"
									   "Really delete whole archive\n"
									   "  \"...%s\"?", name,
									   &(cur->
										 archive[strlen(cur->archive) - 57]));
				else
					msg =
					  str_sprintf2("File \"%s\" is in an archive!\n"
								   "Really delete whole archive\n"
								   "  \"%s\"?", name, cur->archive);
				dlg_message_open(msg, "&Yes|&No", 1, 1, cb_delete_entry,
								 (void *)(SINTPTR_T)actLine);
			} else {
				if (strlen(cur->file) > 50)
					msg = str_sprintf("Delete file \"...%s\"?",
									  &(cur->file[strlen(cur->file) - 47]));
				else
					msg = str_sprintf("Delete file \"%s\"?", cur->file);
				dlg_message_open(msg, "&Yes|&No", 1, 1,
								 cb_delete_entry, (void *)(SINTPTR_T)actLine);
			}
			free(msg);
			break;
		  case 5:				/* shuffle list */
			PL_Randomize(&playlist);
			break;
		  case 7:				/* cancel */
			break;
		  default:
			return;
		}
		/* file menu */
	} else if (menu->id == 1) {
		switch (menu->cur) {
		  case 0:				/* load */
			  freq_open ("Load modules/playlists",
						 config.pl_name, -1, NULL, NULL);
			  break;
		  case 1:				/* insert */
			  freq_open ("Insert modules/playlists",
						 config.pl_name, actLine, NULL, NULL);
			  break;
		  case 2:				/* save */
			if (!PL_Save(&playlist, config.pl_name))
				dlg_error_show("Error saving playlist \"%s\"!",config.pl_name);
			break;
		  case 3:				/* save as */
			dlg_input_str("Save playlist as:", "&Browse|<&Ok>|&Cancel",
						  config.pl_name, PATH_MAX, cb_save_as,
						  (void*)(SINTPTR_T)actLine);
			break;
		  default:
			return;
		}
		/* sort menu */
	} else {
		/* reverse flag */
		sort_rev = (SINTPTR_T)menu->entries[5].data;
		switch (menu->cur) {
		  case 0:				/* by name */
			sort_mode = SORT_NAME;
			PL_Sort(&playlist, cb_cmp_sort);
			break;
		  case 1:				/* by extension */
			sort_mode = SORT_EXT;
			PL_Sort(&playlist, cb_cmp_sort);
			break;
		  case 2:				/* by path */
			sort_mode = SORT_PATH;
			PL_Sort(&playlist, cb_cmp_sort);
			break;
		  case 3:				/* by time */
			sort_mode = SORT_TIME;
			PL_Sort(&playlist, cb_cmp_sort);
			break;
		  default:
			return;
		}
	}
	menu_close(data->menu);
	return;
}

void list_open(int *actLine)
{
	static MENU_DATA menu_data;

	static MENTRY file_entries[] = {
		{"&Load...", 0, "Load new playlists/modules"},
		{"&Insert...", 0, "Insert new playlists/modules in current playlist"},
		{"&Save", 0, NULL},
		{"Save &as...", 0, "Save playlist in a specified file"},
		{NULL,NULL,NULL}
	};
	static MMENU file_menu =
	  { 0, 0, -1, 1, file_entries, cb_handle_menu, NULL, &menu_data, 1 };

	static MENTRY sort_entries[] = {
		{"by &name", 0, "Sort list by name of modules"},
		{"by &extension", 0, "Sort list by extension of modules"},
		{"by &path", 0, "Sort list by path of modules/archives"},
		{"by &time", 0, "Sort list by playing time of modules"},
		{"%---------", 0, NULL},
		{"[%c] &reverse", 0, "Smaller to bigger or reverse sort"},
		{NULL,NULL,NULL}
	};
	static MMENU sort_menu =
	  { 0, 0, -1, 1, sort_entries, cb_handle_menu, NULL, &menu_data, 2 };

	static MENTRY entries[] = {
		{"&Play", 0, "Play selected entry"},
		{"&Remove", 0, "Remove selected entry from list"},
		{"&Delete...", 0,
		 "Remove selected entry from list and delete it on disk"},
		{"%----------", 0, NULL},
		{"&File        %>", &file_menu, "Load/Save playlist/modules"},
		{"&Shuffle", 0, "Shuffle the list"},
		{"S&ort        %>", &sort_menu, "Sort the list"},
		{"&Back", 0, "Leave menu"},
		{NULL,NULL,NULL}
	};
	static MMENU menu =
	  { 0, 0, -1, 1, entries, cb_handle_menu, NULL, &menu_data, 0 };

	menu_data.menu = &menu;
	menu_data.actLine = actLine;
	set_help(&file_entries[2], "Save list in '%s'", config.pl_name);
	menu_open(&menu, 5, 5);
}
