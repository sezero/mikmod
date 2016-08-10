# Makefile for OS/2 using Watcom compiler.
#
# wmake -f Makefile.wat
# - builds mikmod.dll and its import lib (mikmod.lib)
#
# wmake -f Makefile.wat target=static
# - builds the static library mikmod_static.lib

!ifndef target
target = dll
!endif

INCLUDES=-I..\os2 -I..\include
CPPFLAGS=-DMIKMOD_BUILD -DHAVE_FCNTL_H -DHAVE_LIMITS_H -DHAVE_MALLOC_H
#
# To build a debug version:
#CPPFLAGS+= -DMIKMOD_DEBUG

# MMPM/2 driver (will work with any OS/2 version starting from 2.1.)
CPPFLAGS+= -DDRV_OS2
# DART (Direct Audio Real Time) driver (uses less CPU time than the
#                          standard MMPM/2 drivers, requires Warp4.)
CPPFLAGS+= -DDRV_DART
# support for aiff file output:
CPPFLAGS+= -DDRV_AIFF
# support for wav file output:
CPPFLAGS+= -DDRV_WAV
# support for output raw data to a file:
CPPFLAGS+= -DDRV_RAW
# support for output to stdout (not needed by everyone)
#CPPFLAGS+= -DDRV_STDOUT

# disable the high quality mixer (build only with the standart mixer)
#CPPFLAGS+= -DNO_HQMIXER

# drv_os2 and drv_dart require mmpm2
LIBS = mmpm2.lib

CFLAGS = -bt=os2 -bm -fp5 -fpi87 -mf -oeatxh -w4 -ei -zp8
# -5s  :  Pentium stack calling conventions.
# -5r  :  Pentium register calling conventions.
CFLAGS+= -5s
DLLFLAGS=-bd

.SUFFIXES:
.SUFFIXES: .obj .c

DLLNAME=mikmod.dll
EXPNAME=mikmod.exp
LIBNAME=mikmod.lib
LIBSTATIC=mikmod_static.lib

!ifeq target static
BLD_TARGET=$(LIBSTATIC)
!else
CFLAGS+= $(DLLFLAGS)
BLD_TARGET=$(DLLNAME)
!endif

COMPILE=wcc386 $(CFLAGS) $(CPPFLAGS) $(INCLUDES)

OBJ=drv_os2.obj drv_dart.obj &
    drv_raw.obj drv_aiff.obj drv_wav.obj &
    drv_nos.obj drv_stdout.obj &
    load_669.obj load_amf.obj load_asy.obj load_dsm.obj load_far.obj load_gdm.obj load_gt2.obj &
    load_it.obj load_imf.obj load_m15.obj load_med.obj load_mod.obj load_mtm.obj load_okt.obj &
    load_s3m.obj load_stm.obj load_stx.obj load_ult.obj load_umx.obj load_uni.obj load_xm.obj &
    mmalloc.obj mmerror.obj mmio.obj &
    mmcmp.obj pp20.obj s404.obj xpk.obj strcasecmp.obj &
    mdriver.obj mdreg.obj mloader.obj mlreg.obj mlutil.obj mplayer.obj munitrk.obj mwav.obj &
    npertab.obj sloader.obj virtch.obj virtch2.obj virtch_common.obj

all: $(BLD_TARGET)

clean:
	FOR %F IN ( $(DLLNAME) $(EXPNAME) $(LIBNAME) $(LIBSTATIC) $(OBJ) ) DO IF EXIST %F ERASE %F

$(DLLNAME): $(OBJ)
	wlink NAM $@ SYSTEM os2v2_dll INITINSTANCE TERMINSTANCE LIBR {$(LIBS)} FIL {$(OBJ)} OPTION IMPF=$(EXPNAME)
	wlib -q -b -iro -inn $(LIBNAME) +$(DLLNAME)
#	wlib -l $(LIBNAME)
#EXP=$(EXPNAME)
#OPTION IMPL=$(LIBNAME)

$(LIBSTATIC): $(OBJ)
	wlib -q -b -n $@ $(OBJ)

HEADER_DEPS=..\include\mikmod.h ..\include\mikmod_internals.h ../include/mikmod_ctype.h
drv_dart.obj: ..\drivers\drv_dart.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\drivers\drv_dart.c
drv_nos.obj: ..\drivers\drv_nos.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\drivers\drv_nos.c
drv_os2.obj: ..\drivers\drv_os2.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\drivers\drv_os2.c
drv_raw.obj: ..\drivers\drv_raw.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\drivers\drv_raw.c
drv_stdout.obj: ..\drivers\drv_stdout.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\drivers\drv_stdout.c
drv_aiff.obj: ..\drivers\drv_aiff.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\drivers\drv_aiff.c
drv_wav.obj: ..\drivers\drv_wav.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\drivers\drv_wav.c
load_669.obj: ..\loaders\load_669.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_669.c
load_amf.obj: ..\loaders\load_amf.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_amf.c
load_asy.obj: ..\loaders\load_asy.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_asy.c
load_dsm.obj: ..\loaders\load_dsm.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_dsm.c
load_far.obj: ..\loaders\load_far.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_far.c
load_gdm.obj: ..\loaders\load_gdm.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_gdm.c
load_gt2.obj: ..\loaders\load_gt2.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_gt2.c
load_it.obj: ..\loaders\load_it.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_it.c
load_imf.obj: ..\loaders\load_imf.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_imf.c
load_m15.obj: ..\loaders\load_m15.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_m15.c
load_med.obj: ..\loaders\load_med.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_med.c
load_mod.obj: ..\loaders\load_mod.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_mod.c
load_mtm.obj: ..\loaders\load_mtm.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_mtm.c
load_okt.obj: ..\loaders\load_okt.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_okt.c
load_s3m.obj: ..\loaders\load_s3m.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_s3m.c
load_stm.obj: ..\loaders\load_stm.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_stm.c
load_stx.obj: ..\loaders\load_stx.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_stx.c
load_ult.obj: ..\loaders\load_ult.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_ult.c
load_umx.obj: ..\loaders\load_umx.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_umx.c
load_uni.obj: ..\loaders\load_uni.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_uni.c
load_xm.obj: ..\loaders\load_xm.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\loaders\load_xm.c
mmalloc.obj: ..\mmio\mmalloc.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\mmio\mmalloc.c
mmerror.obj: ..\mmio\mmerror.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\mmio\mmerror.c
mmio.obj: ..\mmio\mmio.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\mmio\mmio.c
mmcmp.obj: ..\depackers\mmcmp.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\depackers\mmcmp.c
pp20.obj: ..\depackers\pp20.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\depackers\pp20.c
s404.obj: ..\depackers\s404.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\depackers\s404.c
xpk.obj: ..\depackers\xpk.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\depackers\xpk.c
mdriver.obj: ..\playercode\mdriver.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\mdriver.c
mdreg.obj: ..\playercode\mdreg.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\mdreg.c
mloader.obj: ..\playercode\mloader.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\mloader.c
mlreg.obj: ..\playercode\mlreg.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\mlreg.c
mlutil.obj: ..\playercode\mlutil.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\mlutil.c
mplayer.obj: ..\playercode\mplayer.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\mplayer.c
munitrk.obj: ..\playercode\munitrk.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\munitrk.c
mwav.obj: ..\playercode\mwav.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\mwav.c
npertab.obj: ..\playercode\npertab.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\npertab.c
sloader.obj: ..\playercode\sloader.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\sloader.c
virtch.obj: ..\playercode\virtch.c ..\playercode\virtch_common.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\virtch.c
virtch2.obj: ..\playercode\virtch2.c ..\playercode\virtch_common.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\virtch2.c
virtch_common.obj: ..\playercode\virtch_common.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\playercode\virtch_common.c
strcasecmp.obj: ..\posix\strcasecmp.c $(HEADER_DEPS)
	$(COMPILE) -fo=$^@ ..\posix\strcasecmp.c
