# makefile fragment for m68k-amigaos / vbcc

LDLIBS += -lm040
CFLAGS += -cpu=68060 -fpu=68060
CPPFLAGS += -D__AMIGA__
CPPFLAGS += -DWORDS_BIGENDIAN=1
