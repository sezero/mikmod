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

  $Id: mmenu.c,v 1.1.1.1 2004/01/16 02:07:38 raph Exp $

  Menu functions

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "display.h"
#include "mmenu.h"
#include "mwindow.h"
#include "mdialog.h"
#include "keys.h"
#include "mutilities.h"

static BOOL menu_check (char *text, char ch)
{
	while (*text) {
		if (*text == '%') {
			text++;
			if (*text == ch)
				return 1;
			else if (*text != '%')
				return 0;
		}
		text++;
	}
	return 0;
}

static BOOL menu_is_sub(MENTRY *entry)
{
	int i = 0, end = strlen(entry->text) - 1;
	if (entry->text[end--] == '>') {
		while (entry->text[end--] == '%') i++;
		return (i%2) != 0;
	}
	return 0;
}

static BOOL menu_is_option(MENTRY *entry)
{
	return menu_check (entry->text,'o');
}

static BOOL menu_is_toggle(MENTRY *entry)
{
	return menu_check (entry->text,'c');
}

static BOOL menu_is_int(MENTRY *entry)
{
	return menu_check (entry->text,'d');
}

static BOOL menu_is_str(MENTRY *entry)
{
	return menu_check (entry->text,'s');
}

static BOOL menu_has_sub(MENTRY *entry)
{
	return menu_is_str(entry) || menu_is_int(entry) ||
	  menu_is_option(entry) || menu_is_sub(entry);
}

static int menu_width (char *txt)
{
	int width = strlen(txt);
	char *help;

	while ((help = strchr(txt, '&'))) {
		txt = help+2;
		width--;
	}
	return width;
}

static char *get_text(MENTRY *entry, int width)
{
	char *text, help[100], sub[100], *start, *pos;
	int i;

	if (entry) {
		if (entry->text[0] == '%' && entry->text[1] == '-')
			text = strdup(&entry->text[1]);
		else
			text = strdup(entry->text);
		if (menu_is_sub(entry)) {
			i = strlen(text);
			text[i-2] = '>';
			text[i-1] = '\0';
		}
		pos = text-1;
		do {
			pos++;
			pos = strchr(pos, '%');
			if (pos) pos++;
		} while (pos && (*pos == '%' || *pos == '>' || *pos == '-'));
		if (pos) {
			if (*pos == 'c')
				sprintf(storage, text,
						(SINTPTR_T)(entry->data) ? 'x' : ' ');
			else if ((*pos == 'o') && (start = strchr(pos, '|'))) {
				char *s_pos = NULL;
				int max = 0;

				strncpy(help, text, start - text);
				help[start - text] = '\0';
				help[pos - text] = 's';

				start++;
				i = (SINTPTR_T)(entry->data);
				pos = start;
				while (start) {
					if (!i)
						s_pos = pos;
					if ((start = strchr(pos, '|'))) {
						if (start - pos > max)
							max = start - pos;
						pos = start + 1;
					}
					i--;
				}
				if (strlen(pos) > max)
					max = strlen(pos);
				if (width>0 && menu_width(help)-2+max > width)
					max = width - menu_width(help)+2;
				i = 0;
				while (s_pos && (*s_pos) && (*s_pos != '|'))
					sub[i++] = *s_pos++;
				while (i < max)
					sub[i++] = ' ';
				sub[max] = '\0';
				sprintf(storage, help, sub);
			} else if ((*pos == 'd' || *pos == 's') &&
					   (start = strchr(pos, '|'))) {
				char *right = strrchr(start, '|') + 1;
				strncpy(help, text, pos - text);
				help[pos - text] = '\0';
				if (*pos == 'd') {
					sprintf(sub, "%d", (int)strlen(right));
					strcat(help, sub);
				} else
					strcat(help, right);
				i = strlen(help);
				strncat(help, pos, start - pos);
				help[i + start - pos] = '\0';
				if (*pos == 'd')
					sprintf(storage, help, (int)(SINTPTR_T)(entry->data));
				else {
					char ch;
					sscanf(right, "%d", &i);
					pos = (char *)(entry->data);
					ch = pos[i];
					pos[i] = '\0';
					sprintf(storage, help, pos);
					pos[i] = ch;
				}
			} else
				sprintf(storage, "%s", text);
		} else
			sprintf(storage, "%s", text);
		free (text);

		/* '...%>' -> '...>' */
		i = strlen(storage);
		if (menu_is_sub(entry)) {
			i--;
			while (i < width)
				storage[i++] = ' ';
			storage[i++] = '>';
		} else {
			while (i < width)
				storage[i++] = ' ';
		}
		storage[i] = '\0';
		return storage;
	}
	return NULL;
}

static void menu_do_repaint(MWINDOW * win, int diff)
{
	MMENU *m = (MMENU *) win->data;
	int height, t, hl_pos;
	char *pos, *txt, hl[2], *help;

	height = win->height;
	if (height > m->count)
		height = m->count;
	m->cur += diff;
	if (m->cur < 0)
		m->cur = m->count - 1;
	else if (m->cur >= m->count)
		m->cur = 0;

	while (m->entries[m->cur].text[0] == '%' &&
		   m->entries[m->cur].text[1] == '-')
		m->cur += diff > 0 ? 1 : -1;

	if (m->cur < m->first)
		m->first = m->cur;
	else if (m->cur >= m->first + height)
		m->first = m->cur - height + 1;

	hl[1] = '\0';
	for (t = m->first; t < m->count && t < (height + m->first); t++) {
		txt = get_text(&m->entries[t], win->width);
		hl_pos = -1;
		help = txt;
		while ((pos = strchr(help, '&'))) {
			help = pos+1;
			if ((*(pos + 1) != '&')) {
				hl_pos = pos - txt;
				hl[0] = *(pos + 1);
			}
			for (++pos; *pos; pos++)
				*(pos - 1) = *pos;
			*(pos - 1) = ' ';
			if (hl_pos >= 0)
				txt[hl_pos] = '\0';
		}
		if (t == m->cur) {
			win_attrset(ATTR_MENU_ACTIVE);
			win_print(win, 0, t - m->first, txt);
			if (hl_pos >= 0) {
				win_attrset(ATTR_MENU_AHOTKEY);
				win_print(win, hl_pos, t - m->first, hl);
				win_attrset(ATTR_MENU_ACTIVE);
				win_print(win, hl_pos + 1, t - m->first, &txt[hl_pos + 1]);
			}
			win_status(m->entries[t].help);
		} else if (m->entries[t].text[0] == '%' &&
				   m->entries[t].text[1] == '-') {
			win_attrset(ATTR_MENU_FRAME);
			win_line (win, 0, t - m->first, win->width-1, t - m->first);
		} else {
			win_attrset(ATTR_MENU_INACTIVE);
			if (hl_pos >= 0) {
				win_print(win, 0, t - m->first, txt);
				win_attrset(ATTR_MENU_IHOTKEY);
				win_print(win, hl_pos, t - m->first, hl);
				win_attrset(ATTR_MENU_INACTIVE);
				win_print(win, hl_pos + 1, t - m->first, &txt[hl_pos + 1]);
			} else
				win_print(win, 0, t - m->first, txt);
		}
	}
}

static BOOL menu_repaint(MWINDOW * win)
{
	menu_do_repaint(win, 0);
	return 1;
}

static void handle_opt_menu(MMENU * menu)
{
	int i;
	MMENU *m = (MMENU *) menu->data;

	m->entries[m->cur].data = (void *)(SINTPTR_T)menu->cur;
	menu_close(menu);
	for (i = 0; i < menu->count; i++)
		free(menu->entries[i].text);
	free(menu->entries);
	free(menu);

	if (m->handle_select)
		m->handle_select(m);
}

static BOOL handle_input_str(WIDGET *w, int button, void *input, void *data)
{
	if (button<=0) {
		MMENU* m = (MMENU*) data;
		strcpy((char*)m->entries[m->cur].data, (char*)input);

		if (m->handle_select)
			m->handle_select(m);
	}
	return 1;
}

static BOOL handle_input_int(WIDGET *w, int button, void *input, void *data)
{
	if (button<=0) {
		MMENU* m = (MMENU*) data;
		m->entries[m->cur].data = (void *)(SINTPTR_T)atoi((char*)input);

		if (m->handle_select)
			m->handle_select(m);
	}
	return 1;
}

static BOOL menu_do_select(MWINDOW * win)
{
	MMENU *m = (MMENU *) win->data;
	MENTRY *entry = &m->entries[m->cur];

	if (menu_is_toggle(entry)) {
		entry->data = (void *)(SINTPTR_T)(!((SINTPTR_T)(entry->data)));
		menu_do_repaint(win, 0);
	} else if (menu_is_option(entry)) {
		char *pos, *start;
		MENTRY *sub;
		MMENU *newmenu = (MMENU *) malloc(sizeof(MMENU));
		int cnt = 1, i;

		start = strchr(entry->text, '|');
		pos = ++start;
		while ((pos = strchr(pos, '|'))) {
			pos++;
			cnt++;
		}
		newmenu->cur = (SINTPTR_T)(entry->data);
		newmenu->first = 0;
		newmenu->count = cnt;
		newmenu->key_left = 1;
		newmenu->entries = (MENTRY *) malloc(sizeof(MENTRY) * cnt);
		newmenu->handle_select = handle_opt_menu;
		newmenu->win = NULL;
		newmenu->data = m;
		sub = newmenu->entries;
		for (i = 0; i < cnt; i++) {
			if (!(pos = strchr(start, '|')))
				pos = &start[strlen(start)];
			sub->text = (char *) malloc(sizeof(char) * (pos - start + 1));
			strncpy(sub->text, start, pos - start);
			sub->text[pos - start] = '\0';
			sub->data = NULL;
			sub->help = entry->help;
			start = pos + 1;
			sub++;
		}
		menu_open(newmenu, win->x + win->width + 1, win->y + m->cur - m->first);
		return 1;
	} else if (menu_is_str(entry)) {
		char *msg = NULL, *start, *pos;
		int length = 0;

		start = strchr(entry->text, '|') + 1;
		pos = strchr(start, '|');

		msg = (char *) malloc(sizeof(char) * (pos - start + 1));
		strncpy(msg, start, pos - start);
		msg[pos - start] = '\0';
		sscanf(pos + 1, "%d", &length);

		dlg_input_str(msg, "<&Ok>|&Cancel", (char *)(entry->data), length,
					  handle_input_str, m);
		free(msg);
		return 1;
	} else if (menu_is_int(entry)) {
		const char *start, *pos;
		char *msg = NULL;
		int min = 0, max = 0;

		start = strchr(entry->text, '|') + 1;
		pos = strchr(start, '|');

		msg = (char *) malloc(sizeof(char) * (pos - start + 1));
		strncpy(msg, start, pos - start);
		msg[pos - start] = '\0';
		sscanf(pos + 1, "%d|%d", &min, &max);

		dlg_input_int(msg, "<&Ok>|&Cancel", (SINTPTR_T)(entry->data), min, max,
					  handle_input_int, m);
		free(msg);
		return 1;
	} else if (menu_is_sub(entry)) {
		MMENU *sub = (MMENU *) entry->data;

		sub->cur = sub->first = 0;
		menu_open(sub, win->x + win->width + 1, win->y + m->cur - m->first);
		return 1;
	}
	return 0;
}

void menu_close(MMENU * menu)
{
	int i;

	for (i = 0; i < menu->count; i++)
		if (menu_is_sub(&menu->entries[i]))
			menu_close((MMENU *) menu->entries[i].data);
	if (menu->win) {
		win_status(NULL);
		win_close(menu->win);
		menu->win = NULL;
	}
}

static BOOL menu_handle_key(MWINDOW * win, int ch)
{
	MMENU *menu = (MMENU *) win->data;
	const char *pos, *help;
	int i, key;

	if ((ch < 256) && (isalpha(ch)))
		ch = toupper(ch);
	switch (ch) {
	  case KEY_DOWN:
		menu_do_repaint(win, 1);
		break;
	  case KEY_UP:
		menu_do_repaint(win, -1);
		break;
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
	  case KEY_ESC:
#endif
	  case KEY_LEFT:
		if (menu->key_left)
			menu_close(menu);
		break;
	  case KEY_RIGHT:
		if (menu_has_sub(&menu->entries[menu->cur]))
			menu_do_select(win);
		break;
	  case KEY_ENTER:
	  case '\r':
		  if (!menu_do_select(win))
			  if (menu->handle_select)
				  menu->handle_select(menu);
		break;
	  default:
		for (i = 0; i < menu->count; i++) {
			key = 0;
			help = menu->entries[i].text;
			while ((pos = strchr(help, '&'))) {
				help = pos+2;
				if (*(pos+1) != '&') {
					key = toupper((int)(*(pos + 1)));
					break;
				}
			}
			if (key == ch) {
				menu_do_repaint(win, i - menu->cur);
				if (!menu_do_select(win))
					if (menu->handle_select)
						menu->handle_select(menu);
				return 1;
			}
		}
		return 0;
	}
	return 1;
}

static void menu_handle_resize(MWINDOW * win, int dx, int dy)
{
	int m_y, m_width, m_height;
	MMENU *menu = (MMENU *) win->data;

	win_get_size_max(&m_y, &m_width, &m_height);
	m_width -= 2;
	m_height -= 2;

	if (win->x + win->width > m_width) {
		win->x = m_width - win->width + 1;
		if (win->x < 1)
			win->x = 1;
	}
	if (win->y + win->height - m_y > m_height ||
		win->y + menu->count - m_y > m_height) {
		win->y = m_height - menu->count + m_y + 1;
		if (win->y <= m_y)
			win->y = m_y + 1;
	}
	if (win->height < menu->count)
		win->height = menu->count;
	if (win->height > m_height)
		win->height = m_height;
	if (menu->first + win->height > menu->count)
		menu->first = menu->count - win->height;
	if (menu->first < 0)
		menu->first = 0;
}

void menu_open(MMENU * menu, int x, int y)
{
	MWINDOW *win;
	char *entry;
	int m_y, m_width, m_height, width = 0;

	if (menu->count < 0) {
		menu->count = 0;
		while (menu->entries[menu->count].text)
			menu->count++;
	}
	/* get max. width of entries */
	for (m_y = 0; m_y < menu->count; m_y++) {
		entry = get_text(&menu->entries[m_y], 0);
		m_width = menu_width(entry);
		if (m_width > width)
			width = m_width;
	}

	win_get_size_max(&m_y, &m_width, &m_height);
	m_width -= 2;
	m_height -= 2;

	if (x + width - 1 > m_width)
		x = m_width - width + 1;
	if (x < 1)
		x = 1;
	if (y + menu->count - m_y - 1 > m_height)
		y = m_height - menu->count + m_y + 1;
	if (y < m_y)
		y = m_y + 1;

	menu->win = win_open(x, y, width, menu->count, 1, NULL, ATTR_MENU_FRAME);
	win_set_repaint(menu_repaint);
	win_set_handle_key(menu_handle_key);
	win_set_resize(0, menu_handle_resize);
	win_set_data((void *)menu);

	win = win_get_window();
	if (menu->first + win->height > menu->count)
		menu->first = menu->count - win->height;
	if (menu->first < 0)
		menu->first = 0;
	menu_repaint(win);
}

/* ex:set ts=4: */
