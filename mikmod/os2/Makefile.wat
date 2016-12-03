# Makefile for OS/2 using Watcom compiler.
#
# wmake -f Makefile.wat

CC=wcc386
INCLUDES=-I..\os2 -I..\src
CPPFLAGS=-DHAVE_FCNTL_H -DHAVE_LIMITS_H -DHAVE_SYS_IOCTL_H -DHAVE_SYS_TIME_H -DHAVE_STRERROR -DHAVE_SNPRINTF
# for an exe using mikmod.dll: link to mikmod.lib
# for a statically linked exe: link to mikmod_static.lib which, in turn, requires mmpm2.lib
LIBS=mikmod.lib
#LIBS=mikmod_static.lib mmpm2.lib
CFLAGS = -bt=os2 -bm -fp5 -fpi87 -mf -oeatxh -w4 -zp8 -ei -q
# -5s  :  Pentium stack calling conventions.
# -5r  :  Pentium register calling conventions.
CFLAGS+= -5s

.SUFFIXES:
.SUFFIXES: .obj .c

AOUT=mikmod.exe
COMPILE=$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES)

OBJ = display.obj marchive.obj mconfedit.obj mconfig.obj mdialog.obj mikmod.obj mlist.obj mlistedit.obj &
      mmenu.obj mplayer.obj mutilities.obj mwidget.obj mwindow.obj rcfile.obj
EXTRA_OBJ = mgetopt.obj mgetopt1.obj mfnmatch.obj

all: $(AOUT)

$(AOUT): $(OBJ) $(EXTRA_OBJ)
	wlink N $(AOUT) SYS OS2V2 LIBR {$(LIBS)} F {$(OBJ)} F {$(EXTRA_OBJ)}

clean:
	FOR %F IN ( $(AOUT) $(OBJ) $(EXTRA_OBJ) ) DO IF EXIST %F ERASE %F

display.obj: ../src\display.c ..\src\display.h ..\src\player.h ..\src\mconfig.h &
		..\src\rcfile.h ..\src\mlist.h ..\src\mutilities.h ..\src\mwindow.h &
		..\src\mconfedit.h ..\src\mmenu.h ..\src\keys.h ..\src\mplayer.h &
		..\src\mlistedit.h
	$(COMPILE) -fo=$^@ ..\src\display.c
marchive.obj: ..\src\marchive.c ..\src\mfnmatch.h ..\src\mlist.h ..\src\marchive.h &
		..\src\mconfig.h ..\src\rcfile.h ..\src\mutilities.h ..\src\display.h
	$(COMPILE) -fo=$^@ ..\src\marchive.c
mconfedit.obj: ..\src\mconfedit.c ..\src\rcfile.h ..\src\mconfig.h ..\src\mconfedit.h &
		..\src\mmenu.h ..\src\mwindow.h ..\src\mlist.h ..\src\mdialog.h &
		..\src\mwidget.h ..\src\mutilities.h
	$(COMPILE) -fo=$^@ ..\src\mconfedit.c
mconfig.obj: ..\src\mconfig.c ..\src\player.h ..\src\mconfig.h ..\src\rcfile.h &
		..\src\mwindow.h ..\src\mlist.h ..\src\mutilities.h
	$(COMPILE) -fo=$^@ ..\src\mconfig.c
mdialog.obj: ..\src\mdialog.c ..\src\mwidget.h ..\src\mwindow.h ..\src\mconfig.h &
		..\src\rcfile.h ..\src\mdialog.h ..\src\display.h ..\src\mutilities.h
	$(COMPILE) -fo=$^@ ..\src\mdialog.c
mikmod.obj: ..\src\mikmod.c ..\src\mgetopt.h ..\src\player.h ..\src\mutilities.h &
		..\src\display.h ..\src\rcfile.h ..\src\mconfig.h ..\src\mlist.h &
		..\src\mlistedit.h ..\src\mmenu.h ..\src\mwindow.h ..\src\marchive.h &
		..\src\mdialog.h ..\src\mwidget.h ..\src\mplayer.h ..\src\keys.h
	$(COMPILE) -fo=$^@ ..\src\mikmod.c
mlist.obj: ..\src\mlist.c ..\src\mfnmatch.h ..\src\mlist.h ..\src\marchive.h &
		..\src\mutilities.h
	$(COMPILE) -fo=$^@ ..\src\mlist.c
mlistedit.obj: ..\src\mlistedit.c ..\src\mlistedit.h ..\src\mmenu.h ..\src\mwindow.h &
		..\src\mconfig.h ..\src\rcfile.h ..\src\mlist.h ..\src\player.h &
		..\src\mdialog.h ..\src\mwidget.h ..\src\mconfedit.h ..\src\marchive.h &
		..\src\keys.h ..\src\display.h ..\src\mutilities.h
	$(COMPILE) -fo=$^@ ..\src\mlistedit.c
mmenu.obj: ..\src\mmenu.c ..\src\display.h ..\src\mmenu.h ..\src\mwindow.h &
		..\src\mconfig.h ..\src\rcfile.h ..\src\mdialog.h ..\src\mwidget.h &
		..\src\keys.h ..\src\mutilities.h
	$(COMPILE) -fo=$^@ ..\src\mmenu.c
mplayer.obj: ..\src\mplayer.c ..\src\mplayer.h ..\src\mthreads.h ..\src\mconfig.h &
		..\src\rcfile.h ..\src\mutilities.h
	$(COMPILE) -fo=$^@ ..\src\mplayer.c
mutilities.obj: ..\src\mutilities.c ..\src\player.h ..\src\mlist.h ..\src\marchive.h &
		..\src\mutilities.h
	$(COMPILE) -fo=$^@ ..\src\mutilities.c
mwidget.obj: ..\src\mwidget.c ..\src\display.h ..\src\player.h ..\src\mwindow.h &
		..\src\mconfig.h ..\src\rcfile.h ..\src\mwidget.h ..\src\keys.h &
		..\src\mutilities.h
	$(COMPILE) -fo=$^@ ..\src\mwidget.c
mwindow.obj: ..\src\mwindow.c ..\src\display.h ..\src\player.h ..\src\mwindow.h &
		..\src\mconfig.h ..\src\rcfile.h ..\src\mutilities.h ..\src\keys.h &
		..\src\mthreads.h ..\src\os2video.inc
	$(COMPILE) -fo=$^@ ..\src\mwindow.c
rcfile.obj: ..\src\rcfile.c ..\src\rcfile.h ..\src\mutilities.h
	$(COMPILE) -fo=$^@ ..\src\rcfile.c

mgetopt.obj: ..\src\mgetopt.c ..\src\mgetopt.h
	$(COMPILE) -fo=$^@ ..\src\mgetopt.c
mgetopt1.obj: ..\src\mgetopt1.c ..\src\mgetopt.h
	$(COMPILE) -fo=$^@ ..\src\mgetopt1.c
mfnmatch.obj: ..\src\mfnmatch.c ..\src\mfnmatch.h
	$(COMPILE) -fo=$^@ ..\src\mfnmatch.c
musleep.obj: ..\src\musleep.c
	$(COMPILE) -fo=$^@ ..\src\musleep.c
