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

  $Id: mmenu.h,v 1.1.1.1 2004/01/16 02:07:38 raph Exp $

  Menu functions

==============================================================================*/

#ifndef MMENU_H
#define MMENU_H

#include "mwindow.h"

/* text metacharacters:
   '&x': highlight 'x'
   '&&' -> '&'
   '%%' -> '%'
   '%-' : separator, if at start of text
   '%c': toggle menu
         data: menu active yes|no
   '%o...|opt0|opt1|...': option menu
         data: active option
   '%d...|label|min|max': int input
         data: current value
   '%s...|label|maxlength|length of inserted text': string input
         data: current value
   '%>': submenu, if at end of text
         data: struct *MMENU, the sub menu
   else: normal menu
         data: unused
*/
typedef struct {
	char *text;
	void *data;
	char *help;
} MENTRY;

typedef struct MMENU {
	int cur;				/* selected entry */
	int first;				/* first line of menu which is displayed */
	int count;				/* number of menu entries, -1 -> count is determined */
							/* by first NULL entry in entries[].text */
	BOOL key_left;			/* can menu be closed with KEY_LEFT or KEY_ESC? */
	MENTRY *entries;
	void (*handle_select) (struct MMENU *menu);	/* called on menu selection */
	MWINDOW *win;			/* the window for this menu */

	void *data;				/* not used by menu functions */
	int id;					/* not used by menu functions */
} MMENU;

typedef void (*MenuSelectFunc) (MMENU *menu);

void menu_open(MMENU * menu, int x, int y);
void menu_close(MMENU * menu);

#endif /* MMENU_H */

/* ex:set ts=4: */
