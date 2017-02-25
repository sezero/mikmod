# Makefile for Win32 using Open Watcom compiler.
#
# wmake -f Makefile.wat
#
# to statically link to mikmod:
# wmake -f Makefile.wat target=static

!ifndef target
target = dynamic
!endif

CC=wcc386
!ifndef __UNIX__
INCLUDES=-I..\win32 -I..\src
!else
INCLUDES=-I../win32 -I../src
!endif
CPPFLAGS=-DHAVE_FCNTL_H -DHAVE_LIMITS_H -DHAVE_SYS_TIME_H -DHAVE_STRERROR -DHAVE_SNPRINTF -DHAVE_MKSTEMP

!ifneq target static
LIBS=mikmod.lib
!else
CPPFLAGS+= -DMIKMOD_STATIC
LIBS=mikmod_static.lib winmm.lib dsound.lib dxguid.lib
!endif

CFLAGS = -bt=nt -bm -fp5 -fpi87 -mf -oeatxh -w4 -zp8 -ei -zq
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
	wlink N $(AOUT) SYS NT LIBR {$(LIBS)} F {$(OBJ)} F {$(EXTRA_OBJ)}

.c.obj:
	$(COMPILE) -fo=$^@ $<

!ifndef __UNIX__
.c: ..\src
distclean: clean .symbolic
	@if exist $(AOUT) del $(AOUT)
clean: .symbolic
	@if exist *.obj del *.obj
!else
.c: ../src
distclean: clean .symbolic
	rm -f $(AOUT)
clean: .symbolic
	rm -f *.obj
!endif
