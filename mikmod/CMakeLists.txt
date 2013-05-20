cmake_minimum_required(VERSION 2.8)

INCLUDE(Shlomif_Common)



SET (VERSION "3.1.5")

STRING (REGEX MATCHALL "([0-9]+)" VERSION_DIGITS "${VERSION}")

LIST(GET VERSION_DIGITS 0 CPACK_PACKAGE_VERSION_MAJOR)
LIST(GET VERSION_DIGITS 1 CPACK_PACKAGE_VERSION_MINOR)
LIST(GET VERSION_DIGITS 2 CPACK_PACKAGE_VERSION_PATCH)

SET(MIKMOD_VERSION "${VERSION}")
SET(MIKMOD_VERSION_MAJOR "${CPACK_PACKAGE_VERSION_MAJOR}")
SET(MIKMOD_VERSION_MINOR "${CPACK_PACKAGE_VERSION_MINOR}")
SET(MIKMOD_REVISION "${CPACK_PACKAGE_VERSION_PATCH}")

INCLUDE(FindCurses)
INCLUDE(CheckFunctionExists)

CHECK_MULTI_FUNCTIONS_EXISTS(
    "getopt_long_only"
    mkstemp srandom snprintf vsnprintf strerror
    usleep
    srandom
    fnmatch
    memcmp
    vprintf
)

IF (NOT CURSES_FOUND)
    MESSAGE(FATAL_ERROR "Curses not found.")
ENDIF (NOT CURSES_FOUND)
########### install files ###############

ADD_DEFINITIONS("-DHAVE_CONFIG_H")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# So it can find the includes when building libavl.avl.h.
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

# So it can find config.h
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

install(
    FILES
        mikmodrc
    DESTINATION
        "share"
)

#original Makefile.am contents follow:

#AUTOMAKE_OPTIONS = foreign
#SUBDIRS = src dos os2 win32
#
#pkgdata_DATA = mikmodrc
#
#EXTRA_DIST = mikmod-3.2.2.lsm \
#             convert_playlist mikmod.cfg $(pkgdata_DATA)

add_subdirectory(src)
add_subdirectory(dos)
add_subdirectory(os2)
add_subdirectory(win32)
