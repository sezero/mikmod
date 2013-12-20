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

  $Id: mconfedit.h,v 1.1.1.1 2004/01/16 02:07:43 raph Exp $

  The config editor

==============================================================================*/

#ifndef MCONFEDIT_H
#define MCONFEDIT_H

#include "mmenu.h"

/* set help text of menu entry
   free old menu->help and malloc new entry */
void set_help(MENTRY * entry, const char *str, ...);

/* open config editor */
void config_open(void);

#endif /* MCONFEDIT_H */

/* ex:set ts=4: */
