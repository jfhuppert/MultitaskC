/*------------------------------------------------------------------------*/
/*
	iofuncs.h
*/
/*------------------------------------------------------------------------*/

extern void putchr(char c);
extern char getchr(void);
extern char nb_putchr(char c);
extern char nb_getchr(void);
extern void putstr(char *s);
extern void print(char *m,unsigned long v);
extern void puthex32(unsigned long val);
extern void puthex16(unsigned short val);
extern void puthex8(unsigned char val);
extern void puthex(unsigned long val);
extern char *getevar(char *name);

/*------------------------------------------------------------------------*/
