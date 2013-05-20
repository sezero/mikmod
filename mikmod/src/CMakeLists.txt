
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
   mfnmatch.c
   mgetopt.c
   mgetopt1.c
)

IF (NOT HAVE_USLEEP)
    LIST (APPEND mikmod_SRCS "musleep.c")
ENDIF (NOT HAVE_USLEEP)

find_library(LIBMIKMOD_LIB "mikmod")
IF(LIBMIKMOD_LIB STREQUAL "LIBMIKMOD_LIB-NOTFOUND")
    # Do nothing.
    MESSAGE (FATAL_ERROR "libmikmod must be present")
ELSE(LIBMIKMOD_LIB STREQUAL "LIBMIKMOD_LIB-NOTFOUND")
    SET(LIBMIKMOD_LIB_LIST ${LIBMIKMOD_LIB})
ENDIF(LIBMIKMOD_LIB STREQUAL "LIBMIKMOD_LIB-NOTFOUND")

include_directories(${libmikmod_INCLUDE_DIRS})

add_executable(mikmod ${mikmod_SRCS})

target_link_libraries(mikmod "ncurses" "pthread" ${LIBMIKMOD_LIB_LIST})

install(TARGETS mikmod DESTINATION bin)


########### install files ###############




#original Makefile.am contents follow:

### Process this file with automake to produce Makefile.in
#
#INCLUDES = @LIBMIKMOD_CFLAGS@
#
#bin_PROGRAMS = mikmod
#
#man_MANS = mikmod.1
#
#mikmod_SOURCES = \
#	display.c marchive.c mikmod.c mlist.c mconfig.c mwindow.c mmenu.c \
#	mwidget.c mdialog.c mconfedit.c mutilities.c mplayer.c mlistedit.c \
#	rcfile.c \
#	mfnmatch.c mgetopt.c mgetopt1.c musleep.c
#
#noinst_HEADERS = \
#	display.h keys.h marchive.h mconfedit.h mconfig.h mdialog.h mlist.h \
#	mlistedit.h mmenu.h mplayer.h mthreads.h mutilities.h mwidget.h \
#	mwindow.h player.h rcfile.h
#
#EXTRA_DIST = \
#	mfnmatch.h mgetopt.h $(man_MANS)
#
#mikmod_LDFLAGS = @LIBMIKMOD_LDADD@
#mikmod_LDADD = @EXTRA_OBJ@ @LIBMIKMOD_LIBS@ @PLAYER_LIB@
#
#mikmod_DEPENDENCIES = @EXTRA_OBJ@
#
#mgetopt.o:       $(srcdir)/mgetopt.c $(srcdir)/mgetopt.h
#	$(COMPILE) -o $@ -c $(srcdir)/mgetopt.c
#mgetopt1.o:      $(srcdir)/mgetopt1.c $(srcdir)/mgetopt.h
#	$(COMPILE) -o $@ -c $(srcdir)/mgetopt1.c
#mfnmatch.o:      $(srcdir)/mfnmatch.c $(srcdir)/mfnmatch.h
#	$(COMPILE) -o $@ -c $(srcdir)/mfnmatch.c
#musleep.o:       $(srcdir)/musleep.c
#	$(COMPILE) -o $@ -c $(srcdir)/musleep.c
