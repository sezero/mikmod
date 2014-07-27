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

  $Id: display.c,v 1.8 2004/02/02 01:35:52 raph Exp $

  Display routines for the different panels and the playlist menu

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <mikmod.h>
#include "display.h"
#include "player.h"
#include "mconfig.h"
#include "mlist.h"
#include "mutilities.h"
#include "mwindow.h"
#include "mconfedit.h"
#include "keys.h"
#include "mplayer.h"
#include "mlistedit.h"

/*========== Display layout */

/* minimum width of one column */
#define MINWIDTH	20
/* minimum width of second column */
#define MINVISIBLE	10
/* half width */
int halfwidth;
/* format used for message/banner lines : like "%-80.80s" */
char fmt_fullwidth[20];
/* format used for sample/instrument lines - like "%3i  %-35.35s"
   (the big number being halfwidth-5) */
char fmt_halfwidth[20];
/* start of information panels */
#define PANEL_Y 7

#if LIBMIKMOD_VERSION >= 0x030200
static MP_DATA playdata;

/* The characters used to represent different visual things */
#define CHAR_AMPLITUDE1		'='
#define CHAR_AMPLITUDE0		'-'

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
#define CHAR_SAMPLE_KICK3	'*'
#define CHAR_SAMPLE_KICK2	'\x07'
#define CHAR_SAMPLE_KICK1	'\xf9'
#define CHAR_SAMPLE_KICK0	'\xfa'
#else
#define CHAR_SAMPLE_KICK3	'@'
#define CHAR_SAMPLE_KICK2	'O'
#define CHAR_SAMPLE_KICK1	'o'
#define CHAR_SAMPLE_KICK0	'.'
#endif

static char samp_char[4] = {
	CHAR_SAMPLE_KICK0,
	CHAR_SAMPLE_KICK1,
	CHAR_SAMPLE_KICK2,
	CHAR_SAMPLE_KICK3
};
static ATTRS samp_attr[4] = {
	ATTR_SAMPLES_KICK0,
	ATTR_SAMPLES_KICK1,
	ATTR_SAMPLES_KICK2,
	ATTR_SAMPLES_KICK3
};

/* The routine for dynamically repainting current panel */
static void (*dynamic_repaint) (MWINDOW *win) = NULL;
static MWINDOW *dynamic_repaint_win;
#endif

static void display_title(void);
static void set_window_title(const char *content);

/*========== Variables */

extern BOOL quiet;
extern MODULE *mf;

static MWINDOW *root;
static int cur_display = DISPLAY_SAMPLE, old_display = DISPLAY_SAMPLE;

/* first line of displayed information in the panels */
static int first_help = 0;
static int first_sample = 0;
static int first_inst = 0;
static int first_comment = 0;
static int first_list = 0;
#if LIBMIKMOD_VERSION >= 0x030200
static int first_volbar = 0;
#endif

/* computes printf templates when screen size changes, so that two-column
   display fills the screen */
static void setup_printf(void)
{
	int maxx, winy;

	win_get_size(root, &maxx, &winy);
	if (maxx > MAXWIDTH)
		maxx = MAXWIDTH;
	if (maxx < 0) maxx = 0;

	halfwidth = maxx >> 1;
	if (halfwidth < MINWIDTH)
		halfwidth = MINWIDTH;
	SNPRINTF(fmt_fullwidth, 20, "%%-%d.%ds", maxx, maxx);
	SNPRINTF(fmt_halfwidth, 20, "%%3i  %%-%d.%ds", halfwidth - 5,
			 halfwidth - 5);
}

/* enlarges a text line to fill the root window width */
static void enlarge (int x, char *str)
{
	int winx, winy, len;
	win_get_size(root, &winx, &winy);
	winx -= x;
	len = strlen (str);
	if (len < winx)
		memset(str + len, ' ', winx - len);
	if (winx>=0)
		str[winx] = '\0';
}

/* first line : MikMod version */
static void display_version(void)
{
	if (quiet)
		return;

	strcpy (storage,mikversion);
	enlarge (0,storage);

	win_attrset(ATTR_TITLE);
	win_print(root, 0, 0, storage);
}

static BOOL remove_msg = 0;
static time_t start_time;
static char old_message[STORAGELEN + 1];

/* displays a warning message on the top right corner of the display */
void display_message(char *str)
{
	int len = strlen(str)+1;

	if (quiet)
		return;
	if (len > STORAGELEN)
		len = STORAGELEN;
	old_message[0] = ' ';
	strncpy(&old_message[1], str, len-1);
	old_message[len] = '\0';
	enlarge (strlen(mikversion),old_message);

	win_attrset(ATTR_WARNING);
	win_print(root, strlen(mikversion), 0, str);

	remove_msg = 1;
	start_time = time(NULL);
}

/* changes the warning message */
static void update_message(void)
{
	if (remove_msg && old_message[0]) {
		win_attrset(ATTR_WARNING);
		win_print(root, strlen(mikversion), 0, old_message);
	}
}

/* removes the warning message */
static void remove_message(void)
{
	if (remove_msg) {
		time_t end_time = time(NULL);
		if (end_time - start_time >= 6) {
			display_version();
			remove_msg = 0;
		}
	}
}

/* display a banner/message from line skip, at position origin
   returns updated skip value if it is out of bounds and would prevent the
   message from being seen. */
static int display_banner(MWINDOW *win, const char *banner, int origin,
			  int skip, BOOL wrap)
{
	const char *buf = banner;
	char str[MAXWIDTH + 1];
	int i, n, t, winx, winy;

	win_get_size(win, &winx, &winy);
	if (winx < 5 || winy < 1)
		return skip;

	/* count message lines */
	for (t = 0; *buf; t++) {
		n = 0;
		while ((((n < winx) && (n < MAXWIDTH)) || (!wrap)) &&
			   (*buf != '\r') && (*buf != '\n') && (*buf))
			buf++, n++;
		if ((*buf == '\r') || (*buf == '\n'))
			buf++;
	}

	/* update skip value */
	if (skip < 0)
		skip = 0;
	if (skip + winy - origin > t)
		skip = t - winy + origin;
	if (skip < 0)
		skip = 0;
	if (t - skip + origin > winy)
		t = winy - origin + skip;

	/* skip first lines */
	buf = banner;
	for (i = 0; i < skip && i < t; i++) {
		n = 0;
		while ((((n < winx) && (n < MAXWIDTH)) || (!wrap)) &&
			   ((*buf != '\r') && (*buf != '\n') && (*buf)))
			buf++, n++;
		if ((*buf == '\r') || (*buf == '\n'))
			buf++;
	}

	/* display lines */
	for (i = skip; i < t; i++) {
		for (n = 0; (((n < winx) && (n < MAXWIDTH)) || (!wrap)) &&
			 (*buf != '\r') && (*buf != '\n') && (*buf); buf++) {
			if (*buf < ' ')
				str[n] = ' ';
			else
				str[n] = *buf;
			if (n < MAXWIDTH)
				n++;
		}
		if ((*buf == '\r') || (*buf == '\n'))
			buf++;
		if (n) {
			str[n] = '\0';
			SNPRINTF(storage, STORAGELEN, fmt_fullwidth, str);
			win_print(win, 0, i - skip + origin, storage);
		} else
			win_clrtoeol(win, 0, i - skip + origin);
	}
	if (!origin)
		/* clear to bottom of window */
		for(i += origin - skip; i < winy; i++)
			win_clrtoeol(win, 0, i);

	return skip;
}

/* displays the "paused" banner */
void display_pausebanner(void)
{
	if (quiet)
		return;

	win_attrset(ATTR_BANNER);
	display_banner(root, pausebanner, 1, 0, 0);
}

/* display the "extracting" banner */
void display_extractbanner(void)
{
	if (quiet)
		return;

	win_attrset(ATTR_BANNER);
	display_banner(root, extractbanner, 1, 0, 0);
	win_refresh();
}

/* display the "loading" banner */
void display_loadbanner(void)
{
	if (quiet)
		return;

	win_attrset(ATTR_BANNER);
	display_banner(root, loadbanner, 1, 0, 0);
	win_refresh();
}

/* second line : driver settings */
static void display_driver(void)
{
	char reverb[13];

	if (quiet)
		return;

	if (md_reverb)
		SNPRINTF(reverb, 12, "reverb: %2d", md_reverb);
	else
		strcpy(reverb, "no reverb");

	SNPRINTF(storage, STORAGELEN, "%s: %d bit %s %s, %u Hz, %s",
			 md_driver->Name, (md_mode & DMODE_16BITS) ? 16 : 8,
			 (md_mode & DMODE_INTERP) ?
			 (md_mode & DMODE_SURROUND ? "interp. surround" : "interpolated")
			 : (md_mode & DMODE_SURROUND ? "surround" : "normal"),
			 (md_mode & DMODE_STEREO) ? "stereo" : "mono", md_mixfreq,
			 reverb);

	enlarge(0,storage);
	win_print(root, 0, 1, storage);
}

/* third line : filename */
static void display_file(void)
{
	PLAYENTRY *entry;

	if (quiet)
		return;

	storage[0] = '\0';
	if ((entry = PL_GetCurrent(&playlist))) {
		CHAR *archive = entry->archive, *file;

		if (archive && !config.fullpaths) {
			archive = FIND_LAST_DIRSEP(entry->archive);
			if (archive)
				archive++;
			else
				archive = entry->archive;
		}

		file = FIND_LAST_DIRSEP(entry->file);
		if (file && !config.fullpaths)
			file++;
		else
			file = entry->file;

		if ((archive) && (strlen(file) < MAXWIDTH - 13)) {
			if (strlen(archive) + strlen(file) > MAXWIDTH - 10) {
				archive += strlen(archive) - (MAXWIDTH - 13 - strlen(file));
				SNPRINTF(storage, STORAGELEN, "File: %s (...%s)", file,
						 archive);
			} else
				SNPRINTF(storage, STORAGELEN, "File: %s (%s)", file,
						 archive);
		} else
			SNPRINTF(storage, STORAGELEN, "File: %.70s", file);
	}
	enlarge(0,storage);
	win_print(root, 0, 2, storage);
}

/* fourth and fifth lines : module name and format */
static void display_name(void)
{
	if (quiet || !mf)
		return;

	SNPRINTF(storage, STORAGELEN, "Name: %.70s", mf->songname);
	enlarge(0,storage);
	win_print(root, 0, 3, storage);

	SNPRINTF(storage, STORAGELEN,
			 "Type: %s, Periods: %s, %s",
			 mf->modtype,
			 (mf->flags & UF_XMPERIODS) ? "XM type" : "mod type",
			 (mf->flags & UF_LINEAR) ? "linear" : "log");
	enlarge(0,storage);
	win_print(root, 0, 4, storage);
}

/* sixth line : player status */
void display_status(void)
{
#if LIBMIKMOD_VERSION >= 0x030200
	int i;
	unsigned long cur_time;
	static MP_DATA data;
#endif

	if (quiet)
		return;

	remove_message();
	if (MP_Paused() || !mf)
		return;

	win_attrset(ATTR_SONG_STATUS);
	if (mf->sngpos < mf->numpos) {
		PLAYENTRY *cur = PL_GetCurrent(&playlist);
		char time[7] = "";
		char channels[17] = "";

		if (cur && cur->time > 0)
			SNPRINTF(time, 7, "/%2d:%02d",
					 (int)((cur->time / 60) % 60), (int)(cur->time % 60));
#if LIBMIKMOD_VERSION >= 0x030107
		if (mf->flags & UF_NNA) {
			SNPRINTF(channels, 17, "%2d/%d+%d->%d",
					 mf->realchn, mf->numchn, mf->totalchn - mf->realchn,
					 mf->totalchn);
		} else
#endif
			SNPRINTF(channels, 17, "%2d/%d      ", mf->realchn, mf->numchn);

		SNPRINTF(storage, STORAGELEN,
				 "pat:%03d/%03d pos:%2.2X spd:%2d/%3d "
				 "vol:%3d%%/%3d%% time:%2d:%02d%s chn:%s",
				 mf->sngpos, mf->numpos - 1, mf->patpos, mf->sngspd, mf->bpm,
				 (mf->volume * 100 + 127) >> 7, (md_volume * 100 + 127) >> 7,
				 (int)(((mf->sngtime >> 10) / 60) % 60),
				 (int)((mf->sngtime >> 10) % 60), time, channels);

		enlarge(0,storage);
		win_print(root, 0, 5, storage);
	}

#if LIBMIKMOD_VERSION >= 0x030200
	if (config.fakevolbars) {
		MP_GetData (&data);
		cur_time = Time1000();
		for (i = 0; i < mf->numchn; i++) {
			unsigned int delta = (cur_time - playdata.vstatus[i].time) / 10;
			if (delta>0)
				playdata.vstatus[i].time = cur_time;
			if (playdata.vstatus[i].volamp > delta)
				playdata.vstatus[i].volamp -= delta;
			else
				playdata.vstatus[i].volamp = 0;
		}

		for (i = 0; i < mf->numchn; i++) {
			playdata.vinfo[i] = data.vinfo[i];
			if (playdata.vinfo[i].kick)
				playdata.vstatus[i].volamp = playdata.vinfo[i].volume;
		}
	} else {
		MP_GetData (&playdata);
	}
	if (dynamic_repaint)
		dynamic_repaint(dynamic_repaint_win);
#endif
}

/* seventh line to bottom of screen: information panel */
static BOOL display_information(void)
{
	static const char *panel_name[] = {
		"Help",
		"Samples",
		"Instruments",
		"Message",
		"playList",
		"Configuration",
#if LIBMIKMOD_VERSION >= 0x030200
		"Volume",
#endif
	};
	char paneltitle[STORAGELEN];
	BOOL change = 0;
	int i;
	ATTRS attr;
	char *tmp;

	if (quiet)
		return 1;

	/* sanity check */
	if (!mf && ((cur_display == DISPLAY_INST) ||
				(cur_display == DISPLAY_SAMPLE) ||
				(cur_display == DISPLAY_MESSAGE)
#if LIBMIKMOD_VERSION >= 0x030200
				|| (cur_display == DISPLAY_VOLBARS)
#endif
		)) {
		cur_display = DISPLAY_LIST;
		change = 1;
	}
	while (1) {
		if ((cur_display == DISPLAY_INST && (!(mf->flags & UF_INST))) ||
			(cur_display == DISPLAY_MESSAGE && !mf->comment)) {
			cur_display =
			  (cur_display == old_display) ? DISPLAY_SAMPLE : old_display;
			change = 1;
		} else
			break;
	}
	if (change) {
		win_change_panel(cur_display);
		return 0;
	}

	/* set panel title */
	paneltitle[0] = 0;
	for (i = DISPLAY_HELP; i < DISPLAY_COUNT; i++) {
		if ((i == DISPLAY_SAMPLE && !mf) ||
			(i == DISPLAY_INST && (!mf || !(mf->flags & UF_INST))) ||
			(i == DISPLAY_MESSAGE && (!mf || !mf->comment))
#if LIBMIKMOD_VERSION >= 0x030200
			|| (i == DISPLAY_VOLBARS && !mf)
#endif
			)
			continue;
		SNPRINTF(paneltitle + strlen(paneltitle), STORAGELEN, "%c%s%c",
				 i == cur_display ? '[' : ' ',
				 panel_name[i - 1], i == cur_display ? ']' : ' ');
	}

	enlarge (0,paneltitle);

	tmp = paneltitle + strlen(paneltitle);
	attr = ATTR_INFO_INACTIVE;
	while (--tmp >= paneltitle) {
		ATTRS newattr = attr;
		if (*tmp == ']')
			newattr = ATTR_INFO_ACTIVE;
		else if (tmp[1] == '[')
			newattr = ATTR_INFO_INACTIVE;
		else if (isupper((int)*tmp))
			newattr = (attr == ATTR_INFO_ACTIVE) ? ATTR_INFO_AHOTKEY :
			  (attr == ATTR_INFO_INACTIVE) ? ATTR_INFO_IHOTKEY : newattr;
		else if (isupper((int)tmp[1]))
			newattr = (attr == ATTR_INFO_AHOTKEY) ? ATTR_INFO_ACTIVE :
			  (attr == ATTR_INFO_IHOTKEY) ? ATTR_INFO_INACTIVE : newattr;
		if ((newattr != attr) && (tmp[1])) {
			win_attrset(attr);
			win_print(root, tmp - paneltitle + 1, 6, tmp + 1);
			tmp[1] = 0;
		}
		attr = newattr;
	}
	win_attrset(attr);
	win_print(root, 0, 6, paneltitle);
	return 1;
}

/* help panel */
static void display_help(MWINDOW *win, int diff)
{
/* *INDENT-OFF* */
	static const char helptext[] =
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
		"Keys help             (depending on your terminal and your curses library,\n"
		"=========                      some of these keys might not be recognized)\n"
#else
		"Keys help\n"
		"=========\n"
#endif
		"\n"
		"H/F1      show help panel               "
		"()        decrease/increase tempo\n"
		"S/F2      show samples panel            "
		"{}        decrease/increase bpm\n"
		"I/F3      show instrument panel         "
		":/;       toggle interpolation\n"
		"M/F4      show message panel            "
		"U         toggle surround sound\n"
		"L/F5      show list panel               "
		"1..0      volume 10%..100%\n"
		"C/F6      show config panel             "
		"<>        decrease/increase volume\n"
#if LIBMIKMOD_VERSION >= 0x030200
		"V/F7      show volume bars              "
		"P         switch to previous module\n"
		"ENTER     in list panel, activate menu  "
		"N         switch to next module\n"
		"Left/-    previous pattern              "
		"R         restart module\n"
		"Right/+   next pattern                  "
		"Space     toggle pause\n"
		"Up/Down   scroll panel                  "
		"^L        refresh screen\n"
		"PgUp/PgDn scroll panel (faster)         "
		"F         toggle fake/real volume bars\n"
		"Home/End  start/end of panel            "
		"Q         exit MikMod\n";
#else
		"ENTER     in list panel, activate menu  "
		"P         switch to previous module\n"
		"Left/-    previous pattern              "
		"N         switch to next module\n"
		"Right/+   next pattern                  "
		"R         restart module\n"
		"Up/Down   scroll panel                  "
		"Space     toggle pause\n"
		"PgUp/PgDn scroll panel (faster)         "
		"^L        refresh screen\n"
		"Home/End  start/end of panel            "
		"Q         exit MikMod\n";
#endif
/* *INDENT-ON* */

	first_help += diff;
	win_attrset(ATTR_HELP);
	first_help = display_banner(win, helptext, 0, first_help, 0);
	win_status("");
}

static void convert_string(char *str)
{
	for (; str && *str; str++)
		if (*str < ' ')
			*str = ' ';
}

/* helper function for scrollable panels */
void updatefirst(MWINDOW *win, int *first, int *winx, int *count,
				 int *semicount, int diff, int total)
{
	int wx, scount;

	*first += diff;
	win_get_size(win, &wx, &scount);
	*winx = wx;

	if (semicount) {
		if (wx < MINWIDTH + MINVISIBLE)
			*count = scount;
		else
			*count = scount * 2;
	} else
		*count = scount;
	if ((wx <= 0) || (scount <= 0)) *count = 0;

	if (*first >= total - *count)
		*first = total - *count;
	if (*first < 0)
		*first = 0;

	if (semicount) {
		if ((total > scount) && (total < *count)) {
			scount = (total + 1) >> 1;
			if (wx < MINWIDTH + MINVISIBLE)
				*count = scount;
			else
				*count = scount * 2;
		}
		*semicount = scount;
	}
}

/* sample panel */
static void display_sample(MWINDOW *win, int diff)
{
	int count, semicount, t, winx;

	updatefirst(win, &first_sample, &winx, &count, &semicount, diff, mf->numsmp);
	win_clear(win);		/* Sets attrs */
	for (t = first_sample; t < mf->numsmp && t < (count + first_sample); t++) {
		int x = ((t - first_sample) < semicount) ? 0 : halfwidth;

		if (x < winx) {
			SNPRINTF(storage, STORAGELEN, fmt_halfwidth, t,
					 mf->samples[t].samplename ? mf->samples[t].
					 samplename : "");
			convert_string(storage);
			win_print(win, x, (t - first_sample) % semicount, storage);
		}
	}
	if (mf->numsmp == 1)
		win_status("1 Sample");
	else {
		SNPRINTF(storage, STORAGELEN, "%d Samples", mf->numsmp);
		win_status(storage);
	}
}

#if LIBMIKMOD_VERSION >= 0x030200
static void dynamic_display_sample(MWINDOW *win)
{
	int count, semicount, t, winx;
	int voice, vol, chancount;
	char sampchar[2];

	if (cur_display != DISPLAY_SAMPLE)
		return;

	sampchar[1] = 0;
	updatefirst(win, &first_sample, &winx, &count, &semicount, 0, mf->numsmp);

	for (t = first_sample; t < mf->numsmp && t < (count + first_sample); t++) {
		int x = ((t - first_sample) < semicount) ? 0 : halfwidth;

		sampchar[0] = ' ';
		if (x < winx) {
			vol = chancount = 0;
			for (voice = 0; voice < mf->numchn; voice++) {
				if (playdata.vinfo[voice].s == &mf->samples[t]) {
					vol += playdata.vstatus[voice].volamp;
					chancount++;
				}
			}
			if (chancount) {
				vol /= chancount;
				if (vol >= 56)
					voice = 3;
				else if (vol >= 44)
					voice = 2;
				else if (vol >= 26)
					voice = 1;
				else
					voice = 0;
				sampchar[0] = samp_char[voice];
				win_attrset(samp_attr[voice]);
			} else
				win_attrset(ATTR_SAMPLES);
			win_print(win, x + 3, (t - first_sample) % semicount, sampchar);
		}
	}
}
#endif

/* instrument panel */
static void display_inst(MWINDOW *win, int diff)
{
	int count, semicount, t, winx;

	updatefirst(win, &first_inst, &winx, &count, &semicount, diff, mf->numins);
	win_clear(win);		/* Sets attrs */
	for (t = first_inst; t < mf->numins && t < (count + first_inst); t++) {
		int x = ((t - first_inst) < semicount) ? 0 : halfwidth;

		if (x < winx) {
			SNPRINTF(storage, STORAGELEN, fmt_halfwidth, t,
					 mf->instruments[t].insname ? mf->instruments[t].
					 insname : "");
			convert_string(storage);
			win_print(win, x, (t - first_inst) % semicount, storage);
		}
	}
	if (mf->numins == 1)
		win_status("1 Instrument");
	else {
		SNPRINTF(storage, STORAGELEN, "%d Instruments", mf->numins);
		win_status(storage);
	}
}

#if LIBMIKMOD_VERSION >= 0x030200
static void dynamic_display_inst(MWINDOW *win)
{
	int count, semicount, t, winx;
	int voice, vol, chancount;
	char sampchar[2];

	if (cur_display != DISPLAY_INST)
		return;

	sampchar[1] = 0;
	updatefirst(win, &first_inst, &winx, &count, &semicount, 0, mf->numins);

	for (t = first_inst; t < mf->numins && t < (count + first_inst); t++) {
		int x = ((t - first_inst) < semicount) ? 0 : halfwidth;

		sampchar[0] = ' ';
		if (x < winx) {
			vol = chancount = 0;
			for (voice = 0; voice < mf->numchn; voice++) {
				if (playdata.vinfo[voice].i == &mf->instruments[t]) {
					vol += playdata.vstatus[voice].volamp;
					chancount++;
				}
			}
			if (chancount) {
				vol /= chancount * 16;
				if (vol >= 4) vol = 3;
				sampchar[0] = samp_char[vol];
				win_attrset(samp_attr[vol]);
			} else
				win_attrset(ATTR_SAMPLES);
			win_print(win, x + 3, (t - first_inst) % semicount, sampchar);
		}
	}
}
#endif

/* comment panel */
static void display_comment(MWINDOW *win, int diff)
{
	first_comment += diff;
	win_attrset(ATTR_HELP);
	first_comment = display_banner(win, mf->comment, 0, first_comment, 1);
	win_status("");
}

#if LIBMIKMOD_VERSION >= 0x030200
static void dynamic_display_volbars(MWINDOW *win)
{
	int count, t, i, v, winx, barw;
	int loww, medw;
	char *tmp;

	if (cur_display != DISPLAY_VOLBARS)
		return;

	updatefirst(win, &first_volbar, &winx, &count, NULL, 0, mf->numchn);

	winx -= 5;
	barw = winx / 2;
	if (barw < 3)
		return;
	else if (barw > 30)
		barw = 30;
	loww = barw * 3 / 4;
	medw = (barw - loww) * 3 / 4;

	for (t = first_volbar; t < (first_volbar + count) && t < mf->numchn; t++) {
		v = playdata.vstatus[t].volamp * barw / 32;
		memset(&storage, ' ', barw);
		storage[barw] = '\0';
		memset(&storage, CHAR_AMPLITUDE1, v / 2);
		if (v & 1) {
			storage[v / 2] = CHAR_AMPLITUDE0;
			v = v/2 + 1;
		} else
			v = v/2;
		if (v < barw) {
			win_attrset(ATTR_VOLBAR);
			win_print(win, (mf->numchn > 100 ? 6 : 5) + v, t - first_volbar,
					  storage + v);
			storage[v] = '\0';
		}
		if (v > loww + medw) {
			win_attrset(ATTR_VOLBAR_HIGH);
			win_print(win,(mf->numchn > 100 ? 6 : 5) + loww + medw, t - first_volbar,
					  storage + loww + medw);
			storage[loww + medw] = '\0';
		}
		if (v > loww) {
			win_attrset(ATTR_VOLBAR_MED);
			win_print(win, (mf->numchn > 100 ? 6 : 5) + loww, t - first_volbar,
					  storage + loww);
			storage[loww] = '\0';
		}
		if (v > 0) {
			win_attrset(ATTR_VOLBAR_LOW);
			win_print(win, (mf->numchn > 100 ? 6 : 5), t - first_volbar, storage);
		}
		storage[0] = '\0';
		if (playdata.vinfo[t].i && !config.forcesamples) {
			for (i=0; i < mf->numins && playdata.vinfo[t].i != &mf->instruments[i];
				i++);
			SNPRINTF(storage, STORAGELEN, "%3i %s", i,
					 playdata.vinfo[t].i->insname ?
					 playdata.vinfo[t].i->insname : "");
		} else if (playdata.vinfo[t].s) {
			for (i=0; i < mf->numsmp && playdata.vinfo[t].s != &mf->samples[i];
				i++);
			SNPRINTF(storage, STORAGELEN, "%3i %s", i,
					 playdata.vinfo[t].s->samplename ?
					 playdata.vinfo[t].s->samplename : "");
		}
		convert_string(storage);

		tmp = storage;
		for (v = 0; *tmp && (v < winx - barw - 2); tmp++, v++);
		for (; v < winx - barw - 2; tmp++, v++)
			*tmp = ' ';
		*tmp = 0;
		win_attrset(ATTR_VOLBAR_INSTR);
		win_print(win, (mf->numchn > 100 ? 6 : 5) + barw + 2, t - first_volbar, storage);
	}

	if (mf->numchn == 1)
		strcpy(storage, "1 Channel");
	else
		SNPRINTF(storage, STORAGELEN, "%d Channels", mf->numchn);
	if (!config.forcesamples && (mf->flags & UF_INST))
		strcat(storage, ", displaying instrument names");
	else
		strcat(storage, ", displaying sample names");
	if (config.fakevolbars)
		strcat(storage, " and fake volume bars");
	else
		strcat(storage, " and real volume bars");
	win_status(storage);
}

static void display_volbars(MWINDOW *win, int diff)
{
	int count, t, winx;

	updatefirst(win, &first_volbar, &winx, &count, NULL, diff, mf->numchn);
	win_clear(win);		/* Sets attrs */
	for (t = first_volbar; t < (first_volbar + count) && t < mf->numchn; t++) {
		if (mf->numchn > 100)
			SNPRINTF(storage, STORAGELEN, "[%3d]", t);
		else
			SNPRINTF(storage, STORAGELEN, "[%2d]", t);
		win_print(win, 0, t - first_volbar, storage);
	}

	/* display the remaining of the window immediately to prevent flickering */
	dynamic_display_volbars(win);
}
#endif

static void display_playentry(MWINDOW *win, PLAYENTRY *pos, PLAYENTRY *cur,
							  int nr, int y, int x, BOOL reverse, int width)
{
	char *name, sort;
	char time[8] = "", tmpfmt[30];
	int timelen = 0;

	if (pos->time > 0) {
		SNPRINTF(time, 7, " %2d:%02d",
				 (int)((pos->time / 60) % 60), (int)(pos->time % 60));
		timelen = strlen(time);
	}
	name = FIND_LAST_DIRSEP(pos->file);
	if (name && !config.fullpaths)
		name++;
	else
		name = pos->file;

	if (pos == cur)
		sort = '>';
	else if (pos->played)
		sort = '*';
	else
		sort = ' ';

	if (pos->archive) {
		if (strlen(name) > width - 13 - timelen) {
			name = name + strlen(name) - (width - 16 - timelen);
			if (timelen) {
				sprintf(tmpfmt, "%%4i %%c...%%-%ds%%s(pack)", width - 22);
				SNPRINTF(storage, STORAGELEN, tmpfmt, nr, sort, name, time);
			} else {
				sprintf(tmpfmt, "%%4i %%c...%%-%ds(pack)", width - 16);
				SNPRINTF(storage, STORAGELEN, tmpfmt, nr, sort, name);
			}
		} else if (timelen) {
			sprintf(tmpfmt, "%%4i %%c%%-%ds%%s(pack)", width - 19);
			SNPRINTF(storage, STORAGELEN, tmpfmt, nr, sort, name, time);
		} else {
			sprintf(tmpfmt, "%%4i %%c%%-%ds(pack)", width - 13);
			SNPRINTF(storage, STORAGELEN, tmpfmt, nr, sort, name);
		}
	} else if (strlen(name) > width - 7 - timelen) {
		name = name + strlen(name) - (width - 10 - timelen);
		if (timelen) {
			sprintf(tmpfmt, "%%4i %%c...%%-%ds%%s", width - 16);
			SNPRINTF(storage, STORAGELEN, tmpfmt, nr, sort, name, time);
		} else {
			sprintf(tmpfmt, "%%4i %%c...%%-%ds", width - 10);
			SNPRINTF(storage, STORAGELEN, tmpfmt, nr, sort, name);
		}
	} else if (timelen) {
		sprintf(tmpfmt, "%%4i %%c%%-%ds%%s", width - 13);
		SNPRINTF(storage, STORAGELEN, tmpfmt, nr, sort, name, time);
	} else {
		sprintf(tmpfmt, "%%4i %%c%%-%ds", width - 7);
		SNPRINTF(storage, STORAGELEN, tmpfmt, nr, sort, name);
	}

	win_attrset(reverse ? ATTR_PLAYENTRY_ACTIVE : ATTR_PLAYENTRY_INACTIVE);
	win_print(win, x, y, storage);
}

/* playlist panel */
static void display_list(MWINDOW *win, int diff, COMMAND com)
{
	static const char *no_data = "\nPlaylist is empty!\n";
	static int actLine = -1;
	int count, semicount, playcount, t, winx, x, width;
	PLAYENTRY *cur;

	playcount = PL_GetLength(&playlist);
	if (actLine >= playcount)
		actLine = playcount - 1;

	if (com == MENU_ACTIVATE) {
		list_open (&actLine);
		return;
	}

	win_clear(win);

	if (playcount) {
		win_get_size(win, &winx, &semicount);
		if (semicount < 0) semicount = 0;

		if (winx < 40 + MINVISIBLE) {
			count = semicount;
			width = winx;
		} else {
			count = semicount * 2;
			width = winx >> 1;
		}
		cur = PL_GetCurrent(&playlist);

		if (actLine < 0) {
			actLine = PL_GetCurrentPos(&playlist);
			first_list = actLine - semicount / 2;
			if (first_list < 0)
				first_list = 0;
		}
		actLine += diff;

		if (actLine < 0)
			actLine = 0;
		else if (actLine >= playcount)
			actLine = playcount - 1;

		if (actLine < first_list)
			first_list = actLine;
		else if (actLine >= first_list + count)
			first_list = actLine - count + 1;

		for (t = first_list; t < playcount && t < (count + first_list); t++) {
			x = (t - first_list) < semicount ? 0 : width;
			if (x < winx)
				display_playentry(win, PL_GetEntry(&playlist, t), cur, t,
								  (t - first_list) % semicount,
								  x, actLine == t, width);
		}
	} else {
		first_list += diff;
		first_list = display_banner(win, no_data, 0, first_list, 1);
	}
	switch (playcount) {
		case 0:
			win_status("Press enter to open playlist menu");
			break;
		case 1:
			win_status("1 Module");
			break;
		default:
			SNPRINTF(storage, STORAGELEN, "%d Modules", playcount);
			win_status(storage);
			break;
	}
}

/* open config-editor panel */
static void display_config(MWINDOW *win, int diff)
{
	static BOOL open = 0;

	win_clear(win);
	if (!open) {
		config_open();
		open = 1;
	}
}

/* display panel contents */
static void display_panel(MWINDOW *win, int diff, COMMAND com)
{
#if LIBMIKMOD_VERSION >= 0x030200
	dynamic_repaint = NULL;
	dynamic_repaint_win = win;
#endif
	switch (cur_display) {
	  case DISPLAY_HELP:
		display_help(win, diff);
		break;
	  case DISPLAY_SAMPLE:
#if LIBMIKMOD_VERSION >= 0x030200
		dynamic_repaint = dynamic_display_sample;
#endif
		display_sample(win, diff);
		break;
	  case DISPLAY_INST:
#if LIBMIKMOD_VERSION >= 0x030200
		dynamic_repaint = dynamic_display_inst;
#endif
		display_inst(win, diff);
		break;
	  case DISPLAY_MESSAGE:
		display_comment(win, diff);
		break;
	  case DISPLAY_LIST:
		display_list(win, diff, com);
		break;
	  case DISPLAY_CONFIG:
		display_config(win, diff);
		break;
#if LIBMIKMOD_VERSION >= 0x030200
	  case DISPLAY_VOLBARS:
		dynamic_repaint = dynamic_display_volbars;
		display_volbars(win, diff);
		break;
#endif
	}
}

/* displays the top of the screen */
int display_header(void)
{
	if (quiet)
		return 1;

	display_version();
	update_message();
	if (MP_Paused()) {
		display_pausebanner();
		set_window_title("paused");
	}
	else {
		win_attrset(ATTR_SONG_STATUS);
		display_driver();
		display_file();
		display_name();
		display_status();
		display_title();
	}
	return display_information();
}

static void display_head_resize (MWINDOW *win, int dx, int dy)
{
	setup_printf();
}

static BOOL display_head_repaint(MWINDOW * win)
{
	int cur_panel = win_get_panel();
	if (cur_panel != cur_display)
		old_display = cur_display;
	cur_display = cur_panel;

	return display_header();
}

static BOOL display_panel_repaint(MWINDOW * win)
{
	display_panel(win, 0, COM_NONE);
	return 1;
}

void display_start(void)
{
	if (quiet)
		return;

	first_inst = first_sample = first_comment = 0;
	win_panel_repaint();
}

/* handle interface-specific keys */
static BOOL display_handle_key(MWINDOW * win, int ch)
{
	switch (ch) {
	  case KEY_DOWN:
		display_panel(win, 1, COM_NONE);
		break;
	  case KEY_UP:
		display_panel(win, -1, COM_NONE);
		break;
	  case KEY_RIGHT:
		if (cur_display != DISPLAY_LIST)
			return 0;
		/* fall through */
	  case KEY_NPAGE:
		display_panel(win, win->height, COM_NONE);
		break;
	  case KEY_LEFT:
		if (cur_display != DISPLAY_LIST)
			return 0;
		/* fall through */
	  case KEY_PPAGE:
		display_panel(win, -win->height, COM_NONE);
		break;
	  case KEY_HOME:
		display_panel(win, -32000, COM_NONE);
		break;
#ifdef KEY_END
	  case KEY_END:
		display_panel(win, 32000, COM_NONE);
		break;
#endif
	  case KEY_ENTER:
	  case '\r':
		if (cur_display == DISPLAY_LIST)
			display_panel(win, 0, MENU_ACTIVATE);
		else
			return 0;
		break;
	  default:
		return 0;
	}
	return 1;
}

/* setup interface */
void display_init(void)
{
	static ATTRS attrs[]={ATTR_HELP,			/* Help */
						  ATTR_SAMPLES,			/* Sample */
						  ATTR_SAMPLES,			/* Inst */
						  ATTR_HELP,			/* Message */
						  ATTR_PLAYENTRY_INACTIVE,/* Playlist */
						  ATTR_CONFIG,			/* Config */
						  ATTR_VOLBAR};			/* Volbars */
	int i;

	root = win_get_window_root();
	win_panel_set_repaint(DISPLAY_ROOT, display_head_repaint);
	win_panel_set_resize(DISPLAY_ROOT, 1, display_head_resize);

	for (i = 1; i < DISPLAY_COUNT; i++) {
		win_panel_open(i, 0, PANEL_Y, 999, 999, 0, NULL, attrs[i-1]);
		win_panel_set_repaint(i, display_panel_repaint);
		win_panel_set_handle_key(i, display_handle_key);
		win_panel_set_resize(i, 1, NULL);
	}
	win_change_panel(cur_display);
	setup_printf();
}

static void display_title(void)
{
	char *file;

	if (!mf) { return; }
	
	if (!mf->songname || strlen(mf->songname)==0) {
		PLAYENTRY *entry=NULL;
		entry = PL_GetCurrent(&playlist);

		if (entry != NULL) {
			file = entry->file;

			if (!config.fullpaths) {
					file = FIND_LAST_DIRSEP(entry->file);
					if (file) {
						file++;
					} else {
						file = entry->file;
					}
			}
			set_window_title(file);
		}

		return;
	}

	set_window_title(mf->songname);
}

/* This will set the xterm (or equivalent) Title and Icon title.
 *
 * The title contains -= MikMod x.x.x =- (%s) where %s is the content
 * the icon contains -= MikMod x.x.x =-
 *
 * pass NULL as songname to reset the title
 */
static void set_window_title(const char *content)
{
	/* TODO: Can we do something similar for OS2? */

	/* Win32 console application set title */
#if defined(_WIN32)
	SNPRINTF(storage,STORAGELEN,"%s (%s)", mikversion, content);
	SetConsoleTitle(storage);
#endif

	/* Unix/Xterm (and compatible/similar)
	 *
	 * Written using the 'Xterm-Title mini-howto' 
	 */
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
	char *env_term;
	static int last_config=0;

	if (!config.window_title && !last_config) {
		return;
	}
	if (last_config && !config.window_title) {
		/* xterm title setting has just been disabled */
		content = NULL;
	}

	last_config = config.window_title;

	env_term = getenv("TERM");
	if (env_term==NULL) {
		return;
	}

	if (content!=NULL)
	{
		SNPRINTF(storage,STORAGELEN,"%s (%s)", mikversion, content);
	}
	else
	{
		storage[0] = '\0';
	}

	if ( strcmp(env_term, "xterm")==0 ||
		strcmp(env_term, "xterm-color")==0 ||
		strcmp(env_term, "rxvt")==0 ||
		strcmp(env_term, "aixterm")==0 ||
		strcmp(env_term, "dtterm")==0 ||
		strcmp(env_term, "Eterm")==0 )
	{
		printf("%c]0;%s%c", '\033', storage, '\007');
		printf("%c]1;%s%c", '\033', mikversion, '\007');
	}
	else if (strcmp(env_term, "iris-ansi")==0)
	{
		printf("%cP1.y%s%c\\", '\033', storage, '\033');
		printf("%cP3.y%s%c\\", '\033', mikversion, '\033');
	}
	else if (strcmp(env_term, "hpterm")==0) 
	{
		printf("\033&f0k%dD%s", (int) strlen(storage), storage);
		printf("\033&f-1k%dD%s", (int) strlen(mikversion), mikversion);
	}
#endif
}

/* ex:set ts=4: */
