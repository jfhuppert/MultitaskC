/* test.c - MemTest-86  Version 3.2
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 */
#include "test.h"
#include "config.h"

extern int segs, bail;
extern volatile ulong *p;
extern ulong p1, p2;
extern int test_ticks, nticks;
extern struct tseq tseq[];
void poll_errors();

int ecount = 0;

void print_err( ulong *adr, ulong good, ulong bad, ulong xor) 
{
}

void print_ecc_err(unsigned long page, unsigned long offset, 
	int corrected, unsigned short syndrome, int channel)
{
}

#ifdef PARITY_MEM
void parity_err( unsigned long edi, unsigned long esi) 
{
}
#endif

void printpatn(void)
{
}
	
void do_tick(void)
{
}

void sleep(int n)
{
}
