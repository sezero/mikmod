dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])

AC_INIT([libmikmod],[3.3.3])
LIBMIKMOD_MAJOR_VERSION=3
LIBMIKMOD_MINOR_VERSION=3
LIBMIKMOD_MICRO_VERSION=3
LIBMIKMOD_VERSION=$LIBMIKMOD_MAJOR_VERSION.$LIBMIKMOD_MINOR_VERSION.$LIBMIKMOD_MICRO_VERSION

AM_INIT_AUTOMAKE([1.10 foreign subdir-objects])
AC_CONFIG_SRCDIR([include/mikmod.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_LIBOBJ_DIR([posix])
AM_MAINTAINER_MODE

# Library versioning for libtool: CURRENT, REVISION, AGE
# - library source changed -> increment REVISION
# - interfaces added/removed/changed -> increment CURRENT, REVISION = 0
# - interfaces added -> increment AGE
# - interfaces removed -> AGE = 0
MIKMOD_LT_CURRENT=5
MIKMOD_LT_REVISION=0
MIKMOD_LT_AGE=2

AC_SUBST(MIKMOD_LT_CURRENT)
AC_SUBST(MIKMOD_LT_REVISION)
AC_SUBST(MIKMOD_LT_AGE)

# ===================
# build target:
# ===================

AC_CANONICAL_HOST

# these for convenience
libmikmod_mingw=no
libmikmod_cygwin=no
libmikmod_linux=no
libmikmod_darwin=no
libmikmod_netbsd=no
libmikmod_openbsd=no
libmikmod_os2=no
libmikmod_djgpp=no

case $host_os in
	mingw*) libmikmod_mingw=yes ;;
	cygwin*) libmikmod_cygwin=yes ;;
	linux*) libmikmod_linux=yes ;;
	darwin*) libmikmod_darwin=yes ;;
	netbsd*) libmikmod_netbsd=yes ;;
	openbsd*) libmikmod_openbsd=yes ;;
	emx*) libmikmod_os2=yes;;
	*djgpp) libmikmod_djgpp=yes ;;
esac

# ==============================================================
# libmikmod specific control variables and their default values.
# ==============================================================

libmikmod_driver_af=no
libmikmod_driver_aix=no
libmikmod_driver_alsa=yes
libmikmod_driver_esd=no
libmikmod_driver_nas=no
libmikmod_driver_hp=no
libmikmod_driver_sdl=no
libmikmod_driver_openal=no
libmikmod_driver_oss=yes
libmikmod_driver_sam9407=no
libmikmod_driver_sgi=no
libmikmod_driver_sun=no
libmikmod_driver_osx=yes
libmikmod_driver_mac=no
libmikmod_driver_win=yes
libmikmod_driver_ds=yes
libmikmod_driver_xaudio2=no
libmikmod_driver_ultra=no
libmikmod_driver_os2=yes
libmikmod_driver_dart=yes
libmikmod_driver_stdout=yes
libmikmod_driver_pipe=yes

libmikmod_driverlist=
libmikmod_dynload=yes
libmikmod_dynload_underscores=no
libmikmod_debug=no
libmikmod_threads=yes
libmikmod_simd=no
libmikmod_altivec=no
libmikmod_sse2=no

libmikmod_unix=default

# =========================
# Configure script options.
# =========================

AC_ARG_ENABLE([af], [AS_HELP_STRING([--enable-af],[include AudioFile audio server driver [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_af=yes
else
  libmikmod_driver_af=no
fi])

AC_ARG_ENABLE([alsa], [AS_HELP_STRING([--enable-alsa],[Linux only: include ALSA driver [guessed]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_alsa=yes
else
  libmikmod_driver_alsa=no
fi])

AC_ARG_ENABLE([esd], [AS_HELP_STRING([--enable-esd],[include EsounD (Enlightened Sound Daemon) driver [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_esd=yes
else
  libmikmod_driver_esd=no
fi])

AC_ARG_ENABLE([nas], [AS_HELP_STRING([--enable-nas],[include the Network Audio System driver [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_nas=yes
else
  libmikmod_driver_nas=no
fi])

case $host_os in
  mingw*|emx*|*djgpp)
     # windows, dos, os2 don't need sdl, stdout, or pipe.
     # so default to disabled, but still user-selectable.
     libmikmod_driver_pipe=no
     libmikmod_driver_stdout=no
     libmikmod_driver_sdl=no ;;
  cygwin*|darwin*)
     # cygwin, osx don't need sdl.
     libmikmod_driver_sdl=no ;;
  *) ;;
esac

AC_ARG_ENABLE([stdout], [AS_HELP_STRING([--enable-stdout],[include support for output to stdout [guessed]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_stdout=yes
else
  libmikmod_driver_stdout=no
fi])

AC_ARG_ENABLE([pipe], [AS_HELP_STRING([--enable-pipe],[include support for output via a pipe to another command [guessed]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_pipe=yes
else
  libmikmod_driver_pipe=no
fi])

AC_ARG_ENABLE([sdl], [AS_HELP_STRING([--enable-sdl],[include SDL driver [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_sdl=yes
else
  libmikmod_driver_sdl=no
fi])

try_sdl2=yes
AC_ARG_ENABLE([sdl2], [AS_HELP_STRING([--enable-sdl2],[try SDL2 first, then SDL-1.2 as libsdl [default=yes]])],
[if test "$enableval" = "yes"
then
  try_sdl2=yes
else
  try_sdl2=no
fi])

AC_ARG_ENABLE([openal], [AS_HELP_STRING([--enable-openal],[include OpenAL driver [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_openal=yes
else
  libmikmod_driver_openal=no
fi])

AC_ARG_ENABLE([oss], [AS_HELP_STRING([--enable-oss],[include OSS driver [guessed]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_oss=yes
else
  libmikmod_driver_oss=no
fi])

AC_ARG_ENABLE([osx], [AS_HELP_STRING([--enable-osx],[Darwin only: include OSX CoreAudio driver [guessed]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_osx=yes
else
  libmikmod_driver_osx=no
fi])

AC_ARG_ENABLE([mac], [AS_HELP_STRING([--enable-mac],[Darwin only: include MAC carbon driver [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_mac=yes
else
  libmikmod_driver_mac=no
fi])

AC_ARG_ENABLE([win], [AS_HELP_STRING([--enable-win],[Windows only: include Windows MCI driver [guessed]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_win=yes
else
  libmikmod_driver_win=no
fi])

AC_ARG_ENABLE([ds], [AS_HELP_STRING([--enable-ds],[Windows only: include Direct Sound driver [guessed]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_ds=yes
else
  libmikmod_driver_ds=no
fi])

AC_ARG_ENABLE([xaudio2], [AS_HELP_STRING([--enable-xaudio2],[Windows only: include XAudio2 driver [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_xaudio2=yes
else
  libmikmod_driver_xaudio2=no
fi])

use_xaudio28=no
AC_ARG_ENABLE([xaudio28], [AS_HELP_STRING([--enable-xaudio28],[Windows only: use XAudio2.8 for Windows8 instead of XAudio2.7 [default=no]])],
[if test "$enableval" = "yes"
then
  use_xaudio28=yes
else
  use_xaudio28=no
fi])

AC_ARG_ENABLE([os2], [AS_HELP_STRING([--enable-os2],[OS/2 only: include the OS/2 MCI driver [guessed]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_os2=yes
else
  libmikmod_driver_os2=no
fi])

AC_ARG_ENABLE([dart], [AS_HELP_STRING([--enable-dart],[OS/2 (Warp 4) only: include OS/2 direct audio (DART) [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_dart=yes
else
  libmikmod_driver_dart=no
fi])

AC_ARG_ENABLE([sam9407], [AS_HELP_STRING([--enable-sam9407],[Linux only: include sam9407 driver [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_sam9407=yes
else
  libmikmod_driver_sam9407=no
fi])

AC_ARG_ENABLE([ultra], [AS_HELP_STRING([--enable-ultra],[Linux only: include Ultra driver [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_driver_ultra=yes
else
  libmikmod_driver_ultra=no
fi])

AC_ARG_ENABLE([dl], [AS_HELP_STRING([--enable-dl],[load alsa, esound and ultra drivers at runtime [default=yes]])],
[if test "$enableval" = "yes"
then
  libmikmod_dynload=yes
else
  libmikmod_dynload=no
fi])

AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug],[build a debug version of libmikmod [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_debug=yes
else
  libmikmod_debug=no
fi])

AC_ARG_ENABLE([threads], [AS_HELP_STRING([--enable-threads],[build a thread-safe version of libmikmod [guessed]])],
[if test "$enableval" = "yes"
then
	libmikmod_threads=yes
else
	libmikmod_threads=no
fi])

AC_ARG_ENABLE([simd], [AS_HELP_STRING([--enable-simd],[include SIMD (AltiVec or SSE2) mixer optimizations (Unstable!) [default=no]])],
[if test "$enableval" = "yes"
then
  libmikmod_simd=yes
else
  libmikmod_simd=no
fi])

AC_ARG_ENABLE([unix], [AS_HELP_STRING([--enable-unix],[override the default value of MIKMOD_UNIX in mikmod_internals.h (if you *really* need to.)])],
[if test "$enableval" = "yes" ;then
  libmikmod_unix=1
elif test "$enableval" = "no" ;then
  libmikmod_unix=0
fi])

# ===================
# Check for programs.
# ===================

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

if test $libmikmod_mingw = no && test $libmikmod_darwin = no; then
# possibly needed for NAS support, but the result isn't used:
# instead of $ac_x_includes and $ac_x_libraries, /usr/X11R6/include
# and /usr/X11R6/lib are tested and used.  to be looked into..
	AC_PATH_X
fi

# Check if makeinfo support html output. If it doesn't, pretend it's missing
# rather than failing rebuilding the documentation.
AC_CACHE_CHECK([if makeinfo supports html output],libmikmod_cv_makeinfo_html,
if (makeinfo --help 2>&1 | grep html > /dev/null) 2>/dev/null
then
	libmikmod_cv_makeinfo_html=yes
else
	libmikmod_cv_makeinfo_html=no
fi)
if test $libmikmod_cv_makeinfo_html = no
then
	MAKEINFO="$missing_dir/missing makeinfo"
else
	MAKEINFO="$MAKEINFO --no-split"
fi

# ====================
# Check for libraries.
# ====================

# dl functions
# ------------
if test $libmikmod_mingw = yes; then
	# libmikmod_dynload variable is for dlopen() api only
	libmikmod_dynload=no
fi

if test $libmikmod_dynload = yes
then
	libmikmod_dynload=no
	libmikmod_dllib=
	# SunOS-style shared libraries
	AC_CHECK_HEADERS(dlfcn.h)
	if test $ac_cv_header_dlfcn_h = yes
	then
		AC_CHECK_LIB(c,dlopen,libmikmod_dynload=sun,
		             [AC_CHECK_LIB(dl,dlopen,[libmikmod_dynload=sun
		                                      libmikmod_dllib="-ldl"],libmikmod_dynload=no)])
	else
		# HP-UX-style shared libraries
		AC_CHECK_HEADERS(dl.h)
		if test $ac_cv_header_dl_h = yes
		then
			AC_CHECK_LIB(dld,shl_load,[libmikmod_dynload=hp
			                           libmikmod_dllib="-ldld"
			                           CFLAGS="${CFLAGS} -DMIKMOD_DLAPI_HP=1 -I${srcdir}/dlapi"],
			                          libmikmod_dynload=no)
		fi
	fi
fi

if test $libmikmod_dynload = sun
then
	AC_CACHE_CHECK(
		[wherever dynamically loaded symbols need underscores],
		libmikmod_cv_dynload_underscores,
		libmikmod_oldlibs=$LIBS
		LIBS="$LIBS $libmikmod_dllib"
		AC_LANG_SAVE
		AC_LANG_C
		AC_TRY_RUN([#include <dlfcn.h>
#ifndef	RTLD_GLOBAL
#define	RTLD_GLOBAL 0
#endif

int main(void)
{
	void *libc, *printfptr;
	libc = dlopen("libc.so", RTLD_LAZY | RTLD_GLOBAL);
	if (!libc)
		exit(1);
	
	printfptr = dlsym(libc, "_printf");
	dlclose(libc);
	exit(!printfptr);
}],
			libmikmod_cv_dynload_underscores=yes,
			libmikmod_cv_dynload_underscores=no,
			libmikmod_cv_dynload_underscores=no)
		AC_LANG_RESTORE
		LIBS=$libmikmod_oldlibs
	)
	if test $libmikmod_cv_dynload_underscores = yes
	then
		AC_DEFINE([DLSYM_NEEDS_UNDERSCORE], 1, [Define if your system needs leading underscore to function names in dlsym() calls])
	fi
fi

# POSIX.4 threads
# ---------------
if test $libmikmod_mingw = yes ; then
	# libmikmod_threads variable is for pthreads only
	libmikmod_threads=no
fi

if test $libmikmod_threads = yes
then
	libmikmod_threads=no
	AC_CACHE_CHECK(
		[if compiler recognizes -pthread],
		libmikmod_cv_gcc_pthread,
		ac_save_CFLAGS=$CFLAGS
		CFLAGS="$CFLAGS -pthread"
		AC_LANG_SAVE
		AC_LANG_C
		AC_TRY_LINK([#include <pthread.h>],
			[int p = pthread_create(NULL,NULL,NULL,NULL);],
			libmikmod_cv_gcc_pthread=yes,
			libmikmod_cv_gcc_pthread=no
		)
		AC_LANG_RESTORE
		CFLAGS=$ac_save_CFLAGS
	)
	if test $libmikmod_cv_gcc_pthread = yes
	then
		dnl There is no need for -pthread on darwin, and gcc
		dnl complains about unrecognized option -pthread
		if test $libmikmod_darwin = no
		then
			libmikmod_threads=""
			CFLAGS="$CFLAGS -pthread"
		fi
	else
		dnl AC_CHECK_HEADERS(pthread.h) unreliable
		AC_CHECK_LIB(pthread,pthread_create,libmikmod_threads=-lpthread,
	             [AC_CHECK_LIB(c_r,pthread_create,libmikmod_threads=-lc_r)])
	fi
fi

# ALSA - Linux only
# -----------------
if test $libmikmod_linux = yes && test $libmikmod_driver_alsa = yes
then
	libmikmod_driver_alsa=no
	AC_CHECK_HEADERS(alsa/asoundlib.h)
	if test $ac_cv_header_alsa_asoundlib_h = yes
	then
		AC_CHECK_LIB(asound,snd_pcm_open,libmikmod_driver_alsa=yes)
	fi
else
	libmikmod_driver_alsa=no
fi

# esd
# ---
if test $libmikmod_driver_esd = yes && test $libmikmod_mingw = no && test $libmikmod_darwin = no
then
	libmikmod_driver_esd=no
	AM_PATH_ESD(0.2.18,libmikmod_driver_esd=yes)
else
	libmikmod_driver_esd=no
fi

# nas
# ---
if test $libmikmod_driver_nas = yes && test $libmikmod_mingw = no && test $libmikmod_darwin = no
then
	libmikmod_driver_nas=no

	AC_CHECK_HEADERS(audio/audiolib.h)
	if test $ac_cv_header_audio_audiolib_h = yes
	then
		AC_CHECK_LIB(audio,AuCreateFlow,libmikmod_driver_nas=yes)
	fi
	if test $libmikmod_driver_nas = no
	then
		if test -r /usr/X11R6/include/audio/audiolib.h; then
			libmikmod_driver_nas=yes
			CFLAGS="$CFLAGS -I/usr/X11R6/include"
			LIBRARY_LIB="$LIBRARY_LIB -L/usr/X11R6/lib" # note: -laudio is added later
		fi
	fi
else
	libmikmod_driver_nas=no
fi

# OpenAL - all supported platforms
# -----------------
if test $libmikmod_driver_openal = yes
then
	libmikmod_driver_openal=no
	if test $libmikmod_darwin = yes
	then
		# header-only test is good enough. need 10.4.
		AC_CHECK_HEADERS(OpenAL/al.h OpenAL/alc.h)
		if test $ac_cv_header_OpenAL_al_h = yes && \
		   test $ac_cv_header_OpenAL_alc_h = yes
		then
			libmikmod_driver_openal=yes
		fi
	else
		AC_CHECK_HEADERS(AL/al.h AL/alc.h AL/alext.h,,,
				 [[#if HAVE_AL_AL_H
# include <AL/al.h>
#endif
#if HAVE_AL_ALC_H
# include <AL/alc.h>
#endif]
				 ])
		if test $ac_cv_header_AL_al_h = yes && \
		   test $ac_cv_header_AL_alc_h = yes
		then
			AC_CHECK_LIB(openal,alEnable,libmikmod_driver_openal=yes)
		fi
	fi
else
	libmikmod_driver_openal=no
fi

# SDL - all supported platforms
# -----------------
if test $libmikmod_driver_sdl = yes
then
	libmikmod_driver_sdl=no
	if test $try_sdl2 = yes
	then
		AM_PATH_SDL2("2.0.0",[libmikmod_driver_sdl=yes])
	fi
	if test $libmikmod_driver_sdl != yes
	then
		AM_PATH_SDL("1.2.0",[libmikmod_driver_sdl=yes])
	fi
	# we don't need linking to SDLmain, because we are a library.
	# we also don't need -lmingw32 for MinGW, because gcc adds it automatically.
	# -mwindows doesn't make sense for us, either, i.e. no need for extra gdi32
	# and comdlg32 linkage, nor the extra "--subsystem windows" flag.
	SDL_CFLAGS="`echo $SDL_CFLAGS|sed -e 's/-Dmain=SDL_main//'`"
	SDL_LIBS="`echo $SDL_LIBS|sed -e 's/-lSDLmain//'|sed -e 's/-lmingw32//'|sed -e 's/-mwindows//'`"
else
	libmikmod_driver_sdl=no
fi
AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)

# Windows audio
# ---------------------
case $host_os in
 mingw*|cygwin*)
	# do a windows.h check, just in case..
	AC_CHECK_HEADERS(windows.h)
	if test $ac_cv_header_windows_h != yes ; then
		libmikmod_driver_win=no
		libmikmod_driver_ds=no
		libmikmod_driver_xaudio2=no
	fi
	if test $libmikmod_driver_ds = yes
	then
		libmikmod_driver_ds=no
		AC_CHECK_HEADERS(dsound.h)
		if test $ac_cv_header_dsound_h = yes ; then
			libmikmod_driver_ds=yes
		fi
	fi
	if test $libmikmod_driver_xaudio2 = yes
	then
		libmikmod_driver_xaudio2=no
		ac_save_CPPFLAGS=$CPPFLAGS
		if test $use_xaudio28 = yes; then
			CPPFLAGS="$CPPLAGS -D_WIN32_WINNT=0x0602"
		fi
		AC_CHECK_HEADERS(xaudio2.h)
		CPPFLAGS=$ac_save_CPPFLAGS
		if test $ac_cv_header_xaudio2_h = yes ; then
			libmikmod_driver_xaudio2=yes
		fi
	fi
	;;

 *)	libmikmod_driver_win=no
	libmikmod_driver_ds=no
	libmikmod_driver_xaudio2=no
	;;
esac

# OS/2 audio
# ---------------------
case $host_os in
 emx*)
	# do an os2.h check, just in case..
	AC_CHECK_HEADERS(os2.h)
	if test $ac_cv_header_os2_h != yes ; then
		libmikmod_driver_os2=no
		libmikmod_driver_dart=no
	fi
	;;

 *)	libmikmod_driver_os2=no
	libmikmod_driver_dart=no
	;;
esac

# sgi audio - IRIX only
# ---------------------
case $host_os in
	irix*)
		AC_CHECK_HEADERS(dmedia/audio.h)
		if test $ac_cv_header_dmedia_audio_h = yes
		then
			AC_CHECK_LIB(audio,ALseterrorhandler,libmikmod_driver_sgi=yes)
		fi
	;;
esac

# audiofile
# ---------
if test $libmikmod_driver_af = yes && test $libmikmod_mingw = no && test $libmikmod_darwin = no
then
	libmikmod_driver_af=no
	AC_CHECK_HEADERS(AF/AFlib.h)
	if test $ac_cv_header_AF_AFlib_h = yes
	then
		AC_CHECK_LIB(AF,AFOpenAudioConn,libmikmod_driver_af=yes)
	fi
else
	libmikmod_driver_af=no
fi

# libgus - Linux only
# -------------------
if test $libmikmod_linux = yes && test $libmikmod_driver_ultra = yes
then
	libmikmod_driver_ultra=no
	AC_CHECK_HEADERS(libgus.h)
	if test $ac_cv_header_libgus_h = yes
	then
		AC_CHECK_LIB(gus,gus_cards,libmikmod_driver_ultra=yes)
	fi
else
	libmikmod_driver_ultra=no
fi

# sam9407 - Linux only
# --------------------
if test $libmikmod_linux = yes && test $libmikmod_driver_sam9407 = yes
then
	libmikmod_driver_sam9407=no
	AC_CHECK_HEADERS(sys/sam9407.h)
	if test $ac_cv_header_sys_sam9407_h = yes
	then
		libmikmod_driver_sam9407=yes
	fi
else
	libmikmod_driver_sam9407=no
fi

# =======================
# Check for header files.
# =======================

AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h limits.h malloc.h memory.h strings.h sys/ioctl.h unistd.h)

# =============================================================
# Check for typedefs, structures, and compiler characteristics.
# =============================================================

AC_PROG_GCC_TRADITIONAL
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_C_BIGENDIAN

AC_CACHE_CHECK([if compiler understands the signed keyword],
	libmikmod_cv_gcc_signed,
	AC_LANG_SAVE
	AC_LANG_C
	AC_TRY_COMPILE(,[signed int t = 0;],
		libmikmod_cv_gcc_signed=yes,libmikmod_cv_gcc_signed=no)
	AC_LANG_RESTORE)
if test $libmikmod_cv_gcc_signed = yes
then
	DOES_NOT_HAVE_SIGNED=""
else
	DOES_NOT_HAVE_SIGNED="#define signed"
fi

# ============================
# Check for library functions.
# ============================

AC_CHECK_FUNCS(posix_memalign setenv snprintf srandom)
AC_REPLACE_FUNCS(memcmp strcasecmp strdup strstr)
AC_CHECK_LIB([m],[floor],LIBRARY_LIB="-lm $LIBRARY_LIB")
AC_HEADER_EGREP(srandom,math.h,AC_DEFINE([SRANDOM_IN_MATH_H], 1,
					 [Define if your system defines random(3) and srandom(3) in math.h instead of stdlib.h]))

# ================
# Choose settings.
# ================

AC_DEFINE([DRV_AIFF], 1, [Define if you want a .aiff file writer driver])

if test $libmikmod_driver_pipe = yes ; then
	libmikmod_driverlist="pipe $libmikmod_driverlist"
	AC_DEFINE([DRV_PIPE], 1, [Define if your system supports binary pipes (i.e. Unix)])
fi
if test $libmikmod_driver_stdout = yes ; then
	libmikmod_driverlist="stdout $libmikmod_driverlist"
	AC_DEFINE([DRV_STDOUT], 1, [Define if you want support for output to stdout])
fi

# If we can guess the drivers needed from the system, no need to ask the user
# to specify it manually
case $host_os in
	aix*)
		AC_DEFINE([AIX], 1, [Define if your system is AIX 3.x - might be needed for 4.x too])
		AC_CHECK_HEADERS(sys/audio.h sys/acpa.h)
		if test $ac_cv_header_sys_audio_h = yes && \
		   test $ac_cv_header_sys_acpa_h = yes
		then
			libmikmod_driver_aix=yes
		fi
	;;
	hpux*)
		AC_CHECK_HEADERS(sys/audio.h)
		if test $ac_cv_header_sys_audio_h = yes
		then
			libmikmod_driver_hp=yes
		fi
	;;
	netbsd*|openbsd*)
		AC_CHECK_HEADERS(sys/audioio.h)
		if test $ac_cv_header_sys_audioio_h = yes
		then
			libmikmod_driver_sun=yes
			if test $libmikmod_netbsd = yes
			then
				libmikmod_driverlist="netbsd $libmikmod_driverlist"
			else
				libmikmod_driverlist="openbsd $libmikmod_driverlist"
			fi
		fi
	;;
	sunos*|solaris*)
		AC_CHECK_HEADERS(sun/audioio.h sys/audioio.h)
		if test $ac_cv_header_sun_audioio_h = yes || \
		   test $ac_cv_header_sys_audioio_h = yes
		then
			libmikmod_driver_sun=yes
			libmikmod_driverlist="sun $libmikmod_driverlist"
			if test "$cross_compiling" != yes
			then
			   case `uname -r` in
				4*) AC_DEFINE([SUNOS], 1, [Define if your system is SunOS 4.x]) ;;
			   esac
			fi
		fi
	;;
esac

if test $libmikmod_driver_af = yes
then
	libmikmod_driverlist="$libmikmod_driverlist audiofile"
	AC_DEFINE([DRV_AF], 1, [Define if the AudioFile audio server driver is compiled])
	LIBRARY_LIB="-lAF $LIBRARY_LIB"
fi

if test $libmikmod_driver_win = yes
then
	libmikmod_driverlist="win $libmikmod_driverlist"
	AC_DEFINE([DRV_WIN], 1, [Define if the Windows MCI driver is compiled])
	LIBRARY_LIB="-lwinmm $LIBRARY_LIB"
fi

if test $libmikmod_driver_ds = yes
then
	libmikmod_driverlist="ds $libmikmod_driverlist"
	AC_DEFINE([DRV_DS], 1, [Define if the Windows DirectSound driver is compiled])
	LIBRARY_LIB="-ldsound $LIBRARY_LIB"
fi

if test $libmikmod_driver_xaudio2 = yes
then
	libmikmod_driverlist="xaudio2 $libmikmod_driverlist"
	AC_DEFINE([DRV_XAUDIO2], 1, [Define if the Windows XAudio2 driver is compiled])
	LIBRARY_LIB="-lole32 $LIBRARY_LIB"
	if test $use_xaudio28 = yes
	then
		AC_DEFINE([DRV_XAUDIO28],1,[Define if using XAudio 2.8 for Windows8 with XAudio2 driver])
		LIBRARY_LIB="-lxaudio2_8 $LIBRARY_LIB"
	fi
fi

if test $libmikmod_driver_os2 = yes
then
	libmikmod_driverlist="os2 $libmikmod_driverlist"
	AC_DEFINE([DRV_OS2], 1, [Define if the OS/2 MCI driver is compiled])
	# drv_os2 and drv_dart both link to mmpm2
	if test $libmikmod_driver_dart = no ; then
		LIBRARY_LIB="-lmmpm2 $LIBRARY_LIB"
	fi
fi

if test $libmikmod_driver_dart = yes
then
	libmikmod_driverlist="dart $libmikmod_driverlist"
	AC_DEFINE([DRV_DART], 1, [Define if the OS/2 direct audio (DART) driver is compiled])
	LIBRARY_LIB="-lmmpm2 $LIBRARY_LIB"
fi

if test $libmikmod_driver_aix = yes
then
	libmikmod_driverlist="aix $libmikmod_driverlist"
	AC_DEFINE([DRV_AIX], 1, [Define if the AIX audio driver is compiled])
fi

if test $libmikmod_driver_alsa = yes
then
	libmikmod_driverlist="alsa $libmikmod_driverlist"
	AC_DEFINE([DRV_ALSA], 1, [Define if the Linux ALSA driver is compiled])
	if test $libmikmod_dynload = no
	then
		LIBRARY_LIB="-lasound $LIBRARY_LIB"
	fi
fi

if test $libmikmod_driver_esd = yes
then
	libmikmod_driverlist="esd $libmikmod_driverlist"
	AC_DEFINE([DRV_ESD], 1, [Define if the Enlightened Sound Daemon driver is compiled])
	CFLAGS="$CFLAGS $ESD_CFLAGS"
	# Under Solaris and perhaps other systems, we have to forward esd
	# dependencies to libmikmod's dependency list.
	# However, no need to keep libraries we won't require (libossaudio on BSD
	# systems, libaudiofile, libasound on Linux)
	if test $libmikmod_dynload != no
	then
		ESD_LIBS="`echo $ESD_LIBS|sed -e 's/-lesd//'|sed -e 's/-lasound//'|sed -e 's/-laudiofile//'|sed -e 's/-lossaudio//'`"
	fi
	LIBRARY_LIB="$ESD_LIBS $LIBRARY_LIB"
fi

if test $libmikmod_driver_nas = yes
then
	libmikmod_driverlist="nas $libmikmod_driverlist"
	AC_DEFINE([DRV_NAS], 1, [Define if the Network Audio System driver is compiled])
	LIBRARY_LIB="-laudio $LIBRARY_LIB"
fi

if test $libmikmod_driver_hp = yes
then
	libmikmod_driverlist="hp $libmikmod_driverlist"
	AC_DEFINE([DRV_HP], 1, [Define if the HP-UX audio driver is compiled])
fi

if test $libmikmod_driver_openal = yes
then
	libmikmod_driverlist="openal $libmikmod_driverlist"
	AC_DEFINE([DRV_OPENAL], 1, [Define if the OpenAL driver is compiled])
	if test $libmikmod_darwin = yes
	then
		LIBRARY_LIB="-framework OpenAL $LIBRARY_LIB"
	else
		LIBRARY_LIB="-lopenal $LIBRARY_LIB"
	fi
fi

if test $libmikmod_driver_sdl = yes
then
	libmikmod_driverlist="sdl $libmikmod_driverlist"
	AC_DEFINE([DRV_SDL], 1, [Define if the SDL audio driver is compiled])
	CFLAGS="$SDL_CFLAGS $CFLAGS"
	LIBRARY_LIB="$SDL_LIBS $LIBRARY_LIB"
fi

if test $libmikmod_driver_oss = yes && test $libmikmod_mingw = no && test $libmikmod_darwin = no
then
	# under Linux, /dev/sndstat is deprecated, never check for OSS as the
	# headers are always found.
	if test $libmikmod_linux = no && test "$cross_compiling" != yes
	then
		AC_CACHE_CHECK([for OSS (/dev/sndstat)],libmikmod_cv_oss_dev_sndstat,
		if test -c /dev/sndstat
		then
			libmikmod_cv_oss_dev_sndstat=yes
		else
			libmikmod_cv_oss_dev_sndstat=no
		fi)
	else
		libmikmod_cv_oss_dev_sndstat=yes
	fi
	if test $libmikmod_cv_oss_dev_sndstat = yes
	then
		AC_CHECK_HEADERS(machine/soundcard.h)
		AC_CHECK_HEADERS(sys/soundcard.h)
		if test $ac_cv_header_machine_soundcard_h = yes || \
		   test $ac_cv_header_sys_soundcard_h = yes
		then
			libmikmod_driverlist="oss $libmikmod_driverlist"
			AC_DEFINE([DRV_OSS], 1, [Define if the Open Sound System driver is compiled])
		fi
	fi
else
	libmikmod_driver_oss=no
fi

if test $libmikmod_darwin = yes
then
	if test $libmikmod_driver_osx = yes
	then
		libmikmod_driverlist="osx $libmikmod_driverlist"
		AC_DEFINE([DRV_OSX], 1, [Define this if you want the MacOS X CoreAudio driver])
		LIBRARY_LIB="-framework CoreAudio $LIBRARY_LIB"
	fi
fi

if test $libmikmod_darwin = yes
then
	if test $libmikmod_driver_mac = yes
	then
		libmikmod_driverlist="mac $libmikmod_driverlist"
		AC_DEFINE([DRV_MAC], 1, [Define this if you want the Carbon Mac Audio driver])
		LIBRARY_LIB="-framework Carbon $LIBRARY_LIB"
	fi
fi

if test $libmikmod_driver_sam9407 = yes
then
	libmikmod_driverlist="sam9407 $libmikmod_driverlist"
	AC_DEFINE([DRV_SAM9407], 1, [Define if the Linux SAM9407 driver is compiled])
fi

if test $libmikmod_driver_sgi = yes
then
	libmikmod_driverlist="sgi $libmikmod_driverlist"
	AC_DEFINE([DRV_SGI], 1, [Define if the SGI audio driver is compiled])
	LIBRARY_LIB="-laudio $LIBRARY_LIB"
fi

if test $libmikmod_driver_sun = yes
then
	AC_DEFINE([DRV_SUN], 1, [Define if the Sun audio driver or compatible (NetBSD, OpenBSD) is compiled])
fi

if test $libmikmod_driver_ultra = yes
then
	libmikmod_driverlist="ultra $libmikmod_driverlist"
	AC_DEFINE([DRV_ULTRA], 1, [Define if the Linux Ultra driver is compiled])
	if test $libmikmod_dynload = no
	then
		LIBRARY_LIB="-lgus $LIBRARY_LIB"
	fi
fi

if test $libmikmod_debug = yes
then
	AC_DEFINE([MIKMOD_DEBUG], 1, [Define if you want a debug version of the library])
	CC="$CC -g"
fi

if test $libmikmod_dynload != no
then
	AC_DEFINE([MIKMOD_DYNAMIC], 1, [Define if you want runtime dynamic linking of ALSA and EsounD drivers])

	LIBRARY_LIB="$libmikmod_dllib $LIBRARY_LIB"
	if test $libmikmod_dynload = sun
	then
		# RTLD_GLOBAL is not defined under every system
		AC_CACHE_CHECK([if RTLD_GLOBAL is defined],libmikmod_cv_decl_rtld_global,
		AC_TRY_COMPILE([#include <dlfcn.h>],[int flag=RTLD_GLOBAL;],
			libmikmod_cv_decl_rtld_global=yes,
			libmikmod_cv_decl_rtld_global=no))
		if test $libmikmod_cv_decl_rtld_global = yes
		then
			AC_DEFINE([HAVE_RTLD_GLOBAL], 1, [Define if your system has RTLD_GLOBAL defined in <dlfcn.h>])
		fi
	fi
fi

# If compiling with gcc, use adequate optimization flags
if test $ac_cv_prog_gcc = yes
then
	# On at least x86 platforms, gcc 2.7.2.1 and earlier won't work if -O3 (or
	# -finline-functions) are used. Versions 2.7.2.3, 2.8.x and egcs are ok
	# (didn't test 2.7.2.2).
	# Until there's an easy way to catch broken inlining, we choose flags known
	# to work correctly depending of the compiler version.
	AC_CACHE_CHECK([if inlining functions is safe],
	libmikmod_cv_gcc_inline_safe,
	AC_TRY_COMPILE(,[
#if !(defined __GNUC__ && (__GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ > 7))
#error inlining functions is not safe
#endif
],
	libmikmod_cv_gcc_inline_safe=yes,
	libmikmod_cv_gcc_inline_safe=no))
	if test $libmikmod_cv_gcc_inline_safe = no
	then
		CFLAGS="$CFLAGS -fno-strength-reduce"
	fi
	CFLAGS="$CFLAGS -ffast-math"
	if test $libmikmod_darwin = yes; then
		CFLAGS="$CFLAGS -fno-common"
	fi
	if test $libmikmod_debug = yes
	then
		CFLAGS="$CFLAGS -Wall -Werror"
	else
		CFLAGS="$CFLAGS -Wall"
	fi

	# pgcc 2.95.2 appears not to be able to compile libmikmod, although regular
	# gcc works fine. Issue a warning if the compiler is pgcc, until a reliable
	# way to detect flawed version (or, better, a reliable workaround) is
	# found.
dnl do not cache this test - better rechecking every time
	AC_MSG_CHECKING(if compiler is pgcc)
	if ($CC -v 2>&1 | grep ^pgcc > /dev/null) 2>/dev/null
	then
		libmikmod_gcc_is_pgcc=yes
	else
		libmikmod_gcc_is_pgcc=no
	fi
	AC_MSG_RESULT($libmikmod_gcc_is_pgcc)
	if test $libmikmod_gcc_is_pgcc = yes
	then
		echo "
*** Version 2.95.2 of this compiler, and perhaps others, are unable to
*** compile libmikmod. If compilation fails for playercode/virtch.c
*** around line 650, with the error ``internal error--insn does not
*** satisfy its constraints'', then you'll have to use another compiler.
" >&2
	fi
fi

if test x$libmikmod_threads != xno
then
	AC_DEFINE([HAVE_PTHREAD], 1, [Define if your system provides POSIX.4 threads])
	CFLAGS="$CFLAGS -D_REENTRANT"
	LIBRARY_LIB="$libmikmod_threads $LIBRARY_LIB"
	REENTRANT="-D_REENTRANT"
	if test $libmikmod_cv_gcc_pthread = yes
	then
		dnl There is no need for -pthread on darwin, and gcc
		dnl complains about unrecognized option -pthread
		if test $libmikmod_darwin = no
		then
			REENTRANT="-pthread $REENTRANT"
			LIB_LDADD="-pthread"
		fi
	fi
fi

case $host_cpu in
ppc|ppc64|powerpc|powerpc64)
	AC_MSG_CHECKING(whether to use altivec)
	if test x$libmikmod_simd = xyes
	then
		case $host_os in
		darwin*)
			dnl compile all Altivec functions, check at run-time
			CFLAGS="$CFLAGS -faltivec -force_cpusubtype_ALL"
			;;
		*)
			dnl -faltivec and -force_cpusubtype_ALL are Apple-only
			dnl features.  -maltivec compiles all code for AltiVec..
			CFLAGS="$CFLAGS -maltivec"
			;;
		esac
		libmikmod_altivec=yes
		libmikmod_simd=altivec
	fi
	AC_MSG_RESULT($libmikmod_altivec)
	;;

i*86|x86_64)
	AC_MSG_CHECKING(whether to use sse2)
	if test x$libmikmod_simd = xyes
	then
		dnl x86_64 already enables sse2
		dnl add -msse2 for x86 to enable it
		case `arch` in
		i*86)
			CFLAGS="$CFLAGS -msse2"
			;;
		esac
		libmikmod_sse2=yes
		libmikmod_simd=sse2
	fi
	AC_MSG_RESULT($libmikmod_sse2)
	;;
esac

# see if we set libmikmod_simd properly
case $libmikmod_simd in
altivec|sse2)
	AC_DEFINE([MIKMOD_SIMD], 1, [Define if you want to use SIMD (AltiVec or SSE2) optimizations (Unstable!)])
	;;
*)
	libmikmod_simd=no
	;;
esac

# MIKMOD_UNIX override
if test "$libmikmod_unix" = "0" || test "$libmikmod_unix" = "1"
then
   AC_DEFINE_UNQUOTED([MIKMOD_UNIX], $libmikmod_unix, [Define to 0 or 1 to override MIKMOD_UNIX in mikmod_internals.h.])
fi

# symbol visibility
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden -Werror"
AC_CACHE_CHECK([if compiler supports visibility attributes],
	libmikmod_cv_gcc_visibility,
	AC_LANG_SAVE
	AC_LANG_C
	AC_TRY_COMPILE([void foo(void);
__attribute__((visibility("default"))) void foo(void) {}],
		[],
		libmikmod_cv_gcc_visibility=yes,libmikmod_cv_gcc_visibility=no)
	AC_LANG_RESTORE)
# we want symbol -fvisibility for elf targets, however it works
# with darwin/macho too.  other than that, windows, dos and os2
# do not need it:  for any such targets, the -Werror switch is
# supposed to fail the above check. (I'm adding the manual test
# below nonetheless, just in case.)
case $host_os in
   mingw*|cygwin*|emx*|*djgpp)
	libmikmod_cv_gcc_visibility=no
	;;
esac
CFLAGS="$ac_save_CFLAGS"
if test $libmikmod_cv_gcc_visibility = yes ;then
   CFLAGS="$CFLAGS -DSYM_VISIBILITY -fvisibility=hidden"
fi

# =================
# Create Makefiles.
# =================

AC_SUBST(REENTRANT)
AC_SUBST(DOES_NOT_HAVE_SIGNED)

AC_SUBST(LIBRARY_LIB)

AC_SUBST(LIBMIKMOD_MAJOR_VERSION)
AC_SUBST(LIBMIKMOD_MINOR_VERSION)
AC_SUBST(LIBMIKMOD_MICRO_VERSION)
AC_SUBST(LIBMIKMOD_VERSION)

AC_SUBST(LIB_LDADD)

AC_OUTPUT(
libmikmod-config
libmikmod.pc
libmikmod.spec
Makefile
docs/Makefile
docs/libmikmod-config.1
dlapi/Makefile
drivers/Makefile
include/Makefile
include/mikmod.h
include/mikmod_build.h:include/mikmod.h.in
loaders/Makefile
mmio/Makefile
playercode/Makefile
posix/Makefile
macintosh/Makefile
macintosh/mikmod.h:include/mikmod.h.in
macintosh/mikmod_build.h:include/mikmod.h.in
macintosh/_libmikmodversion.r
macosx/Makefile
macosx/mikmod.h:include/mikmod.h.in
macosx/mikmod_build.h:include/mikmod.h.in
macosx/Info.plist
macosx/InfoPlist.strings
os2/Makefile
dos/Makefile
dos/include/Makefile
dos/src/Makefile
win32/Makefile
win32/mikmod.h:include/mikmod.h.in
win32/mikmod_build.h:include/mikmod.h.in
examples/Makefile)

chmod +x libmikmod-config
cat macintosh/_libmikmodversion.r | tr '\n' '\r' > macintosh/libmikmodversion.r

# ====================
# Print configuration.
# ====================

libmikmod_driverlist=`echo $libmikmod_driverlist raw wav aiff nosound|sed -e 's,  , ,g'`
if test $libmikmod_dynload != no
then
	libmikmod_dynload="yes, ${libmikmod_dynload} style"
fi

echo "
Library configuration:

  Source code location: $srcdir
  Compiler: $CC
  Compiler flags: $CFLAGS
  Debug version: $libmikmod_debug
  SIMD optimizations: $libmikmod_simd
  Dynamically loaded drivers whenever possible: $libmikmod_dynload
  Drivers to be compiled: $libmikmod_driverlist
  Install path: $prefix/lib, $prefix/include
"
