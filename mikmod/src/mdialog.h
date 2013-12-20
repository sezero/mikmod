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

  $Id: mdialog.h,v 1.1.1.1 2004/01/16 02:07:40 raph Exp $

  Some common dialog types

==============================================================================*/

#ifndef MDIALOG_H
#define MDIALOG_H

#include "mwidget.h"

/* Function which is called on input
   w     : dlg_input()  : the input widgets
           dlg_message(): the button widget
   button: selected button (str- and int-fields selected -> button==-1)
   input : input in a int- or str-field
   data  : user-pointer which was passed to dlg-function
   Return: close dialog? */
typedef BOOL (*handleDlgFunc) (WIDGET *w, int button, void *input, void *data);

/* Opens a message box
   msg   : text to display, can contain '\n'
   button: ".&..|...|...", &: hotkey, e.g.: "&Yes|&No"
   active: active button (0...n)
   warn  : open message box with ATTR_WARNING?
   data  : passed to handle_dlg */
void dlg_message_open(const char *msg, const char *button, int active, BOOL warn,
					  handleDlgFunc handle_dlg, void *data);

/* Shows a message. If errno is set a text describing the errno
   error code is appended to the message. */
void dlg_error_show(const char *txt, ...);

/* Opens a string input dialog
   msg    : text to display, can contain '\n'
   buttons: definition of the dialog buttons
   str    : default text
   length : max allowed input length */
void dlg_input_str(const char *msg, const char *buttons, const char *str, int length,
				   handleDlgFunc handle_dlg, void *data);

/* Opens an integer input dialog
   msg    : text to display, can contain '\n'
   buttons: definition of the dialog buttons
   value  : default integer
   min,max: min, max allowed values */
void dlg_input_int(const char *msg, const char *buttons, int value, int min, int max,
				   handleDlgFunc handle_dlg, void *data);

#endif /* MDIALOG_H */

/* ex:set ts=4: */
