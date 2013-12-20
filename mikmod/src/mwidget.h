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

  $Id: mwidget.h,v 1.1.1.1 2004/01/16 02:07:33 raph Exp $

  Widget and Dialog creation functions

==============================================================================*/

#ifndef MWIDGET_H
#define MWIDGET_H

#include "mwindow.h"

#define EVENT_HANDLED	100

#define FOCUS_NEXT		(1)			/* next widget gets focus */
#define FOCUS_PREV		(-1)		/* prev widget gets focus */
#define FOCUS_ACTIVATE	(EVENT_HANDLED+1)	/* button select, return in input field */
#define FOCUS_DONT		(EVENT_HANDLED+2)	/* on hotkey: action is done (e.g. toggle */
											/* button is toggled), focus is not changed */
typedef enum {
	WID_SEL_SINGLE,
	WID_SEL_BROWSE
} WID_SEL_MODE;

typedef enum {
	WID_GET_FOCUS,
	WID_HOTKEY,
	WID_KEY
} WID_EVENT;

typedef enum {
	TYPE_LABEL,
	TYPE_STR,
	TYPE_INT,
	TYPE_BUTTON,
	TYPE_LIST,
	TYPE_CHECK,
	TYPE_TOGGLE,
	TYPE_COLORSEL
} WID_TYPE;

typedef struct WIDGET WIDGET;

typedef struct {
	int active;					/* active widget */
	int cnt;					/* Nuber of widgets */
	ATTRS attrs;				/* >=0: use it for DLG_FRAME and DLG_LABEL */
	MWINDOW *win;
	WIDGET **widget;		/* the widgets */
} DIALOG;

struct WIDGET {
	WID_TYPE type;
	BOOL can_focus;				/* can the widget have the focus? */
	BOOL has_focus;				/* has this widget the focus? */
	int x, y, width, height;	/* pos and size of widget (calculated) */
	int def_width, def_height;	/* size set by wid_set_size(), can be used */
								/* by the widget as a default size */

	/* >0 : Number of free lines to last widget
	   =0 : Start of a new column of widget
	   <0 : Start of a new row of columns of widgets, value is spacing
            between this and the previous row */
	int spacing;
	DIALOG *d;

	void (*w_free) (WIDGET *w);
	void (*w_paint) (WIDGET *w);
	int (*w_handle_event) (WIDGET *w, WID_EVENT event, int ch);
	void (*w_get_size) (WIDGET *w, int *width, int *height);

	int (*handle_key) (WIDGET *w, int ch);
	int (*handle_focus) (WIDGET *w, int focus);

	void *data;					/* not used by widget functions */
};

/* called on key press, back:
	+/-n : Widget n entries before/behind Widget w gets the focus
	EVENT_HANDLED: Key is not processed any more
	0 : key is processed by the widgets own handleEventFunc */
typedef int (*handleKeyFunc) (WIDGET *w, int ch);
/* called on focus loose with FOCUS_NEXT, FOCUS_PREV, or FOCUS_ACTIVATE,
   back: EVENT_HANDLED, FOCUS_ACTIVATE, +/-n, or 0 */
typedef int (*handleFocusFunc) (WIDGET *w, int focus);

/* Free substructs of w and w itself */
typedef void (*freeFunc) (WIDGET *w);
/* Display widget w */
typedef void (*paintFunc) (WIDGET *w);
/*
   GET_FOCUS: Widget w gets the focus
     ch: -1: Last active widget was behind the new one
          1: Last active widget was before the new one
   HOTKEY:
     ch: The Key which was pressed
     back:
       FOCUS_ACTIVATE: Widget w gets the focus
       EVENT_HANDLED : Focus is not changed, Key is not processed any more,
                       e.g. necessary if function closes the dialog
   KEY: Key ch was pressed
     back:
       +/-n : Widget n entries before/behind Widget w gets the focus
       EVENT_HANDLED: Key is not processed any more
       0 : event HOTKEY is send to the widgets
*/
typedef int (*handleEventFunc) (WIDGET *w, WID_EVENT event, int ch);
/* Return the size of widget w
   Input: preferred maximal size */
typedef void (*getSizeFunc) (WIDGET *w, int *width, int *height);

typedef struct {
	WIDGET w;
	char *msg;
} WID_LABEL;

typedef struct {
	WIDGET w;
	char *input;
	int cur_pos;				/* cursor position */
	int start;					/* first visible char */
	int length;					/* max length of input */
} WID_STR;

typedef struct {
	WIDGET w;
	char *input;
	int cur_pos;				/* cursor position */
	int start;					/* first visible char */
	int length;					/* max length of input */
} WID_INT;

typedef struct {
	WIDGET w;
	char *button;				/* &but1|but2|... */
	int cnt;					/* number of buttons */
	int active;					/* active button */
} WID_BUTTON;

typedef struct {
	WIDGET w;
	int cur;					/* selected entry */
	int first;					/* first line of list which is displayed */
	int cnt;					/* number of list entries */
	char **entries;				/* the list entries */
	char *title;
	WID_SEL_MODE sel_mode;		/* SINGLE: call of handle_focus() only on return */
} WID_LIST;						/* BROWSE: call of handle_focus() when cur changes */

typedef struct {
	WIDGET w;
	char *button;				/* &but1|but2\nbu&t3\n... */
	int cnt;					/* number of buttons */
	int selected;				/* selected buttons */
	int active;					/* active button */
} WID_CHECK;

typedef struct {
	WIDGET w;
	char *button;				/* &but1|but2\nbu&t3\n... */
	int cnt;					/* number of buttons */
	int selected;				/* selected buttons */
	int active;					/* active button */
} WID_TOGGLE;

typedef struct {
	WIDGET w;
	int active;					/* selected color */
	char hkeys[5];				/* hotkeys to move the selector <>^v */
	WID_SEL_MODE sel_mode;		/* SINGLE: call of handle_focus() only on return */
} WID_COLORSEL;					/* BROWSE: call of handle_focus() when cur changes */

/* spacing:
     >0 : Number of free lines to last widget
     =0 : Start of a new column of widget
     <0 : Start of a new row of columns of widgets, value is spacing
          between this and the previous row */
WIDGET *wid_label_add(DIALOG *d, int spacing, const char *msg);
void wid_label_set_label (WID_LABEL *w, const char *label);

WIDGET *wid_str_add(DIALOG *d, int spacing, const char *input, int length);
void wid_str_set_input (WID_STR *w, const char *input, int length);

WIDGET *wid_int_add(DIALOG *d, int spacing, int value, int length);
void wid_int_set_input(WID_INT *w, int value, int length);

WIDGET *wid_button_add(DIALOG *d, int spacing, const char *button, int active);

WIDGET *wid_list_add(DIALOG *d, int spacing, const char **entries, int cnt);
void wid_list_set_title(WID_LIST *w, const char *title);
void wid_list_set_entries(WID_LIST *w, const char **entries, int cur, int cnt);
void wid_list_set_active(WID_LIST *w, int cur);
void wid_list_set_selection_mode (WID_LIST *w, WID_SEL_MODE mode);

WIDGET *wid_check_add(DIALOG *d, int spacing, const char *button,
					  int selected, int active);
void wid_check_set_selected(WID_CHECK *w, int selected);

WIDGET *wid_toggle_add(DIALOG *d, int spacing, const char *button,
					   int selected, int active);
void wid_toggle_set_selected(WID_TOGGLE *w, int selected);

WIDGET *wid_colorsel_add(DIALOG *d, int spacing, const char *hotkeys, int active);
void wid_colorsel_set_active(WID_COLORSEL *w, int active);

/* Set default size of widget, -1: ignore value */
void wid_set_size (WIDGET *w, int width, int height);
void wid_set_func(WIDGET *w, handleKeyFunc key, handleFocusFunc focus,
				  void *data);

void wid_repaint (WIDGET *w);

DIALOG *dialog_new(void);
void dialog_open(DIALOG *d, const char *title);

/* set attribute which is used for DLG_FRAME and DLG_LABEL,
   works only before dialog_open() */
void dialog_set_attr (DIALOG *d, ATTRS attrs);
BOOL dialog_repaint(MWINDOW *win);
void dialog_close(DIALOG *d);

#endif /* MWIDGET_H */

/* ex:set ts=4: */
