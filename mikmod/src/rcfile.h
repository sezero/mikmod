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

  $Id: rcfile.h,v 1.1.1.1 2004/01/16 02:07:41 raph Exp $

  General configuration file management

==============================================================================*/

#ifndef RCFILE_H
#define RCFILE_H

typedef struct {
    int id;
    const char *label;
} LABEL_CONV;

/* Write argument arg with optional description (multiple lines
  allowed) and mark it with label.
  Return: arg successfully written ? */
BOOL rc_write_bool (const char *label, int arg, const char *description);
BOOL rc_write_bit (const char *label, int arg, int mask, const char *description);
BOOL rc_write_int (const char *label, int arg, const char *description);
BOOL rc_write_float (const char *label, float arg, const char *description);
BOOL rc_write_label(const char *label, LABEL_CONV *convert, int arg, const char *description);
BOOL rc_write_string (const char *label, const char *arg, const char *description);
BOOL rc_write_struct (const char *label, const char *description);
BOOL rc_write_struct_end (const char *description);

/* Read 'value', which is saved in the config-file under label.
   Change 'value' only if label is present in config-file and
   associated value is valid.
   Return: value changed ? */
BOOL rc_read_bool (const char *label, BOOL *value);
BOOL rc_read_bit (const char *label, int *value, int mask);
BOOL rc_read_int (const char *label, int *value, int min, int max);
BOOL rc_read_float (const char *label, float *value, float min, float max);
BOOL rc_read_label(const char *label, int *value, LABEL_CONV *convert);
BOOL rc_read_struct (const char *label);
BOOL rc_read_struct_end (void);

/* Free old *value and allocate min(strlen(newvalue),length)+1 bytes for new string. */
void rc_set_string (char **value, const char *arg, int length);
/* Read a string. Free old *value and allocate
   min(strlen(newvalue),length)+1 bytes for new string. */
BOOL rc_read_string (const char *label, char **value, int length);

/* open config-file 'name' and parse the file for following rc_read_...() */
BOOL rc_load (const char *name);

/* open config-file 'name' for following rc_write_...()
   and write a header for program 'prg_name' */
BOOL rc_save (const char *name, const char *prg_name);

/* close config-file opened by rc_load() or rc_save() */
void rc_close (void);

#endif /* RCFILE_H */
