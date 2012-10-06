#-----------------------------------------------------

CFLAGS = -O -fsigned-char \
		 -msoft-float \
		 -pipe \
		 -fno-builtin \
		 -Wno-uninitialized \
		 -fomit-frame-pointer -fno-strength-reduce

#-----------------------------------------------------

smapping.bin : crt0.o smapping.o
	m68k-coff-ld -T smapping.ls -o smapping crt0.o smapping.o
	m68k-coff-objcopy -O binary -R .comment -S -g smapping smapping.bin

smapping.o : smapping.c
	m68k-coff-gcc -m68000 -o $@ $(CFLAGS) -c $<

crt0.o : crt0.S
	m68k-coff-gcc -m68000 -o $@ $(CFLAGS) -c $<

#-----------------------------------------------------

clean :
	rm -f smapping smapping.bin *.o
