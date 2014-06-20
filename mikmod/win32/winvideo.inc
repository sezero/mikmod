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

  $Id: winvideo.inc,v 1.1.1.1 2004/01/16 02:07:45 raph Exp $

  Windows console i/o routines

==============================================================================*/

#include <conio.h>

struct SCREEN {
	WORD act_attr;
	char *changed;
	CHAR_INFO *text;
	SHORT minx,miny,maxx,maxy;
} screen = {A_NORMAL, NULL, NULL, 0, 0, 0, 0};

static HANDLE WINAPI GetConHandle (const TCHAR *name)
{
	SECURITY_ATTRIBUTES sa;

	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	return CreateFile (name, GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					&sa, OPEN_EXISTING,
					(DWORD) 0, (HANDLE) 0);
}

#define GetConOut() GetConHandle(TEXT("CONOUT$"))
#define GetConIn() GetConHandle(TEXT("CONIN$"))

static void console_store (BOOL restore)
{
	HANDLE conOut;
	COORD bufSize, bufOrg;
	SMALL_RECT srSource;
	static int oldx = -1, oldy = -1;
	static CHAR_INFO *screen_content = NULL;

	if (!restore && (oldx!=winx || oldy!=winy)) {
		screen_content = (CHAR_INFO *) realloc(screen_content, sizeof(CHAR_INFO)*winx*winy);
		oldx = winx;
		oldy = winy;
	}
	if (!screen_content) return;

	conOut = GetConOut();
	if (conOut == INVALID_HANDLE_VALUE) return;

	srSource.Left = screen.minx;
	srSource.Top = screen.miny;
	srSource.Right = screen.maxx;
	srSource.Bottom = screen.maxy;
	bufSize.X = srSource.Right - srSource.Left + 1;
	bufSize.Y = srSource.Bottom - srSource.Top + 1;
	bufOrg.X = 0;
	bufOrg.Y = 0;

	if (restore)
		WriteConsoleOutput (conOut, screen_content, bufSize, bufOrg, &srSource);
	else
		ReadConsoleOutput (conOut, screen_content, bufSize, bufOrg, &srSource);

	CloseHandle (conOut);
}

static void console_get_size (SHORT *minx, SHORT *miny, SHORT *maxx, SHORT *maxy)
{
	HANDLE conOut;
	CONSOLE_SCREEN_BUFFER_INFO bufInfo;

	conOut = GetConOut();
	if (conOut != INVALID_HANDLE_VALUE) {
		if (GetConsoleScreenBufferInfo (conOut, &bufInfo)) {
			*minx = bufInfo.srWindow.Left;
			*miny = bufInfo.srWindow.Top;
			*maxx = bufInfo.srWindow.Right;
			*maxy = bufInfo.srWindow.Bottom;
		}
		CloseHandle (conOut);
	}
}

static void screen_alloc (void)
{
	int oldx = screen.maxx-screen.minx+1;
	int oldy = screen.maxy-screen.miny+1;
	int x, y;

	console_get_size (&screen.minx, &screen.miny, &screen.maxx, &screen.maxy);
	winx = screen.maxx-screen.minx+1;
	winy = screen.maxy-screen.miny+1;

	screen.changed = (char *) realloc (screen.changed, winx*winy);
	screen.text = (CHAR_INFO *) realloc(screen.text, sizeof(CHAR_INFO)*winx*winy);

	for (y=0; y<oldy && y<winy; y++) {
		for (x=oldx; x<winx; x++) {
			screen.changed[y*winx+x] = 0;
			screen.text[y*winx+x].Attributes = screen.act_attr;
			screen.text[y*winx+x].Char.AsciiChar = ' ';
		}
	}
	for (y=oldy; y<winy; y++) {
		for (x=0; x<winx; x++) {
			screen.changed[y*winx+x] = 0;
			screen.text[y*winx+x].Attributes = screen.act_attr;
			screen.text[y*winx+x].Char.AsciiChar = ' ';
		}
	}
}

void clear(void)
{
	int i;

	memset (screen.changed, 1, winx*winy);
	for (i=0; i<winx*winy; i++) {
		screen.text[i].Attributes = screen.act_attr;
		screen.text[i].Char.AsciiChar = ' ';
	}
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
		if (screen.act_attr != screen.text[d].Attributes || *str != screen.text[d].Char.AsciiChar) {
			screen.changed[d] = 1;
			screen.text[d].Attributes = screen.act_attr;
			screen.text[d].Char.AsciiChar = *str;
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

void gotoxy (int x, int y)
{
	HANDLE conOut;
	COORD coord;

	conOut = GetConOut();
	if (conOut != INVALID_HANDLE_VALUE) {
		coord.X = screen.minx + x;
		coord.Y = screen.miny + y;
		SetConsoleCursorPosition (conOut, coord);

		CloseHandle (conOut);
	}
}

void win_cursor_set(BOOL visible)
{
	HANDLE conOut;
	CONSOLE_CURSOR_INFO cci;

	conOut = GetConOut();
	if (conOut != INVALID_HANDLE_VALUE) {
		GetConsoleCursorInfo (conOut, &cci);
		cci.bVisible = visible;
		SetConsoleCursorInfo (conOut, &cci);
		CloseHandle (conOut);
	}
}

void win_refresh(void)
{
	int x, y, d, start;
	HANDLE conOut;
	COORD bufSize, bufOrg;
	SMALL_RECT srDest;

	conOut = GetConOut();
	if (conOut == INVALID_HANDLE_VALUE) return;

	for (y=0; y<winy; y++) {
		x = 0;
		do {
			d = y*winx+x-1;
			start = x;
			while (start<winx && !screen.changed[++d]) start++;
			if (start>=winx) break;

			d--;
			x = start;
			while (screen.changed[++d] && x<winx) {
				screen.changed[d] = 0;
				x++;
			}
			srDest.Left = screen.minx + start;
			srDest.Top = screen.miny + y;
			srDest.Right = screen.minx + x-1;
			srDest.Bottom = screen.miny + y;

			bufSize.X = srDest.Right - srDest.Left + 1;
			bufSize.Y = srDest.Bottom - srDest.Top + 1;
			bufOrg.X = 0;
			bufOrg.Y = 0;

			WriteConsoleOutput (conOut, &screen.text[d-(x-start)], bufSize, bufOrg, &srDest);
		} while (x<winx);
	}
	CloseHandle (conOut);
}

/* system dependant window init function */
void win_init_system(void)
{
	char *term = getenv("TERM");
	HANDLE conIn;
	DWORD mode;

	/* If we have a mono term lover, give him what he expects */
	use_colors = !(term && strstr(term, "mono"));

	if (win_quiet) return;

	conIn = GetConIn();
	if (conIn != INVALID_HANDLE_VALUE) {
		if (GetConsoleMode(conIn, &mode)) {
			mode = mode & ENABLE_WINDOW_INPUT;
			SetConsoleMode (conIn, mode);
		}
		CloseHandle (conIn);
	}
	screen_alloc();
	console_store(0);
	clear();
	gotoxy (winx-1,winy-1);
	win_cursor_set(0);
	curses_on = 1;
}

/* clean up(e.g. exit curses) */
void win_exit(void)
{
	HANDLE conOut;

	if (win_quiet || !curses_on)
		return;

	console_store(1);
	win_cursor_set(1);
	memset(storage,' ',winx);
	storage[winx] = '\0';
	memcpy(storage, mikversion, strlen(mikversion));

	gotoxy (0, winy-2);
	conOut = GetConOut();
	if (conOut != INVALID_HANDLE_VALUE)
		SetConsoleTextAttribute (conOut, (WORD) A_REVERSE);
#ifdef __WATCOMC__
	 cputs (storage);
#else
	_cputs (storage);
#endif
	if (conOut != INVALID_HANDLE_VALUE) {
		SetConsoleTextAttribute (conOut, (WORD) A_NORMAL);
		CloseHandle (conOut);
	}
	curses_on = 0;
}

static void resize_window (void)
{
	int oldx = winx, oldy = winy;

	screen_alloc();
	win_do_resize (winx-oldx, winy-oldy, 1);
}

static int win_getch(void)
{
	HANDLE conIn;
	DWORD nevents, nread;
	INPUT_RECORD input;
	int ch = 0;

	conIn = GetConIn();
	if (conIn == INVALID_HANDLE_VALUE) return 0;

	GetNumberOfConsoleInputEvents (conIn, &nevents);
	while (!ch && nevents > 0) {
		ReadConsoleInputA (conIn, &input, 1, &nread);

		if (input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
			KEY_EVENT_RECORD *key = &input.Event.KeyEvent;

			if (key->uChar.AsciiChar > 0) {
				ch = key->uChar.AsciiChar;
			} else {
				DWORD control = key->dwControlKeyState & ~CAPSLOCK_ON;

				ch = 0x100 | key->wVirtualScanCode;
				if (control == SHIFT_PRESSED && (ch >= KEY_F(1) && ch <= KEY_F(10))) {
					ch = KEY_SF(1) + ch - KEY_F(1);
				} else if ((control & ENHANCED_KEY) || control == 0) {
					if (key->wVirtualScanCode == 83)
						ch = KEY_DC;
				} else
					ch = 0;
			}
		} else if (input.EventType == WINDOW_BUFFER_SIZE_EVENT) {
			/* new size: input.Event.WindowBufferSizeEvent.dwSize.{X|Y} */
			resize_window();
		}
		GetNumberOfConsoleInputEvents (conIn, &nevents);
	}
	CloseHandle (conIn);

	return ch;
}
