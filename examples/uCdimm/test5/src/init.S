/*------------------------------------------------------------------------*/
/* crt0_rom.S : runtime init for a standard C program.                    */
/*------------------------------------------------------------------------*/

#define CONFIG_MEMORY_RESERVE 0

	.global _stext
	.global _start
	.global putc
	.global int_handler
	.global int_handler5

        .global _rambase
        .global _ramvec
        .global _ramstart
        .global _ramend

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

        .data

/*
 *      Set up the usable of RAM stuff. Size of RAM is determined then
 *      an initial stack set up at the end.
 */
.align 4
_ramvec:
.long   0
_rambase:
.long   0
_ramstart:
.long   0
_ramend:
.long   0

	.text

buserr:
system_call:
ignore:
trap:
	rte

int_handler:
#if 0
	jsr	int_handler_c_dummy
#endif
	rte

int_handler5:
#if 0
	jsr	int_handler_c_5
#endif
	rte

	.text
_start:
_stext:	movew	#0x2700,%sr

	moveb	#0x0,	0xfffff000

	moveal  #__ramend-CONFIG_MEMORY_RESERVE*0x100000 - 0x10, %sp
	movew	#32767, %d0  /* PLL settle wait loop */
1:	subq	#1, %d0
	bne	1b

	/* Copy data segment from ROM to RAM */
	moveal	#__data_rom_start, %a0
	moveal	#_sdata, %a1
	moveal	#_edata, %a2

	/* Copy %a0 to %a1 until %a1 == %a2 */
1:	movel	%a0@+, %a1@+
	cmpal	%a1, %a2
	bhi	1b

	moveal	#_sbss, %a0
	moveal	#_ebss, %a1
	/* Copy 0 to %a0 until %a0 == %a1 */
	
1:
	clrl	%a0@+
	cmpal	%a0, %a1
	bhi	1b

        movel   #_sdata, %d0    
        movel   %d0,    _rambase        
        movel   #_ebss,  %d0
        movel   %d0,    _ramstart
	movel	#__ramend-CONFIG_MEMORY_RESERVE*0x100000, %d0
	movel	%d0,	_ramend
	movel	#__ramvec,	%d0
	movel	%d0,	_ramvec

	/* Enable RXD TXD port bits to enable UART2 */
	moveb	#0xcf,   0xfffff43b
	
	pea	0
	pea	env
	pea	%sp@(4)
	pea	0

	moveb	#'C',%d7
	jsr	putc

1:	jsr	_main
        bra 1b

putc:
	moveb	%d7,0xfffff907
1:
	movew	0xfffff906, %d7
	andw	#0x2000, %d7
	beq	1b
	rts


	.data
env:
	.long	0

	.text
