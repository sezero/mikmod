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

  $Id: mdialog.c,v 1.1.1.1 2004/01/16 02:07:40 raph Exp $

  Some common dialog types

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "mwidget.h"
#include "mdialog.h"
#include "display.h"
#include "mutilities.h"

typedef struct {
	handleDlgFunc handle_dlg;
	WIDGET *w;
	void *input;
	void *data;
	int min, max;
} DLG_DATA;

static int handle_focus(struct WIDGET *w, int focus)
{
	if (focus == FOCUS_ACTIVATE) {
		DLG_DATA *data = (DLG_DATA *) w->data;
		if (data) {
			int button = -1;
			if (w->type == TYPE_BUTTON)
				button = ((WID_BUTTON *) w)->active;

			if ((button <= 0) && (data->min >= 0) && (data->max >= 0)) {
				int value = atoi((char*)data->input);
				if ((value < data->min) || (value > data->max))
					return focus;
			}
			if (data->handle_dlg(data->w, button, data->input, data->data)) {
				free(data);
				dialog_close(w->d);
			}
		} else
			dialog_close(w->d);
		return EVENT_HANDLED;
	}
	return focus;
}

static DLG_DATA *init_dlg_data(handleDlgFunc handle_dlg,
							   WIDGET *w, void *input, void *data)
{
	DLG_DATA *dlg_data = NULL;
	if (handle_dlg) {
		dlg_data = (DLG_DATA *) malloc(sizeof(DLG_DATA));
		dlg_data->handle_dlg = handle_dlg;
		dlg_data->w = w;
		dlg_data->input = input;
		dlg_data->data = data;
		dlg_data->min = dlg_data->max = -1;
	}
	return dlg_data;
}

/* Opens a message box
   msg   : text to display,can contain '\n'
   button: ".&..|...|...",&: hotkey,e.g.: "&Yes|&No"
   active: active button(0...n)
   warn  : open message box with ATTR_WARNING?
   data  : passed to handle_dlg */
void dlg_message_open(const char *msg, const char *button, int active, BOOL warn,
					  handleDlgFunc handle_dlg, void *data)
{
	WIDGET *w;
	DIALOG *d = dialog_new();

	if (warn) dialog_set_attr (d,ATTR_WARNING);
	wid_label_add(d, 1, msg);
	w = wid_button_add(d, 2, button, active);
	if (handle_dlg)
		wid_set_func(w, NULL, handle_focus,
					 init_dlg_data(handle_dlg, w, NULL, data));
	dialog_open(d, "Message");
}

/* Shows a message. If errno is set a text describing the errno
   error code is appended to the message. */
void dlg_error_show(const char *txt, ...)
{
	va_list args;
	char *err = NULL;
	int len;

	if (errno) {
#ifdef HAVE_STRERROR
		err = strerror(errno);
#else
		err = (errno >= sys_nerr) ? "(unknown error)" : sys_errlist[errno];
#endif
	}
	va_start(args, txt);
	VSNPRINTF (storage, STORAGELEN, txt, args);
	va_end(args);

	len = strlen(storage);
	if (len<STORAGELEN-2 && err && *err) {
		strncat (storage,"\n",STORAGELEN-len);
		strncat (storage,err,STORAGELEN-len-1);
	}
	dlg_message_open(storage, "&Ok", 0, 1, NULL, NULL);
}

/* Opens a string input dialog
   msg   : text to display,can contain '\n'
   str   : default text
   length: max allowed input length */
void dlg_input_str(const char *msg, const char *buttons, const char *str, int length,
				   handleDlgFunc handle_dlg, void *data)
{
	WIDGET *w, *str_wid;
	DLG_DATA *dlg_data;
	DIALOG *d = dialog_new();

	if (msg)
		wid_label_add(d, 1, msg);
	str_wid = wid_str_add(d, 1, str, length);
	w = wid_button_add(d, 2, buttons, 0);

	dlg_data = init_dlg_data(handle_dlg, str_wid, ((WID_STR*)str_wid)->input, data);
	wid_set_func(str_wid, NULL, handle_focus, dlg_data);
	wid_set_func(w, NULL, handle_focus, dlg_data);

	dialog_open(d, "Enter string");
}

/* Opens an integer input dialog
   msg   : text to display,can contain '\n'
   value : default integer
   min,max: min,max allowed values */
void dlg_input_int(const char *msg, const char *buttons, int value, int min, int max,
				   handleDlgFunc handle_dlg, void *data)
{
	char title[40];
	WIDGET *w, *int_wid;
	DLG_DATA *dlg_data;
	DIALOG *d = dialog_new();

	if (msg)
		wid_label_add(d, 1, msg);
	sprintf(title, "%d", max);
	int_wid = wid_int_add(d, 1, value, strlen(title));
	w = wid_button_add(d, 2, buttons, 0);

	dlg_data = init_dlg_data(handle_dlg, int_wid, ((WID_INT*)int_wid)->input, data);
	dlg_data->min = min;
	dlg_data->max = max;
	wid_set_func(int_wid, NULL, handle_focus, dlg_data);
	wid_set_func(w, NULL, handle_focus, dlg_data);

	sprintf(title, "Enter value(%d - %d)", min, max);
	dialog_open(d, title);
}

/* ex:set ts=4: */
