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

  $Id: mikmod.c,v 1.3 2004/01/30 18:01:40 raph Exp $

  Module player which uses the MikMod library as the player engine.

==============================================================================*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_GETOPT_LONG_ONLY
#  include <getopt.h>
#else
#  include "mgetopt.h"
#endif
#include <ctype.h>
#ifndef _WIN32
#  include <signal.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__OS2__)||defined(__EMX__)
#  define INCL_DOS
#  define INCL_KBD
#  define INCL_DOSPROCESS
#  include <os2.h>
#endif

#if defined(__FreeBSD__)||defined(__NetBSD__)||defined(__OpenBSD__)
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  endif
#  include <sys/resource.h>
#  include <sys/types.h>
#  ifdef __FreeBSD__
#    include <sys/rtprio.h>
#  endif
#endif
#if defined(__linux)
#  ifdef BROKEN_SCHED
#    define _P __P
#  endif
#  include <sched.h>
#endif

#include <mikmod.h>

#include "player.h"
#include "mutilities.h"
#include "display.h"
#include "rcfile.h"
#include "mconfig.h"
#include "mlist.h"
#include "mlistedit.h"
#include "marchive.h"
#include "mwindow.h"
#include "mdialog.h"
#include "mplayer.h"
#include "keys.h"

#define CFG_MAXCHN		128

/* Long options definition */
static struct option options[] = {
	/* Output options */
	{"driver", required_argument, NULL, 'd'},
	{"output", required_argument, NULL, 'o'},
	{"frequency", required_argument, NULL, 'f'},
	{"interpolate", no_argument, NULL, 'i'},
	{"nointerpolate", no_argument, NULL, 1},
	{"hqmixer", no_argument, NULL, 2},
	{"nohqmixer", no_argument, NULL, 3},
	{"surround", no_argument, NULL, 4},
	{"nosurround", no_argument, NULL, 5},
	{"reverb", required_argument, NULL, 'r'},
	/* Playback options */
	{"volume", required_argument, NULL, 'v'},
	{"fadeout", no_argument, NULL, 'F'},
	{"nofadeout", no_argument, NULL, 6},
	{"loops", no_argument, NULL, 'l'},
	{"noloops", no_argument, NULL, 7},
	{"panning", no_argument, NULL, 'a'},
	{"nopanning", no_argument, NULL, 8},
	{"protracker", no_argument, NULL, 'x'},
	{"noprotracker", no_argument, NULL, 9},
	/* Loading options */
	{"directory", required_argument, NULL, 'y'},
	{"curious", no_argument, NULL, 'c'},
	{"nocurious", no_argument, NULL, 10},
	{"playmode", required_argument, NULL, 'p'},
	{"tolerant", no_argument, NULL, 't'},
	{"notolerant", no_argument, NULL, 11},
	/* Scheduling options */
	{"renice", no_argument, NULL, 's'},
	{"norenice", no_argument, NULL, 12},
	{"realtime", no_argument, NULL, 'S'},
	{"norealtime", no_argument, NULL, 12},
	/* Display options */
	{"quiet", no_argument, NULL, 'q'},
	/* Information options */
	{"information", optional_argument, NULL, 'n'},
	{"drvinfo", required_argument, NULL, 'N'},
	{"version", no_argument, NULL, 'V'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, NULL, 0}
};

static const CHAR *PRG_NAME;
PLAYLIST playlist;
CONFIG config;
MODULE *mf = NULL;				/* current module */
BOOL quiet = 0;					/* set if quiet mode is enabled */

typedef enum {
	STATE_INIT,					/* Library not initialised */
	STATE_INIT_ERROR,			/* Error during MikMod_Init() */
	STATE_ERROR,				/* Error during MikMod_Reset() */
	STATE_READY,				/* Player is ready for playing */
	STATE_PLAY					/* Playing in progess */
} PL_STATE;

static struct {
	PL_STATE state;
	BOOL quit;					/* quit was scheduled */
	BOOL listend;				/* end of playlist was reached */
	BOOL norc;					/* don't load default config file */
} status = {STATE_INIT,0,0,0};

/* playlist handling */
static int next = 0;			/* 0 or a PL_CONT_xxx code */
static int next_pl_pos = 0;		/* for PL_CONT_POS, next pos in playlist */
static int next_sng_pos = 0;	/* next pos in module */

static BOOL settime = 1;
static int uservolume = 128;

/* help text */
#define S_B(b) ((b)?"Yes":"No")
static void help(CONFIG * c)
{
	char output[4];
	char *conf_name = CF_GetFilename();

	puts(mikcopyr);

	SNPRINTF(output, 4, "%s%c", c->mode_16bit ? "16" : "8",
			 c->stereo ? 's' : 'm');
	printf("\n" "Usage: %s [option|-y dir]... [module|playlist]...\n" "\n"
		   "Output options:\n"
		   "  -d[river] n,options     Use nth driver for output (0: autodetect), default: %d\n"
		   "  -o[utput] 8m|8s|16m|16s 8/16 bit output in stereo/mono, default: %s\n"
		   "  -f[requency] nnnnn      Set mixing frequency, default: %d\n"
		   "* -i[nterpolate]          Use interpolate mixing, default: %s\n"
		   "* -hq[mixer]              Use high-quality (but slower) software mixer,\n"
		   "                          default: %s\n"
		   "* -su[rround]             Use surround mixing, default: %s\n"
		   "  -r[everb] nn            Set reverb amount (0-15), default: %d\n"
		   "Playback options:\n"
		   "  -v[olume] nn            Set volume from 0%% (silence) to 100%%, default: %d%%\n"
		   "* -F, -fa[deout]          Force volume fade at the end of module, default: %s\n"
		   "* -l[oops]                Enable in-module loops, default: %s\n"
		   "* -a, -pa[nning]          Process panning effects, default: %s\n"
		   "* -x, -pr[otracker]       Disable extended protracker effects, default: %s\n"
		   "Loading options:\n"
		   "  -y, -di[rectory] dir    Scan directory recursively for modules\n"
		   "* -c[urious]              Look for hidden patterns in module, default: %s\n"
		   "  -p[laymode] n           Playlist mode (1: loop module, 2: list multi\n"
		   "                             4: shuffle list, 8: list random), default: %d\n"
		   "* -t[olerant]             Don't halt on file access errors, default: %s\n",
			PRG_NAME, c->driver, output, c->frequency,
			S_B(c->interpolate), S_B(c->hqmixer), S_B(c->surround), c->reverb,
			c->volume, S_B(c->fade), S_B(c->loop), S_B(c->panning), 
			S_B(!c->extspd), S_B(c->curious), c->playmode, S_B(c->tolerant));
#if defined(__OS2__)||defined(__EMX__)||defined(__linux)||defined(__FreeBSD__)||defined(__NetBSD__)||defined(__OpenBSD__)
#if defined(__OS2__)||defined(__EMX__)
	printf("Scheduling options:\n");
#else
	printf("Scheduling options (need root privileges or a setuid root binary):\n");
#endif
	printf("* -s, -ren[ice]           Renice to -20 (more scheduling priority), default: %s\n", 
			(c->renice == RENICE_PRI ? "Yes" : "No" ));
#if !defined(__NetBSD__)&&!defined(__OpenBSD__)
	printf("* -S, -rea[ltime]         Get realtime priority (will hog CPU power), default: %s\n",
			(c->renice == RENICE_REAL ? "Yes" : "No" ));
		
#endif
#endif
	printf("Display options:\n"
		   "  -q[uiet]                Quiet mode, no interface, displays only errors.\n"
		   "Information options:\n"
		   "  -n, -in[formation]      List all available drivers and module loaders.\n"
		   "  -N n, -drvinfo          Print information on a specific driver.\n"
		   "  -V -ve[rsion]           Display MikMod version.\n"
		   "  -h[elp]                 Display this help screen.\n"
		   "Configuration option:\n"
		   "  -norc                   Don't parse the file '%s' on startup\n"
		   "\n"
		   "Options marked with '*' also exist in negative form (eg -nointerpolate)\n"
		   "F1 or H while playing: Display help panel.\n",
		   conf_name);

	if (conf_name)
		free(conf_name);
}

/* nice exit function */
static void exit_player(int exitcode, const char *message, ...)
{
	va_list args;

	win_exit();
	if (status.state > STATE_INIT) {
		MikMod_Exit();
		status.state = STATE_INIT;
	}
	if (message) {
		va_start(args, message);
		if (exitcode > 0)
			vfprintf(stderr, message, args);
		else if (!quiet)
			vprintf(message, args);
		va_end(args);
	}
	if (!exitcode && !status.norc) {
		if (config.save_config)
			CF_Save(&config);
		if (config.save_playlist)
			PL_SaveDefault(&playlist);
	}

	printf("\n");
	exit(exitcode);
}

#ifndef _WIN32
/* signal handlers */
static RETSIGTYPE GotoNext(int signum)
{
	next = PL_CONT_NEXT;

	signal(SIGUSR1, GotoNext);
}

static RETSIGTYPE GotoPrev(int signum)
{
	next = PL_CONT_PREV;

	signal(SIGUSR2, GotoPrev);
}

static RETSIGTYPE ExitGracefully(int signum)
{
	/* can't exit now if playing */
	if (status.state == STATE_PLAY) {
		status.quit = 1;
		signal(signum, ExitGracefully);
	} else {
		win_exit();

		if (!quiet)
			fputs((signum == SIGTERM) ? "Halted by SIGTERM\n" :
				  "Halted by SIGINT\n", stderr);

		signal(SIGINT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		exit(0);
	}
}
#endif

static void Player_SetNextModPos(int pos, int sng_pos)
{
	next_pl_pos = pos;
	next_sng_pos = sng_pos;
	next = PL_CONT_POS;
}

void Player_SetNextMod(int pos)
{
	Player_SetNextModPos(pos, 0);
}

static void Player_InitLib(void)
{
	long engineversion = MikMod_GetVersion();

	if (engineversion < LIBMIKMOD_VERSION)
		exit_player(2,
					"The current engine version (%ld.%ld.%ld) is too old.\n"
					"This programs requires at least version %ld.%ld.%ld\n",
					(engineversion >> 16) & 255, (engineversion >> 8) & 255,
					(engineversion) & 255, LIBMIKMOD_VERSION_MAJOR,
					LIBMIKMOD_VERSION_MINOR, LIBMIKMOD_REVISION);

	/* Register the loaders we want to use:  */
	MikMod_RegisterAllLoaders();

	/* Register the drivers we want to use: */
	MikMod_RegisterAllDrivers();
}

static void set_priority(CONFIG *cfg)
{
	if (cfg->renice == RENICE_PRI) {
#if defined(__FreeBSD__)||defined(__NetBSD__)||defined(__OpenBSD__)
		setpriority(PRIO_PROCESS, 0, -20);
#endif
#ifdef __linux
		nice(-20);
#endif
#if defined(__OS2__)||defined(__EMX__)
		DosSetPriority(PRTYS_PROCESSTREE, PRTYC_NOCHANGE, 20, 0);
#endif
	} else if (cfg->renice == RENICE_REAL) {
#ifdef __FreeBSD__
		struct rtprio rtp;

		rtp.type = RTP_PRIO_REALTIME;
		rtp.prio = 0;
		rtprio(RTP_SET, 0, &rtp);
#endif
#ifdef __linux
		struct sched_param sp;

		memset(&sp, 0, sizeof(struct sched_param));
		sp.sched_priority = sched_get_priority_min(SCHED_RR);
		sched_setscheduler(0, SCHED_RR, &sp);
#endif
#if defined(__OS2__)||defined(__EMX__)
		DosSetPriority(PRTYS_PROCESSTREE, PRTYC_TIMECRITICAL, 20, 0);
#endif
	}
}

static BOOL cmp_bit (int value, int mask, BOOL cmp)
{
	return (BTST(value, mask)) ? cmp : !cmp;
}
static void set_bit (UWORD *value, int mask, BOOL boolv)
{
	if (boolv)
		*value |= mask;
	else
		*value &= ~mask;
}
static void config_error (const char *err, PL_STATE state)
{
	if (quiet) {
		exit_player (1, "%s: %s.\n", err, MikMod_strerror(MikMod_errno));
	} else {
		if (win_get_panel() != DISPLAY_CONFIG)
			win_change_panel (DISPLAY_CONFIG);
		sprintf (storage, "%s:\n  %s.\nTry changing the configuration.",
				 err, MikMod_strerror(MikMod_errno));
		dlg_message_open (storage, "&Ok", 0, 1, NULL, NULL);
		status.state = state;
	}
}

void Player_SetConfig (CONFIG * cfg)
{
#if LIBMIKMOD_VERSION >= 0x030107
	static char *driveroptions = NULL;
#endif
	BOOL restart =
		MP_Active() &&
		( (cfg->frequency != md_mixfreq) ||
		  ((cfg->driver) && (cfg->driver != md_device)) ||
		  (!cmp_bit(md_mode, DMODE_16BITS, cfg->mode_16bit)) ||
		  (!cmp_bit(md_mode, DMODE_STEREO, cfg->stereo)) ||
		  (!cmp_bit(md_mode, DMODE_HQMIXER, cfg->hqmixer))
#if LIBMIKMOD_VERSION >= 0x030107
		  || ( (!driveroptions && cfg->driveroptions) ||
			   (driveroptions &&
				strcmp(driveroptions, cfg->driveroptions)))
#endif
	   );
	PL_STATE oldstate = status.state;

	if (status.state <= STATE_ERROR)
		status.state = STATE_READY;

#if LIBMIKMOD_VERSION >= 0x030107
	if (driveroptions)
		free(driveroptions);
	driveroptions = strdup(cfg->driveroptions);
#endif

	md_pansep = 128;			/* panning separation (0=mono 128=full stereo) */
	md_volume = (cfg->volume * 128) / 100;
	md_reverb = cfg->reverb;

	md_device = cfg->driver;
	md_mixfreq = cfg->frequency;
	md_mode |= DMODE_SOFT_MUSIC;

	set_bit (&md_mode, DMODE_INTERP, cfg->interpolate);
	set_bit (&md_mode, DMODE_HQMIXER, cfg->hqmixer);
	set_bit (&md_mode, DMODE_SURROUND, cfg->surround);
	set_bit (&md_mode, DMODE_16BITS, cfg->mode_16bit);
	set_bit (&md_mode, DMODE_STEREO, cfg->stereo);

	if (!win_has_colors() && cfg->themes[cfg->theme].color)
		cfg->theme = THEME_MONO;
	win_set_theme (&cfg->themes[cfg->theme]);

	if (restart || oldstate == STATE_ERROR) {
		int cur = PL_GetCurrentPos(&playlist), pos = 0;

		if (cur >= 0) {
			if (mf) pos = mf->sngpos;
			Player_SetNextModPos(cur, pos);
		}
		if (mf) MP_End();
#if LIBMIKMOD_VERSION >= 0x030107
		if (MikMod_Reset(cfg->driveroptions))
#else
		if (MikMod_Reset())
#endif
			config_error ("MikMod reset error", STATE_ERROR);
		cfg->frequency = md_mixfreq;
	} else
		win_panel_repaint();
	win_init_status(cfg->statusbar);

	if (mf)
		mf->wrap = (BTST(config.playmode, PM_MODULE) ? 1 : 0);
	if (oldstate == STATE_INIT || oldstate == STATE_INIT_ERROR)
#if LIBMIKMOD_VERSION >= 0x030107
		if (MikMod_Init(config.driveroptions))
#else
		if (MikMod_Init())
#endif
			config_error ("MikMod initialisation error", STATE_INIT_ERROR);
}

/* Display the error when loading a file, and take the appropriate resume
   action */
static void handle_ListError(BOOL tolerant, const CHAR *filename, const CHAR *archive,
							 BOOL mm_error)
{
	char buf[PATH_MAX + 40] = "";

	if (!tolerant) {
		if (mm_error)
			SNPRINTF(buf, PATH_MAX + 40, "(reason: %s)\n",
					 MikMod_strerror(MikMod_errno));
		if (!filename)
			exit_player(1, "Corrupted playlist, filename is NULL.\n%s", buf);
		else if (archive)
			exit_player(1,
						"MikMod error: can't load \"%s\" from archive \"%s\".\n%s",
						filename, archive, buf);
		else
			exit_player(1, "MikMod error: can't load %s\n%s", filename, buf);
	} else {
		if (filename)
			SNPRINTF(buf, PATH_MAX + 40, "Error loading list entry \"%s\" !",
					 filename);
		else
			SNPRINTF(buf, PATH_MAX + 40, "Error loading list entry !");
		display_message(buf);
		PL_DelEntry(&playlist, PL_GetCurrentPos(&playlist));
	}
}

/* parse an integer argument */
static void get_int(const char *arg, int *value, int min, int max)
{
	char *end = NULL;
	int t = min - 1;

	if (arg)
		t = strtol(arg, &end, 10);
	if (end && (!*end) && (t >= min) && (t <= max))
		*value = t;
	else
		exit_player(1, mikcopyr "\n\n"
					"Argument '%s' out of bounds, must be between %d and %d.\n"
					"Use '%s --help' for more information.\n",
					arg ? arg : "(not given)", min, max, PRG_NAME);
}

static void display_driver_help (int drvno)
{
#define MAX_VALUES	64
	char *version, *cmdline, *cmdend, *cur;

	driver_get_info (drvno, &version, &cmdline);
	if (!drvno || !version)
		exit_player (1, "Bad driver ordinal number: %d\n", drvno);

	printf ("Parameter list for %s:\n", version);
	free (version);
	if (!cmdline) {
		printf ("    No arguments with this driver\n");
		return;
	}

	cmdend = cmdline + strlen (cmdline);
	cur = cmdline;
	while (cur < cmdend) {
		char *tmp, *tmp2, *lineend;
		char *values [MAX_VALUES];
		int nvalues = 0;
		char valuetype;
		int i;

		lineend = strchr (cur, '\n');
		if (!lineend)
			lineend = cur + strlen (cur);
		*lineend = 0;
		if (!(tmp = strchr (cur, ':'))) break;
		*tmp++ = 0;
		valuetype = *tmp;
		if (!(tmp = strchr (tmp, ':'))) break;
		tmp++;
		if (!(tmp2 = strchr (tmp, ':'))) break;
		if (valuetype != 't') {
			while (tmp < tmp2 && nvalues < MAX_VALUES) {
				values [nvalues++] = tmp;
				tmp = strchr (tmp, ',');
				if (tmp && tmp < tmp2)
					*tmp++ = 0;
				else
					break;
			}
		} else
			values [nvalues++] = tmp;
		tmp = tmp2;
		*tmp++ = 0;
		printf ("    %s (%s): %s\n", cur,
				(valuetype == 'c') ? "choice" :
				(valuetype == 't') ? "text" :
				(valuetype == 'r') ? "range" :
				(valuetype == 'b') ? "yes/no" : "unknown",
				tmp);
		if (valuetype == 'c' || valuetype == 'r') {
			printf ("        %s:", valuetype == 'c' ? "values" : "range");
			for (i = 0; i < nvalues - 1; i++)
				printf (" %s%c", values [i],
						i < nvalues - 2 ? ',' : '\n');
		}
		printf ("        default value: %s\n", values [nvalues - 1]);
		cur = lineend + 1;
	}
	free (cmdline);
}

/* handle global keys */
static BOOL player_handle_key(MWINDOW *win, int ch)
{
	BOOL handled = 1;

	if (ch < 256 && isalpha(ch))
		ch = toupper(ch);

	/* always enabled commands */
	switch (ch) {
		case ' ':			/* toggle pause */
			MP_TogglePause();
			win_panel_repaint();
			break;
		case 'N':
			next = PL_CONT_NEXT;
			break;
		case 'P':
			next = PL_CONT_PREV;
			break;
		case 'Q':
			status.quit = 1;
			break;
		case CTRL_L:
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__DJGPP__)&&!defined(_WIN32)
		case KEY_CLEAR:
#endif
			win_panel_repaint_force();
			break;
		case 'H':
			win_change_panel(DISPLAY_HELP);
			break;
		case 'S':
			win_change_panel(DISPLAY_SAMPLE);
			break;
		case 'I':
			win_change_panel(DISPLAY_INST);
			break;
		case 'M':
			win_change_panel(DISPLAY_MESSAGE);
			break;
		case 'L':
			win_change_panel(DISPLAY_LIST);
			break;
		case 'C':
			win_change_panel(DISPLAY_CONFIG);
			break;
#if LIBMIKMOD_VERSION >= 0x030200
		case 'V':
			win_change_panel(DISPLAY_VOLBARS);
			break;
		case 'F':
			config.fakevolbars = 1 - config.fakevolbars;
			break;
#endif
		default:
			handled = 0;
	}
	/* commands which only work when module is not paused */
	if (!MP_Paused()) {
		handled = 1;
		switch (ch) {
			case '+':
			case KEY_RIGHT:
				Player_NextPosition();
				settime = 0;
				break;
			case '-':
			case KEY_LEFT:
				Player_PrevPosition();
				settime = 0;
				break;
			case 'R':
				Player_SetPosition(0);
				settime = 1;
				break;
			case '(':
				if (mf)
					Player_SetSpeed(mf->sngspd - 1);
				settime = 0;
				break;
			case ')':
				if (mf)
					Player_SetSpeed(mf->sngspd + 1);
				settime = 0;
				break;
			case '{':
				if (mf)
					Player_SetTempo(mf->bpm - 1);
				settime = 0;
				break;
			case '}':
				if (mf)
					Player_SetTempo(mf->bpm + 1);
				settime = 0;
				break;
			case ';':
			case ':':
				md_mode ^= DMODE_INTERP;
				display_header();
				break;
			case 'U':
				md_mode ^= DMODE_SURROUND;
				display_header();
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				Player_SetVolume(uservolume =
								 ((ch - '0') << 7) / 10);
				break;
			case '0':
				Player_SetVolume(uservolume = 128);
				break;
			case '<':
				if (mf && mf->volume)
					Player_SetVolume(uservolume = mf->volume - 1);
				break;
			case '>':
				if (mf && mf->volume < 128)
					Player_SetVolume(uservolume = mf->volume + 1);
				break;
			default:
				handled = 0;
		}
	}
	return handled;
}

static void player_quit(void)
{
	if (status.quit)
		exit_player(0,NULL);
	else if (!status.listend)
		exit_player(1, "MikMod error: %s\n",
					MikMod_strerror(MikMod_errno));
	else
		exit_player(0,"Finished playlist...");
}

static BOOL player_timeout (MWINDOW *win, void *data)
{
	char *filename, *archive;

	/* exit if quit was scheduled */
	if (status.quit) {
		if (status.state == STATE_PLAY) {
			MP_End();
			Player_Stop();
			Player_Free(mf);
			status.state = STATE_READY;
		}
		mf = NULL;
		player_quit();
	}

	if (status.state >= STATE_READY &&
		(!MP_Active() || next || PL_CurrentDeleted(&playlist)) &&
		(!status.listend || (PL_GetLength(&playlist) > 0))) {

		/* stop playing */
		if (status.state == STATE_PLAY) {
			MP_End();
			if (!BTST(config.playmode, PM_MODULE) && !next && settime)
				PL_SetTimeCurrent(&playlist, mf->sngtime);
			PL_SetPlayedCurrent(&playlist);

			Player_Stop();
			Player_Free(mf);
			status.state = STATE_READY;
		}
		mf = NULL;

		filename = archive = NULL;
		switch (next) {
			case 0:
			case PL_CONT_NEXT:
				status.listend = !PL_ContNext(&playlist, &filename, &archive,
											  config.playmode);
				break;
			case PL_CONT_PREV:
				status.listend = !PL_ContPrev(&playlist, &filename, &archive);
				break;
			case PL_CONT_POS:
				status.listend = !PL_ContPos(&playlist, &filename, &archive,
											 next_pl_pos);
				break;
		}
		next = 0;
		settime = 1;
		if (status.listend && (PL_GetLength(&playlist) > 0 || quiet))
			player_quit();

		if (!status.listend) {
			int playfd;
			FILE *playfile = NULL;
			char *playname;

			if (!filename) {
				handle_ListError(config.tolerant, filename, archive, 0);
				return 1;
			}

			/* load the module */
			playfd = MA_dearchive(archive, filename, &playname);
			if (playfd >= 0) playfile = fdopen (playfd, "rb");
			if (playfd < 0 || !playfile) {
				handle_ListError(config.tolerant, filename, archive, 0);
				return 1;
			}
			display_loadbanner();
			mf = Player_LoadFP(playfile, CFG_MAXCHN, config.curious);
			fclose (playfile);
			if (playname) {
				unlink (path_conv_sys(playname));
				free (playname);
			}
			if (!mf) {
				handle_ListError(config.tolerant, filename, archive, 1);
				return 1;
			}

			/* start playing */
			mf->extspd = config.extspd;
			mf->panflag = config.panning;
			mf->wrap = (BTST(config.playmode, PM_MODULE) ? 1 : 0);
			mf->loop = config.loop;
			mf->fadeout = config.fade;
			Player_Start(mf);
			if (mf->volume > uservolume)
				Player_SetVolume(uservolume);
			if (next_sng_pos > 0) {
				Player_SetPosition(next_sng_pos);
				settime = 0;
				next_sng_pos = 0;
			}
			MP_Start();
			status.state = STATE_PLAY;
		}
		display_start();
	}

	MP_Update();
	if (config.volrestrict && mf)
		if (mf->volume > uservolume)
			MP_Volume(uservolume);

	/* update the status display... */
	display_status();
	win_refresh();
	return 1;
}

int main(int argc, char *argv[])
{
	int t;
	BOOL use_threads = 0;
	char *pos = NULL;
	long engineversion = MikMod_GetVersion();

#ifdef __EMX__
	_wildcard(&argc, &argv);
#endif

	/* Find program name without path component */
	pos = FIND_LAST_DIRSEP(argv[0]);
	PRG_NAME = (pos)? pos + 1 : argv[0];

	for (t = 0; t < argc; t++)
		if ((!strcmp(argv[t], "-norc")) || (!strcmp(argv[t], "--norc"))) {
			status.norc = 1;
			argv[t][0] = 0;
			break;
		}

	/* Read configuration */
	CF_Init(&config);
	if (!status.norc)
		CF_Load(&config);

	/* Initialize libmikmod */
	Player_InitLib();

	/* Setup playlist */
	PL_InitList(&playlist);

	/* Parse commandline */
	opterr = 0;
	while ((t = getopt_long_only(argc, argv,
								 "d:o:f:r:v:y:p:iFlaxctsSqn::N:Vh",
								 options, NULL)) != -1) {
		switch (t) {
		  case 'd':				/* -d --driver */
#if LIBMIKMOD_VERSION >= 0x030107
			if (strlen(optarg) > 2) {
				char *opts = strchr(optarg, ',');

				if (opts) {
					*opts = 0;

					/* numeric driver specification ? */
					if (opts - optarg <= 2)
						get_int(optarg, &config.driver, 0, 999);
					else
						config.driver = MikMod_DriverFromAlias(optarg);

					rc_set_string(&config.driveroptions, ++opts, 99);
				} else
					config.driver = MikMod_DriverFromAlias(optarg);
			} else
#endif
				get_int(optarg, &config.driver, 0, 999);
			break;
		  case 'o':				/* -o --output */
			for (pos = optarg; pos && *pos; pos++)
				switch (toupper((int)*pos)) {
				  case '1':
				  case '6':
					config.mode_16bit = 1;
					break;
				  case '8':
					config.mode_16bit = 0;
					break;
				  case 'S':
					config.stereo = 1;
					break;
				  case 'M':
					config.stereo = 0;
					break;
				}
			break;
		  case 'f':				/* -f --frequency */
			get_int(optarg, &config.frequency, 4000, 60000);
			break;
		  case 'i':				/* -i --interpolate */
			config.interpolate = 1;
			break;
		  case 1:				/* --nointerpolate */
			config.interpolate = 0;
			break;
		  case 2:				/* --hqmixer */
			config.hqmixer = 1;
			break;
		  case 3:				/* --nohqmixer */
			config.hqmixer = 0;
			break;
		  case 4:				/* --surround */
			config.surround = 1;
			break;
		  case 5:				/* --nosurround */
			config.surround = 0;
			break;
		  case 'r':				/* -r --reverb */
			get_int(optarg, &config.reverb, 0, 15);
			break;
		  case 'v':				/* -v --volume */
			get_int(optarg, &config.volume, 0, 100);
			break;
		  case 'F':				/* -F --fadeout */
			config.fade = 1;
			break;
		  case 6:				/* --nofadeout */
			config.fade = 0;
			break;
		  case 'l':				/* -l --loops */
			config.loop = 1;
			break;
		  case 7:				/* --noloops */
			config.loop = 0;
			break;
		  case 'a':				/* -a --panning */
			config.panning = 1;
			break;
		  case 8:				/* --nopanning */
			config.panning = 0;
			break;
		  case 'x':				/* -x --protracker */
			config.extspd = 0;
			break;
		  case 9:				/* --noprotracker */
			config.extspd = 1;
			break;
		  case 'y':				/* -y --directory */
			path_conv(optarg);
			list_scan_dir (optarg,quiet);
			break;
		  case 'c':				/* -c --curious */
			config.curious = 1;
			break;
		  case 10:				/* --nocurious */
			config.curious = 0;
			break;
		  case 'p':				/* -p --playmode */
			get_int(optarg, &config.playmode, 0,
					PM_MODULE | PM_MULTI | PM_SHUFFLE | PM_RANDOM);
			break;
		  case 't':				/* -t --tolerant */
			config.tolerant = 1;
			break;
		  case 11:				/* --notolerant */
			config.tolerant = 0;
			break;
		  case 's':				/* -s --renice */
			config.renice = RENICE_PRI;
			break;
		  case 'S':				/* -S --realtime */
			config.renice = RENICE_REAL;
			break;
		  case 12:				/* --norenice --norealtime */
			config.renice = RENICE_NONE;
			break;
		  case 'q':				/* -q --quiet */
			quiet = 1;
			break;
		  case 'n':				/* -n --information */
			if (optarg) {
				int drvno;
				get_int(optarg, &drvno, 1, 99);
				puts(mikcopyr);
				display_driver_help(drvno);
			} else {
				puts(mikcopyr);
				printf("Sound engine version %ld.%ld.%ld\n",
					   (engineversion >> 16) & 255, (engineversion >> 8) & 255,
					   (engineversion) & 255);
				printf("\nAvailable drivers are :\n%s\n"
					   "\nRecognized module formats are :\n%s\n",
					   MikMod_InfoDriver(), MikMod_InfoLoader());
			}
			exit(0);
 		  case 'N':
			{
				int drvno;
				get_int(optarg, &drvno, 1, 99);
				puts(mikcopyr);
				display_driver_help(drvno);
				exit(0);
			}
		  case 'V':				/* --version */
			puts(mikcopyr);
			printf("Sound engine version %ld.%ld.%ld\n",
				   (engineversion >> 16) & 255, (engineversion >> 8) & 255,
				   (engineversion) & 255);
			exit(0);
		  case 'h':				/* -h --help */
			help(&config);
			exit(0);
		  default:
			/* ignore errors */
			break;
		}
	}
	set_priority(&config);

	/* Add remaining parameters to the playlist */
	for (t = optind; t < argc; t++) {
		if (!quiet) {
			printf("\rScanning files... %c (%d left) ", ("/-\\|")[t & 3],
				   argc - t);
			fflush(stdout);
		}
		path_conv(argv[t]);
		MA_FindFiles(&playlist, argv[t]);
	}

	if (!PL_GetLength(&playlist) && !status.norc)
		PL_LoadDefault(&playlist);

	PL_DelDouble(&playlist);
	if (BTST(config.playmode, PM_SHUFFLE))
		PL_Randomize(&playlist);
	PL_InitCurrent(&playlist);

	if (!quiet)
		puts(mikbanner);

	/* initialize interface */
	win_init(quiet);
	display_init();

	Player_SetConfig(&config);
	use_threads = MP_Init();

#ifndef _WIN32
	signal(SIGTERM, ExitGracefully);
	signal(SIGINT, ExitGracefully);
#if defined(__linux)
	if (!use_threads)
#endif
	{
		signal(SIGUSR1, GotoNext);
		signal(SIGUSR2, GotoPrev);
	}
#endif

	if (!quiet)
		win_panel_set_handle_key(DISPLAY_ROOT, player_handle_key);
	win_timeout_add (5, player_timeout, NULL);

	win_run();
	return 0;	/* never reached */
}

/* ex:set ts=4: */
