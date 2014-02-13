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

  $Id: mconfig.h,v 1.3 2004/01/29 03:09:23 raph Exp $

  Configuration file management

==============================================================================*/

#ifndef MCONFIG_H
#define MCONFIG_H

#include <mikmod.h>

#include "rcfile.h"

#define RENICE_NONE		0
#define RENICE_PRI		1
#define RENICE_REAL		2

/*========== Color and attribute definitions */

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
#define A_REVERSE			0x70
#define A_NORMAL			0x07
#define A_BOLD				0x0f
#endif

#define COLOR_BLACK_B		0x00
#define COLOR_BLUE_B		0x10
#define COLOR_GREEN_B		0x20
#define COLOR_CYAN_B		0x30
#define COLOR_RED_B			0x40
#define COLOR_MAGENTA_B		0x50
#define COLOR_BROWN_B		0x60
#define COLOR_GRAY_B		0x70

#define COLOR_BMASK			0x70
#define COLOR_BSHIFT		4

#define COLOR_BLACK_F		0x00
#define COLOR_BLUE_F		0x01
#define COLOR_GREEN_F		0x02
#define COLOR_CYAN_F		0x03
#define COLOR_RED_F			0x04
#define COLOR_MAGENTA_F		0x05
#define COLOR_BROWN_F		0x06
#define COLOR_GRAY_F		0x07
#define COLOR_DGRAY_F		0x08
#define COLOR_LBLUE_F		0x09
#define COLOR_LGREEN_F		0x0a
#define COLOR_LCYAN_F		0x0b
#define COLOR_LRED_F		0x0c
#define COLOR_LMAGENTA_F	0x0d
#define COLOR_YELLOW_F		0x0e
#define COLOR_WHITE_F		0x0f

#define COLOR_FMASK			0x07
#define COLOR_FSHIFT		0

#define COLOR_BOLDMASK		0x08

#define COLOR_CNT			8

/* These are the color table indices for win_attrset(); to the right
   in comment brackets are the default values for monochrome palette */

typedef enum {
	ATTR_NONE=-1,
	ATTR_WARNING,			/* A_REVERSE */

	ATTR_TITLE,				/* A_REVERSE */
	ATTR_BANNER,			/* A_NORMAL */
	ATTR_SONG_STATUS,		/* A_NORMAL */
	ATTR_INFO_INACTIVE,		/* A_REVERSE */
	ATTR_INFO_ACTIVE,		/* A_NORMAL */
	ATTR_INFO_IHOTKEY,		/* A_NORMAL */
	ATTR_INFO_AHOTKEY,		/* A_NORMAL */

	ATTR_HELP,				/* A_NORMAL */
	ATTR_PLAYENTRY_INACTIVE,/* A_NORMAL */
	ATTR_PLAYENTRY_ACTIVE,	/* A_REVERSE */

	ATTR_SAMPLES,			/* A_NORMAL */
	ATTR_SAMPLES_KICK3,		/* A_BOLD */
	ATTR_SAMPLES_KICK2,		/* A_NORMAL */
	ATTR_SAMPLES_KICK1,		/* A_NORMAL */
	ATTR_SAMPLES_KICK0,		/* A_NORMAL */

	ATTR_CONFIG,			/* A_NORMAL */

	ATTR_VOLBAR,			/* A_NORMAL */
	ATTR_VOLBAR_LOW,		/* A_NORMAL */
	ATTR_VOLBAR_MED,		/* A_NORMAL */
	ATTR_VOLBAR_HIGH,		/* A_BOLD */
	ATTR_VOLBAR_INSTR,		/* A_NORMAL */

	ATTR_MENU_FRAME,		/* A_REVERSE */
	ATTR_MENU_INACTIVE,		/* A_REVERSE */
	ATTR_MENU_ACTIVE,		/* A_NORMAL */
	ATTR_MENU_IHOTKEY,		/* A_NORMAL */
	ATTR_MENU_AHOTKEY,		/* A_REVERSE */

	ATTR_DLG_FRAME,			/* A_REVERSE */
	ATTR_DLG_LABEL,			/* A_REVERSE */
	ATTR_DLG_STR_TEXT,		/* A_NORMAL */
	ATTR_DLG_STR_CURSOR,	/* A_REVERSE */
	ATTR_DLG_BUT_INACTIVE,	/* A_REVERSE */
	ATTR_DLG_BUT_ACTIVE,	/* A_BOLD */
	ATTR_DLG_BUT_IHOTKEY,	/* A_NORMAL */
	ATTR_DLG_BUT_AHOTKEY,	/* A_REVERSE */
	ATTR_DLG_BUT_ITEXT,		/* A_REVERSE */
	ATTR_DLG_BUT_ATEXT,		/* A_BOLD */
	ATTR_DLG_LIST_FOCUS,	/* A_BOLD */
	ATTR_DLG_LIST_NOFOCUS,	/* A_NORMAL */

	ATTR_STATUS_LINE,		/* A_NORMAL */
	ATTR_STATUS_TEXT		/* A_NORMAL */
} ATTRS;

#define ATTRS_COUNT				((int)ATTR_STATUS_TEXT+1)

#define THEME_COLOR		0
#define THEME_MONO		1
#define THEME_COUNT		2		/* number of program intern themes */

#define THEME_NAME_LEN	99		/* max length of theme name */

extern const char *attrs_label[ATTRS_COUNT];	/* "WARNING", "TITLE", ... */

typedef struct {
	char *name;					/* name of the theme */
	BOOL color;					/* color or mono */
	int *attrs;					/* attributes for the different screen elements */
} THEME;

typedef struct {
	int location;				/* if < 0, file extensions are checked */
	char *marker;				/* signature or possible file extensions */
	char *list;
	int nameoffset;				/* position of file name in list output */
	char *extract;
	char *skippat;
	int skipstart, skipend;		/* lines to skip in the extracted file */
} ARCHIVE;

typedef struct {
	int driver;					/* nth driver for output */
#if LIBMIKMOD_VERSION >= 0x030107
	char *driveroptions;
#endif
	BOOL stereo;				/* mono/stereo output */
	BOOL mode_16bit;			/* 8/16 bit output */
	int frequency;				/* mixing frequency */
	BOOL interpolate;			/* Use interpolate mixing */
	BOOL hqmixer;				/* Use high-quality (but slow) mixer */
	BOOL surround;				/* surround mixing */
	int reverb;					/* reverb amount (0-15) */

	int volume;					/* volume from 0% (silence) to 100% */
	BOOL volrestrict;			/* restrict playervolume to volume supplied by user */
	BOOL fade;					/* allow volume fade at the end of the module */
	BOOL loop;					/* allow in-module loops */
	BOOL panning;				/* process panning effects */
	BOOL extspd;				/* extended protracker effects */

	int playmode;				/* PM_MODULE | PM_MULTI | PM_SHUFFLE | PM_RANDOM */
	BOOL curious;				/* look for hidden patterns in module */
	BOOL tolerant;				/* don't halt on file access errors */
	int renice;					/* RENICE_xxx */
	int statusbar;				/* size of statusbar */
	BOOL save_config;			/* save config on exit */
	BOOL save_playlist;			/* save playlist on exit */
	char *pl_name;				/* current playlist name */
	int cnt_hotlist;			/* size of next entry */
	char **hotlist;				/* entries in the directory hotlist */
	BOOL fullpaths;				/* display full path of the filenames */
#if LIBMIKMOD_VERSION >= 0x030200
	BOOL forcesamples;			/* always display sample names in bars panel */
	BOOL fakevolbars;			/* display fast, not accurate, volume bars */
#endif
	BOOL window_title;			/* set the title in xterm (or equivalent) */
	int theme;					/* active theme */
	int cnt_themes;				/* size of next entry */
	THEME *themes;				/* the known themes (color definitions) */
	int cnt_archiver;			/* size of next entry */
	ARCHIVE *archiver;			/* definition of archivers (lha tar, ...) */
} CONFIG;

extern CONFIG config;

char *CF_GetFilename(void);

void CF_theme_free (THEME *theme);
void CF_theme_copy (THEME*dest, THEME *src);

/* Free all themes and return {NULL, 0} */
void CF_themes_free (THEME **themes, int *cnt);
/* Free the user themes (themes above THEME_COUNT) */
void CF_themes_free_user (THEME **themes, int *cnt);
/* Free the theme at 'pos' in the array themes (length: cnt) */
void CF_theme_remove (int pos, THEME **themes, int *cnt);
/* Copy theme and insert it alphabetically sorted in themes
   (after the intern themes). cnt: size of the array themes
   Return: position of insertion */
int CF_theme_insert (THEME **themes, int *cnt, THEME *theme);

void CF_string_array_insert (int pos, char ***value, int *cnt,
							 char *arg, int length);
void CF_string_array_remove (int pos, char ***value, int *cnt);

void CF_Init(CONFIG * cfg);
BOOL CF_Save(CONFIG * cfg);
BOOL CF_Load(CONFIG * cfg);

void Player_SetConfig(CONFIG * cfg);

#endif

/* ex:set ts=4: */
