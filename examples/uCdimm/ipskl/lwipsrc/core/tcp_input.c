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
 * $Id: tcp_input.c,v 1.1 2001/12/12 10:00:58 adam Exp $
 */

/*-----------------------------------------------------------------------------------*/
/* tcp_input.c
 *
 * The input processing functions of TCP.
 *
 * These functions are generally called in the order (ip_input() ->) tcp_input() ->
 * tcp_process() -> tcp_receive() (-> application).
 *
 */
/*-----------------------------------------------------------------------------------*/

#include "lwip/debug.h"

#include "lwip/def.h"
#include "lwip/opt.h"

#include "lwip/netif.h"
#include "lwip/mem.h"
#include "lwip/memp.h"

#include "lwip/inet.h"
#include "lwip/tcp.h"

#include "lwip/stats.h"

#include "arch/perf.h"

/* Forward declarations. */
static err_t tcp_process(struct tcp_seg *seg, struct tcp_pcb *pcb);
static void tcp_receive(struct tcp_seg *seg, struct tcp_pcb *pcb);

/*-----------------------------------------------------------------------------------*/
/* tcp_input:
 *
 * The initial input processing of TCP. It verifies the TCP header, demultiplexes
 * the segment between the PCBs and passes it on to tcp_process(), which implements
 * the TCP finite state machine. This function is called by the IP layer (in
 * ip_input()).
 */
/*-----------------------------------------------------------------------------------*/
void
tcp_input(struct pbuf *p, struct netif *inp)
{
  struct tcp_hdr *tcphdr;
  struct tcp_pcb *pcb, *prev;
  struct ip_hdr *iphdr;
  struct tcp_seg *seg;
  u8_t offset;
  err_t err;


  PERF_START;
  
  dbg_putstr("tcp_input 0\r\n");
#ifdef TCP_STATS
  ++stats.tcp.recv;
#endif /* TCP_STATS */
  
  tcphdr = p->payload;
  iphdr = (struct ip_hdr *)((u8_t *)p->payload - IP_HLEN/sizeof(u8_t));

  /* Don't even process incoming broadcasts/multicasts. */
  if(ip_addr_isbroadcast(&(iphdr->dest), &(inp->netmask)) ||
     ip_addr_ismulticast(&(iphdr->dest))) {
    pbuf_free(p);
    dbg_putstr("tcp_input 1\r\n");
    return;
  }

  
  dbg_putstr("tcp_input 2\r\n");
  /* Verify TCP checksum. */
  if(inet_chksum_pseudo(p, (struct ip_addr *)&(iphdr->src),
			(struct ip_addr *)&(iphdr->dest),
			IP_PROTO_TCP, p->tot_len) != 0) {
    DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: packet discarded due to failing checksum 0x%04x\n", inet_chksum_pseudo(p, (struct ip_addr *)&(iphdr->src),
			(struct ip_addr *)&(iphdr->dest),
			IP_PROTO_TCP, p->tot_len)));
#if TCP_DEBUG
    tcp_debug_print(tcphdr);
#endif /* TCP_DEBUG */
#ifdef TCP_STATS
    ++stats.tcp.chkerr;
    ++stats.tcp.drop;
#endif /* TCP_STATS */

    dbg_putstr("tcp_input 3\r\n");
    pbuf_free(p);
    return;
  }

  dbg_putstr("tcp_input 4\r\n");
  /* Move the payload pointer in the pbuf so that it points to the
     TCP data instead of the TCP header. */
  offset = TCPH_OFFSET(tcphdr) >> 4;

  pbuf_header(p, -(offset * 4));

  /* Convert fields in TCP header to host byte order. */
  tcphdr->src = ntohs(tcphdr->src);
  tcphdr->dest = ntohs(tcphdr->dest);
  tcphdr->seqno = ntohl(tcphdr->seqno);
  tcphdr->ackno = ntohl(tcphdr->ackno);
  tcphdr->wnd = ntohs(tcphdr->wnd);
  
  dbg_putstr("tcp_input 5\r\n");
  /* Demultiplex an incoming segment. First, we check if it is destined
     for an active connection. */
  prev = NULL;  
  for(pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
    ASSERT("tcp_input: active pcb->state != CLOSED", pcb->state != CLOSED);
    ASSERT("tcp_input: active pcb->state != TIME-WAIT", pcb->state != TIME_WAIT);
    ASSERT("tcp_input: active pcb->state != LISTEN", pcb->state != LISTEN);
    if(pcb->dest_port == tcphdr->src &&
       pcb->local_port == tcphdr->dest &&
       ip_addr_cmp(&(pcb->dest_ip), &(iphdr->src)) &&
       ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest))) {

      /* Move this PCB to the front of the list so that subsequent
	 lookups will be faster (we exploit locality in TCP segment
	 arrivals). */
      ASSERT("tcp_input: pcb->next != pcb (before cache)", pcb->next != pcb);
      if(prev != NULL) {
        prev->next = pcb->next;
        pcb->next = tcp_active_pcbs;
        tcp_active_pcbs = pcb; 
      }
      ASSERT("tcp_input: pcb->next != pcb (after cache)", pcb->next != pcb);
      break;
    }
    prev = pcb;
  }

  dbg_putstr("tcp_input 6\r\n");
  /* If it did not go to an active connection, we check the connections
     in the TIME-WAIT state. */
  if(pcb == NULL) {
    dbg_putstr("tcp_input 7\r\n");
    for(pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
      ASSERT("tcp_input: TIME-WAIT pcb->state == TIME-WAIT", pcb->state == TIME_WAIT);
      if(pcb->dest_port == tcphdr->src &&
	 pcb->local_port == tcphdr->dest &&
	 ip_addr_cmp(&(pcb->dest_ip), &(iphdr->src)) &&
         ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest))) {
	/* We don't really care enough to move this PCB to the front
	   of the list since we are not very likely to receive that
	   many segments for connections in TIME-WAIT. */
	break;
      }
      dbg_putstr("tcp_input 8\r\n");
    }  
  
    dbg_putstr("tcp_input 9\r\n");
    /* Finally, if we still did not get a match, we check all PCBs that
     are LISTENing for incomming connections. */
    prev = NULL;  
    if(pcb == NULL) {
      dbg_putstr("tcp_input 10\r\n");
      for(pcb = (struct tcp_pcb *)tcp_listen_pcbs; pcb != NULL; pcb = pcb->next) {
	ASSERT("tcp_input: LISTEN pcb->state == LISTEN", pcb->state == LISTEN);
	if((ip_addr_isany(&(pcb->local_ip)) ||
               ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest))) &&
	   pcb->local_port == tcphdr->dest) {	  
	  /* Move this PCB to the front of the list so that subsequent
	     lookups will be faster (we exploit locality in TCP segment
	     arrivals). */
	  if(prev != NULL) {
	    prev->next = pcb->next;
	    pcb->next = (struct tcp_pcb *)tcp_listen_pcbs;
	    tcp_listen_pcbs = (struct tcp_pcb_listen *)pcb; 
	  }
	  break;
	}
	prev = pcb;
      }
      dbg_putstr("tcp_input 11\r\n");
    }
  }
  
#if TCP_INPUT_DEBUG
  DEBUGF(TCP_INPUT_DEBUG, ("+-+-+-+-+-+-+-+-+-+-+-+-+-+- tcp_input: flags "));
  tcp_debug_print_flags(tcphdr->flags);
  DEBUGF(TCP_INPUT_DEBUG, ("-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"));
#endif /* TCP_INPUT_DEBUG */

  dbg_putstr("tcp_input 12\r\n");
  seg = memp_malloc2(MEMP_TCP_SEG);
  if(seg != NULL && pcb != NULL) {
      
    dbg_putstr("tcp_input 13\r\n");
#if TCP_INPUT_DEBUG
#if TCP_DEBUG
    tcp_debug_print_state(pcb->state);
#endif /* TCP_DEBUG */
#endif /* TCP_INPUT_DEBUG */
    
    /* Set up a tcp_seg structure. */
    seg->next = NULL;
    seg->len = p->tot_len;
    seg->dataptr = p->payload;
    seg->p = p;
    seg->tcphdr = tcphdr;
    
    /* The len field in the tcp_seg structure is the segment length
       in TCP terms. In TCP, the SYN and FIN segments are treated as
       one byte, hence increment the len field. */
    if(TCPH_FLAGS(tcphdr) & TCP_FIN || TCPH_FLAGS(tcphdr) & TCP_SYN) {
      ++seg->len;
    }    

    if(pcb->state != LISTEN && pcb->state != TIME_WAIT) {
      pcb->recv_data = NULL;
    }
    tcp_process(seg, pcb);
    if(pcb->state != LISTEN) {
      if(pcb->flags & TF_RESET) {
	if(pcb->state < FIN_WAIT_1) {	  
	  if(pcb->errf != NULL) {
	    pcb->errf(pcb->callback_arg, ERR_RST);
	  }
	}
	if(pcb->state == TIME_WAIT) {
	  tcp_pcb_remove(&tcp_tw_pcbs, pcb);
	} else {
	  tcp_pcb_remove(&tcp_active_pcbs, pcb);      
	}
	memp_free(MEMP_TCP_PCB, pcb);
      } else if(pcb->flags & TF_CLOSED) {
	tcp_pcb_remove(&tcp_active_pcbs, pcb);
	memp_free(MEMP_TCP_PCB, pcb);
      } else {
	if(pcb->state < TIME_WAIT) {
	  err = ERR_OK;
	  /* If the application has registered a "sent" function to be
	     called when new send buffer space is avaliable, we call it
	     now. */
	  if(pcb->acked > 0 && pcb->sent != NULL) {
	    err = pcb->sent(pcb->callback_arg, pcb, pcb->acked);
	  }
	  if(pcb->recv != NULL) {
	    if(pcb->recv_data != NULL) {
	      err = pcb->recv(pcb->callback_arg, pcb, pcb->recv_data, ERR_OK);
	    }
	    if(pcb->flags & TF_GOT_FIN) {
	      err = pcb->recv(pcb->callback_arg, pcb, NULL, ERR_OK);
	    }
	  } else {
	    err = ERR_OK;
	    pbuf_free(pcb->recv_data);	  
	  }	  
	  if(err == ERR_OK) {
	    tcp_output(pcb);
	  }
	} else if(pcb->state == TIME_WAIT) {
	  pbuf_free(pcb->recv_data);	  
	  tcp_output(pcb);
	}
      }
    }
    
    dbg_putstr("tcp_input 14\r\n");
    tcp_seg_free(seg);
#if TCP_INPUT_DEBUG
#if TCP_DEBUG
    tcp_debug_print_state(pcb->state);
#endif /* TCP_DEBUG */
#endif /* TCP_INPUT_DEBUG */
    
  } else {
    dbg_putstr("tcp_input 15\r\n");
    if(seg == NULL) {
      dbg_putstr("tcp_input 16\r\n");
#ifdef TCP_STATS
      ++stats.tcp.memerr;
      ++stats.tcp.drop;
#endif /* TCP_STATS */      
      pbuf_free(p);
    } else {
      dbg_putstr("tcp_input 17\r\n");
      /* If no matching PCB was found, send a TCP RST (reset) to the
	 sender. */
      DEBUGF(TCP_RST_DEBUG, ("tcp_input: no PCB match found, resetting.\n"));
      if(!(TCPH_FLAGS(tcphdr) & TCP_RST)) {
        dbg_putstr("tcp_input 18\r\n");
#ifdef TCP_STATS
	++stats.tcp.proterr;
	++stats.tcp.drop;
#endif /* TCP_STATS */      
	tcp_rst(tcphdr->ackno, tcphdr->seqno + p->tot_len +
		((TCPH_FLAGS(tcphdr) & TCP_FIN || TCPH_FLAGS(tcphdr) & TCP_SYN)? 1: 0),
		&(iphdr->dest), &(iphdr->src),
		tcphdr->dest, tcphdr->src);
      }
      dbg_putstr("tcp_input 19\r\n");
      memp_free(MEMP_TCP_SEG, seg);
      pbuf_free(p);
    }

  }
  ASSERT("tcp_input: tcp_pcbs_sane()", tcp_pcbs_sane());  
  dbg_putstr("tcp_input 20\r\n");
  PERF_STOP("tcp_input");
}
/*-----------------------------------------------------------------------------------*/
/* tcp_process
 *
 * Implements the TCP state machine. Called by tcp_input. In some
 * states tcp_receive() is called to receive data. The tcp_seg
 * argument will be freed by the caller (tcp_input()) unless the
 * recv_data pointer in the pcb is set.
 */
/*-----------------------------------------------------------------------------------*/
static err_t
tcp_process(struct tcp_seg *seg, struct tcp_pcb *pcb)
{
  struct tcp_pcb *npcb;
  struct ip_hdr *iphdr;
  struct tcp_hdr *tcphdr;
  u32_t seqno, ackno;
  u8_t flags;
  u32_t optdata;
  struct tcp_seg *rseg;
  u8_t acceptable = 0;
  
  dbg_putstr("tcp_process 0\r\n");
  iphdr = (struct ip_hdr *)((u8_t *)seg->tcphdr - IP_HLEN/sizeof(u8_t));
  tcphdr = seg->tcphdr;
  flags = TCPH_FLAGS(tcphdr);
  seqno = tcphdr->seqno;
  ackno = tcphdr->ackno;


  dbg_putstr("tcp_process 1\r\n");
  
  /* Process incoming RST segments. */
  if(flags & TCP_RST) {
    dbg_putstr("tcp_process 2\r\n");
    /* First, determine if the reset is acceptable. */
    if(pcb->state != LISTEN) {
      if(pcb->state == SYN_SENT) {
	if(ackno == pcb->snd_nxt) {
	  acceptable = 1;
	}
      } else {
	if(TCP_SEQ_GEQ(seqno, pcb->rcv_nxt) &&
	   TCP_SEQ_LEQ(seqno, pcb->rcv_nxt + pcb->rcv_wnd)) {
	  acceptable = 1;
	}
      }
    }
    if(acceptable) {
      DEBUGF(TCP_INPUT_DEBUG, ("tcp_process: Connection RESET\n"));
      ASSERT("tcp_input: pcb->state != CLOSED", pcb->state != CLOSED);
      pcb->flags |= TF_RESET;
      pcb->flags &= ~TF_ACK_NEXT;
    } else {
      DEBUGF(TCP_INPUT_DEBUG, ("tcp_process: unacceptable reset seqno %lu rcv_nxt %lu\n",
	     seqno, pcb->rcv_nxt));
      DEBUGF(TCP_DEBUG, ("tcp_process: unacceptable reset seqno %lu rcv_nxt %lu\n",
	     seqno, pcb->rcv_nxt));
    }

    return ERR_RST;
  }

  dbg_putstr("tcp_process 3\r\n");
  /* Update the PCB timer unless we are in the LISTEN state, in
     which case we don't even have memory allocated for the timer,
     much less use it. */
  if(pcb->state != LISTEN) {
    pcb->tmr = tcp_ticks;
  }
  
  dbg_putstr("tcp_process 4\r\n");
  /* Do different things depending on the TCP state. */
  switch(pcb->state) {
  case CLOSED:
    dbg_putstr("tcp_process 5\r\n");
    /* Do nothing in the CLOSED state. In fact, this case should never occur
       since PCBs in the CLOSED state are never found in the list of
       active PCBs. */
    break;
  case LISTEN:
    dbg_putstr("tcp_process 6\r\n");
    /* In the LISTEN state, we check for incoming SYN segments,
       creates a new PCB, and responds with a SYN|ACK. */
    if(flags & TCP_ACK) {
      dbg_putstr("tcp_process 7\r\n");
      /* For incoming segments with the ACK flag set, respond with a
	 RST. */
      DEBUGF(TCP_RST_DEBUG, ("tcp_process: ACK in LISTEN, sending reset\n"));
      tcp_rst(ackno + 1, seqno + seg->len,
	      &(iphdr->dest), &(iphdr->src),
	      tcphdr->dest, tcphdr->src);
    } else if(flags & TCP_SYN) {
      DEBUGF(DEMO_DEBUG, ("TCP connection request %d -> %d.\n", seg->tcphdr->src, seg->tcphdr->dest));
      dbg_putstr("tcp_process 8\r\n");
      npcb = tcp_new();
      /* If a new PCB could not be created (probably due to lack of memory),
	 we don't do anything, but rely on the sender will retransmit the
	 SYN at a time when we have more memory avaliable. */
      if(npcb == NULL) {
#ifdef TCP_STATS
        ++stats.tcp.memerr;
#endif /* TCP_STATS */
	break;
      }
      dbg_putstr("tcp_process 9\r\n");
      /* Set up the new PCB. */
      ip_addr_set(&(npcb->local_ip), &(iphdr->dest));
      npcb->local_port = pcb->local_port;
      ip_addr_set(&(npcb->dest_ip), &(iphdr->src));
      npcb->dest_port = tcphdr->src;
      npcb->state = SYN_RCVD;
      npcb->rcv_nxt = seqno + 1;
      npcb->snd_wnd = tcphdr->wnd;
      npcb->ssthresh = npcb->snd_wnd;
      npcb->snd_wl1 = tcphdr->seqno;
      npcb->accept = pcb->accept;
      npcb->callback_arg = pcb->callback_arg;

      dbg_putstr("tcp_process 10\r\n");
      /* Register the new PCB so that we can begin receiving segments
	 for it. */
      dbg_putstr("tcp_process 100\r\n");
      TCP_REG(&tcp_active_pcbs, npcb);
      dbg_putstr("tcp_process 101\r\n");
      
      /* Build an MSS option. */
      optdata = HTONL((2 << 24) | 
		      (4 << 16) | 
		      ((npcb->mss / 256) << 8) |
		      (npcb->mss & 255));
      /* Send a SYN|ACK together with the MSS option. */
      dbg_putstr("tcp_process 102\r\n");
      tcp_enqueue(npcb, NULL, 0, TCP_SYN | TCP_ACK, 0, (u8_t *)&optdata, 4);
      dbg_putstr("tcp_process 11\r\n");
      return tcp_output(npcb);
    }  
    break;
  case SYN_SENT: {
      /* ######## */
    struct tcp_seg *tmp_unacked=pcb->unacked;
    struct tcp_hdr *tmp_tcphdr=tmp_unacked->tcphdr;
    dbg_putstr("tcp_process 12\r\n");
    DEBUGF(TCP_INPUT_DEBUG, ("SYN-SENT: ackno %lu pcb->snd_nxt %lu unacked %lu\n", ackno,
	   pcb->snd_nxt, ntohl(pcb->unacked->tcphdr->seqno)));
    if(flags & TCP_ACK &&
       flags & TCP_SYN &&
       ackno == ntohl(tmp_tcphdr->seqno) + 1) {
      pcb->rcv_nxt = seqno + 1;
      pcb->rcv_wnd = tcphdr->wnd;
      pcb->state = ESTABLISHED;
      pcb->cwnd = pcb->mss;
      rseg = pcb->unacked;
      pcb->unacked = rseg->next;
      tcp_seg_free(rseg);
      /* Call the user specified function to call when sucessfully
	 connected. */
      if(pcb->connected != NULL) {
	pcb->connected(pcb->callback_arg, pcb, ERR_OK);
      }
      tcp_ack(pcb);
    }    
      /* ######## */
    }    
    break;
  case SYN_RCVD:
    dbg_putstr("tcp_process 13\r\n");
    if(flags & TCP_ACK &&
       !(flags & TCP_RST)) {
      if(TCP_SEQ_LT(pcb->lastack, ackno) &&
	 TCP_SEQ_LEQ(ackno, pcb->snd_nxt)) {
        pcb->state = ESTABLISHED;
        DEBUGF(DEMO_DEBUG, ("TCP connection established %d -> %d.\n", seg->tcphdr->src, seg->tcphdr->dest));
	/* Call the accept function. */
        if(pcb->accept != NULL) {
          if(pcb->accept(pcb->callback_arg, pcb, ERR_OK) != ERR_OK) {
	    /* If the accept function returns with an error, we abort
               the connection. */
	    tcp_abort(pcb);
	    break;
	  }
        } else {
	  /* If a PCB does not have an accept function (i.e., no
             application is connected to it), the connection would
             linger in memory until the connection reset by the remote
             peer (which might never happen). Therefore, we abort the
             connection before it is too late. */
	  tcp_abort(pcb);
	  break;
	}
	/* If there was any data contained within this ACK,
	   we'd better pass it on to the application as well. */
	tcp_receive(seg, pcb);	
        pcb->cwnd = pcb->mss;
      }	
    }  
    break;
  case CLOSE_WAIT:
  case ESTABLISHED:
    dbg_putstr("tcp_process 14\r\n");
    tcp_receive(seg, pcb);	  
    if(flags & TCP_FIN) {
      tcp_ack_now(pcb);
      pcb->state = CLOSE_WAIT;
    }
    break;
  case FIN_WAIT_1:
    dbg_putstr("tcp_process 15\r\n");
    tcp_receive(seg, pcb);
    if(flags & TCP_FIN) {
      if(flags & TCP_ACK && ackno == pcb->snd_nxt) {
        DEBUGF(DEMO_DEBUG,
	       ("TCP connection closed %d -> %d.\n", seg->tcphdr->src, seg->tcphdr->dest));
	tcp_ack_now(pcb);
	tcp_pcb_purge(pcb);
	TCP_RMV(&tcp_active_pcbs, pcb);
	pcb->state = TIME_WAIT;
	/*        pcb = memp_realloc(MEMP_TCP_PCB, MEMP_TCP_PCB_TW, pcb);*/
	TCP_REG(&tcp_tw_pcbs, pcb);
      } else {
	tcp_ack_now(pcb);
	pcb->state = CLOSING;
      }
    } else if(flags & TCP_ACK && ackno == pcb->snd_nxt) {
      pcb->state = FIN_WAIT_2;
    }
    break;
  case FIN_WAIT_2:
    dbg_putstr("tcp_process 16\r\n");
    tcp_receive(seg, pcb);
    if(flags & TCP_FIN) {
      DEBUGF(DEMO_DEBUG, ("TCP connection closed %d -> %d.\n", seg->tcphdr->src, seg->tcphdr->dest));
      tcp_ack_now(pcb);
      tcp_pcb_purge(pcb);
      TCP_RMV(&tcp_active_pcbs, pcb);
      /*      pcb = memp_realloc(MEMP_TCP_PCB, MEMP_TCP_PCB_TW, pcb);      */
      pcb->state = TIME_WAIT;
      TCP_REG(&tcp_tw_pcbs, pcb);
    }
    break;
  case CLOSING:
    dbg_putstr("tcp_process 17\r\n");
    tcp_receive(seg, pcb);
    if(flags & TCP_ACK && ackno == pcb->snd_nxt) {
      DEBUGF(DEMO_DEBUG, ("TCP connection closed %d -> %d.\n", seg->tcphdr->src, seg->tcphdr->dest));
      tcp_ack_now(pcb);
      tcp_pcb_purge(pcb);
      TCP_RMV(&tcp_active_pcbs, pcb);
      /*      pcb = memp_realloc(MEMP_TCP_PCB, MEMP_TCP_PCB_TW, pcb);      */
      pcb->state = TIME_WAIT;
      TCP_REG(&tcp_tw_pcbs, pcb);
    }
    break;
  case LAST_ACK:
    dbg_putstr("tcp_process 18\r\n");
    tcp_receive(seg, pcb);
    if(flags & TCP_ACK && ackno == pcb->snd_nxt) {
      DEBUGF(DEMO_DEBUG, ("TCP connection closed %d -> %d.\n", seg->tcphdr->src, seg->tcphdr->dest));
      pcb->state = CLOSED;
      pcb->flags |= TF_CLOSED;
    }
    break;
  case TIME_WAIT:
    dbg_putstr("tcp_process 19\r\n");
    /*    DEBUGF(TCP_RST_DEBUG, ("tcp_process: in TIME_WAIT, sending RST\n"));
	  tcp_rst(ackno + 1, seqno + seg->len,
	  &(iphdr->dest), &(iphdr->src),
	  tcphdr->dest, tcphdr->src);*/
    if(TCP_SEQ_GT(seqno + seg->len, pcb->rcv_nxt)) {
      pcb->rcv_nxt = seqno + seg->len;
    }
    tcp_ack_now(pcb);
    break;
  }
  
  dbg_putstr("tcp_process 20\r\n");
  return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
/* tcp_receive:
 *
 * Called by tcp_process. Checks if the given segment is an ACK for outstanding
 * data, and if so frees the memory of the buffered data. Next, is places the
 * segment on any of the receive queues (pcb->recved or pcb->ooseq). If the segment
 * is buffered, the pbuf is referenced by pbuf_ref so that it will not be freed until
 * i it has been removed from the buffer.
 *
 * If the incoming segment constitutes an ACK for a segment that was used for RTT
 * estimation, the RTT is estimated here as well.
 */
/*-----------------------------------------------------------------------------------*/
static void
tcp_receive(struct tcp_seg *seg, struct tcp_pcb *pcb)
{
  struct tcp_seg *next, *prev, *cseg;
  struct pbuf *p;
  u32_t ackno, seqno;
  s32_t off;
  int m;
      /* ######## */
  struct tcp_hdr *tmp_tcphdr;
  dbg_putstr("tcp_receive 0\r\n");
      /* ######## */
  tmp_tcphdr=seg->tcphdr;
  ackno = tmp_tcphdr->ackno;
  seqno = tmp_tcphdr->seqno;

  dbg_putstr("tcp_receive 1\r\n");
  if(TCPH_FLAGS(tmp_tcphdr) & TCP_ACK) {
    /* Update window. */
      /* ######## */
    tmp_tcphdr=seg->tcphdr;
    if(TCP_SEQ_LT(pcb->snd_wl1, seqno) ||
       (pcb->snd_wl1 == seqno && TCP_SEQ_LT(pcb->snd_wl2, ackno)) ||
       (pcb->snd_wl2 == ackno && tmp_tcphdr->wnd > pcb->snd_wnd)) {
      /* ######## */
      pcb->snd_wnd = tmp_tcphdr->wnd;
      pcb->snd_wl1 = seqno;
      pcb->snd_wl2 = ackno;
      DEBUGF(TCP_WND_DEBUG, ("tcp_receive: window update %lu\n", pcb->snd_wnd));
#if TCP_WND_DEBUG
    } else {
      /* ######## */
      if(pcb->snd_wnd != tmp_tcphdr->wnd) {
        DEBUGF(TCP_WND_DEBUG, ("tcp_receive: no window update lastack %lu snd_max %lu ackno %lu wl1 %lu seqno %lu wl2 %lu\n",
                               pcb->lastack, pcb->snd_max, ackno, pcb->snd_wl1, seqno, pcb->snd_wl2));
      }
#endif /* TCP_WND_DEBUG */
    }
    

    dbg_putstr("tcp_receive 2\r\n");
    if(pcb->lastack == ackno) {
      dbg_putstr("tcp_receive 3\r\n");
      ++pcb->dupacks;
      if(pcb->dupacks >= 3 && pcb->unacked != NULL) {
        if(!(pcb->flags & TF_INFR)) {
          /* This is fast retransmit. Retransmit the first unacked segment. */
          DEBUGF(TCP_FR_DEBUG, ("tcp_receive: dupacks %d (%lu), fast retransmit %lu\n",
                                pcb->dupacks, pcb->lastack,
                                ntohl(pcb->unacked->tcphdr->seqno)));
          tcp_rexmit_seg(pcb, pcb->unacked);
          /* Set ssthresh to max (FlightSize / 2, 2*SMSS) */
          pcb->ssthresh = UMAX((pcb->snd_max -
                                pcb->lastack) / 2,
                               2 * pcb->mss);

          pcb->cwnd = pcb->ssthresh + 3 * pcb->mss;
          pcb->flags |= TF_INFR;          
        } else {         
          pcb->cwnd += pcb->mss;
        }
      }
    } else if(TCP_SEQ_LT(pcb->lastack, ackno) &&
              TCP_SEQ_LEQ(ackno, pcb->snd_max)) {
      dbg_putstr("tcp_receive 4\r\n");
      /* We come here when the ACK acknowledges new data. */

      /* Reset the "IN Fast Retransmit" flag, since we are no longer
         in fast retransmit. Also reset the congestion window to the
         slow start threshold. */
      if(pcb->flags & TF_INFR) {
	pcb->flags &= ~TF_INFR;
	pcb->cwnd = pcb->ssthresh;
      }

      /* Reset the number of retransmissions. */
      pcb->nrtx = 0;
      /* Reset the retransmission time-out. */
      pcb->rto = (pcb->sa >> 3) + pcb->sv;
      
      /* Update the send buffer space. */
      pcb->acked = ackno - pcb->lastack;
      pcb->snd_buf += pcb->acked;

      /* Reset the fast retransmit variables. */
      pcb->dupacks = 0;
      pcb->lastack = ackno;

      
      dbg_putstr("tcp_receive 5\r\n");
      /* Update the congestion control variables (cwnd and
         ssthresh). */
      if(pcb->state >= ESTABLISHED) {
        dbg_putstr("tcp_receive 50\r\n");
        if(pcb->cwnd < pcb->ssthresh) {
          dbg_putstr("tcp_receive 51\r\n");
          pcb->cwnd += pcb->mss;
          DEBUGF(TCP_CWND_DEBUG, ("tcp_receive: slow start cwnd %u\n", pcb->cwnd));
          dbg_putstr("tcp_receive 52\r\n");
        } else {
          u16_t tmp0,tmp1,tmp2;
          dbg_putstr("tcp_receive 53\r\n");
          tmp0=pcb->mss;
          tmp1=tmp0;
          tmp2=tmp0*tmp1;
          tmp1=pcb->cwnd;
          tmp0=tmp2/tmp1;
          tmp0+=tmp1;
          pcb->cwnd=tmp0;
          // pcb->cwnd += pcb->mss * pcb->mss / pcb->cwnd;
          DEBUGF(TCP_CWND_DEBUG, ("tcp_receive: congestion avoidance cwnd %u\n", pcb->cwnd));
          dbg_putstr("tcp_receive 54\r\n");
        }
        dbg_putstr("tcp_receive 55\r\n");
      }
      dbg_putstr("tcp_receive 6\r\n");
      DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: ACK for %lu, unacked->seqno %lu:%lu\n",
                               ackno,
                               pcb->unacked != NULL?
                               ntohl(pcb->unacked->tcphdr->seqno): 0,
                               pcb->unacked != NULL?
                               ntohl(pcb->unacked->tcphdr->seqno) + pcb->unacked->len: 0));

      /* We go through the ->unsent list to see if any of the segments
         on the list are acknowledged by the ACK. This may seem
         strange since an "unsent" segment shouldn't be acked. The
         rationale is that lwIP puts all outstanding segments on the
         ->unsent list after a retransmission, so these segments may
         in fact be sent once. */
      /* ######## */
      {
      struct tcp_seg *tmp_unsent;
      struct tcp_hdr *tmp_tcphdr;
      while(pcb->unsent != NULL && 
	    TCP_SEQ_LEQ(ntohl((tmp_unsent=pcb->unsent,tmp_tcphdr=tmp_unsent->tcphdr,tmp_tcphdr->seqno)) + (tmp_unsent=pcb->unsent,tmp_unsent->len),
                        ackno)) {
	DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: removing %lu:%lu from pcb->unsent\n",
				 ntohl(pcb->unsent->tcphdr->seqno),
				 ntohl(pcb->unsent->tcphdr->seqno) +
				 pcb->unsent->len));

        tmp_unsent=pcb->unsent;
	next = tmp_unsent->next;
	pcb->snd_queuelen -= tcp_seg_free(pcb->unsent);
	pcb->unsent = next;
#ifdef LWIP_DEBUG
	if(pcb->snd_queuelen != 0) {
	  ASSERT("tcp_receive: valid queue length", pcb->unacked != NULL ||
		 pcb->unsent != NULL);      
	}
#endif /* LWIP_DEBUG */
	
        if(pcb->unsent != NULL) {
          tmp_unsent=pcb->unsent;
          tmp_tcphdr=tmp_unsent->tcphdr;
          pcb->snd_nxt = htonl(tmp_tcphdr->seqno);
        }
      }
      /* ######## */
      }
      
      dbg_putstr("tcp_receive 7\r\n");
      /* Remove segment from the unacknowledged list if the incoming
         ACK acknowlegdes them. */
      /* ######## */
      {
      struct tcp_seg *tmp_unacked;
      struct tcp_hdr *tmp_tcphdr;
      while(pcb->unacked != NULL && 
	    TCP_SEQ_LEQ(ntohl((tmp_unacked=pcb->unacked,tmp_tcphdr=tmp_unacked->tcphdr,tmp_tcphdr->seqno)) + (tmp_unacked=pcb->unacked,tmp_unacked->len),
                        ackno)) {
	DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: removing %lu:%lu from pcb->unacked\n",
	       ntohl(pcb->unacked->tcphdr->seqno),
	       ntohl(pcb->unacked->tcphdr->seqno) +
	       pcb->unacked->len));

        tmp_unacked=pcb->unacked;
	next = tmp_unacked->next;
	pcb->snd_queuelen -= tcp_seg_free(pcb->unacked);
	pcb->unacked = next;
#ifdef LWIP_DEBUG
	if(pcb->snd_queuelen != 0) {
	  ASSERT("tcp_receive: valid queue length", pcb->unacked != NULL ||
		 pcb->unsent != NULL);      
	}
#endif /* LWIP_DEBUG */

	
      }
      /* ######## */
      }

      dbg_putstr("tcp_receive 8\r\n");
      pcb->polltmr = 0;
    }
    /* End of ACK for new data processing. */
    
    dbg_putstr("tcp_receive 9\r\n");
    DEBUGF(TCP_RTO_DEBUG, ("tcp_receive: pcb->rttest %d rtseq %lu ackno %lu\n",
	   pcb->rttest, pcb->rtseq, ackno));
    
    /* RTT estimation calculations. This is done by checking if the
       incoming segment acknowledges the segment we use to take a
       round-trip time measurement. */
    if(pcb->rttest && TCP_SEQ_LT(pcb->rtseq, ackno)) {
      dbg_putstr("tcp_receive 10\r\n");
      m = tcp_ticks - pcb->rttest;

      DEBUGF(TCP_RTO_DEBUG, ("tcp_receive: experienced rtt %d ticks (%d msec).\n",
	     m, m * TCP_SLOW_INTERVAL));

      /* This is taken directly from VJs original code in his paper */      
      m = m - (pcb->sa >> 3);
      pcb->sa += m;
      if(m < 0) {
	m = -m;
      }
      m = m - (pcb->sv >> 2);
      pcb->sv += m;
      pcb->rto = (pcb->sa >> 3) + pcb->sv;
      
      DEBUGF(TCP_RTO_DEBUG, ("tcp_receive: RTO %d (%d miliseconds)\n",
			     pcb->rto, pcb->rto * TCP_SLOW_INTERVAL));

      pcb->rttest = 0;
      dbg_putstr("tcp_receive 11\r\n");
    } 
  }
  
  dbg_putstr("tcp_receive 12\r\n");
  /* If the incoming segment contains data, we must process it
     further. */
  if(seg->len > 0) {
    dbg_putstr("tcp_receive 13\r\n");
    /* This code basically does three things:

     +) If the incoming segment contains data that is the next
        in-sequence data, this data is passed to the application. This
        might involve trimming the first edge of the data. The rcv_nxt
        variable and the advertised window are adjusted.       

     +) If the incoming segment has data that is above the next
        sequence number expected (->rcv_nxt), the segment is placed on
        the ->ooseq queue. This is done by finding the appropriate
        place in the ->ooseq queue (which is ordered by sequence
        number) and trim the segment in both ends if needed. An
        immediate ACK is sent to indicate that we received an
        out-of-sequence segment.

     +) Finally, we check if the first segment on the ->ooseq queue
        now is in sequence (i.e., if rcv_nxt >= ooseq->seqno). If
        rcv_nxt > ooseq->seqno, we must trim the first edge of the
        segment on ->ooseq before we adjust rcv_nxt. The data in the
        segments that are now on sequence are chained onto the
        incoming segment so that we only need to call the application
        once.
    */

    /* First, we check if we must trim the first edge. We have to do
       this if the sequence number of the incoming segment is less
       than rcv_nxt, and the sequence number plus the length of the
       segment is larger than rcv_nxt. */
    if(TCP_SEQ_LT(seqno, pcb->rcv_nxt) &&
       TCP_SEQ_LT(pcb->rcv_nxt, seqno + seg->len)) {
      /* Trimming the first edge is done by pushing the payload
         pointer in the pbuf downwards. This is somewhat tricky since
         we do not want to discard the full contents of the pbuf up to
         the new starting point of the data since we have to keep the
         TCP header which is present in the first pbuf in the chain.

	 What is done is really quite a nasty hack: the first pbuf in
	 the pbuf chain is pointed to by seg->p. Since we need to be
	 able to deallocate the whole pbuf, we cannot change this
	 seg->p pointer to point to any of the later pbufs in the
	 chain. Instead, we point the ->payload pointer in the first
	 pbuf to data in one of the later pbufs. We also set the
	 seg->data pointer to point to the right place. This way, the
	 ->p pointer will still point to the first pbuf, but the
	 ->p->payload pointer will point to data in another pbuf.
	 
	 After we are done with adjusting the pbuf pointers we must
	 adjust the ->data pointer in the seg and the segment
	 length.*/
      /* ######## */
      struct pbuf *tmp_pbuf;
      struct tcp_hdr *tmp_tcphdr;
      dbg_putstr("tcp_receive 14\r\n");
      off = pcb->rcv_nxt - seqno;
      tmp_pbuf=seg->p;
      if(tmp_pbuf->len < off) {
	p = seg->p;
	while(p->len < off) {
	  off -= p->len;
	  tmp_pbuf->tot_len -= p->len;
	  p->len = 0;
	  p = p->next;
	}
	pbuf_header(p, -off);
	tmp_pbuf->tot_len -= off;
	/*	seg->p->payload = p->payload;*/
      } else {
	pbuf_header(seg->p, -off);
      }
      dbg_putstr("tcp_receive 15\r\n");
      seg->dataptr = tmp_pbuf->payload;
      seg->len -= pcb->rcv_nxt - seqno;      
      /* ######## */
      tmp_tcphdr=seg->tcphdr;
      tmp_tcphdr->seqno = seqno = pcb->rcv_nxt;
      dbg_putstr("tcp_receive 16\r\n");
    }

    dbg_putstr("tcp_receive 17\r\n");
    /* The sequence number must be within the window (above rcv_nxt
       and below rcv_nxt + rcv_wnd) in order to be further
       processed. */
    if(TCP_SEQ_GEQ(seqno, pcb->rcv_nxt) &&
       TCP_SEQ_LT(seqno, pcb->rcv_nxt + pcb->rcv_wnd)) {
      dbg_putstr("tcp_receive 18\r\n");
      if(pcb->rcv_nxt == seqno) {			
        dbg_putstr("tcp_receive 19\r\n");
	/* The incoming segment is the next in sequence. We check if
           we have to trim the end of the segment and update rcv_nxt
           and pass the data to the application. */
#if TCP_QUEUE_OOSEQ
      /* ######## */
        {
        struct tcp_seg *tmp_ooseq;
        struct tcp_hdr *tmp_tcphdr;
	if(pcb->ooseq != NULL &&
	   TCP_SEQ_LEQ((tmp_ooseq=pcb->ooseq,tmp_tcphdr=tmp_ooseq->tcphdr,tmp_tcphdr->seqno), seqno + seg->len)) {
	  /* We have to trim the second edge of the incoming
             segment. */
          tmp_ooseq=pcb->ooseq;
          tmp_tcphdr=tmp_ooseq->tcphdr;
	  seg->len = tmp_tcphdr->seqno - seqno;
	  pbuf_realloc(seg->p, seg->len);
	}
        }
#endif /* TCP_QUEUE_OOSEQ */
	
        dbg_putstr("tcp_receive 20\r\n");
	pcb->rcv_nxt += seg->len;	
	
	/* Update the receiver's (our) window. */
	if(pcb->rcv_wnd < seg->len) {
	  pcb->rcv_wnd = 0;
	} else {
	  pcb->rcv_wnd -= seg->len;
	}
	
	/* If there is data in the segment, we make preparations to
	   pass this up to the application. The ->recv_data variable
	   is used for holding the pbuf that goes to the
	   application. The code for reassembling out-of-sequence data
	   chains its data on this pbuf as well.
	   
	   If the segment was a FIN, we set the TF_GOT_FIN flag that will
	   be used to indicate to the application that the remote side has
	   closed its end of the connection. */      
      /* ######## */
        {
        struct pbuf *tmp_pbuf;
        struct tcp_hdr *tmp_tcphdr;
        tmp_pbuf=seg->p;
	if(tmp_pbuf->tot_len > 0) {
	  pcb->recv_data = seg->p;
	  /* Since this pbuf now is the responsibility of the
	     application, we delete our reference to it so that we won't
	     (mistakingly) deallocate it. */
	  seg->p = NULL;
	}
        tmp_tcphdr=seg->tcphdr;
	if(TCPH_FLAGS(tmp_tcphdr) & TCP_FIN) {
	  DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: received FIN."));
	  pcb->flags |= TF_GOT_FIN;
	}
      /* ######## */
        }
	
        dbg_putstr("tcp_receive 21\r\n");
#if TCP_QUEUE_OOSEQ
      /* ######## */
        {
        struct tcp_seg *tmp_ooseq;
        struct tcp_hdr *tmp_tcphdr;
	/* We now check if we have segments on the ->ooseq queue that
           is now in sequence. */
	while(pcb->ooseq != NULL &&
	      (tmp_ooseq=pcb->ooseq,tmp_tcphdr=tmp_ooseq->tcphdr,tmp_tcphdr->seqno) == pcb->rcv_nxt) {

	  seg = pcb->ooseq;
          tmp_ooseq=pcb->ooseq;
          tmp_tcphdr=tmp_ooseq->tcphdr;
	  seqno = tmp_tcphdr->seqno;
	  
	  pcb->rcv_nxt += seg->len;
	  if(pcb->rcv_wnd < seg->len) {
	    pcb->rcv_wnd = 0;
	  } else {
	    pcb->rcv_wnd -= seg->len;
	  }
      /* ######## */
          {
          struct pbuf *tmp_pbuf;
          struct tcp_hdr *tmp_tcphdr;
          tmp_pbuf=seg->p;
	  if(tmp_pbuf->tot_len > 0) {
	    /* Chain this pbuf onto the pbuf that we will pass to
	       the application. */
	    pbuf_chain(pcb->recv_data, seg->p);
	    seg->p = NULL;
	  }
          tmp_tcphdr=seg->tcphdr;
	  if(TCPH_FLAGS(tmp_tcphdr) & TCP_FIN) {
	    DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: dequeued FIN."));
	    pcb->flags |= TF_GOT_FIN;
	  }	    
          }
	  

	  pcb->ooseq = seg->next;
	  tcp_seg_free(seg);
	}
        }
#endif /* TCP_QUEUE_OOSEQ */

        dbg_putstr("tcp_receive 22\r\n");

	/* Acknowledge the segment(s). */
	tcp_ack(pcb);

        dbg_putstr("tcp_receive 23\r\n");
      } else {
        dbg_putstr("tcp_receive 24\r\n");
	/* We get here if the incoming segment is out-of-sequence. */
	tcp_ack_now(pcb);
#if TCP_QUEUE_OOSEQ
	/* We queue the segment on the ->ooseq queue. */
	if(pcb->ooseq == NULL) {
          dbg_putstr("tcp_receive 25\r\n");
	  pcb->ooseq = tcp_seg_copy(seg);
	} else {
	  /* If the queue is not empty, we walk through the queue and
	  try to find a place where the sequence number of the
	  incoming segment is between the sequence numbers of the
	  previous and the next segment on the ->ooseq queue. That is
	  the place where we put the incoming segment. If needed, we
	  trim the second edges of the previous and the incoming
	  segment so that it will fit into the sequence.

	  If the incoming segment has the same sequence number as a
	  segment on the ->ooseq queue, we discard the segment that
	  contains less data. */

      /* ######## */
          struct tcp_hdr *tmp_tcphdr;
          dbg_putstr("tcp_receive 26\r\n");
	  prev = NULL;
	  for(next = pcb->ooseq; next != NULL; next = next->next) {
            tmp_tcphdr=next->tcphdr;
	    if(seqno == tmp_tcphdr->seqno) {
	      /* The sequence number of the incoming segment is the
                 same as the sequence number of the segment on
                 ->ooseq. We check the lengths to see which one to
                 discard. */
	      if(seg->len > next->len) {
		/* The incoming segment is larger than the old
                   segment. We replace the old segment with the new
                   one. */
		cseg = tcp_seg_copy(seg);
		if(cseg != NULL) {
		  cseg->next = next->next;
		  if(prev != NULL) {
		    prev->next = cseg;
		  } else {
		    pcb->ooseq = cseg;
		  }
		}
		break;
	      } else {
		/* Either the lenghts are the same or the incoming
                   segment was smaller than the old one; in either
                   case, we ditch the incoming segment. */
		break;
	      } 
	    } else {
	      if(prev == NULL) {
      /* ######## */
                tmp_tcphdr=next->tcphdr;
		if(TCP_SEQ_LT(seqno, tmp_tcphdr->seqno)) {
		  /* The sequence number of the incoming segment is lower
		     than the sequence number of the first segment on the
		     queue. We put the incoming segment first on the
		     queue. */
		  
      /* ######## */
                  tmp_tcphdr=next->tcphdr;
		  if(TCP_SEQ_GT(seqno + seg->len, tmp_tcphdr->seqno)) {
		    /* We need to trim the incoming segment. */
      /* ######## */
		    seg->len = tmp_tcphdr->seqno - seqno;
		    pbuf_realloc(seg->p, seg->len);
		  }
		  cseg = tcp_seg_copy(seg);
		  if(cseg != NULL) {
		    cseg->next = next;
		    pcb->ooseq = cseg;
		  }
		  break;
		}
	      } else if(TCP_SEQ_LT((tmp_tcphdr=prev->tcphdr,tmp_tcphdr->seqno), seqno) &&
		 TCP_SEQ_LT(seqno, (tmp_tcphdr=next->tcphdr,tmp_tcphdr->seqno))) {
		/* The sequence number of the incoming segment is in
                   between the sequence numbers of the previous and
                   the next segment on ->ooseq. We trim and insert the
                   incoming segment and trim the previous segment, if
                   needed. */
		if(TCP_SEQ_GT(seqno + seg->len, (tmp_tcphdr=next->tcphdr,tmp_tcphdr->seqno))) {
		  /* We need to trim the incoming segment. */
                  tmp_tcphdr=next->tcphdr;
		  seg->len = tmp_tcphdr->seqno - seqno;
		  pbuf_realloc(seg->p, seg->len);
		}

		cseg = tcp_seg_copy(seg);
		if(cseg != NULL) {		 		  
		  cseg->next = next;
		  prev->next = cseg;
      /* ######## */
                  tmp_tcphdr=prev->tcphdr;
		  if(TCP_SEQ_GT(tmp_tcphdr->seqno + prev->len, seqno)) {
		    /* We need to trim the prev segment. */
		    prev->len = seqno - tmp_tcphdr->seqno;
		    pbuf_realloc(prev->p, prev->len);
		  }
		}
		break;
		}
	      /* If the "next" segment is the last segment on the
                 ooseq queue, we add the incoming segment to the end
                 of the list. */
      /* ######## */
              tmp_tcphdr=next->tcphdr;
	      if(next->next == NULL &&
		 TCP_SEQ_GT(seqno, tmp_tcphdr->seqno)) {
		next->next = tcp_seg_copy(seg);
      /* ######## */
              tmp_tcphdr=next->tcphdr;
		if(next->next != NULL) {		
		  if(TCP_SEQ_GT(tmp_tcphdr->seqno + next->len, seqno)) {
		    /* We need to trim the last segment. */
		    next->len = seqno - tmp_tcphdr->seqno;
		    pbuf_realloc(next->p, next->len);
		  }
		}
		break;
	      }
	    }
	    prev = next;
	  }    
          dbg_putstr("tcp_receive 27\r\n");
	} 
        dbg_putstr("tcp_receive 28\r\n");
#endif /* TCP_QUEUE_OOSEQ */
	     
      }
      dbg_putstr("tcp_receive 29\r\n");
      
    }
    dbg_putstr("tcp_receive 30\r\n");
  }
  dbg_putstr("tcp_receive 31\r\n");
}
/*-----------------------------------------------------------------------------------*/
  
