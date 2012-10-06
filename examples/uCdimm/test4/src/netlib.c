/*----------------------------------------------------------------------------*/
/* Netlib functions.                                                              */
/*----------------------------------------------------------------------------*/

#include "lwip/inet.h"
#include <iofuncs.h>

typedef int size_t;

/*----------------------------------------------------------------------------*/
u16_t htons(u16_t n) { return(HTONS(n)); }
u16_t ntohs(u16_t n) { return(NTOHS(n)); }
u32_t htonl(u32_t n) { return(HTONL(n)); }
u32_t ntohl(u32_t n) { return(NTOHL(n)); }
/*----------------------------------------------------------------------------*/
static int isdnum(char c)
{
	return('0'<=c && c<='9');
}
/*----------------------------------------------------------------------------*/
static char *next_num(char *str,unsigned long *a)
{
unsigned long
	val;

	*a=val=0;
	while(*str!='.' && *str!='\0' && isdnum(*str)) {
		val*=10;
		val+=((*str)-'0');
		str++;
	}
	*a=val;
	return(str);
}
/*----------------------------------------------------------------------------*/
static void ip_convert(char *str,unsigned long *a,unsigned long *b,
	unsigned long *c,unsigned long *d)
{
	str=next_num(str,a);
	// putstr("a="); puthex8(*a); putstr(",");
	str=next_num(str+1,b);
	// putstr("b="); puthex8(*b); putstr(",");
	str=next_num(str+1,c);
	// putstr("c="); puthex8(*c); putstr(",");
	str=next_num(str+1,d);
	// putstr("d="); puthex8(*d); putstr("\r\n");
}
/*----------------------------------------------------------------------------*/
void networkp_init(struct ip_addr *ia,struct ip_addr *nm,struct ip_addr *gw)
{
unsigned long
	_a,_b,_c,_d;

char
	*var;

	var=getevar("IA");
	if(var==0) var="192.168.1.100";
	ip_convert(var,&_a,&_b,&_c,&_d);
	IP4_ADDR(ia,_a,_b,_c,_d);
	putstr("Using IP address : "); putstr(var); putstr("\r\n");

	var=getevar("GW");
	if(var==0) var="192.168.1.254";
	ip_convert(var,&_a,&_b,&_c,&_d);
	IP4_ADDR(gw,_a,_b,_c,_d);
	putstr("Using gateway    : "); putstr(var); putstr("\r\n");

	var=getevar("NM");
	if(var==0) var="255.255.255.0";
	ip_convert(var,&_a,&_b,&_c,&_d);
	IP4_ADDR(nm,_a,_b,_c,_d);
	putstr("Using netmask    : "); putstr(var); putstr("\r\n");
}
/*----------------------------------------------------------------------------*/
