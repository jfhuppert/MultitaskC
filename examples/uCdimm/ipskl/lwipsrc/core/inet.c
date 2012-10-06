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
 * $Id: inet.c,v 1.1 2001/12/12 10:00:58 adam Exp $
 */

/*-----------------------------------------------------------------------------------*/
/* inet.c
 *
 * Functions common to all TCP/IP modules, such as the Internet checksum and the
 * byte order functions.
 *
 */
/*-----------------------------------------------------------------------------------*/

#include "lwip/debug.h"

#include "lwip/arch.h"

#include "lwip/def.h"
#include "lwip/inet.h"


/*-----------------------------------------------------------------------------------*/
/* chksum:
 *
 * Sums up all 16 bit words in a memory portion. Also includes any odd byte.
 * This function is used by the other checksum functions.
 *
 * For now, this is not optimized. Must be optimized for the particular processor
 * arcitecture on which it is to run. Preferebly coded in assembler.
 */
/*-----------------------------------------------------------------------------------*/
static u32_t 
chksum(void *dataptr, int len)
{
  u32_t acc;
  unsigned char *dptr=(unsigned char *)dataptr;
  u16_t tmp0;
    
  dbg_putstr("chksum 0 dataptr==0x");
  dbg_puthex((unsigned long)dptr);
  dbg_putstr("\r\n");
  for(acc = 0; len > 1; len -= 2) {
    tmp0=(u16_t)((((*dptr)&0xff)<<8)|((*(dptr+1))&0xff));
    // acc += (u16_t*((u16_t *)dataptr)++;
    acc += tmp0;
    dptr+=2;
  }

  dbg_putstr("chksum 1\r\n");
  /* add up any odd byte */
  if(len == 1) {
    dbg_putstr("chksum 2\r\n");
    acc += htons((u16_t)((*(u8_t *)dptr) & 0xff) << 8);
    DEBUGF(INET_DEBUG, ("inet: chksum: odd byte %d\n", *(u8_t *)dataptr));
  }

  dbg_putstr("chksum 3\r\n");
  return acc;
}
/*-----------------------------------------------------------------------------------*/
/* inet_chksum_pseudo:
 *
 * Calculates the pseudo Internet checksum used by TCP and UDP for a pbuf chain.
 */
/*-----------------------------------------------------------------------------------*/
u16_t
inet_chksum_pseudo(struct pbuf *p,
		   struct ip_addr *src, struct ip_addr *dest,
		   u8_t proto, u16_t proto_len)
{
  u32_t acc;
  struct pbuf *q;
  u8_t swapped;

  dbg_putstr("inet_chksum_pseudo 0\r\n");
  acc = 0;
  swapped = 0;
  for(q = p; q != NULL; q = q->next) {    
    acc += chksum(q->payload, q->len);
    while(acc >> 16) {
      acc = (acc & 0xffff) + (acc >> 16);
    }
    if(q->len % 2 != 0) {
      swapped = 1 - swapped;
      acc = ((acc & 0xff) << 8) | ((acc & 0xff00) >> 8);
    }
  }

  dbg_putstr("inet_chksum_pseudo 1\r\n");
  if(swapped) {
    dbg_putstr("inet_chksum_pseudo 2\r\n");
    acc = ((acc & 0xff) << 8) | ((acc & 0xff00) >> 8);
  }
  dbg_putstr("inet_chksum_pseudo 3\r\n");
  acc += (src->addr & 0xffff);
  acc += ((src->addr >> 16) & 0xffff);
  acc += (dest->addr & 0xffff);
  acc += ((dest->addr >> 16) & 0xffff);
  acc += (u32_t)htons((u16_t)proto);
  acc += (u32_t)htons(proto_len);  
  
  dbg_putstr("inet_chksum_pseudo 4\r\n");
  while(acc >> 16) {
    acc = (acc & 0xffff) + (acc >> 16);
  }    
  dbg_putstr("inet_chksum_pseudo 5\r\n");
  return ~(acc & 0xffff);
}
/*-----------------------------------------------------------------------------------*/
/* inet_chksum:
 *
 * Calculates the Internet checksum over a portion of memory. Used primarely for IP
 * and ICMP.
 */
/*-----------------------------------------------------------------------------------*/
u16_t
inet_chksum(void *dataptr, u16_t len)
{
  u32_t acc;

  dbg_putstr("inet_chksum 0\r\n");
  acc = chksum(dataptr, len);
  dbg_putstr("inet_chksum 1\r\n");
  while(acc >> 16) {
    acc = (acc & 0xffff) + (acc >> 16);
  }    
  dbg_putstr("inet_chksum 2\r\n");
  return ~(acc & 0xffff);
}
/*-----------------------------------------------------------------------------------*/
u16_t
inet_chksum_pbuf(struct pbuf *p)
{
  u32_t acc;
  struct pbuf *q;
  u8_t swapped;
  
  dbg_putstr("inet_chksum_pbuf 0\r\n");
  acc = 0;
  swapped = 0;
  for(q = p; q != NULL; q = q->next) {
    dbg_putstr("inet_chksum_pbuf 1\r\n");
    acc += chksum(q->payload, q->len);
    while(acc >> 16) {
      acc = (acc & 0xffff) + (acc >> 16);
    }    
    dbg_putstr("inet_chksum_pbuf 2\r\n");
    if(q->len % 2 != 0) {
      swapped = 1 - swapped;
      acc = (acc & 0xff << 8) | (acc & 0xff00 >> 8);
    }
  }
 
  dbg_putstr("inet_chksum_pbuf 3\r\n");
  if(swapped) {
    acc = ((acc & 0xff) << 8) | ((acc & 0xff00) >> 8);
  }
  dbg_putstr("inet_chksum_pbuf 4\r\n");
  return ~(acc & 0xffff);
}


/*-----------------------------------------------------------------------------------*/
