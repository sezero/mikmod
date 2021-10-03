# makefile fragment for m68k-amigaos / vbcc

CFLAGS += -cpu=68020 -fpu=68881
CPPFLAGS += -D__AMIGA__
CPPFLAGS += -DWORDS_BIGENDIAN=1
