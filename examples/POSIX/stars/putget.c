/*------------------------------------------------------------------------------*/
/*
 * putget.c
*/
/*------------------------------------------------------------------------------*/

#include "putget.h"

extern int printf(const char *format, ...);

/*------------------------------------------------------------------------------*/
void putdec(unsigned long number)
{
   printf("%lu",number);
}
/*------------------------------------------------------------------------------*/
void puthex(unsigned long v)
{
   printf("%lx",v);
}
/*------------------------------------------------------------------------------*/
void putstr(char *s)
{
   while(*s) putchar(*s++);
}
/*------------------------------------------------------------------------------*/
