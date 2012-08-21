# Makefile

p0 : p0.o putget.o unix_main.o
	gcc -o p0 p0.o putget.o unix_main.o

p0.o : p0.c
	gcc -c -O0 p0.c 

p0.c : p0.mtc
	mtcc -e -m p0.mtc p0.c

putget.o : putget.c
	gcc -c -O6 putget.c 

unix_main.o : unix_main.c
	gcc -c -O6 unix_main.c 

clean :
	rm -fr *.o p0 p0.c
