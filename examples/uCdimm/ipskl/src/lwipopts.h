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
 * $Id: lwipopts.h,v 1.1 2001/12/12 10:00:53 adam Exp $
 */
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/* ---------- Statistics options ---------- */
#undef STATS

/* ---------- IP options ---------- */
/* Define IP_FORWARD to 1 if you wish to have the ability to forward
   IP packets across network interfaces. If you are going to run lwIP
   on a device with only one network interface, don'define this to
   0. */
#define IP_FORWARD              0

/* ---------- ICMP options ---------- */
#define ICMP_TTL                255

/* ---------- UDP options ---------- */
#define UDP_TTL                 255

/* ---------- TCP options ---------- */
#define TCP_TTL                 255

#define TCP_QUEUE_OOSEQ         1

/* TCP Maximum segment size. */
#define TCP_MSS                 64

/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF             128
/* TCP sender buffer space (pbufs). This must be at least = 2 *
   TCP_SND_BUF/TCP_MSS for things to work. */
#define TCP_SND_QUEUELEN        2 * TCP_SND_BUF/TCP_MSS

/* TCP receive window. */
#define TCP_WND                 1024

/* Maximum number of retransmissions of data segments. */
#define TCP_MAXRTX              12

/* Maximum number of retransmissions of SYN segments. */
#define TCP_SYNMAXRTX           4

/* ---------- Memory options ---------- */
#define MEM_ALIGNMENT           2

#define MEM_SIZE                16384

#define MEM_RECLAIM             1
#define MEMP_RECLAIM            1

#define MEMP_NUM_PBUF           4 
#define MEMP_NUM_UDP_PCB        0
#define MEMP_NUM_TCP_PCB        5
#define MEMP_NUM_TCP_PCB_LISTEN 8
#define MEMP_NUM_TCP_SEG        5 
#define MEMP_NUM_NETBUF         2 
#define MEMP_NUM_NETCONN        4 
#define MEMP_NUM_API_MSG        8
#define MEMP_NUM_TCPIP_MSG      8

#define MEMP_NUM_SYS_TIMEOUT    2

/* ---------- Pbuf options ---------- */

#define PBUF_POOL_SIZE          6
#define PBUF_POOL_BUFSIZE       128

#define PBUF_LINK_HLEN          16

#endif /* __LWIPOPTS_H__ */
