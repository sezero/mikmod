# Makefile for OS/2 using Open Watcom compiler.
#
# wmake -f Makefile.wat
#
# to statically link to mikmod:
# wmake -f Makefile.wat target=static

!ifndef target
target = dynamic
!endif

CC=wcc386

INCLUDES=-I.
CPPFLAGS=-DHAVE_CONFIG_H

!ifneq target static
LIBS=mikmod.lib
!else
CPPFLAGS+= -DMIKMOD_STATIC
LIBS=mikmod_static.lib mmpm2.lib
!endif

CFLAGS = -bt=os2 -bm -fp5 -fpi87 -mf -oeatxh -w4 -zp8 -ei -zq
# newer OpenWatcom versions enable W303 by default.
CFLAGS+= -wcd=303
# -5s  :  Pentium stack calling conventions.
# -5r  :  Pentium register calling conventions.
CFLAGS+= -5s

.SUFFIXES:
.SUFFIXES: .obj .c

AOUT=mikmod.exe
COMPILE=$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES)

OBJ = display.obj marchive.obj mconfedit.obj mconfig.obj mdialog.obj mikmod.obj mlist.obj mlistedit.obj &
      mmenu.obj mplayer.obj mutilities.obj mwidget.obj mwindow.obj rcfile.obj
EXTRA_OBJ = getopt_long.obj

all: $(AOUT)

$(AOUT): $(OBJ) $(EXTRA_OBJ)
	wlink N $(AOUT) SYS OS2V2 OP q LIBR {$(LIBS)} F {$(OBJ) $(EXTRA_OBJ)}

.c: ../src
.c.obj:
	$(COMPILE) -fo=$^@ $<

distclean: clean .symbolic
	rm -f $(AOUT)
clean: .symbolic
	rm -f *.obj
