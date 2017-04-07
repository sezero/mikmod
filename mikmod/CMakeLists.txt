# if necessary, set CMAKE_PREFIX_PATH to the path where libmikmod
# is installed, which you can do on your cmake command line, like:
# cmake -DCMAKE_PREFIX_PATH=/path/to/libmikmod_dir ....

PROJECT(mikmod C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
LIST(APPEND CMAKE_MODULE_PATH "${mikmod_SOURCE_DIR}/cmake")

SET(VERSION "3.2.8")

STRING(REGEX MATCHALL "([0-9]+)" VERSION_DIGITS "${VERSION}")

LIST(GET VERSION_DIGITS 0 CPACK_PACKAGE_VERSION_MAJOR)
LIST(GET VERSION_DIGITS 1 CPACK_PACKAGE_VERSION_MINOR)
LIST(GET VERSION_DIGITS 2 CPACK_PACKAGE_VERSION_PATCH)

# package generation (make package[_source])
SET(CPACK_PACKAGE_NAME "mikmod")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MikMod - a module player")
SET(CPACK_PACKAGE_VENDOR "Shlomi Fish")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

SET(base_with_ver "mikmod-[0-9]+\\\\.[0-9]+\\\\.[0-9]+")
SET(CPACK_SOURCE_IGNORE_FILES
    "/_CPack_Packages/"
    "/CMakeFiles/"
    "/.deps/"
    "^${base_with_ver}(-Source|-Linux)?/"
    "${base_with_ver}.tar\\\\.(gz|bz2|Z|lzma|xz)$"
    "\\\\.o$"
    "~$"
    "/\\\\.svn/"
    "/CMakeCache\\\\.txt$"
    "/CTestTestfile\\\\.cmake$"
    "/cmake_install\\\\.cmake$"
    "/CPackConfig\\\\.cmake$"
    "/CPackSourceConfig\\\\.cmake$"
    "/tags$"
    "^config\\\\.h$"
    "/install_manifest\\\\.txt$"
    "/Testing/"
    "ids-whitelist\\\\.txt"
    "/_Inline/"
    "/(B|build|BUILD)/"
    "/autom4te.cache/"
)

INCLUDE(CPack)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCSourceCompiles)
include(mik_macros)

CHECK_MULTI_INCLUDE_FILES(
    "ncurses.h" "curses.h" "ncurses/curses.h" "termios.h"
    "fcntl.h" "fnmatch.h" "inttypes.h" "limits.h" "memory.h" "sched.h"
    "sys/ioctl.h" "sys/param.h" "sys/wait.h" "sys/time.h" "sys/types.h"
    "sys/stat.h" "stdint.h" "stdlib.h" "string.h" "strings.h" "unistd.h"
    "pthread.h"
)
CHECK_SYMBOL_EXISTS(TIOCGWINSZ "sys/ioctl.h" GWINSZ_IN_SYS_IOCTL)
CHECK_SYMBOL_EXISTS(usleep unistd.h HAVE_USLEEP_PROTO)
IF (NOT HAVE_USLEEP_PROTO)
    CHECK_SYMBOL_EXISTS(usleep "sys/unistd.h" HAVE_USLEEP_PROTO)
ENDIF()

SET(EXTRA_LIBS )

find_path(MIKMOD_INCLUDE_DIR mikmod.h)
find_library(MIKMOD_LIBRARIES mikmod)
IF (MIKMOD_LIBRARIES STREQUAL "MIKMOD_LIBRARIES-NOTFOUND")
    MESSAGE(FATAL_ERROR "libmikmod not found.")
ELSE()
    MESSAGE(STATUS "Found MikMod: ${MIKMOD_LIBRARIES}")
ENDIF()

IF(UNIX OR APPLE)
  INCLUDE(FindCurses)
  IF(NOT CURSES_FOUND)
    MESSAGE(FATAL_ERROR "Curses not found.")
  ENDIF()

  IF(HAVE_NCURSES_H)
     SET(CURSES_HDR "ncurses.h")
  ELSEIF(HAVE_CURSES_H)
     SET(CURSES_HDR "curses.h")
  ELSEIF(HAVE_NCURSES_CURSES_H)
     SET(CURSES_HDR "ncurses/curses.h")
  ELSE()
      MESSAGE(FATAL_ERROR "Neither ncurses.h nor curses.h found.")
  ENDIF()

  SET(CMAKE_REQUIRED_LIBRARIES ${CURSES_LIBRARY})
  CHECK_FUNCTION_EXISTS (resizeterm HAVE_NCURSES_RESIZETERM)
  CHECK_C_SOURCE_COMPILES(
   "#include <${CURSES_HDR}>
     int main(void) {
#ifdef ACS_ULCORNER
     return ACS_ULCORNER;
#endif
     return 0; }"
     CURSES_LINKSOK
  )
  IF(CURSES_LINKSOK)
    LIST (APPEND EXTRA_LIBS ${CURSES_LIBRARY})
  ELSE()
    find_library(TINFO_LIBRARY tinfo)
    IF(TINFO_LIBRARY STREQUAL "TINFO_LIBRARY-NOTFOUND")
      MESSAGE(FATAL_ERROR "libtinfo needed for ncurses, but not found.")
    ELSE()
      MESSAGE(STATUS "Found libtinfo: ${TINFO_LIBRARY}")
      SET(CMAKE_REQUIRED_LIBRARIES ${CURSES_LIBRARY} ${TINFO_LIBRARY})
      CHECK_C_SOURCE_COMPILES(
       "#include <${CURSES_HDR}>
         int main(void) {
#ifdef ACS_ULCORNER
         return ACS_ULCORNER;
#endif
         return 0; }"
         TINFO_LINKSOK
      )
      IF(TINFO_LINKSOK)
        LIST (APPEND EXTRA_LIBS ${CURSES_LIBRARY})
        LIST (APPEND EXTRA_LIBS ${TINFO_LIBRARY})
      ELSE()
        MESSAGE(FATAL_ERROR "failed linking to ncurses library.")
      ENDIF()
    ENDIF()
  ENDIF()
ENDIF()

SET(HAVE_PTHREAD)
IF (NOT WIN32)
 INCLUDE(FindThreads)
 IF (CMAKE_USE_PTHREADS_INIT)
    SET (HAVE_PTHREAD 1)
    IF (CMAKE_THREAD_LIBS_INIT)
        LIST (APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})
    ENDIF()
 ENDIF()
ENDIF()

CHECK_MULTI_FUNCTIONS_EXISTS(
    "getopt_long_only"
    mkstemp srandom snprintf vsnprintf strerror
    usleep
    srandom
    fnmatch
    memcmp
    vprintf
)

CHECK_C_SOURCE_COMPILES(
 "#include <sys/types.h>
  #include <signal.h>
  int main(void) { return *(signal(0,0))(0) == 1; }"
  RETSIGTYPE_INT
)
if(RETSIGTYPE_INT)
  set(RETSIGTYPE int)
else()
  set(RETSIGTYPE void)
endif()
MESSAGE(STATUS "Return type of signal handlers: ${RETSIGTYPE}")

SET(CMAKE_REQUIRED_INCLUDES ${MIKMOD_INCLUDE_DIR})
SET(CMAKE_REQUIRED_LIBRARIES ${MIKMOD_LIBRARIES})
CHECK_FUNCTION_EXISTS (MikMod_free HAVE_MIKMOD_FREE)

########### compiler flags ##############

SET(COMPILER_FLAGS_TO_CHECK
    "-Wall" "-Werror=implicit-function-declaration"
)

IF (CPU_ARCH)
    LIST(APPEND COMPILER_FLAGS_TO_CHECK "-march=${CPU_ARCH}")
ENDIF(CPU_ARCH)

SET (IDX 1)
FOREACH (CFLAG_TO_CHECK ${COMPILER_FLAGS_TO_CHECK})
    SET (FLAG_EXISTS_VAR "FLAG_EXISTS_${IDX}")
    MATH (EXPR IDX "${IDX} + 1")
    CHECK_C_COMPILER_FLAG("${CFLAG_TO_CHECK}" ${FLAG_EXISTS_VAR})
    IF (${FLAG_EXISTS_VAR})
        ADD_DEFINITIONS(${CFLAG_TO_CHECK})
    ENDIF (${FLAG_EXISTS_VAR})
ENDFOREACH(CFLAG_TO_CHECK)

########### install files ###############

ADD_DEFINITIONS("-DHAVE_CONFIG_H")
SET (PACKAGE_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/mikmod")

configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

# So it can find config.h
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_BINARY_DIR})

install(
    FILES
        mikmodrc
    DESTINATION
        "share/mikmod"
)

add_subdirectory(src)
