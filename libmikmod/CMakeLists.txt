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
#
# (This copyright notice applies only to this file)


INCLUDE( "${CMAKE_CURRENT_SOURCE_DIR}/Common.cmake" )

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# This is the equivalent to perform a "make dist"/"make distdir" etc.
SET(CPACK_PACKAGE_NAME "libmikmod")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libmikmod - a module file playing and sound library")
SET(CPACK_PACKAGE_VENDOR "Shlomi Fish")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER")

# Process and extract the version number.
# FILE( READ "ver.txt" VERSION)

# CHOMP (VERSION)
SET (VERSION "3.3.0")

STRING (REGEX MATCHALL "([0-9]+)" VERSION_DIGITS "${VERSION}")

LIST(GET VERSION_DIGITS 0 CPACK_PACKAGE_VERSION_MAJOR)
LIST(GET VERSION_DIGITS 1 CPACK_PACKAGE_VERSION_MINOR)
LIST(GET VERSION_DIGITS 2 CPACK_PACKAGE_VERSION_PATCH)

SET(LIBMIKMOD_VERSION_MAJOR "${CPACK_PACKAGE_VERSION_MAJOR}")
SET(LIBMIKMOD_VERSION_MINOR "${CPACK_PACKAGE_VERSION_MINOR}")
SET(LIBMIKMOD_REVISION "${CPACK_PACKAGE_VERSION_PATCH}")
MESSAGE( "LIBMIKMOD_REVISION == <${LIBMIKMOD_REVISION}>")
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
    "/Makefile$"
    "/CTestTestfile\\\\.cmake$"
    "/cmake_install\\\\.cmake$"
    "/CPackConfig\\\\.cmake$"
    "/CPackSourceConfig\\\\.cmake$"
    "/tags$"
    "/freecell-solver-config$"
    "/libfreecell-solver\\\\.pc$"
    "/([0-9]+)\\\\.board$"
    "/config\\\\.h$"
    "/install_manifest\\\\.txt$"
    "/t/card-test-(parse|render)\\\\.c$"
    "/Testing/"
    "ids-whitelist\\\\.txt"
    "/_Inline/"
    "/(build|BUILD)/"
    "/m4/"
    "/autom4te.cache/"
)

IF(WIN32 AND NOT UNIX)
    # There is a bug in NSI that does not handle full unix paths properly. Make
    # sure there is at least one set of four (4) backlasshes.
    SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\dondorf-king.bmp")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\fc-solve.exe")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\mikmod.shlomifish.org")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\mikmod.shlomifish.org")
    SET(CPACK_NSIS_DISPLAY_NAME "libmikmod")
    SET(CPACK_NSIS_CONTACT "shlomif@cpan.org")
    SET(CPACK_NSIS_MODIFY_PATH ON)
    # Setting for NSIS :
    SET(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\fc-solve.ico")
    SET(CPACK_NSIS_MUI_UNIICON ${CPACK_NSIS_MUI_ICON})
    SET(CPACK_PACKAGE_ICON ${CPACK_NSIS_MUI_ICON})
    SET(CPACK_NSIS_MODIFY_PATH "ON")
ELSE(WIN32 AND NOT UNIX)
    # SET(CPACK_STRIP_FILES "fc-solve")
    # SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)

### This is to set the RPATH correctly, so when installed under a prefix
### the executables will find the libraries.
###
### See:
###
### http://www.cmake.org/Wiki/CMake_RPATH_handling
###
### (Taken from that wiki page)

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

SET (ENABLE_AF 1 CACHE BOOL "Include the Audio File driver")
SET (ENABLE_DL 1 CACHE BOOL "Load ALSA and Ultra Drivers at Runtime")
SET (ENABLE_THREADS 1 CACHE BOOL "build a thread-safe version of libmikmod")

INCLUDE(CPack)

include(CheckFunctionExists)
INCLUDE(CheckCCompilerFlag)
INCLUDE(FindThreads)

ADD_SUBDIRECTORY ("dlapi")
ADD_SUBDIRECTORY ("docs")
ADD_SUBDIRECTORY ("dos")
ADD_SUBDIRECTORY ("drivers")
ADD_SUBDIRECTORY ("include")
ADD_SUBDIRECTORY ("libmikmod")
ADD_SUBDIRECTORY ("loaders")
ADD_SUBDIRECTORY ("macintosh")
ADD_SUBDIRECTORY ("mmio")
ADD_SUBDIRECTORY ("os2")
ADD_SUBDIRECTORY ("playercode")
ADD_SUBDIRECTORY ("posix")
ADD_SUBDIRECTORY ("win32")

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(COMPILER_FLAGS_TO_CHECK
    "-Wall" "-Werror=implicit-function-declaration"
    # Cancelling because libmikmod does not like it.
    # Should be restored in the future.
    # "-fvisibility=hidden"
)

IF (CPU_ARCH)
    LIST(APPEND COMPILER_FLAGS_TO_CHECK "-march=${CPU_ARCH}")
ENDIF(CPU_ARCH)

SET (IS_DEBUG "")
IF ((CMAKE_BUILD_TYPE STREQUAL debug) OR (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo))
    SET (IS_DEBUG 1)

    # Removed these flags because they emitted spurious warnings, which were of
    # no use to us:
    # "-Winline"
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
        LIST(APPEND MY_LINK_FLAGS "${CFLAG_TO_CHECK}")
    ENDIF (${FLAG_EXISTS_VAR})
ENDFOREACH(CFLAG_TO_CHECK)

# So it can find the includes when building libavl.avl.h.
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# So it can find config.h and mikmod.h
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
FILE (MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}/include")

CHECK_MULTI_INCLUDE_FILES(
    "fcntl.h" "malloc.h" "memory.h" "strings.h" "sys/ioctl.h" "unistd.h"
)


ADD_DEFINITIONS("-DHAVE_CONFIG_H")


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
    drivers/drv_oss.c
    drivers/drv_osx.c
    drivers/drv_pipe.c
    drivers/drv_raw.c
    drivers/drv_sam9407.c
    drivers/drv_sgi.c
    drivers/drv_stdout.c
    drivers/drv_sun.c
    drivers/drv_ultra.c
    drivers/drv_wav.c
    drivers/drv_win.c
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

SET (FREECELL_SOLVER_LIBS)

ADD_LIBRARY (mikmod
    SHARED
    ${MIKMOD_LIB_MODULES}
    )

LIST (APPEND FREECELL_SOLVER_LIBS "mikmod")
IF (BUILD_STATIC_LIBRARY)
    ADD_LIBRARY (mikmod-static
        STATIC
        ${MIKMOD_LIB_MODULES}
    )
    LIST (APPEND FREECELL_SOLVER_LIBS "mikmod-static")

    SET_TARGET_PROPERTIES(
        mikmod-static
        PROPERTIES OUTPUT_NAME "mikmod"
    )
    SET_TARGET_PROPERTIES(
        ${FREECELL_SOLVER_LIBS}
        PROPERTIES CLEAN_DIRECT_OUTPUT 1
    )
ENDIF (BUILD_STATIC_LIBRARY)

SET_TARGET_PROPERTIES(mikmod
    PROPERTIES VERSION 3.0.0 SOVERSION 3
    )

SET(DRV_PIPE 1)
SET(DRV_AIFF 1)
SET(EXTRA_LIBS )

INCLUDE (FindThreads)

IF (CMAKE_THREAD_LIBS_INIT)
    LIST (APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})
    IF (CMAKE_USE_PTHREADS_INIT)
        SET (HAVE_PTHREAD 1)
    ENDIF (CMAKE_USE_PTHREADS_INIT)
ENDIF (CMAKE_THREAD_LIBS_INIT)

INCLUDE(CheckLibraryExists)

IF (ENABLE_AF)
    SET (ENABLE_AF "")
    CHECK_MULTI_INCLUDE_FILES("AF/AFlib.h")
    IF (HAVE_AF_AFLIB_H)
        CHECK_LIBRARY_EXISTS(AF AFOpenAudioConn '' ENABLE_AF)
    ENDIF (HAVE_AF_AFLIB_H)
ENDIF (ENABLE_AF)

IF (ENABLE_AF)
    SET (DRV_AF 1)
    LIST (APPEND EXTRA_LIBS "AF")
ENDIF (ENABLE_AF)

CHECK_MULTI_INCLUDE_FILES("machine/soundcard.h" "sys/soundcard.h")

IF ("${HAVE_MACHINE_SOUNDCARD_H}" OR "${HAVE_SYS_SOUNDCARD_H}")
    SET (DRV_OSS 1)
ENDIF ("${HAVE_MACHINE_SOUNDCARD_H}" OR "${HAVE_SYS_SOUNDCARD_H}")

include(FindALSA)

IF (ALSA_FOUND)
    SET (HAVE_ALSA_ASOUNDLIB_H "1")
    ADD_DEFINITIONS("-DHAVE_ALSA_ASOUNDLIB_H=1")
    SET (DRV_ALSA 1)
ENDIF (ALSA_FOUND)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/mikmod.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/include/mikmod.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/mikmod.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/include/mikmod_build.h")

LIST (APPEND EXTRA_LIBS ${ALSA_LIBRARIES})

FOREACH (TGT ${FREECELL_SOLVER_LIBS})
    TARGET_LINK_LIBRARIES ("${TGT}"
        ${EXTRA_LIBS})
    INCLUDE_DIRECTORIES(AFTER ${ALSA_INCLUDE_DIRS})
ENDFOREACH (TGT)

INSTALL(
    TARGETS ${FREECELL_SOLVER_LIBS}
    LIBRARY DESTINATION "lib${LIB_SUFFIX}"
    ARCHIVE DESTINATION "lib${LIB_SUFFIX}"
)

