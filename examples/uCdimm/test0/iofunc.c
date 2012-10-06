/*----------------------------------------------------------------------------*/
/* IO functions.                                                              */
/*----------------------------------------------------------------------------*/

void putchr(char c);
char getchr(void);
char nb_getchr(void);
void putstr(char *s);
void print(char *m,unsigned long v);
void puthex(unsigned long val);
void banner(void);

/*----------------------------------------------------------------------------*/
void banner(void)
{
	putstr("Compiled on " __DATE__ " at " __TIME__ "\r\n");
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
void putchr(char c)
{
	*(volatile char *)0xfffff907=c;
	while(((*(volatile unsigned short *)0xfffff906)&0x2000)==0);
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
void puthex(unsigned long val)
{
	unsigned char buf[10];
	int i;

	for (i = 7;  i >= 0;  i--) {
		buf[i] = "0123456789ABCDEF"[val & 0x0F];
		val >>= 4;
	}
	buf[8] = '\0';
	putstr(buf);
}
/*----------------------------------------------------------------------------*/
