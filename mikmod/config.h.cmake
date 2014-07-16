/* Define if your system is AIX 3.* - might be needed for 4.* too. */
#cmakedefine AIX

/* Define if your copy of <sched.h> has a _P instead of __P (old Linux libc5).
   */
#cmakedefine BROKEN_SCHED

/* Define to 1 if `TIOCGWINSZ' requires <sys/ioctl.h>. */
#cmakedefine GWINSZ_IN_SYS_IOCTL

/* Define to 1 if you have the <curses.h> header file. */
#cmakedefine HAVE_CURSES_H

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H

/* Define to 1 if your system has a working POSIX `fnmatch' function. */
#cmakedefine HAVE_FNMATCH

/* Define to 1 if you have the <fnmatch.h> header file. */
#cmakedefine HAVE_FNMATCH_H

/* Define to 1 if you have the `getopt_long_only' function. */
#cmakedefine HAVE_GETOPT_LONG_ONLY

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H

/* Define to 1 if you have the <limits.h> header file. */
#cmakedefine HAVE_LIMITS_H

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H

/* Define if your libmikmod has MikMod_free (not found in <= 3.2.0-beta2). */
#cmakedefine HAVE_MIKMOD_FREE

/* Define to 1 if you have the `mkstemp' function. */
#cmakedefine HAVE_MKSTEMP

/* Define to 1 if you have the <ncurses/curses.h> header file. */
#cmakedefine HAVE_NCURSES_CURSES_H

/* Define to 1 if you have the <ncurses.h> header file. */
#cmakedefine HAVE_NCURSES_H

/* Define if your libncurses defines resizeterm (not found in <4.2). */
#cmakedefine HAVE_NCURSES_RESIZETERM

/* Define if your system provides POSIX.4 threads. */
#cmakedefine HAVE_PTHREAD

/* Define to 1 if you have the <sched.h> header file. */
#cmakedefine HAVE_SCHED_H

/* Define to 1 if you have the `snprintf' function. */
#cmakedefine HAVE_SNPRINTF

/* Define to 1 if you have the `srandom' function. */
#cmakedefine HAVE_SRANDOM

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H

/* Define to 1 if you have the `strerror' function. */
#cmakedefine HAVE_STRERROR

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#cmakedefine HAVE_SYS_IOCTL_H

/* Define to 1 if you have the <sys/param.h> header file. */
#cmakedefine HAVE_SYS_PARAM_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/time.h> header file. */
#cmakedefine HAVE_SYS_TIME_H

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#cmakedefine HAVE_SYS_WAIT_H

/* Define to 1 if you have the <termios.h> header file. */
#cmakedefine HAVE_TERMIOS_H

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H

/* Define to 1 if you have the `usleep' function. */
#cmakedefine HAVE_USLEEP

/* Define if your system has the prototype for usleep(3). */
#cmakedefine HAVE_USLEEP_PROTO

/* Define to 1 if you have the `vprintf' function. */
#cmakedefine HAVE_VPRINTF

/* Define to 1 if you have the `vsnprintf' function. */
#cmakedefine HAVE_VSNPRINTF

/* Define the directory for shared data. */
#cmakedefine PACKAGE_DATA_DIR "${PACKAGE_DATA_DIR}"

/* Define as the return type of signal handlers (`int' or `void'). */
#cmakedefine RETSIGTYPE ${RETSIGTYPE}

/* Define if your system is SOLARIS. */
#cmakedefine SOLARIS

/* Define if your system defines random(3) and srandom(3) in math.h instead of
   stdlib.h. */
#cmakedefine SRANDOM_IN_MATH_H

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#cmakedefine TIME_WITH_SYS_TIME

/* Version number of package */
#cmakedefine VERSION "${VERSION}"

/* Define to empty if `const' does not conform to ANSI C. */
#cmakedefine const

/* Define to `int' if <sys/types.h> does not define. */
#cmakedefine pid_t

/* Define to `unsigned int' if <sys/types.h> does not define. */
#cmakedefine size_t
