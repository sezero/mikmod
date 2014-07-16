/*  MikMod module player
	(c) 1998-2014 Miodrag Vallat and others - see file AUTHORS for
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

  $Id: mwindow.h,v 1.1.1.1 2004/01/16 02:07:36 raph Exp $

  Some window functions

==============================================================================*/

#ifndef MWINDOW_H
#define MWINDOW_H

#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
#  ifdef HAVE_NCURSES_H
#    include <ncurses.h>
#  elif defined HAVE_CURSES_H
#    include <curses.h>
#  elif defined HAVE_NCURSES_CURSES_H
#    include <ncurses/curses.h>
#  endif
#  define MIK_CURSES_ERROR ERR
#else
#  define MIK_CURSES_ERROR (-1)
#endif

#include <mikmod.h>
#include "mconfig.h"

typedef struct MWINDOW {
	int x, y, width, height;	/* Inner pos. and size */
	ATTRS attrs;				/* Window attributes, used for border */
								/* and win_clear() */
	BOOL border;				/* Has window a border? */
	BOOL resize;				/* Window is automatically resized */
	char *title;
	BOOL (*repaint) (struct MWINDOW * win);
	BOOL (*handle_key) (struct MWINDOW * win, int ch);
	void (*handle_resize) (struct MWINDOW * win, int dx, int dy);
	struct MWINDOW *next;

	void *data;					/* not used by window functions */
} MWINDOW;

/* return: 1: continue repaint with other windows
		   0: cancel repaint (if a new repaint was scheduled in the repaint
              func,e.g. by win_change_panel() */
typedef BOOL (*WinRepaintFunc) (MWINDOW *win);
/* return: 1: key was handled */
typedef BOOL (*WinKeyFunc) (MWINDOW *win, int ch);
/* dx,dy: amount of window size change */
typedef void (*WinResizeFunc) (MWINDOW *win, int dx, int dy);
/* called on a timeout, timeout is removed if 0 is returned  */
typedef BOOL (*WinTimeoutFunc) (MWINDOW *win, void *data);

/* init window functions (e.g. init curses) */
void win_init(BOOL quiet);
/* clean up (e.g. exit curses) */
void win_exit(void);

/* Does the terminal support colors? */
BOOL win_has_colors(void);
/* set the attribute translation table */
void win_set_theme (THEME *new_theme);

/* open new window on current panel */
MWINDOW *win_open(int x, int y, int width, int height, BOOL border,
				  const char *title, ATTRS attrs);
/* open new window on panel 'panel' */
MWINDOW *win_panel_open(int dst_panel, int x, int y, int width, int height,
				BOOL border, const char *title, ATTRS attrs);
/* set function which should be called on a repaint request */
void win_set_repaint(WinRepaintFunc func);
void win_panel_set_repaint(int panel, WinRepaintFunc func);
/* set function which sould be called on a key press */
void win_set_handle_key(WinKeyFunc func);
void win_panel_set_handle_key(int panel, WinKeyFunc func);
/* should window be automatically resized?
   should a function be called on resize? */
void win_set_resize(BOOL auto_resize, WinResizeFunc func);
void win_panel_set_resize(int panel, BOOL auto_resize, WinResizeFunc func);
/* set private data */
void win_set_data(void *data);
void win_panel_set_data(int panel, void *data);

/* close window win */
void win_close(MWINDOW * win);

/* repaint the whole panel */
void win_panel_repaint(void);

/* repaint the whole panel, clear whole panel before */
void win_panel_repaint_force(void);

/* init the status line (height=0,1,2  0: no status line) */
void win_init_status(int height);
/* set the status line */
void win_status(const char *msg);

/* clear to end of line on window win */
void win_clrtoeol(MWINDOW *win, int x, int y);
/* clear window win */
BOOL win_clear(MWINDOW *win);

/* get size of window win */
void win_get_size(MWINDOW *win, int *x, int *y);
/* get maximal size of a new window without a border and therefore the needed
   minimal y position */
void win_get_size_max(int *y, int *width, int *height);
/* get uppermost window */
MWINDOW *win_get_window(void);
/* get root window */
MWINDOW *win_get_window_root(void);

/* print string in window win */
void win_print(MWINDOW *win, int x, int y, const char *str);

/* draw horizontal/verticall line */
void win_line(MWINDOW *win, int x1, int y1, int x2, int y2);
/* draw a box with colored background
   back:  background colors from UL UR LR LL to UL */
void win_box_color(MWINDOW *win, int x1, int y1, int x2, int y2, ATTRS *back);
/* draw a box */
void win_box(MWINDOW *win, int x1, int y1, int x2, int y2);

/* set attribute for the following output operations,
   "attrs" is an index into the theme->attr translation table */
void win_attrset(ATTRS attrs);
ATTRS win_get_theme_color (ATTRS attrs);
/* set color for the following output operations */
void win_set_color(ATTRS attrs);
void win_set_forground(ATTRS fg);
void win_set_background(ATTRS bg);

void win_cursor_set(BOOL visible);

/* update window -> call curses.refresh() */
void win_refresh(void);
/* change current panel */
void win_change_panel(int new_panel);
/* return current panel */
int win_get_panel(void);
/* handle key press (panel change and call of key handler
   of uppermost window), return: was key handled */
BOOL win_handle_key(int ch);

/* add a new timeout function called approx. every interval ms */
void win_timeout_add (int interval, WinTimeoutFunc func, void *data);
/* Handle scheduled timeouts and up to one key press,
   return 1 if key presses are pending. */
BOOL win_main_iteration(void);
/* main event handling routine, does NOT return */
void win_run(void);

#endif /* MWINDOW_H */
