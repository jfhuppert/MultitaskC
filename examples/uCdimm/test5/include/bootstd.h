
#define NR_BSC 23            /* last used bootloader system call */

#define __BN_reset        0  /* reset and start the bootloader */
#define __BN_test         1  /* tests the system call interface */
#define __BN_exec         2  /* executes a bootloader image */
#define __BN_exit         3  /* terminates a bootloader image */
#define __BN_program      4  /* program FLASH from a chain */
#define __BN_erase        5  /* erase sector(s) of FLASH */
#define __BN_open         6
#define __BN_write        7
#define __BN_read         8
#define __BN_close        9
#define __BN_mmap         10 /* map a file descriptor into memory */
#define __BN_munmap       11 /* remove a file to memory mapping */
#define __BN_gethwaddr    12 /* get the hardware address of my interfaces */
#define __BN_getserialnum 13 /* get the serial number of this board */
#define __BN_getbenv      14 /* get a bootloader envvar */
#define __BN_setbenv      15 /* get a bootloader envvar */
#define __BN_setpmask     16 /* set the protection mask */
#define __BN_readbenv     17 /* read environment variables */
#define __BN_flash_chattr_range		18
#define __BN_flash_erase_range		19
#define __BN_flash_write_range		20
#define __BN_ramload			21 /* load kernel into RAM and exec */
#define __BN_program2      22  /* program second FLASH from a chain */


/* Calling conventions compatible to (uC)linux/68k
 * We use simmilar macros to call into the bootloader as for uClinux
 */

#define __bsc_return(type, res) \
do { \
   if ((unsigned long)(res) >= (unsigned long)(-64)) { \
      /* let errno be a function, preserve res in %d0 */ \
      /* int __err = -(res); */ \
      /* errno = __err; */ \
      res = -1; \
   } \
   return (type)(res); \
} while (0)

#define _bsc0(type,name) \
type name(void) \
{ \
  long __res;						\
  __asm__ __volatile__ ("movel  %1, %%d0\n\t"           \
                        "trap   #2\n\t"                 \
                        "movel  %%d0, %0"               \
                        : "=g" (__res)                  \
                        : "i" (__BN_##name)             \
                        : "cc", "%d0");                 \
   __bsc_return(type,__res); \
}

#define _bsc1(type,name,atype,a) \
type name(atype a) \
{ \
  long __res;						\
  __asm__ __volatile__ ("movel  %2, %%d1\n\t"           \
                        "movel  %1, %%d0\n\t"           \
                        "trap   #2\n\t"                 \
                        "movel  %%d0, %0"               \
                        : "=g" (__res)                  \
                        : "i" (__BN_##name),            \
                          "g" ((long)a)                 \
                        : "cc", "%d0", "%d1");          \
   __bsc_return(type,__res); \
}

#define _bsc2(type,name,atype,a,btype,b) \
type name(atype a, btype b) \
{ \
  long __res;						\
  __asm__ __volatile__ ("movel  %3, %%d2\n\t"           \
                        "movel  %2, %%d1\n\t"           \
                        "movel  %1, %%d0\n\t"           \
                        "trap   #2\n\t"                 \
                        "movel  %%d0, %0"               \
                        : "=g" (__res)                  \
                        : "i" (__BN_##name),            \
                          "a" ((long)a),                \
                          "g" ((long)b)                 \
                        : "cc", "%d0", "%d1", "%d2");   \
   __bsc_return(type,__res); \
}

#define _bsc3(type,name,atype,a,btype,b,ctype,c) \
type name(atype a, btype b, ctype c) \
{ \
  long __res;						\
  __asm__ __volatile__ ("movel  %4, %%d3\n\t"           \
                        "movel  %3, %%d2\n\t"           \
                        "movel  %2, %%d1\n\t"           \
                        "movel  %1, %%d0\n\t"           \
                        "trap   #2\n\t"                 \
                        "movel  %%d0, %0"               \
                        : "=g" (__res)                  \
                        : "i" (__BN_##name),            \
                          "a" ((long)a),                \
                          "a" ((long)b),                \
                          "g" ((long)c)                 \
                        : "cc", "%d0", "%d1", "%d2", 	\
			   "%d3");                      \
   __bsc_return(type,__res); \
}

#define _bsc4(type,name,atype,a,btype,b,ctype,c,dtype,d) \
type name(atype a, btype b, ctype c, dtype d) \
{ \
  long __res;						\
  __asm__ __volatile__ ("movel  %5, %%d4\n\t"           \
                        "movel  %4, %%d3\n\t"           \
                        "movel  %3, %%d2\n\t"           \
                        "movel  %2, %%d1\n\t"           \
                        "movel  %1, %%d0\n\t"           \
                        "trap   #2\n\t"                 \
                        "movel  %%d0, %0"               \
                        : "=g" (__res)                  \
                        : "i" (__BN_##name),            \
                          "a" ((long)a),                \
                          "a" ((long)b),                \
                          "a" ((long)c),                \
                          "g" ((long)d)                 \
                        : "cc", "%d0", "%d1", "%d2", 	\
			   "%d3", "%d4");               \
   __bsc_return(type,__res); \
}

#define _bsc5(type,name,atype,a,btype,b,ctype,c,dtype,d,etype,e) \
type name(atype a, btype b, ctype c, dtype d, etype e)  \
{ \
  long __res;						\
  __asm__ __volatile__ ("movel  %6, %%d5\n\t"           \
                        "movel  %5, %%d4\n\t"           \
                        "movel  %4, %%d3\n\t"           \
                        "movel  %3, %%d2\n\t"           \
                        "movel  %2, %%d1\n\t"           \
                        "movel  %1, %%d0\n\t"           \
                        "trap   #2\n\t"                 \
                        "movel  %%d0, %0"               \
                        : "=g" (__res)                  \
                        : "i" (__BN_##name),            \
                          "a" ((long)a),                \
                          "a" ((long)b),                \
                          "a" ((long)c),                \
                          "a" ((long)d),                \
                          "g" ((long)e)                 \
                        : "cc", "%d0", "%d1", "%d2", 	\
			   "%d3", "%d4", "%d5");        \
   __bsc_return(type,__res); \
}
