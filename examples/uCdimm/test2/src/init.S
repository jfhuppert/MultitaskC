/*------------------------------------------------------------------------*/
/* crt0_rom.S : runtime init for a standard C program.                    */
/*------------------------------------------------------------------------*/

	.global _stext
	.global _start
	.global __bss_start
	.global	_ramend
	.global putc
	.global int_handler
	.global int_handler5

	.section .romvec
e_vectors:
	.long _ramend-4, _start + 0x00000000, buserr, trap, trap, trap, trap, trap
	.long trap, trap, trap, trap, trap, trap, trap, trap
	.long trap, trap, trap, trap, trap, trap, trap, trap
	.long trap, trap, trap, trap
	.long trap, trap, trap, trap
	.long system_call, trap, trap, trap, trap, trap, trap, trap
	.long trap, trap, trap, trap, trap, trap, trap, trap
	.long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.text

buserr:
system_call:
ignore:
trap:
	rte

int_handler:
	jsr	int_handler_c_dummy
	rte

int_handler5:
	jsr	int_handler_c_5
	rte

	.text

_start:
_stext:	
	moval	#_ramend-0x4,%sp
	movew	#0x2700,%sr			/* Exceptions off! */
	moveb	#0xe0,0xfffff000	/* SCR BETEN=FALSE SO=FALSE DMAP=FALSE WDTH8=FALSE */
    movew	#0x0000,0xfffffb0a	/* WATCHDOG EN=FALSE */
	moveb	#0x0008,0xfffffd0c	/* ICEMCR HMDIS=TRUE */
	moveb	#0x07,0xfffffd0e	/* ICEMSR Disable Level 7 Interrupts */
	moveb	#0x0f,0xfffff423	/* PESEL SPITXD, SPIRXD, SPICLK2, ~DWE/~UCKL No Other Dedicated Functionality */
	moveb	#0xff,0xfffff443	/* PKSEL No Dedicated Functionality */
	moveb	#0x00,0xfffff44b	/* PMSEL Dedicated Functionality Only */
	moveb	#0x00,0xfffff40b	/* PBSEL Dedicated Functionality Only*/
	moveb	#0x40,0xfffff300	/* IVR */
	movel	#0x007fffff,0xfffff304	/* IMR */
	moveb	#0xcf,0xfffff43b

	/* Here's the stack */
	moveal	#_ramend-0x10,%sp

	moveal	#__data_start, %a0		/* Address %a0=SDRAM */
	moveal	#__data_rom_start, %a1	/* Address %a1=FLASH */
	moveal	#__data_end, %a2		/* Address %a2=End Of data segment in Ram */
LD1:
	movel	%a1@+, %d0		/* Copy %a0 -> %a1 until %a1=%a2 */
	movel	%d0, %a0@+
	cmpal	%a0, %a2
	bhi	LD1

	jmp     ram_jump		/* Jump To The Copy In SDRAM */
ram_jump:

	moveb	#'L',%d7
	jsr	putc

	moveal	#__bss_start,%a0
	moveal	#end,%a1

L1:
	movel	#0,%a0@+		/* Clear Out BSS */
	cmpal	%a0,%a1
	bhi	L1

	moveb	#'L',%d7
	jsr	putc

	pea	0
	pea	env
	pea	%sp@(4)
	pea	0

	moveb	#'C',%d7
	jsr	putc

lp:
	jsr	_main
	jmp lp

_exit:
	jmp _exit 

putc:
	moveb	%d7,0xfffff907
pclp:
	movew	0xfffff906,%d7
	andw	#0x2000,%d7
	beq	pclp
	rts

	.data
env:
	.long	0

	.text
