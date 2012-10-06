/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: debug.h,v 1.1 2001/12/12 10:01:00 adam Exp $
 */
#ifndef __LWIP_DEBUG_H__
#define __LWIP_DEBUG_H__

#undef LLC_LWIP_DEBUG

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

#ifdef LLC_LWIP_DEBUG
#define dbg_putchr(c) putchr(c)
#define dbg_getchr() getchr()
#define dbg_nb_putchr(c) nb_putchr(c)
#define dbg_nb_getchr() nb_getchr()
#define dbg_putstr(s) putstr(s)
#define dbg_print(m,v) print(m,v)
#define dbg_puthex32(val) puthex32(val)
#define dbg_puthex16(val) puthex16(val)
#define dbg_puthex8(val) puthex8(val)
#define dbg_puthex(val) puthex(val)
#else
#define dbg_putchr(c)
#define dbg_getchr()
#define dbg_nb_putchr(c)
#define dbg_nb_getchr()
#define dbg_putstr(s)
#define dbg_print(m,v)
#define dbg_puthex32(val)
#define dbg_puthex16(val)
#define dbg_puthex8(val)
#define dbg_puthex(val)
#endif

#ifdef LWIP_DEBUG

#define ASSERT(x,y) if(!(y)) {printf("Assertion \"%s\" failed at line %d in %s\n", \
                                     x, __LINE__, __FILE__); fflush(NULL); abort();}

/* These defines control the amount of debugging output: */
#define MEM_TRACKING

#define DEMO_DEBUG       0

#define ARP_DEBUG        0

#define NETIF_DEBUG      1
#define PBUF_DEBUG       0
#define DELIF_DEBUG      0
#define DROPIF_DEBUG     0
#define TUNIF_DEBUG      0
#define UNIXIF_DEBUG     0
#define TAPIF_DEBUG      0

#define API_LIB_DEBUG    0
#define API_MSG_DEBUG    0
#define SOCKETS_DEBUG    1
#define ICMP_DEBUG       0
#define INET_DEBUG       0
#define IP_DEBUG         0
#define MEM_DEBUG        0
#define MEMP_DEBUG       0
#define SYS_DEBUG        0
#define TCP_DEBUG        0
#define TCP_INPUT_DEBUG  0
#define TCP_FR_DEBUG     0
#define TCP_RTO_DEBUG    0
#define TCP_REXMIT_DEBUG 0
#define TCP_CWND_DEBUG   0
#define TCP_WND_DEBUG    0
#define TCP_OUTPUT_DEBUG 0
#define TCP_RST_DEBUG    0
#define UDP_DEBUG        0
#define TCPIP_DEBUG      0
#define TCPDUMP_DEBUG    0

#include <stdio.h>
#define DEBUGF(debug, x) do { if(debug){ printf x; } } while(0)


#else /* LWIP_DEBUG */

/* DEBUG is not defined, so we define null macros for ASSERT and DEBUGF */

#define ASSERT(x,y)
#define DEBUGF(debug, x)

/* And we define those to be zero: */

#define DEMO_DEBUG       0
#define ARP_DEBUG        0
#define NETIF_DEBUG      0
#define PBUF_DEBUG       0
#define DELIF_DEBUG      0
#define DROPIF_DEBUG     0
#define TUNIF_DEBUG      0
#define UNIXIF_DEBUG     0
#define TAPIF_DEBUG      0
#define API_LIB_DEBUG    0
#define API_MSG_DEBUG    0
#define SOCKETS_DEBUG    0
#define ICMP_DEBUG       0
#define INET_DEBUG       0
#define IP_DEBUG         0
#define MEM_DEBUG        0
#define MEMP_DEBUG       0
#define SYS_DEBUG        0
#define TCP_DEBUG        0
#define TCP_INPUT_DEBUG  0
#define TCP_FR_DEBUG     0
#define TCP_RTO_DEBUG    0
#define TCP_REXMIT_DEBUG 0
#define TCP_CWND_DEBUG   0
#define TCP_WND_DEBUG    0
#define TCP_OUTPUT_DEBUG 0
#define TCP_RST_DEBUG    0
#define UDP_DEBUG        0
#define TCPIP_DEBUG      0
#define TCPDUMP_DEBUG    0

#endif /* LWIP_DEBUG */


#endif /* __LWIP_DEBUG_H__ */






