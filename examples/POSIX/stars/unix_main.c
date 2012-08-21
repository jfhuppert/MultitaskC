#include <stdio.h>
#include "putget.h"

#define BIGBUFFSIZE (1024*1024)

static char bigbuff[BIGBUFFSIZE];

int main(int argc,char *argv[])
{
#if 1
   setbuf(stdout,NULL);
#else
   setbuffer(stdout,bigbuff,BIGBUFFSIZE);
#endif
   putstr("\x1b" "[?25l" "\x1b" "[1;1H" "\x1b" "[2J");
   p0();
}
