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
 * $Id: ip.c,v 1.1 2001/12/12 10:00:58 adam Exp $
 */


/*-----------------------------------------------------------------------------------*/
/* ip.c
 *
 * This is the code for the IP layer.
 *
 */   
/*-----------------------------------------------------------------------------------*/

#include "lwip/debug.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/ip.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/icmp.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"

#include "lwip/stats.h"

#include "arch/perf.h"
/*-----------------------------------------------------------------------------------*/
/* ip_init:
 *
 * Initializes the IP layer.
 */
/*-----------------------------------------------------------------------------------*/
void
ip_init(void)
{
}
/*-----------------------------------------------------------------------------------*/
/* ip_route:
 *
 * Finds the appropriate network interface for a given IP address. It searches the
 * list of network interfaces linearly. A match is found if the masked IP address of
 * the network interface equals the masked IP address given to the function.
 */
/*-----------------------------------------------------------------------------------*/
struct netif *
ip_route(struct ip_addr *dest)
{
  struct netif *netif;
  
  for(netif = netif_list; netif != NULL; netif = netif->next) {
    if(ip_addr_maskcmp(dest, &(netif->ip_addr), &(netif->netmask))) {
      return netif;
    }
  }

  return netif_default;
}
#if IP_FORWARD
/*-----------------------------------------------------------------------------------*/
/* ip_forward:
 *
 * Forwards an IP packet. It finds an appropriate route for the packet, decrements
 * the TTL value of the packet, adjusts the checksum and outputs the packet on the
 * appropriate interface.
 */
/*-----------------------------------------------------------------------------------*/
static void
ip_forward(struct pbuf *p, struct ip_hdr *iphdr, struct netif *inp)
{
  static struct netif *netif;
  
  PERF_START;
  
  if((netif = ip_route((struct ip_addr *)&(iphdr->dest))) == NULL) {

    DEBUGF(IP_DEBUG, ("ip_forward: no forwarding route for 0x%lx found\n",
		      iphdr->dest.addr));

    return;
  }

  /* Don't forward packets onto the same network interface on which
     they arrived. */
  if(netif == inp) {
    DEBUGF(IP_DEBUG, ("ip_forward: not forward packets back on incoming interface.\n"));

    return;
  }
  
  /* Decrement TTL and send ICMP if ttl == 0. */
  IPH_TTL_SET(iphdr, IPH_TTL(iphdr) - 1);
  if(IPH_TTL(iphdr) == 0) {
    /* Don't send ICMP messages in response to ICMP messages */
    if(IPH_PROTO(iphdr) != IP_PROTO_ICMP) {
      icmp_time_exceeded(p, ICMP_TE_TTL);
    }
    return;       
  }
  
  /* Incremental update of the IP checksum. */
  if(IPH_CHKSUM(iphdr) >= htons(0xffff - 0x100)) {
    IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + htons(0x100) + 1);
  } else {
    IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + htons(0x100));
  }

  DEBUGF(IP_DEBUG, ("ip_forward: forwarding packet to 0x%lx\n",
		    iphdr->dest.addr));

#ifdef IP_STATS
  ++stats.ip.fw;
  ++stats.ip.xmit;
#endif /* IP_STATS */

  PERF_STOP("ip_forward");
  
  netif->output(netif, p, (struct ip_addr *)&(iphdr->dest));
}
#endif /* IP_FORWARD */
/*-----------------------------------------------------------------------------------*/
/* ip_input:
 *
 * This function is called by the network interface device driver when an IP packet is
 * received. The function does the basic checks of the IP header such as packet size
 * being at least larger than the header size etc. If the packet was not destined for
 * us, the packet is forwarded (using ip_forward). The IP checksum is always checked.
 *
 * Finally, the packet is sent to the upper layer protocol input function.
 */
/*-----------------------------------------------------------------------------------*/
err_t
ip_input(struct pbuf *p, struct netif *inp) {
  static struct ip_hdr *iphdr;
  static struct netif *netif;
  static u8_t hl;

  
  PERF_START;
  
#ifdef IP_STATS
  ++stats.ip.recv;
#endif /* IP_STATS */
  
  dbg_putstr("ip_input 00\r\n");
  /* identify the IP header */
  iphdr = p->payload;
  if(IPH_V(iphdr) != 4) {
    dbg_putstr("ip_input 01\r\n");
    DEBUGF(IP_DEBUG, ("IP packet dropped due to bad version number %d\n", IPH_V(iphdr)));
#if IP_DEBUG
    ip_debug_print(p);
#endif /* IP_DEBUG */
    pbuf_free(p);
#ifdef IP_STATS
    ++stats.ip.err;
    ++stats.ip.drop;
#endif /* IP_STATS */
    return ERR_OK;
  }
  
  dbg_putstr("ip_input 02\r\n");
  hl = IPH_HL(iphdr);
  dbg_putstr("ip_input 03\r\n");
  
  if(hl * 4 > p->len) {
    DEBUGF(IP_DEBUG, ("IP packet dropped due to too short packet %d\n", p->len));

    pbuf_free(p);
#ifdef IP_STATS
    ++stats.ip.lenerr;
    ++stats.ip.drop;
#endif /* IP_STATS */
    return ERR_OK;
  }

  dbg_putstr("ip_input 04\r\n");
  /* verify checksum */
  if(inet_chksum(iphdr, hl * 4) != 0) {

    DEBUGF(IP_DEBUG, ("IP packet dropped due to failing checksum 0x%x\n", inet_chksum(iphdr, hl * 4)));
#if IP_DEBUG
    ip_debug_print(p);
#endif /* IP_DEBUG */
    pbuf_free(p);
#ifdef IP_STATS
    ++stats.ip.chkerr;
    ++stats.ip.drop;
#endif /* IP_STATS */
    return ERR_OK;
  }
  
  dbg_putstr("ip_input 05\r\n");
  /* Trim pbuf. This should have been done at the netif layer,
     but we'll do it anyway just to be sure that its done. */
  pbuf_realloc(p, ntohs(IPH_LEN(iphdr)));

  dbg_putstr("ip_input 06\r\n");
  /* is this packet for us? */
  for(netif = netif_list; netif != NULL; netif = netif->next) {

    DEBUGF(IP_DEBUG, ("ip_input: iphdr->dest 0x%lx netif->ip_addr 0x%lx (0x%lx, 0x%lx, 0x%lx)\n",
		      iphdr->dest.addr, netif->ip_addr.addr,
		      iphdr->dest.addr & netif->netmask.addr,
		      netif->ip_addr.addr & netif->netmask.addr,
		      iphdr->dest.addr & ~(netif->netmask.addr)));

    if(ip_addr_isany(&(netif->ip_addr)) ||
       ip_addr_cmp(&(iphdr->dest), &(netif->ip_addr)) ||
       (ip_addr_isbroadcast(&(iphdr->dest), &(netif->netmask)) &&
	ip_addr_maskcmp(&(iphdr->dest), &(netif->ip_addr), &(netif->netmask)))) {
      break;
    }
  }

  
  dbg_putstr("ip_input 07\r\n");
  if(netif == NULL) {
    /* packet not for us, route or discard */
#if IP_FORWARD
    if(!ip_addr_isbroadcast(&(iphdr->dest), &(inp->netmask))) {
      ip_forward(p, iphdr, inp);
    }
#endif /* IP_FORWARD */
    pbuf_free(p);
    return ERR_OK;
  }

  dbg_putstr("ip_input 8\r\n");
  if((IPH_OFFSET(iphdr) & htons(IP_OFFMASK | IP_MF)) != 0) {
    pbuf_free(p);
    DEBUGF(IP_DEBUG, ("IP packet dropped since it was fragmented (0x%x).\n",
		      ntohs(IPH_OFFSET(iphdr))));
#ifdef IP_STATS
    ++stats.ip.opterr;
    ++stats.ip.drop;
#endif /* IP_STATS */
    return ERR_OK;
  }
  
  dbg_putstr("ip_input 9\r\n");
  if(hl * 4 > IP_HLEN) {
    DEBUGF(IP_DEBUG, ("IP packet dropped since there were IP options.\n"));

    pbuf_free(p);    
#ifdef IP_STATS
    ++stats.ip.opterr;
    ++stats.ip.drop;
#endif /* IP_STATS */
    return ERR_OK;
  }
  

  dbg_putstr("ip_input 10\r\n");
  /* send to upper layers */
#if IP_DEBUG
  DEBUGF(IP_DEBUG, ("ip_input: \n"));
  ip_debug_print(p);
  DEBUGF(IP_DEBUG, ("ip_input: p->len %d p->tot_len %d\n", p->len, p->tot_len));
#endif /* IP_DEBUG */
   

  dbg_putstr("ip_input 11\r\n");
  pbuf_header(p, -IP_HLEN);
  dbg_putstr("ip_input 12\r\n");

  switch(IPH_PROTO(iphdr)) {
#if LWIP_UDP > 0    
  case IP_PROTO_UDP:
    dbg_putstr("ip_input 13\r\n");
    udp_input(p, inp);
    break;
#endif /* LWIP_UDP */
#if LWIP_TCP > 0    
  case IP_PROTO_TCP:
    dbg_putstr("ip_input 14\r\n");
    tcp_input(p, inp);
    break;
#endif /* LWIP_TCP */
  case IP_PROTO_ICMP:
    dbg_putstr("ip_input 15\r\n");
    icmp_input(p, inp);
    break;
  default:
    dbg_putstr("ip_input 16\r\n");
    /* send ICMP destination protocol unreachable unless is was a broadcast */
    if(!ip_addr_isbroadcast(&(iphdr->dest), &(inp->netmask)) &&
       !ip_addr_ismulticast(&(iphdr->dest))) {
      p->payload = iphdr;
      icmp_dest_unreach(p, ICMP_DUR_PROTO);
    }
    pbuf_free(p);

    DEBUGF(IP_DEBUG, ("Unsupported transportation protocol %d\n", IPH_PROTO(iphdr)));

#ifdef IP_STATS
    ++stats.ip.proterr;
    ++stats.ip.drop;
#endif /* IP_STATS */

  }
  dbg_putstr("ip_input 17\r\n");
  PERF_STOP("ip_input");
  return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
/* ip_output_if:
 *
 * Sends an IP packet on a network interface. This function constructs the IP header
 * and calculates the IP header checksum. If the source IP address is NULL,
 * the IP address of the outgoing network interface is filled in as source address.
 */
/*-----------------------------------------------------------------------------------*/
err_t
ip_output_if(struct pbuf *p, struct ip_addr *src, struct ip_addr *dest,
	     u8_t ttl,
	     u8_t proto, struct netif *netif)
{
  static struct ip_hdr *iphdr;
  static u16_t ip_id = 0;

  
  PERF_START;
  
  dbg_putstr("ip_output_if 0\r\n");
  if(pbuf_header(p, IP_HLEN)) {
    dbg_putstr("ip_output_if 1\r\n");
    DEBUGF(IP_DEBUG, ("ip_output: not enough room for IP header in pbuf\n"));

#ifdef IP_STATS
    ++stats.ip.err;
#endif /* IP_STATS */
    pbuf_free(p);
    return ERR_BUF;
  }

  dbg_putstr("ip_output_if 2\r\n");
  iphdr = p->payload;
  

  if(dest != IP_HDRINCL) {
    dbg_putstr("ip_output_if 3\r\n");
    IPH_TTL_SET(iphdr, ttl);
    IPH_PROTO_SET(iphdr, proto);
    
    dbg_putstr("ip_output_if 4\r\n");
    ip_addr_set(&(iphdr->dest), dest);

    dbg_putstr("ip_output_if 5\r\n");
    IPH_VHLTOS_SET(iphdr, 4, IP_HLEN / 4, 0);
    dbg_putstr("ip_output_if 6\r\n");
    IPH_LEN_SET(iphdr, htons(p->tot_len));
    dbg_putstr("ip_output_if 7\r\n");
    IPH_OFFSET_SET(iphdr, 0);
    dbg_putstr("ip_output_if 8\r\n");
    IPH_ID_SET(iphdr, htons(++ip_id));
    dbg_putstr("ip_output_if 9\r\n");

    if(ip_addr_isany(src)) {
      dbg_putstr("ip_output_if 10\r\n");
      ip_addr_set(&(iphdr->src), &(netif->ip_addr));
    } else {
      dbg_putstr("ip_output_if 11\r\n");
      ip_addr_set(&(iphdr->src), src);
    }

    dbg_putstr("ip_output_if 12\r\n");
    IPH_CHKSUM_SET(iphdr, 0);
    dbg_putstr("ip_output_if 13\r\n");
    IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, IP_HLEN));
  } else {
    dbg_putstr("ip_output_if 14\r\n");
    dest = &(iphdr->dest);
  }

#ifdef IP_STATS
  stats.ip.xmit++;
#endif /* IP_STATS */
  DEBUGF(IP_DEBUG, ("ip_output_if: %c%c ", netif->name[0], netif->name[1]));
#if IP_DEBUG
  ip_debug_print(p);
#endif /* IP_DEBUG */

  PERF_STOP("ip_output_if");

  dbg_putstr("ip_output_if 15\r\n");
  return netif->output(netif, p, dest);  
}
/*-----------------------------------------------------------------------------------*/
/* ip_output:
 *
 * Simple interface to ip_output_if. It finds the outgoing network interface and
 * calls upon ip_output_if to do the actual work.
 */
/*-----------------------------------------------------------------------------------*/
err_t
ip_output(struct pbuf *p, struct ip_addr *src, struct ip_addr *dest,
	  u8_t ttl, u8_t proto)
{
  static struct netif *netif;

  dbg_putstr("ip_output 0\r\n");
  
  if((netif = ip_route(dest)) == NULL) {
    DEBUGF(IP_DEBUG, ("ip_output: No route to 0x%lx\n", dest->addr));

    dbg_putstr("ip_output 1\r\n");
#ifdef IP_STATS
    ++stats.ip.rterr;
#endif /* IP_STATS */
    pbuf_free(p);
    return ERR_RTE;
  }

  dbg_putstr("ip_output 2\r\n");
  return ip_output_if(p, src, dest, ttl, proto, netif);
}
/*-----------------------------------------------------------------------------------*/
#if IP_DEBUG
void
ip_debug_print(struct pbuf *p)
{
  struct ip_hdr *iphdr = p->payload;
  u8_t *payload;

  payload = (u8_t *)iphdr + IP_HLEN/sizeof(u8_t);
  
  DEBUGF(IP_DEBUG, ("IP header:\n"));
  DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  DEBUGF(IP_DEBUG, ("|%2d |%2d |   %2d  |      %4d     | (v, hl, tos, len)\n",
		    IPH_V(iphdr),
		    IPH_HL(iphdr),
		    IPH_TOS(iphdr),
		    ntohs(IPH_LEN(iphdr))));
  DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  DEBUGF(IP_DEBUG, ("|    %5d      |%d%d%d|    %4d   | (id, flags, offset)\n",
		    ntohs(IPH_ID(iphdr)),
		    ntohs(IPH_OFFSET(iphdr)) >> 15 & 1,
		    ntohs(IPH_OFFSET(iphdr)) >> 14 & 1,
		    ntohs(IPH_OFFSET(iphdr)) >> 13 & 1,
		    ntohs(IPH_OFFSET(iphdr)) & IP_OFFMASK));
  DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  DEBUGF(IP_DEBUG, ("|   %2d  |   %2d  |    0x%04x     | (ttl, proto, chksum)\n",
		    IPH_TTL(iphdr),
		    IPH_PROTO(iphdr),
		    ntohs(IPH_CHKSUM(iphdr))));
  DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  DEBUGF(IP_DEBUG, ("|  %3ld  |  %3ld  |  %3ld  |  %3ld  | (src)\n",
		    ntohl(iphdr->src.addr) >> 24 & 0xff,
		    ntohl(iphdr->src.addr) >> 16 & 0xff,
		    ntohl(iphdr->src.addr) >> 8 & 0xff,
		    ntohl(iphdr->src.addr) & 0xff));
  DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  DEBUGF(IP_DEBUG, ("|  %3ld  |  %3ld  |  %3ld  |  %3ld  | (dest)\n",
		    ntohl(iphdr->dest.addr) >> 24 & 0xff,
		    ntohl(iphdr->dest.addr) >> 16 & 0xff,
		    ntohl(iphdr->dest.addr) >> 8 & 0xff,
		    ntohl(iphdr->dest.addr) & 0xff));
  DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
}
#endif /* IP_DEBUG */
/*-----------------------------------------------------------------------------------*/





