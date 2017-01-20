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

  $Id: marchive.c,v 1.2 2004/02/01 16:31:16 raph Exp $

  Archive support

  These routines are used to detect different archive/compression formats
  and decompress/de-archive the mods from them if necessary.

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifndef HAVE_FNMATCH_H
#include "mfnmatch.h"
#else
#include <fnmatch.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#if !defined(S_IREAD) && defined(S_IRUSR)
#define S_IREAD  S_IRUSR
#endif
#if !defined(S_IWRITE) && defined(S_IWUSR)
#define S_IWRITE S_IWUSR
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
#include <pwd.h>
#include <signal.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef WIFEXITED
#define WIFEXITED(x) (((x) & 255) == 0)
#endif
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include <mikmod.h>

#include "mlist.h"
#include "marchive.h"
#include "mconfig.h"
#include "mutilities.h"
#include "display.h"

/* module filenames patterns */
static const CHAR *modulepatterns[] = {
	"*.669",
	"*.[Aa][Mm][Ff]",
	"*.[Aa][Pp][Uu][Nn]",
	"*.[Dd][Ss][Mm]",
	"*.[Ff][Aa][Rr]",
	"*.[Gg][Dd][Mm]",
	"*.[Ii][Mm][Ff]",
	"*.[Ii][Tt]",
	"*.[Mm][Ee][Dd]",
	"*.[Mm][Oo][Dd]",
	"*.[Mm][Tt][Mm]",
	"*.[Nn][Ss][Tt]",			/* noisetracker */
	"*.[Ss]3[Mm]",
	"*.[Ss][Tt][Mm]",
	"*.[Ss][Tt][Xx]",
	"*.[Uu][Ll][Tt]",
#if LIBMIKMOD_VERSION >= 0x030303
	"*.[Uu][Mm][Xx]",			/* unreal umx container */
#endif
	"*.[Uu][Nn][Ii]",
	"*.[Xx][Mm]",
	NULL
};

static const CHAR *prefixmodulepatterns[] = {
	"[Mm][Ee][Dd].*",
	"[Mm][Oo][Dd].*",
	"[Nn][Ss][Tt].*",
	"[Xx][Mm].*",				/* found on Aminet */
	NULL
};

#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)&&!defined(_mikmod_amiga)
/* Drop all root privileges we might have. */
BOOL DropPrivileges(void)
{
	if (!geteuid()) {
		if (getuid()) {
			/* we are setuid root -> drop setuid to become the real user */
			if (setuid(getuid()))
				return 1;
		} else {
			/* we are run as root -> drop all and become user 'nobody' */
			struct passwd *nobody;
			int uid;

			if (!(nobody = getpwnam("nobody")))
				return 1;		/* no such user ? */
			uid = nobody->pw_uid;
			if (!uid)			/* user 'nobody' has root privileges ? weird... */
				return 1;
			if (setuid(uid))
				return 1;
		}
	}
	return 0;
}
#endif

/* Determines if a filename matches a module filename pattern */
static BOOL MA_isModuleFilename(const CHAR *filename)
{
	int t = 0;

	while (modulepatterns[t])
		if (!fnmatch(modulepatterns[t++], filename, FNM_NOESCAPE))
			return 1;
	return 0;
}

/* The same, but also checks for prefix names */
static BOOL MA_isModuleFilename2(const CHAR *filename)
{
	int t = 0;

	if (MA_isModuleFilename(filename))
		return 1;
	else
		while (prefixmodulepatterns[t])
			if (!fnmatch(prefixmodulepatterns[t++], filename, FNM_NOESCAPE))
				return 1;
	return 0;
}

/* Determines if a filename extension matches an archive filename extension
   pattern */
static BOOL MA_MatchExtension(const CHAR *archive, const CHAR *ends)
{
	const CHAR *pos = ends;
	int nr, arch_nr;

	do {
		while (*pos && *pos != ' ')
			pos++;
		nr = pos - ends;
		arch_nr = strlen(archive);
		while (nr > 0 && arch_nr > 0 &&
			   toupper((int)archive[arch_nr - 1]) == *(ends + nr - 1))
			nr--, arch_nr--;
		if (nr <= 0)
			return 1;

		pos++;
		ends = pos;
	} while (*(pos - 1));

	return 0;
}

/* Tests if 'filename' has the signature 'header-string' at offset
   'header_location' */
static int MA_identify(const CHAR *filename, int header_location,
					const CHAR *header_string)
{
	int len = MIN(strlen(header_string), 255);

	if (!len) return 0;

	if (header_location < 0) {
		/* check extension of file rather than signature */

		return MA_MatchExtension(filename, header_string);
	} else {
		/* check in-file signature */
		FILE *fp;
		CHAR id[255+1];

		if (!(fp = fopen(path_conv_sys(filename), "rb")))
			return 0;

		fseek(fp, header_location, SEEK_SET);

		if (!fread(id, len, 1, fp)) {
			fclose(fp);
			return 0;
		}
		if (!memcmp(id, header_string, len)) {
			fclose(fp);
			return 1;
		}
		fclose(fp);
	}
	return 0;
}

#if defined(__DJGPP__)

#include <dpmi.h>
#include <go32.h>

static BOOL filename2short (const char *l, char *s, int len_s)
{
	__dpmi_regs r;

	r.x.ax = 0x7160;
	r.h.cl = 1;					/* 2 for short -> long conversion */
	r.h.ch = 0x80;
	dosmemput (l, strlen(l)+1,
			   _go32_info_block.linear_address_of_transfer_buffer);
	r.x.si = _go32_info_block.linear_address_of_transfer_buffer & 0x0f;
	r.x.ds = _go32_info_block.linear_address_of_transfer_buffer >> 4;
	r.x.di = (_go32_info_block.linear_address_of_transfer_buffer+512) & 0x0f;
	r.x.es = (_go32_info_block.linear_address_of_transfer_buffer+512) >> 4;

	__dpmi_int (0x21, &r);

	if (r.x.flags & 1) {		/* is carry flag set (-> error) ?  */
		strncpy (s, l, len_s);
		s[len_s - 1] = '\0';
		return 0;
	} else {
		dosmemget (_go32_info_block.linear_address_of_transfer_buffer+512,
				   len_s, s);
		s[len_s - 1] = '\0';
		return 1;
	}
}

#elif defined(_WIN32)

static BOOL filename2short (const char *l, char *s, int len_s)
{
	int copied = GetShortPathName (l, s, len_s);
	if (copied == 0 || copied >= len_s) {
		strncpy (s, l, len_s);
		s[len_s - 1] = '\0';
		return 0;
	} else
		return 1;
}

#else

static BOOL filename2short (const char *l, char *s, int len_s)
{
	strncpy (s, l, len_s);
	s[len_s - 1] = '\0';
	return 1;
}

#endif

/* Copy pattern, replace in the copy %A with arc, %a with a short version
   of arc, %f with file, and %d with dest, and return the copy. */
static char* get_command (const char *pattern, const char *arc, const char *file, const char *dest)
{
	int i = 0, len = 0;
	const char *arg[3];
	char *pos, *pat, *command;
	char buf[PATH_MAX];

	pat = strdup (pattern);
	len = strlen(pattern) + 1;

	for (pos=pat; i<3 && *pos; pos++) {
		if (*pos == '%' &&
			(*(pos+1) == 'A' || *(pos+1) == 'a' || *(pos+1) == 'f' || *(pos+1) == 'd')) {
			switch (*(pos+1)) {
				case 'A':
					arg[i] = arc;
					break;
				case 'a':
					filename2short (arc, buf, PATH_MAX);
					arg[i] = buf;
					break;
				case 'f':
					arg[i] = file;
					break;
				case 'd':
					arg[i] = dest;
					break;
			}
			*(pos+1) = 's';
			len += strlen(arg[i]);
			i++;
		}
	}
	command = (char *) malloc (len*sizeof(char));
	SNPRINTF (command,len,pat,arg[0],arg[1],arg[2]);
	free (pat);
	return command;
}

#if !(defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)||defined(_mikmod_amiga))
/* Split command in single arguments by inserting '\0' in command and
   store them in argv. Size of argv: sizeargv */
static void split_command (char *command, char **argv, int sizeargv)
{
	char *pos = command;
	int i = 0;

	while (1) {
		if (!*pos || i >= sizeargv-1) {
			argv[i] = NULL;
			return;
		}
		if (isspace((int)*pos)) {
			*pos = '\0';
			pos++;
			while (isspace((int)*pos))
				pos++;
		}
		if (*pos == '"') {
			*pos++ = '\0';
			argv[i++] = pos;
			while (*pos != '"' && *pos)
				pos++;
			if (*pos)
				*pos++ = '\0';
		} else {
			argv[i++] = pos;
			while (!isspace((int)*pos) && *pos)
				pos++;
		}
	}
}
#endif

/* Create a copy of file 'fd' with the first 'start' lines and the
   last 'end' lines removed. Ignore all lines up to the first
   occurence of startpat. Unlink the copy and return a file
   descriptor to the copy. If the file could not be unlinked (e.g.
   under Windows an open file can not be unlinked), return its
   name in 'file'.*/
static int MA_truncate (int fd, const char *startpat, int start, int end, char **file)
{
#define BUFSIZE	5000
	char buf[BUFSIZE];
	const char *pos;
	int dest, cnt = -1;
	long size;
	char *fdest;

	if (file) *file = NULL;

	size = (long) lseek (fd, 0, SEEK_END);
	if (size < 0) return -1;

	dest = get_tmp_file(NULL, &fdest);
	if (dest < 0) return -1;
	if (unlink (path_conv_sys(fdest)) == 0) {
		free (fdest);
		fdest = NULL;
	}

	if (end>0 &&
		!lseek(fd, size>BUFSIZE ? -BUFSIZE:-size, SEEK_END) &&
		(cnt=read(fd, buf, sizeof(char)*BUFSIZE)) > 0) {
		pos = buf+cnt-1;
		while (end>0 && pos>=buf) {
			if (*pos == '\n') end--;
			pos--;
			size--;
		}
		if (pos>=buf && *pos == '\r')
			size--;
	}

	lseek (fd, 0, SEEK_SET);
	if (((startpat && *startpat) || start>0) &&
		(cnt=read(fd, buf, sizeof(char)*(size>BUFSIZE ? BUFSIZE:size)))
		> 0) {
		pos = NULL;
		if (startpat && *startpat) pos = strstr(buf, startpat);
		if (!pos) pos = buf;
		while (start>0 && pos-buf < cnt) {
			if (*pos == '\n') start--;
			pos++;
		}
		if (pos-buf < cnt)
			write (dest, pos, sizeof(char)*(cnt-(pos-buf)));
		size -= cnt;
	}

	while (size>0) {
		cnt = read(fd, buf, sizeof(char)*(size>BUFSIZE ? BUFSIZE:size));
		write (dest, buf, sizeof(char)*cnt);
		size -= cnt;
	}

	if (file) {
		if (fdest) *file = fdest;
	} else if (fdest)
		free (fdest);

	lseek (dest, 0, SEEK_SET);
	return dest;
}

#ifdef _mikmod_amiga
#define start_redirect() do {} while (0)
#define stop_redirect() do {} while (0)
#endif

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)

static int rd_err, rd_outbak=-1, rd_errbak;
#ifdef _WIN32
static char *rd_file = NULL;
#endif

static void start_redirect (void)
{
	fflush(stdin);  /* so any buffered chars will be written out */
	fflush(stdout);
	fflush(stderr);

#ifdef _WIN32
	/* "nul" seems not to work, use a temp file instead */
	rd_err = get_tmp_file(NULL, &rd_file);
#else
	rd_err = open("nul", O_WRONLY | O_CREAT, S_IREAD | S_IWRITE);
#endif
	if (rd_err != -1) {
		rd_outbak=dup(1);
		rd_errbak=dup(2);

		dup2(rd_err,1);
		dup2(rd_err,2);

		close(rd_err);
	}
}

static void stop_redirect (void)
{
	if (rd_outbak != -1) {
		dup2(rd_outbak,1);
		dup2(rd_errbak,2);

		close(rd_outbak);
		close(rd_errbak);
		rd_outbak = -1;

#ifdef _WIN32
		if (rd_file) {
			unlink (path_conv_sys(rd_file));
			free (rd_file);
			rd_file = NULL;
		}
#endif
	}
}
#endif

/* Extracts the file 'file' from the archive 'arc'. Return a file
   descriptor to the extracted file. If the file could not be unlinked
   (e.g. under Windows an open file can not be unlinked), return its
   name in 'extracted'. */
int MA_dearchive(const CHAR *arc, const CHAR *file, CHAR **extracted)
{
	CHAR *tmp_file = NULL, tmp_file_sys[PATH_MAX+1], *command;
	int tmp_fd = -1, t;

	if (extracted) *extracted = NULL;

	/* not an archive file... */
	if (!arc || !arc[0]) {
		tmp_fd = open (path_conv_sys(file), O_RDONLY | O_BINARY, 0600);
		return tmp_fd;
	}
	tmp_file_sys[PATH_MAX] = '\0';

	for (t = 0; t<config.cnt_archiver; t++) {
		if (MA_identify(arc, config.archiver[t].location, config.archiver[t].marker)) {
			/* display "extracting" message, as this may take some time... */
			display_extractbanner();
#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)||defined(_mikmod_amiga)
			/* extracting, the non-Unix way */

			tmp_file = get_tmp_name();
			if (!tmp_file) return -1;
			strncpy (tmp_file_sys, path_conv_sys(tmp_file), PATH_MAX);

			command = get_command (config.archiver[t].extract,
								   path_conv_sys(arc),
								   path_conv_sys2(file), tmp_file_sys);
			if (!command || !*command) return -1;

			start_redirect();
			system(command);
			stop_redirect();
			free (command);

			tmp_fd = open (tmp_file_sys, O_RDONLY | O_BINARY, 0600);
			if (tmp_fd >= 0) {
				if (unlink(tmp_file_sys) == 0) {
					free (tmp_file);
					tmp_file = NULL;
				}
			}
#else
			/* extracting, the Unix way */
			{
				pid_t pid;
				int status;
				char *argv[20];

				tmp_fd = get_tmp_file (NULL, &tmp_file);
				if (tmp_fd < 0) return -1;
				strncpy (tmp_file_sys, path_conv_sys(tmp_file), PATH_MAX);
				unlink (tmp_file_sys);
				free (tmp_file);
				tmp_file = NULL;

				switch (pid = fork()) {
				  case -1:		/* fork failed */
					close (tmp_fd);
					return -1;
					break;
				  case 0:		/* fork succeeded, child process code */
					/* if we have root privileges, drop them */
					if (DropPrivileges())
						exit(0);

					close(0);
					close(1);
					close(2);
					dup2(tmp_fd, 1);
					signal(SIGINT, SIG_DFL);
					signal(SIGQUIT, SIG_DFL);

					command = get_command (config.archiver[t].extract,
										   path_conv_sys(arc),
										   path_conv_sys2(file), NULL);
					if (command && *command) {
						split_command (command, argv, 20);
						execvp (argv[0], argv);
						free (command);
					}
					close(1);

					exit(0);
					break;
				  default:		/* fork succeeded, main process code */
					waitpid(pid, &status, 0);
					if (!WIFEXITED(status)) {
						close(tmp_fd);
						return -1;
					}
					break;
				}
			}
#endif
			break;
		}
	}

	if (tmp_fd >= 0) {
		lseek (tmp_fd, 0, SEEK_SET);
		if ((config.archiver[t].skippat && config.archiver[t].skippat[0]) ||
			config.archiver[t].skipstart>0 || config.archiver[t].skipend>0) {
			char *f;

			t = MA_truncate (tmp_fd, config.archiver[t].skippat, config.archiver[t].skipstart,
							 config.archiver[t].skipend, &f);
			close (tmp_fd);
			if (tmp_file) {
				unlink (tmp_file_sys);
				free (tmp_file);
			}
			tmp_file = f;
			tmp_fd = t;
		}
	}

	if (extracted) {
		if (tmp_file) *extracted = tmp_file;
	} else if (tmp_file)
		free (tmp_file);

	return tmp_fd;
}

/* Test if filename looks like a module or an archive
   playlist==1: also test against a playlist
   deep==1    : use Player_LoadTitle() for testing against a module,
		        otherwise test based on the filename */
#if LIBMIKMOD_VERSION < 0x030302
BOOL MA_TestName (char *filename, BOOL plist, BOOL deep)
#else
BOOL MA_TestName (const char *filename, BOOL plist, BOOL deep)
#endif
{
	int t;

	if (plist && PL_isPlaylistFilename(filename))
		return 1;
	if (deep) {
		char *title;
		if ((title=Player_LoadTitle(path_conv_sys(filename)))) {
#if (LIBMIKMOD_VERSION >= 0x030200) && defined(HAVE_MIKMOD_FREE)
			MikMod_free (title);
#else
			free (title);
#endif
			return 1;
		} else if (MikMod_errno != MMERR_NOT_A_MODULE)
			return 1;
	} else if (MA_isModuleFilename2(filename))
		return 1;

	/* FIXME: should only be on if deep==1 */
	for (t = 0; t<config.cnt_archiver; t++)
		if (MA_identify(filename,
						config.archiver[t].location, config.archiver[t].marker))
			return 1;
	return 0;
}

/* Examines file 'filename' to add modules to the playlist 'pl' */
void MA_FindFiles(PLAYLIST * pl, const CHAR *filename)
{
	int t, archive = -1;
	struct stat statbuf;

	if (stat(path_conv_sys(filename), &statbuf))
		return;					/* File does not exist or not enough rights */

	if ((S_ISDIR(statbuf.st_mode)) || (S_ISCHR(statbuf.st_mode)) ||
		(S_ISBLK(statbuf.st_mode)))
		return;					/* Directories and devices can't be modules */

	/* if filename looks like a playlist, load as a playlist */
	if (PL_isPlaylistFilename(filename))
		if (PL_Load(pl, filename))
			return;

	/* if filename looks like a module and not like an archive, don't try to
	   unzip the file... */
	if (MA_isModuleFilename(filename)) {
		PL_Add(pl, filename, NULL, 0, 0);
		return;
	}

	for (t = 0; t<config.cnt_archiver; t++)
		if (MA_identify (filename, config.archiver[t].location,
						 config.archiver[t].marker)) {
			archive = t;
			break;
		}

	if (archive >= 0) {
		if (config.archiver[archive].list && *config.archiver[archive].list) {
			/* multi-file archive, need to invoke list function */
			BOOL endspace = config.archiver[archive].nameoffset < 0;
			int offset = endspace ? 0:config.archiver[archive].nameoffset;
			char *string = (char *) malloc (PATH_MAX + 2 + offset);
			char *command;

#if defined(__OS2__)||defined(__EMX__)||defined(__DJGPP__)||defined(_WIN32)||defined(_mikmod_amiga)
/* Archive display, the non-Unix way */
			FILE *file;
			char *dest = NULL;

#ifdef _mikmod_amiga
			dest = get_tmp_name();
#endif
			command = get_command (config.archiver[archive].list,
								   path_conv_sys(filename), NULL, dest);
			start_redirect();
#ifdef _mikmod_amiga
			system(command);
			file = fopen(dest, "r");
#elif defined(__WATCOMC__)||(defined(_WIN32)&&!defined(__LCC__))
			file = _popen (command, "r");
#else
			file = popen (command, "r");
#endif
			stop_redirect();
			free (command);

			fgets(string, PATH_MAX + offset + 1, file);
			while (!feof(file)) {
				string[strlen(string) - 1] = 0;
				if (endspace) {
					for (t = 0; string[t]!=' ' && string[t]!='\0'; t++);
					string[t] = 0;
				}
				t = offset;
				while (isspace((int)*(string+t)))
					t++;
				if (MA_isModuleFilename2(string + t))
					PL_Add(pl, string + t, filename, 0, 0);
				fgets(string, PATH_MAX + offset + 1, file);
			}
#ifdef _mikmod_amiga
			fclose(file);
			unlink(dest);
			free(dest);
#elif defined(__WATCOMC__)||defined(_WIN32)
			_pclose(file);
#else
			pclose(file);
#endif

#else /* Archive display, the Unix way */
			int fd[2];

			if (!pipe(fd)) {
				pid_t pid;
				int status, cur, finished = 0;
				char ch;

				switch (pid = fork()) {
				  case -1:		/* fork failed */
					break;
				  case 0:		/* fork succeeded, child process code */
				  {
					  char *argv[20];

					  /* if we have root privileges, drop them */
					  if (DropPrivileges())
						  exit(0);
					  close(0); close(1); close(2);
					  dup2 (fd[1], 1);
					  dup2 (fd[1], 2);
					  signal (SIGINT, SIG_DFL);
					  signal (SIGQUIT, SIG_DFL);

					  command = get_command (config.archiver[archive].list,
											 path_conv_sys(filename), NULL, NULL);
					  split_command (command, argv, 20);
					  execvp (argv[0], argv);
					  free (command);
					  close(fd[1]);
					  exit(0);
					  break;
				  }
				  default:		/* fork succeeded, main process code */
					/* have to wait for the child to ensure the command was
					   successful and the pipe contains useful
					   information */

					/* read from the pipe */
					close(fd[1]);
					cur = 0;
					for (;;) {
						/* check if child process has finished */
						if (!finished && waitpid(pid, &status, WNOHANG)) {
							finished = 1;
							/* abnormal exit */
							if (!WIFEXITED(status)) {
								close(fd[0]);
								break;
							}
						}

						/* check for end of pipe, otherwise read char */
						if (!read(fd[0], &ch, 1) && finished)
							break;

						if (ch == '\n')
							ch = 0;
						string[cur++] = ch;
						if (cur >= PATH_MAX + offset + 1)
							cur = PATH_MAX + offset;
						if (!ch) {
							cur = 0;
							if (endspace) {
								for (t = 0; string[t]!=' ' && string[t]!='\0'; t++);
								string[t] = 0;
							}
							t = offset;
							while (isspace((int)*(string+t)))
								t++;
							if (MA_isModuleFilename2(string + t))
								PL_Add(pl, string + t, filename, 0, 0);
						}
					}
					close(fd[0]);
					break;
				}
			}
#endif
			free (string);
		} else {
			/* single-file archive, guess the name */
			const CHAR *dot, *slash;
			CHAR *spare;

			dot = strrchr(filename, '.');
			slash = FIND_LAST_DIRSEP(filename);
			if (!slash)
				slash = filename;
			else
				slash++;
			if (!dot)
				for (dot = slash; *dot; dot++);

			spare = (CHAR *) malloc((1 + dot - slash) * sizeof(CHAR));
			if (spare) {
				strncpy(spare, slash, dot - slash);
				spare[dot - slash] = 0;

				if (MA_isModuleFilename2(spare))
					PL_Add(pl, spare, filename, 0, 0);
				free(spare);
			}
		}
	} else
		PL_Add(pl, filename, NULL, 0, 0);
}

/* ex:set ts=4: */
