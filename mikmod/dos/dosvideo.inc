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

  $Id: dosvideo.inc,v 1.1.1.1 2004/01/16 02:07:29 raph Exp $

  DOS/DJGPP console i/o routines

==============================================================================*/

#include <conio.h>

static struct text_info screen_info;
static unsigned char *screen_contents;
static int cursor_old = 0;

struct SCREEN {
	int act_attr;
	char *changed;
	char *attrs;
	char *text;
} screen = {A_NORMAL, NULL, NULL, NULL};

void clear(void)
{
	memset (screen.changed, 1,               winx*winy);
	memset (screen.attrs,   screen.act_attr, winx*winy);
	memset (screen.text,    ' ',             winx*winy);
}

int attrset(int attrs)
{
	screen.act_attr = attrs;
	return 1;
}

void mvaddnstr(int y,int x,const char *str,int len)
{
	int i, d;

	if (y<0 || y>=winy) return;
	if (x<0) {
		str -= x;
		len += x;
		x = 0;
	}

	d = y*winx+x;
	for (i=0; i<len && i+x < winx; i++) {
		if (screen.act_attr != screen.attrs[d] || *str != screen.text[d]) {
			screen.changed[d] = 1;
			screen.attrs[d] = screen.act_attr;
			screen.text[d] = *str;
		}
		str++;
		d++;
	}
}

void mvaddch(int y,int x,char ch)
{
	char str[1];
	str[0]=ch;
	mvaddnstr (y,x,str,1);
}

/* clear to end of line on uppermost window */
void win_clrtoeol(MWINDOW *win, int x,int y)
{
	int len = win->width - x;

	if (len > 0) {
		memset(storage, ' ', len);
		mvaddnstr(win->y + y, win->x + x, storage, len);
	}
}

void win_cursor_set(BOOL visible)
{
	_setcursortype(visible ? cursor_old : _NOCURSOR);
}

void win_refresh(void)
{
	int x, y, d, start, pos;
	char buffer[STORAGELEN * 2];

	for (y=0; y<winy; y++) {
		x = 0;
		do {
			d = y*winx+x-1;
			start = x;
			while (start<winx && !screen.changed[++d]) start++;
			if (start>=winx) break;

			d--;
			x = start;
			pos = 0;
			while (x<winx && screen.changed[++d]) {
				screen.changed[d] = 0;
				buffer[pos++] = screen.text[d];
				buffer[pos++] = screen.attrs[d];
				x++;
			}

			textattr(buffer[(x-start) * 2 - 1]);
			puttext(start + 1, y + 1, x, y + 1, buffer);
		} while (x<winx);
	}
}

static void screen_alloc (void)
{
	screen.changed = (char *) malloc(winx*winy);
	screen.attrs = (char *) malloc(winx*winy);
	screen.text = (char *) malloc(winx*winy);
	clear();
}

/* system dependant window init function */
void win_init_system(void)
{
	char *term = getenv("TERM");

	/* If we have a mono term lover, give him what he expects */
	use_colors = !(term && strstr(term, "mono"));

	if (win_quiet) return;

	gettextinfo(&screen_info);
	winx = screen_info.screenwidth;
	winy = screen_info.screenheight;
	screen_contents = (unsigned char *) malloc(winx * winy * 2);
	gettext(1, 1, winx , winy , screen_contents);

	screen_alloc();
	cursor_old = _NORMALCURSOR;
	win_cursor_set(0);
	curses_on = 1;
}

/* clean up(e.g. exit curses) */
void win_exit(void)
{
	if (win_quiet || !curses_on)
		return;

	puttext(1, 1, screen_info.screenwidth, screen_info.screenheight,
			screen_contents);
	free(screen_contents);
	win_cursor_set(1);
	memset(storage,' ',winx);
	storage[winx] = '\0';
	memcpy(storage, mikversion, strlen(mikversion));
	textattr(A_REVERSE);
	gotoxy (1,winy-1);
	cputs (storage);
	curses_on = 0;
}

static int win_getch(void)
{
	int c = 0;
	if (kbhit()) {
		c = getch();
		if ((!c) || (c == 0xe0))
			c = 0x100 | getch();
	}
	return c;
}

