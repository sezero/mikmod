/*  MikMod module player
	(c) 1998 - 2014 Miodrag Vallat and others - see file AUTHORS for
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

  Some utility functions

==============================================================================*/

#ifndef MUTILITIES_H
#define MUTILITIES_H

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(__OS2__)||defined(__EMX__)
#include <os2.h>
#ifndef HAVE_CONFIG_H
#define RETSIGTYPE void
#endif
#endif

#if defined(__MORPHOS__) || defined(__AROS__) || defined(AMIGAOS)	|| \
    defined(__amigaos__) || defined(__amigados__)			|| \
    defined(AMIGA) || defined(_AMIGA) || defined(__AMIGA__)
#include <exec/types.h>
#define _mikmod_amiga 1
#endif

#include <mikmod.h>	/* for BOOL */

/*========== Constants */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef PATH_MAX
#if defined(MAXPATHLEN) /* <sys/param.h> */
#define PATH_MAX MAXPATHLEN
#elif defined(_WIN32) && defined(_MAX_PATH)
#define PATH_MAX _MAX_PATH
#elif defined(_WIN32) && defined(MAX_PATH)
#define PATH_MAX MAX_PATH
#elif defined(__OS2__) && defined(CCHMAXPATH)
#define PATH_MAX CCHMAXPATH
#else
#define PATH_MAX 256
#endif
#endif /* PATH_MAX */

#include <string.h>

#define PATH_SEP '/'
#define PATH_SEP_STR "/"

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)

#define PATH_SEP_SYS '\\'
#define PATH_SEP_SYS_STR "\\"
void path_conv(char *file);
char *path_conv_sys(const char *file);
char *path_conv_sys2(const char *file);

#else

#define PATH_SEP_SYS '/'
#define PATH_SEP_SYS_STR "/"
#define path_conv(file)
#define path_conv_sys(file) (file)
#define path_conv_sys2(file) (file)

#endif

#ifdef _mikmod_amiga
#define IS_PATH_SEP(c) ((c) == PATH_SEP || (c) == ':')
static inline char *FIND_FIRST_DIRSEP(const char *_the_path) {
    char *p = strchr(_the_path, ':');
    if (p != NULL) return p;
    return strchr(_the_path, PATH_SEP);
}
static inline char *FIND_LAST_DIRSEP (const char *_the_path) {
    char *p = strrchr(_the_path, PATH_SEP);
    if (p != NULL) return p;
    return strchr(_the_path, ':');
}
#else
#define IS_PATH_SEP(c) ((c) == PATH_SEP)
#define FIND_FIRST_DIRSEP(p) strchr((p), PATH_SEP)
#define FIND_LAST_DIRSEP(p) strrchr((p), PATH_SEP)
#endif

/*========== Types */

/* pointer-sized signed int (ssize_t/intptr_t) : */
#if defined(_WIN64) /* win64 is LLP64, not LP64  */
typedef long long       SINTPTR_T;
#else
/* long should be pointer-sized for all others : */
typedef long            SINTPTR_T;
#endif

/*========== Variables */

/* storage buffer length - used everywhere */
#define STORAGELEN	320
extern char storage[STORAGELEN+2];

/*========== Routines and macros */

#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define BTST(v, m) ((v) & (m) ? 1 : 0)

#ifdef _WIN32
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

#if defined(__EMX__)||defined(_WIN32)
#undef S_ISBLK /* MinGW sys/stat.h does define S_ISBLK */
#define S_ISBLK(st_mode)  0
#endif

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
#undef S_ISLNK /* djgpp-v2.04 does define S_ISLNK (and has lstat, too..) */
#define lstat             stat
#define S_ISSOCK(st_mode) 0
#define S_ISLNK(st_mode)  0
#endif

#if defined(_WIN32)&&!defined(__MINGW32__)&&!defined(__WATCOMC__)
typedef struct dirent {
	char name[PATH_MAX+1];
	unsigned long* handle;
	int filecnt;
	char d_name[PATH_MAX+1];
} DIR;

DIR* opendir (const char* dirName);
struct dirent *readdir (DIR* dir);
int closedir (DIR* dir);
#endif /* dirent _WIN32 */

/* allocate memory for a formated string and do a sprintf */
char *str_sprintf2(const char *fmt, const char *arg1, const char *arg2);
char *str_sprintf(const char *fmt, const char *arg);

/* tmpl: file name template ending in 'XXXXXX' without path or NULL
   name_used: if !=NULL pointer to name of temp file, must be freed
   return: file descriptor or -1 */
int get_tmp_file (const char *tmpl, char **name_used);

/* allocate and return a name for a temporary file
   (under UNIX not used because of tempnam race condition) */
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)||defined(_mikmod_amiga)
char *get_tmp_name(void);
#endif

BOOL file_exist(const char *file);
/* determines if a given path is absolute or relative */
BOOL path_relative(const char *path);

/* allocate and return a filename including the path for a config file
   'name': filename without the path */
char *get_cfg_name(const char *name);

/* Return precise time in milliseconds */
unsigned long Time1000(void);

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)||defined(_mikmod_amiga)
#define filecmp strcasecmp
#else
#define filecmp strcmp
#endif
#if defined(__OS2__)||defined(__EMX__)||(defined(_WIN32)&&!defined(__MINGW32__))
#define strcasecmp(s,t) stricmp(s,t)
#endif

#ifdef HAVE_VSNPRINTF
# ifdef _WIN32
#  define VSNPRINTF _vsnprintf
# else
#  define VSNPRINTF vsnprintf
# endif
#else
#define VSNPRINTF(str,size,format,ap) vsprintf(str,format,ap)
#endif

#ifndef HAVE_SNPRINTF
#define SNPRINTF mik_snprintf
int mik_snprintf(char *buffer, size_t n, const char *format, ...);
#else
# ifdef _WIN32
#  define SNPRINTF _snprintf
# else
#  define SNPRINTF snprintf
# endif
#endif

/* Return newly malloced version and cmdline for the driver
   with the number drvno. */
BOOL driver_get_info (int drvno, char **version, char **cmdline);

#endif /* MUTILITIES_H */

/* ex:set ts=4: */
