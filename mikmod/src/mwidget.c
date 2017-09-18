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

  $Id: mwidget.c,v 1.1.1.1 2004/01/16 02:07:33 raph Exp $

  Widget and Dialog creation functions

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "display.h"
#include "player.h"
#include "mwindow.h"
#include "mwidget.h"
#include "keys.h"
#include "mutilities.h"

#define STR_WIDTH_MAX 70
#define STR_WIDTH_MIN 20
#define INT_WIDTH_MAX 11

#define LIST_WIDTH_DEFAULT 60
#define LIST_WIDTH_MIN 15
#define LIST_HEIGHT_DEFAULT 20
#define LIST_HEIGHT_MIN 5

#define WWIN(w) ((w)->w.d->win)

static ATTRS base_attr (DIALOG *d, ATTRS attrs)
{
	if (d->attrs >= 0) return d->attrs;
	return attrs;
}

static void label_free(WID_LABEL *w)
{
	free(w->msg);
	free(w);
}

static void label_paint(WID_LABEL *w)
{
	char *start, *pos;
	int y = w->w.y;

	win_attrset(base_attr(w->w.d,ATTR_DLG_LABEL));
	start = w->msg;
	for (pos = w->msg; *pos; pos++) {
		if (*pos == '\n') {
			*pos = '\0';
			win_print(WWIN(w),w->w.x, y, start);
			*pos = '\n';
			start = pos + 1;
			y++;
		}
	}
	win_print(WWIN(w),w->w.x, y, start);
}

static int label_handle_event(WID_LABEL *w, WID_EVENT event, int ch)
{
	return 0;
}

static void label_get_size(WID_LABEL *w, int *width, int *height)
{
	char *pos;
	int x = 0;

	*width = 0;
	*height = 0;
	for (pos = w->msg; *pos; pos++) {
		if (*pos == '\n') {
			(*height)++;
			if (x > *width)
				*width = x;
			x = -1;
		}
		x++;
	}
	if (x > *width)
		*width = x;
	(*height)++;
}

static void str_free(WID_STR *w)
{
	free(w->input);
	free(w);
}

static void str_paint(WID_STR *w)
{
	char hl[2] = " ", ch = ' ', *pos = &w->input[w->start];
	int dx = 0, len;

	win_attrset(ATTR_DLG_STR_TEXT);
	if (w->w.has_focus) {
		hl[0] = ch = w->input[w->cur_pos];
		if (!hl[0])
			hl[0] = ' ';
		w->input[w->cur_pos] = '\0';
		if (*pos)
			win_print(WWIN(w),w->w.x, w->w.y, pos);
		dx = strlen(pos);
		win_attrset(ATTR_DLG_STR_CURSOR);
		win_print(WWIN(w),w->w.x + dx, w->w.y, hl);
		win_attrset(ATTR_DLG_STR_TEXT);
		pos += dx;
		dx++;
		*pos = ch;
		if (*pos)
			pos++;
	}
	len = strlen(pos);
	if (len + dx > w->w.width) {
		ch = w->input[w->w.width + w->start];
		w->input[w->w.width + w->start] = '\0';
	}
	win_print(WWIN(w),w->w.x + dx, w->w.y, pos);
	if (len + dx > w->w.width)
		w->input[w->w.width + w->start] = ch;
	else if (len + dx < w->w.width) {
		dx += len;
		for (len = 0; len < w->w.width - dx; len++)
			storage[len] = ' ';
		storage[len] = '\0';
		win_print(WWIN(w),w->w.x + dx, w->w.y, storage);
	}
}

static int handle_focus(WIDGET *w, int ret, int from_activate)
{
	if (ret && (ret != EVENT_HANDLED) && w->handle_focus) {
		return w->handle_focus((WIDGET *) w, ret);
	} else {
		if (ret == FOCUS_ACTIVATE) {
			ret = from_activate;
			if (ret == EVENT_HANDLED)
				dialog_close(w->d);
		}
		return ret;
	}
}

static int input_handle_event(WID_STR *w, WID_EVENT event,
							  int ch, BOOL int_input)
{
	char *pos;
	int i;

	if (event == WID_HOTKEY)
		return 0;
	if (event == WID_GET_FOCUS)
		return EVENT_HANDLED;
	if ((event == WID_KEY) && w->w.handle_key) {
		i = w->w.handle_key((WIDGET *) w, ch);
		if (i)
			return i;
	}

	switch (ch) {
	  case KEY_UP:
		return handle_focus((WIDGET*)w, FOCUS_PREV, 0);
	  case KEY_TAB:
	  case KEY_DOWN:
		return handle_focus((WIDGET*)w, FOCUS_NEXT, 0);
	  case KEY_LEFT:
	  case CTRL_B:
		if (w->cur_pos > 0)
			w->cur_pos--;
		break;
	  case KEY_RIGHT:
	  case CTRL_F:
		if (w->cur_pos < strlen(w->input))
			w->cur_pos++;
		break;
	  case KEY_HOME:
	  case KEY_PPAGE:
	  case CTRL_A:
		w->cur_pos = 0;
		break;
#ifdef KEY_END
	  case KEY_END:
#endif
	  case KEY_NPAGE:
	  case CTRL_E:
		w->cur_pos = strlen(w->input);
		break;
	  case CTRL_K:
		w->input[w->cur_pos] = '\0';
		break;
	  case CTRL_U:
		w->cur_pos = 0;
		w->input[w->cur_pos] = '\0';
		break;
	  case KEY_DC:
	  case CTRL_D:
#ifdef KEY_ASCII_DEL
	  case KEY_ASCII_DEL:
#endif
		if (w->cur_pos < strlen(w->input))
			for (pos = &w->input[w->cur_pos]; *pos; pos++)
				*pos = *(pos + 1);
		break;
	  case KEY_BACKSPACE:
#ifdef KEY_ASCII_BS
	  case KEY_ASCII_BS:
#endif
		if (w->cur_pos > 0) {
			for (pos = &w->input[w->cur_pos - 1]; *pos; pos++)
				*pos = *(pos + 1);
			w->cur_pos--;
		}
		break;
	  case KEY_ENTER:
	  case '\r':
		return handle_focus((WIDGET*)w, FOCUS_ACTIVATE, FOCUS_NEXT);
	  default:
		if (ch >= 256 || ch < ' ')
			return 0;

		if ((int_input && isdigit(ch)) || !int_input) {
			i = strlen(w->input);
			if (i < w->length) {
				for (; i >= w->cur_pos; i--)
					w->input[i + 1] = w->input[i];
				w->input[w->cur_pos] = ch;
				w->cur_pos++;
			}
		}
	}
	if (w->cur_pos < w->start)
		w->start = w->cur_pos;
	if (w->cur_pos >= w->start + w->w.width)
		w->start = w->cur_pos - w->w.width + 1;
	str_paint(w);
	return EVENT_HANDLED;
}

static int str_handle_event(WID_STR *w, WID_EVENT event, int ch)
{
	return input_handle_event(w, event, ch, 0);
}

static void str_get_size(WID_STR *w, int *width, int *height)
{
	if (*width > w->w.def_width)
		*width = w->w.def_width;
	if (*width > w->length)
		*width = w->length + 1;
	if (*width < STR_WIDTH_MIN)
		*width = STR_WIDTH_MIN;

	w->start = w->cur_pos - *width + 1;
	if (w->start < 0)
		w->start = 0;

	*height = 1;
}

static void int_free(WID_INT *w)
{
	free(w->input);
	free(w);
}

static void int_paint(WID_INT *w)
{
	str_paint((WID_STR *) w);
}

static BOOL int_handle_event(WID_INT *w, WID_EVENT event, int ch)
{
	return input_handle_event((WID_STR *) w, event, ch, 1);
}

static void int_get_size(WID_INT *w, int *width, int *height)
{
	*width = w->w.def_width;
	*height = 1;
}

static void button_free(WID_BUTTON *w)
{
	free(w->button);
	free(w);
}

static void button_paint(WID_BUTTON *w)
{
	int cur, x, cnt_hl = 0;
	char *pos, *hl_pos, *start, hl[2];
	BOOL end;

	for (pos = w->button; *pos; pos++)
		if (*pos == '&')
			cnt_hl++;
	x = (w->w.d->win->width - 1 - w->w.x -
			((int)strlen(w->button) + 5 * w->cnt - 1 - cnt_hl)) / 2;
	cur = 0;
	hl_pos = NULL;
	hl[1] = '\0';
	start = w->button;
	end = 0;
	for (pos = w->button; !end; pos++) {
		end = !(*pos);
		if ((*pos == '|') || (*pos == '\0')) {
			*pos = '\0';
			if ((w->active != cur) || (!w->w.has_focus))
				win_attrset(ATTR_DLG_BUT_INACTIVE);
			else
				win_attrset(ATTR_DLG_BUT_ACTIVE);
			win_print(WWIN(w),w->w.x + x, w->w.y, "[ ");
			if ((w->active != cur) || (!w->w.has_focus))
				win_attrset(ATTR_DLG_BUT_ITEXT);
			else
				win_attrset(ATTR_DLG_BUT_ATEXT);
			win_print(WWIN(w),w->w.x + x + 2, w->w.y, start);
			x += strlen(start) + 2;
			if (hl_pos) {
				if ((w->active != cur) || (!w->w.has_focus))
					win_attrset(ATTR_DLG_BUT_IHOTKEY);
				else
					win_attrset(ATTR_DLG_BUT_AHOTKEY);
				win_print(WWIN(w),w->w.x + x, w->w.y, hl);
				if ((w->active != cur) || (!w->w.has_focus))
					win_attrset(ATTR_DLG_BUT_ITEXT);
				else
					win_attrset(ATTR_DLG_BUT_ATEXT);
				win_print(WWIN(w),w->w.x + x + 1, w->w.y, hl_pos);
				*(hl_pos - 2) = '&';
				x += strlen(hl_pos) + 1;
				hl_pos = NULL;
			}
			if ((w->active != cur) || (!w->w.has_focus))
				win_attrset(ATTR_DLG_BUT_INACTIVE);
			else
				win_attrset(ATTR_DLG_BUT_ACTIVE);
			win_print(WWIN(w),w->w.x + x, w->w.y, " ]");
			x += 4;
			*pos = '|';
			start = pos + 1;
			cur++;
		}
		if (*pos == '&') {
			*pos = '\0';
			pos++;
			hl_pos = pos + 1;
			hl[0] = *pos;
		}
	}
	*(pos-1) = '\0';
}

static BOOL button_handle_event(WID_BUTTON *w, WID_EVENT event, int ch)
{
	int cur;
	char *pos;

	if (event == WID_GET_FOCUS) {
		if (ch < 0)
			w->active = w->cnt - 1;
		else
			w->active = 0;
		return EVENT_HANDLED;
	}
	if ((event == WID_KEY) && (w->w.handle_key)) {
		cur = w->w.handle_key((WIDGET *) w, ch);
		if (cur)
			return cur;
	}
	if ((ch < 256) && (isalpha(ch)))
		ch = toupper(ch);
	switch (ch) {
	  case KEY_UP:
	  case KEY_LEFT:
		if (event == WID_KEY) {
			w->active--;
			if (w->active < 0)
				return handle_focus ((WIDGET*)w, FOCUS_PREV, 0);
			button_paint(w);
		}
		break;
	  case KEY_DOWN:
	  case KEY_RIGHT:
	  case KEY_TAB:
		if (event == WID_KEY) {
			w->active++;
			if (w->active >= w->cnt)
				return handle_focus ((WIDGET*)w, FOCUS_NEXT, 0);
			button_paint(w);
		}
		break;
	  case KEY_ENTER:
	  case '\r':
		if (event == WID_KEY)
			return handle_focus ((WIDGET*)w, FOCUS_ACTIVATE, EVENT_HANDLED);
		break;
	  default:
		cur = 0;
		for (pos = w->button; *pos; pos++) {
			if (*pos == '|')
				cur++;
			if (*pos == '&' && (toupper((int)*(pos+1)) == ch)) {
				w->active = cur;
				button_paint(w);
				return handle_focus ((WIDGET*)w, FOCUS_ACTIVATE, EVENT_HANDLED);
			}
		}
		return 0;
	}
	return EVENT_HANDLED;
}

static void button_get_size(WID_BUTTON *w, int *width, int *height)
{
	char *pos;
	int hl_cnt = 0;

	w->cnt = 1;
	for (pos = w->button; *pos; pos++) {
		if (*pos == '&')
			hl_cnt++;
		if (*pos == '|')
			w->cnt++;
	}
	*width = strlen(w->button) + 5 * w->cnt - 1 - hl_cnt;
	*height = 1;
}

static void list_free(WID_LIST *w)
{
	int i;
	for (i=0; i<w->cnt; i++)
		free (w->entries[i]);
	free (w->entries);
	if (w->title) free (w->title);
	free (w);
}

static void list_paint(WID_LIST *w)
{
	int i,x,visible;
	char ch;

	x = w->w.x+w->w.width-1;
	visible = w->w.height-2;

	win_attrset(base_attr(w->w.d,ATTR_DLG_FRAME));
	win_box (WWIN(w),w->w.x, w->w.y, x, w->w.y+w->w.height-1);
	if (w->title) {
		if (strlen(w->title) > w->w.width-2) {
			ch = w->title[w->w.width-2];
			w->title[w->w.width-2] = '\0';
			win_print (WWIN(w),w->w.x+1, w->w.y, w->title);
			w->title[w->w.width-2] = ch;
		} else
			win_print (WWIN(w),w->w.x+1, w->w.y, w->title);
	}

	if (w->first > 0)
		win_print (WWIN(w),x, w->w.y+1, "^");
	else
		win_print (WWIN(w),x, w->w.y+1, "-");
	if (w->first+visible < w->cnt)
		win_print (WWIN(w),x, w->w.y+w->w.height-2, "v");
	else
		win_print (WWIN(w),x, w->w.y+w->w.height-2, "-");
	if (visible>2) {
		i = 0;
		if (w->cnt > 1)
			i = w->cur*(visible-3)/(w->cnt-1);
		win_print (WWIN(w),x, w->w.y+i+2, "*");
	}

	for (i=w->first; i<visible+w->first; i++) {
		storage[0] = '\0';
		if (i == w->cur) {
			if (w->w.has_focus)
				win_attrset(ATTR_DLG_LIST_FOCUS);
			else
				win_attrset(ATTR_DLG_LIST_NOFOCUS);
		} else
			win_attrset(base_attr(w->w.d,ATTR_DLG_FRAME));
		if (i < w->cnt) {
			strncpy (storage,w->entries[i],w->w.width-2);
			storage[w->w.width-2] = '\0';
		}
		for (x=strlen(storage); x<w->w.width-2; x++)
			storage[x] = ' ';
		storage[w->w.width-2] = '\0';
		win_print (WWIN(w),w->w.x+1, w->w.y+i-w->first+1, storage);
	}
}

static int list_handle_event(WID_LIST *w, WID_EVENT event, int ch)
{
	int i, old_cur;

	if (event == WID_HOTKEY)
		return 0;
	if (event == WID_GET_FOCUS)
		return EVENT_HANDLED;
	if ((event == WID_KEY) && w->w.handle_key) {
		i = w->w.handle_key((WIDGET *) w, ch);
		if (i)
			return i;
	}
	old_cur = w->cur;
	switch (ch) {
		case KEY_UP:
			if (w->cur>0) w->cur--;
			break;
		case KEY_DOWN:
			if (w->cur<w->cnt-1) w->cur++;
			break;
		case KEY_PPAGE:
			w->cur -= w->w.height-3;
			if (w->cur<0) w->cur = 0;
			break;
		case KEY_NPAGE:
			w->cur += w->w.height-3;
			if (w->cur>=w->cnt)
				w->cur = w->cnt>0 ? w->cnt-1 : 0;
			break;
		case KEY_HOME:
			w->cur = 0;
			break;
#ifdef KEY_END
		case KEY_END:
			w->cur = w->cnt-1;
			break;
#endif
		case KEY_LEFT:
			return handle_focus((WIDGET*)w, FOCUS_PREV, 0);
		case KEY_RIGHT:
		case KEY_TAB:
			return handle_focus((WIDGET*)w, FOCUS_NEXT, 0);
		case KEY_ENTER:
		case '\r':
			return handle_focus((WIDGET*)w, FOCUS_ACTIVATE, FOCUS_ACTIVATE);
		default:
			return 0;
	}
	if (w->cur < w->first)
		w->first = w->cur;
	if (w->cur >= w->first + w->w.height-2)
		w->first = w->cur - w->w.height + 3;
	list_paint(w);
	if (w->sel_mode == WID_SEL_BROWSE && old_cur != w->cur)
		return handle_focus((WIDGET*)w, FOCUS_ACTIVATE, FOCUS_ACTIVATE);

	return EVENT_HANDLED;
}

static void list_get_size(WID_LIST *w, int *width, int *height)
{
	if (*width > w->w.def_width)
		*width = w->w.def_width;
	if (*width < LIST_WIDTH_MIN)
		*width = LIST_WIDTH_MIN;

	if (*height > w->w.def_height)
		*height = w->w.def_height;
	if (*height < LIST_HEIGHT_MIN)
		*height = LIST_HEIGHT_MIN;
}

static void check_toggle_paint(WID_CHECK *w, BOOL toggle)
{
	char *start, *pos, *hl_pos, hl[2], end;
	char marker[] = " x", help[STORAGELEN];
	int cur = 0, x, xx;

	hl_pos = NULL;
	hl[1] = '\0';
	if (toggle)
		strcpy (help,"[ ] ");
	else {
		strcpy (help,"( ) ");
		marker[1] = '*';
	}
	help[w->w.width] = '\0';
	start = w->button;

	pos = w->button-1;
	do {
		pos++;
		if ((*pos == '|') || (*pos == '\0')) {
			end = *pos;
			*pos = '\0';
			if ((w->active != cur) || (!w->w.has_focus))
				win_attrset(ATTR_DLG_BUT_ITEXT);
			else
				win_attrset(ATTR_DLG_BUT_ATEXT);
			help[1] = marker[BTST(w->selected,1<<cur)];
			strcpy (&help[4],start);
			x = strlen(start) + 4;
			if (hl_pos) {
				win_print(WWIN(w),w->w.x, w->w.y+cur, help);
				if ((w->active != cur) || (!w->w.has_focus))
					win_attrset(ATTR_DLG_BUT_IHOTKEY);
				else
					win_attrset(ATTR_DLG_BUT_AHOTKEY);
				win_print(WWIN(w),w->w.x + x, w->w.y+cur, hl);
				x++;
				if ((w->active != cur) || (!w->w.has_focus))
					win_attrset(ATTR_DLG_BUT_ITEXT);
				else
					win_attrset(ATTR_DLG_BUT_ATEXT);
				strcpy (&help[x],hl_pos);
				xx = x+strlen(hl_pos);
				if (xx != w->w.width)
					memset (&help[xx],' ',w->w.width-xx);
				win_print(WWIN(w),w->w.x + x, w->w.y+cur, &help[x]);
				*(hl_pos - 2) = '&';
				hl_pos = NULL;
			} else {
				if (x != w->w.width)
					memset (&help[x],' ',w->w.width-x);
				win_print(WWIN(w),w->w.x, w->w.y+cur, help);
			}
			*pos = end;
			start = pos + 1;
			cur++;
		} else if (*pos == '&') {
			*pos = '\0';
			pos++;
			hl_pos = pos + 1;
			hl[0] = *pos;
		}
	} while (*pos);
}

static BOOL check_toggle_handle_event(WID_CHECK *w, WID_EVENT event,
									  int ch, BOOL toggle)
{
	static WID_EVENT last = WID_KEY;
	int cur;
	char *pos;

	if (event == WID_GET_FOCUS) {
		if (last != WID_HOTKEY) {		/* active entry was already set */
			if (ch < 0)
				w->active = w->cnt - 1;
			else
				w->active = 0;
		}
		return EVENT_HANDLED;
	}
	last = event;
	if ((event == WID_KEY) && (w->w.handle_key)) {
		cur = w->w.handle_key((WIDGET *) w, ch);
		if (cur)
			return cur;
	}
	if ((ch < 256) && (isalpha(ch)))
		ch = toupper(ch);
	switch (ch) {
	  case KEY_UP:
	  case KEY_LEFT:
		if (event == WID_KEY) {
			w->active--;
			if (w->active < 0)
				return handle_focus ((WIDGET*)w, FOCUS_PREV, 0);
			check_toggle_paint(w,toggle);
		}
		break;
	  case KEY_DOWN:
	  case KEY_RIGHT:
	  case KEY_TAB:
		if (event == WID_KEY) {
			w->active++;
			if (w->active >= w->cnt)
				return handle_focus ((WIDGET*)w, FOCUS_NEXT, 0);
			check_toggle_paint(w,toggle);
		}
		break;
	  case KEY_ENTER:
	  case '\r':
		if (event == WID_KEY) {
			cur = handle_focus ((WIDGET*)w, FOCUS_ACTIVATE, 0);
			if (cur && cur!=FOCUS_ACTIVATE && cur!=FOCUS_DONT)
				return cur;
			if (toggle)
				w->selected ^= 1<<w->active;
			else
				w->selected = 1<<w->active;
			check_toggle_paint(w,toggle);
		}
		break;
	  default:
		cur = 0;
		for (pos = w->button; *pos; pos++) {
			if (*pos == '|')
				cur++;
			if (*pos == '&' && (toupper((int)*(pos+1)) == ch)) {
				w->active = cur;
				check_toggle_paint(w,toggle);
				cur = handle_focus ((WIDGET*)w, FOCUS_ACTIVATE, FOCUS_ACTIVATE);
				if (cur!=FOCUS_ACTIVATE && cur!=FOCUS_DONT)
					return cur;
				if (toggle)
					w->selected ^= 1<<w->active;
				else
					w->selected = 1<<w->active;
				check_toggle_paint(w,toggle);
				return cur;
			}
		}
		return 0;
	}
	return EVENT_HANDLED;
}

static void check_free(WID_CHECK *w)
{
	free(w->button);
	free(w);
}

static void check_paint(WID_CHECK *w)
{
	check_toggle_paint(w, 0);
}

static BOOL check_handle_event(WID_CHECK *w, WID_EVENT event, int ch)
{
	return check_toggle_handle_event(w, event, ch, 0);
}

static void check_get_size(WID_CHECK *w, int *width, int *height)
{
	char *pos;
	int x = 0, hl_cnt = 0;

	*width = 0;
	*height = 0;
	w->cnt = 0;
	for (pos = w->button; *pos; pos++) {
		if (*pos == '&')
			hl_cnt++;
		if (*pos == '|') {
			w->cnt++;
			(*height)++;
			x += 4 - hl_cnt;
			if (x > *width)
				*width = x;
			hl_cnt = 0;
			x = -1;
		}
		x++;
	}
	w->cnt++;
	(*height)++;
	x += 4 - hl_cnt;
	if (x > *width)
		*width = x;
}

static void toggle_free(WID_TOGGLE *w)
{
	free(w->button);
	free(w);
}

static void toggle_paint(WID_TOGGLE *w)
{
	check_toggle_paint((WID_CHECK *) w, 1);
}

static BOOL toggle_handle_event(WID_TOGGLE *w, WID_EVENT event, int ch)
{
	return check_toggle_handle_event((WID_CHECK *) w, event, ch, 1);
}

static void toggle_get_size(WID_TOGGLE *w, int *width, int *height)
{
	check_get_size((WID_CHECK *) w, width, height);
}

static void colorsel_free(WID_COLORSEL *w)
{
	free(w);
}

static void colorsel_paint(WID_COLORSEL *w)
{
	int y = w->w.y, x = w->w.x;
	int act_x = (w->active & COLOR_BMASK) >> COLOR_BSHIFT;
	int act_y = (w->active & COLOR_FMASK) >> COLOR_FSHIFT;
	ATTRS border[COLOR_CNT+2][COLOR_CNT*3+2], b[12], attr;

	win_attrset(base_attr(w->w.d, ATTR_DLG_FRAME));
	win_box (WWIN(w), w->w.x, w->w.y, w->w.x+COLOR_CNT*3+1, w->w.y+COLOR_CNT+1);

	attr = (win_get_theme_color(ATTR_DLG_FRAME) & COLOR_BMASK) >> COLOR_BSHIFT;
	for (x=0; x<COLOR_CNT*3+2; x++)
		border[0][x] = border[COLOR_CNT+1][x] = attr;
	for (y=1; y<COLOR_CNT+1; y++)
		border[y][0] = border[y][COLOR_CNT*3+1] = attr;

	for (y=0; y<COLOR_CNT; y++) {
		for (x=0; x<COLOR_CNT; x++) {
			border[y+1][x*3+1] = border[y+1][x*3+2] = border[y+1][x*3+3] = x;
			win_set_color ((x << COLOR_BSHIFT) + (y << COLOR_FSHIFT));
			win_print (WWIN(w),w->w.x+x*3+1, w->w.y+y+1, " X ");
		}
	}
	{
		ATTRS hotkey = w->w.has_focus ? ATTR_DLG_BUT_AHOTKEY:ATTR_DLG_BUT_IHOTKEY;
		ATTRS text = w->w.has_focus ? ATTR_DLG_BUT_ATEXT:ATTR_DLG_BUT_ITEXT;
		char key[2] = " ";
		const char *pat[2] = {".......< h      h >",
						"..^h  hv"};
		int p, h = 0;

		for (p=0; p<2; p++) {
			for (x=0; x<strlen(pat[p]); x++) {
				if (pat[p][x] != '.') {
					border[x*p][x*(1-p)] =
						(win_get_theme_color(text) & COLOR_BMASK) >> COLOR_BSHIFT;
					win_attrset (text);
					key[0] = pat[p][x];
					if (pat[p][x] == 'h') {
						if (w->hkeys[h]) {
							border[x*p][x*(1-p)] =
								(win_get_theme_color(hotkey) & COLOR_BMASK) >> COLOR_BSHIFT;
							win_attrset (hotkey);
							key[0] = w->hkeys[h];
						}
						h++;
					}
					win_print (WWIN(w), w->w.x+x*(1-p), w->w.y+x*p, key);
				}
			}
		}
	}

	for (x=0; x<5; x++) {
		b[x] = border[act_y][act_x*3+x];
		b[10-x] = border[act_y+2][act_x*3+x];
	}
	b[5] = border[act_y+1][act_x*3+4];
	b[11] = border[act_y+1][act_x*3];

	win_set_forground (COLOR_BLACK_F);
	win_box_color (WWIN(w),w->w.x+act_x*3, w->w.y+act_y,
				   w->w.x+act_x*3+4, w->w.y+act_y+2, b);
}

static int colorsel_handle_event(WID_COLORSEL *w, WID_EVENT event, int ch)
{
	int act_x = (w->active & COLOR_BMASK) >> COLOR_BSHIFT;
	int act_y = (w->active & COLOR_FMASK) >> COLOR_FSHIFT;
	int i, old_act_x = act_x, old_act_y = act_y;

	if (event == WID_GET_FOCUS)
		return EVENT_HANDLED;
	if (event == WID_KEY && w->w.handle_key) {
		i = w->w.handle_key((WIDGET *) w, ch);
		if (i)
			return i;
	}
	if (ch < 256 && isalpha(ch))
		ch = toupper(ch);
	switch (ch) {
		case KEY_UP:
			if (event == WID_KEY && act_y>0)
				act_y--;
			break;
		case KEY_LEFT:
			if (event == WID_KEY && act_x>0)
				act_x--;
			break;
		case KEY_DOWN:
			if (event == WID_KEY && act_y<COLOR_CNT-1)
				act_y++;
			break;
		case KEY_RIGHT:
			if (event == WID_KEY && act_x<COLOR_CNT-1)
				act_x++;
			break;
		case KEY_ENTER:
		case '\r':
			if (event == WID_KEY)
				return handle_focus ((WIDGET*)w, FOCUS_ACTIVATE, FOCUS_ACTIVATE);
			break;
		case KEY_TAB:
			if (event == WID_KEY)
				return handle_focus ((WIDGET*)w, FOCUS_NEXT, 0);
			break;
		default:
			if (strchr (w->hkeys, ch)) {
				if (ch == w->hkeys[0])
					if (act_x>0) act_x--;
				if (ch == w->hkeys[1])
					if (act_x<COLOR_CNT-1) act_x++;
				if (ch == w->hkeys[2])
					if (act_y>0) act_y--;
				if (ch == w->hkeys[3])
					if (act_y<COLOR_CNT-1) act_y++;
				w->active = (act_x << COLOR_BSHIFT) + (act_y << COLOR_FSHIFT);
				colorsel_paint(w);

				i = handle_focus ((WIDGET*)w, FOCUS_ACTIVATE, FOCUS_ACTIVATE);
				if (i != FOCUS_ACTIVATE && i != FOCUS_DONT)
					return i;

				colorsel_paint(w);
				return i;
			}
			return 0;
	}
	w->active = (act_x << COLOR_BSHIFT) + (act_y << COLOR_FSHIFT);
	colorsel_paint (w);
	if (w->sel_mode == WID_SEL_BROWSE && (old_act_x != act_x || old_act_y != act_y))
		return handle_focus((WIDGET*)w, FOCUS_ACTIVATE, FOCUS_ACTIVATE);

	return EVENT_HANDLED;
}

static void colorsel_get_size(WID_COLORSEL *w, int *width, int *height)
{
	*width = 26;
	*height = 10;
}

static void dialog_add(DIALOG *d, WIDGET *w)
{
	d->widget = (WIDGET **) realloc(d->widget, (d->cnt + 1) * sizeof(WIDGET *));
	d->widget[d->cnt] = w;
	d->cnt++;
}

static void widget_init(WIDGET *w, DIALOG *d, BOOL focus, int spacing)
{
	w->x = w->y = w->width = w->height = 1;
	w->def_width = w->def_height = -1;
	w->spacing = spacing;
	w->can_focus = focus;
	w->has_focus = 0;
	w->d = d;
	w->handle_key = w->handle_focus = NULL;
	w->w_free = w->w_paint = NULL;
	w->w_handle_event = NULL;
	w->w_get_size = NULL;
	w->data = NULL;
}

WIDGET *wid_label_add(DIALOG *d, int spacing, const char *msg)
{
	WID_LABEL *w = (WID_LABEL *) malloc(sizeof(WID_LABEL));

	widget_init((WIDGET *) w, d, 0, spacing);
	w->w.type = TYPE_LABEL;
	w->w.w_free = (freeFunc) label_free;
	w->w.w_paint = (paintFunc) label_paint;
	w->w.w_handle_event = (handleEventFunc) label_handle_event;
	w->w.w_get_size = (getSizeFunc) label_get_size;

	w->msg = strdup(msg);
	dialog_add(d, (WIDGET *) w);
	return (WIDGET *) w;
}

void wid_label_set_label (WID_LABEL *w, const char *label)
{
	if (w->msg) free (w->msg);
	w->msg = strdup (label);
}

WIDGET *wid_str_add(DIALOG *d, int spacing, const char *input, int length)
{
	int i;
	WID_STR *w = (WID_STR *) malloc(sizeof(WID_STR));

	widget_init((WIDGET *) w, d, 1, spacing);
	w->w.type = TYPE_STR;
	w->w.w_free = (freeFunc) str_free;
	w->w.w_paint = (paintFunc) str_paint;
	w->w.w_handle_event = (handleEventFunc) str_handle_event;
	w->w.w_get_size = (getSizeFunc) str_get_size;

	w->length = length;
	w->w.def_width = STR_WIDTH_MAX;

	w->input = (char *) malloc(length + 1);

	i = MIN(strlen(input), length);
	strncpy(w->input, input, i);
	w->input[i] = '\0';
	w->cur_pos = strlen(w->input);

	dialog_add(d, (WIDGET *) w);
	return (WIDGET *) w;
}

void wid_str_set_input (WID_STR *w, const char *input, int length)
{
	if (length>=0) {
		if (w->input) free (w->input);
		if (length) w->input = (char *) malloc(length + 1);
		w->length = length;
	}
	if (w->length == 0) {
		w->input = NULL;
		w->cur_pos = w->start = 0;
	} else {
		int i = MIN (strlen(input), w->length);
		strncpy (w->input, input, i);
		w->input[i] = '\0';
		if (w->cur_pos > strlen(w->input))
			w->cur_pos = strlen(w->input);

		if (w->cur_pos < w->start)
			w->start = w->cur_pos;
		if (w->cur_pos >= w->start + w->w.width)
			w->start = w->cur_pos - w->w.width + 1;
	}
}

WIDGET *wid_int_add(DIALOG *d, int spacing, int value, int length)
{
	WID_INT *w = (WID_INT *) malloc(sizeof(WID_INT));

	widget_init((WIDGET *) w, d, 1, spacing);
	w->w.type = TYPE_INT;
	w->w.w_free = (freeFunc) int_free;
	w->w.w_paint = (paintFunc) int_paint;
	w->w.w_handle_event = (handleEventFunc) int_handle_event;
	w->w.w_get_size = (getSizeFunc) int_get_size;

	w->start = 0;
	w->length = length;
	w->w.def_width = INT_WIDTH_MAX;

	w->input = (char *) malloc(w->length + 1);
	sprintf(w->input, "%d", value);
	w->cur_pos = strlen(w->input);

	dialog_add(d, (WIDGET *) w);
	return (WIDGET *) w;
}

void wid_int_set_input (WID_INT *w, int value, int length)
{
	char val[20];
	sprintf(val, "%d", value);
	wid_str_set_input ((WID_STR*)w, val,length);
}

WIDGET *wid_button_add(DIALOG *d, int spacing, const char *button, int active)
{
	WID_BUTTON *w = (WID_BUTTON *) malloc(sizeof(WID_BUTTON));

	widget_init((WIDGET *) w, d, 1, spacing);
	w->w.type = TYPE_BUTTON;
	w->w.w_free = (freeFunc) button_free;
	w->w.w_paint = (paintFunc) button_paint;
	w->w.w_handle_event = (handleEventFunc) button_handle_event;
	w->w.w_get_size = (getSizeFunc) button_get_size;

	w->button = strdup(button);
	w->active = active;
	dialog_add(d, (WIDGET *) w);
	return (WIDGET *) w;
}

WIDGET *wid_list_add(DIALOG *d, int spacing, const char **entries, int cnt)
{
	WID_LIST *w = (WID_LIST *) malloc(sizeof(WID_LIST));

	widget_init((WIDGET *) w, d, 1, spacing);
	w->w.type = TYPE_LIST;

	w->title = NULL;
	w->entries = NULL;
	w->sel_mode = WID_SEL_SINGLE;
	w->cnt = w->cur = w->first = 0;
	w->w.def_width = LIST_WIDTH_DEFAULT;
	w->w.def_height = LIST_HEIGHT_DEFAULT;
	wid_list_set_entries (w,entries,-1,cnt);

	w->w.w_free = (freeFunc) list_free;
	w->w.w_paint = (paintFunc) list_paint;
	w->w.w_handle_event = (handleEventFunc) list_handle_event;
	w->w.w_get_size = (getSizeFunc) list_get_size;

	dialog_add(d, (WIDGET *) w);
	return (WIDGET *) w;
}

void wid_list_set_title (WID_LIST *w, const char *title)
{
	if (w->title) free (w->title);
	w->title = strdup (title);
}

void wid_list_set_entries (WID_LIST *w, const char **entries, int cur, int cnt)
{
	int i;

	if (w->entries) {
		for (i=0; i<w->cnt; i++)
			free (w->entries[i]);
		free (w->entries);
		w->entries = NULL;
	}
	w->cnt = cnt;
	if (cur>=0) {
		w->cur = cur;
		w->first = cur>0 ? cur-1:0;
	}
	if (w->cur >= cnt) w->cur = cnt>0 ? cnt-1:0;
	if (w->first > w->cur) w->first = w->cur>0 ? w->cur-1:0;

	if (cnt>0) {
		w->entries = (char **) malloc(sizeof(char*) * cnt);
		for (i=0; i<cnt; i++)
			w->entries[i] = strdup(entries[i]);
	}
}

void wid_list_set_active (WID_LIST *w, int cur)
{
	if (cur>=0 && cur < w->cnt) {
		w->cur = cur;

		if (w->cur < w->first)
			w->first = w->cur;
		if (w->cur >= w->first + w->w.height-2)
			w->first = w->cur - w->w.height + 3;
	}
}

void wid_list_set_selection_mode (WID_LIST *w, WID_SEL_MODE mode)
{
	w->sel_mode = mode;
}

WIDGET *wid_check_add(DIALOG *d, int spacing, const char *button, int active, int selected)
{
	WID_CHECK *w = (WID_CHECK *) malloc(sizeof(WID_CHECK));

	widget_init((WIDGET *) w, d, 1, spacing);
	w->w.type = TYPE_CHECK;
	w->w.w_free = (freeFunc) check_free;
	w->w.w_paint = (paintFunc) check_paint;
	w->w.w_handle_event = (handleEventFunc) check_handle_event;
	w->w.w_get_size = (getSizeFunc) check_get_size;

	w->button = strdup(button);
	w->active = active;
	w->selected = selected;
	dialog_add(d, (WIDGET *) w);
	return (WIDGET *) w;
}

void wid_check_set_selected(WID_CHECK *w, int selected)
{
	w->selected = selected;
}

WIDGET *wid_toggle_add(DIALOG *d, int spacing, const char *button, int active, int selected)
{
	WID_TOGGLE *w = (WID_TOGGLE *) malloc(sizeof(WID_TOGGLE));

	widget_init((WIDGET *) w, d, 1, spacing);
	w->w.type = TYPE_TOGGLE;
	w->w.w_free = (freeFunc) toggle_free;
	w->w.w_paint = (paintFunc) toggle_paint;
	w->w.w_handle_event = (handleEventFunc) toggle_handle_event;
	w->w.w_get_size = (getSizeFunc) toggle_get_size;

	w->button = strdup(button);
	w->active = active;
	w->selected = selected;
	dialog_add(d, (WIDGET *) w);
	return (WIDGET *) w;
}

void wid_toggle_set_selected(WID_TOGGLE *w, int selected)
{
	w->selected = selected;
}

WIDGET *wid_colorsel_add(DIALOG *d, int spacing, const char *hotkeys, int active)
{
	WID_COLORSEL *w = (WID_COLORSEL *) malloc(sizeof(WID_COLORSEL));
	int i;

	widget_init((WIDGET *) w, d, 1, spacing);
	w->w.type = TYPE_COLORSEL;
	w->w.w_free = (freeFunc) colorsel_free;
	w->w.w_paint = (paintFunc) colorsel_paint;
	w->w.w_handle_event = (handleEventFunc) colorsel_handle_event;
	w->w.w_get_size = (getSizeFunc) colorsel_get_size;

	w->active = active;
	if (hotkeys && *hotkeys) {
		strcpy (w->hkeys, hotkeys);
		w->hkeys[4] = '\0';
		for (i=0; i<strlen(w->hkeys); i++)
			w->hkeys[i] = toupper(w->hkeys[i]);
	} else
		memset (w->hkeys, 0, 5);
	w->sel_mode = WID_SEL_SINGLE;
	dialog_add(d, (WIDGET *) w);
	return (WIDGET *) w;
}

void wid_colorsel_set_active(WID_COLORSEL *w, int active)
{
	w->active = active;
}

void wid_set_size (WIDGET *w, int width, int height)
{
	if (width>=0) w->def_width = width;
	if (height>=0) w->def_height = height;
}

void wid_set_func(WIDGET *w, handleKeyFunc key, handleFocusFunc focus,
				  void *data)
{
	w->handle_key = key;
	w->handle_focus = focus;
	w->data = data;
}

void wid_repaint (WIDGET *w)
{
	if (w->w_paint) w->w_paint (w);
}

BOOL dialog_repaint(MWINDOW *win)
{
	DIALOG *d = (DIALOG *) win->data;
	int i = 0;

	win_attrset(base_attr(d,ATTR_DLG_FRAME));
	win_clear(win);
	for (i = 0; i < d->cnt; i++)
		d->widget[i]->w_paint(d->widget[i]);

	return 1;
}

void dialog_close(DIALOG *d)
{
	int i;

	for (i = 0; i < d->cnt; i++)
		d->widget[i]->w_free(d->widget[i]);
	if (d->cnt)
		free(d->widget);
	win_close(d->win);
	free(d);
}

static BOOL dialog_handle_key(MWINDOW *win, int ch)
{
	DIALOG *d = (DIALOG *) win->data;
	int ret, i;

	/* Handle keys common for all widgets here */
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
	if (ch == KEY_ESC) {
		dialog_close(d);
		return 1;
	}
#endif

	ret = d->widget[d->active]->w_handle_event(d->widget[d->active], WID_KEY,ch);
	if (!ret) {
		/* KEY not handled -> try the hotkeys */
		for (i = 0; !ret && i < d->cnt; i++) {
			ret = d->widget[i]->w_handle_event(d->widget[i], WID_HOTKEY, ch);
			if (ret == FOCUS_ACTIVATE) {
				d->widget[d->active]->has_focus = 0;
				d->widget[i]->has_focus = 1;
				d->active = i;
				d->widget[d->active]->w_handle_event(d->widget[d->active],
													 WID_GET_FOCUS, ret);
				dialog_repaint(win);
			}
		}
	} else if (ret < EVENT_HANDLED) {
		/* FOCUS_{NEXT|PREV} */
		d->widget[d->active]->has_focus = 0;
		do {
			d->active += ret;
			if (d->active < 0)
				d->active = d->cnt - 1;
			else if (d->active >= d->cnt)
				d->active = 0;
		} while (!d->widget[d->active]->can_focus);
		d->widget[d->active]->has_focus = 1;
		d->widget[d->active]->w_handle_event(d->widget[d->active],
											 WID_GET_FOCUS, ret);
		dialog_repaint(win);
	}
	return !!ret;
}

/* Return size of column of widgets which starts at widget start */
static void column_dim (DIALOG *d, int start, int *width, int *height)
{
	int i;

	*width = d->widget[start]->width;
	i = start+1;
	while (i<d->cnt && d->widget[i]->spacing>0) {
		if (d->widget[i]->width > *width)
			*width = d->widget[i]->width;
		i++;
	};
	*height = d->widget[i-1]->y+d->widget[i-1]->height-d->widget[start]->y;
}

/* Layout the dialog widgets and return the calculated size and position
   of the dialog window (which must be still opened).
   initial = true: the the focus of the widgets is changed */
static void dialog_layout(DIALOG *d, int initial,
						  int *w_x, int *w_y, int *w_width, int *w_height)
{
	int m_y, m_width = 0, m_height = 0, i, x, y, width, height;
	int spacing, c_spacing = 1, c_height, c_width;
	BOOL focus = 1;

	i = 0;
	width = 1;
	height = c_width = c_height = m_height = m_width = 0;
	while (i < d->cnt) {		/* Init all widgets(position and focus) */
		spacing = d->widget[i]->spacing;
		if (i==0 || spacing<0)
			c_spacing = (spacing == 0 ? 1:abs(spacing));

		x = 999; y = 999;
		d->widget[i]->w_get_size(d->widget[i], &x, &y);
		d->widget[i]->width = x;
		d->widget[i]->height = y;

		if (spacing>0) {
			c_height += spacing-1;
			d->widget[i]->x = width;
			d->widget[i]->y = m_height+c_height;
			if (x>c_width) c_width = x;
		} else if (spacing==0) {
			if (c_height>height) height = c_height;
			c_height = c_spacing-1;
			width += c_width + 1;

			d->widget[i]->x = width;
			d->widget[i]->y = m_height+c_height;

			c_width = x;
		} else {
			width += c_width + 1;
			if (width > m_width) m_width = width;
			if (c_height>height) height = c_height;
			m_height += height;
			c_height = -spacing-1;
			height = 0;
			width = 1;

			d->widget[i]->x = width;
			d->widget[i]->y = m_height+c_height;

			c_width = x;
		}
		c_height += y;

		if (initial) {
			if (d->widget[i]->can_focus) {
				d->widget[i]->has_focus = focus;
				if (focus)
					d->active = i;
				focus = 0;
			} else
				d->widget[i]->has_focus = 0;
		}
		i++;
	}
	width += c_width+1;
	if (width > m_width) m_width = width;
	if (c_height>height) height = c_height;
	m_height += height;

	width = m_width;
	height = m_height;
	win_get_size_max(&m_y, &m_width, &m_height);
	if (width > m_width-2 || height > m_height-2) {
		/* preferred size of widgets was to big, try to reduce the size */
		int dx = width-m_width+2, dy = height-m_height+2, free_x,
			old_width = width, old_height = height,
			i_start, m_c_height, m_c_height_old, c_height_old, c_width_old, m_x;

		m_width = m_height = 0;
		i = 0;
		while (i < d->cnt) {		/* reinit all widget positions */
			spacing = abs(d->widget[i]->spacing);
			m_height += (spacing == 0 ? 0:spacing-1);
			width = 1;

			/* get height of highest column in row */
			x = i;
			do {
				x++;
			} while (x<d->cnt && d->widget[x]->spacing>=0);
			if (x<d->cnt)
				m_c_height_old = d->widget[x]->y+d->widget[x]->spacing - d->widget[i]->y+1;
			else
				m_c_height_old =  old_height - d->widget[i]->y;

			/* get max x coordinate of last column in row */
			m_x = 0;
			do {
				x--;
				if ((d->widget[x]->x+d->widget[x]->width - 1) > m_x)
					m_x = d->widget[x]->x+d->widget[x]->width - 1;
			} while (x>0 && d->widget[x]->spacing>0);
			/* free space on right side of last column in row */
			free_x = old_width-1-m_x;
			m_c_height = 0;

			/* for all columns in one row */
			do {
				c_height = c_width = 0;
				i_start = i;
				column_dim (d, i_start, &c_width_old, &c_height_old);
				/* for all widgets in one column */
				do {
					x = d->widget[i]->width - dx + free_x;
					y = d->widget[i]->height - dy + m_c_height_old-c_height_old;
					d->widget[i]->w_get_size(d->widget[i], &x, &y);

					if (i>0 && d->widget[i]->spacing > 0)
						c_height += d->widget[i]->spacing-1;
					d->widget[i]->x = width;
					d->widget[i]->y = m_height + c_height;
					d->widget[i]->width = x;
					d->widget[i]->height = y;
					if (x>c_width) c_width = x;
					c_height += y;

					y = c_width_old;
					column_dim (d, i_start, &c_width_old, &c_height_old);
					free_x += y - c_width_old;

					i++;
				} while ((i < d->cnt) && (d->widget[i]->spacing > 0));

				width += c_width +1;
				if (c_height > m_c_height)
					m_c_height = c_height;
			} while ((i < d->cnt) && (d->widget[i]->spacing >= 0));
			if (width > m_width)
				m_width = width;
			m_height += m_c_height;
		}
		width = m_width;
		height = m_height;
		win_get_size_max(&m_y, &m_width, &m_height);
	}
	m_width -= 2;
	m_height -= 2;

	*w_x = (m_width - width) / 2 + 1;
	if (*w_x < 1)
		*w_x = 1;
	*w_y = (m_height - height) / 2 + m_y +1;
	if (*w_y <= m_y)
		*w_y = m_y+1;
	*w_width = (width>m_width ? m_width:width);
	*w_height = (height>m_height ? m_height:height);
}

static void dialog_handle_resize(MWINDOW *win, int dx, int dy)
{
	DIALOG *d = (DIALOG *) win->data;
	int x,y,width,height;

	dialog_layout (d,0,&x,&y,&width,&height);
	win->x = x;
	win->y = y;
	win->width = width;
	win->height = height;
}

void dialog_open(DIALOG *d, const char *title)
{
	int x,y,width,height;

	dialog_layout (d,1,&x,&y,&width,&height);

	if (!title)
		title = "Dialog";
	win_open(x, y, width, height, 1, title, base_attr(d,ATTR_DLG_FRAME));
	win_set_repaint(dialog_repaint);
	win_set_handle_key(dialog_handle_key);
	win_set_resize(0, dialog_handle_resize);
	win_set_data((void *)d);

	d->win = win_get_window();
	dialog_repaint(d->win);
}

/* set attribute which is used for DLG_FRAME and DLG_LABEL,
   works only before dialog_open() */
void dialog_set_attr (DIALOG *d, ATTRS attrs)
{
	d->attrs = attrs;
}

DIALOG *dialog_new(void)
{
	DIALOG *d = (DIALOG *) malloc(sizeof(DIALOG));

	d->active = 0;
	d->cnt = 0;
	d->attrs = ATTR_NONE;
	d->win = NULL;
	d->widget = NULL;
	return d;
}

/* ex:set ts=4: */
