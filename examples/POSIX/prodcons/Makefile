all : spc dpc

spc : pc.mtc spc.h
	mtcc -DSTATIC pc.mtc spc.c
	gcc -o spc spc.c

dpc : pc.mtc dpc.h
	mtcc pc.mtc dpc.c
	gcc -o dpc dpc.c

clean :
	@-rm -f spc dpc spc.c dpc.c
