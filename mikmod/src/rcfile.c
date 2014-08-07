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

  $Id: rcfile.c,v 1.1.1.1 2004/01/16 02:07:41 raph Exp $

  General configuration file management

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <mikmod.h>
#include "rcfile.h"
#include "mutilities.h"

#define INDENT_MAX		40
#define LINE_LEN		1024
#define OPTION_BLOCK	10
#define BTST(v, m)		((v) & (m) ? 1 : 0)

typedef struct _OPTION OPTION;
typedef struct _OPTIONS OPTIONS;
typedef struct _STACK STACK;

struct _OPTION {
	char *label;
	char *arg;
	OPTIONS *options;
};

struct _OPTIONS {
	int cnt, max;
	OPTION *option;
	OPTIONS *parent;
};

struct _STACK {
	STACK *next;
	char *data;
};

static FILE *fp = NULL;
static OPTIONS *options = NULL;
static char indent[INDENT_MAX+1] = "";
static STACK *structs = NULL;

static void indent_change (int delta)
{
	int len = strlen(indent);
	delta *= 2;
	if (len+delta>=0 && len+delta<=INDENT_MAX)
		len += delta;
	indent[len] = '\0';
	if (len>0) {
		indent[len-1] = ' ';
		indent[len-2] = ' ';
	}
}

static void options_free (OPTIONS *opts)
{
	if (opts && opts->max>0) {
		while (opts->cnt>0) {
			opts->cnt--;
			if (opts->option[opts->cnt].label)
				free (opts->option[opts->cnt].label);
			if (opts->option[opts->cnt].arg)
				free (opts->option[opts->cnt].arg);
			if (opts->option[opts->cnt].options) {
				options_free (opts->option[opts->cnt].options);
			}
		}
		free (opts->option);
		opts->cnt = opts->max = 0;
	}
	if (opts) free (opts);
}

/* Save desc in file fp. Add '# ' in front of all lines. */
static void write_description (const char *desc)
{
	const char *start;
	if (fp && desc) {
		fputs ("\n",fp);
		while (*desc) {
			start = desc;
			while (*desc && *desc!='\n') desc++;
			fprintf (fp, "%s# ", indent);
			fwrite (start,desc-start,1,fp);
			fputs ("\n",fp);
			if (*desc) desc++;
		}
	}
}

/* write argument arg with optional description and mark it with label */
BOOL rc_write_bool (const char *label, int arg, const char *description)
{
	if (fp) {
		write_description (description);
		if (arg)
			return fprintf(fp, "%s%s = yes\n", indent, label) > 0;
		else
			return fprintf(fp, "%s%s = no\n", indent, label) > 0;
	}
	return 0;
}

BOOL rc_write_bit (const char *label, int arg, int mask, const char *description)
{
	return rc_write_bool (label,BTST(arg,mask),description);
}

BOOL rc_write_int (const char *label, int arg, const char *description)
{
	if (fp) {
		write_description (description);
		return fprintf(fp, "%s%s = %d\n", indent, label, arg) > 0;
	}
	return 0;
}

BOOL rc_write_float (const char *label, float arg, const char *description)
{
	if (fp) {
		write_description (description);
		return fprintf(fp, "%s%s = %f\n", indent, label, arg) > 0;
	}
	return 0;
}

BOOL rc_write_label(const char *label, LABEL_CONV *convert, int arg, const char *description)
{
	if (fp) {
		int i;
		write_description (description);
		for (i = 0; convert[i].id != arg; i++);
		return fprintf(fp, "%s%s = %s\n", indent, label, convert[i].label) > 0;
	}
	return 0;
}

BOOL rc_write_string (const char *label, const char *arg, const char *description)
{
	if (fp) {
		write_description (description);
		if (arg) {
			if (fprintf(fp,"%s%s = \"", indent,label) <= 0)
				return 0;
			while (*arg) {
				if (*arg<32 || (unsigned char)*arg>127)
					fprintf (fp, "\\x%02x",*(const unsigned char*)arg);
				else if (*arg == '"')
					fputs ("\\\"", fp);
				else
					fputc (*arg, fp);
				arg++;
			}
			return fprintf(fp,"\"\n") > 0;
		} else
			return fprintf(fp,"%s%s = \"\"\n", indent,label) > 0;
	}
	return 0;
}

BOOL rc_write_struct (const char *label, const char *description)
{
	if (fp) {
		BOOL ret;
		STACK *newstack = (STACK *) malloc (sizeof(STACK));

		newstack->data = strdup (label);
		newstack->next = structs;
		structs = newstack;

		write_description (description);
		ret = fprintf(fp,"%sBEGIN \"%s\"\n", indent, label) > 0;
		indent_change (1);
		return ret;
	}
	return 0;
}

BOOL rc_write_struct_end (const char *description)
{
	if (fp && structs) {
		BOOL ret;
		STACK *next = structs->next;
		char *label = structs->data;
		free (structs);
		structs = next;

		indent_change (-1);
		write_description (description);
		ret = fprintf(fp,"%sEND \"%s\"\n", indent, label) > 0;
		free (label);
		return ret;
	}
	return 0;
}

/* search for label in loaded options and return the associated value */
static char *get_argument (const char *label)
{
	int i;
	for (i=0; i<options->cnt; i++)
		if (options->option[i].label &&
			!strcasecmp (options->option[i].label,label)) {
			/* mark entry as handled */
			free (options->option[i].label);
			options->option[i].label = NULL;
			return options->option[i].arg;
		}
	return NULL;
}

/* search for label in loaded options and return the associated value */
static OPTIONS *get_begin (const char *label)
{
	int i;
	for (i=0; i<options->cnt; i++)
		if (options->option[i].label &&
			!strcasecmp (options->option[i].label,"BEGIN") &&
			!strcasecmp (options->option[i].arg,label)) {
			/* mark entry as handled */
			free (options->option[i].label);
			options->option[i].label = NULL;
			return options->option[i].options;
		}
	return NULL;
}

/* Read 'value', which is saved in the config-file under label.
   Change 'value' only if label is present in config-file and
   associated value is valid.
   Return: value changed ? */
BOOL rc_read_bool (const char *label, BOOL *value)
{
	char *arg = get_argument (label);
	if (arg) {
		if ((!strcasecmp(arg, "YES")) || (!strcasecmp(arg, "ON")) || (*arg == '1')) {
			*value = 1;
			return 1;
		} else if ((!strcasecmp(arg, "NO")) || (!strcasecmp(arg, "OFF")) ||
				 (*arg == '0')) {
			*value = 0;
			return 1;
		}
	}
	return 0;
}

BOOL rc_read_bit (const char *label, int *value, int mask)
{
	const char *arg = get_argument (label);
	if (arg) {
		if ((!strcasecmp(arg, "YES")) || (!strcasecmp(arg, "ON")) || (*arg == '1')) {
			*value |= mask;
			return 1;
		} else if ((!strcasecmp(arg, "NO")) || (!strcasecmp(arg, "OFF")) ||
				 (*arg == '0')) {
			*value &= ~mask;
			return 1;
		}
	}
	return 0;
}

BOOL rc_read_int (const char *label, int *value, int min, int max)
{
	const char *arg = get_argument (label);
	if (arg) {
		char *end;
		int t = strtol(arg, &end, 10);
		if ((!*end) && (t >= min) && (t <= max)) {
			*value = t;
			return 1;
		}
	}
	return 0;
}

BOOL rc_read_float (const char *label, float *value, float min, float max)
{
	const char *arg = get_argument (label);
	if (arg) {
		float t;
		if (sscanf (arg,"%f",&t) == 1)
			if ((t >= min) && (t <= max)) {
				*value = t;
				return 1;
			}
	}
	return 0;
}

BOOL rc_read_label(const char *label, int *value, LABEL_CONV *convert)
{
	const char *arg = get_argument (label);
	if (arg) {
		int i = 0;
		while (convert[i].label) {
			if (!strcasecmp(convert[i].label, arg)) {
				*value = convert[i].id;
				return 1;
			}
			i++;
		}
	}
	return 0;
}

BOOL rc_read_struct (const char *label)
{
	OPTIONS *arg = get_begin (label);
	if (arg) {
		options = arg;
		return 1;
	}
	return 0;
}

BOOL rc_read_struct_end (void)
{
	if (options->parent) {
		options = options->parent;
		return 1;
	} else
		return 0;
}

/* Free old *value and allocate min(strlen(newvalue),length)+1 bytes for new string. */
void rc_set_string (char **value, const char *arg, int length)
{
	int len = strlen(arg);

	if (len > length)
		len = length;
	if (*value)
		free(*value);
	*value = (char *)malloc((len + 1) * sizeof(char));
	strncpy(*value, arg, len);
	(*value)[len] = '\0';
}

/* Read a string. Free old *value and allocate
   min(strlen(newvalue),length)+1 bytes for new string. */
BOOL rc_read_string (const char *label, char **value, int length)
{
	const char *arg = get_argument (label);
	if (arg) {
		rc_set_string (value,arg,length);
		return 1;
	}
	return 0;
}

static char skip_space (char **line)
{
	while (**line==' ' || **line=='\t')
		(*line)++;
	return **line;
}

static BOOL parse_line (char *line, char **label, char **arg)
{
	char *end;

	*label = NULL;
	*arg = NULL;

	if (skip_space(&line) == '#') return 0;

	*label = line;
	while (isalnum((int)*line) || *line == '_') {
		*line = toupper ((int)*line);
		line++;
	}
	end = line;

	skip_space(&line);
	if (*line=='=') {
		line++;
		*end = '\0';
		skip_space (&line);
	} else {
		*end = '\0';
		if (strcmp(*label,"BEGIN") && strcmp(*label,"END"))
			return 0;
	}
	if (isgraph((int)*line)) {
		char *pos, ch1, ch2;
		BOOL string = (*line == '"');

		if (string) line++;
		*arg = pos = line;
		while ((!string && *line && *line != '#') ||
			   (string && *line && *line!='"')) {
			if (!string) {
				*line=toupper((int)*line);
			} else {
				if (*line == '\\') {
					line++;
					switch (*line) {
						case 'a':
							*pos = '\a';
							break;
						case 'b':
							*pos = '\b';
							break;
						case 'f':
							*pos = '\f';
							break;
						case 'n':
							*pos = '\n';
							break;
						case 'r':
							*pos = '\r';
							break;
						case 't':
							*pos = '\t';
							break;
						case 'v':
							*pos = '\v';
							break;
						case '\'':
							*pos = '\'';
							break;
						case '"':
							*pos = '\"';
							break;
						case '\\':
							*pos = '\\';
							break;
						case 'x':
							ch1 = toupper((int)*(line+1));
							ch2 = toupper((int)*(line+2));
							*pos = (ch1>='A' ? (ch1-'A'+10):(ch1-'0'))*16+
								   (ch2>='A' ? (ch2-'A'+10):(ch2-'0'));
							line += 2;
							break;
						default:
							line--;
							*pos = *line;
					}
				} else
					*pos = *line;
			}
			line++; pos++;
		}
		if (!string) {
			do {
				pos--;
			} while (*pos == ' ' || *pos == '\t');
			pos++;
		}
		*pos = '\0';
		return 1;
	}
	return 0;
}

static BOOL rc_parse (OPTIONS *opts, const char *sec_name)
{
	char line[LINE_LEN],*label,*arg;
	BOOL ret = 1;

	while (ret && fgets(line,LINE_LEN,fp)) {
		if (line[strlen(line)-1]=='\n') line[strlen(line)-1]='\0';

		if (parse_line(line,&label,&arg)) {
			if (!strcmp("END", label)) {
				if (strcmp(arg,sec_name)) {
					fprintf (stderr,
							 "Error in config file: expected 'END %s', found 'END %s'"
							 "                      Ignoring (remaining) config file...",
							 sec_name, arg);
					return 0;
				}
				return 1;
			} else {
				if (opts->cnt >= opts->max) {
					opts->max += OPTION_BLOCK;
					opts->option = (OPTION *) realloc (opts->option,sizeof(OPTION)*opts->max);
				}
				opts->option[opts->cnt].label = strdup (label);
				opts->option[opts->cnt].arg = strdup (arg);
				if (!strcmp("BEGIN", label)) {
					OPTIONS *new_opts = (OPTIONS *) malloc(sizeof(OPTIONS));
					new_opts->cnt = new_opts->max = 0;
					new_opts->option = NULL;
					new_opts->parent = opts;
					opts->option[opts->cnt].options = new_opts;
					ret = rc_parse (new_opts, opts->option[opts->cnt].arg);
				} else {
					opts->option[opts->cnt].options = NULL;
				}
				opts->cnt++;
			}
		}
	}
	if (ferror(fp))
		fprintf (stderr, "Error in config file, ignoring (remaining) file...");
	return ret && !ferror(fp);
}

/* open config-file 'name' and parse the file for following rc_read_...() */
BOOL rc_load (const char *name)
{
	BOOL ret = 0;

	if (!(fp = fopen (path_conv_sys(name),"r"))) return 0;
	options = (OPTIONS *) malloc(sizeof(OPTIONS));
	options->cnt = options->max = 0;
	options->option = NULL;
	options->parent = NULL;
	ret = rc_parse (options,"'NO END'");
	fclose (fp);
	fp = NULL;
	return ret;
}

/* open config-file 'name' for following rc_write_...()
   and write a header for program 'prg_name' */
BOOL rc_save (const char *name, const char *prg_name)
{
	if (!(fp=fopen(path_conv_sys(name),"w"))) return 0;

	if (fprintf (fp,"#\n"
				 "# %s\n"
				 "# configuration file\n"
				 "#\n",prg_name) <= 0) {
		fclose (fp);
		fp = NULL;
		return 0;
	}
	return 1;
}

/* close config-file opened by rc_load() or rc_save() */
void rc_close (void)
{
	if (fp) {
		fclose (fp);
		fp = NULL;
	}
	options_free (options);
	options = NULL;
}
