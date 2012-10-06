	.global _start
	.extern start_kernel

	.text
	
_start:
	movew	#0x2700, %sr            /* Exceptions off! */

#if 0
	/* Init chip registers.  uCsimm specific */
	moveb	#0x00,   0xfffffb0b	/* Watchdog off */
	moveb	#0x10,   0xfffff000	/* SCR */

	movew   #0x2400, 0xfffff200	/* PLLCR */
	movew   #0x0123, 0xfffff202	/* PLLFSR */

	moveb	#0x00,   0xfffff40b	/* enable chip select */
	moveb	#0x00,   0xfffff423	/* enable /DWE */
	moveb	#0x08,   0xfffffd0d	/* disable hardmap */
	moveb	#0x07,   0xfffffd0e	/* level 7 interrupt clear */

	movew	#0x8600, 0xfffff100	/* FLASH at 0x10c00000 */
	movew	#0x018b, 0xfffff110	/* 2Meg, enable, 0ws */

	movew	#0x8f00, 0xfffffc00	/* DRAM configuration */
	movew	#0x9667, 0xfffffc02	/* DRAM control */
	movew	#0x0000, 0xfffff106	/* DRAM at 0x00000000 */
	movew	#0x068f, 0xfffff116	/* 8Meg, enable, 0ws */

	moveb	#0x40,   0xfffff300	/* IVR */
	movel	#0x007FFFFF, %d0	/* IMR */
	movel	%d0,     0xfffff304

	moveb	0xfffff42b, %d0
	andb	#0xe0,	 %d0
	moveb	%d0,	 0xfffff42b

	moveb	#0x08,   0xfffff907	/* Ignore CTS */
	movew	#0x0226, 0xfffff902	/* BAUD to 19200 */
	movew	#0xe100, 0xfffff900	/* enable */

	movew	#16384, %d0  /* PLL settle wait loop */
L0:
	subw	#1, %d0
	bne	L0
#endif

pclp2:
	moveq	#70, %d7		/* 'F' */
	moveb	%d7,0xfffff906          /* No absolute addresses */
	beq	pclp2

	moveq	#70, %d7		/* 'F' */
	moveb	%d7,0xfffff906          /* No absolute addresses */
pclp1:
	movew	0xfffff906, %d7
	andw	#0x2000, %d7
	beq	pclp1

	moveq	#71,%d7
	moveb	%d7,0xfffff906
	moveb	#0xcf,0xfffff43B

	moveq	#82, %d7		/* 'R' */
	moveb	%d7,0xfffff906          /* No absolute addresses */
pclp3:
	movew	0xfffff906, %d7
	andw	#0x2000, %d7
	beq	pclp3

	moveq	#67, %d7                /* 'C' */
	jsr	putc

#if 0
	pea	0
	pea	0x80000
	pea	%sp@(4)
	pea	0

	moveq	#70, %d7		/* 'F' */
	jsr	putc

lp:
	jsr	start_kernel
#endif
	rts

_exit:
	jmp	_exit

putc:
	moveb	%d7,0xfffff906
pclp:
	movew	0xfffff906, %d7
	andw	#0x2000, %d7
	beq	pclp
	rts
