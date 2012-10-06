# Test Makefile

CCOMP=m68k-coff-

GCC=$(CCOMP)gcc
LD=$(CCOMP)ld
OBJCOPY=$(CCOMP)objcopy

CFLAGS=-Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strength-reduce -pipe -m68000

test.bin : test.text test.data
	cat test.text test.data > test.bin

test.text : test
	$(OBJCOPY) -O binary --remove-section=.ramvec --remove-section=.bss --remove-section=.data \
     --remove-section=.eram --set-section-flags=.romvec=CONTENTS,ALLOC,LOAD,READONLY,CODE test test.text

test.data : test
	$(OBJCOPY) -O binary --remove-section=.romvec --remove-section=.text --remove-section=.ramvec \
     --remove-section=.bss --remove-section=.eram test test.data

test : init.o main.o iofunc.o
	$(LD) -g -T rom.ld init.o main.o iofunc.o -o test

init.o : init.S
	$(GCC) -g -pipe -m68000 -Wa,--bitwise-or -c init.S -o init.o

main.o : main.c
	$(GCC) -g $(CFLAGS) -c -o main.o main.c

iofunc.o : iofunc.c
	$(GCC) -g $(CFLAGS) -c -o iofunc.o iofunc.c

main.c : main.llC
	llC -w main.llC main.c

clean :
	rm -f *.o test test.bin test.data test.text main.c
