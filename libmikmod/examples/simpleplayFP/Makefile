CC=gcc
LD=$(CC)

CFLAGS=-Wall -g `libmikmod-config --cflags`
LDFLAGS=`libmikmod-config --libs`

PROG=splayFP

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))

.PHONY : clean install

all: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(PROG)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(PROG)

install:
	@echo "Install not done yet. Sorry"

