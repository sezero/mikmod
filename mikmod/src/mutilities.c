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

  $Id: mutilities.c,v 1.1.1.1 2004/01/16 02:07:34 raph Exp $

  Some utility functions

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
#include <pwd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <time.h>
#elif defined(__OS2__) || defined(__EMX__)
#define INCL_DOS
#include <os2.h>
#include <io.h>
#else
#include <sys/time.h>
#endif

#include "player.h"
#include "mlist.h"
#include "marchive.h"
#include "mutilities.h"

#ifdef _mikmod_amiga
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#if defined(__DJGPP__)
static const char *get_homedir (void)
{
	return "C:"; /* good enough for msdos */
}
#elif defined(_mikmod_amiga)
static const char *get_homedir (void)
{
	static char homdir[PATH_MAX];
	static char *home = NULL;
	if (!home) {
		BPTR lock = GetProgramDir();
		if (!lock || !NameFromLock(lock, homdir, PATH_MAX))
			strcpy(homdir, "SYS:");
		if (!homdir[0]) /* possible?? */
			strcpy(homdir, "SYS:");
		else {
			home = homdir + strlen(homdir);
			if (!IS_PATH_SEP(home[-1])) {
				home[0] = PATH_SEP;
				home[1] = 0;
			}
		}
		home = homdir;
	}
	return home;
}
#elif defined(__OS2__)||defined(__EMX__)
static const char *get_homedir (void)
{
	const char *home = getenv("HOME");
	if (!home || !*home) return "C:";
	return home;
}
#elif defined(_WIN32)
static const char *get_homedir (void)
{
	const char *home;
# ifndef _WIN64
	static int is_w9x = -1;
	if (is_w9x < 0) {
		OSVERSIONINFO v;
		v.dwOSVersionInfoSize = sizeof(v);
		if (!GetVersionEx(&v) || v.dwMajorVersion < 4 ||
			v.dwPlatformId < VER_PLATFORM_WIN32_NT) {
			is_w9x = 1;
		}
		else	is_w9x = 0;
	}
	if (is_w9x) return "C:";
# endif
	home = getenv("USERPROFILE");
	if (!home || !*home) return "C:";
	return home;
}
#else /* unix */
static const char *get_homedir (void)
{
	static const char *home = NULL;
	static char d[PATH_MAX];
	if (!home) {
		struct passwd *pw = getpwuid(getuid());
		memset(d, 0, sizeof(d));
		if (pw && pw->pw_dir) {
			strncpy(d, pw->pw_dir, sizeof(d));
			d[sizeof(d) - 1] = 0;
			home = d;
		}
		else if ((home = getenv("HOME")) != NULL) {
			strncpy(d, home, sizeof(d));
			d[sizeof(d) - 1] = 0;
			home = d;
		}
		else {
			home = ""; /* fubar'ed.. */
		}
	}
	return home;
}
#endif /* get_homedir */

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)

void path_conv(char *file)
{
	if (!file) return;
	for (; *file; file++) {
		if (*file == PATH_SEP_SYS)
			*file = PATH_SEP;
	}
}

char *path_conv_sys(const char *file)
{
	static char f[PATH_MAX];
	char *pos = f;
	const char *end = file + PATH_MAX-1;

	if (!file) return NULL;
	for (; *file && file<end; file++, pos++) {
		if (*file == PATH_SEP)
			*pos = PATH_SEP_SYS;
		else
			*pos = *file;
	}
	if (pos-1>f && *(pos-1) == PATH_SEP_SYS && *(pos-2) != ':')
		pos--;
	*pos = '\0';
	return f;
}

char *path_conv_sys2(const char *file)
{
	static char f[PATH_MAX];
	char *pos = f;
	const char *end = file + PATH_MAX-1;

	if (!file) return NULL;
	for (; *file && file<end; file++, pos++) {
		if (*file == PATH_SEP)
			*pos = PATH_SEP_SYS;
		else
			*pos = *file;
	}
	*pos = '\0';
	return f;
}

#endif

/* allocate memory for a formated string and do a sprintf */
char *str_sprintf2(const char *fmt, const char *arg1, const char *arg2)
{
	char *msg = (char *) malloc(strlen(fmt) + strlen(arg1) + strlen(arg2) + 1);

	if (msg)
		sprintf(msg, fmt, arg1, arg2);
	return msg;
}

/* allocate memory for a formated string and do a sprintf */
char *str_sprintf(const char *fmt, const char *arg)
{
	return str_sprintf2(fmt, arg, "");
}

BOOL file_exist(const char *file)
{
	struct stat sb;

	return (stat(path_conv_sys(file), &sb) == -1) ? 0 : 1;
}

/* determines if a given path is absolute or relative */
BOOL path_relative(const char *path)
{
	if (!path)
		return 1;

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
	if (*path && (path[1] == ':'))
		return 0;
#elif defined(_mikmod_amiga)
	if (strchr(path, ':')) return 0;
#endif

	return (*path != PATH_SEP);
}

/* mkstemp() implementation is based on the GNU C library implementation.
   Copyright (C) 1991,92,93,94,95,96,97,98,99 Free Software Foundation, Inc. */
static int m_mkstemp (char *tmpl)
{
#ifdef HAVE_MKSTEMP
	return mkstemp (tmpl);
#else
	static const char letters[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	static const int NLETTERS = sizeof (letters) - 1;
	static int counter = 0;
	int len, count, fd;
	char *XXXXXX;
	long value;

	len = strlen (tmpl);
	if (len < 6 || strcmp (&tmpl[len - 6], "XXXXXX"))
		return -1;

	/* This is where the Xs start.  */
	XXXXXX = &tmpl[len - 6];

	/* Get some more or less random data.  */
#ifdef _WIN32
	value = Time1000();
	value = ((value % 1000) ^ (value / 1000)) + counter++;
#else
	{
		struct timeval tv;
		gettimeofday (&tv, NULL);
		value = (tv.tv_usec ^ tv.tv_sec) + counter++;
	}
#endif

	for (count = 0; count < 100; value += 7777, ++count) {
		long v = value;

		/* Fill in the random bits.  */
		XXXXXX[0] = letters[v % NLETTERS];
		v /= NLETTERS;
		XXXXXX[1] = letters[v % NLETTERS];
		v /= NLETTERS;
		XXXXXX[2] = letters[v % NLETTERS];
		v /= NLETTERS;
		XXXXXX[3] = letters[v % NLETTERS];
		v /= NLETTERS;
		XXXXXX[4] = letters[v % NLETTERS];
		v /= NLETTERS;
		XXXXXX[5] = letters[v % NLETTERS];

		fd = open (tmpl, O_RDWR | O_CREAT | O_EXCL | O_BINARY, 0600);

		if (fd >= 0)
			return fd;
		else if (errno != EEXIST)
			/* Any other error will apply also to other names we might
			   try, and there are 2^32 or so of them, so give up now. */
			return -1;
	}

	/* We got out of the loop because we ran out of combinations to try.  */
	return -1;
#endif
}

/* tmpl: file name template ending in 'XXXXXX' without path or NULL
   name_used: if !=NULL pointer to name of temp file, must be freed
   return: file descriptor or -1 */
int get_tmp_file (const char *tmpl, char **name_used)
{
	static const char *tmpdir = NULL;
	static const char *tmpsep = "";
	char *fulltmpl;
	int retval;

	if (!tmpdir) {
#if defined(_mikmod_amiga)
		tmpdir = "T:";
#else /* ! amiga: */
		tmpdir = getenv ("TMPDIR");
		if (!tmpdir) tmpdir = getenv ("TMP");
		if (!tmpdir) tmpdir = getenv ("TEMP");
#ifdef P_tmpdir
		if (!tmpdir) tmpdir = P_tmpdir;
#endif
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)
		if (!tmpdir) tmpdir = "C:\\";
#else
		if (!tmpdir) tmpdir = "/tmp";
#endif
		if (*tmpdir && tmpdir[strlen(tmpdir) - 1] == PATH_SEP_SYS)
			tmpsep = "";
		else	tmpsep = PATH_SEP_SYS_STR;
#endif /* !amiga */
	}
	if (tmpl == NULL) tmpl = "mmXXXXXX";

	fulltmpl = (char *) malloc (strlen(tmpdir)+strlen(tmpsep)+strlen(tmpl)+1);
	sprintf (fulltmpl, "%s%s%s", tmpdir, tmpsep, tmpl);

	retval = m_mkstemp (fulltmpl);
	if (retval == -1) {
		free (fulltmpl);
		return -1;
	}

	if (name_used) {
		path_conv (fulltmpl);
		*name_used = fulltmpl;
	} else
		free (fulltmpl);

	return retval;
}

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)||defined(_mikmod_amiga)
/* allocate and return a name for a temporary file
   (under UNIX not used because of tempnam race condition) */
char *get_tmp_name(void)
{
	CHAR *tmp_file;
#if defined(__OS2__) && defined(__WATCOMC__)
	tmp_file = str_sprintf2("%s" PATH_SEP_STR "%s", getenv("TEMP"),
							"!MikMod.tmp");
#elif defined(_WIN32)
	if (!(tmp_file = _tempnam(NULL, ".mod")))
		if (!(tmp_file = _tempnam(get_homedir(), ".mod")))
			return NULL;
#elif defined(_mikmod_amiga)
	char s[16];
	sprintf(s,"%d", rand());
	tmp_file = str_sprintf("T:%s.mik", s);
#else
	if (!(tmp_file = tempnam(NULL, ".mod")))
		if (!(tmp_file = tempnam(get_homedir(), ".mod")))
			return NULL;
#endif
	path_conv(tmp_file);
	return tmp_file;
}
#endif

/* allocate and return a filename including the path for a config file
   'name': filename without the path */
char *get_cfg_name(const char *name)
{
#if defined(_mikmod_amiga)
	char *p = str_sprintf2("%s%s", get_homedir(), name);
#else
	char *p = str_sprintf2("%s" PATH_SEP_STR "%s", get_homedir(), name);
#endif
	path_conv (p);
	return p;
}

#ifndef HAVE_SNPRINTF
/* Not a viable snprintf implementation, but makes code more clear */
int mik_snprintf(char *buffer, size_t n, const char *format, ...)
{
	va_list args;
	int len;

	va_start(args, format);
	len = VSNPRINTF(buffer, n, format, args);
	va_end(args);
	if (len < 0) len = (int)n;
	if ((size_t)len >= n) buffer[n-1] = '\0';
	return len;
}
#endif

unsigned long Time1000(void)
{
#ifdef _WIN32
	static __int64 Freq = 0;
	static __int64 LastCount = 0;
	static __int64 LastRest = 0;
	static long LastTime = 0;
	__int64 Count, Delta;

	/* Freq was set to -1, if the current hardware does not support high
	   resolution timers. We will use GetTickCount instead then. */
	if (Freq < 0)
		return GetTickCount();

	/* Freq is 0 the first time this function is being called. */
	if (!Freq)
		/* try to determine the frequency of the high resulution timer */
		if (!QueryPerformanceFrequency((LARGE_INTEGER *) & Freq)) {
			/* There is no such timer... */
			Freq = -1;
			return GetTickCount();
		}

	/* retrieve current count */
	Count = 0;
	QueryPerformanceCounter((LARGE_INTEGER *) & Count);

	/* calculate the time passed since last call, and add the rest of those
	   tics that didn't make it into the last reported time. */
	Delta = 1000 * (Count - LastCount) + LastRest;

	LastTime += (long)(Delta / Freq);	/* save the new value */
	LastRest = Delta % Freq;	/* save those ticks not being counted */
	LastCount = Count;			/* save last count */

	return LastTime;
#elif defined(__OS2__) || defined(__EMX__)
	static int first = 1;
	static ULONG Freq;
	static long long LastCount = 0;
	static long long LastRest = 0;
	static long LastTime = 0;
	long long Delta, Count;

	if (first) {
		first = 0;
		DosTmrQueryFreq(&Freq);
	}

	DosTmrQueryTime((QWORD *) & Count);
	Delta = 1000 * (Count - LastCount) + LastRest;
	LastTime += (long)(Delta / Freq);
	LastRest = Delta % Freq;
	LastCount = Count;
	return LastTime;
#else
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

#if defined(_WIN32)&&!defined(__MINGW32__)&&!defined(__WATCOMC__)
DIR* opendir (const char* dirName)
{
	struct stat statbuf;
	DIR* dir;

	if (stat(dirName,&statbuf) || !S_ISDIR(statbuf.st_mode))
		return NULL;

	dir = (DIR*)malloc(sizeof(DIR));

	strcpy (dir->name, dirName);
	if (dir->name[strlen(dir->name)-1] != PATH_SEP_SYS &&
		dir->name[strlen(dir->name)-1] != PATH_SEP)
		strcat (dir->name,PATH_SEP_SYS_STR);

	strcat (dir->name, "*");
	dir->handle = INVALID_HANDLE_VALUE;
	dir->filecnt = 0;

	return dir;
}

struct dirent *readdir (DIR* dir)
{
	WIN32_FIND_DATA fileBuffer;

	if (dir->filecnt == 0) {
		dir->handle = FindFirstFile (dir->name, &fileBuffer);
		if (dir->handle == INVALID_HANDLE_VALUE)
			return NULL;
	} else if (!FindNextFile (dir->handle, &fileBuffer))
		return NULL;

	strcpy (dir->d_name, fileBuffer.cFileName);
	dir->filecnt++;

	return dir;
}

int closedir (DIR* dir)
{
	if (!FindClose(dir->handle)) {
		free (dir);
		return -1;
	}
	free (dir);
	return 0;
}
#endif /* dirent _WIN32 */

#if LIBMIKMOD_VERSION < 0x030200
static char *skip_number(char *str)
{
	while (str && *str == ' ')
		str++;
	while (str && isdigit((int)*str))
		str++;
	while (str && *str == ' ')
		str++;
	return str;
}
#endif

/* Return newly malloced version and cmdline for the driver
   with the number drvno. */
BOOL driver_get_info (int drvno, char **version, char **cmdline)
{
#if LIBMIKMOD_VERSION >= 0x030200

	struct MDRIVER *driver = MikMod_DriverByOrdinal (drvno);

	if (version) *version = NULL;
	if (cmdline) *cmdline = NULL;

	if (drvno<=0 || !driver) return 0;

	if (driver->Version && version)
		*version = strdup (driver->Version);
	if (driver->CmdLineHelp && cmdline)
		*cmdline = strdup (driver->CmdLineHelp);
	return 1;

#else

	static char *drv_cmdlineNul[] = {
		NULL, NULL};
	static char *drv_cmdline317[] = {
		"AudioFile",		"machine:t::Audio server machine (hostname:port)\n",
		"AIX Audio",		"buffer:r:12,19,15:Audio buffer log2 size\n",
		"Advanced Linux Sound",	"card:r:0,31,0:Soundcard number\n"
							"pcm:r:0,3,0:PCM device number\n"
							"buffer:r:2,16,4:Number of buffer fragments\n",
		"OS/2 DART",		NULL,
		"DirectSound",		"buffer:r:12,19,16:Audio buffer log2 size\n",
		"Enlightened sound daemon","machine:t::Audio server machine (hostname:port)\n",
		"HP-UX Audio",		"buffer:r:12,19,15:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n",
		"Macintosh Sound Manager", NULL,
		"Nosound",			NULL,
		"OS/2 MMPM/2 MCI",	"buffer:r:12,19,16:Audio buffer log2 size\n",
		"Open Sound System","buffer:r:7,17,14:Audio buffer log2 size\n"
							"count:r:2,255,16:Audio buffer count\n",
		"Piped Output",		"pipe:t::Pipe command\n",
		"Raw disk writer",	"file:t:music.raw:Output file name\n",
		"Linux sam9407",	"card:r:0,999,0:Device number (/dev/sam%d_mod)",
		"SGI Audio System",	"fragsize:r:0,99999,20000:Sound buffer fragment size\n"
							"bufsize:r:0,199999,40000:Sound buffer total size\n",
		"Standard output",	NULL,
		"OpenBSD audio",	"buffer:r:7,17,12:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n",
		"NetBSD audio",		"buffer:r:7,17,12:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n",
		"SunOS audio",		"buffer:r:7,17,12:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n",
		"Sun audio",		"buffer:r:7,17,12:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n",
		"Solaris audio",	"buffer:r:7,17,12:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n",
		"Linux Ultrasound",	NULL,
		"Wav disk writer",	"file:t:music.wav:Output file name\n",
		"Windows waveform-audio", NULL,
		NULL, NULL};
	static char *drv_cmdline318[] = {
		"OS/2 DART",		"device:r:0,8,0:Waveaudio device index to use (0 - default)\n"
							"buffer:r:12,16:Audio buffer log2 size\n"
							"count:r:2,8,2:Number of audio buffers\n",
		"OS/2 MMPM/2 MCI",	"device:r:0,8,0:Waveaudio device index to use (0 - default)\n"
							"buffer:r:12,16:Audio buffer log2 size\n",
		"OpenBSD audio",	"buffer:r:7,17,12:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n"
							"speaker:b:0:Use speaker\n",
		"NetBSD audio",		"buffer:r:7,17,12:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n"
							"speaker:b:0:Use speaker\n",
		"SunOS audio",		"buffer:r:7,17,12:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n"
							"speaker:b:0:Use speaker\n",
		"Sun audio",		"buffer:r:7,17,12:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n"
							"speaker:b:0:Use speaker\n",
		"Solaris audio",	"buffer:r:7,17,12:Audio buffer log2 size\n"
							"headphone:b:0:Use headphone\n"
							"speaker:b:0:Use speaker\n",
		NULL, NULL};
	static char *drv_cmdline319[] = {
		"DirectSound",		"buffer:r:12,19,16:Audio buffer log2 size\n"
							"globalfocus:b:0:Play if window does not have the focus\n",
		"Open Sound System","buffer:r:7,17,14:Audio buffer log2 size\n"
							"count:r:2,255,16:Audio buffer count\n"
							"card:r:0,99,0:Device number (/dev/dsp%d)\n",
		NULL, NULL};
	static char *drv_cmdline3113[] = {
	/* 3.1.13 retires alsa-0.4/0.5 driver, adds alsa-1.0.x driver
	 * and removes options */
		"Advanced Linux Sound",		NULL,
		NULL, NULL};
#define VERSION_MAX		7
	static char **drv_cmdline[VERSION_MAX] = {
		drv_cmdline317,
		drv_cmdline318,
		drv_cmdline319,
		drv_cmdlineNul,
		drv_cmdlineNul,
		drv_cmdlineNul,
		drv_cmdline3113};

	char *driver = MikMod_InfoDriver(), *pos, *start;
	char **cmd;

	if (version) *version = NULL;
	if (cmdline) *cmdline = NULL;

	for (pos = skip_number(driver); pos && *pos; pos++) {
		if (*pos == '\n') {
			drvno--;
			pos = skip_number(pos + 1);
		}
		if (drvno == 1) {
			int mm_version = (MikMod_GetVersion() & 255) - 7;

			mm_version = mm_version < 0 ?
				0 : (mm_version >= VERSION_MAX ? VERSION_MAX-1 : mm_version);

			for (; mm_version>=0; mm_version--) {
				for (cmd = drv_cmdline[mm_version]; *cmd; cmd+=2) {
					if (!strncmp (*cmd, pos, strlen(*cmd))) {
						if (version) {
							start = pos;
							while (*pos && *pos != '\n') pos++;
							*version = (char *) malloc (pos-start+1);
							strncpy (*version, start, pos-start);
							(*version)[pos-start] = '\0';
						}
#if LIBMIKMOD_VERSION >= 0x030107
						cmd++;
						if (*cmd && cmdline) *cmdline = strdup (*cmd);
#else
						if (cmdline) *cmdline = strdup ("???\n");
#endif
						free (driver);
						return 1;
					}
				}
			}
			/* unknown driver */
			if (cmdline) *cmdline = strdup ("???\n");
			break;
		}
	}
	free (driver);
	return 0;
#endif
}

/* ex:set ts=4: */
