/* config.h.in.  Generated manually for DOS/DJGPP.  */

/* Define if your libmikmod has MikMod_free (not found in <= 3.2.0-beta2).  */
#define HAVE_MIKMOD_FREE 1

/* djgpp-v2.04 and newer provide snprintf() and vsnprintf().
 * djgpp-v2.05 is already released, so let's enable them by
 * default here. */
/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1
/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define if your system has a working fnmatch function.  */
#define HAVE_FNMATCH	1

/* Define if you have the mkstemp function.  */
#define HAVE_MKSTEMP	1

/* Define if your system has random(3) and srandom(3) */
#if defined(__DJGPP__)
#define HAVE_SRANDOM	1
#endif

/* Define if you have the usleep function.  */
#define HAVE_USLEEP	1

/* Define if your system has the prototype for usleep(3). */
#define HAVE_USLEEP_PROTO

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H	1

/* Define if you have the <fnmatch.h> header file.  */
#define HAVE_FNMATCH_H	1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H	1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H		1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H		1
