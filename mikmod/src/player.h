/*  MikMod module player
	(c) 1998 - 2000 Miodrag Vallat and others - see file AUTHORS for
	(c) 2004, Raphael Assenat
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

  $Id: player.h,v 1.3 2004/01/29 02:48:06 raph Exp $

  Module player which uses the MikMod library as the player engine.

==============================================================================*/

#ifndef PLAYER_H
#define PLAYER_H

/*========== Messages */

#define playerversion "3.2.8"

#define mikversion "-= MikMod " playerversion " =-"

#define mikcopyr mikversion \
"\n(c) 2004 Raphael Assenat and others - see file AUTHORS for complete list"

#define mikbanner mikcopyr "\n\n" \
" - MikMod authors and contributors are:\n"                                    \
"   Jean-Philippe Ajirent - Peter Amstutz - Raphael Assenat - Anders Bjoerklund\n"\
"   Dimitri Boldyrev - Peter Breitling - Arne de Bruijn - Douglas Carmichael\n"\
"   Chris Conn - Arnout Cosman - Shlomi Fish - Paul Fisher - Tobias Gloth\n"   \
"   Roine Gustaffson - Bjornar Henden - Simon Hosie - Stephan Kanthak\n"       \
"   Alexander Kerkhove - ``Kodiak'' - Mario Koeppen - Mike Leibow\n"           \
"   Andy Lo A Foe - Frank Loemker - Sylvain Marchand - Claudio Matsuoka\n"     \
"   Jeremy McDonald - Steve McIntyre - Brian McKinney - Samuel A Megens\n"     \
"   ``MenTaLguY'' - Jean-Paul Mikkers - Thomas Neumann - C Ray C\n"            \
"   Steffen Rusitschka - Ozkan Sezer - Jake Stine - Stefan Tibus - Tinic Urou\n"\
"   Miodrag Vallat - Kev Vance - Lutz Vieweg - Vince Vu\n"                     \
"   Valtteri Vuorikoski - Andrew Zabolotny\n"                                  \
"\n"                                                                           \
" - This program is free software covered by the GNU General Public License\n" \
"   and comes with ABSOLUTELY NO WARRANTY.\n"                                  \
"\nType 'mikmod -h' for command line options!\n"

#define pausebanner \
"'||''|.    |   '||'  '|' .|'''.| '||''''| '||''|.  \n" \
" ||   ||  |||   ||    |  ||..  '  ||  .    ||   || \n" \
" ||...|' |  ||  ||    |   ''|||.  ||''|    ||    ||\n" \
" ||     .''''|. ||    | .     '|| ||       ||    ||\n" \
".||.   .|.  .||. '|..'  |'....|' .||.....|.||...|' \n"

#define extractbanner \
"'||''''|          .                         .   ||               \n" \
" ||  .   ... ....||. ... ..  ....    .... .||. ... .. ...   ... .\n" \
" ||''|    '|..'  ||   ||' '''' .|| .|   '' ||   ||  ||  || || || \n" \
" ||        .|.   ||   ||    .|' || ||      ||   ||  ||  ||  |''  \n" \
".||.....|.|  ||. '|.'.||.   '|..'|' '|...' '|.'.||..||. ||.'||||.\n" \
"                                                          .|....'\n"
#define loadbanner \
"'||'                          '||   ||                 \n" \
" ||         ...    ....     .. ||  ...  .. ...    ... .\n" \
" ||       .|  '|. '' .||  .'  '||   ||   ||  ||  || || \n" \
" ||       ||   || .|' ||  |.   ||   ||   ||  ||   |''  \n" \
".||.....|  '|..|' '|..'|' '|..'||. .||. .||. || .'||||.\n" \
"                                                .|....'\n"

/*========== Player control */

void Player_SetNextMod(int pos);

#endif

/* ex:set ts=4: */
