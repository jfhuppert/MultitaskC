# Test Makefile

CCOMP=m68k-elf-

GCC=$(CCOMP)gcc
LD=$(CCOMP)ld
OBJCOPY=$(CCOMP)objcopy
LWIPDIR=../../ipskl/lwipsrc
LWIPLDIR=../../ipskl/src

LWOBJS=$(LWIPLDIR)/arp.o $(LWIPLDIR)/api_lib.o $(LWIPLDIR)/api_msg.o $(LWIPLDIR)/err.o $(LWIPLDIR)/icmp.o $(LWIPLDIR)/inet.o $(LWIPLDIR)/ip.o $(LWIPLDIR)/loopif.o $(LWIPLDIR)/mem.o $(LWIPLDIR)/memp.o $(LWIPLDIR)/netif.o $(LWIPLDIR)/pbuf.o $(LWIPLDIR)/stats.o $(LWIPLDIR)/sys_arch.o $(LWIPLDIR)/tcp_input.o $(LWIPLDIR)/tcp.o $(LWIPLDIR)/tcp_output.o $(LWIPLDIR)/uCcs8900.o $(LWIPLDIR)/udp.o

IDIR=../include
ODIR=../obj

# CFLAGS=-I$(IDIR) -DIPv4 -fpack-struct -O2 -fomit-frame-pointer -fno-strength-reduce -pipe -m68000 -msoft-float
CFLAGS=-I$(IDIR) -DIPv4 -fpack-struct -O2 -fomit-frame-pointer -fno-strength-reduce -pipe -m68000 \
-I$(LWIPDIR)/include -I$(LWIPDIR)/arch/llC/include -I$(LWIPDIR)/include/ipv4 -I$(LWIPLDIR)

test.bin : $(ODIR)/test.text $(ODIR)/test.data
	cat $(ODIR)/test.text $(ODIR)/test.data > test.bin
	@-ln -fs test.bin t
	@-rm -f $(ODIR)/banner.o

ipskl :
	@-make -C $(LWIPLDIR)

$(ODIR)/test.text : $(ODIR)/test
	$(OBJCOPY) -O binary --remove-section=.ramvec --remove-section=.bss --remove-section=.data \
     --remove-section=.eram --set-section-flags=.romvec=CONTENTS,ALLOC,LOAD,READONLY,CODE $(ODIR)/test $(ODIR)/test.text

$(ODIR)/test.data : $(ODIR)/test
	$(OBJCOPY) -O binary --remove-section=.romvec --remove-section=.text --remove-section=.ramvec \
     --remove-section=.bss --remove-section=.eram $(ODIR)/test $(ODIR)/test.data

$(ODIR)/test : $(ODIR)/init.o $(ODIR)/mulsi3.o $(ODIR)/banner.o $(ODIR)/main.o $(ODIR)/iofuncs.o $(ODIR)/netlib.o ipskl
	$(LD) -T rom.ld $(ODIR)/init.o $(ODIR)/mulsi3.o $(ODIR)/banner.o $(ODIR)/main.o $(ODIR)/iofuncs.o \
	$(ODIR)/netlib.o $(LWOBJS) /opt/ecos/gnutools/m68k-elf/lib/gcc/m68k-elf/4.3.2/libgcc.a -o $(ODIR)/test

$(ODIR)/init.o : init.S
	$(GCC) -pipe -m68000 -Wa,--bitwise-or -c init.S -o $(ODIR)/init.o

$(ODIR)/mulsi3.o : mulsi3.S
	$(GCC) -pipe -m68000 -Wa,--bitwise-or -c mulsi3.S -o $(ODIR)/mulsi3.o

$(ODIR)/main.o : main.c
	$(GCC) $(CFLAGS) -c -o $(ODIR)/main.o main.c

$(ODIR)/iofuncs.o : iofuncs.c
	$(GCC) $(CFLAGS) -c -o $(ODIR)/iofuncs.o iofuncs.c

$(ODIR)/netlib.o : netlib.c
	$(GCC) $(CFLAGS) -c -o $(ODIR)/netlib.o netlib.c

$(ODIR)/banner.o :
	$(GCC) $(CFLAGS) -c -o $(ODIR)/banner.o banner.c

main.c : main.mtc
	mtcc -I$(IDIR) -I$(LWIPDIR)/include -I$(LWIPDIR)/arch/llC/include -I$(LWIPDIR)/include/ipv4 -I$(LWIPLDIR) -w main.mtc main.c

clean :
	rm -f t $(ODIR)/*.o $(ODIR)/test test.bin $(ODIR)/test.data $(ODIR)/test.text main.c
	make -C $(LWIPLDIR) clean
