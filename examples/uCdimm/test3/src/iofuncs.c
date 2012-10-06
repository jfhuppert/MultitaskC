/*----------------------------------------------------------------------------*/
/* IO functions.                                                              */
/*----------------------------------------------------------------------------*/

#include <iofuncs.h>
#include <bootstd.h>

_bsc1(char *, getbenv, char *, a);

typedef int size_t;

/*----------------------------------------------------------------------------*/
void bcopy(const void *src, void *dest, size_t n)
{
register unsigned char
	*p,
	*s;

	p=dest;
	s=(unsigned char *)src;
	while(n--) *p++=*s++;
}
/*----------------------------------------------------------------------------*/
void bzero(void *s, size_t n)
{
register unsigned char
	*p;

	p=s;
	while(n--) *p++=0;
}
/*----------------------------------------------------------------------------*/
char *getevar(char *name)
{
  char *p;
  p=getbenv(name);
  return(p);
}
/*----------------------------------------------------------------------------*/
char nb_getchr(void)
{
unsigned long
	reg;

	reg=*(volatile unsigned short *)0xfffff904;
	if((reg&0x2000)==0) return(0);
	return(reg&0xff);
}
/*----------------------------------------------------------------------------*/
char getchr(void)
{
unsigned long
	reg;

	while(((reg=*(volatile unsigned short *)0xfffff904)&0x2000)==0);
	return(reg&0xff);
}
/*----------------------------------------------------------------------------*/
char nb_putchr(char c)
{
	if(((*(volatile unsigned short *)0xfffff906)&0x2000)!=0) {
		*(volatile char *)0xfffff907=c;
		return(c);
	}
	return(0);
}
/*----------------------------------------------------------------------------*/
void putchr(char c)
{
	while(((*(volatile unsigned short *)0xfffff906)&0x2000)==0);
	*(volatile char *)0xfffff907=c;
}
/*----------------------------------------------------------------------------*/
void putstr(char *str)
{
	while(*str) putchr(*str++);
}
/*----------------------------------------------------------------------------*/
void print(char *m,unsigned long v)
{
	putstr(m);
	putstr(" 0x");
	puthex(v);
	putstr("\r\n");
}
/*----------------------------------------------------------------------------*/
void puthex8(unsigned char val)
{
unsigned char
	buf[10];

int
	i;

	for(i=1;i>=0;i--) { buf[i]="0123456789ABCDEF"[val&0x0F]; val>>=4; }
	buf[2]='\0';
	putstr(buf);
}
/*----------------------------------------------------------------------------*/
void puthex16(unsigned short val)
{
unsigned char
	buf[10];

int
	i;

	for(i=3;i>=0;i--) { buf[i]="0123456789ABCDEF"[val&0x0F]; val>>=4; }
	buf[4]='\0';
	putstr(buf);
}
/*----------------------------------------------------------------------------*/
void puthex32(unsigned long val)
{
	puthex(val);
}
/*----------------------------------------------------------------------------*/
void puthex(unsigned long val)
{
unsigned char
	buf[10];

int
	i;

	for(i=7;i>=0;i--) { buf[i]="0123456789ABCDEF"[val&0x0F]; val>>=4; }
	buf[8]='\0';
	putstr(buf);
}
/*----------------------------------------------------------------------------*/
