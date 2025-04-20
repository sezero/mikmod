# Copyright (c) 2012 Shlomi Fish
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# (This copyright notice applies only to this file)

CMAKE_MINIMUM_REQUIRED(VERSION 3.1...3.10)
PROJECT(libmikmod C)
LIST(APPEND CMAKE_MODULE_PATH "${libmikmod_SOURCE_DIR}/cmake")

SET(LIBMIKMOD_MAJOR_VERSION 3)
SET(LIBMIKMOD_MINOR_VERSION 3)
SET(LIBMIKMOD_MICRO_VERSION 13)

SET(VERSION "${LIBMIKMOD_MAJOR_VERSION}.${LIBMIKMOD_MINOR_VERSION}.${LIBMIKMOD_MICRO_VERSION}")
SET(LIBMIKMOD_VERSION "${LIBMIKMOD_MAJOR_VERSION}.${LIBMIKMOD_MINOR_VERSION}.${LIBMIKMOD_MICRO_VERSION}")
SET(CPACK_PACKAGE_VERSION_MAJOR ${LIBMIKMOD_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${LIBMIKMOD_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${LIBMIKMOD_MICRO_VERSION})

# package generation (make package[_source])
SET(CPACK_PACKAGE_NAME "libmikmod")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libmikmod - a module file playing and sound library")
SET(CPACK_PACKAGE_VENDOR "Shlomi Fish")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING.LESSER")

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

SET(base_with_ver "libmikmod-[0-9]+\\\\.[0-9]+\\\\.[0-9]+")
SET(CPACK_SOURCE_IGNORE_FILES
    "/_CPack_Packages/"
    "/CMakeFiles/"
    "/.deps/"
    "^${base_with_ver}(-Source|-Linux)?/"
    "${base_with_ver}.tar\\\\.(gz|bz2|Z|lzma|xz)$"
    "\\\\.o$"
    "~$"
    "/libmikmod\\\\.(a|la)$"
    "\\\\.so(\\\\.[0-9]+)*$"
    "/\\\\.svn/"
    "\\\\.(pdf|fo)$"
    "/CMakeCache\\\\.txt$"
    "/CTestTestfile\\\\.cmake$"
    "/cmake_install\\\\.cmake$"
    "/CPackConfig\\\\.cmake$"
    "/CPackSourceConfig\\\\.cmake$"
    "/tags$"
    "/libmikmod-config$"
    "^config\\\\.h$"
    "/install_manifest\\\\.txt$"
    "/t/"
    "/Testing/"
    "run-tests\\\\.pl"
    "/_Inline/"
    "/(B|build|BUILD)/"
    "/autom4te.cache/"
)

# some convenience macros for detection of several Unix OSs - based on SDL.
if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
set(SYS_LINUX 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "OpenBSD.*")
set(SYS_OPENBSD 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "kFreeBSD.*")
set(SYS_FREEBSD 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
set(SYS_NETBSD 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
set(SYS_OPENBSD 1)
elseif(CMAKE_SYSTEM_NAME MATCHES ".*GNU.*")
set(SYS_GNUHURD 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "DragonFly.*|FreeBSD")
set(SYS_FREEBSD 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "Solaris.*")
set(SYS_SOLARIS 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "AIX.*")
set(SYS_AIX 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "HP-UX.*")
set(SYS_HPUX 1)
endif()

if(SYS_OPENBSD)
set(USE_SUNAUDIO_DEFAULT OFF)
set(USE_SNDIO_DEFAULT  ON)
set(USE_RPATH_DEFAULT  ON)
else()
set(USE_SUNAUDIO_DEFAULT  ON)
set(USE_SNDIO_DEFAULT OFF)
set(USE_RPATH_DEFAULT OFF)
endif()

include(CMakeDependentOption)

cmake_dependent_option(ENABLE_ALSA "Include the ALSA driver" ON "SYS_LINUX" OFF)
cmake_dependent_option(ENABLE_OSS "Include the OSS driver" ON "UNIX;NOT APPLE" OFF)
cmake_dependent_option(ENABLE_ESD "Include the EsounD (Enlightened Sound Daemon) driver" OFF "UNIX;NOT APPLE" OFF)
cmake_dependent_option(ENABLE_PULSE "Include the PulseAudio driver" ON "UNIX;NOT APPLE" OFF)
cmake_dependent_option(ENABLE_SNDIO "Include the OpenBSD sndio driver" ${USE_SNDIO_DEFAULT} "UNIX;NOT APPLE" OFF)
cmake_dependent_option(ENABLE_NAS "Include the Network Audio System driver" OFF "UNIX;NOT APPLE" OFF)
#cmake_dependent_option(ENABLE_AF "Include the DEC AudioFile server (AF) driver" OFF "UNIX;NOT APPLE" OFF)
cmake_dependent_option(ENABLE_COREAUDIO "Include the CoreAudio driver" ON "APPLE" OFF)
cmake_dependent_option(ENABLE_DSOUND "Include the DirectSound driver" ON "WIN32" OFF)
cmake_dependent_option(ENABLE_WINMM "Include the Windows MCI driver" ON "WIN32" OFF)
cmake_dependent_option(ENABLE_XAUDIO2 "Include the XAudio2 driver" OFF "WIN32" OFF)
cmake_dependent_option(ENABLE_XAUDIO28 "Use XAudio2.8 for Windows8 instead of XAudio2.7" OFF "WIN32" OFF)
cmake_dependent_option(ENABLE_STDOUT "Support for output to stdout" ON "UNIX" OFF)
cmake_dependent_option(ENABLE_PIPE "Support for output via a pipe to another command" ON "UNIX" OFF)
cmake_dependent_option(ENABLE_AIX "Include the AIX audio driver" ON "SYS_AIX" OFF)
cmake_dependent_option(ENABLE_HPUX "Include the HP-UX audio driver" ON "SYS_HPUX" OFF)
cmake_dependent_option(ENABLE_SUNAUDIO "Include the SunAudio (or compatibles) driver" ${USE_SUNAUDIO_DEFAULT} "SYS_NETBSD OR SYS_OPENBSD OR SYS_SOLARIS" OFF)
option(ENABLE_OPENAL "Include the OpenAL driver" OFF)
option(ENABLE_SDL "Include the SDL driver" OFF)
option(ENABLE_AIFF "Support for output to an .aiff file" ON)
option(ENABLE_WAV "Support for output to a .wav file" ON)
option(ENABLE_RAW "Support for output raw pcm data to a file" ON)

cmake_dependent_option(ENABLE_DL "Load ALSA, PulseAudio or EsounD drivers at runtime" ON "ENABLE_ALSA OR ENABLE_PULSE OR ENABLE_ESD" OFF)
cmake_dependent_option(ENABLE_THREADS "build a thread-safe version of libmikmod" ON "UNIX" OFF) # only for pthreads.

option(DISABLE_DEPACKERS "Exclude support for module depackers" OFF)
option(DISABLE_HQMIXER "Exclude support for high quality mixer" OFF)

option(ENABLE_SIMD "Use SIMD (AltiVec or SSE2) optimizations (UNSTABLE!!!)" OFF)
option(ENABLE_RPATH "Whether to use RPATH when linking" ${USE_RPATH_DEFAULT})
option(ENABLE_SHARED "Whether to build the shared library" ON)
option(ENABLE_STATIC "Whether to build the static library" ON)

IF (NOT ENABLE_STATIC AND NOT ENABLE_SHARED)
    message(FATAL_ERROR "Both static and shared builds got disabled. You must enable at least one of them.")
ENDIF()

include(GNUInstallDirs)

### See:  http://www.cmake.org/Wiki/CMake_RPATH_handling
IF (ENABLE_RPATH)
  # use, i.e. don't skip the full RPATH for the build tree
    SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

  # when building, don't use the install RPATH already
  # (but later on when installing)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

  # the RPATH to be used when installing
    SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})

  # add the automatically determined parts of the RPATH
  # which point to directories outside the build tree to the install RPATH
    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
ELSE()
#   SET (CMAKE_SKIP_RPATH YES)
ENDIF()

IF (POLICY CMP0075)
    CMAKE_POLICY(SET CMP0075 NEW)
ENDIF()

include(CPack)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(FindPkgConfig)
include(TestBigEndian)
include(mik_macros)

ADD_SUBDIRECTORY ("docs")

IF (CMAKE_COMPILER_IS_GNUCC OR (CMAKE_C_COMPILER_ID MATCHES "Clang"))
    ADD_DEFINITIONS("-Wall" "-Wwrite-strings" "-Wshadow" "-Wundef" "-Wcast-qual")
ENDIF()

SET(COMPILER_FLAGS_TO_CHECK "-Werror=implicit-function-declaration")
LIST(APPEND COMPILER_FLAGS_TO_CHECK "-ffast-math")

IF (APPLE)
    LIST(APPEND COMPILER_FLAGS_TO_CHECK "-fno-common")
ENDIF()

IF (CPU_ARCH)
    LIST(APPEND COMPILER_FLAGS_TO_CHECK "-march=${CPU_ARCH}")
ENDIF()

IF (ENABLE_SIMD)
    IF(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
        LIST(APPEND COMPILER_FLAGS_TO_CHECK "-msse2")

    # -faltivec is a useful Apple extension. it is not the same as -maltivec.
    ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)" AND (CMAKE_SYSTEM_NAME MATCHES "Darwin"))
        LIST(APPEND COMPILER_FLAGS_TO_CHECK "-faltivec")

    ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64" AND (CMAKE_SYSTEM_NAME MATCHES "Darwin"))
        LIST(APPEND COMPILER_FLAGS_TO_CHECK "-faltivec")

    ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)")
        LIST(APPEND COMPILER_FLAGS_TO_CHECK "-maltivec")

    ENDIF()
ENDIF()

SET (IDX 1)
FOREACH (CFLAG_TO_CHECK ${COMPILER_FLAGS_TO_CHECK})
    SET (FLAG_EXISTS_VAR "FLAG_EXISTS_${IDX}")
    MATH (EXPR IDX "${IDX} + 1")
    CHECK_C_COMPILER_FLAG("${CFLAG_TO_CHECK}" ${FLAG_EXISTS_VAR})
    IF (${FLAG_EXISTS_VAR})
        ADD_DEFINITIONS(${CFLAG_TO_CHECK})
    ENDIF()
ENDFOREACH()

# check inline keyword
CHECK_C_SOURCE_COMPILES("static inline int static_foo() {return 0;}
                         int main(void) {return 0;}" HAVE_C_INLINE)
CHECK_C_SOURCE_COMPILES("static __inline__ int static_foo() {return 0;}
                         int main(void) {return 0;}" HAVE_C___INLINE__)
CHECK_C_SOURCE_COMPILES("static __inline int static_foo() {return 0;}
                         int main(void) {return 0;}" HAVE_C___INLINE)

# check for symbol visibility attributes
IF(NOT WIN32 AND NOT CYGWIN)
    SET(OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS} -Werror -fvisibility=hidden")
    CHECK_C_SOURCE_COMPILES("__attribute__((visibility(\"default\"))) int foo(void);
                             __attribute__((visibility(\"hidden\")))  int bar(void);
                             int foo (void) { return 0; }
                             int bar (void) { return 1; }
                             int main(void) { return 0; }" HAVE_VISIBILITY)
    IF(HAVE_VISIBILITY)
        ADD_DEFINITIONS(-fvisibility=hidden)
        ADD_DEFINITIONS("-DSYM_VISIBILITY")
    ENDIF()
    SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS}")
ENDIF()

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

# to find mikmod_internals.h:
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(BEFORE "${PROJECT_SOURCE_DIR}/include")

# to find config.h:
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_BINARY_DIR})

CHECK_MULTI_INCLUDE_FILES(
    "dlfcn.h" "fcntl.h" "limits.h" "malloc.h" "memory.h" "sys/ioctl.h" "unistd.h" "windows.h"
)

IF(WIN32 AND NOT HAVE_WINDOWS_H)
    MESSAGE(FATAL_ERROR "windows.h not found.")
ENDIF()

IF(SYS_LINUX)
    CHECK_MULTI_INCLUDE_FILES("alsa/asoundlib.h" "sys/sam9407.h")
ENDIF()

CHECK_MULTI_FUNCTIONS_EXISTS(
    "posix_memalign" "setenv" "snprintf" "srandom"
)

ADD_DEFINITIONS("-DHAVE_CONFIG_H")
# always define MIKMOD_BUILD when building libmikmod
ADD_DEFINITIONS("-DMIKMOD_BUILD")

SET (MIKMOD_LIB_MODULES
    drivers/drv_AF.c
    drivers/drv_aiff.c
    drivers/drv_ahi.c
    drivers/drv_aix.c
    drivers/drv_alsa.c
    drivers/drv_dart.c
    drivers/drv_ds.c
    drivers/drv_esd.c
    drivers/drv_gp32.c
    drivers/drv_hp.c
    drivers/drv_mac.c
    drivers/drv_n64.c
    drivers/drv_nas.c
    drivers/drv_nos.c
    drivers/drv_openal.c
    drivers/drv_osles.c
    drivers/drv_oss.c
    drivers/drv_osx.c
    drivers/drv_pipe.c
    drivers/drv_psp.c
    drivers/drv_pulseaudio.c
    drivers/drv_raw.c
    drivers/drv_sam9407.c
    drivers/drv_sdl.c
    drivers/drv_sgi.c
    drivers/drv_sndio.c
    drivers/drv_stdout.c
    drivers/drv_sun.c
    drivers/drv_ultra.c
    drivers/drv_wav.c
    drivers/drv_win.c
    drivers/drv_xaudio2.c
    loaders/load_669.c
    loaders/load_amf.c
    loaders/load_asy.c
    loaders/load_dsm.c
    loaders/load_far.c
    loaders/load_gdm.c
    loaders/load_gt2.c
    loaders/load_imf.c
    loaders/load_it.c
    loaders/load_m15.c
    loaders/load_med.c
    loaders/load_mod.c
    loaders/load_mtm.c
    loaders/load_okt.c
    loaders/load_s3m.c
    loaders/load_stm.c
    loaders/load_stx.c
    loaders/load_ult.c
    loaders/load_umx.c
    loaders/load_uni.c
    loaders/load_xm.c
    mmio/mmalloc.c
    mmio/mmerror.c
    mmio/mmio.c
    depackers/mmcmp.c
    depackers/pp20.c
    depackers/s404.c
    depackers/xpk.c
    posix/strcasecmp.c
    playercode/mdreg.c
    playercode/mdriver.c
    playercode/mdulaw.c
    playercode/mloader.c
    playercode/mlreg.c
    playercode/mlutil.c
    playercode/mplayer.c
    playercode/munitrk.c
    playercode/mwav.c
    playercode/npertab.c
    playercode/sloader.c
    playercode/virtch.c
    playercode/virtch2.c
    playercode/virtch_common.c
)

SET (LIBMIKMOD_TARGETS)

IF (ENABLE_SHARED)
    IF (MINGW)
        SET(CMAKE_SHARED_LIBRARY_NAME_WITH_VERSION 1)
    ENDIF()
    IF (MSVC)
        SET(CMAKE_SHARED_LIBRARY_PREFIX "lib")
    ENDIF()
    ADD_LIBRARY (mikmod
        SHARED
        ${MIKMOD_LIB_MODULES}
    )
    LIST (APPEND LIBMIKMOD_TARGETS "mikmod")

    SET_TARGET_PROPERTIES(mikmod
        PROPERTIES VERSION 3.3.0 SOVERSION 3
    )
    IF(WIN32)
        SET_TARGET_PROPERTIES(mikmod
            PROPERTIES DEFINE_SYMBOL DLL_EXPORT
        )
    ENDIF()
    IF(OS2)
        TARGET_SOURCES(mikmod PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/os2/mikmod.def)
        SET_TARGET_PROPERTIES(mikmod
            PROPERTIES OUTPUT_NAME "mikmod3"
        )
    ENDIF()
ENDIF()

IF (ENABLE_STATIC)
    ADD_LIBRARY (mikmod-static
        STATIC
        ${MIKMOD_LIB_MODULES}
    )
    LIST (APPEND LIBMIKMOD_TARGETS "mikmod-static")
    IF (NOT MSVC)
        SET_TARGET_PROPERTIES(
            mikmod-static
            PROPERTIES OUTPUT_NAME "mikmod"
        )
    ENDIF()
    SET_TARGET_PROPERTIES(
        ${LIBMIKMOD_TARGETS}
        PROPERTIES CLEAN_DIRECT_OUTPUT 1
    )
ENDIF()

IF(DISABLE_HQMIXER)
  SET(NO_HQMIXER 1)
ENDIF()
IF(DISABLE_DEPACKERS)
  SET(NO_DEPACKERS 1)
ENDIF()
IF(ENABLE_PIPE)
  SET(DRV_PIPE 1)
ENDIF()
IF(ENABLE_STDOUT)
  SET(DRV_STDOUT 1)
ENDIF()
IF(ENABLE_AIFF)
SET(DRV_AIFF 1)
ENDIF()
IF(ENABLE_WAV)
SET(DRV_WAV 1)
ENDIF()
IF(ENABLE_RAW)
SET(DRV_RAW 1)
ENDIF()
IF(ENABLE_SIMD)
  SET(MIKMOD_SIMD 1)
ENDIF()
IF (ENABLE_DL)
  SET(MIKMOD_DYNAMIC 1)
ENDIF()

SET(LIBRARY_LIB )
SET(EXTRA_LIBS )

SET(HAVE_PTHREAD)
IF (NOT WIN32)
 INCLUDE(FindThreads)
 IF (CMAKE_USE_PTHREADS_INIT)
  IF (ENABLE_THREADS)
    SET (HAVE_PTHREAD 1)
    IF (CMAKE_THREAD_LIBS_INIT)
        LIST (APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})
        SET(LIBRARY_LIB "${LIBRARY_LIB} ${CMAKE_THREAD_LIBS_INIT}")
    ENDIF()
  ENDIF()
 ENDIF()
ENDIF()

IF (ENABLE_PULSE)
    PKG_CHECK_MODULES(PKG_PULSESIMPLE "libpulse-simple")
    IF(PKG_PULSESIMPLE_FOUND)
        SET(DRV_PULSEAUDIO 1)
        SET(PULSESIMPLE_LIBRARIES ${PKG_PULSESIMPLE_LDFLAGS})
        ADD_DEFINITIONS (${PKG_PULSESIMPLE_CFLAGS})
    ELSE()
        find_path(PULSESIMPLE_INCLUDE_DIR "pulse/simple.h")
        find_library(PULSESIMPLE_LIBRARIES "pulse-simple")
        IF (NOT PULSESIMPLE_LIBRARIES)
            MESSAGE(STATUS "Could NOT find PulseAudio.")
            SET(ENABLE_PULSE 0)
            MESSAGE(WARNING "PulseAudio support is disabled.")
        ELSE()
            MESSAGE(STATUS "Found PulseAudio: ${PULSESIMPLE_LIBRARIES}")
            SET(DRV_PULSEAUDIO 1)
            INCLUDE_DIRECTORIES(${PULSESIMPLE_INCLUDE_DIR})
        ENDIF()
    ENDIF()
    IF(ENABLE_PULSE AND NOT ENABLE_DL)
      LIST (APPEND EXTRA_LIBS ${PULSESIMPLE_LIBRARIES})
      SET(LIBRARY_LIB "${LIBRARY_LIB} -lpulse-simple -lpulse")
    ENDIF()
ENDIF()

IF (ENABLE_ESD)
    PKG_CHECK_MODULES(PKG_ESD esound)
    IF(PKG_ESD_FOUND)
        SET(DRV_ESD 1)
        SET(ESD_LIBRARIES ${PKG_ESD_LDFLAGS})
        ADD_DEFINITIONS (${PKG_ESD_CFLAGS})
    ELSE()
        find_path(ESD_INCLUDE_DIR esd.h)
        find_library(ESD_LIBRARIES esd)
        IF (NOT ESD_LIBRARIES)
            MESSAGE(STATUS "Could NOT find EsounD.")
            SET(ENABLE_ESD 0)
            MESSAGE(WARNING "EsounD support is disabled.")
        ELSE()
            MESSAGE(STATUS "Found EsounD: ${ESD_LIBRARIES}")
            SET(DRV_ESD 1)
            INCLUDE_DIRECTORIES(${ESD_INCLUDE_DIR})
            IF(NOT ENABLE_DL)
               LIST (APPEND EXTRA_LIBS ${ESD_LIBRARIES})
               ## FIXME: -laudiofile is needed here, too??
               SET(LIBRARY_LIB "${LIBRARY_LIB} -lesd")
            ENDIF()
        ENDIF()
    ENDIF()
ENDIF()

IF (ENABLE_NAS)
  CHECK_INCLUDE_FILE("audio/audiolib.h" HAVE_AUDIO_AUDIOLIB_H)
  find_library(NAS_LIBRARIES audio)
  IF(NAS_LIBRARIES)
    SET(OLD_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
    SET(CMAKE_REQUIRED_LIBRARIES "audio")
    CHECK_FUNCTION_EXISTS(AuCreateFlow NAS_LIBS_OK)
    SET(CMAKE_REQUIRED_LIBRARIES "${OLD_REQUIRED_LIBRARIES}")
  ENDIF()
  IF((NOT NAS_LIBS_OK) OR (NOT HAVE_AUDIO_AUDIOLIB_H))
    MESSAGE(STATUS "Could not find NAS.")
    SET(ENABLE_NAS 0)
    MESSAGE(WARNING "NAS (Network Audio System) support is disabled.")
  ELSE()
    MESSAGE(STATUS "Found NAS: ${NAS_LIBRARIES}")
    SET(DRV_NAS 1)
    LIST (APPEND EXTRA_LIBS ${NAS_LIBRARIES})
    SET(LIBRARY_LIB "${LIBRARY_LIB} -laudio")
  ENDIF()
ENDIF()

#IF (ENABLE_AF)
#  CHECK_INCLUDE_FILE("AF/AFlib.h" HAVE_AF_AFLIB_H)
#  find_library(AF_LIBRARIES AF)
#  IF(AF_LIBRARIES)
#    SET(OLD_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
#    SET(CMAKE_REQUIRED_LIBRARIES "AF")
#    CHECK_FUNCTION_EXISTS(AFOpenAudioConn AF_LIBS_OK)
#    SET(CMAKE_REQUIRED_LIBRARIES "${OLD_REQUIRED_LIBRARIES}")
#  ENDIF()
#  IF((NOT AF_LIBS_OK) OR (NOT HAVE_AF_AFLIB_H))
#    MESSAGE(STATUS "Could not find DEC AudioFile server.")
#    SET(ENABLE_AF 0)
#    MESSAGE(WARNING "DEC AudioFile server support is disabled.")
#  ELSE()
#    MESSAGE(STATUS "Found DEC AudioFile server: ${AF_LIBRARIES}")
#    SET(DRV_AF 1)
#    LIST (APPEND EXTRA_LIBS ${AF_LIBRARIES})
#    SET(LIBRARY_LIB "${LIBRARY_LIB} -lAF")
#  ENDIF()
#ENDIF()

IF (ENABLE_OSS)
  CHECK_MULTI_INCLUDE_FILES("sys/soundcard.h" "machine/soundcard.h" "soundcard.h")

  # NetBSD and OpenBSD uses ossaudio emulation layer,
  # otherwise no link library is needed.
  IF(SYS_NETBSD OR SYS_OPENBSD) # AND HAVE_SOUNDCARD_H ??
    FIND_LIBRARY(OSSAUDIO_LIBRARIES "ossaudio")
    IF(OSSAUDIO_LIBRARIES)
      MESSAGE(STATUS "Found libossaudio: ${OSSAUDIO_LIBRARIES}")
    ENDIF()
  ELSE()
    SET(OSSAUDIO_LIBRARIES)
  ENDIF()
  SET(OLD_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
  IF(OSSAUDIO_LIBRARIES)
       SET(CMAKE_REQUIRED_LIBRARIES ${OSSAUDIO_LIBRARIES})
  ENDIF()

  IF(HAVE_SYS_SOUNDCARD_H)
    CHECK_C_SOURCE_COMPILES("#include <sys/ioctl.h>
                             #include <sys/soundcard.h>
                             int main() {return SNDCTL_DSP_RESET;}" HAVE_OSS)
  ELSEIF(HAVE_MACHINE_SOUNDCARD_H)
    CHECK_C_SOURCE_COMPILES("#include <sys/ioctl.h>
                             #include <machine/soundcard.h>
                             int main() {return SNDCTL_DSP_RESET;}" HAVE_OSS)
  ELSEIF(HAVE_SOUNDCARD_H)
    CHECK_C_SOURCE_COMPILES("#include <sys/ioctl.h>
                             #include <soundcard.h>
                             int main() {return SNDCTL_DSP_RESET;}" HAVE_OSS)
  ENDIF()
  SET(CMAKE_REQUIRED_LIBRARIES "${OLD_REQUIRED_LIBRARIES}")

  IF(HAVE_OSS)
    SET (DRV_OSS 1)
    IF(OSSAUDIO_LIBRARIES)
      LIST(APPEND EXTRA_LIBS ${OSSAUDIO_LIBRARIES})
      SET(LIBRARY_LIB "${LIBRARY_LIB} -lossaudio")
    ENDIF()
  ELSE()
    SET (ENABLE_OSS 0)
    MESSAGE(WARNING "OSS support is disabled.")
  ENDIF()
ENDIF()

IF (ENABLE_ALSA)
  include(FindALSA)
  IF (ALSA_FOUND)
    SET (DRV_ALSA 1)
    IF(NOT ENABLE_DL)
      LIST (APPEND EXTRA_LIBS ${ALSA_LIBRARIES})
      SET(LIBRARY_LIB "${LIBRARY_LIB} -lasound")
    ENDIF()
  ELSE()
    SET (ENABLE_ALSA 0)
    MESSAGE(WARNING "ALSA support is disabled.")
  ENDIF()
ENDIF()

IF (ENABLE_SNDIO)
  CHECK_INCLUDE_FILE("sndio.h" HAVE_SNDIO_H)
  find_library(SNDIO_LIBRARIES sndio)
  IF(SNDIO_LIBRARIES)
    SET(OLD_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
    SET(CMAKE_REQUIRED_LIBRARIES "sndio")
    CHECK_FUNCTION_EXISTS(sio_open SNDIO_LIBS_OK)
    SET(CMAKE_REQUIRED_LIBRARIES "${OLD_REQUIRED_LIBRARIES}")
  ENDIF()
  IF((NOT SNDIO_LIBS_OK) OR (NOT HAVE_SNDIO_H))
    SET (ENABLE_SNDIO 0)
    MESSAGE(WARNING "OpenBSD sndio support is disabled.")
  ELSE()
    SET (DRV_SNDIO 1)
    LIST (APPEND EXTRA_LIBS ${SNDIO_LIBRARIES})
    SET(LIBRARY_LIB "${LIBRARY_LIB} -lsndio")
  ENDIF()
ENDIF()

IF (ENABLE_SUNAUDIO)
  CHECK_MULTI_INCLUDE_FILES("sys/audioio.h" "sun/audioio.h")
  IF ("${HAVE_SYS_AUDIOIO_H}" OR "${HAVE_SUN_AUDIOIO_H}")
    SET (DRV_SUN 1)
  ELSE()
    SET (ENABLE_SUNAUDIO 0)
    MESSAGE(WARNING "SunAudio driver support is disabled.")
  ENDIF()
ENDIF()

IF (ENABLE_AIX)
  CHECK_MULTI_INCLUDE_FILES("sys/audio.h" "sys/acpa.h")
  IF ("${HAVE_SYS_AUDIO_H}" AND "${HAVE_SYS_ACPA_H}")
    SET (DRV_AIX 1)
  ELSE()
    SET (ENABLE_AIX 0)
    MESSAGE(WARNING "AIX audio driver support is disabled.")
  ENDIF()
ENDIF()

IF (ENABLE_HPUX)
  CHECK_INCLUDE_FILE("sys/audio.h" HAVE_SYS_AUDIO_H)
  IF ("${HAVE_SYS_AUDIO_H}")
    SET (DRV_HPUX 1)
  ELSE()
    SET (ENABLE_HPUX 0)
    MESSAGE(WARNING "HP-UX audio driver support is disabled.")
  ENDIF()
ENDIF()

IF (ENABLE_COREAUDIO)
    SET (DRV_OSX 1)
    LIST (APPEND EXTRA_LIBS -Wl,-framework,AudioToolbox)
    LIST (APPEND EXTRA_LIBS -Wl,-framework,AudioUnit)
    LIST (APPEND EXTRA_LIBS -Wl,-framework,CoreServices)
    SET(LIBRARY_LIB "${LIBRARY_LIB} -framework AudioToolbox -framework AudioUnit -framework CoreServices")
ENDIF()

IF (ENABLE_WINMM)
    SET (DRV_WIN 1)
    LIST (APPEND EXTRA_LIBS "winmm")
    SET(LIBRARY_LIB "${LIBRARY_LIB} -lwinmm")
ENDIF()

IF (ENABLE_DSOUND)
    CHECK_INCLUDE_FILE("dsound.h" HAVE_DSOUND_H)
    IF (HAVE_DSOUND_H)
      SET (DRV_DS 1)
      LIST (APPEND EXTRA_LIBS "dsound")
      SET(LIBRARY_LIB "${LIBRARY_LIB} -ldsound")
    ELSE()
      SET (ENABLE_DSOUND 0)
      MESSAGE(WARNING "DirectSound support is disabled.")
    ENDIF()
ENDIF()

IF (ENABLE_XAUDIO2)
    SET(OLD_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
    IF (ENABLE_XAUDIO28)
      SET(CMAKE_REQUIRED_DEFINITIONS "-D_WIN32_WINNT=0x0602")
    ENDIF()
    CHECK_INCLUDE_FILE("xaudio2.h" HAVE_XAUDIO2_H)
    SET(CMAKE_REQUIRED_DEFINITIONS "${OLD_REQUIRED_DEFINITIONS}")
    IF(HAVE_XAUDIO2_H)
      CHECK_SYMBOL_EXISTS(XAUDIO2_VOICE_NOSAMPLESPLAYED xaudio2.h XAUDIO2_7_HEADER)
      IF (XAUDIO2_7_HEADER AND ENABLE_XAUDIO28)
        MESSAGE(WARNING "xaudio2.h <= 2.7 and xaudio2.8 enabled - build will likely fail.")
      ELSEIF((NOT XAUDIO2_7_HEADER) AND (NOT ENABLE_XAUDIO28))
        MESSAGE(WARNING "xaudio2.h >= 2.8 and xaudio2.8 disabled - build will likely fail.")
      ENDIF()
      SET (DRV_XAUDIO2 1)
      LIST (APPEND EXTRA_LIBS "ole32")
      SET(LIBRARY_LIB "${LIBRARY_LIB} -lole32")
      IF (ENABLE_XAUDIO28)
          ADD_DEFINITIONS("-D_WIN32_WINNT=0x0602")
          SET (DRV_XAUDIO28 1)
          LIST (APPEND EXTRA_LIBS "xaudio2_8")
          SET(LIBRARY_LIB "${LIBRARY_LIB} -lxaudio2_8")
      ENDIF()
    ELSE()
      SET (ENABLE_XAUDIO2 0)
      SET (ENABLE_XAUDIO28 0)
      MESSAGE(WARNING "XAudio2 support is disabled.")
    ENDIF()
ENDIF()

IF (ENABLE_SDL)
  SET (SDL_BUILDING_LIBRARY TRUE)
  include(FindSDL)
  IF (SDL_FOUND)
    SET (DRV_SDL 1)
    LIST (APPEND EXTRA_LIBS ${SDL_LIBRARY})
    ## FIXME: SDL dependency libraries here??
    SET(LIBRARY_LIB "${LIBRARY_LIB} -lSDL")
  ELSE()
    SET (ENABLE_SDL 0)
    MESSAGE(WARNING "SDL support is disabled.")
  ENDIF()
ENDIF()

IF (ENABLE_OPENAL)
  include(FindOpenAL)
  IF (OPENAL_FOUND)
    SET (DRV_OPENAL 1)
    CHECK_MULTI_INCLUDE_FILES("AL/al.h" "AL/alc.h")
    IF (HAVE_AL_AL_H)
        # alext.h depends on al.h and alc.h, therefore
        # a plain CHECK_INCLUDE_FILE() doesn't work.
        CHECK_C_SOURCE_COMPILES("#include <AL/al.h>
                                 #include <AL/alc.h>
                                 #include <AL/alext.h>
                                 int main() {return 0;}" HAVE_AL_ALEXT_H)
    ENDIF()
    LIST (APPEND EXTRA_LIBS ${OPENAL_LIBRARY})
    IF(APPLE)
      SET(LIBRARY_LIB "${LIBRARY_LIB} -framework OpenAL")
    ELSE()
      SET(LIBRARY_LIB "${LIBRARY_LIB} -lopenal")
    ENDIF()
  ELSE()
    SET (ENABLE_OPENAL 0)
    MESSAGE(WARNING "OpenAL support is disabled.")
  ENDIF()
ENDIF()

IF (ENABLE_DL AND CMAKE_DL_LIBS)
  LIST (APPEND EXTRA_LIBS ${CMAKE_DL_LIBS})
  SET(LIBRARY_LIB "${LIBRARY_LIB} -l${CMAKE_DL_LIBS}")
ENDIF()

CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/config.h.cmake" "${PROJECT_BINARY_DIR}/config.h")

SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix "\${prefix}")
SET(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
SET(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
STRING(STRIP "${LIBRARY_LIB}" LIBRARY_LIB)
CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/libmikmod-config.in" "${PROJECT_BINARY_DIR}/libmikmod-config" @ONLY)
CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/libmikmod.pc.in" "${PROJECT_BINARY_DIR}/libmikmod.pc" @ONLY)

FOREACH (TGT ${LIBMIKMOD_TARGETS})
    TARGET_LINK_LIBRARIES ("${TGT}"
        ${EXTRA_LIBS})
    INCLUDE_DIRECTORIES(AFTER ${ALSA_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIR} ${SDL_INCLUDE_DIR})
ENDFOREACH (TGT)

INSTALL(
    TARGETS ${LIBMIKMOD_TARGETS}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
INSTALL(
    FILES "${PROJECT_BINARY_DIR}/libmikmod-config"
    PERMISSIONS GROUP_EXECUTE GROUP_READ OWNER_EXECUTE OWNER_READ OWNER_WRITE WORLD_EXECUTE WORLD_READ
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
INSTALL(
    FILES
        "${PROJECT_SOURCE_DIR}/include/mikmod.h"
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}
)
INSTALL(
    FILES
        "${PROJECT_BINARY_DIR}/libmikmod.pc"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
