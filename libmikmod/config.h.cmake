/* config.h.in.  Generated from configure.in by autoheader.  */
/* ========== Package information */

/* Package name (libmikmod) */
#undef PACKAGE
/* Package version */
#undef VERSION

/* ========== Features selected */

/* Define if your system supports binary pipes (i.e. Unix) */
#cmakedefine DRV_PIPE 1

/* Define if you want a .aiff file writer driver */
#cmakedefine DRV_AIFF 1

/* Define if the AudioFile driver is compiled */
#cmakedefine DRV_AF 1
/* Define if the AIX audio driver is compiled */
#cmakedefine DRV_AIX 1
/* Define if the Linux ALSA driver is compiled */
#cmakedefine DRV_ALSA 1
/* Define if the Enlightened Sound Daemon driver is compiled */
#cmakedefine DRV_ESD 1
/* Define if the HP-UX audio driver is compiled */
#cmakedefine DRV_HP 1
/* Define if the Network Audio System driver is compiled */
#cmakedefine DRV_NAS 1
/* Define if the Open Sound System driver is compiled */
#cmakedefine DRV_OSS 1
/* Define if the Linux SAM9407 driver is compiled */
#undef DRV_SAM9407
/* Define if the SGI audio driver is compiled */
#cmakedefine DRV_SGI 1
/* Define if the Sun audio driver or compatible (NetBSD, OpenBSD)
   is compiled */
#cmakedefine DRV_SUN 1
/* Define if the Linux Ultra driver is compiled */
#cmakedefine DRV_ULTRA 1
/* Define this if you want the MacOS X CoreAudio driver */
#cmakedefine DRV_OSX 1
/* Define this if you want the Carbon Mac Audio driver */
#cmakedefine DRV_MAC 1

/* Define if you want a debug version of the library */
#undef MIKMOD_DEBUG
/* Define if you want runtime dynamic linking of ALSA and EsounD drivers */
#undef MIKMOD_DYNAMIC
/* Define if your system provides POSIX.4 threads */
#undef HAVE_PTHREAD

/* ========== Build environment information */

/* Define if your system is SunOS 4.* */
#undef SUNOS
/* Define if your system is AIX 3.* - might be needed for 4.* too */
#undef AIX
/* Define if your system defines random(3) and srandom(3) in math.h instead
   of stdlib.h */
#undef SRANDOM_IN_MATH_H
/* Define if EsounD driver depends on ALSA */
#undef MIKMOD_DYNAMIC_ESD_NEEDS_ALSA
/* Define if your system has RTLD_GLOBAL defined in <dlfcn.h> */
#undef HAVE_RTLD_GLOBAL
/* Define if your system needs leading underscore to function names in dlsym() calls */
#undef DLSYM_NEEDS_UNDERSCORE

/* define this if you are running a bigendian system (motorola, sparc, etc) */
#undef WORDS_BIGENDIAN

/* Define if building universal (internal helper macro) */
#undef AC_APPLE_UNIVERSAL_BUILD

/* Define to 1 if you have the <AF/AFlib.h> header file. */
#cmakedefine HAVE_AF_AFLIB_H 1

/* Define to 1 if you have the <alsa/asoundlib.h> header file. */
#cmakedefine HAVE_ALSA_ASOUNDLIB_H 1

/* Define to 1 if you have the <audio/audiolib.h> header file. */
#cmakedefine HAVE_AUDIO_AUDIOLIB_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you have the <dl.h> header file. */
#cmakedefine HAVE_DL_H 1

/* Define to 1 if you have the <dmedia/audio.h> header file. */
#cmakedefine HAVE_DMEDIA_AUDIO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <libgus.h> header file. */
#cmakedefine HAVE_LIBGUS_H 1

/* Define to 1 if you have the <machine/soundcard.h> header file. */
#cmakedefine HAVE_MACHINE_SOUNDCARD_H 1

/* Define to 1 if you have the <malloc.h> header file. */
#cmakedefine HAVE_MALLOC_H 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the `setenv' function. */
#cmakedefine HAVE_SETENV 1

/* Define to 1 if you have the `snprintf' function. */
#cmakedefine HAVE_SNPRINTF 1

/* Define to 1 if you have the `srandom' function. */
#cmakedefine HAVE_SRANDOM 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#cmakedefine HAVE_STRCASECMP 1

/* Define to 1 if you have the `strdup' function. */
#cmakedefine HAVE_STRDUP 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the `strstr' function. */
#cmakedefine HAVE_STRSTR 1

/* Define to 1 if you have the <sun/audioio.h> header file. */
#cmakedefine HAVE_SUN_AUDIOIO_H 1

/* Define to 1 if you have the <sys/acpa.h> header file. */
#cmakedefine HAVE_SYS_ACPA_H 1

/* Define to 1 if you have the <sys/audioio.h> header file. */
#cmakedefine HAVE_SYS_AUDIOIO_H 1

/* Define to 1 if you have the <sys/audio.h> header file. */
#cmakedefine HAVE_SYS_AUDIO_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#cmakedefine HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/sam9407.h> header file. */
#cmakedefine HAVE_SYS_SAM9407_H 1

/* Define to 1 if you have the <sys/soundcard.h> header file. */
#cmakedefine HAVE_SYS_SOUNDCARD_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#cmakedefine HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#undef LT_OBJDIR

/* Name of package */
#undef PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the home page for this package. */
#undef PACKAGE_URL

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* Define to 1 if you have the ANSI C header files. */
#undef STDC_HEADERS

/* Version number of package */
#undef VERSION

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#  undef WORDS_BIGENDIAN
# endif
#endif

/* Define to empty if `const' does not conform to ANSI C. */
#undef const

/* Define to `int' if <sys/types.h> does not define. */
#undef pid_t

/* Define to `unsigned int' if <sys/types.h> does not define. */
#undef size_t
