# Makefile for DOS (FLAT model) using Open Watcom compiler.
# Edit config.h to disable/enable certain drivers, etc, if
# necessary.
#
# wmake -f Makefile.wat
# - builds static library mikmod.lib

INCLUDES=-I. -I"../drivers/dos" -I"../include"
CPPFLAGS=-DMIKMOD_BUILD -DHAVE_CONFIG_H -DMIKMOD_STATIC=1

CFLAGS = -bt=dos -fp5 -fpi87 -mf -oeatxh -w4 -ei -zp8 -zq
# newer OpenWatcom versions enable W303 by default.
CFLAGS+= -wcd=303
# -5s  :  Pentium stack calling conventions.
# -5r  :  Pentium register calling conventions.
CFLAGS+= -5s

.SUFFIXES:
.SUFFIXES: .obj .c

LIBSTATIC=mikmod.lib

COMPILE=wcc386 $(CFLAGS) $(CPPFLAGS) $(INCLUDES)

OBJ=dosdma.obj dosirq.obj dosutil.obj &
    dosgus.obj drv_ultra.obj dossb.obj drv_sb.obj doswss.obj drv_wss.obj &
    drv_raw.obj drv_aiff.obj drv_wav.obj &
    drv_nos.obj drv_stdout.obj &
    load_669.obj load_amf.obj load_asy.obj load_dsm.obj load_far.obj load_gdm.obj load_gt2.obj &
    load_it.obj load_imf.obj load_m15.obj load_med.obj load_mod.obj load_mtm.obj load_okt.obj &
    load_s3m.obj load_stm.obj load_stx.obj load_ult.obj load_umx.obj load_uni.obj load_xm.obj &
    mmalloc.obj mmerror.obj mmio.obj &
    mmcmp.obj pp20.obj s404.obj xpk.obj strcasecmp.obj &
    mdriver.obj mdreg.obj mloader.obj mlreg.obj mlutil.obj mplayer.obj munitrk.obj mwav.obj &
    npertab.obj sloader.obj virtch.obj virtch2.obj virtch_common.obj

all: $(LIBSTATIC)

$(LIBSTATIC): $(OBJ)
	wlib -q -b -n -c -pa -s -t -zld -ii -io $@ $(OBJ)

.c: ../drivers;../drivers/dos;../loaders;../depackers;../mmio;../playercode;../posix;
.c.obj:
	$(COMPILE) -fo=$^@ $<

distclean: clean .symbolic
	rm -f $(LIBSTATIC)
clean: .symbolic
	rm -f *.obj
