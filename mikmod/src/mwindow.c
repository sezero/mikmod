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

  $Id: mwindow.c,v 1.1.1.1 2004/01/16 02:07:36 raph Exp $

  Some window functions

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if !defined(GWINSZ_IN_SYS_IOCTL) && defined(HAVE_TERMIOS_H)
#include <termios.h>
#endif
#endif

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
#if defined(__OS2__)||defined(__EMX__)
#define INCL_VIO
#define INCL_DOS
#define INCL_KBD
#define INCL_DOSPROCESS
#endif
#include <conio.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <mikmod.h>
#include "display.h"
#include "player.h"
#include "mwindow.h"
#include "mutilities.h"
#include "keys.h"
#include "mthreads.h"

#define INVISIBLE(w)		(win_quiet || ((w)!=cur_window && (w)!=panel[0]))
#define INVISIBLE_RET(w)	if (win_quiet || ((w)!=cur_window && (w)!=panel[0])) return;

#ifdef ACS_ULCORNER

#define BOX_UL		ACS_ULCORNER
#define BOX_UR		ACS_URCORNER
#define BOX_LL		ACS_LLCORNER
#define BOX_LR		ACS_LRCORNER
#define BOX_HLINE	ACS_HLINE
#define BOX_VLINE	ACS_VLINE

#else

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
#define BOX_UL		'\xda'
#define BOX_UR		'\xbf'
#define BOX_LL		'\xc0'
#define BOX_LR		'\xd9'
#define BOX_HLINE	'\xc4'
#define BOX_VLINE	'\xb3'
#else
#define BOX_UL		'+'
#define BOX_UR		'+'
#define BOX_LL		'+'
#define BOX_LR		'+'
#define BOX_HLINE	'-'
#define BOX_VLINE	'|'
#endif

#endif

void win_do_resize(int dx, int dy, BOOL root);

/* text creation buffer */
char storage[STORAGELEN+2];

static int root_y1 = 7, root_y2 = 0;	/* size of visible root window partions */
static BOOL curses_on = 0, win_quiet = 1;
static int cur_panel = 0, old_panel = 0;
static MWINDOW *panel[DISPLAY_COUNT], *cur_window = NULL;

static BOOL use_colors = 1;
static int act_color = A_NORMAL;
static THEME *theme = NULL;

static int winx = 0, winy = 0;			/* screen size */

typedef struct TIMEOUT {
	WinTimeoutFunc func;
	void *data;
	int interval;
	/* remaining time for the execution of this timeout compared
	   to the timeout located in the timeouts array before this one */
	int remaining;
} TIMEOUT;

static int cnt_timeouts = 0;
static TIMEOUT *timeouts = NULL;

/*========== Display routines */

#if defined(__OS2__)||defined(__EMX__)
#include "os2video.inc"

#elif defined(__DJGPP__)
#include "dosvideo.inc"

#elif defined(_WIN32)
#include "winvideo.inc"

#else /* unix, ncurses */

static int cursor_old = 0;
static BOOL resize = 0;

/* old AIX curses are very limited */
#if defined(AIX) && !defined(mvaddnstr)
void mvaddnstr(int y, int x, const char *str, int len)
{
	char buffer[STORAGELEN];
	int l = strlen(str);

	strncpy(buffer, str, len);
	if (l < len)
		while (l < len)
			buffer[l++] = ' ';
	buffer[len] = '\0';
	mvaddstr(y, x, buffer);
}
#endif

/* HP-UX curses macros don't work with every cpp */
#if defined(__hpux)
void getmaxyx_hpux(MWINDOW * win, int *y, int *x)
{
	*y = __getmaxy(win);
	*x = __getmaxx(win);
}

#define getmaxyx(win,y,x) getmaxyx_hpux((win),&(y),&(x))
#endif

#if defined(AIX) && !defined(NCURSES_VERSION) && !defined(getmaxyx)
#define getmaxyx(win,y,x) (y = LINES, x = COLS)
#endif

#if !defined(getmaxyx)
#define getmaxyx(w,y,x) ((y) = getmaxy(w), (x) = getmaxx(w))
#endif

/* handler for terminal resize events */
RETSIGTYPE sigwinch_handler(int signum)
{
	/* schedule a resizeterm() */
	resize = 1;

	signal(SIGWINCH, sigwinch_handler);
}

/* update window */
void win_refresh(void)
{
	if (win_quiet)
		return;
	refresh();
}

void win_cursor_set(BOOL visible)
{
	if (cursor_old != MIK_CURSES_ERROR) {
		if (visible)
			curs_set(cursor_old);
		else
			curs_set(0);
	}
}

#define COLOR_CNT 8
static void init_curses(void)
{
	initscr();
	cbreak();
	noecho();
	nonl();
	nodelay(stdscr, TRUE);
#if !defined(AIX) || defined(NCURSES_VERSION)
	timeout(0);
#endif
	keypad(stdscr, TRUE);
	cursor_old = curs_set(0);
	curses_on = 1;

	/* Color setup */
	start_color();
	if (has_colors() && (COLOR_PAIRS >= COLOR_CNT*COLOR_CNT)) {
		static short colors[] = {
			COLOR_BLACK,
			COLOR_BLUE,
			COLOR_GREEN,
			COLOR_CYAN,
			COLOR_RED,
			COLOR_MAGENTA,
			COLOR_YELLOW,
			COLOR_WHITE
		};
		int i,j;
		for (i = 0; i < COLOR_CNT; i++)
			for (j = 0; j < COLOR_CNT; j++)
				if (i*COLOR_CNT+j+1 < COLOR_CNT*COLOR_CNT)
					init_pair(i*COLOR_CNT+j+1, colors[j], colors[i]);
		use_colors = 1;
	} else
		use_colors = 0;
}

static int color_to_pair (int attrs)
{
	return 1 +
		((attrs & COLOR_FMASK) >> COLOR_FSHIFT) +
		((attrs & COLOR_BMASK) >> COLOR_BSHIFT) * COLOR_CNT;
}

/* system dependant window init function */
void win_init_system(void)
{
	if (!win_quiet) {
		init_curses();
		getmaxyx(stdscr, winy, winx);
		signal(SIGWINCH, sigwinch_handler);
	}
}

/* clean up (e.g. exit curses) */
void win_exit(void)
{
	if (win_quiet || !curses_on)
		return;

	signal(SIGWINCH, SIG_DFL);
	clear();
	mvaddnstr(winy - 2, 0, mikversion, winx);
	win_refresh();
	win_cursor_set(1);
	endwin();
	curses_on = 0;
}

/* clear to end of line on window win */
void win_clrtoeol(MWINDOW *win, int x, int y)
{
	int len = win->width - x;

	INVISIBLE_RET(win);

	if (len > 0) {
		memset(storage, ' ', len);
		storage[len] = '\0';
		mvaddnstr(win->y + y, win->x + x, storage, len);
	}
}

/* check if a resize was scheduled and do it */
BOOL win_check_resize(void)
{
	static BOOL in_check_resize = 0;

	if (win_quiet || in_check_resize)
		return 0;
	in_check_resize = 1;

	/* if a resize was scheduled, do it now */
	if (resize) {
		int oldx, oldy;
#if (NCURSES_VERSION_MAJOR >= 4) && defined(TIOCGWINSZ) && defined(HAVE_NCURSES_RESIZETERM)
		struct winsize ws;

		ws.ws_col = ws.ws_row = 0;
		ioctl(0, TIOCGWINSZ, &ws);
		if (ws.ws_col && ws.ws_row)
			resizeterm(ws.ws_row, ws.ws_col);
#else
		endwin();
		init_curses();
		win_refresh();
#endif
		resize = 0;
		oldx = winx;
		oldy = winy;
		getmaxyx(stdscr, winy, winx);
		win_do_resize(winx - oldx, winy - oldy, 1);
		in_check_resize = 0;
		return 1;
	}
	in_check_resize = 0;
	return 0;
}

static int win_getch(void)
{
	int c = getch();
	win_check_resize();
/*	if (c>0) fprintf (stderr," %d ",c);*/
	return c == MIK_CURSES_ERROR ? 0 : c;
}

#endif /* #ifdef unix */

/*========== Windowing system */

/* init window functions (e.g. init curses) */
void win_init(BOOL quiet)
{
	win_quiet = quiet;

	win_init_system();

	win_open(0, 0, winx, winy, 0, NULL, ATTR_SONG_STATUS);
	win_set_resize(1, NULL);
}

/* Does the terminal support colors? */
BOOL win_has_colors (void)
{
	return use_colors;
}

/* set the attribute translation table */
void win_set_theme (THEME *new_theme)
{
	theme = new_theme;
}

/* clear window win */
BOOL win_clear(MWINDOW * win)
{
	if (INVISIBLE(win)) return 1;

	if ((win->width > 0) && (win->height > 0)) {
		int i;

		win_attrset(win->attrs);
		memset(storage, ' ', win->width);
		storage[win->width] = '\0';

		if (win==panel[0]) {
			for (i = 0; i < win->height && i < root_y1; i++)
				mvaddnstr(win->y + i, win->x, storage, win->width);
			i = win->height - root_y2;
			if (i < 0)
				i = 0;
			for (; i < win->height; i++)
				mvaddnstr(win->y + i, win->x, storage, win->width);
		} else {
			for (i = 0; i < win->height; i++)
				mvaddnstr(win->y + i, win->x, storage, win->width);
		}
	}
	return 1;
}

void win_box_win(int x1, int y1, int x2, int y2, const char *title)
{
	int i, sx1, sx2, sy1, sy2;

	if (win_quiet)
		return;

	sx1 = x1 >= 0 ? x1 + 1 : 0;
	sx2 = x2 < winx ? x2 - 1 : winx - 1;
	sy1 = y1 >= root_y1 ? y1 + 1 : root_y1;
	sy2 = y2 < winy - root_y2 ? y2 - 1 : winy - root_y2;

	if (y2 < winy - root_y2) {
		if (x1 >= 0)
			mvaddch(y2, x1, BOX_LL);
		if (x2 < winx)
			mvaddch(y2, x2, BOX_LR);

		for (i = sx1; i <= sx2; i++)
			mvaddch(y2, i, BOX_HLINE);
	}
	if (y1 >= root_y1) {
		if (x1 >= 0)
			mvaddch(y1, x1, BOX_UL);
		if (x2 < winx)
			mvaddch(y1, x2, BOX_UR);

		i = sx1;
		if (title)
			for (; i <= sx2 && *title; i++)
				mvaddch(y1, i, *title++);
		for (; i <= sx2; i++)
			mvaddch(y1, i, BOX_HLINE);
	}
	for (i = sy1; i <= sy2; i++) {
		if (x1 >= 0)
			mvaddch(i, x1, BOX_VLINE);
		if (x2 < winx)
			mvaddch(i, x2, BOX_VLINE);
	}
}

/* open new window on panel 'panel' */
MWINDOW *win_panel_open(int dst_panel, int x, int y, int width, int height,
				BOOL border, const char *title, ATTRS attrs)
{
	MWINDOW *win = (MWINDOW *) malloc(sizeof(MWINDOW)), *help;

	int ofs = (border ? 1 : 0);

	if (x < ofs)
		x = ofs;
	if (!dst_panel) {			/* root panel */
		if (y < ofs)
			y = ofs;
		if (y + height > winy - ofs)
			height = winy - y - ofs;
	} else {
		if (y < ofs + root_y1)
			y = ofs + root_y1;
		if (y + height > winy - ofs - root_y2)
			height = winy - y - ofs - root_y2;
	}
	if (x + width > winx - ofs)
		width = winx - x - ofs;
	if (width < 0)
		width = 0;

	if (height < 0)
		height = 0;

	win_attrset(attrs);
	if (border && (dst_panel == cur_panel))
		win_box_win(x - 1, y - 1, x + width, y + height, title);

	win->x = x;
	win->y = y;
	win->width = width;
	win->height = height;
	win->attrs = attrs;
	win->border = border;
	win->resize = 0;
	if (title)
		win->title = strdup(title);
	else
		win->title = NULL;
	win->next = NULL;
	win->repaint = win_clear;
	win->handle_key = NULL;
	win->handle_resize = NULL;
	for (help = panel[dst_panel]; help && help->next; help = help->next);
	if (help)
		help->next = win;
	else
		panel[dst_panel] = win;
	if (dst_panel == cur_panel)
		cur_window = win;
	return win;
}

/* open new window on current panel */
MWINDOW *win_open(int x, int y, int width, int height, BOOL border,
				  const char *title, ATTRS attrs)
{
	return win_panel_open(cur_panel, x, y, width, height, border, title, attrs);
}

MWINDOW *win_get_first(int dst_panel)
{
	MWINDOW *win;
	for (win = panel[dst_panel]; win && win->next; win = win->next);
	return win;
}

/* set function which sould be called on a repaint request */
void win_set_repaint(WinRepaintFunc func)
{
	cur_window->repaint = func;
}
void win_panel_set_repaint(int _panel, WinRepaintFunc func)
{
	win_get_first(_panel)->repaint = func;
}

/* set function which sould be called on a key press */
void win_set_handle_key(WinKeyFunc func)
{
	cur_window->handle_key = func;
}
void win_panel_set_handle_key(int _panel, WinKeyFunc func)
{
	win_get_first(_panel)->handle_key = func;
}

/* should window be automatically resized?
   should a function be called on resize? */
void win_set_resize(BOOL auto_resize, WinResizeFunc func)
{
	cur_window->resize = auto_resize;
	cur_window->handle_resize = func;
}
void win_panel_set_resize(int _panel, BOOL auto_resize, WinResizeFunc func)
{
	MWINDOW *win = win_get_first(_panel);
	win->resize = auto_resize;
	win->handle_resize = func;
}

/* set private data */
void win_set_data(void *data)
{
	cur_window->data = data;
}

void win_panel_set_data(int _panel, void *data)
{
	win_get_first(_panel)->data = data;
}

void win_do_resize(int dx, int dy, BOOL root)
{
	MWINDOW *win;
	int i = root ? 0 : 1;

	if (win_quiet)
		return;

	for (; i < DISPLAY_COUNT; i++)
		for (win = panel[i]; win; win = win->next) {
			if (win->resize) {
				win->width += dx;
				win->height += dy;
			}
			if (win->handle_resize)
				win->handle_resize(win, dx, dy);
		}
	win_panel_repaint_force();
}

static char status_message[MAXWIDTH + 2];

static void win_status_repaint(void)
{
	MWINDOW *win = panel[0];
	int i;
	if (win_quiet)
		return;

	if ((root_y2 > 1) && (win->height > root_y1+1)) {
		win_attrset(ATTR_STATUS_LINE);
		for (i = 0; i < win->width; i++)
			mvaddch(win->height - 2, i, BOX_HLINE);
	}
}

/* init the status line(height=0,1,2  0: no status line) */
void win_init_status(int height)
{
	int old_y2 = root_y2;

	if (height != root_y2) {
		root_y2 = height < 0 ? 0 : (height > 2 ? 2 : height);
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
		status_message[0] = '\0';
#else
		status_message[0] = '\n';
		status_message[1] = '\0';
#endif
		win_do_resize(0, old_y2 - root_y2, 0);
	}
}

/* set the status line */
void win_status(const char *msg)
{
	MWINDOW *win = panel[0];
	int len;

	if (msg) {
		len = strlen(msg);
		if (len > MAXWIDTH)
			len = MAXWIDTH;
		strncpy(status_message, msg, len);
	} else
		len = strlen(status_message);
	status_message[len] = '\0';

	if (win_quiet)
		return;
	if ((root_y2 > 0) && (win->height > root_y1) && (win->width>0)) {
		win_attrset(ATTR_STATUS_TEXT);
		mvaddnstr(win->y + win->height - 1, win->x, status_message,
				  win->width);
		win_clrtoeol(win, win->x + len, win->y + win->height - 1);
	}
}

/* repaint the whole panel */
void win_panel_repaint(void)
{
	if (win_quiet)
		return;

	if (panel[cur_panel])
		win_clear(panel[cur_panel]);

	if (panel[0]->repaint)
		if (!panel[0]->repaint(panel[0]))
			return;
	for (cur_window = panel[cur_panel]; cur_window;
		 cur_window = cur_window->next) {
		win_attrset(cur_window->attrs);
		if (cur_window->border && cur_window->width >= 0 &&
			cur_window->height >= 0)
			win_box_win(cur_window->x - 1, cur_window->y - 1,
						cur_window->x + cur_window->width,
						cur_window->y + cur_window->height, cur_window->title);
		if (cur_window->repaint && cur_window->width > 0 &&
			cur_window->height > 0)
			if (!cur_window->repaint(cur_window))
				return;
	}
	win_status_repaint();
	win_status(NULL);
	for (cur_window = panel[cur_panel]; cur_window && cur_window->next;
		 cur_window = cur_window->next);
}

/* repaint the whole panel, clear whole panel before */
void win_panel_repaint_force(void)
{
	if (win_quiet) return;
	clear();
	win_panel_repaint();
}

/* close window win */
void win_close(MWINDOW * win)
{
	int i;
	MWINDOW *pos;

	for (i = 0; i < DISPLAY_COUNT; i++)
		for (pos = panel[i]; pos; pos = pos->next)
			if (pos == win) {
				if (win == cur_window)
					for (cur_window = panel[i]; cur_window->next != win;
						 cur_window = cur_window->next);
				for (pos = panel[i]; pos->next != win; pos = pos->next);
				pos->next = win->next;
				if (win->title)
					free(win->title);
				free(win);
				if (i == cur_panel)
					win_panel_repaint();
				return;
			}
}

/* get size of window win */
void win_get_size(MWINDOW *win, int *x, int *y)
{
	*x = win->width;
	*y = win->height;
}

/* get maximal size of a new window without a border and therefore the needed
   minimal y position */
void win_get_size_max(int *y, int *width, int *height)
{
	*y = root_y1;
	*width = panel[0]->width;
	*height = panel[0]->height - root_y1 - root_y2;
	if (*height < 0)
		*height = 0;
}

/* get uppermost window */
MWINDOW *win_get_window(void)
{
	return cur_window;
}

/* get root window */
MWINDOW *win_get_window_root(void)
{
	return panel[0];
}

/* print string in window win */
void win_print(MWINDOW *win, int x, int y, const char *str)
{
	int len = strlen(str);

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
	if (len > 1 && str[len - 1] == '\n' && str[len - 2] == '\r')
		len--;
#endif
	INVISIBLE_RET(win);

	if ((x >= win->width) || (y >= win->height) ||
		((win != panel[0]) && (y + win->y >= winy - root_y2)) ||
		(!len))
		return;

	if (len + x > win->width)
		len = win->width - x;
	if (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r'))
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
		if (win->x + win->width < winx)
#endif
		{
			len--;
			win_clrtoeol(win, x + len, y);
		}
	mvaddnstr(y + win->y, x + win->x, str, len);
}

/* draw horizontal/verticall line */
void win_line(MWINDOW *win, int x1, int y1, int x2, int y2)
{
	int i;

	INVISIBLE_RET(win);

	if (y1 == y2) {
		if (y1 < cur_window->height)
			for (i = x1; i <= x2 && i < cur_window->width; i++)
				mvaddch(y1 + cur_window->y, i + cur_window->x, BOX_HLINE);
	} else {
		if (x1 < cur_window->width)
			for (i = y1; i <= y2 && i < cur_window->height; i++)
				mvaddch(i + cur_window->y, x1 + cur_window->x, BOX_VLINE);
	}
}

/* draw a box with colored background
   back:  background colors from UL UR LR LL to UL */
void win_box_color(MWINDOW *win, int x1, int y1, int x2, int y2, ATTRS *back)
{
	int i, j, k, sx1, sx2, sy1, sy2, maxx, maxy;

	INVISIBLE_RET(win);

	x1 += win->x;
	x2 += win->x;
	y1 += win->y;
	y2 += win->y;

	maxx = win->x+win->width-1;
	maxy = win->y+win->height-1;

	sx1 = x1>=win->x ? x1+1 : win->x;
	sy1 = y1>=win->y ? y1+1 : win->y;
	sx2 = x2<=maxx ? x2 - 1 : maxx;
	sy2 = y2<=maxy ? y2 - 1 : maxy;

	if (y2 <= maxy) {
		if (x1 >= win->x) {
			if (back)
				win_set_background (back[x2-x1+x2-x1+y2-y1]);
			mvaddch(y2, x1, BOX_LL);
		}
		if (x2 <= maxx) {
			if (back)
				win_set_background (back[x2-x1+y2-y1]);
			mvaddch(y2, x2, BOX_LR);
		}
		j = x2-x1+x2-sx1+y2-y1;
		for (i = sx1; i <= sx2; i++) {
			if (back)
				win_set_background (back[j--]);
			mvaddch(y2, i, BOX_HLINE);
		}
	}
	if (y1 >= win->y) {
		if (x1 >= win->x) {
			if (back)
				win_set_background (back[0]);
			mvaddch(y1, x1, BOX_UL);
		}
		if (x2 <= maxx) {
			if (back)
				win_set_background (back[x2-x1]);
			mvaddch(y1, x2, BOX_UR);
		}
		j = sx1-x1;
		for (i=sx1; i <= sx2; i++) {
			if (back)
				win_set_background (back[j++]);
			mvaddch(y1, i, BOX_HLINE);
		}
	}
	j = x2-x1+sy1-y1;
	k = x2-x1+x2-x1+y2-y1+y2-sy1;
	for (i = sy1; i <= sy2; i++) {
		if (x1 >= win->x) {
			if (back)
				win_set_background (back[k--]);
			mvaddch(i, x1, BOX_VLINE);
		}
		if (x2 <= maxx) {
			if (back)
				win_set_background (back[j++]);
			mvaddch(i, x2, BOX_VLINE);
		}
	}
}
/* draw a box */
void win_box(MWINDOW *win, int x1, int y1, int x2, int y2)
{
	win_box_color (win, x1, y1, x2, y2, NULL);
}

/* set attribute for the following output operations,
   "attrs" is an index into the theme->attr translation table */
void win_attrset(ATTRS attrs)
{
	if (theme && !win_quiet) {
		int theme_attr = theme->attrs[attrs];
		act_color = theme_attr;
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
		if (theme->color) {
			int pair;
			if (theme_attr == ((COLOR_CNT-1) << COLOR_BSHIFT) + ((COLOR_CNT-1) << COLOR_FSHIFT))
				theme_attr = ((COLOR_CNT-1) << COLOR_BSHIFT) + ((COLOR_CNT-2) << COLOR_FSHIFT);
			act_color = theme_attr;
			pair = COLOR_PAIR(color_to_pair(theme_attr));
			if (theme_attr & COLOR_BOLDMASK)
				attrset(pair | A_BOLD);
			else
				attrset(pair);
		} else
#endif
			attrset(theme_attr);
	}
}

ATTRS win_get_theme_color (ATTRS attrs)
{
	if (theme) {
		int theme_attr = theme->attrs[attrs];
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
		if (theme->color)
			if (theme_attr == ((COLOR_CNT-1) << COLOR_BSHIFT) + ((COLOR_CNT-1) << COLOR_FSHIFT))
				theme_attr = ((COLOR_CNT-1) << COLOR_BSHIFT) + ((COLOR_CNT-2) << COLOR_FSHIFT);
#endif
		return theme_attr;
	} else
		return 0;
}

/* set color for the following output operations */
void win_set_color(ATTRS attrs)
{
	if (win_quiet) return;

	act_color = attrs;
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
	if (win_has_colors()) {
		int pair;
		if (attrs == ((COLOR_CNT-1) << COLOR_BSHIFT) + ((COLOR_CNT-1) << COLOR_FSHIFT))
			attrs = ((COLOR_CNT-1) << COLOR_BSHIFT) + ((COLOR_CNT-2) << COLOR_FSHIFT);
		act_color = attrs;
		pair = COLOR_PAIR(color_to_pair(attrs));
		if (attrs & COLOR_BOLDMASK)
			attrset(pair | A_BOLD);
		else
			attrset(pair);
	} else
#endif
		attrset(attrs);
}

void win_set_forground(ATTRS fg)
{
	if (win_has_colors())
		win_set_color ((act_color & COLOR_BMASK) + (fg << COLOR_FSHIFT));
}

void win_set_background(ATTRS bg)
{
	if (win_has_colors())
		win_set_color ((act_color & COLOR_FMASK) + (bg << COLOR_BSHIFT));
}

/* change current panel */
void win_change_panel(int new_panel)
{
	if (new_panel == cur_panel)
		new_panel = old_panel;
	old_panel = cur_panel;
	if (new_panel != cur_panel) {
		cur_panel = new_panel;
		for (cur_window = panel[cur_panel]; cur_window && cur_window->next;
			 cur_window = cur_window->next);
		win_panel_repaint();
	}
}

int win_get_panel(void)
{
	return cur_panel;
}

/* handle key press(panel change and call of key handler
   of uppermost window),return: was key handled */
BOOL win_handle_key(int ch)
{
	int ret;
	switch (ch) {
	  case KEY_F(1):
		win_change_panel(DISPLAY_HELP);
		break;
	  case KEY_F(2):
		win_change_panel(DISPLAY_SAMPLE);
		break;
	  case KEY_F(3):
		win_change_panel(DISPLAY_INST);
		break;
	  case KEY_F(4):
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
	  case KEY_SF(9):			/* shift-F9 */
#else
	  case KEY_F(19):			/* shift-F9 on some curses implementations */
#endif
		win_change_panel(DISPLAY_MESSAGE);
		break;
	  case KEY_F(5):
		win_change_panel(DISPLAY_LIST);
		break;
	  case KEY_F(6):
		win_change_panel(DISPLAY_CONFIG);
		break;
#if LIBMIKMOD_VERSION >= 0x030200
	  case KEY_F(7):
		win_change_panel(DISPLAY_VOLBARS);
		break;
#endif
	  default:
		ret = 0;
		if (cur_window->handle_key)
			ret = cur_window->handle_key(cur_window, ch);
		if (!ret && panel[0]->handle_key)
			ret = panel[0]->handle_key(panel[0], ch);
		return ret;
	}
	return 1;
}

/* Insert src (src!=NULL) or timeouts[0] (src==NULL) sorted after next
   execution in timeouts and set remaining appropriate.
   Expand timeouts array if src!=NULL. */
static void win_timeout_insert (TIMEOUT *src)
{
	int time;
	int sum = 0, oldsum = 0, pos = 0, i;

	if (!src) {
		time = timeouts[0].interval;
		pos++;
	} else
		time = src->interval;

	for (; pos<=cnt_timeouts; pos++) {
		oldsum = sum;
		if (pos<cnt_timeouts)
			sum += timeouts[pos].remaining;
		if (sum>time || pos==cnt_timeouts) {
			if (src) {
				timeouts = (TIMEOUT *) realloc (timeouts,
									sizeof(TIMEOUT)*(++cnt_timeouts));
				for (i=cnt_timeouts-1; i>pos; i--)
					timeouts[i] = timeouts[i-1];
				timeouts[pos] = *src;
			} else {
				TIMEOUT help = timeouts[0];
				pos--;
				for (i=0; i<pos; i++)
					timeouts[i] = timeouts[i+1];
				timeouts[pos] = help;
			}
			timeouts[pos].remaining = time-oldsum;
			return;
		}
	}
}

/* add a new timeout function called approx. every interval ms */
void win_timeout_add (int interval, WinTimeoutFunc func, void *data)
{
	TIMEOUT new_timeout;
	new_timeout.func = func;
	new_timeout.data = data;
	new_timeout.interval = interval;
	win_timeout_insert (&new_timeout);
}

static void win_timeout_remove (int number)
{
	int i;
	for (i=number+1; i<cnt_timeouts; i++)
		timeouts[i-1] = timeouts[i];
	cnt_timeouts--;
	if (cnt_timeouts) {
		timeouts = (TIMEOUT *) realloc (timeouts,sizeof(TIMEOUT)*cnt_timeouts);
	} else {
		free (timeouts);
		timeouts = NULL;
	}
}

/* Handle scheduled timeouts and up to one key press,
   return 1 if key presses are pending */
BOOL win_main_iteration(void)
{
	static unsigned long last_time = 0, time;
	static int last_ch = 0;
	int ch;

	time = Time1000();
	if (timeouts && last_time)
		timeouts[0].remaining -= time-last_time;
	last_time = time;

	while (timeouts && timeouts[0].remaining <= 0) {
		if (timeouts[0].func &&
			! timeouts[0].func (cur_window, timeouts[0].data)) {
			win_timeout_remove (0);
		} else
			win_timeout_insert (NULL);
	}

	if (win_quiet) return 0;

	if (last_ch)
		ch = last_ch;
	else
		ch = win_getch();
	if (ch) {
		win_handle_key(ch);
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
/*		flushinp(); */
#endif
		last_ch = win_getch();
	}

	return last_ch != 0;
}

void win_run(void)
{
	do {
		while (win_main_iteration());
		SLEEP(1);
	} while (1);
}

/* ex:set ts=4: */
