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

  $Id: mconfedit.c,v 1.2 2004/01/29 17:36:13 raph Exp $

  The config editor

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <mikmod.h>
#include "rcfile.h"
#include "mconfig.h"
#include "mconfedit.h"
#include "mlist.h"
#include "mmenu.h"
#include "mdialog.h"
#include "mutilities.h"

#define OPT_DRIVER		0
#if LIBMIKMOD_VERSION >= 0x030107
#define OPT_DRV_OPTION	1
#define OPT_STEREO		2
#define OPT_MODE_16BIT	3
#define OPT_FREQUENCY	4
#define OPT_INTERPOLATE	5
#define OPT_HQMIXER		6
#define OPT_SURROUND	7
#define OPT_REVERB		8
#else
#define OPT_STEREO		1
#define OPT_MODE_16BIT	2
#define OPT_FREQUENCY	3
#define OPT_INTERPOLATE	4
#define OPT_HQMIXER		5
#define OPT_SURROUND	6
#define OPT_REVERB		7
#endif

#define OPT_VOLUME		0
#define OPT_VOLRESTRICT	1
#define OPT_FADE		2
#define OPT_LOOP		3
#define OPT_PANNING		4
#define OPT_EXTSPD		5

#define OPT_PM_MODULE	0
#define OPT_PM_MULTI	1
#define OPT_PM_SHUFFLE	2
#define OPT_PM_RANDOM	3

#define OPT_CURIOUS		1
#define OPT_TOLERANT	2
#define OPT_FULLPATHS	3
#define OPT_EDITTHEME	4
#define OPT_THEME		5
#define OPT_WINDOWTITLE 6
#if LIBMIKMOD_VERSION >= 0x030200
#define OPT_SAMPLES		7
#define OPT_FAKEVOLBARS	8
#define OPT_RENICE		9
#define OPT_STATUSBAR	10
#else
#define OPT_RENICE		7
#define OPT_STATUSBAR	8
#endif

#define OPT_S_CONFIG	0
#define OPT_S_PLAYLIST	1

#define MENU_MAIN		0
#define MENU_OUTPUT		1
#define MENU_PLAYBACK	2
#define MENU_OTHER		3
#define MENU_USE		4
#define MENU_SAVE		5
#define MENU_REVERT		6

static void handle_menu(MMENU *menu);

#if LIBMIKMOD_VERSION >= 0x030107
static char driveroptions[100] = "";
#endif

static MENTRY output_entries[] = {
	{NULL, 0, "The device driver for output"},
#if LIBMIKMOD_VERSION >= 0x030107
	{NULL, driveroptions,
		"Driver options (e.g. \"buffer=14,count=16\" for the OSS-driver)"},
#endif
	{"[%c] &Stereo", 0, "mono/stereo output"},
	{"[%c] 16 &bit output", 0, "8/16 bit output"},
	{"&Frequency        [%d]|Enter mixing frequency:|4000|60000", 0,
	 "Mixing frequency in hertz (from 4000 Hz to 60000 Hz)"},
	{"[%c] &Interpolate", 0, "Use interpolated mixing"},
	{"[%c] &HQmixer", 0, "Use high-quality (but slower) software mixer"},
	{"[%c] S&urround", 0, "Use surround mixing"},
	{"&Reverb              [%d]|Enter reverb amount:|0|15", 0,
	 "Reverb amount from 0 (no reverb) to 15"},
	{NULL,NULL,NULL}
};
static MMENU output_menu =
	{ 0, 0, -1, 1, output_entries, handle_menu, NULL, NULL, 1 };

static MENTRY playback_entries[] = {
	{"&Volume     [%d]|Enter output volume:|0|100",
	 0, "Output volume from 0 to 100 in %"},
	{"[%c] &Restrict Volume", 0,
	 "Restrict volume of player to volume supplied by user (with 1..0,<,>)"},
	{"[%c] &Fadeout", 0, "Force volume fade at the end of module"},
	{"[%c] &Loops", 0, "Enable in-module loops"},
	{"[%c] &Panning", 0, "Process panning effects"},
	{"[%c] Pro&tracker", 0, "Use extended protracker effects"},
	{NULL,NULL,NULL}
};
static MMENU playback_menu =
	{ 0, 0, -1, 1, playback_entries, handle_menu, NULL, NULL, 2 };

static MENTRY plmode_entries[] = {
	{"[%c] Loop &module", 0, "Loop current module"},
	{"[%c] Loop &list", 0, "Play the list repeatedly"},
	{"[%c] &Shuffle list", 0,
	 "Shuffle list at start and when all entries are played"},
	{"[%c] List &random", 0, "Play list in random order"},
	{NULL,NULL,NULL}
};
static MMENU plmode_menu =
	{ 0, 0, -1, 1, plmode_entries, handle_menu, NULL, NULL, 4 };

static MENTRY exit_entries[] = {
	{"[%c] Save &config", 0, NULL},
	{"[%c] Save &playlist", 0, NULL},
	{NULL,NULL,NULL}
};
static MMENU exit_menu =
	{ 0, 0, -1, 1, exit_entries, handle_menu, NULL, NULL, 5 };

static MENTRY other_entries[] = {
	{"&Playmode  %>", &plmode_menu, "Playlist playing mode"},
	{"[%c] &Curious", 0, "Look for hidden patterns in module"},
	{"[%c] &Tolerant", 0, "Don't halt on file access errors"},
	{"[%c] &Full path", 0, "Display full path of files"},
	{"&Edit theme", 0, "Copy, edit, or delete active theme"},
	{NULL, 0, "Color theme to use ((C) color theme, (M) mono theme)"},
	{"[%c] &Window title", 0, "Set the term/window title to song name/filename"},
#if LIBMIKMOD_VERSION >= 0x030200
	{"[%c] Sample&names", 0, "Always display sample names in volumebars panel"},
	{"[%c] Fake &volumebars", 0, "Display fast (non CPU-intensive) volumebars"},
#endif
	{"&Scheduling       [%o]|Normal|Renice|Realtime", 0,
	 "Change process priority, MikMod must be restarted to change this"},
	{"Status&bar           [%o]|None|Small|Big", 0, "Size of the statusbar"},
	{"&On exit   %>", &exit_menu, ""},
	{NULL,NULL,NULL}
};
static MMENU other_menu =
	{ 0, 0, -1, 1, other_entries, handle_menu, NULL, NULL, 3 };

static MENTRY entries[] = {
	{"&Output options %>", &output_menu, ""},
	{"&Playback options %>", &playback_menu, ""},
	{"O&ther options %>", &other_menu, ""},
	{"%------------", 0, NULL},
	{"&Use config", 0, "Activate the edited configuration"},
	{"S&ave config", 0, "Save and activate the edited configuration"},
	{"R&evert config", 0, "Reset the configuration to the actual used one"},
	{NULL,NULL,NULL}
};
static MMENU menu =
	{ 0, 0, -1, 0, entries, handle_menu, NULL, NULL, 0 };


typedef struct {
	WIDGET *w;			/* bold/... - indicator */
	WID_STR *str_w;
	WID_COLORSEL *col_w;
	WID_LIST *list_w;
	int cur_attr;		/* selected attribute in list widget */
	THEME theme;
	THEME test_theme;
	int orig_theme;		/* index into themes-arry */
} THEME_DATA;

/* Copies of the config theme entries, needed for use/save/revert config */
static int cnt_themes = 0;
static THEME *themes = NULL;

/* set help text of menu entry
   free old menu->help and malloc new entry */
void set_help(MENTRY *entry, const char *str, ...)
{
	va_list args;
	int len = 0;

	if (entry->help)
		free(entry->help);
	va_start(args, str);
	VSNPRINTF (storage, STORAGELEN, str, args);
	va_end(args);

	len = MIN(strlen(storage), STORAGELEN);
	entry->help = (char *) malloc(sizeof(char) * (len + 1));
	strncpy(entry->help, storage, len);
	entry->help[len] = '\0';
}

static char *skip_number(char *str)
{
	while (str && *str == ' ')
		str++;
	while (str && isdigit((int)*str))
		str++;
	while (str && *str == ' ')
		str++;
	return str;
}

/* extract drivers for the option menu */
static void get_drivers(MENTRY *entry)
{
	char *driver = MikMod_InfoDriver(), *pos, *start;
	int len = 0, x = 0;
	BOOL end;

	for (pos = skip_number(driver); pos && *pos; pos++) {
		if (*pos == '\n') {
			if (x > 35)
				x = 35;
			len += x;
			x = 0;
			pos = skip_number(pos + 1);
		}
		x++;
	}
	x--;
	if (*(pos - 1) != '\n')
		len += (x >= 35 ? 35 : x);

	if (entry->text)
		free(entry->text);
	entry->text = (char *) malloc(sizeof(char) * (len + 25));
	strcpy(entry->text, "&Driver [%o]|Autodetect");

	start = skip_number(driver);
	end = !(start && *start);
	for (pos = start; !end; pos++) {
		end = !*pos;
		if (*pos == '\n' || (!*pos && *(pos - 1) != '\n')) {
			strcat(entry->text, "|");
			len = strlen(entry->text);

			/* don't embed text in braces or 'v#.#' in string */
			for (x = 0; x < 34 && start + x <= pos; x++) {
				if (*(start + x) == '(')
					break;
				if ((*(start + x)) == 'v' &&
					isdigit((int)*(start + x + 1))) break;
			}
			while (x > 0 && *(start + x - 1) == ' ')
				x--;

			strncat(entry->text, start, x);
			entry->text[len + x] = '\0';
			pos = skip_number(pos + 1);
			start = pos;
		}
	}
#if (LIBMIKMOD_VERSION >= 0x030200) && defined(HAVE_MIKMOD_FREE)
/* MikMod_free() is in libmikmod-3.2.0 beta3 and newer versions. */
	MikMod_free(driver);
#else
	free(driver);
#endif
}

/* extract drivers options for the option menu */
static void get_driver_options(MENTRY *entry, MENTRY *dr_entry)
{
	int drvno = (SINTPTR_T) dr_entry->data;
	char *cmdline;

	if (entry->text)
		free (entry->text);
	if (driver_get_info (drvno, NULL, &cmdline) && drvno) {
		int cmdlen = 0, i = drvno;
		char *end, *pos = strchr(dr_entry->text, '|');

		while (pos && i>0) {
			pos = strchr(pos+1, '|');
			i--;
		}
		end = pos;
		if (pos && *pos && i==0) {
			pos++;
			end = strchr(pos, '|');
			if (!end)
				end = pos+strlen(pos);
		}
		if (cmdline) {
			cmdlen = strlen (cmdline);
			if (cmdline[cmdlen-1] == '\n')
				cmdlen--;
		}
		entry->text = (char *) malloc(sizeof(char) * (cmdlen+end-pos+50+20));
		strcpy(entry->text, "Driver &options [%s]|Enter driver options");
		if (end > pos && cmdlen > 0) {
			strcat (entry->text, " (Options for ");
			strncat(entry->text, pos, end-pos);
			strcat (entry->text, ":\n");
			strncat(entry->text, cmdline, cmdlen);
			strcat (entry->text, "):|255|16");
		} else
			strcat(entry->text, ":|255|16");
		if (cmdline) free (cmdline);
	} else {
		entry->text = (char *) malloc(sizeof(char) * 50);
		strcpy(entry->text, "Driver &options [%s]|Enter driver options:|255|16");
	}
}

/* extract themes for the other menu */
static void get_themes(MENTRY *entry)
{
	int i, j, len = 0;

	for (i=0; i<cnt_themes; i++) {
		j = strlen (themes[i].name);
		len += (j>32 ? 32:j) + 5;
	}
	if (entry->text)
		free (entry->text);
	entry->text = (char *) malloc(sizeof(char) * (len + 15));
	strcpy(entry->text, "T&heme  [%o]");
	for (i=0; i<cnt_themes; i++) {
		strcat(entry->text, "|");
		strcat(entry->text,themes[i].color ? "(C) ":"(M) ");

		len = strlen(entry->text);
		j = strlen (themes[i].name);
		j = (j>32 ? 32:j);
		strncat(entry->text, themes[i].name, j);
		entry->text[len + j] = '\0';
	}
}

static int config_get_act_theme(void)
{
	return (SINTPTR_T)other_entries[OPT_THEME].data;
}

static void config_set_act_theme(int act_theme)
{
	other_entries[OPT_THEME].data = (void *)(SINTPTR_T)act_theme;
}

static void theme_get_attrs (THEME_DATA *data)
{
	int *attr = &data->theme.attrs[data->cur_attr];

	if (data->theme.color) {
		if (data->col_w)
			*attr = data->col_w->active;
		if (((WID_TOGGLE*)data->w)->selected == 1)
			*attr |= COLOR_BOLDMASK;
		else
			*attr &= ~COLOR_BOLDMASK;
	} else {
		WID_CHECK *cw = (WID_CHECK*)data->w;
		if (cw->selected == 1)
			*attr = A_NORMAL;
		else if (cw->selected == 2)
			*attr = A_BOLD;
		else if (cw->selected == 4)
			*attr = A_REVERSE;
	}
}

static void theme_set_attrs (THEME_DATA *data, int repaint)
{
	int cur = data->cur_attr, i = 0;

	if (data->theme.color) {
		if (data->col_w) {
			wid_colorsel_set_active((WID_COLORSEL*)data->col_w,
									data->theme.attrs[cur]);
			if (repaint)
				wid_repaint ((WIDGET*)data->col_w);
		}
		if (data->theme.attrs[cur] & COLOR_BOLDMASK)
			i = 1;
		else
			i = 0;
		wid_toggle_set_selected((WID_TOGGLE*)data->w, i);
	} else {
		if (data->theme.attrs[cur] == A_NORMAL)
			i = 1;
		else if (data->theme.attrs[cur] == A_BOLD)
			i = 2;
		else if (data->theme.attrs[cur] == A_REVERSE)
			i = 4;
		wid_check_set_selected ((WID_CHECK*)data->w, i);
	}
	if (repaint)
		wid_repaint (data->w);
}

static int cb_theme_list_focus(struct WIDGET *w, int focus)
{
	if (focus == FOCUS_ACTIVATE) {
		THEME_DATA *data = (THEME_DATA *) w->data;

		theme_get_attrs (data);
		data->cur_attr = ((WID_LIST*)w)->cur;
		theme_set_attrs (data,1);

		return EVENT_HANDLED;
	}
	return focus;
}

static void theme_edit_close (THEME_DATA *data)
{
	win_set_theme (&config.themes[config.theme]);
	config_set_act_theme (data->orig_theme);
	get_themes(&other_entries[OPT_THEME]);

	if (data->w) dialog_close(data->w->d);
	CF_theme_free (&data->theme);
	CF_theme_free (&data->test_theme);
	free (data);
}

static int cb_theme_button_focus(WIDGET *w, int focus)
{
	if (focus == FOCUS_ACTIVATE) {
		THEME_DATA *data = (THEME_DATA *) w->data;
		int button = ((WID_BUTTON *) w)->active;

		switch (button) {
			case 0:						/* Ok */
				theme_get_attrs (data);
				if (data->theme.name) free (data->theme.name);
				data->theme.name = strdup(data->str_w->input);

				CF_theme_remove (data->orig_theme,&themes,&cnt_themes);
				data->orig_theme = CF_theme_insert (&themes, &cnt_themes, &data->theme);

				theme_edit_close (data);
				break;
			case 1:						/* Test */
				if (win_has_colors() || !data->theme.color) {
					theme_get_attrs (data);
					CF_theme_free (&data->test_theme);
					CF_theme_copy (&data->test_theme,&data->theme);
					win_set_theme (&data->test_theme);
					win_panel_repaint();
				} else {				/* Cancel */
					theme_edit_close (data);
				}
				break;
			case 2:						/* Cancel */
				theme_edit_close (data);
				break;
		}
		return EVENT_HANDLED;
	}
	return focus;
}

static int cb_focus(struct WIDGET *w, int focus)
{
	if (focus == FOCUS_ACTIVATE) {
		THEME_DATA *data = (THEME_DATA *) w->data;
		if (data->list_w->w.has_focus)
			return FOCUS_DONT;
	}
	return focus;
}

static void theme_edit (int act_theme)
{
	DIALOG *d = dialog_new();
	WIDGET *w;
	char title[200];
	THEME_DATA *data = (THEME_DATA *) malloc(sizeof(THEME_DATA));

	data->cur_attr = 0;
	data->orig_theme = act_theme;
	data->test_theme.name = NULL;
	data->test_theme.attrs = NULL;
	CF_theme_copy (&data->theme, &themes[act_theme]);

	w = wid_list_add(d, 1, attrs_label, ATTRS_COUNT);
	wid_list_set_selection_mode ((WID_LIST*)w, WID_SEL_BROWSE);
	wid_set_size (w, 20, -1);
	wid_set_func(w, NULL, cb_theme_list_focus, data);
	data->list_w = (WID_LIST*) w;

	data->str_w = (WID_STR*)wid_str_add(d, 0, data->theme.name, THEME_NAME_LEN);
	wid_set_size ((WIDGET*)data->str_w, 26, -1);

	if (data->theme.color) {
		if (win_has_colors()) {
			data->col_w = (WID_COLORSEL*)wid_colorsel_add(d, 1, "sdex", 0);
			wid_set_func((WIDGET*)data->col_w, NULL, cb_focus, data);
			data->w = wid_toggle_add (d,0,"&bold",0,0);
		} else {
			data->col_w = NULL;
			data->w = wid_toggle_add (d,2,"&bold",0,0);
		}
		wid_set_func(data->w, NULL, cb_focus, data);
	} else {
		data->w = wid_check_add (d,2,"&normal|&bold|&reverse",0,0);
		wid_set_func(data->w, NULL, cb_focus, data);
	}
	theme_set_attrs (data,0);

	if (win_has_colors() || !data->theme.color)
		w = wid_button_add(d, -1, "&Ok|&Test|&Cancel", 0);
	else
		w = wid_button_add(d, -1, "&Ok|&Cancel", 0);
	wid_set_func(w, NULL, cb_theme_button_focus, data);

	strcpy (title,"Edit theme ");
	strncat (title, data->theme.name, 180);
	dialog_open(d, title);
}

/* Return a unique name among the themes which is based on src_name */
static char *theme_uniq_name (char *src_name)
{
	char buf[THEME_NAME_LEN+1], *pos, *name;
	int i, n, len;

	strncpy (buf,src_name,THEME_NAME_LEN);
	buf[THEME_NAME_LEN] = '\0';
	for (pos = buf+strlen(buf)-1; pos>=buf && isspace((int)*pos); pos--)
		*pos = '\0';
	if (pos>buf && isdigit((int)*pos) && isdigit((int)*(pos-1)))
		*(pos-2) = '\0';

	if (strlen(buf) > THEME_NAME_LEN-5)
		buf[THEME_NAME_LEN-5] = '\0';
	strcat (buf," %02d");

	len = strlen(buf)+1;
	name = (char *) malloc (sizeof(char)*len);
	n = 2;
	do {
		SNPRINTF (name,len,buf,n);
		for (i=0; i<cnt_themes; i++)
			if (!strcmp(themes[i].name,name)) break;
		if (i>=cnt_themes)
			return name;
		n++;
	} while (n<100);
	free (name);
	return NULL;
}

static void theme_copy (int *act_theme)
{
	THEME newtheme;
	newtheme.color = themes[*act_theme].color;
	newtheme.attrs = themes[*act_theme].attrs;
	if ((newtheme.name = theme_uniq_name (themes[*act_theme].name))) {
		*act_theme = CF_theme_insert (&themes,&cnt_themes,&newtheme);
		free (newtheme.name);
	}
}

/* theme edit callback */
static BOOL cb_themeedit (WIDGET *w, int button, void *input, void *data)
{
	int act_theme = config_get_act_theme();
	BOOL user_theme = act_theme >= THEME_COUNT;

	if (button>2 || (!user_theme && button>1))
		return 1;

	switch (button) {
		case 0:				/* Copy */
			theme_copy (&act_theme);
			break;
		case 1:				/* Edit or Copy + Edit */
			if (!user_theme)
				theme_copy (&act_theme);
			theme_edit (act_theme);
			break;
		case 2:				/* Delete (if user_theme) */
			CF_theme_remove (act_theme,&themes,&cnt_themes);
			if (act_theme>=cnt_themes)
				act_theme--;
			break;
	}
	config_set_act_theme (act_theme);
	get_themes(&other_entries[OPT_THEME]);
	return 1;
}

static void config_set_config(CONFIG *cfg)
{
	int i;

	output_entries[OPT_DRIVER].data = (void *)(SINTPTR_T)cfg->driver;
#if LIBMIKMOD_VERSION >= 0x030107
	strcpy ((char *)output_entries[OPT_DRV_OPTION].data,cfg->driveroptions);
#endif
	output_entries[OPT_STEREO].data = (void *)(SINTPTR_T)cfg->stereo;
	output_entries[OPT_MODE_16BIT].data = (void *)(SINTPTR_T)cfg->mode_16bit;
	output_entries[OPT_FREQUENCY].data = (void *)(SINTPTR_T)cfg->frequency;
	output_entries[OPT_INTERPOLATE].data = (void *)(SINTPTR_T)cfg->interpolate;
	output_entries[OPT_HQMIXER].data = (void *)(SINTPTR_T)cfg->hqmixer;
	output_entries[OPT_SURROUND].data = (void *)(SINTPTR_T)cfg->surround;
	output_entries[OPT_REVERB].data = (void *)(SINTPTR_T)cfg->reverb;

	playback_entries[OPT_VOLUME].data = (void *)(SINTPTR_T)cfg->volume;
	playback_entries[OPT_VOLRESTRICT].data =
	  (void *)(SINTPTR_T)cfg->volrestrict;
	playback_entries[OPT_FADE].data = (void *)(SINTPTR_T)cfg->fade;
	playback_entries[OPT_LOOP].data = (void *)(SINTPTR_T)cfg->loop;
	playback_entries[OPT_PANNING].data = (void *)(SINTPTR_T)cfg->panning;
	playback_entries[OPT_EXTSPD].data = (void *)(SINTPTR_T)cfg->extspd;

	plmode_entries[OPT_PM_MODULE].data =
	  (void *)(SINTPTR_T)BTST(cfg->playmode, PM_MODULE);
	plmode_entries[OPT_PM_MULTI].data =
	  (void *)(SINTPTR_T)BTST(cfg->playmode, PM_MULTI);
	plmode_entries[OPT_PM_SHUFFLE].data =
	  (void *)(SINTPTR_T)BTST(cfg->playmode, PM_SHUFFLE);
	plmode_entries[OPT_PM_RANDOM].data =
	  (void *)(SINTPTR_T)BTST(cfg->playmode, PM_RANDOM);
	other_entries[OPT_CURIOUS].data = (void *)(SINTPTR_T)cfg->curious;
	other_entries[OPT_TOLERANT].data = (void *)(SINTPTR_T)cfg->tolerant;
	other_entries[OPT_FULLPATHS].data = (void *)(SINTPTR_T)cfg->fullpaths;
	other_entries[OPT_WINDOWTITLE].data = (void *)(SINTPTR_T)cfg->window_title;
#if LIBMIKMOD_VERSION >= 0x030200
	other_entries[OPT_SAMPLES].data = (void *)(SINTPTR_T)cfg->forcesamples;
	other_entries[OPT_FAKEVOLBARS].data = (void *)(SINTPTR_T)cfg->fakevolbars;
#endif
	other_entries[OPT_RENICE].data = (void *)(SINTPTR_T)cfg->renice;
	other_entries[OPT_STATUSBAR].data = (void *)(SINTPTR_T)cfg->statusbar;

	exit_entries[OPT_S_CONFIG].data = (void *)(SINTPTR_T)cfg->save_config;
	exit_entries[OPT_S_PLAYLIST].data = (void *)(SINTPTR_T)cfg->save_playlist;

#if LIBMIKMOD_VERSION >= 0x030107
	get_driver_options (&output_entries[OPT_DRV_OPTION],
						&output_entries[OPT_DRIVER]);
#endif

	CF_themes_free (&themes, &cnt_themes);
	for (i = 0; i < cfg->cnt_themes; i++)
		CF_theme_insert (&themes, &cnt_themes, &cfg->themes[i]);
	config_set_act_theme(cfg->theme);
	get_themes(&other_entries[OPT_THEME]);
}

static void config_get_config(CONFIG *cfg)
{
	int i;

	cfg->driver = (SINTPTR_T)output_entries[OPT_DRIVER].data;
#if LIBMIKMOD_VERSION >= 0x030107
	rc_set_string(&cfg->driveroptions, (char *)output_entries[OPT_DRV_OPTION].data, 99);
#endif
	cfg->stereo = (BOOL)(SINTPTR_T)output_entries[OPT_STEREO].data;
	cfg->mode_16bit = (BOOL)(SINTPTR_T)output_entries[OPT_MODE_16BIT].data;
	cfg->frequency = (SINTPTR_T)output_entries[OPT_FREQUENCY].data;
	cfg->interpolate = (BOOL)(SINTPTR_T)output_entries[OPT_INTERPOLATE].data;
	cfg->hqmixer = (BOOL)(SINTPTR_T)output_entries[OPT_HQMIXER].data;
	cfg->surround = (BOOL)(SINTPTR_T)output_entries[OPT_SURROUND].data;
	cfg->reverb = (SINTPTR_T)output_entries[OPT_REVERB].data;

	cfg->volume = (SINTPTR_T)playback_entries[OPT_VOLUME].data;
	cfg->volrestrict = (BOOL)(SINTPTR_T)playback_entries[OPT_VOLRESTRICT].data;
	cfg->fade = (BOOL)(SINTPTR_T)playback_entries[OPT_FADE].data;
	cfg->loop = (BOOL)(SINTPTR_T)playback_entries[OPT_LOOP].data;
	cfg->panning = (BOOL)(SINTPTR_T)playback_entries[OPT_PANNING].data;
	cfg->extspd = (BOOL)(SINTPTR_T)playback_entries[OPT_EXTSPD].data;

	cfg->playmode =
	  (((BOOL)(SINTPTR_T)plmode_entries[OPT_PM_MODULE].data) ? PM_MODULE : 0) |
	  (((BOOL)(SINTPTR_T)plmode_entries[OPT_PM_MULTI].data) ? PM_MULTI : 0) |
	  (((BOOL)(SINTPTR_T)plmode_entries[OPT_PM_SHUFFLE].data) ? PM_SHUFFLE : 0) |
	  (((BOOL)(SINTPTR_T)plmode_entries[OPT_PM_RANDOM].data) ? PM_RANDOM : 0);
	cfg->curious = (BOOL)(SINTPTR_T)other_entries[OPT_CURIOUS].data;
	cfg->tolerant = (BOOL)(SINTPTR_T)other_entries[OPT_TOLERANT].data;
	cfg->fullpaths = (BOOL)(SINTPTR_T)other_entries[OPT_FULLPATHS].data;
	cfg->window_title = (BOOL)(SINTPTR_T)other_entries[OPT_WINDOWTITLE].data;
#if LIBMIKMOD_VERSION >= 0x030200
	cfg->forcesamples = (BOOL)(SINTPTR_T)other_entries[OPT_SAMPLES].data;
	cfg->fakevolbars = (BOOL)(SINTPTR_T)other_entries[OPT_FAKEVOLBARS].data;
#endif
	cfg->renice = (SINTPTR_T)other_entries[OPT_RENICE].data;
	cfg->statusbar = (SINTPTR_T)other_entries[OPT_STATUSBAR].data;

	cfg->save_config = (BOOL)(SINTPTR_T)exit_entries[OPT_S_CONFIG].data;
	cfg->save_playlist = (BOOL)(SINTPTR_T)exit_entries[OPT_S_PLAYLIST].data;

	CF_themes_free_user (&cfg->themes, &cfg->cnt_themes);
	for (i=THEME_COUNT; i<cnt_themes; i++)
		CF_theme_insert (&cfg->themes, &cfg->cnt_themes, &themes[i]);
	cfg->theme = config_get_act_theme();
}

static void handle_menu(MMENU *mn)
{
	switch (mn->id) {
	  case MENU_MAIN:
		switch (mn->cur) {
		  case MENU_USE:
			config_get_config(&config);
			Player_SetConfig(&config);
			win_status("Configuration activated");
			config_set_config(&config);
			break;
		  case MENU_SAVE:
			config_get_config(&config);
			CF_Save(&config);
			Player_SetConfig(&config);
			win_status("Configuration saved and activated");
			config_set_config(&config);
			break;
		  case MENU_REVERT:
			config_set_config(&config);
			win_status("Changed configuration reseted");
			break;
		}
		break;
	  case MENU_OUTPUT:
#if LIBMIKMOD_VERSION >= 0x030107
		if (mn->cur == OPT_DRIVER)
			get_driver_options(&output_entries[OPT_DRV_OPTION],
							   &output_entries[OPT_DRIVER]);
#endif
		break;
	  case MENU_OTHER:
		if (mn->cur == OPT_EDITTHEME) {
			if (config_get_act_theme() < THEME_COUNT)
				dlg_message_open("Copy or copy and edit active (default-)theme?",
								 "&Copy|Copy + &Edit|&Cancel", 2, 0,
								 cb_themeedit, NULL);
			else
				dlg_message_open("Copy, edit, or delete the active theme?",
								 "&Copy|&Edit|Delete|&Cancel", 3, 0,
								 cb_themeedit, NULL);
		  }
		break;
	}
}

/* open config editor */
void config_open(void)
{
	char *name = CF_GetFilename();
	set_help(&exit_entries[OPT_S_CONFIG],
			 "Save config at exit in '%s'", name);
	if (name)
		free(name);
	name = PL_GetFilename();
	set_help(&exit_entries[OPT_S_PLAYLIST],
			 "Save playlist at exit in '%s'", name);
	if (name)
		free(name);

	get_drivers(&output_entries[OPT_DRIVER]);
	config_set_config(&config);

	menu_open(&menu, 5, 5);
}

/* ex:set ts=4: */
