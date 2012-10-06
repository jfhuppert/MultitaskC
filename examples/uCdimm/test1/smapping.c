/*----------------------------------------------------------------------------
 * smapping.c
 *--------------------------------------------------------------------------*/

#include "smapping.h"
#include "MC68VZ328.h"

#define DEBUG 1

static void exit(int d);
static void puthex(unsigned long val);
static void print(const char *s, unsigned long v);
static void outchr(const char c);
static void outstr(const char *string);

#ifdef DEBUG
#define printf(m,v) print(m,v)
#else
#define printf(m,v) do {} while (0)
#endif

static char hugo[5]="HUGO";
static char crlf[3]="\r\n";

/*----------------------------------------------------------------------------*/
void start_kernel(void)
{
volatile long i;

	PDSEL|=0x01;
	PDDIR|=0x01;

	for(;;) {
		PDDATA ^= 0x01;
		for(i=0;i<0x10000;) i++;
	}

/*
	outstr(hugo);
	outstr(crlf);
	exit(0);
*/
}
/*----------------------------------------------------------------------------*/
static void outchr(const char c)
{
	*(volatile char *)0xfffff906=c;
}
/*----------------------------------------------------------------------------*/
static void outstr(const char *str)
{
	while(*str++) outchr(*str);
}
/*----------------------------------------------------------------------------*/
static void
print(
	const char *m,
	unsigned long v)
{
	outstr(m);
	outstr(" 0x");
	puthex(v);
}
/*----------------------------------------------------------------------------*/
static void
exit(
	int d)
{
	if (d!=0) {
		outstr("exit(");
		puthex(d);
		outstr(")\r\n");
	}
}
/*----------------------------------------------------------------------------*/
static void
puthex(
	unsigned long val)
{
	unsigned char buf[10];
	int i;

	for (i = 7;  i >= 0;  i--) {
		buf[i] = "0123456789ABCDEF"[val & 0x0F];
		val >>= 4;
	}
	buf[8] = '\0';
	outstr(buf);
}
/*----------------------------------------------------------------------------*/
