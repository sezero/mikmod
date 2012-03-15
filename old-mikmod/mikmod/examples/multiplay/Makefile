OBJS=winmain.o ../../../mmio/src/log.o Script1.o

%.o: %.cpp
	gcc -I../../include -I../../../mmio/include -o $@ -c $<

%.o: %.c
	gcc -I../../include -I../../../mmio/include -o $@ -c $<

%.o: %.RES
	windres -i $< -o $@ -O coff

default: $(OBJS)
	gcc -o multiplay.exe $(OBJS) ../../libmikmod.a -lwinmm -lcomdlg32 -lcomctl32

clean:
	rm $(OBJS)
