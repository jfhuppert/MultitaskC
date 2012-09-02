# Makefile for llC standard PC C RunTime Environement.
#

CC=gcc
CCFLAGS=-Wall -march=i486 -m32 -O3 -fomit-frame-pointer -fno-builtin -ffreestanding
AS=as -32

OBJS= head.o reloc.o main.o test.o init.o lib.o screen_buffer.o linuxbios.o memsize.o pci.o
# OBJS= head.o reloc.o main.o test.o init.o lib.o patn.o screen_buffer.o config.o linuxbios.o memsize.o pci.o controller.o random.o extra.o

all: standalone.iso

standalone.iso : standalone.bin
	@./makeiso.sh

reloc.o: reloc.c
	$(CC) -c -m32 -fPIC -Wall -O -fno-strict-aliasing reloc.c

test.o: test.c test.h defs.h config.h
	$(CC) -c $(CCFLAGS) test.c

main.o: main.c test.h defs.h
	$(CC) -c $(CCFLAGS) -fPIC main.c

init.o: init.c test.h defs.h io.h config.h controller.h pci.h
	$(CC) -c $(CCFLAGS) -fPIC init.c

linuxbios.o: linuxbios.c test.h linuxbios_tables.h defs.h config.h
	$(CC) -c $(CCFLAGS) -fPIC linuxbios.c

memsize.o: memsize.c test.h defs.h config.h
	$(CC) -c $(CCFLAGS) -fPIC memsize.c

lib.o: lib.c test.h defs.h io.h screen_buffer.h serial.h config.h
	$(CC) -c $(CCFLAGS) -fPIC lib.c

screen_buffer.o: screen_buffer.c screen_buffer.h test.h config.h
	$(CC) -c $(CCFLAGS) -fPIC screen_buffer.c

pci.o: pci.c pci.h io.h
	$(CC) -c $(CCFLAGS) -fPIC pci.c

head.s: head.S
	$(CC) -E -m32 -traditional $< -o $@

head.o: head.s
	$(AS) -o $@ $<

makedefs: makedefs.c defs.h
	 $(CC) $(CCFLAGS) makedefs.c -o $@

# Link it statically once so I know I don't have undefined
# symbols and then link it dynamically so I have full
# relocation information
standalone_shared: $(OBJS) standalone_shared.lds Makefile
	$(LD) --warn-constructors --warn-common -static -T standalone_shared.lds -o $@ $(OBJS) && \
	$(LD) -shared -Bsymbolic -T standalone_shared.lds -o $@ $(OBJS)

standalone_shared.bin: standalone_shared
	objcopy -O binary $< standalone_shared.bin

bootsect.s: bootsect.S defs.h
	$(CC) -E -traditional $< -o $@

bootsect.o: bootsect.s
	$(AS) -o $@ $<

bootsect: bootsect.o
	$(LD) -Ttext 0x00 -s --oformat binary -e _main --just-symbols=standalone_shared.o -o $@ $<

setup.s: setup.S config.h defs.h
	$(CC) -E -traditional $< -o $@

setup.o: setup.s
	$(AS) -o $@ $<

standalone.bin: standalone_shared.bin bootsect.o setup.o standalone.bin.lds
	$(LD) -T standalone.bin.lds bootsect.o setup.o -b binary standalone_shared.bin -o standalone.bin

clean:
	rm -f *.o *.s standalone.bin standalone_shared standalone_shared.bin standalone.iso
