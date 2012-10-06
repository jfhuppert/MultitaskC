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
 * $Id: tcp.h,v 1.1 2001/12/12 10:01:01 adam Exp $
 */
#ifndef __LWIP_TCP_H__
#define __LWIP_TCP_H__

#include "lwip/sys.h"
#include "lwip/mem.h"

#include "lwip/pbuf.h"
#include "lwip/opt.h"
#include "lwip/ip.h"
#include "lwip/icmp.h"

#include "lwip/sys.h"

#include "lwip/err.h"

struct tcp_pcb;

/* Functions for interfacing with TCP: */

/* Lower layer interface to TCP: */
void             tcp_init    (void);  /* Must be called first to
					 initialize TCP. */
void             tcp_slowtmr (void);  /* Must be called every
					 TCP_SLOW_INTERVAL ms. */
void             tcp_fasttmr (void);  /* Must be called every
					 TCP_FAST_INTERVAL ms. */

/* Application program's interface: */
struct tcp_pcb * tcp_new     (void);

void             tcp_arg     (struct tcp_pcb *pcb, void *arg);
void             tcp_accept  (struct tcp_pcb *pcb,
			      err_t (* accept)(void *arg, struct tcp_pcb *newpcb,
					       err_t err));
void             tcp_recv    (struct tcp_pcb *pcb,
			      err_t (* recv)(void *arg, struct tcp_pcb *tpcb,
				  struct pbuf *p, err_t err));
void             tcp_sent    (struct tcp_pcb *pcb,
			      err_t (* sent)(void *arg, struct tcp_pcb *tpcb,
					     u16_t len));
void             tcp_poll    (struct tcp_pcb *pcb,
			      err_t (* poll)(void *arg, struct tcp_pcb *tpcb),
			      u8_t interval);
void             tcp_err     (struct tcp_pcb *pcb,
			      void (* err)(void *arg, err_t err));

#define          tcp_sndbuf(pcb)   ((pcb)->snd_buf)

void             tcp_recved  (struct tcp_pcb *pcb, u16_t len);
err_t            tcp_bind    (struct tcp_pcb *pcb, struct ip_addr *ipaddr,
			      u16_t port);
err_t            tcp_connect (struct tcp_pcb *pcb, struct ip_addr *ipaddr,
			      u16_t port, err_t (* connected)(void *arg,
							      struct tcp_pcb *tpcb,
							      err_t err));
struct tcp_pcb * tcp_listen  (struct tcp_pcb *pcb);
void             tcp_abort   (struct tcp_pcb *pcb);
err_t            tcp_close   (struct tcp_pcb *pcb);
err_t            tcp_write   (struct tcp_pcb *pcb, const void *dataptr, u16_t len,
			      u8_t copy);

/* Only used by IP to pass a TCP segment to TCP: */
void             tcp_input   (struct pbuf *p, struct netif *inp);
/* Used within the TCP code only: */
err_t            tcp_output  (struct tcp_pcb *pcb);




#define TCP_SEQ_LT(a,b)     ((s32_t)((a)-(b)) < 0)
#define TCP_SEQ_LEQ(a,b)    ((s32_t)((a)-(b)) <= 0)
#define TCP_SEQ_GT(a,b)     ((s32_t)((a)-(b)) > 0)
#define TCP_SEQ_GEQ(a,b)    ((s32_t)((a)-(b)) >= 0)

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

/* Length of the TCP header, excluding options. */
#define TCP_HLEN 20

#define TCP_FAST_INTERVAL      200  /* the fine grained timeout in
				       milliseconds */
#define TCP_SLOW_INTERVAL      500  /* the coarse grained timeout in
				       milliseconds */
#define TCP_FIN_WAIT_TIMEOUT 20000 /* milliseconds */
#define TCP_SYN_RCVD_TIMEOUT 20000 /* milliseconds */

#define TCP_OOSEQ_TIMEOUT        6 /* x RTO */

#define TCP_MSL 60000  /* The maximum segment lifetime in microseconds */

#if 0
struct tcp_hdr {
  u16_t src, dest;
  u32_t seqno, ackno;
  u8_t offset;
  u8_t flags;
  u16_t wnd;
  u16_t chksum;
  u16_t urgp;
};
#endif /* 0 */

struct tcp_hdr {
  u16_t src, dest;
  u32_t seqno, ackno;
  u16_t _offset_flags;
  u16_t wnd;
  u16_t chksum;
  u16_t urgp;
};

#define TCPH_OFFSET(hdr) (NTOHS((hdr)->_offset_flags) >> 8)
#define TCPH_FLAGS(hdr) (NTOHS((hdr)->_offset_flags) & 0xff)

#define TCPH_OFFSET_SET(hdr, offset) { struct tcp_hdr *tmp=hdr; tmp->_offset_flags = HTONS(((offset) << 8) | TCPH_FLAGS(tmp)); }
#define TCPH_FLAGS_SET(hdr, flags) { struct tcp_hdr *tmp=hdr; tmp->_offset_flags = HTONS((TCPH_OFFSET(tmp) << 8) | (flags)); }

enum tcp_state {
  CLOSED      = 0,
  LISTEN      = 1,
  SYN_SENT    = 2,
  SYN_RCVD    = 3,
  ESTABLISHED = 4,
  FIN_WAIT_1  = 5,
  FIN_WAIT_2  = 6,
  CLOSE_WAIT  = 7,
  CLOSING     = 8,
  LAST_ACK    = 9,
  TIME_WAIT   = 10
};


/* the TCP protocol control block */
struct tcp_pcb {
  struct tcp_pcb *next;   /* for the linked list */

  enum tcp_state state;   /* TCP state */

  void *callback_arg;
  
  /* Function to call when a listener has been connected. */
  err_t (* accept)(void *arg, struct tcp_pcb *newpcb, err_t err);

  struct ip_addr local_ip;
  u16_t local_port;
  
  struct ip_addr dest_ip;
  u16_t dest_port;
  
  /* receiver varables */
  u32_t rcv_nxt;   /* next seqno expected */
  u16_t rcv_wnd;   /* receiver window */

  /* Timers */
  u16_t tmr;

  /* Retransmission timer. */
  u8_t rtime;
  
  u16_t mss;   /* maximum segment size */

  u8_t flags;
#define TF_ACK_NEXT 0x01   /* Delayed ACK. */
#define TF_INFR     0x02   /* In fast recovery. */
#define TF_RESET    0x04   /* Connection was reset. */
#define TF_CLOSED   0x08   /* Connection was sucessfully closed. */
#define TF_GOT_FIN  0x10   /* Connection was closed by the remote end. */
  
  /* RTT estimation variables. */
  u16_t rttest; /* RTT estimate in 500ms ticks */
  u32_t rtseq;  /* sequence number being timed */
  s32_t sa, sv;

  u16_t rto;    /* retransmission time-out */
  u8_t nrtx;    /* number of retransmissions */

  /* fast retransmit/recovery */
  u32_t lastack; /* Highest acknowledged seqno. */
  u8_t dupacks;
  
  /* congestion avoidance/control variables */
  u16_t cwnd;  
  u16_t ssthresh;

  /* sender variables */
  u32_t snd_nxt,       /* next seqno to be sent */
    snd_max,       /* Highest seqno sent. */
    snd_wnd,       /* sender window */
    snd_wl1, snd_wl2,
    snd_lbb;      

  u16_t snd_buf;   /* Avaliable buffer space for sending. */
  u8_t snd_queuelen;

  /* Function to be called when more send buffer space is avaliable. */
  err_t (* sent)(void *arg, struct tcp_pcb *pcb, u16_t space);
  u16_t acked;
  
  /* Function to be called when (in-sequence) data has arrived. */
  err_t (* recv)(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
  struct pbuf *recv_data;

  /* Function to be called when a connection has been set up. */
  err_t (* connected)(void *arg, struct tcp_pcb *pcb, err_t err);

  /* Function which is called periodically. */
  err_t (* poll)(void *arg, struct tcp_pcb *pcb);

  /* Function to be called whenever a fatal error occurs. */
  void (* errf)(void *arg, err_t err);
  
  u8_t polltmr, pollinterval;
  
  /* These are ordered by sequence number: */
  struct tcp_seg *unsent;   /* Unsent (queued) segments. */
  struct tcp_seg *unacked;  /* Sent but unacknowledged segments. */
#if TCP_QUEUE_OOSEQ  
  struct tcp_seg *ooseq;    /* Received out of sequence segments. */
#endif /* TCP_QUEUE_OOSEQ */

};

struct tcp_pcb_listen {  
  struct tcp_pcb_listen *next;   /* for the linked list */
  
  enum tcp_state state;   /* TCP state */

  void *callback_arg;
  
  /* Function to call when a listener has been connected. */
  void (* accept)(void *arg, struct tcp_pcb *newpcb);

  struct ip_addr local_ip;
  u16_t local_port;
};

/* This structure is used to repressent TCP segments. */
struct tcp_seg {
  struct tcp_seg *next;    /* used when putting segements on a queue */
  struct pbuf *p;          /* buffer containing data + TCP header */
  void *dataptr;           /* pointer to the TCP data in the pbuf */
  u16_t len;               /* the TCP length of this segment */
  struct tcp_hdr *tcphdr;  /* the TCP header */
};

/* Internal functions and global variables: */
struct tcp_pcb *tcp_pcb_copy(struct tcp_pcb *pcb);
void tcp_pcb_purge(struct tcp_pcb *pcb);
void tcp_pcb_remove(struct tcp_pcb **pcblist, struct tcp_pcb *pcb);

u8_t tcp_segs_free(struct tcp_seg *seg);
u8_t tcp_seg_free(struct tcp_seg *seg);
struct tcp_seg *tcp_seg_copy(struct tcp_seg *seg);

#define tcp_ack(pcb)     if((pcb)->flags & TF_ACK_NEXT) { \
                            tcp_send_ctrl((pcb), TCP_ACK); \
                            (pcb)->flags &= ~TF_ACK_NEXT; \
                         } else { \
                            (pcb)->flags |= TF_ACK_NEXT; \
                         }

#define tcp_ack_now(pcb) tcp_send_ctrl((pcb), TCP_ACK); \
                         if((pcb)->flags & TF_ACK_NEXT) { \
                            (pcb)->flags &= ~TF_ACK_NEXT; \
                         } \
                         tcp_output(pcb)

err_t tcp_send_ctrl(struct tcp_pcb *pcb, u8_t flags);
err_t tcp_enqueue(struct tcp_pcb *pcb, void *dataptr, u16_t len,
		u8_t flags, u8_t copy,
                u8_t *optdata, u8_t optlen);

void tcp_rexmit_seg(struct tcp_pcb *pcb, struct tcp_seg *seg);

void tcp_rst(u32_t seqno, u32_t ackno,
	     struct ip_addr *local_ip, struct ip_addr *dest_ip,
	     u16_t local_port, u16_t dest_port);

u32_t tcp_next_iss(void);

extern u32_t tcp_ticks;

#if TCP_DEBUG
void tcp_debug_print(struct tcp_hdr *tcphdr);
void tcp_debug_print_flags(u8_t flags);
void tcp_debug_print_state(enum tcp_state s);
void tcp_debug_print_pcbs(void);
int tcp_pcbs_sane(void);
#else
#define tcp_pcbs_sane() 1
#endif /* TCP_DEBUG */


/* The TCP PCB lists. */
extern struct tcp_pcb_listen *tcp_listen_pcbs;  /* List of all TCP PCBs in LISTEN state. */
extern struct tcp_pcb *tcp_active_pcbs;  /* List of all TCP PCBs that are in a
					    state in which they accept or send
					    data. */
extern struct tcp_pcb *tcp_tw_pcbs;      /* List of all TCP PCBs in TIME-WAIT. */

extern struct tcp_pcb *tcp_tmp_pcb;      /* Only used for temporary storage. */

/* Axoims about the above lists:   
   1) Every TCP PCB that is not CLOSED is in one of the lists.
   2) A PCB is only in one of the lists.
   3) All PCBs in the tcp_listen_pcbs list is in LISTEN state.
   4) All PCBs in the tcp_tw_pcbs list is in TIME-WAIT state.
*/

/* Define two macros, TCP_REG and TCP_RMV that registers a TCP PCB
   with a PCB list or removes a PCB from a list, respectively. */
#ifdef LWIP_DEBUG
#define TCP_REG(pcbs, npcb) do {\
                            DEBUGF(TCP_DEBUG, ("TCP_REG %p local port %d\n", npcb, npcb->local_port)); \
                            for(tcp_tmp_pcb = *pcbs; \
				  tcp_tmp_pcb != NULL; \
				tcp_tmp_pcb = tcp_tmp_pcb->next) { \
                                ASSERT("TCP_REG: already registered\n", tcp_tmp_pcb != npcb); \
                            } \
                            ASSERT("TCP_REG: pcb->state != CLOSED", npcb->state != CLOSED); \
                            npcb->next = *pcbs; \
                            ASSERT("TCP_REG: npcb->next != npcb", npcb->next != npcb); \
                            *pcbs = npcb; \
                            ASSERT("TCP_RMV: tcp_pcbs sane", tcp_pcbs_sane()); \
                            } while(0)
#define TCP_RMV(pcbs, npcb) do { \
                            ASSERT("TCP_RMV: pcbs != NULL", *pcbs != NULL); \
                            DEBUGF(TCP_DEBUG, ("TCP_RMV: removing %p from %p\n", npcb, *pcbs)); \
                            if(*pcbs == npcb) { \
                               *pcbs = (*pcbs)->next; \
                            } else for(tcp_tmp_pcb = *pcbs; tcp_tmp_pcb != NULL; tcp_tmp_pcb = tcp_tmp_pcb->next) { \
                               if(tcp_tmp_pcb->next != NULL && tcp_tmp_pcb->next == npcb) { \
                                  tcp_tmp_pcb->next = npcb->next; \
                                  break; \
                               } \
                            } \
                            npcb->next = NULL; \
                            ASSERT("TCP_RMV: tcp_pcbs sane", tcp_pcbs_sane()); \
                            DEBUGF(TCP_DEBUG, ("TCP_RMV: removed %p from %p\n", npcb, *pcbs)); \
                            } while(0)

#else /* LWIP_DEBUG */
#define TCP_REG(pcbs, npcb) do { \
                            npcb->next = *pcbs; \
                            *pcbs = npcb; \
                            } while(0)
#define TCP_RMV(pcbs, npcb) do { \
                            if(*pcbs == npcb) { \
                               *pcbs = (*pcbs)->next; \
                            } else for(tcp_tmp_pcb = *pcbs; tcp_tmp_pcb != NULL; tcp_tmp_pcb = tcp_tmp_pcb->next) { \
                               if(tcp_tmp_pcb->next != NULL && tcp_tmp_pcb->next == npcb) { \
                                  tcp_tmp_pcb->next = npcb->next; \
                                  break; \
                               } \
                            } \
                            npcb->next = NULL; \
                            } while(0)
#endif /* LWIP_DEBUG */
#endif /* __LWIP_TCP_H__ */



