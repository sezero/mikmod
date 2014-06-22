
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
ENDIF()
IF (NOT HAVE_FNMATCH)
    LIST (APPEND mikmod_SRCS "mfnmatch.c")
ENDIF()
IF (NOT HAVE_GETOPT_LONG_ONLY)
    LIST (APPEND mikmod_SRCS "mgetopt.c")
    LIST (APPEND mikmod_SRCS "mgetopt1.c")
ENDIF()

include_directories(${MIKMOD_INCLUDE_DIR})

add_executable(mikmod ${mikmod_SRCS})

target_link_libraries(mikmod ${MIKMOD_LIBRARIES} ${EXTRA_LIBS})

install(TARGETS mikmod DESTINATION bin)

