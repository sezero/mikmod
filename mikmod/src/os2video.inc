/*  MikMod module player
	(c) 1999 Miodrag Vallat and others - see file AUTHORS for
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

  OS/2 console i/o routines

==============================================================================*/

static	HVIO hvio = 0;
static	VIOCURSORINFO viocursorinfo;
static	BYTE clearscreen [2] = { ' ', A_NORMAL };
static	BYTE mvattr = A_NORMAL;

#define attrset(a) mvattr = a

void clear(void)
{
	/* overwrite entire screen with 0s */
	clearscreen [1] = mvattr;
	VioWrtNCell(clearscreen, winy * winx, 0, 0, hvio);
}

void mvaddnstr(int y,int x,const char *str,int len)
{
	char buffer[STORAGELEN];
	int l=strlen(str);

	strncpy(buffer,str,len);
	if (l<len)
		while(l<len) buffer[l++]=' ';
	buffer[len]='\0';

	VioWrtCharStrAtt(buffer,len,y,x,&mvattr,hvio);
}

void mvaddch(int y,int x,char ch)
{
	char text[2];
	text[0]=ch;
	text[1]=0;
	VioWrtCharStrAtt(text,1,y,x,&mvattr,hvio);
}

void win_refresh(void)
{
}

void win_cursor_set(BOOL visible)
{
	if (visible)
		VioSetCurType(&viocursorinfo,hvio);
	else {
		VIOCURSORINFO tmpviocursorinfo;
		tmpviocursorinfo.yStart = 0;
		tmpviocursorinfo.cEnd = 0;
		tmpviocursorinfo.cx = viocursorinfo.cx;
		tmpviocursorinfo.attr = -1; /* hide cursor */
		VioSetCurType(&tmpviocursorinfo, hvio);
	}
}

/* system dependant window init function */
void win_init_system(void)
{
	VIOMODEINFO tmpviomodeinfo;
	char *term = getenv("TERM");

	if (!win_quiet) {
		/* get old cursor */
		VioGetCurType(&viocursorinfo, hvio);
		win_cursor_set(0);
		/* get video mode info */
		tmpviomodeinfo.cb = sizeof(tmpviomodeinfo);
		VioGetMode(&tmpviomodeinfo, hvio);
		winy = tmpviomodeinfo.row;
		winx = tmpviomodeinfo.col;

		clear();
		VioSetCurPos(winy - 1, 0, hvio);

		curses_on = 1;
	}
	/* If we have a mono term lover, give him what he expects */
	use_colors = !(term && strstr(term, "mono"));
}

/* clean up */
void win_exit(void)
{
	if (win_quiet || !curses_on) return;

	clear();
	attrset(A_REVERSE);
	mvaddnstr(winy - 2, 0, mikversion, winx);

	win_cursor_set(1);
	VioSetCurPos(winy - 1, 0, hvio);
	curses_on = 0;
}

/* clear to end of line on uppermost window */
void win_clrtoeol(MWINDOW *win, int x,int y)
{
	if (win->width-x>0) {
		clearscreen[1] = mvattr;
		VioWrtNCell(clearscreen, win->width - x,
					win->y + y, win->x + x, hvio);
	}
}

#ifdef __EMX__
static int _mik_kbhit(void)
{
	KBDKEYINFO k;
	if (KbdPeek(&k, 0))
		return 0;
	return (k.fbStatus & KBDTRF_FINAL_CHAR_IN);
}
#else
#define _mik_kbhit kbhit
#endif

static int win_getch(void)
{
	int c = 0;
	if (_mik_kbhit()) {
		c = getch();
		if ((!c) || (c == 0xe0))
			c = 0x100 | getch();
	}
	return c;
}

