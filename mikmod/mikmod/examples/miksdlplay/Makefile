OBJS=font.o gui.o miksdlplay.o
MIKLIB=../../..

%.o: %.c
	gcc -g `sdl-config --cflags` -I$(MIKLIB)/mikmod/include \
	-I$(MIKLIB)/mmio/include -o $@ -c $<

default: $(OBJS)
	gcc -o miksdlplay $(OBJS) $(MIKLIB)/mikmod/libnewmikmod.so \
	`sdl-config --libs`

clean:
	rm -f $(OBJS)
