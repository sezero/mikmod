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

  $Id: keys.h,v 1.1.1.1 2004/01/16 02:07:41 raph Exp $

  Various key definitions

==============================================================================*/

#ifndef KEYS_H
#define KEYS_H

#define CTRL_A  1
#define CTRL_B  2
#define CTRL_D  4
#define CTRL_E  5
#define CTRL_F  6
#define CTRL_K 11
#define CTRL_L 12
#define CTRL_U 21
#define KEY_TAB			('\t')

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)

#define KEY_ESC			27 /* '\e' isn't recognized by some compilers */
#define KEY_UP			(0x100|72)
#define KEY_DOWN		(0x100|80)
#define KEY_LEFT		(0x100|75)
#define KEY_RIGHT		(0x100|77)
#define KEY_NPAGE		(0x100|81)
#define KEY_PPAGE		(0x100|73)
#define KEY_HOME		(0x100|71)
#define KEY_END			(0x100|79)
#define KEY_ENTER		('\n')
#define KEY_DC			(127)
#define KEY_IC			(0x100|82)
#define KEY_BACKSPACE	(8)
#define KEY_F(x)		(0x100|(58+(x)))
#define KEY_SF(x)		(0x100|(83+(x)))

#else

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#elif defined HAVE_CURSES_H
#include <curses.h>
#elif defined HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#endif

#define KEY_ASCII_DEL	127
#define KEY_ASCII_BS	('\b')

#endif

#endif /* ifndef KEYS_H */

/* ex:set ts=4: */
