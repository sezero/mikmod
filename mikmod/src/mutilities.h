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

  $Id: mutilities.h,v 1.1.1.1 2004/01/16 02:07:34 raph Exp $

  Some utility functions

==============================================================================*/

#ifndef MUTILITIES_H
#define MUTILITIES_H

#ifdef WIN32
#include <windows.h>
#elif defined(__OS2__)||defined(__EMX__)
#include <os2.h>
#endif

#include <mikmod.h>	/* for BOOL */

/*========== Constants */

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef PATH_MAX
#ifdef _POSIX_PATH_MAX
#define PATH_MAX _POSIX_PATH_MAX
#elif defined(_MAX_PATH)
#define PATH_MAX _MAX_PATH
#else
#define PATH_MAX 256
#endif
#endif

#define PATH_SEP '/'
#define PATH_SEP_STR "/"

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(WIN32)

#define PATH_SEP_SYS '\\'
#define PATH_SEP_SYS_STR "\\"
void path_conv(char *file);
char *path_conv_sys(char *file);
char *path_conv_sys2(char *file);

#else

#define PATH_SEP_SYS '/'
#define PATH_SEP_SYS_STR "/"
#define path_conv(file)
#define path_conv_sys(file) (file)
#define path_conv_sys2(file) (file)

#endif

/*========== Variables */

/* storage buffer length - used everywhere */
#define STORAGELEN	320
extern char storage[STORAGELEN+2];

/*========== Routines and macros */

#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define BTST(v, m) ((v) & (m) ? 1 : 0)

#ifdef WIN32
#define stat              _stat
#ifndef S_ISDIR
#define S_ISDIR(st_mode)  ((st_mode & _S_IFDIR) == _S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR(st_mode)  ((st_mode & _S_IFCHR) == _S_IFCHR)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(st_mode) ((st_mode & _S_IFIFO) == _S_IFIFO)
#endif
#endif

#if defined(__EMX__)||defined(WIN32)
#define S_ISBLK(st_mode)  0
#endif

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(WIN32)
#undef S_ISLNK /* djgpp-v2.04 does define S_ISLNK (and has lstat, too..) */
#define lstat             stat
#define S_ISSOCK(st_mode) 0
#define S_ISLNK(st_mode)  0
#endif

#if defined(__OS2__)||defined(__EMX__)||defined(WIN32)

/* FIXME , untested under OS2 */

typedef struct dirent {
	char name[PATH_MAX+1];
	unsigned long* handle;
	int filecnt;
	char d_name[PATH_MAX+1];
} DIR;

DIR* opendir (const char* dirName);
struct dirent *readdir (DIR* dir);
int closedir (DIR* dir);

#endif

/* allocate memory for a formated string and do a sprintf */
char *str_sprintf2(char *fmt, char *arg1, char *arg2);
char *str_sprintf(char *fmt, char *arg);

/* tmpl: file name template ending in 'XXXXXX' without path or NULL
   name_used: if !=NULL pointer to name of temp file, must be freed
   return: file descriptor or -1 */
int get_tmp_file (char *tmpl, char **name_used);

/* allocate and return a name for a temporary file
   (under UNIX not used because of tempnam race condition) */
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(WIN32)
char *get_tmp_name(void);
#endif

BOOL file_exist(char *file);
/* determines if a given path is absolute or relative */
BOOL path_relative(char *path);

/* allocate and return a filename including the path for a config file
   'name': filename without the path */
char *get_cfg_name(char *name);

/* Return precise time in milliseconds */
unsigned long Time1000(void);

#if defined(__OS2__)||defined(__EMX__)||defined(WIN32)
#define strcasecmp(s,t) stricmp(s,t)
#endif

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(WIN32)
#define filecmp strcasecmp
#else
#define filecmp strcmp
#endif

#ifdef HAVE_VSNPRINTF
#define VSNPRINTF vsnprintf
#else
#define VSNPRINTF(str,size,format,ap) vsprintf(str,format,ap)
#endif

#ifndef HAVE_SNPRINTF
int snprintf(char *buffer, size_t n, const char *format, ...);
#endif

/* Return newly malloced version and cmdline for the driver
   with the number drvno. */
BOOL driver_get_info (int drvno, char **version, char **cmdline);

#endif /* MUTILITIES_H */

/* ex:set ts=4: */
