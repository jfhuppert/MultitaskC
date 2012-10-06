# Test Makefile

CCOMP=m68k-coff-

GCC=$(CCOMP)gcc
LD=$(CCOMP)ld
OBJCOPY=$(CCOMP)objcopy

IDIR=../include
ODIR=../obj

CFLAGS=-I$(IDIR) -O2 -fomit-frame-pointer -fno-strength-reduce -pipe -m68000
# CFLAGS=-I$(IDIR) -g -fomit-frame-pointer -fno-strength-reduce -pipe -m68000

test.bin : $(ODIR)/test.text $(ODIR)/test.data
	cat $(ODIR)/test.text $(ODIR)/test.data > test.bin
	ln -fs test.bin t

$(ODIR)/test.text : $(ODIR)/test
	$(OBJCOPY) -O binary --remove-section=.ramvec --remove-section=.bss --remove-section=.data \
     --remove-section=.eram --set-section-flags=.romvec=CONTENTS,ALLOC,LOAD,READONLY,CODE $(ODIR)/test $(ODIR)/test.text

$(ODIR)/test.data : $(ODIR)/test
	$(OBJCOPY) -O binary --remove-section=.romvec --remove-section=.text --remove-section=.ramvec \
     --remove-section=.bss --remove-section=.eram $(ODIR)/test $(ODIR)/test.data

$(ODIR)/test : $(ODIR)/init.o $(ODIR)/main.o $(ODIR)/iofuncs.o $(ODIR)/cs8900.o
	$(LD) -T rom.ld $(ODIR)/init.o $(ODIR)/main.o $(ODIR)/iofuncs.o $(ODIR)/cs8900.o -o $(ODIR)/test

$(ODIR)/init.o : init.S
	$(GCC) -pipe -m68000 -Wa,--bitwise-or -c init.S -o $(ODIR)/init.o

$(ODIR)/main.o : main.c
	$(GCC) $(CFLAGS) -c -o $(ODIR)/main.o main.c

$(ODIR)/iofuncs.o : iofuncs.c
	$(GCC) $(CFLAGS) -c -o $(ODIR)/iofuncs.o iofuncs.c

$(ODIR)/cs8900.o : cs8900.c
	$(GCC) $(CFLAGS) -c -o $(ODIR)/cs8900.o cs8900.c

main.c : main.llC
	llC -I$(IDIR) -w main.llC main.c

clean :
	rm -f t $(ODIR)/*.o $(ODIR)/test test.bin $(ODIR)/test.data $(ODIR)/test.text main.c
