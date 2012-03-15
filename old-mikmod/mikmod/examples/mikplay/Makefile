OBJS= message.o mikplay.o

%.o: %.c
	gcc -g -I../../include -I../../../mmio/include -o $@ -c $<

default: $(OBJS)
	gcc -o mikplay $(OBJS) ../../libnewmikmod.so 

clean:
	rm $(OBJS)
