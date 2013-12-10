
########### next target ###############

SET(mikmod_SRCS
   display.c
   marchive.c
   mikmod.c
   mlist.c
   mconfig.c
   mwindow.c
   mmenu.c
   mwidget.c
   mdialog.c
   mconfedit.c
   mutilities.c
   mplayer.c
   mlistedit.c
   rcfile.c
)

IF (NOT HAVE_USLEEP)
    LIST (APPEND mikmod_SRCS "musleep.c")
ENDIF (NOT HAVE_USLEEP)
IF (NOT HAVE_FNMATCH)
    LIST (APPEND mikmod_SRCS "mfnmatch.c")
ENDIF (NOT HAVE_FNMATCH)
IF (NOT HAVE_GETOPT_LONG_ONLY)
    LIST (APPEND mikmod_SRCS "mgetopt.c")
    LIST (APPEND mikmod_SRCS "mgetopt1.c")
ENDIF (NOT HAVE_GETOPT_LONG_ONLY)

include_directories(${MIKMOD_INCLUDE_DIR})

add_executable(mikmod ${mikmod_SRCS})

target_link_libraries(mikmod "ncurses" "pthread" ${MIKMOD_LIBRARIES})

install(TARGETS mikmod DESTINATION bin)

