dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])

AC_INIT([mikmod],[3.2.8])
AC_CONFIG_AUX_DIR([autotools])
AM_INIT_AUTOMAKE([1.7 foreign])
AC_CONFIG_SRCDIR([src/mikmod.c])
AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE

AC_CANONICAL_HOST

dnl ==============================================================
dnl mikmod specific control variables and their default values.
dnl ==============================================================

mikmod_threads=yes

dnl =========================
dnl Configure script options.
dnl =========================

AC_ARG_ENABLE([threads],[AS_HELP_STRING([--enable-threads],[use an own thread for the player [default=guessed]])],
[if test "$enableval" = "yes"
then
    mikmod_threads=yes
else
    mikmod_threads=no
fi])

dnl ====================
dnl Checks for programs.
dnl ====================

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl =================================
dnl Use -Wall warning level with gcc.
dnl =================================

if test $ac_cv_prog_gcc = yes ; then
	CFLAGS="$CFLAGS -Wall"
fi

dnl ==============================================================
dnl Checks for typedefs, structures, and compiler characteristics.
dnl ==============================================================

AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T

dnl ========================
dnl Checks for header files.
dnl ========================

AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS(fcntl.h limits.h sys/ioctl.h sys/param.h sys/time.h unistd.h)
AC_CHECK_HEADERS(fnmatch.h)
AC_CHECK_HEADERS(sched.h)
AC_CHECK_HEADERS(ncurses.h curses.h ncurses/curses.h)
AC_CHECK_HEADERS(termios.h)

AC_HEADER_TIOCGWINSZ

dnl Checks for bogus Linux old libc5 sched.h
case "$host_os" in
    linux*)
	AC_MSG_CHECKING([whether sched.h is correct])
	AC_TRY_COMPILE([#include <sched.h>], [sched_yield();],
		broken_sched=no, broken_sched=yes)
	if test "$broken_sched" = "yes"
	then
		AC_DEFINE(BROKEN_SCHED, 1,
			  [Define if your copy of <sched.h> has a _P instead of __P (old Linux libc5).])
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
	fi
	;;
esac

dnl =====================
dnl Checks for libraries.
dnl =====================

dnl libmikmod
AM_PATH_LIBMIKMOD(3.1.5, , AC_MSG_ERROR([
--- ERROR: No suitable libmikmod library found.
           You need at least libmikmod 3.1.5 for this program to work.
]))
# MikMod_free() is in libmikmod-3.2.0b3 and later. The only fool-proof
# way of detecting MikMod_free() is a configury check at compile time
# or a dlsym() check at runtime, and the bad thing is 3.2.0beta1/2 were
# (still are?) in distros..
ac_save_LIBS=$LIBS
LIBS="$LIBS $LIBMIKMOD_LIBS"
AC_CHECK_LIB(mikmod, MikMod_free, AC_DEFINE(HAVE_MIKMOD_FREE, 1,
		[Define if your libmikmod has MikMod_free (not found in <= 3.2.0-beta2).]))
LIBS="$ac_save_LIBS"

dnl ncurses
case $host_os in
    mingw*|emx*|*djgpp)
	need_curses=no  ;;
    *)
	need_curses=yes ;;
esac

if test "$need_curses" = "yes" ; then
	AC_CHECK_LIB([ncurses], [initscr], [libcurses=ncurses],
			AC_CHECK_LIB([curses], [initscr], [libcurses=curses],
				AC_MSG_ERROR([--- ERROR: No curses library found.])))
	AC_CHECK_LIB([tinfo], [tgetflag], [have_tinfo=yes], [have_tinfo=no])

dnl resizeterm is an optional part of ncurses
	AC_CHECK_LIB($libcurses, resizeterm, AC_DEFINE(HAVE_NCURSES_RESIZETERM, 1,
		[Define if your libncurses defines resizeterm (not found in <4.2).]))

	ac_save_LIBS=$LIBS
	LIBS="$LIBS -l$libcurses"
	AC_MSG_CHECKING([whether curses links without libtinfo])
	AC_TRY_LINK([#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#elif defined(HAVE_CURSES_H)
#include <curses.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#include <ncurses/curses.h>
#endif],
[#ifdef ACS_ULCORNER
return ACS_ULCORNER;
#endif],
		[need_tinfo=no], [need_tinfo=yes]
	)
	if test "$need_tinfo" = "yes" ; then
		AC_MSG_RESULT(no)
		if test "$have_tinfo" = "no" ; then
			AC_MSG_ERROR([--- ERROR: libtinfo needed for ncurses, but not found.])
		else
			AC_MSG_CHECKING([whether ncurses links with libtinfo])
			LIBS="$LIBS -ltinfo"
			AC_TRY_LINK([#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#elif defined(HAVE_CURSES_H)
#include <curses.h>
#endif],
[#ifdef ACS_ULCORNER
return ACS_ULCORNER;
#endif],
				AC_MSG_RESULT(yes),
				AC_MSG_ERROR([--- ERROR: failed linking to ncurses library.])
			)
		fi
	else
		AC_MSG_RESULT(yes)
	fi
	LIBS="$ac_save_LIBS"
fi

dnl POSIX.4 threads
dnl ---------------
case "$host_os" in
  # mikmod_threads variable is for pthreads only
    mingw*|amigaos*|aros*|morphos*) mikmod_threads=no ;;
esac
if test "$mikmod_threads" = "yes"; then
	mikmod_threads=no
	dnl AC_CHECK_HEADERS(pthread.h) unreliable
	AC_CHECK_LIB([pthread], [pthread_create],
			[mikmod_threads=-lpthread],
			AC_CHECK_LIB([c_r], [pthread_attr_init],
					[mikmod_threads=-lc_r])
		)
fi

dnl =============================
dnl Checks for library functions.
dnl =============================

AC_FUNC_FNMATCH
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF

AC_CHECK_FUNCS(getopt_long_only, have_getopt_long_only=yes)
AC_CHECK_FUNCS(mkstemp srandom snprintf vsnprintf strerror)

AC_EGREP_HEADER(usleep, unistd.h, AC_DEFINE(HAVE_USLEEP_PROTO, 1,
		[Define if your system has the prototype for usleep(3).]))
AC_EGREP_HEADER(usleep, sys/unistd.h, AC_DEFINE(HAVE_USLEEP_PROTO))
AC_EGREP_HEADER(srandom, math.h, AC_DEFINE(SRANDOM_IN_MATH_H, 1,
		[Define if your system defines random(3) and srandom(3)
		 in math.h instead of stdlib.h.]))

dnl =================================
dnl Set PACKAGE_DATA_DIR in config.h.
dnl =================================

AX_RECURSIVE_EVAL(${datadir}/${PACKAGE},ax_package_data_dir)
AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR],"$ax_package_data_dir",[Define the directory for shared data.])
#AC_SUBST(PACKAGE_DATA_DIR)

dnl ================
dnl Choose settings.
dnl ================

case $host in
    *-aix*)
	AC_DEFINE(AIX, 1, [Define if your system is AIX 3.* - might be needed for 4.* too.])
    ;;
esac

if test "$mikmod_threads" != "no"; then
	AC_DEFINE(HAVE_PTHREAD, 1, [Define if your system provides POSIX.4 threads.])
	CFLAGS="$CFLAGS -D_REENTRANT"
	PLAYER_LIB="$mikmod_threads $PLAYER_LIB"
	REENTRANT="-D_REENTRANT"
fi

dnl ===================
dnl Choose extra stuff.
dnl ===================

dnl solaris usleep is not thread safe, use an alternative
dnl implementation on this system

case $host in
    *-*-solaris*)
	if test "$mikmod_threads" != "no"; then
		have_usleep=no
	else
		AC_CHECK_FUNCS(usleep, have_usleep=yes)
	fi
	AC_DEFINE(SOLARIS, 1, [Define if your system is SOLARIS.])
	;;
    *)
	AC_CHECK_FUNCS(usleep, have_usleep=yes)
	;;
esac

if test "$have_getopt_long_only" != "yes"; then
	EXTRA_OBJ="mgetopt.o mgetopt1.o $EXTRA_OBJ"
fi

dnl Yet another kluge to get the result of AC_FUNC_FNMATCH.
if test "$ac_cv_func_fnmatch_works" != "yes"; then
	EXTRA_OBJ="mfnmatch.o $EXTRA_OBJ"
fi

if test "$have_usleep" != "yes"; then
	EXTRA_OBJ="musleep.o $EXTRA_OBJ"
fi

if test "$need_curses" = "yes"; then
	PLAYER_LIB="$PLAYER_LIB -l$libcurses"
	if test "$need_tinfo" = "yes"; then
		PLAYER_LIB="$PLAYER_LIB -ltinfo"
	fi
fi

dnl =================
dnl Create Makefiles.
dnl =================

AC_SUBST(EXTRA_OBJ)
AC_SUBST(PLAYER_LIB)

AC_CONFIG_FILES([Makefile
src/Makefile])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT
