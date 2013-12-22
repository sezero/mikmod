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

PROJECT(libmikmod C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
LIST(APPEND CMAKE_MODULE_PATH "${libmikmod_SOURCE_DIR}/cmake")

SET(LIBMIKMOD_MAJOR_VERSION 3)
SET(LIBMIKMOD_MINOR_VERSION 3)
SET(LIBMIKMOD_MICRO_VERSION 5)

SET(VERSION "${LIBMIKMOD_MAJOR_VERSION}.${LIBMIKMOD_MINOR_VERSION}.${LIBMIKMOD_MICRO_VERSION}")
SET(LIBMIKMOD_VERSION "${LIBMIKMOD_MAJOR_VERSION}.${LIBMIKMOD_MINOR_VERSION}.${LIBMIKMOD_MICRO_VERSION}")
SET(CPACK_PACKAGE_VERSION_MAJOR ${LIBMIKMOD_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${LIBMIKMOD_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${LIBMIKMOD_MICRO_VERSION})

# package generation (make package[_source])
SET(CPACK_PACKAGE_NAME "libmikmod")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libmikmod - a module file playing and sound library")
SET(CPACK_PACKAGE_VENDOR "Shlomi Fish")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER")

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

SET (base_with_ver "libmikmod-[0-9]+\\\\.[0-9]+\\\\.[0-9]+")
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
    "/t/t/.*\\\\.exe$"
    "/t/Presets"
    "/CMakeCache\\\\.txt$"
    "/CTestTestfile\\\\.cmake$"
    "/cmake_install\\\\.cmake$"
    "/CPackConfig\\\\.cmake$"
    "/CPackSourceConfig\\\\.cmake$"
    "/tags$"
    "/libmikmod-config$"
    "^config\\\\.h$"
    "/install_manifest\\\\.txt$"
    "/t/card-test-(parse|render)\\\\.c$"
    "/Testing/"
    "ids-whitelist\\\\.txt"
    "/_Inline/"
    "/(B|build|BUILD)/"
    "/autom4te.cache/"
)

SET (ENABLE_ALSA 1 CACHE BOOL "Include the ALSA driver")
SET (ENABLE_ESD 0 CACHE BOOL "Include the EsounD (Enlightened Sound Daemon) driver")
SET (ENABLE_OPENAL 0 CACHE BOOL "Include the OpenAL driver")
SET (ENABLE_OSS 1 CACHE BOOL "Include the OSS driver")
SET (ENABLE_SDL 0 CACHE BOOL "Include the SDL driver")
SET (ENABLE_STDOUT 1 CACHE BOOL "Support for output to stdout")
SET (ENABLE_PIPE 1 CACHE BOOL "Support for output via a pipe to another command")
SET (ENABLE_DL 0 CACHE BOOL "Load ALSA and Ultra Drivers at Runtime")
SET (ENABLE_SIMD 0 CACHE BOOL "Use SIMD (AltiVec or SSE2) optimizations (Unstable!)")
SET (ENABLE_THREADS 1 CACHE BOOL "build a thread-safe version of libmikmod")
SET (ENABLE_RPATH "1" CACHE BOOL "Whether to use an rpath when linking" )
SET (ENABLE_STATIC "1" CACHE BOOL "Whether to build the static library (which takes more time)" )

### This is to set the RPATH correctly, so when installed
### under a prefix the executables will find the libraries.
### See:  http://www.cmake.org/Wiki/CMake_RPATH_handling
IF (ENABLE_RPATH)
    # use, i.e. don't skip the full RPATH for the build tree
    SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

    # when building, don't use the install RPATH already
    # (but later on when installing)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

    # the RPATH to be used when installing
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")

    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
#ELSE(ENABLE_RPATH)
#   SET (CMAKE_SKIP_RPATH YES)
ENDIF(ENABLE_RPATH)

INCLUDE(CPack)
include(CheckFunctionExists)
include(CheckSymbolExists)
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCSourceCompiles)
include(FindPkgConfig)
include(TestBigEndian)
include(mik_macros)

ADD_SUBDIRECTORY ("dlapi")
ADD_SUBDIRECTORY ("docs")
ADD_SUBDIRECTORY ("dos")
ADD_SUBDIRECTORY ("drivers")
ADD_SUBDIRECTORY ("include")
ADD_SUBDIRECTORY ("loaders")
ADD_SUBDIRECTORY ("macintosh")
ADD_SUBDIRECTORY ("macosx")
ADD_SUBDIRECTORY ("mmio")
ADD_SUBDIRECTORY ("os2")
ADD_SUBDIRECTORY ("playercode")
ADD_SUBDIRECTORY ("posix")
ADD_SUBDIRECTORY ("win32")

SET(COMPILER_FLAGS_TO_CHECK
    "-Wall" "-Werror=implicit-function-declaration"
)

IF (CPU_ARCH)
    LIST(APPEND COMPILER_FLAGS_TO_CHECK "-march=${CPU_ARCH}")
ENDIF(CPU_ARCH)

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

    ENDIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
ENDIF(ENABLE_SIMD)

IF ((CMAKE_BUILD_TYPE STREQUAL debug) OR (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo))
    IF (${CMAKE_COMPILER_IS_GNUCC})
        ADD_DEFINITIONS(
            "-Wold-style-declaration"
            "-Wmissing-prototypes"
            "-Wformat-nonliteral"
            "-Wcast-align"
            "-Wpointer-arith"
            "-Wbad-function-cast"
            "-Wstrict-prototypes"
            "-Wmissing-declarations"
            "-Wundef"
            "-Wnested-externs"
            "-Wcast-qual"
            "-Wshadow"
            "-Wwrite-strings"
            "-Wunused"
            "-Wfloat-equal"
            "-Wold-style-definition"
            "-Wdeclaration-after-statement"
        )
    ENDIF (${CMAKE_COMPILER_IS_GNUCC})
ENDIF ((CMAKE_BUILD_TYPE STREQUAL debug) OR (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo))

IF (CMAKE_BUILD_TYPE STREQUAL release)
    LIST(APPEND COMPILER_FLAGS_TO_CHECK "-flto")
ENDIF (CMAKE_BUILD_TYPE STREQUAL release)

SET (IDX 1)
FOREACH (CFLAG_TO_CHECK ${COMPILER_FLAGS_TO_CHECK})
    SET (FLAG_EXISTS_VAR "FLAG_EXISTS_${IDX}")
    MATH (EXPR IDX "${IDX} + 1")
    CHECK_C_COMPILER_FLAG("${CFLAG_TO_CHECK}" ${FLAG_EXISTS_VAR})
    IF (${FLAG_EXISTS_VAR})
        ADD_DEFINITIONS(${CFLAG_TO_CHECK})
    ENDIF (${FLAG_EXISTS_VAR})
ENDFOREACH(CFLAG_TO_CHECK)

# check for symbol visibility attributes
IF(NOT WIN32 AND NOT CYGWIN)
    SET(OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS} -Werror")
    CHECK_C_SOURCE_COMPILES("int foo(void) __attribute__((visibility(\"default\")));
                             int main(void) {return 0;}" HAVE_VISIBILITY_DEFAULT)
    IF(HAVE_VISIBILITY_DEFAULT)
        CHECK_C_COMPILER_FLAG(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN)
        IF(HAVE_VISIBILITY_HIDDEN)
            ADD_DEFINITIONS(-fvisibility=hidden)
            ADD_DEFINITIONS("-DSYM_VISIBILITY")
        ENDIF()
    ENDIF()
    SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS}")
ENDIF()

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

# to find mikmod_internals.h:
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# to find config.h and mikmod[_build].h:
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
FILE (MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}/include")

CHECK_MULTI_INCLUDE_FILES(
    "dlfcn.h" "fcntl.h" "limits.h" "malloc.h" "memory.h" "strings.h" "sys/ioctl.h" "unistd.h"
)

CHECK_MULTI_FUNCTIONS_EXISTS(
    "posix_memalign" "setenv" "snprintf" "srandom"
)

CHECK_SYMBOL_EXISTS(RTLD_GLOBAL dlfcn.h HAVE_RTLD_GLOBAL)
CHECK_SYMBOL_EXISTS(srandom math.h SRANDOM_IN_MATH_H)

ADD_DEFINITIONS("-DHAVE_CONFIG_H")
# always define MIKMOD_BUILD when building libmikmod
ADD_DEFINITIONS("-DMIKMOD_BUILD")

SET (MIKMOD_LIB_MODULES
    drivers/drv_AF.c
    drivers/drv_aiff.c
    drivers/drv_aix.c
    drivers/drv_alsa.c
    drivers/drv_dart.c
    drivers/drv_ds.c
    drivers/drv_esd.c
    drivers/drv_gp32.c
    drivers/drv_hp.c
    drivers/drv_mac.c
    drivers/drv_nas.c
    drivers/drv_nos.c
    drivers/drv_openal.c
    drivers/drv_oss.c
    drivers/drv_osx.c
    drivers/drv_pipe.c
    drivers/drv_raw.c
    drivers/drv_sam9407.c
    drivers/drv_sdl.c
    drivers/drv_sgi.c
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

REPLACE_FUNCTIONS_FROMDIR(MIKMOD_LIB_MODULES
                          posix
                          memcmp strcasecmp strdup strstr)

SET (LIBMIKMOD_LIBS)

ADD_LIBRARY (mikmod
    SHARED
    ${MIKMOD_LIB_MODULES}
    )

LIST (APPEND LIBMIKMOD_LIBS "mikmod")
IF (ENABLE_STATIC)
    ADD_LIBRARY (mikmod-static
        STATIC
        ${MIKMOD_LIB_MODULES}
    )
    LIST (APPEND LIBMIKMOD_LIBS "mikmod-static")

    SET_TARGET_PROPERTIES(
        mikmod-static
        PROPERTIES OUTPUT_NAME "mikmod"
    )
    SET_TARGET_PROPERTIES(
        ${LIBMIKMOD_LIBS}
        PROPERTIES CLEAN_DIRECT_OUTPUT 1
    )
ENDIF (ENABLE_STATIC)

SET_TARGET_PROPERTIES(mikmod
    PROPERTIES VERSION 3.2.0 SOVERSION 3
    )
IF(WIN32)
    SET_TARGET_PROPERTIES(mikmod
        PROPERTIES DEFINE_SYMBOL DLL_EXPORT
    )
ENDIF(WIN32)

IF (UNIX AND ENABLE_PIPE)
    SET(DRV_PIPE 1)
ENDIF(UNIX AND ENABLE_PIPE)
IF (UNIX AND ENABLE_STDOUT)
    SET(DRV_STDOUT 1)
ENDIF(UNIX AND ENABLE_STDOUT)

SET(DRV_AIFF 1)

IF (ENABLE_SIMD)
    SET(MIKMOD_SIMD 1)
ENDIF (ENABLE_SIMD)

IF (ENABLE_DL)
    SET(MIKMOD_DYNAMIC 1)
ENDIF (ENABLE_DL)

SET(EXTRA_LIBS )

INCLUDE (FindThreads)

IF (CMAKE_THREAD_LIBS_INIT)
  IF (ENABLE_THREADS)
    LIST (APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})
    IF (CMAKE_USE_PTHREADS_INIT)
        SET (HAVE_PTHREAD 1)
    ENDIF (CMAKE_USE_PTHREADS_INIT)
  ENDIF (ENABLE_THREADS)
ENDIF (CMAKE_THREAD_LIBS_INIT)

INCLUDE(CheckLibraryExists)

IF (ENABLE_ESD)
    PKG_CHECK_MODULES(PKG_ESD esound)
    IF(PKG_ESD_FOUND)
        SET(DRV_ESD 1)
        SET(ESD_LIBRARIES ${PKG_ESD_LDFLAGS})
        ADD_DEFINITIONS (${PKG_ESD_CFLAGS})
    ELSE(PKG_ESD_FOUND)
        find_path(ESD_INCLUDE_DIR esd.h)
        find_library(ESD_LIBRARIES esd)
        IF (ESD_LIBRARIES STREQUAL "ESD_LIBRARIES-NOTFOUND")
            MESSAGE(STATUS "Could NOT find EsounD.")
            SET (ENABLE_ESD 0)
        ELSE()
            MESSAGE(STATUS "Found EsounD: ${ESD_LIBRARIES}")
            SET(DRV_ESD 1)
            INCLUDE_DIRECTORIES(${ESD_INCLUDE_DIR})
        ENDIF()
    ENDIF(PKG_ESD_FOUND)
ENDIF(ENABLE_ESD)

IF (ENABLE_OSS)
CHECK_MULTI_INCLUDE_FILES("machine/soundcard.h" "sys/soundcard.h")
IF ("${HAVE_MACHINE_SOUNDCARD_H}" OR "${HAVE_SYS_SOUNDCARD_H}")
    SET (DRV_OSS 1)
ENDIF ("${HAVE_MACHINE_SOUNDCARD_H}" OR "${HAVE_SYS_SOUNDCARD_H}")
ENDIF (ENABLE_OSS)

IF (ENABLE_ALSA)
include(FindALSA)
IF (ALSA_FOUND)
    SET (HAVE_ALSA_ASOUNDLIB_H "1")
    ADD_DEFINITIONS("-DHAVE_ALSA_ASOUNDLIB_H=1")
    SET (DRV_ALSA 1)
ENDIF (ALSA_FOUND)
ENDIF(ENABLE_ALSA)

IF (ENABLE_SDL)
SET (SDL_BUILDING_LIBRARY TRUE)
include(FindSDL)
IF (SDL_FOUND)
    SET (DRV_SDL 1)
ENDIF (SDL_FOUND)
ENDIF(ENABLE_SDL)

IF (ENABLE_OPENAL)
include(FindOpenAL)
IF (OPENAL_FOUND)
    SET (DRV_OPENAL 1)
    CHECK_MULTI_INCLUDE_FILES("AL/al.h" "AL/alc.h")
    IF (HAVE_AL_AL_H)
        # alext.h depends on al.h and alc.h: cmake does
        # not handle it and CHECK_INCLUDE_FILE fails.
        CHECK_C_SOURCE_COMPILES("#include <AL/al.h>
                                 #include <AL/alc.h>
                                 #include <AL/alext.h>
                                 int main() {return 0;}" HAVE_AL_ALEXT_H)
    ENDIF (HAVE_AL_AL_H)
ENDIF (OPENAL_FOUND)
ENDIF(ENABLE_OPENAL)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/mikmod.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/mikmod.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/mikmod.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/mikmod_build.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/mikmod.h.in" "${CMAKE_CURRENT_BINARY_DIR}/macintosh/mikmod.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/mikmod.h.in" "${CMAKE_CURRENT_BINARY_DIR}/macintosh/mikmod_build.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/mikmod.h.in" "${CMAKE_CURRENT_BINARY_DIR}/macosx/mikmod.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/mikmod.h.in" "${CMAKE_CURRENT_BINARY_DIR}/macosx/mikmod_build.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/mikmod.h.in" "${CMAKE_CURRENT_BINARY_DIR}/win32/mikmod.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/mikmod.h.in" "${CMAKE_CURRENT_BINARY_DIR}/win32/mikmod_build.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/macosx/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/macosx/Info.plist")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/macosx/InfoPlist.strings.in" "${CMAKE_CURRENT_BINARY_DIR}/macosx/InfoPlist.strings")

SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix "\${prefix}")
SET(libdir "\${exec_prefix}/lib${LIB_SUFFIX}")
SET(includedir "\${prefix}/include")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/libmikmod-config.in" "${CMAKE_CURRENT_BINARY_DIR}/libmikmod-config" @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/libmikmod.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/libmikmod.pc" @ONLY)

IF (ENABLE_OPENAL)
    LIST (APPEND EXTRA_LIBS ${OPENAL_LIBRARY})
ENDIF (ENABLE_OPENAL)

IF (ENABLE_SDL)
    LIST (APPEND EXTRA_LIBS ${SDL_LIBRARY})
ENDIF (ENABLE_SDL)

IF (ENABLE_DL)
# add -ldl but only if a driver using dlopen() is active
  IF(ENABLE_ALSA OR ENABLE_ESD)
    LIST (APPEND EXTRA_LIBS ${CMAKE_DL_LIBS})
  ENDIF()

ELSE(ENABLE_DL)
# add alsa and/or esd libs but only when not dlopen()ing
  IF(ENABLE_ALSA)
    LIST (APPEND EXTRA_LIBS ${ALSA_LIBRARIES})
  ENDIF()

  IF(ENABLE_ESD)
    LIST (APPEND EXTRA_LIBS ${ESD_LIBRARIES})
  ENDIF()
ENDIF (ENABLE_DL)

IF (UNIX)
    SET(MATH_LIB "m")
ELSE(UNIX)
    SET(MATH_LIB )
ENDIF(UNIX)

LIST (APPEND EXTRA_LIBS ${MATH_LIB})

FOREACH (TGT ${LIBMIKMOD_LIBS})
    TARGET_LINK_LIBRARIES ("${TGT}"
        ${EXTRA_LIBS})
    INCLUDE_DIRECTORIES(AFTER ${ALSA_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIR} ${SDL_INCLUDE_DIR})
ENDFOREACH (TGT)

INSTALL(
    TARGETS ${LIBMIKMOD_LIBS}
    LIBRARY DESTINATION "lib${LIB_SUFFIX}"
    ARCHIVE DESTINATION "lib${LIB_SUFFIX}"
)

INSTALL(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/libmikmod-config"
    PERMISSIONS GROUP_EXECUTE GROUP_READ OWNER_EXECUTE OWNER_READ OWNER_WRITE WORLD_EXECUTE WORLD_READ
    DESTINATION "bin"
)

INSTALL(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/include/mikmod.h"
    DESTINATION
        "include"
)

INSTALL(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/libmikmod.pc"
    DESTINATION
        "lib${LIB_SUFFIX}/pkgconfig"
)
