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
 * $Id: udp.c,v 1.1 2001/12/12 10:00:58 adam Exp $
 */

/*-----------------------------------------------------------------------------------*/
/* udp.c
 *
 * The code for the User Datagram Protocol UDP.
 *
 */
/*-----------------------------------------------------------------------------------*/
#include "lwip/debug.h"

#include "lwip/def.h"
#include "lwip/memp.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/udp.h"
#include "lwip/icmp.h"

#include "lwip/stats.h"

/*-----------------------------------------------------------------------------------*/

/* The list of UDP PCBs. */
static struct udp_pcb *udp_pcbs = NULL;

#if UDP_DEBUG
int udp_debug_print(struct udp_hdr *udphdr);
#endif /* UDP_DEBUG */
	  
/*-----------------------------------------------------------------------------------*/
void
udp_init(void)
{
}
/*-----------------------------------------------------------------------------------*/
void
udp_input(struct pbuf *p, struct netif *inp)
{
  struct udp_hdr *udphdr;  
  struct udp_pcb *pcb;
  struct ip_hdr *iphdr;

#ifdef UDP_STATS
  ++stats.udp.recv;
#endif /* UDP_STATS */
  
  iphdr = (struct ip_hdr *)((char *)p->payload - IP_HLEN);
  udphdr = p->payload;

  
#ifdef IPv6
  if(iphdr->nexthdr == IP_PROTO_UDPLITE) {    
#else
  if(IPH_PROTO(iphdr) == IP_PROTO_UDPLITE) {    
#endif /* IPv4 */
    /* Do the UDP Lite checksum */
    if(inet_chksum_pseudo(p, (struct ip_addr *)&(iphdr->src),
			  (struct ip_addr *)&(iphdr->dest),
			  IP_PROTO_UDPLITE, ntohs(udphdr->len)) != 0) {
      DEBUGF(UDP_DEBUG, ("udp_input: UDP Lite datagram discarded due to failing checksum\n"));
#ifdef UDP_STATS
      ++stats.udp.chkerr;
      ++stats.udp.drop;
#endif /* UDP_STATS */
      pbuf_free(p);
      return;
    }
 } else {
    if(udphdr->chksum != 0) {
      if(inet_chksum_pseudo(p, (struct ip_addr *)&(iphdr->src),
			    (struct ip_addr *)&(iphdr->dest),
			    IP_PROTO_UDP, p->tot_len) != 0) {
	DEBUGF(UDP_DEBUG, ("udp_input: UDP datagram discarded due to failing checksum\n"));

#ifdef UDP_STATS
	++stats.udp.chkerr;
	++stats.udp.drop;
#endif /* UDP_STATS */
	pbuf_free(p);
	return;
      }
    }
  }

  pbuf_header(p, -UDP_HLEN);

  DEBUGF(UDP_DEBUG, ("udp_input: received datagram of length %d\n", p->tot_len));
	
  udphdr->src = ntohs(udphdr->src);
  udphdr->dest = ntohs(udphdr->dest);
  /*  udphdr->len = ntohs(udphdr->len);*/

#if UDP_DEBUG
  udp_debug_print(udphdr);
#endif /* UDP_DEBUG */
  
  /* Demultiplex packet. First, go for a perfect match. */
  for(pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
    DEBUGF(UDP_DEBUG, ("udp_input: pcb local port %d (dgram %d)\n",
		       pcb->local_port, udphdr->dest));
    if(pcb->dest_port == udphdr->src &&
       pcb->local_port == udphdr->dest &&
       (ip_addr_isany(&pcb->dest_ip) ||
	ip_addr_cmp(&(pcb->dest_ip), &(iphdr->src))) &&
       (ip_addr_isany(&pcb->local_ip) ||
	ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest)))) {
      pcb->recv(pcb->recv_arg, pcb, p, &(iphdr->src), udphdr->src);
      return;
    }
  }
  
  for(pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
    DEBUGF(UDP_DEBUG, ("udp_input: pcb local port %d (dgram %d)\n",
		       pcb->local_port, udphdr->dest));
    if(pcb->local_port == udphdr->dest &&
       (ip_addr_isany(&pcb->dest_ip) ||
	ip_addr_cmp(&(pcb->dest_ip), &(iphdr->src))) &&
       (ip_addr_isany(&pcb->local_ip) ||
	ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest)))) {
      pcb->recv(pcb->recv_arg, pcb, p, &(iphdr->src), udphdr->src);
      return;
    }
  }
  
  
  /* No match was found, send ICMP destination port unreachable unless
     destination address was broadcast/multicast. */

  if(!ip_addr_isbroadcast(&iphdr->dest, &inp->netmask) &&
     !ip_addr_ismulticast(&iphdr->dest)) {
    
    /* deconvert from host to network byte order */
    udphdr->src = htons(udphdr->src);
    udphdr->dest = htons(udphdr->dest); 
  
    /* adjust pbuf pointer */
    p->payload = iphdr;
    icmp_dest_unreach(p, ICMP_DUR_PORT);
  }
#ifdef UDP_STATS
  ++stats.udp.proterr;
  ++stats.udp.drop;
#endif /* UDP_STATS */  
  pbuf_free(p);
}
/*-----------------------------------------------------------------------------------*/
err_t
udp_send(struct udp_pcb *pcb, struct pbuf *p)
{
  struct udp_hdr *udphdr;
  struct netif *netif;
  struct ip_addr *src_ip;
  err_t err;
  
  if(pbuf_header(p, UDP_HLEN)) {
    DEBUGF(UDP_DEBUG, ("udp_send: not enough room for UDP header in pbuf\n"));

#ifdef UDP_STATS
    ++stats.udp.err;
#endif /* UDP_STATS */
    return ERR_BUF;
  }

  udphdr = p->payload;
  udphdr->src = htons(pcb->local_port);
  udphdr->dest = htons(pcb->dest_port);
  udphdr->chksum = 0x0000;

  if((netif = ip_route(&(pcb->dest_ip))) == NULL) {
    DEBUGF(UDP_DEBUG, ("udp_send: No route to 0x%lx\n", pcb->dest_ip.addr));
#ifdef UDP_STATS
    ++stats.udp.rterr;
#endif /* UDP_STATS */
    return ERR_RTE;
  }

  if(ip_addr_isany(&pcb->local_ip)) {
    src_ip = &(netif->ip_addr);
  } else {
    src_ip = &(pcb->local_ip);
  }
  
  DEBUGF(UDP_DEBUG, ("udp_send: sending datagram of length %d\n", p->tot_len));
  
  if(pcb->flags & UDP_FLAGS_UDPLITE) {
    udphdr->len = htons(pcb->chksum_len);
    /* calculate checksum */
    udphdr->chksum = inet_chksum_pseudo(p, src_ip, &(pcb->dest_ip),
					IP_PROTO_UDP, pcb->chksum_len);
    if(udphdr->chksum == 0x0000) {
      udphdr->chksum = 0xffff;
    }
    err = ip_output_if(p, src_ip, &pcb->dest_ip, UDP_TTL, IP_PROTO_UDPLITE, netif);    
  } else {
    udphdr->len = htons(p->tot_len);
    /* calculate checksum */
    if((pcb->flags & UDP_FLAGS_NOCHKSUM) == 0) {
      udphdr->chksum = inet_chksum_pseudo(p, src_ip, &pcb->dest_ip,
					  IP_PROTO_UDP, p->tot_len);
      if(udphdr->chksum == 0x0000) {
	udphdr->chksum = 0xffff;
      }
    }
    err = ip_output_if(p, src_ip, &pcb->dest_ip, UDP_TTL, IP_PROTO_UDP, netif);
  }
  
#ifdef UDP_STATS
  ++stats.udp.xmit;
#endif /* UDP_STATS */
  return err;
}
/*-----------------------------------------------------------------------------------*/
err_t
udp_bind(struct udp_pcb *pcb, struct ip_addr *ipaddr, u16_t port)
{
  struct udp_pcb *ipcb;
  if(ipaddr != NULL) {
    pcb->local_ip = *ipaddr;
  }
  pcb->local_port = port;

  /* Insert UDP PCB into the list of active UDP PCBs. */
  for(ipcb = udp_pcbs; ipcb != NULL; ipcb = ipcb->next) {
    if(pcb == ipcb) {
      /* Already on the list, just return. */
      return ERR_OK;
    }
  }
  /* We need to place the PCB on the list. */
  pcb->next = udp_pcbs;
  udp_pcbs = pcb;

  DEBUGF(UDP_DEBUG, ("udp_bind: bound to port %d\n", port));
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
err_t
udp_connect(struct udp_pcb *pcb, struct ip_addr *ipaddr, u16_t port)
{
  struct udp_pcb *ipcb;
  if(ipaddr != NULL) {
    pcb->dest_ip = *ipaddr;
  }
  pcb->dest_port = port;

  /* Insert UDP PCB into the list of active UDP PCBs. */
  for(ipcb = udp_pcbs; ipcb != NULL; ipcb = ipcb->next) {
    if(pcb == ipcb) {
      /* Already on the list, just return. */
      return ERR_OK;
    }
  }
  /* We need to place the PCB on the list. */
  pcb->next = udp_pcbs;
  udp_pcbs = pcb;
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
void
udp_recv(struct udp_pcb *pcb,
	 void (* recv)(void *arg, struct udp_pcb *upcb, struct pbuf *p,
		       struct ip_addr *addr, u16_t port),
	 void *recv_arg)
{
  pcb->recv = recv;
  pcb->recv_arg = recv_arg;
}
/*-----------------------------------------------------------------------------------*/
void
udp_remove(struct udp_pcb *pcb)
{
  struct udp_pcb *pcb2;
  
  if(udp_pcbs == pcb) {
    udp_pcbs = udp_pcbs->next;
  } else for(pcb2 = udp_pcbs; pcb2 != NULL; pcb2 = pcb2->next) {
    if(pcb2->next != NULL && pcb2->next == pcb) {
      pcb2->next = pcb->next;
    }
  }

  memp_free(MEMP_UDP_PCB, pcb);  
}
/*-----------------------------------------------------------------------------------*/
struct udp_pcb *
udp_new(void) {
  struct udp_pcb *pcb;
  pcb = memp_malloc(MEMP_UDP_PCB);
  if(pcb != NULL) {
    bzero(pcb, sizeof(struct udp_pcb));
    return pcb;
  }
  return NULL;

}
/*-----------------------------------------------------------------------------------*/
#if UDP_DEBUG
int
udp_debug_print(struct udp_hdr *udphdr)
{
  DEBUGF(UDP_DEBUG, ("UDP header:\n"));
  DEBUGF(UDP_DEBUG, ("+-------------------------------+\n"));
  DEBUGF(UDP_DEBUG, ("|     %5d     |     %5d     | (src port, dest port)\n",
		     ntohs(udphdr->src), ntohs(udphdr->dest)));
  DEBUGF(UDP_DEBUG, ("+-------------------------------+\n"));
  DEBUGF(UDP_DEBUG, ("|     %5d     |     0x%04x    | (len, chksum)\n",
		     ntohs(udphdr->len), ntohs(udphdr->chksum)));
  DEBUGF(UDP_DEBUG, ("+-------------------------------+\n"));
  return 0;
}
#endif /* UDP_DEBUG */
/*-----------------------------------------------------------------------------------*/










