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

  $Id: display.h,v 1.1.1.1 2004/01/16 02:07:35 raph Exp $

  Common display definitions, curses-related

==============================================================================*/

#ifndef DISPLAY_H
#define DISPLAY_H

/*========== Core definitions */

/* maximum screen width we handle */
#define MAXWIDTH			200

/*========== Panel definitions */

#define DISPLAY_ROOT    	0
#define DISPLAY_HELP    	1
#define DISPLAY_SAMPLE  	2
#define DISPLAY_INST    	3
#define DISPLAY_MESSAGE 	4
#define DISPLAY_LIST    	5
#define DISPLAY_CONFIG  	6
#if LIBMIKMOD_VERSION >= 0x030200
#define DISPLAY_VOLBARS 	7

#define DISPLAY_COUNT   	8
#else
#define DISPLAY_COUNT   	7
#endif

/*========== Routines */

typedef enum {
	COM_NONE,
	MENU_ACTIVATE
} COMMAND;

void display_message(char *str);

void display_status(void);
int display_header(void);

void display_start(void);

void display_extractbanner(void);
void display_loadbanner(void);
void display_pausebanner(void);

void display_init(void);

#endif /* DISPLAY_H */

/* ex:set ts=4: */
