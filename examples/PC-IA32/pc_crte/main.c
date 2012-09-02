/* main.c - MemTest-86  Version 3.2
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 */

#include "pci.h"
#include "test.h"

static char
	buff[8192];

void _main(void)
{
volatile register int
	i;

unsigned long
	res;

	cprint(0,0,"_main Compiled on " __DATE__ " at " __TIME__);

	i=pci_init();
	cprint(1,0,"pci_init() = ");
	itoa(buff,i);
	cprint(1,13,buff);
	i=pci_conf_read(0,0,0,8,2,&res);
	cprint(2,0,"pci_conf_read(0,0,0,8,2,&res) = ");
	itoa(buff,i);
	cprint(2,32,buff);
	cprint(3,0,"res = 0x");
	hprint(3,8,res);

	for(i=0;;) i++;
}
