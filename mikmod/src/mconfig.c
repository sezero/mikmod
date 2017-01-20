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

  $Id: mconfig.c,v 1.3 2004/01/29 17:36:13 raph Exp $

  Configuration file management

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "player.h"
#include "mconfig.h"
#include "mwindow.h"
#include "mlist.h"
#include "mutilities.h"
#include "rcfile.h"

static LABEL_CONV renice_conv[] = {
	{RENICE_NONE, "RENICE_NONE"},
	{RENICE_PRI, "RENICE_PRI"},
	{RENICE_REAL, "RENICE_REAL"},
	{-1, NULL}
};

static LABEL_CONV attrs_mono_conv[] = {
	{A_NORMAL, "normal"},
	{A_BOLD, "bold"},
	{A_REVERSE, "reverse"},
	{-1, NULL}
};

static const char *attrs_colf_label[] = {
	"black","blue","green","cyan","red","magenta","brown","gray",
	"b_black","b_blue","b_green","b_cyan","b_red","b_magenta",
	"yellow","white",
	NULL
};

static const char *attrs_colb_label[] = {
	"black","blue","green","cyan","red","magenta","brown","gray",
	NULL
};

const char *attrs_label[ATTRS_COUNT] = {
	"WARNING",

	"TITLE",
	"BANNER",
	"SONG_STATUS",
	"INFO_INACTIVE",
	"INFO_ACTIVE",
	"INFO_IHOTKEY",
	"INFO_AHOTKEY",

	"HELP",
	"PLAYENTRY_INACTIVE",
	"PLAYENTRY_ACTIVE",

	"SAMPLES",
	"SAMPLES_KICK3",
	"SAMPLES_KICK2",
	"SAMPLES_KICK1",
	"SAMPLES_KICK0",

	"CONFIG",

	"VOLBAR",
	"VOLBAR_LOW",
	"VOLBAR_MED",
	"VOLBAR_HIGH",
	"VOLBAR_INSTR",

	"MENU_FRAME",
	"MENU_INACTIVE",
	"MENU_ACTIVE",
	"MENU_IHOTKEY",
	"MENU_AHOTKEY",

	"DLG_FRAME",
	"DLG_LABEL",
	"DLG_STR_TEXT",
	"DLG_STR_CURSOR",
	"DLG_BUT_INACTIVE",
	"DLG_BUT_ACTIVE",
	"DLG_BUT_IHOTKEY",
	"DLG_BUT_AHOTKEY",
	"DLG_BUT_ITEXT",
	"DLG_BUT_ATEXT",
	"DLG_LIST_FOCUS",
	"DLG_LIST_NOFOCUS",

	"STATUS_LINE",
	"STATUS_TEXT"
};

/*========== Color scheme */

static int color_attributes[ATTRS_COUNT] = {
	COLOR_RED_B | COLOR_WHITE_F,	/* ATTR_WARNING */

	COLOR_CYAN_B | COLOR_WHITE_F,	/* ATTR_TITLE */
	COLOR_BLACK_B | COLOR_LGREEN_F,	/* ATTR_BANNER */
	COLOR_BLUE_B | COLOR_WHITE_F,	/* ATTR_SONG_STATUS */
	COLOR_CYAN_B | COLOR_BLUE_F,	/* ATTR_INFO_INACTIVE */
	COLOR_BLACK_B | COLOR_WHITE_F,	/* ATTR_INFO_ACTIVE */
	COLOR_CYAN_B | COLOR_YELLOW_F,	/* ATTR_INFO_IHOTKEY */
	COLOR_BLACK_B | COLOR_YELLOW_F,	/* ATTR_INFO_AHOTKEY */

	COLOR_BLACK_B | COLOR_BROWN_F,	/* ATTR_HELP */
	COLOR_BLACK_B | COLOR_CYAN_F,	/* ATTR_PLAYENTRY_INACTIVE */
	COLOR_BLACK_B | COLOR_LCYAN_F,	/* ATTR_PLAYENTRY_ACTIVE */

	COLOR_BLACK_B | COLOR_CYAN_F,	/* ATTR_SAMPLES */
	COLOR_BLACK_B | COLOR_WHITE_F,	/* ATTR_SAMPLES_KICK3 */
	COLOR_BLACK_B | COLOR_LCYAN_F,	/* ATTR_SAMPLES_KICK2 */
	COLOR_BLACK_B | COLOR_LBLUE_F,	/* ATTR_SAMPLES_KICK1 */
	COLOR_BLACK_B | COLOR_BLUE_F,	/* ATTR_SAMPLES_KICK0 */

	COLOR_BLACK_B | COLOR_CYAN_F,	/* ATTR_CONFIG */

	COLOR_BLACK_B | COLOR_CYAN_F,	/* ATTR_VOLBAR */
	COLOR_BLACK_B | COLOR_LGREEN_F,	/* ATTR_VOLBAR_LOW */
	COLOR_BLACK_B | COLOR_YELLOW_F,	/* ATTR_VOLBAR_MED */
	COLOR_BLACK_B | COLOR_LRED_F,	/* ATTR_VOLBAR_HIGH */
	COLOR_BLACK_B | COLOR_GREEN_F,	/* ATTR_VOLBAR_INSTR */

	COLOR_CYAN_B | COLOR_BLACK_F,	/* ATTR_MENU_FRAME */
	COLOR_CYAN_B | COLOR_BLACK_F,	/* ATTR_MENU_INACTIVE */
	COLOR_BLACK_B | COLOR_WHITE_F,	/* ATTR_MENU_ACTIVE */
	COLOR_CYAN_B | COLOR_YELLOW_F,	/* ATTR_MENU_IHOTKEY */
	COLOR_BLACK_B | COLOR_YELLOW_F,	/* ATTR_MENU_AHOTKEY */

	COLOR_GRAY_B | COLOR_BLACK_F,	/* ATTR_DLG_FRAME */
	COLOR_GRAY_B | COLOR_BLUE_F,	/* ATTR_DLG_LABEL */
	COLOR_BLACK_B | COLOR_WHITE_F,	/* ATTR_DLG_STR_TEXT */
	COLOR_CYAN_B | COLOR_BLACK_F,	/* ATTR_DLG_STR_CURSOR */
	COLOR_CYAN_B | COLOR_GRAY_F,	/* ATTR_DLG_BUT_INACTIVE */
	COLOR_BLACK_B | COLOR_WHITE_F,	/* ATTR_DLG_BUT_ACTIVE */
	COLOR_CYAN_B | COLOR_YELLOW_F,	/* ATTR_DLG_BUT_IHOTKEY */
	COLOR_BLACK_B | COLOR_YELLOW_F,	/* ATTR_DLG_BUT_AHOTKEY */
	COLOR_CYAN_B | COLOR_BLACK_F,	/* ATTR_DLG_BUT_ITEXT */
	COLOR_BLACK_B | COLOR_WHITE_F,	/* ATTR_DLG_BUT_ATEXT */
	COLOR_CYAN_B | COLOR_BLACK_F,	/* ATTR_DLG_LIST_FOCUS */
	COLOR_CYAN_B | COLOR_YELLOW_F,	/* ATTR_DLG_LIST_NOFOCUS */

	COLOR_BLACK_B | COLOR_LCYAN_F,	/* ATTR_STATUS_LINE */
	COLOR_BLACK_B | COLOR_CYAN_F	/* ATTR_STATUS_TEXT */
};

static int mono_attributes[ATTRS_COUNT] = {
	A_REVERSE,			/* ATTR_WARNING */

	A_REVERSE,			/* ATTR_TITLE */
	A_NORMAL,			/* ATTR_BANNER */
	A_NORMAL,			/* ATTR_SONG_STATUS */
	A_REVERSE,			/* ATTR_INFO_INACTIVE */
	A_NORMAL,			/* ATTR_INFO_ACTIVE */
	A_NORMAL,			/* ATTR_INFO_IHOTKEY */
	A_NORMAL,			/* ATTR_INFO_AHOTKEY */

	A_NORMAL,			/* ATTR_HELP */
	A_NORMAL,			/* ATTR_PLAYENTRY_INAVTIVE */
	A_REVERSE,			/* ATTR_PLAYENTRY_ACTIVE */

	A_NORMAL,			/* ATTR_SAMPLES */
	A_BOLD,				/* ATTR_SAMPLES_KICK3 */
	A_NORMAL,			/* ATTR_SAMPLES_KICK2 */
	A_NORMAL,			/* ATTR_SAMPLES_KICK1 */
	A_NORMAL,			/* ATTR_SAMPLES_KICK0 */

	A_NORMAL,			/* ATTR_CONFIG */

	A_NORMAL,			/* ATTR_VOLBAR */
	A_NORMAL,			/* ATTR_VOLBAR_LOW */
	A_NORMAL,			/* ATTR_VOLBAR_MED */
	A_BOLD,				/* ATTR_VOLBAR_HIGH */
	A_NORMAL,			/* ATTR_VOLBAR_INSTR */

	A_REVERSE,			/* ATTR_MENU_FRAME */
	A_REVERSE,			/* ATTR_MENU_INACTIVE */
	A_NORMAL,			/* ATTR_MENU_ACTIVE */
	A_NORMAL,			/* ATTR_MENU_IHOTKEY */
	A_REVERSE,			/* ATTR_MENU_AHOTKEY */

	A_REVERSE,			/* ATTR_DLG_FRAME */
	A_REVERSE,			/* ATTR_DLG_LABEL */
	A_NORMAL,			/* ATTR_DLG_STR_TEXT */
	A_REVERSE,			/* ATTR_DLG_STR_CURSOR */
	A_REVERSE,			/* ATTR_DLG_BUT_INACTIVE */
	A_BOLD,				/* ATTR_DLG_BUT_ACTIVE */
	A_NORMAL,			/* ATTR_DLG_BUT_IHOTKEY */
	A_REVERSE,			/* ATTR_DLG_BUT_AHOTKEY */
	A_REVERSE,			/* ATTR_DLG_BUT_ITEXT */
	A_BOLD,				/* ATTR_DLG_BUT_ATEXT */
	A_BOLD,				/* ATTR_DLG_LIST_FOCUS */
	A_NORMAL,			/* ATTR_DLG_LIST_NOFOCUS */

	A_NORMAL,			/* ATTR_STATUS_LINE */
	A_NORMAL			/* ATTR_STATUS_TEXT */
};

/*========== default archiver */
/*
	The following table describes how MikMod should deal with archives.

	The first two fields are for identification. The code will consider that
	a given file is a recognized archive if a signature is found at a fixed
	location in the file. The first field is the offset into the archive of
	the signature, and the second field points to the signature to check.
	If the offset is negative, the extension of the file is matched against
	the parts of the second field.

	The third field contains the name of the program and its arguments to
	invoke to list the archive. Here %A is replaced with the archive name
	and %a with a short version of the archive name (for DOS and WIN) or
	the archive name.

	For the special case of mono-file archives (gzip and bzip2 compressed
	files, for example), set this field to NULL. In this case, the code
	will determine the contents of the file without having to invoke the
	list function of the archiver. This is necessary since bzip2 has no list
	function, and the only way to get the archive contents is to test it,
	which can be a really slow process.

	The fourth field is the column in the archive listing output where the
	filenames begin (starting from zero for the leftmost column). A good
	archiver will put them last on the line, so they can embed spaces and
	be as long as necessary.

	The fifth field contains the program and its arguments to extract the
	modules from archives. Here %A is replaced with the archive name, %a
	with a short version of the archive name (for DOS and WIN) or the
	archive name, %f with the file name, and %d with the destination name
	(for non UNIX systems only).

	The last three fields specify which part to use from the extracted file
	(if the extraction program mixes status information and the module).
	The first skipstart lines starting from the first occurence of skippat
	and the last skipend lines from the extracted file are removed.
*/

/* use similar signature idea to "file" to see what format we have... */
static char pksignat[] = "PK\x03\x04";
static char zoosignat[] = "\xdc\xa7\xc4\xfd";
static char rarsignat[] = "Rar!";
static char gzsignat[] = "\x1f\x8b";
static char bzip2signat[] = "BZh";
static char tarsignat[] = "ustar";
static char lhsignat[] = "-lh";
static char lzsignat[] = "-lz";

/* interesting file extensions */
static char targzext[] = ".TAR.GZ .TAZ .TGZ";
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
static char tarbzip2ext[] = ".TAR.BZ2 .TBZ .TBZ2";
#endif

static ARCHIVE archiver_def[] = {
	/* location, marker, list, filenames column, extract, skippat, skipstart, skipend */
#ifdef _mikmod_amiga
	{  0,  pksignat, "unzip -vqq \"%a\" > \"%d\"",
				 58, "unzip -pqq \"%a\" \"%f\" > \"%d\"", NULL, 0, 0},
	{ 20, zoosignat, "zoo lq \"%a\" > \"%d\"",
				 47, "zoo xpq \"%a\" \"%f\" > \"%d\"", NULL, 0, 0},
	{  0, rarsignat, "unrar v -c- \"%a\" > \"%d\"",
				  1, "unrar p -inul \"%a\" \"%f\" > \"%d\"", NULL, 0, 0},
	{  2,  lhsignat, "lha vvq \"%a\" > \"%d\"",
				 -1, "lha pq \"%a\" \"%f\" > \"%d\"", NULL, 0, 0},
	{  2,  lzsignat, "lha vvq \"%a\" > \"%d\"",
				 -1, "lha pq \"%a\" \"%f\" > \"%d\"", NULL, 0, 0},
	{257, tarsignat, "tar -tf \"%a\" > \"%d\"",
				  0, "tar -xOf \"%a\" \"%f\" > \"%d\"", NULL, 0, 0},
	{ -1,  targzext, "tar -tzf \"%a\" > \"%d\"",
				  0, "tar -xOzf \"%a\" \"%f\" > \"%d\"", NULL, 0, 0},
	{ -1, tarbzip2ext, "tar --use-compress-program=bzip2 -tf \"%a\" > \"%d\"",
				    0, "tar --use-compress-program=bzip2 -xOf \"%a\" \"%f\" > \"%d\"", NULL, 0, 0},
	{  0,    gzsignat, NULL, 0, "gzip -dqc \"%a\" > \"%d\"", NULL, 0, 0},
	{  0, bzip2signat, NULL, 0, "bzip2 -dqc \"%a\" > \"%d\"", NULL, 0, 0}
#elif !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
	{  0,  pksignat, "unzip -vqq \"%a\"",
				 58, "unzip -pqq \"%a\" \"%f\"", NULL, 0, 0},
	{ 20, zoosignat, "zoo lq \"%a\"",
				 47, "zoo xpq \"%a\" \"%f\"", NULL, 0, 0},
	{  0, rarsignat, "unrar v -c- \"%a\"",
				  1, "unrar p -inul \"%a\" \"%f\"", NULL, 0, 0},
	{  2,  lhsignat, "lha vvq \"%a\"",
				 -1, "lha pq \"%a\" \"%f\"", NULL, 0, 0},
	{  2,  lzsignat, "lha vvq \"%a\"",
				 -1, "lha pq \"%a\" \"%f\"", NULL, 0, 0},
	{257, tarsignat, "tar -tf \"%a\"",
				  0, "tar -xOf \"%a\" \"%f\"", NULL, 0, 0},
	{ -1,  targzext, "tar -tzf \"%a\"",
				  0, "tar -xOzf \"%a\" \"%f\"", NULL, 0, 0},
	{ -1, tarbzip2ext, "tar --use-compress-program=bzip2 -tf \"%a\"",
				    0, "tar --use-compress-program=bzip2 -xOf \"%a\" \"%f\"", NULL, 0, 0},
	{  0,    gzsignat, NULL, 0, "gzip -dqc \"%a\"", NULL, 0, 0},
	{  0, bzip2signat, NULL, 0, "bzip2 -dqc \"%a\"", NULL, 0, 0}
#else
/*	{  0,  pksignat, "unzip -lqq \"%a\"",
				 41, "unzip -pqq \"%a\" \"%f\" >\"%d\"", NULL, 0, 0},
	{  0, rarsignat, "unrar v -c- \"%a\"",
				  1, "unrar p -inul \"%a\" \"%f\" >\"%d\"", NULL, 0, 0},
	{257, tarsignat, "tar -tf \"%a\"",
				  0, "tar -xOf \"%a\" \"%f\" >\"%d\"", NULL, 0, 0},
*/
	{  0,  pksignat, "pkunzip -vb \"%a\"",
				 47, "pkunzip -c \"%a\" \"%f\" >\"%d\"", "to console", 2, 1},
	{ 20, zoosignat, "zoo lq \"%a\"",
				 47, "zoo xpq \"%a\" \"%f\" >\"%d\"", NULL, 0, 0},
	{  0, rarsignat, "rar v -y -c- \"%a\"",
				  1, "rar p -y -c- \"%a\" \"%f\" >\"%d\"", "--- Printing ", 2, 2},
	{  2,  lhsignat, "lha v %a",
				 -1, "lha p /n %a %f >\"%d\"", NULL, 3, 0},
	{  2,  lzsignat, "lha v %a",
				 -1, "lha p /n %a %f >\"%d\"", NULL, 3, 0},
	{257, tarsignat, "djtar -t \"%A\"",
				 36, "djtar -x -p -b -o \"%f\" \"%A\" >\"%d\"", NULL, 0, 0},
	{ -1,  targzext, "djtar -t \"%A\"",
				 36, "djtar -x -p -b -o \"%f\" \"%A\" >\"%d\"", NULL, 0, 0},
	{  0,  gzsignat, NULL, 27, "gzip -dqc \"%a\" >\"%d\"", NULL, 0, 0},
	{  0, bzip2signat, NULL, 0, "bzip2 -dqc \"%a\" >\"%d\"", NULL, 0, 0}
#endif
};

#define CNT_ARCHIVER_DEF (sizeof(archiver_def)/sizeof(archiver_def[0]))

char *CF_GetFilename(void)
{
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)||defined(_mikmod_amiga)
	return get_cfg_name("mikmod.cfg");
#else
	return get_cfg_name(".mikmodrc");
#endif
}

char *CF_GetDefaultFilename(void)
{
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)||defined(_mikmod_amiga)
	return NULL;
#else
	return str_sprintf2("%s" PATH_SEP_STR "%s", PACKAGE_DATA_DIR, "mikmodrc");
#endif
}

static void init_themes(CONFIG *cfg)
{
	cfg->cnt_themes = THEME_COUNT;
	cfg->themes = (THEME *) malloc (sizeof(THEME)*cfg->cnt_themes);
	cfg->themes[THEME_COLOR].name = "<defaultColor>";
	cfg->themes[THEME_COLOR].color = 1;
	cfg->themes[THEME_COLOR].attrs = color_attributes;
	cfg->themes[THEME_MONO].name = "<defaultMono>";
	cfg->themes[THEME_MONO].color = 0;
	cfg->themes[THEME_MONO].attrs = mono_attributes;
	cfg->theme = THEME_COLOR;
}

static void write_theme(THEME *theme)
{
	int i;
	rc_write_string("NAME", theme->name, NULL);
	if (theme->color) {
		char str[30];
		for (i=0; i<ATTRS_COUNT; i++) {
			strcpy (str,attrs_colf_label
					[(theme->attrs[i] & (COLOR_FMASK+COLOR_BOLDMASK)) >> COLOR_FSHIFT]);
			strcat (str,",");
			strcat (str,attrs_colb_label
					[(theme->attrs[i] & COLOR_BMASK) >> COLOR_BSHIFT]);
			rc_write_string(attrs_label[i], str, NULL);
		}
	} else {
		for (i=0; i<ATTRS_COUNT; i++)
			rc_write_label(attrs_label[i], attrs_mono_conv, theme->attrs[i],NULL);
	}
}

void CF_theme_free (THEME *theme)
{
	if (theme) {
		if (theme->name)
			free (theme->name);
		if (theme->attrs)
			free (theme->attrs);
	}
}

void CF_theme_copy (THEME *dest, THEME *src)
{
	dest->color = src->color;
	dest->name = strdup (src->name);
	dest->attrs = (int *) malloc (sizeof(int)*ATTRS_COUNT);
	memcpy (dest->attrs,src->attrs,sizeof(int)*ATTRS_COUNT);
}

/* Free all themes and return {NULL, 0} */
void CF_themes_free (THEME **themes, int *cnt)
{
	if (themes && *themes) {
		int i;
		for (i=0; i<*cnt; i++)
			CF_theme_free (&(*themes)[i]);
		free (*themes);
	}
	*cnt = 0;
	if (themes) *themes = NULL;
}

/* Free the user themes (themes above THEME_COUNT) */
void CF_themes_free_user (THEME **themes, int *cnt)
{
	if (themes && *themes) {
		int i;
		for (i=THEME_COUNT; i<*cnt; i++)
			CF_theme_free (&(*themes)[i]);
		*cnt = THEME_COUNT;
		*themes = (THEME *) realloc (*themes, sizeof(THEME)*(*cnt));
	}
}

/* Free the theme at 'pos' in the array themes (length: cnt) */
void CF_theme_remove (int pos, THEME **themes, int *cnt)
{
	int i;

	if (*cnt>0) {
		(*cnt)--;
		if (*themes)
			CF_theme_free (&(*themes)[pos]);
		if (*cnt>0) {
			for (i=pos; i<*cnt; i++)
				(*themes)[i] = (*themes)[i+1];
			*themes = (THEME *) realloc (*themes, sizeof(THEME)*(*cnt));
		} else {
			free (*themes);
			*themes = NULL;
		}
	}
}

/* Copy theme and insert it alphabetically sorted in themes
   (after the intern themes). cnt: size of the array themes
   Return: position of insertion */
int CF_theme_insert (THEME **themes, int *cnt, THEME *theme)
{
	int i, pos = *cnt;

	if (*cnt >= THEME_COUNT) {
		pos = THEME_COUNT;
		while (pos<*cnt && strcasecmp((*themes)[pos].name,theme->name) < 0)
			pos++;
	}

	(*cnt)++;
	*themes = (THEME *) realloc (*themes,sizeof(THEME)*(*cnt));
	for (i=*cnt-1; i>pos; i--)
		(*themes)[i] = (*themes)[i-1];

	CF_theme_copy (&(*themes)[pos], theme);
	return pos;
}

static void read_theme(CONFIG *cfg)
{
	int i, fg, bg;
	int attrs[ATTRS_COUNT];
	THEME theme = {NULL,-1,NULL};
	char *str = NULL, *pos, *end;

	theme.attrs = attrs;
	if (!rc_read_string("NAME", &theme.name, THEME_NAME_LEN))
		return;

	for (i=0; i<ATTRS_COUNT; i++) {
		if (rc_read_string(attrs_label[i],&str,99)) {
			pos = str;
			while (isspace((int)*pos)) pos++;
			end = pos;
			while (isalnum((int)*end) || *end == '_') end++;
			if (*end) *end++ = '\0';
			fg = 0;
			while (attrs_mono_conv[fg].label &&
				   strcasecmp(attrs_mono_conv[fg].label,pos)) fg++;
			if (attrs_mono_conv[fg].label) {
				if (theme.color != 1) {
					if (theme.color == -1)
						memcpy (attrs,mono_attributes,sizeof(int)*ATTRS_COUNT);
					theme.attrs[i] = attrs_mono_conv[fg].id;
					theme.color = 0;
				}
			} else if (*end) {
				fg = 0;
				while (attrs_colf_label[fg] &&
					   strcasecmp(attrs_colf_label[fg],pos)) fg++;
				if (attrs_colf_label[fg]) {
					while (!isalnum((int)*end) && *end != '_') end++;
					pos = end;
					while (isalnum((int)*end) || *end == '_') end++;
					*end = '\0';
					bg = 0;
					while (attrs_colb_label[bg] &&
						   strcasecmp(attrs_colb_label[bg],pos)) bg++;
					if (attrs_colb_label[bg] && theme.color != 0) {
						if (theme.color == -1)
							memcpy (attrs,color_attributes,sizeof(int)*ATTRS_COUNT);
						theme.color = 1;
						theme.attrs[i] = (fg << COLOR_FSHIFT) + (bg << COLOR_BSHIFT);
					}
				}
			}
		}
	}
	if (str) free (str);
	CF_theme_insert (&cfg->themes, &cfg->cnt_themes, &theme);
	free (theme.name);
}

static void write_archiver(ARCHIVE *archiver)
{
	rc_write_int("LOCATION", archiver->location, NULL);
	rc_write_string("MARKER", archiver->marker, NULL);
	rc_write_string("LIST", archiver->list, NULL);
	rc_write_int("NAMEOFFSET", archiver->nameoffset, NULL);
	rc_write_string("EXTRACT", archiver->extract, NULL);
	rc_write_string("SKIPPAT", archiver->skippat, NULL);
	rc_write_int("SKIPSTART", archiver->skipstart, NULL);
	rc_write_int("SKIPEND", archiver->skipend, NULL);
}

static void read_archiver(CONFIG *cfg)
{
	ARCHIVE arch;

	memset (&arch, 0, sizeof(ARCHIVE));
	if (!rc_read_int("LOCATION", &arch.location, -1, 999))
		return;
	rc_read_string("MARKER", &arch.marker, 999);
	rc_read_string("LIST", &arch.list, PATH_MAX+200);
	rc_read_int("NAMEOFFSET", &arch.nameoffset, -1, 999);
	rc_read_string("EXTRACT", &arch.extract, PATH_MAX+200);
	rc_read_string("SKIPPAT", &arch.skippat, 999);
	rc_read_int("SKIPSTART", &arch.skipstart, 0, 999);
	rc_read_int("SKIPEND", &arch.skipend, 0, 999);

	if (cfg->archiver == archiver_def) {
		cfg->cnt_archiver = 1;
		cfg->archiver = (ARCHIVE *) malloc (sizeof(ARCHIVE));
	} else {
		cfg->cnt_archiver++;
		cfg->archiver = (ARCHIVE *) realloc (cfg->archiver, sizeof(ARCHIVE)*cfg->cnt_archiver);
	}
	cfg->archiver[cfg->cnt_archiver-1] = arch;
}

void CF_Init(CONFIG *cfg)
{
	cfg->driver = 0;
#if LIBMIKMOD_VERSION >= 0x030107
	rc_set_string(&cfg->driveroptions, "", 255);
#endif
	cfg->stereo = 1;
	cfg->mode_16bit = 1;
	cfg->frequency = 44100;
	cfg->interpolate = 1;
	cfg->hqmixer = 0;
	cfg->surround = 0;
	cfg->reverb = 0;

	cfg->volume = 100;
	cfg->volrestrict = 0;
	cfg->fade = 0;
	cfg->loop = 0;
	cfg->panning = 1;
	cfg->extspd = 1;

	cfg->playmode = PM_MULTI;
	cfg->curious = 0;
	cfg->tolerant = 1;
	cfg->renice = RENICE_NONE;
	cfg->statusbar = 2;

	cfg->save_config = 1;
	cfg->save_playlist = 1;
	rc_set_string(&cfg->pl_name, "playlist.mpl", PATH_MAX);
	cfg->cnt_hotlist = 0;
	cfg->hotlist = NULL;
	cfg->fullpaths = 0;
#if LIBMIKMOD_VERSION >= 0x030200
	cfg->forcesamples = 0;
	cfg->fakevolbars = 1;
#endif
	cfg->window_title = 1; 
	init_themes (cfg);
	cfg->cnt_archiver = CNT_ARCHIVER_DEF;
	cfg->archiver = archiver_def;
}

BOOL CF_Save(CONFIG * cfg)
{
	char *name;
	int i;

	if (!(name = CF_GetFilename()))
		return 0;

	if (!rc_save (name,mikversion)) {
		free(name);
		rc_close();
		return 0;
	}
	free(name);

	rc_write_int("DRIVER", cfg->driver,
				 "DRIVER = <val>, nth driver for output, default: 0\n");
#if LIBMIKMOD_VERSION >= 0x030107
	rc_write_string("DRV_OPTIONS", cfg->driveroptions,
					"DRV_OPTIONS = \"options\", the driver options, e.g. \"buffer=14,count=16\"\n"
					"                         for the OSS-driver\n");
#endif
	rc_write_bool("STEREO", cfg->stereo,
				  "STEREO = Yes|No, stereo or mono output, default: stereo\n");
	rc_write_bool("16BIT", cfg->mode_16bit,
				  "16BIT = Yes|No, 8 or 16 bit output, default: 16 bit\n");
	rc_write_int("FREQUENCY", cfg->frequency,
				 "FREQUENCY = <val>, mixing frequency, default: 44100 Hz\n");
	rc_write_bool("INTERPOLATE", cfg->interpolate,
				  "INTERPOLATE = Yes|No, use interpolate mixing, default: Yes\n");
	rc_write_bool("HQMIXER", cfg->hqmixer,
				  "HQMIXER = Yes|No, use high-quality (but slow) software mixer, default: No\n");
	rc_write_bool("SURROUND", cfg->surround,
				  "SURROUND = Yes|No, use surround mixing, default: No\n");
	rc_write_int("REVERB", cfg->reverb,
				 "REVERB = <val>, set reverb amount (0-15), default: 0 (none)\n");

	rc_write_int("VOLUME", cfg->volume,
				 "VOLUME = <val>, volume from 0 (silence) to 100, default: 100\n");
	rc_write_bool("VOLRESTRICT", cfg->volrestrict,
				  "VOLRESTRICT = Yes|No, restrict volume of player to volume supplied by user,\n"
				  "                      default: No\n");
	rc_write_bool("FADEOUT", cfg->fade,
				  "FADEOUT = Yes|No, volume fade at the end of the module, default: No\n");
	rc_write_bool("LOOP", cfg->loop,
				  "LOOP = Yes|No, enable in-module loops, default: No\n");
	rc_write_bool("PANNING", cfg->panning,
				  "PANNING = Yes|No, process panning effects, default: Yes\n");
	rc_write_bool("EXTSPD", cfg->extspd,
				  "EXTSPD = Yes|No, process Protracker extended speed effect, default: Yes\n");

	rc_write_bool("PM_MODULE", BTST(cfg->playmode, PM_MODULE),
				  "PM_MODULE = Yes|No, Module repeats, default: No\n");
	rc_write_bool("PM_MULTI", BTST(cfg->playmode, PM_MULTI),
				  "PM_MULTI = Yes|No, PlayList repeats, default: Yes\n");
	rc_write_bool("PM_SHUFFLE", BTST(cfg->playmode, PM_SHUFFLE),
				  "PM_SHUFFLE = Yes|No, Shuffle list at start and if all entries are played,\n"
				  "                     default: No\n");
	rc_write_bool("PM_RANDOM", BTST(cfg->playmode, PM_RANDOM),
				  "PM_RANDOM = Yes|No, PlayList in random order, default: No\n");

	rc_write_bool("CURIOUS", cfg->curious,
				  "CURIOUS = Yes|No, look for hidden patterns in module, default: No\n");
	rc_write_bool("TOLERANT", cfg->tolerant,
				  "TOLERANT = Yes|No, don't halt on file access errors, default: Yes\n");
	rc_write_label("RENICE", renice_conv, cfg->renice,
				   "RENICE = RENICE_NONE (change nothing), RENICE_PRI (Renice to -20) or\n"
				   "         RENICE_REAL (get realtime priority), default: RENICE_NONE\n"
				   "  Note that RENICE_PRI is only available under FreeBSD, Linux, NetBSD,\n"
				   "  OpenBSD and OS/2, and RENICE_REAL is only available under FreeBSD, Linux\n"
				   "  and OS/2.\n");
	rc_write_int("STATUSBAR", cfg->statusbar,
				 "STATUSBAR = <val>, size of statusbar from 0 to 2, default: 2\n");

	rc_write_bool("SAVECONFIG", cfg->save_config,
				  "SAVECONFIG = Yes|No, save configuration on exit, default: Yes\n");
	rc_write_bool("SAVEPLAYLIST", cfg->save_playlist,
				  "SAVEPLAYLIST = Yes|No, save playlist on exit, default: Yes\n");

	rc_write_string("PL_NAME", cfg->pl_name,
					"PL_NAME = \"name\", name under which the playlist will be saved\n"
					"                  by selecting 'Save' in the playlist-menu\n");
	if (cfg->cnt_hotlist > 0) {
		rc_write_string("HOTLIST", cfg->hotlist[0],
						"HOTLIST = \"name\", entries in the directory hotlist,\n"
						"                  can occur any time in this file\n");
		for (i=1; i<cfg->cnt_hotlist; i++)
			rc_write_string("HOTLIST",cfg->hotlist[i],NULL);
	}
	rc_write_bool("FULLPATHS", cfg->fullpaths,
				  "FULLPATHS = Yes|No, display full path of files, default: Yes\n");
#if LIBMIKMOD_VERSION >= 0x030200
	rc_write_bool("FORCESAMPLES", cfg->forcesamples,
				  "FORCESAMPLES = Yes|No, always display sample names (instead of\n"
				  "    instrument names) in volumebars panel, default: No\n");
	rc_write_bool("FAKEVOLUMEBARS", cfg->fakevolbars,
				  "FAKEVOLUMEBARS = Yes|No, display fast, but not always accurate, volumebars\n"
				  "    in volumebars panel, default: Yes\n"
				  "    The real volumebars (when this setting is \"No\") take some CPU time to\n"
				  "    be computed, and don't work with every driver.\n");
#endif
	rc_write_bool("WINDOWTITLE", cfg->window_title,
	              "WINDOWTITLE = Yes|No, set the term/window title to song name\n"
				  "    (or filename if song has no title), default: Yes\n");

	rc_write_string ("THEME",cfg->themes[cfg->theme].name,
					 "THEME = \"name\", name of the theme to use, default: <defaultColor>");
	if (cfg->cnt_themes>THEME_COUNT) {
		rc_write_struct ("THEME",
						 "Definition of the themes\n"
						 "  NAME = \"name\", specifies the name of the theme\n"
						 "  <screen_element> = normal | bold | reverse  , for mono themes or\n"
						 "  <screen_element> = <fgcolor>,<bgcolor>      , for color themes\n"
						 "    where <fgcolor> = black | blue | green | cyan | red | magenta |\n"
						 "                      brown | gray | b_black | b_blue | b_green |\n"
						 "                      b_cyan | b_red | b_magenta | yellow | white\n"
						 "          <bgcolor> = black | blue | green | cyan | red | magenta |\n"
						 "                      brown | gray\n");
		write_theme (&cfg->themes[THEME_COUNT]);
		rc_write_struct_end (NULL);
		for (i=THEME_COUNT+1; i<cfg->cnt_themes; i++) {
			rc_write_struct ("THEME",NULL);
			write_theme (&cfg->themes[i]);
			rc_write_struct_end (NULL);
		}
	}

	if (cfg->cnt_archiver > 0) {
		rc_write_struct ("ARCHIVER",
			 "Definition of the archiver\n"
			 "  LOCATION = <val>, -1: MARKER gives list of possible file extensions\n"
			 "             otherwise: location where MARKER must be found in the file\n"
			 "  MARKER = <string>, see LOCATION, e.g. \".TAR.GZ .TGZ\" or \"PK\\x03\\x04\"\n"
			 "  LIST = <command>, command to list archive content (%A archive name,\n"
			 "                    %a short(DOS/WIN) archive name)\n"
			 "  NAMEOFFSET = <val>, column where file names begin,\n"
			 "               -1: start at column 0 and end at first space\n"
			 "  EXTRACT = <command>, command to extract a file to stdout (%A archive name,\n"
			 "             %a short archive name, %f file name, %d destination name(non UNIX))\n"
			 "  SKIPPAT = <string>, Remove the first SKIPSTART lines starting from the first\n"
			 "                      occurence of SKIPPAT and the last SKIPEND lines from the\n"
			 "                      extracted file (if the command EXTRACT mixes status\n"
			 "                      information and the module).\n"
			 "  SKIPSTART = <val>, \n"
			 "  SKIPEND = <val>, \n");
		write_archiver (&cfg->archiver[0]);
		rc_write_struct_end (NULL);
		for (i=1; i<cfg->cnt_archiver; i++) {
			rc_write_struct ("ARCHIVER",NULL);
			write_archiver (&cfg->archiver[i]);
			rc_write_struct_end (NULL);
		}
	}
	rc_close();
	return 1;
}

void CF_string_array_insert (int pos, char ***value, int *cnt,
							 char *arg, int length)
{
	int i;

	(*cnt)++;
	*value = (char **) realloc (*value, sizeof(char*)*(*cnt));
	for (i=*cnt-1; i>pos; i--)
		(*value)[i] = (*value)[i-1];
	(*value)[pos] = NULL;
	rc_set_string (&(*value)[pos], arg, length);
}

void CF_string_array_remove (int pos, char ***value, int *cnt)
{
	int i;

	if (*cnt>0) {
		(*cnt)--;
		if (*value && (*value)[pos])
			free ((*value)[pos]);
		if (*cnt>0) {
			for (i=pos; i<*cnt; i++)
				(*value)[i] = (*value)[i+1];
			*value = (char **) realloc (*value, sizeof(char*)*(*cnt));
		} else {
			free (*value);
			*value = NULL;
		}
	}
}

BOOL CF_Load(CONFIG *cfg)
{
	char *name = CF_GetFilename(), *str = NULL;
	int i;

	if (!name)
		return 0;
	if (!rc_load(name)) {
		free(name);
		rc_close();

		name = CF_GetDefaultFilename();
		if (!name)
			return 0;
		if (!rc_load(name)) {
			free(name);
			rc_close();
			return 0;
		}
	}
	free(name);

	rc_read_int("DRIVER", &cfg->driver, 0, 999);
#if LIBMIKMOD_VERSION >= 0x030107
	rc_read_string("DRV_OPTIONS", &cfg->driveroptions, 255);
#endif
	rc_read_bool("STEREO", &cfg->stereo);
	rc_read_bool("16BIT", &cfg->mode_16bit);
	rc_read_int("FREQUENCY", &cfg->frequency, 4000, 60000);
	rc_read_bool("INTERPOLATE", &cfg->interpolate);
	rc_read_bool("HQMIXER", &cfg->hqmixer);
	rc_read_bool("SURROUND", &cfg->surround);
	rc_read_int("REVERB", &cfg->reverb, 0, 15);
	rc_read_int("VOLUME", &cfg->volume, 0, 100);
	rc_read_bool("VOLRESTRICT", &cfg->volrestrict);
	rc_read_bool("FADEOUT", &cfg->fade);
	rc_read_bool("LOOP", &cfg->loop);
	rc_read_bool("PANNING", &cfg->panning);
	rc_read_bool("EXTSPD", &cfg->extspd);
	rc_read_bit("PM_MODULE", &cfg->playmode, PM_MODULE);
	rc_read_bit("PM_MULTI",&cfg->playmode, PM_MULTI);
	rc_read_bit("PM_SHUFFLE", &cfg->playmode, PM_SHUFFLE);
	rc_read_bit("PM_RANDOM", &cfg->playmode, PM_RANDOM);
	rc_read_bool("CURIOUS", &cfg->curious);
	rc_read_bool("TOLERANT", &cfg->tolerant);
	rc_read_label("RENICE", &cfg->renice, renice_conv);
	rc_read_int("STATUSBAR", &cfg->statusbar, 0, 2);
	rc_read_bool("SAVECONFIG", &cfg->save_config);
	rc_read_bool("SAVEPLAYLIST", &cfg->save_playlist);
	rc_read_string("PL_NAME", &cfg->pl_name, PATH_MAX);
	path_conv(cfg->pl_name);

	while (rc_read_string("HOTLIST",&str,PATH_MAX)) {
		path_conv(str);
		CF_string_array_insert (cfg->cnt_hotlist, &cfg->hotlist,
								&cfg->cnt_hotlist, str, PATH_MAX);
	}

	rc_read_bool("FULLPATHS", &cfg->fullpaths);
#if LIBMIKMOD_VERSION >= 0x030200
	rc_read_bool("FORCESAMPLES", &cfg->forcesamples);
	rc_read_bool("FAKEVOLUMEBARS", &cfg->fakevolbars);
#endif
	rc_read_bool("WINDOWTITLE", &cfg->window_title);
	
	while (rc_read_struct("THEME")) {
		read_theme (cfg);
		rc_read_struct_end();
	}
	if (rc_read_string("THEME", &str, THEME_NAME_LEN)) {
		for (i=0; i<cfg->cnt_themes; i++) {
			if (!strcasecmp(str,cfg->themes[i].name)) {
				cfg->theme = i;
				break;
			}
		}
	}

	while (rc_read_struct("ARCHIVER")) {
		read_archiver (cfg);
		rc_read_struct_end();
	}

	free (str);
	rc_close();
	return 1;
}

/* ex:set ts=4: */
