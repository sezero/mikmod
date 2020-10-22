## Process this file with automake to produce Makefile.in

AM_CFLAGS = @LIBMIKMOD_CFLAGS@

bin_PROGRAMS = mikmod

man_MANS = mikmod.1

mikmod_SOURCES = \
	display.c marchive.c mikmod.c mlist.c mconfig.c mwindow.c mmenu.c \
	mwidget.c mdialog.c mconfedit.c mutilities.c mplayer.c mlistedit.c \
	rcfile.c

noinst_HEADERS = \
	display.h keys.h marchive.h mconfedit.h mconfig.h mdialog.h mlist.h \
	mlistedit.h mmenu.h mplayer.h mthreads.h mutilities.h mwidget.h \
	mwindow.h player.h rcfile.h

EXTRA_mikmod_SOURCES = \
	mfnmatch.c getopt_long.c musleep.c

EXTRA_DIST = CMakeLists.txt \
	dosvideo.inc os2video.inc winvideo.inc mfnmatch.h getopt_long.h $(man_MANS)

mikmod_LDFLAGS = @LIBMIKMOD_LDADD@
mikmod_LDADD = @EXTRA_OBJ@ @LIBMIKMOD_LIBS@ @PLAYER_LIB@

mikmod_DEPENDENCIES = @EXTRA_OBJ@

getopt_long.o: $(srcdir)/getopt_long.c $(srcdir)/getopt_long.h
	$(COMPILE) -o $@ -c $(srcdir)/getopt_long.c
mfnmatch.o: $(srcdir)/mfnmatch.c $(srcdir)/mfnmatch.h
	$(COMPILE) -o $@ -c $(srcdir)/mfnmatch.c
musleep.o: $(srcdir)/musleep.c
	$(COMPILE) -o $@ -c $(srcdir)/musleep.c
