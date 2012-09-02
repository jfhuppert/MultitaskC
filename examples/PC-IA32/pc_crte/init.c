/* init.c - MemTest-86  Version 3.2
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 * ----------------------------------------------------
 * MemTest86+ V1.11 Specific code (GPL V2.0)
 * By Samuel DEMEULEMEESTER, sdemeule@memtest.org
 * http://www.x86-secret.com - http://www.memtest.org
 */

#include "test.h"
#include "defs.h"
#include "config.h"
#include "controller.h"
#include "pci.h"
#include "io.h"

extern short memsz_mode;
extern short firmware;

struct cpu_ident cpu_id;
ulong st_low, st_high;
ulong end_low, end_high;
ulong cal_low, cal_high;
ulong extclock;

#define FLAT 0

static unsigned long mapped_window = 1;
void paging_off(void)
{
	if (!v->pae)
		return;
	mapped_window = 1;
	__asm__ __volatile__ (
		/* Disable paging */
		"movl %%cr0, %%eax\n\t"
		"andl $0x7FFFFFFF, %%eax\n\t"
		"movl %%eax, %%cr0\n\t"
		/* Disable pae */
		"movl %%cr4, %%eax\n\t"
		"andl $0xFFFFFFDF, %%eax\n\t"
		:
		:
		: "ax"
		);
}

static void paging_on(void *pdp)
{
	if (!v->pae)
		return;
	__asm__ __volatile__(
		/* Load the page table address */
		"movl %0, %%cr3\n\t"
		/* Enable pae */
		"movl %%cr4, %%eax\n\t"
		"orl $0x00000020, %%eax\n\t"
		"movl %%eax, %%cr4\n\t"
		/* Enable paging */
		"movl %%cr0, %%eax\n\t"
		"orl $0x80000000, %%eax\n\t"
		"movl %%eax, %%cr0\n\t"
		:
		: "r" (pdp)
		: "ax"
		);
}

int map_page(unsigned long page)
{
	unsigned long i;
	struct pde {
		unsigned long addr_lo;
		unsigned long addr_hi;
	};
	extern unsigned char pdp[];
	extern struct pde pd2[];
	unsigned long window = page >> 19;
	if (FLAT || (window == mapped_window)) {
		return 0;
	}
	if (window == 0) {
		return 0;
	}
	if (!v->pae || (window >= 32)) {
		/* Fail either we don't have pae support
		 * or we want an address that is out of bounds
		 * even for pae.
		 */
		return -1;
	}
	/* Compute the page table entries... */
	for(i = 0; i < 1024; i++) {
		pd2[i].addr_lo = ((window & 1) << 31) + ((i & 0x3ff) << 21) + 0xE3;
		pd2[i].addr_hi = (window >> 1);
	}
	paging_off();
	if (window > 1) {
		paging_on(pdp);
	}
	mapped_window = window;
	return 0;
}

void *mapping(unsigned long page_addr)
{
	void *result;
	if (FLAT || (page_addr < 0x80000)) {
		/* If the address is less that 1GB directly use the address */
		result = (void *)(page_addr << 12);
	}
	else {
		unsigned long alias;
		alias = page_addr & 0x7FFFF;
		alias += 0x80000;
		result = (void *)(alias << 12);
	}
	return result;
}

void *emapping(unsigned long page_addr)
{
	void *result;
	result = mapping(page_addr -1);
	/* The result needs to be 256 byte alinged... */
	result = ((unsigned char *)result) + 0xf00;
	return result;
}

unsigned long page_of(void *addr)
{
	unsigned long page;
	page = ((unsigned long)addr) >> 12;
	if (!FLAT && (page >= 0x80000)) {
		page &= 0x7FFFF;
		page += mapped_window << 19;
	}
#if 0
	cprint(LINE_SCROLL -2, 0, "page_of(        )->            ");
	hprint(LINE_SCROLL -2, 8, ((unsigned long)addr));
	hprint(LINE_SCROLL -2, 20, page);
#endif	
	return page;
}
