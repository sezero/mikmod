cd dos
wmake %1 -f dos.wc
cd..
wmake %1 -f miklib.wc
wmake %1 -f makefile.wc
