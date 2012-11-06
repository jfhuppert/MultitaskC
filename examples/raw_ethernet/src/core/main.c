/*------------------------------------------------------------------------------*/
/* main.c for raw_ethernet with MultitaskC. */
/*------------------------------------------------------------------------------*/

/* #define MDEBUG */

#include "etherboot.h"
#include "dev.h"
#include "nic.h"
#include "disk.h"
#include "http.h"
#include "timer.h"
#include "cpu.h"
#include <stdarg.h>

#ifdef CONSOLE_BTEXT
#include "btext.h"
#endif

#ifdef CONFIG_FILO
#include <lib.h>
#endif

jmp_buf	restart_etherboot;
int	url_port;		

char as_main_program = 1;

static const char broadcast[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

/*------------------------------------------------------------------------------*/
/* in_call(): the entry point to Etherboot.  Generally called from
 * arch_in_call(), which in turn will have been invoked from
 * platform-specific assembly code.
 */
int in_call ( in_call_data_t *data, uint32_t opcode, va_list params ) {
	int old_as_main_program = as_main_program;
	int ret = 0;

	/* Set flat to indicate that we are not running as the main
	 * program (i.e. we are something like a PXE stack).
	 */
	as_main_program = 0;

	/* NOTE: params will cease to be valid if we relocate, since
	 * it represents a virtual address
	 */
	switch ( EB_OPCODE(opcode) ) {
		
	case EB_OPCODE_CHECK:
		/* Installation check
		 */
		ret = EB_CHECK_RESULT;
		break;
	case EB_OPCODE_MAIN:
		/* Start up Etherboot as a standalone program. */
		as_main_program = 1;
		ret = main ( data, params );
		break;
#ifdef PXE_EXPORT
	case EB_OPCODE_PXE:
		/* !PXE API call */
		ret = pxe_in_call ( data, params );
		break;
#endif
	default:
		printf ( "Unsupported API \"%c%c\"\n",
			 EB_OPCODE(opcode) >> 8, EB_OPCODE(opcode) & 0xff );
		ret = -1;
		break;
	}

	as_main_program = old_as_main_program;
	return ret;
}
/*------------------------------------------------------------------------------*/
void console_init(void)
{
#ifdef	CONSOLE_SERIAL
	(void)serial_init();
#endif
#ifdef 	CONSOLE_DIRECT_VGA
       	video_init();
#endif
#ifdef	CONSOLE_BTEXT
	map_boot_text();
#endif
}
/*------------------------------------------------------------------------------*/
int main(in_call_data_t *data, va_list params)
{
char *p;
unsigned long eth_data;
unsigned long eth_npackets;

	for(p=_bss;p<_ebss;p++) *p=0;	/* Zero BSS */

	console_init();
	arch_main(data,params);

	if(rom.rom_segment) {
		printf("ROM segment %#hx length %#hx reloc %#x\n",rom.rom_segment,rom.rom_length,_text);
	}

	cpu_setup();
	setup_timers();
	gateA20_set();
#if 0
	print_config();
#endif
	get_memsizes();
	cleanup();

	printf("MultitaskC raw_ethernet started\n");

	if(eth_probe2()) {
		printf("No adapter found\n");
		exit(0);
	}

	eth_data=0;
	eth_npackets=0;
	for(;;) {
		eth_transmit(broadcast,0x0a56,sizeof(eth_data),&eth_data);
		eth_data+=1;
		if(eth_poll(1)) {
			printf("got a packet %d\r",eth_npackets);
			eth_npackets+=1;
		}
		sleep(1);
	}
}
/*------------------------------------------------------------------------------*/
void exit(int code)
{
	printf("exit(%d)\n",code);
	for(;;);
}
/*------------------------------------------------------------------------------*/
void cleanup(void)
{
}
/*------------------------------------------------------------------------------*/
int loadkernel(const char *fname)
{
	printf("loadkernel(%s)\n",fname);
	return(0);
}
/*------------------------------------------------------------------------------*/
