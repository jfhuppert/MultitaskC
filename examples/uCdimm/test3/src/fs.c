/*-----------------------------------------------------------------------------------*/

#define NULL 0

#include "lwip/debug.h"

#include "fs.h"
#include "fsdata.h"
#include "fsdata.c"

/*-----------------------------------------------------------------------------------*/
int strncmp(const char *s0,const char *s1,unsigned int size)
{
  dbg_putstr("strncmp 0\r\n");
  while(size>0 && (*s0-*s1)==0) {
    dbg_putstr("strncmp ");
    dbg_puthex8(*s0);
    dbg_putstr(" ");
    dbg_puthex8(*s1);
    dbg_putstr("\r\n");
    size--,s0++,s1++;
  }
  dbg_putstr("strncmp 1\r\n");
  return(size==0?0:(*(s0-1)-*(s1-1)));
}
/*-----------------------------------------------------------------------------------*/
int strcmp(const char *s0,const char *s1)
{
  dbg_putstr("strcmp 0\r\n");
  while(*s0!='\0' && *s1!='\0' && (*s0-*s1)==0) {
    dbg_putstr("strcmp ");
    dbg_puthex8(*s0);
    dbg_putstr(" ");
    dbg_puthex8(*s1);
    dbg_putstr("\r\n");
    s0++,s1++;
  }
  dbg_putstr("strcmp 1\r\n");
  return(*s0-*s1);
}
/*-----------------------------------------------------------------------------------*/
int
fs_open(char *name, struct fs_file *file)
{
  struct fsdata_file_noconst *f;

  for(f = (struct fsdata_file_noconst *)FS_ROOT; f != NULL; f = (struct fsdata_file_noconst *)f->next) {
    if(!strcmp(name, f->name)) {
      file->data = f->data;
      file->len = f->len;
      dbg_putstr("cmp ");
      dbg_putstr(name);
      dbg_putstr(" and ");
      dbg_putstr(f->name);
      dbg_putstr(" : match 0x");
      dbg_puthex((unsigned long)f->data);
      dbg_putstr("\r\n");
      return 1;
    }
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
