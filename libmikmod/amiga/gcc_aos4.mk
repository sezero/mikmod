# makefile fragment for ppc-amigaos4 / gcc

#CRT_FLAGS = -mcrt=clib2
#CRT_FLAGS = -noixemul
CRT_FLAGS = -mcrt=newlib

LDFLAGS+= $(CRT_FLAGS)
CFLAGS += $(CRT_FLAGS)
CPPFLAGS+= -DWORDS_BIGENDIAN=1
#CPPFLAGS+= -D__USE_INLINE__ -D__USE_OLD_TIMEVAL__

