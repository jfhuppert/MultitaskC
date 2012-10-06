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
 * $Id: tcp_output.c,v 1.1 2001/12/12 10:00:58 adam Exp $
 */

/*-----------------------------------------------------------------------------------*/
/* tcp_output.c
 *
 * The output functions of TCP.
 *
 */
/*-----------------------------------------------------------------------------------*/

#include "lwip/debug.h"

#include "lwip/def.h"
#include "lwip/opt.h"

#include "arch/lib.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"

#include "lwip/netif.h"

#include "lwip/inet.h"
#include "lwip/tcp.h"

#include "lwip/stats.h"


#define MIN(x,y) (x) < (y)? (x): (y)



/* Forward declarations.*/
static void tcp_output_segment(struct tcp_seg *seg, struct tcp_pcb *pcb);


/*-----------------------------------------------------------------------------------*/
err_t
tcp_send_ctrl(struct tcp_pcb *pcb, u8_t flags)
{
  return tcp_enqueue(pcb, NULL, 0, flags, 1, NULL, 0);

}
/*-----------------------------------------------------------------------------------*/
err_t
tcp_write(struct tcp_pcb *pcb, const void *arg, u16_t len, u8_t copy)
{
  if(pcb->state == SYN_SENT ||
     pcb->state == SYN_RCVD ||
     pcb->state == ESTABLISHED ||
     pcb->state == CLOSE_WAIT) {
    if(len > 0) {
      return tcp_enqueue(pcb, (void *)arg, len, 0, copy, NULL, 0);
    }
    return ERR_OK;
  } else {
    return ERR_CONN;
  }
}
/*-----------------------------------------------------------------------------------*/
err_t
tcp_enqueue(struct tcp_pcb *pcb, void *arg, u16_t len,
	    u8_t flags, u8_t copy,
            u8_t *optdata, u8_t optlen)
{
  struct pbuf *p;
  struct tcp_seg *seg, *useg, *queue;
  u32_t left, seqno;
  u16_t seglen;
  struct tcp_hdr *tcphdr;
  void *ptr;
  u8_t queuelen;

  left = len;
  ptr = arg;
  
  dbg_putstr("tcp_enqueue 0\r\n");
  if(len == 0 && flags == TCP_ACK) {
    dbg_putstr("tcp_enqueue 1\r\n");
    /* If we are requested to send a bare ACK segment, we get here. */
    p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_RAM);
    if(p == NULL) {
      DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: (ACK) could not allocate pbuf\n"));
      return ERR_BUF;
    }
    DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: sending ACK for %lu\n", pcb->rcv_nxt));    
    if(pbuf_header(p, TCP_HLEN)) {
      DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: (ACK) no room for TCP header in pbuf.\n"));
      
#ifdef TCP_STATS
      ++stats.tcp.err;
#endif /* TCP_STATS */
      pbuf_free(p);
      return ERR_BUF;
    }
    
    dbg_putstr("tcp_enqueue 2\r\n");
    tcphdr = p->payload;
    tcphdr->src = htons(pcb->local_port);
    tcphdr->dest = htons(pcb->dest_port);
    tcphdr->seqno = htonl(pcb->snd_nxt);
    tcphdr->ackno = htonl(pcb->rcv_nxt);
    TCPH_FLAGS_SET(tcphdr, flags);
    tcphdr->wnd = htons(pcb->rcv_wnd);
    tcphdr->urgp = 0;
    TCPH_OFFSET_SET(tcphdr, 5 << 4);
    
    tcphdr->chksum = 0;
    dbg_putstr("tcp_enqueue 3\r\n");
    tcphdr->chksum = inet_chksum_pseudo(p, &(pcb->local_ip), &(pcb->dest_ip),
					IP_PROTO_TCP, p->tot_len);
    
    dbg_putstr("tcp_enqueue 4\r\n");
    seg = memp_malloc(MEMP_TCP_SEG);
    if(seg == NULL) {
      DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: (ACK) could not allocate memory for seg\n"));
#ifdef TCP_STATS
      ++stats.tcp.memerr;
#endif /* TCP_STATS */
      pbuf_free(p);
      return ERR_MEM;
    }
    seg->p = p;
    seg->tcphdr = tcphdr;
    seg->next = pcb->unsent;
    seg->len = 0;
    pcb->unsent = seg;
    dbg_putstr("tcp_enqueue 5\r\n");
    return ERR_OK;
  } else {
    dbg_putstr("tcp_enqueue 6\r\n");

    /* We get here if we are requested to enqueue application data. */    
    if(len > pcb->snd_buf) {
      DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: too much data %d\n", len));
      return ERR_MEM;
    }

    seqno = pcb->snd_lbb;
    
    dbg_putstr("tcp_enqueue 7\r\n");
    queue = NULL;
    queuelen = pcb->snd_queuelen;
    if(queuelen >= TCP_SND_QUEUELEN) {
      DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: too long queue %d (max %d)\n", queuelen, TCP_SND_QUEUELEN));
      goto memerr;
    }   
    
#ifdef LWIP_DEBUG
    if(pcb->snd_queuelen != 0) {
      ASSERT("tcp_enqueue: valid queue length", pcb->unacked != NULL ||
	     pcb->unsent != NULL);      
    }
#endif /* LWIP_DEBUG */

    seg = NULL;
    seglen = 0;
    
    dbg_putstr("tcp_enqueue 8\r\n");
    while(queue == NULL || left > 0) {
      
      seglen = left > pcb->mss? pcb->mss: left;

      /* allocate memory for tcp_seg, and fill in fields */
      seg = memp_malloc(MEMP_TCP_SEG);
      if(seg == NULL) {
	DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: could not allocate memory for tcp_seg\n"));
	goto memerr;
      }
      seg->next = NULL;
      seg->p = NULL;

      
      if(queue == NULL) {
	queue = seg;
      } else {
	for(useg = queue; useg->next != NULL; useg = useg->next);
	useg->next = seg;
      }
      
      /* If copy is set, memory should be allocated
	 and data copied into pbuf, otherwise data comes from
	 ROM or other static memory, and need not be copied. If
         optdata is != NULL, we have options instead of data. */
      if(optdata != NULL) {
        dbg_putstr("tcp_enqueue 9\r\n");
	if((seg->p = pbuf_alloc(PBUF_TRANSPORT, optlen, PBUF_RAM)) == NULL) {
	  goto memerr;
	}
	++queuelen;
      /* ######## */
	// seg->dataptr = seg->p->payload;
	{ struct pbuf *tmp; tmp=seg->p; seg->dataptr = tmp->payload; }
        dbg_putstr("tcp_enqueue 10\r\n");
      } else if(copy) {
        dbg_putstr("tcp_enqueue 11\r\n");
	if((seg->p = pbuf_alloc(PBUF_TRANSPORT, seglen, PBUF_RAM)) == NULL) {
	  DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: could not allocate memory for pbuf copy\n"));	  
	  goto memerr;
	}
	++queuelen;
	if(arg != NULL) {
      /* ######## */
          struct pbuf *tmp_pbuf=seg->p;
	  bcopy(ptr, tmp_pbuf->payload, seglen);
	}
        dbg_putstr("tcp_enqueue 12\r\n");
      /* ######## */
	{ struct pbuf *tmp=seg->p; seg->dataptr = tmp->payload; }
        dbg_putstr("tcp_enqueue 13\r\n");
      } else {
        dbg_putstr("tcp_enqueue 14\r\n");
        /* Do not copy the data. */
	if((p = pbuf_alloc(PBUF_TRANSPORT, seglen, PBUF_ROM)) == NULL) {
	  DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: could not allocate memory for pbuf non-copy\n"));	  	  
	  goto memerr;
	}
	++queuelen;
        p->payload = ptr;
	seg->dataptr = ptr;
	if((seg->p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_RAM)) == NULL) {
          pbuf_free(p);
	  DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: could not allocate memory for header pbuf\n"));		  
	  goto memerr;
	}
	++queuelen;
	pbuf_chain(seg->p, p);
        dbg_putstr("tcp_enqueue 15\r\n");
      }
      if(queuelen > TCP_SND_QUEUELEN) {
	DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: queue too long %d (%d)\n", queuelen, TCP_SND_QUEUELEN)); 	
	goto memerr;
      }
      
      seg->len = seglen;
      if((flags & TCP_SYN) || (flags & TCP_FIN)) { 
	++seg->len;
      }
      
      /* build TCP header */
      if(pbuf_header(seg->p, TCP_HLEN)) {
	
	DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: no room for TCP header in pbuf.\n"));
	
#ifdef TCP_STATS
	++stats.tcp.err;
#endif /* TCP_STATS */
	goto memerr;
      }
      dbg_putstr("tcp_enqueue 16\r\n");
      /* ######## */
      //seg->tcphdr = seg->p->payload;
      { struct pbuf *tmp; tmp=seg->p; seg->tcphdr = tmp->payload; }
      { struct tcp_hdr *tmp=seg->tcphdr;
      //seg->tcphdr->src = htons(pcb->local_port);
      //seg->tcphdr->dest = htons(pcb->dest_port);
      //seg->tcphdr->seqno = htonl(seqno);
      //seg->tcphdr->urgp = 0;
      tmp->src = htons(pcb->local_port);
      tmp->dest = htons(pcb->dest_port);
      tmp->seqno = htonl(seqno);
      tmp->urgp = 0;
      }
      TCPH_FLAGS_SET(seg->tcphdr, flags);
      /* don't fill in tcphdr->ackno and tcphdr->wnd until later */
      
      dbg_putstr("tcp_enqueue 17\r\n");
      if(optdata == NULL) {
	TCPH_OFFSET_SET(seg->tcphdr, 5 << 4);
      } else {
	TCPH_OFFSET_SET(seg->tcphdr, (5 + optlen / 4) << 4);
	/* Copy options into data portion of segment.
	   Options can thus only be sent in non data carrying
	   segments such as SYN|ACK. */
	bcopy(optdata, seg->dataptr, optlen);
      }
      DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_enqueue: queueing %lu:%lu (0x%x)\n",
				ntohl(seg->tcphdr->seqno),
				ntohl(seg->tcphdr->seqno) + seg->len,
				flags));

      left -= seglen;
      seqno += seglen;
      ptr = (void *)((char *)ptr + seglen);
      dbg_putstr("tcp_enqueue 18\r\n");
    }

    dbg_putstr("tcp_enqueue 19\r\n");
    
    /* Go to the last segment on the ->unsent queue. */    
    if(pcb->unsent == NULL) {
      useg = NULL;
    } else {
      for(useg = pcb->unsent; useg->next != NULL; useg = useg->next);
    }
    
    dbg_putstr("tcp_enqueue 20\r\n");
    /* If there is room in the last pbuf on the unsent queue,
       chain the first pbuf on the queue together with that. */
      /* ######## */
    {
    struct tcp_hdr *tmp_tcphdr;
    tmp_tcphdr=useg->tcphdr;
    if(useg != NULL &&
       useg->len != 0 &&
       !(TCPH_FLAGS(tmp_tcphdr) & (TCP_SYN | TCP_FIN)) &&
       !(flags & (TCP_SYN | TCP_FIN)) &&
       useg->len + queue->len <= pcb->mss) {
      /* Remove TCP header from first segment. */
      pbuf_header(queue->p, -TCP_HLEN);
      pbuf_chain(useg->p, queue->p);
      useg->len += queue->len;
      useg->next = queue->next;
      
      DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_output: chaining, new len %u\n", useg->len));
      if(seg == queue) {
	seg = NULL;
      }
      memp_free(MEMP_TCP_SEG, queue);
    } else {      
      if(useg == NULL) {
	pcb->unsent = queue;
      } else {
	useg->next = queue;
      }
    }
    }
    if((flags & TCP_SYN) || (flags & TCP_FIN)) {
      ++len;
    }
    pcb->snd_lbb += len;
    pcb->snd_buf -= len;
    pcb->snd_queuelen = queuelen;
    dbg_putstr("tcp_enqueue 21\r\n");

#ifdef LWIP_DEBUG
    if(pcb->snd_queuelen != 0) {
      ASSERT("tcp_enqueue: valid queue length", pcb->unacked != NULL ||
	     pcb->unsent != NULL);
      
    }
#endif /* LWIP_DEBUG */
    
    /* Set the PSH flag in the last segment that we enqueued, but only
       if the segment has data (indicated by seglen > 0). */
    if(seg != NULL && seglen > 0 && seg->tcphdr != NULL) {
      /* ######## */
      struct tcp_hdr *tmp_tcphdr=seg->tcphdr;
      TCPH_FLAGS_SET(seg->tcphdr, TCPH_FLAGS(tmp_tcphdr) | TCP_PSH);
    }
  }
  dbg_putstr("tcp_enqueue 22\r\n");
  return ERR_OK;
 memerr:
  dbg_putstr("tcp_enqueue 23\r\n");
#ifdef TCP_STATS
  ++stats.tcp.memerr;
#endif /* TCP_STATS */

  if(queue != NULL) {
    tcp_segs_free(queue);
  }
#ifdef LWIP_DEBUG
    if(pcb->snd_queuelen != 0) {
      ASSERT("tcp_enqueue: valid queue length", pcb->unacked != NULL ||
	     pcb->unsent != NULL);
      
    }
#endif /* LWIP_DEBUG */
      
  return ERR_MEM;
}
/*-----------------------------------------------------------------------------------*/
/* find out what we can send and send it */
err_t
tcp_output(struct tcp_pcb *pcb)
{
  struct tcp_seg *seg, *useg;
  u32_t wnd;
#if TCP_CWND_DEBUG
  int i = 0;
#endif /* TCP_CWND_DEBUG */
    
  dbg_putstr("tcp_output 0\r\n");
  wnd = MIN(pcb->snd_wnd, pcb->cwnd);
  
  seg = pcb->unsent;
  
#if TCP_OUTPUT_DEBUG
  if(seg == NULL) {
    DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_output: nothing to send\n"));
  }
#endif /* TCP_OUTPUT_DEBUG */
#if TCP_CWND_DEBUG
  if(seg == NULL) {
    DEBUGF(TCP_CWND_DEBUG, ("tcp_output: snd_wnd %lu, cwnd %lu, wnd %lu, seg == NULL, ack %lu\n",
                            pcb->snd_wnd, pcb->cwnd, wnd,
                            pcb->lastack));
  } else {
    DEBUGF(TCP_CWND_DEBUG, ("tcp_output: snd_wnd %lu, cwnd %lu, wnd %lu, effwnd %lu, seq %lu, ack %lu\n",
                            pcb->snd_wnd, pcb->cwnd, wnd,
                            ntohl(seg->tcphdr->seqno) - pcb->lastack + seg->len,
                            ntohl(seg->tcphdr->seqno), pcb->lastack));
  }
#endif /* TCP_CWND_DEBUG */

      /* ######## */
  {
  struct tcp_hdr *tmp_tcphdr;
  dbg_putstr("tcp_output 1\r\n");
  while(seg != NULL &&
	ntohl((tmp_tcphdr=seg->tcphdr,tmp_tcphdr->seqno)) - pcb->lastack + seg->len <= wnd) {
    dbg_putstr("tcp_output 2\r\n");
    pcb->rtime = 0;
#if TCP_CWND_DEBUG
    DEBUGF(TCP_CWND_DEBUG, ("tcp_output: snd_wnd %lu, cwnd %lu, wnd %lu, effwnd %lu, seq %lu, ack %lu, i%d\n",
                            pcb->snd_wnd, pcb->cwnd, wnd,
                            ntohl(seg->tcphdr->seqno) + seg->len -
                            pcb->lastack,
                            ntohl(seg->tcphdr->seqno), pcb->lastack, i));
    ++i;
#endif /* TCP_CWND_DEBUG */

    pcb->unsent = seg->next;
    
    
    if(pcb->state != SYN_SENT) {
      /* ######## */
      struct tcp_hdr *tmp_tcphdr=seg->tcphdr;
      dbg_putstr("tcp_output 3\r\n");
      TCPH_FLAGS_SET(seg->tcphdr, TCPH_FLAGS(tmp_tcphdr) | TCP_ACK);
      if(pcb->flags & TF_ACK_NEXT) {
	pcb->flags &= ~TF_ACK_NEXT;
      }
    }
    
    tcp_output_segment(seg, pcb);
      /* ######## */
    dbg_putstr("tcp_output 4\r\n");
    tmp_tcphdr=seg->tcphdr;
    pcb->snd_nxt = ntohl(tmp_tcphdr->seqno) + seg->len;
    if(TCP_SEQ_LT(pcb->snd_max, pcb->snd_nxt)) {
      pcb->snd_max = pcb->snd_nxt;
    }
    dbg_putstr("tcp_output 4\r\n");
    /* put segment on unacknowledged list if length > 0 */
    if(seg->len > 0) {
      seg->next = NULL;
      if(pcb->unacked == NULL) {
        pcb->unacked = seg;
      } else {
        for(useg = pcb->unacked; useg->next != NULL; useg = useg->next);
        useg->next = seg;
      }
      /*      seg->rtime = 0;*/      
    } else {
      tcp_seg_free(seg);
    }
    seg = pcb->unsent;
  }  
  dbg_putstr("tcp_output 5\r\n");
  }
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static void
tcp_output_segment(struct tcp_seg *seg, struct tcp_pcb *pcb)
{
  u16_t len, tot_len;
  struct netif *netif;

  dbg_putstr("tcp_output_segment 0\r\n");
  /* The TCP header has already been constructed, but the ackno and
   wnd fields remain. */
      /* ######## */
  { struct tcp_hdr *tmp_tcphdr=seg->tcphdr; tmp_tcphdr->ackno = htonl(pcb->rcv_nxt); }

  dbg_putstr("tcp_output_segment 1\r\n");
  /* silly window avoidance */
  if(pcb->rcv_wnd < pcb->mss) {
    dbg_putstr("tcp_output_segment 2\r\n");
      /* ######## */
    { struct tcp_hdr *tmp_tcphdr=seg->tcphdr; tmp_tcphdr->wnd = 0; }
  } else {
    dbg_putstr("tcp_output_segment 3\r\n");
      /* ######## */
    { struct tcp_hdr *tmp_tcphdr=seg->tcphdr; tmp_tcphdr->wnd= htons(pcb->rcv_wnd); }
  }

  /* If we don't have a local IP address, we get one by
     calling ip_route(). */
  dbg_putstr("tcp_output_segment 3\r\n");
  if(ip_addr_isany(&(pcb->local_ip))) {
    netif = ip_route(&(pcb->dest_ip));
    if(netif == NULL) {
      return;
    }
    ip_addr_set(&(pcb->local_ip), &(netif->ip_addr));
  }
  dbg_putstr("tcp_output_segment 4\r\n");

  pcb->rtime = 0;
  
  if(pcb->rttest == 0) {
    dbg_putstr("tcp_output_segment 5\r\n");
    pcb->rttest = tcp_ticks;
      /* ######## */
    { struct tcp_hdr *tmp_tcphdr=seg->tcphdr; pcb->rtseq = ntohl(tmp_tcphdr->seqno); }

    DEBUGF(TCP_RTO_DEBUG, ("tcp_output_segment: rtseq %lu\n", pcb->rtseq));
  }
  DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_output_segment: %lu:%lu\n",
			    htonl(seg->tcphdr->seqno), htonl(seg->tcphdr->seqno) +
			    seg->len));

      /* ######## */
  {
  struct tcp_hdr *tmp_tcphdr;
  struct pbuf *tmp_pbuf;
  dbg_putstr("tcp_output_segment 6\r\n");
  tmp_tcphdr=seg->tcphdr;
  tmp_pbuf=seg->p;
  tmp_tcphdr->chksum = 0;
  tmp_tcphdr->chksum = inet_chksum_pseudo(seg->p,
					   &(pcb->local_ip),
					   &(pcb->dest_ip),
					   IP_PROTO_TCP, tmp_pbuf->tot_len);
  dbg_putstr("tcp_output_segment 7\r\n");
#ifdef TCP_STATS
  ++stats.tcp.xmit;
#endif /* TCP_STATS */

      /* ######## */
  tmp_pbuf=seg->p;
  len = tmp_pbuf->len;
  tot_len = tmp_pbuf->tot_len;
  dbg_putstr("tcp_output_segment 8\r\n");
  ip_output(seg->p, &(pcb->local_ip), &(pcb->dest_ip), TCP_TTL,
	    IP_PROTO_TCP);
      /* ######## */
  dbg_putstr("tcp_output_segment 9\r\n");
  tmp_pbuf=seg->p;
  tmp_pbuf->len = len;
  tmp_pbuf->tot_len = tot_len;
  tmp_pbuf->payload = seg->tcphdr;
  }

  dbg_putstr("tcp_output_segment 10\r\n");
}
/*-----------------------------------------------------------------------------------*/
void
tcp_rexmit_seg(struct tcp_pcb *pcb, struct tcp_seg *seg)
{
  u32_t wnd;
  u16_t len, tot_len;
  struct netif *netif;
      /* ######## */
  struct tcp_hdr *tmp_tcphdr;
  struct pbuf *tmp_pbuf;
  
  DEBUGF(TCP_REXMIT_DEBUG, ("tcp_rexmit_seg: skickar %ld:%ld\n",
			    ntohl(seg->tcphdr->seqno),
			    ntohl(seg->tcphdr->seqno) + seg->len));

  wnd = MIN(pcb->snd_wnd, pcb->cwnd);
  
      /* ######## */
  tmp_tcphdr=seg->tcphdr;
  if(ntohl(tmp_tcphdr->seqno) - pcb->lastack + seg->len <= wnd) {

    /* Count the number of retranmissions. */
    ++pcb->nrtx;
    
    if((netif = ip_route((struct ip_addr *)&(pcb->dest_ip))) == NULL) {
      DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_rexmit_segment: No route to 0x%lx\n", pcb->dest_ip.addr));
#ifdef TCP_STATS
      ++stats.tcp.rterr;
#endif /* TCP_STATS */
      return;
    }

    tmp_tcphdr=seg->tcphdr;
    tmp_tcphdr->ackno = htonl(pcb->rcv_nxt);
    tmp_tcphdr->wnd = htons(pcb->rcv_wnd);

    /* Recalculate checksum. */
    tmp_pbuf=seg->p;
    tmp_tcphdr->chksum = 0;
    tmp_tcphdr->chksum = inet_chksum_pseudo(seg->p,
                                             &(pcb->local_ip),
                                             &(pcb->dest_ip),
                                             IP_PROTO_TCP, tmp_pbuf->tot_len);
    
    tmp_pbuf=seg->p;
    len = tmp_pbuf->len;
    tot_len = tmp_pbuf->tot_len;
    ip_output_if(seg->p, NULL, IP_HDRINCL, TCP_TTL, IP_PROTO_TCP, netif);
    tmp_pbuf=seg->p;
    tmp_pbuf->len = len;
    tmp_pbuf->tot_len = tot_len;
    tmp_pbuf->payload = seg->tcphdr;

#ifdef TCP_STATS
    ++stats.tcp.xmit;
    ++stats.tcp.rexmit;
#endif /* TCP_STATS */

    pcb->rtime = 0;
    
    /* Don't take any rtt measurements after retransmitting. */    
    pcb->rttest = 0;
  } else {
    DEBUGF(TCP_REXMIT_DEBUG, ("tcp_rexmit_seg: no room in window %lu to send %lu (ack %lu)\n",
                              wnd, ntohl(seg->tcphdr->seqno), pcb->lastack));
  }
}
/*-----------------------------------------------------------------------------------*/
void
tcp_rst(u32_t seqno, u32_t ackno,
	struct ip_addr *local_ip, struct ip_addr *dest_ip,
	u16_t local_port, u16_t dest_port)
{
  struct pbuf *p;
  struct tcp_hdr *tcphdr;
  dbg_putstr("tcp_rst 0 0x");
  p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_RAM);
  dbg_puthex((unsigned long)p);
  dbg_putstr(" 0x");
  dbg_puthex((unsigned long)(p->payload));
  dbg_putstr("\r\n");
  if(p == NULL) {
    dbg_putstr("tcp_rst 1\r\n");
#if MEM_RECLAIM
    mem_reclaim(sizeof(struct pbuf));
    p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_RAM);
#endif /* MEM_RECLAIM */    
    dbg_putstr("tcp_rst 2\r\n");
    if(p == NULL) {
      dbg_putstr("tcp_rst 3\r\n");
      DEBUGF(TCP_DEBUG, ("tcp_rst: could not allocate memory for pbuf\n"));
      return;
    }
  }
  dbg_putstr("tcp_rst 4");
  dbg_putstr(" 0x");
  dbg_puthex((unsigned long)(p->payload));
  dbg_putstr("\r\n");
  if(pbuf_header(p, TCP_HLEN)) {
  dbg_putstr("tcp_rst 5\r\n");
    DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_send_data: no room for TCP header in pbuf.\n"));

#ifdef TCP_STATS
    ++stats.tcp.err;
#endif /* TCP_STATS */
    return;
  }

  dbg_putstr("tcp_rst 6");
  tcphdr = p->payload;
  dbg_putstr(" 0x");
  dbg_puthex((unsigned long)(p->payload));
  dbg_putstr("\r\n");
  dbg_putstr("tcp_rst 7\r\n");
  tcphdr->src = htons(local_port);
  dbg_putstr("tcp_rst 8\r\n");
  tcphdr->dest = htons(dest_port);
  dbg_putstr("tcp_rst 9\r\n");
  tcphdr->seqno = htonl(seqno);
  dbg_putstr("tcp_rst 10\r\n");
  tcphdr->ackno = htonl(ackno);
  dbg_putstr("tcp_rst 11 0x");
  dbg_puthex((unsigned long)(p->payload));
  dbg_putstr("\r\n");
  TCPH_FLAGS_SET(tcphdr, TCP_RST | TCP_ACK);
  dbg_putstr("tcp_rst 12\r\n");
  tcphdr->wnd = 0;
  dbg_putstr("tcp_rst 13\r\n");
  tcphdr->urgp = 0;
  dbg_putstr("tcp_rst 14\r\n");
  TCPH_OFFSET_SET(tcphdr, 5 << 4);
  dbg_putstr("tcp_rst 15 0x");
  dbg_puthex((unsigned long)(p->payload));
  dbg_putstr("\r\n");
  
  tcphdr->chksum = 0;
  dbg_putstr("tcp_rst 16\r\n");
  tcphdr->chksum = inet_chksum_pseudo(p, local_ip, dest_ip,
				      IP_PROTO_TCP, p->tot_len);
  dbg_putstr("tcp_rst 17\r\n");

  dbg_putstr("tcp_rst 18\r\n");
#ifdef TCP_STATS
  ++stats.tcp.xmit;
#endif /* TCP_STATS */
  ip_output(p, local_ip, dest_ip, TCP_TTL, IP_PROTO_TCP);
  dbg_putstr("tcp_rst 19\r\n");
  pbuf_free(p);
  DEBUGF(TCP_RST_DEBUG, ("tcp_rst: seqno %lu ackno %lu.\n", seqno, ackno));
  dbg_putstr("tcp_rst 20\r\n");
}
/*-----------------------------------------------------------------------------------*/











