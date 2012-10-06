static inline unsigned short _swapw(volatile unsigned short v)
{
    return ((v << 8) | (v >> 8));
}

static inline unsigned int _swapl(volatile unsigned long v)
{
    return ((v << 24) | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8) | (v >> 24));
}

#define readb(addr) \
    ({ unsigned char __v = (*(volatile unsigned char *) (addr)); __v; })
#define readw(addr) \
    ({ unsigned short __v = (*(volatile unsigned short *) (addr)); __v; })
#define readl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })

#define writeb(b,addr) ((*(volatile unsigned char *) (addr)) = (b))
#define writew(b,addr) ((*(volatile unsigned short *) (addr)) = (b))
#define writel(b,addr) ((*(volatile unsigned int *) (addr)) = (b))

#define inb(addr) \
    ({ unsigned char __v = (*(volatile unsigned char *) (addr)); __v; })
#define inw(addr) \
    ({ unsigned short __v = (*(volatile unsigned short *) (addr)); \
       _swapw(__v); })
#define inl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); _swapl(__v); })

#define outb(b,addr) ((*(volatile unsigned char *) (addr)) = (b))
#define outw(b,addr) ((*(volatile unsigned short *) (addr)) = (_swapw(b)))
#define outl(b,addr) ((*(volatile unsigned int *) (addr)) = (_swapl(b)))

#define outsw(addr,buf,len) \
    ({ unsigned short * __e = (unsigned short *)(buf) + (len); \
       unsigned short * __p = (unsigned short *)(buf); \
       while (__p < __e) { \
	  *(volatile unsigned short *)(addr) = *__p++;\
       } \
     })

#define insw(addr,buf,len) \
    ({ unsigned short * __e = (unsigned short *)(buf) + (len); \
       unsigned short * __p = (unsigned short *)(buf); \
       while (__p < __e) { \
          *(__p++) = *(volatile unsigned short *)(addr); \
       } \
     })
