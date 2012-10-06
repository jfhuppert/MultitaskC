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
 * $Id: api_msg.c,v 1.1 2001/12/12 10:00:55 adam Exp $
 */

#include "lwip/debug.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/api_msg.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"

/*-----------------------------------------------------------------------------------*/
static err_t
recv_tcp(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
  struct netconn *conn;

  conn = arg;

  if(conn == NULL) {
    pbuf_free(p);
    return ERR_VAL;
  }
  
  if(conn->recvmbox != LLC_SYS_MBOX_NULL) {
    conn->err = err;
    llC_sys_mbox_post(conn->recvmbox, p);
  }  
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static void
recv_udp(void *arg, struct udp_pcb *pcb, struct pbuf *p,
	 struct ip_addr *addr, u16_t port)
{
  struct netbuf *buf;
  struct netconn *conn;

  conn = arg;
  
  if(conn == NULL) {
    pbuf_free(p);
    return;
  }

  if(conn->recvmbox != LLC_SYS_MBOX_NULL) {
    buf = memp_mallocp(MEMP_NETBUF);
    if(buf == NULL) {
      pbuf_free(p);
      return;
    } else {
      buf->p = p;
      buf->ptr = p;
      buf->fromaddr = addr;
      buf->fromport = port;
    }
    
    llC_sys_mbox_post(conn->recvmbox, buf);
  }
}
/*-----------------------------------------------------------------------------------*/
static err_t
poll_tcp(void *arg, struct tcp_pcb *pcb)
{
  struct netconn *conn;

  conn = arg;
  if(conn != NULL &&
     (conn->state == NETCONN_WRITE || conn->state == NETCONN_CLOSE) &&
     conn->sem != LLC_SYS_SEM_NULL) {
    llC_sys_sem_signal(conn->sem);
  }
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static err_t
sent_tcp(void *arg, struct tcp_pcb *pcb, u16_t len)
{
  struct netconn *conn;

  conn = arg;
  if(conn != NULL && conn->sem != LLC_SYS_SEM_NULL) {
    llC_sys_sem_signal(conn->sem);
  }
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static void
err_tcp(void *arg, err_t err)
{
  struct netconn *conn;

  conn = arg;

  conn->err = err;
  if(conn->recvmbox != LLC_SYS_MBOX_NULL) {
    llC_sys_mbox_post(conn->recvmbox, NULL);
  }
  if(conn->mbox != LLC_SYS_MBOX_NULL) {
    llC_sys_mbox_post(conn->mbox, NULL);
  }
  if(conn->acceptmbox != LLC_SYS_MBOX_NULL) {
    llC_sys_mbox_post(conn->acceptmbox, NULL);
  }
  if(conn->sem != LLC_SYS_SEM_NULL) {
    llC_sys_sem_signal(conn->sem);
  }
}
/*-----------------------------------------------------------------------------------*/
static void
setup_tcp(struct netconn *conn)
{
  struct tcp_pcb *pcb;
  
  pcb = conn->pcb.tcp;
  tcp_arg(pcb, conn);
  tcp_recv(pcb, recv_tcp);
  tcp_sent(pcb, sent_tcp);
  tcp_poll(pcb, poll_tcp, 4);
  tcp_err(pcb, err_tcp);
}
/*-----------------------------------------------------------------------------------*/
static err_t
accept_function(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  llC_sys_mbox_t *mbox;
  struct netconn *newconn;
  
#if API_MSG_DEBUG
#if TCP_DEBUG
  tcp_debug_print_state(newpcb->state);
#endif /* TCP_DEBUG */
#endif /* API_MSG_DEBUG */
  mbox = (llC_sys_mbox_t *)arg;
  dbg_putstr("accept_function 0\r\n");
  newconn = memp_mallocp(MEMP_NETCONN);
  if(newconn == NULL) {
    dbg_putstr("accept_function 1\r\n");
    return ERR_MEM;
  }
  dbg_putstr("accept_function 2\r\n");
  newconn->type = NETCONN_TCP;
  newconn->pcb.tcp = newpcb;
  setup_tcp(newconn);
  dbg_putstr("accept_function 3\r\n");
  newconn->recvmbox = llC_sys_mbox_new();
  if(newconn->recvmbox == LLC_SYS_MBOX_NULL) {
    dbg_putstr("accept_function 4\r\n");
    memp_free(MEMP_NETCONN, newconn);
    return ERR_MEM;
  }
  dbg_putstr("accept_function 5\r\n");
  newconn->mbox = llC_sys_mbox_new();
  if(newconn->mbox == LLC_SYS_MBOX_NULL) {
    dbg_putstr("accept_function 6\r\n");
    llC_sys_mbox_free(newconn->recvmbox);
    memp_free(MEMP_NETCONN, newconn);
    return ERR_MEM;
  }
  dbg_putstr("accept_function 7\r\n");
  newconn->sem = llC_sys_sem_new(0);
  if(newconn->sem == LLC_SYS_SEM_NULL) {
    dbg_putstr("accept_function 8\r\n");
    llC_sys_mbox_free(newconn->recvmbox);
    llC_sys_mbox_free(newconn->mbox);
    memp_free(MEMP_NETCONN, newconn);
    return ERR_MEM;
  }
  newconn->acceptmbox = LLC_SYS_MBOX_NULL;
  newconn->err = err;
  dbg_putstr("accept_function 9 0x");
  dbg_puthex((unsigned long)mbox);
  dbg_putstr(" 0x");
  dbg_puthex((unsigned long)newconn);
  dbg_putstr("\r\n");
  llC_sys_mbox_post(*mbox, newconn);
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static void
do_newconn(struct api_msg_msg *msg)
{
}
/*-----------------------------------------------------------------------------------*/
static void
do_delconn(struct api_msg_msg *msg)
{
  struct netconn *co=msg->conn;
  struct udp_pcb *ud;
  struct tcp_pcb *tc;
  if(co->pcb.tcp != NULL) {
    switch(co->type) {
    case NETCONN_UDPLITE:
      /* FALLTHROUGH */
    case NETCONN_UDPNOCHKSUM:
      /* FALLTHROUGH */
    case NETCONN_UDP:
      ud=co->pcb.udp;
      ud->recv_arg = NULL;
      udp_remove(co->pcb.udp);
      break;
    case NETCONN_TCP:
      tcp_arg(co->pcb.tcp, NULL);
      tcp_sent(co->pcb.tcp, NULL);
      tcp_recv(co->pcb.tcp, NULL);
      tcp_accept(co->pcb.tcp, NULL);
      tcp_poll(co->pcb.tcp, NULL, 0);
      tcp_err(co->pcb.tcp, NULL);
      tc=co->pcb.tcp;
      if(tc->state == LISTEN) {
	tcp_close(co->pcb.tcp);
      } else {
	if(tcp_close(co->pcb.tcp) != ERR_OK) {
	  tcp_abort(co->pcb.tcp);
	}
      }
    break;
    }
  }
  llC_sys_mbox_post(co->mbox, NULL);
}
/*-----------------------------------------------------------------------------------*/
static void
do_bind(struct api_msg_msg *msg)
{
  struct netconn *co=msg->conn;
  dbg_putstr("do_bind 0x");
  dbg_puthex((unsigned long)msg->conn);
  co=msg->conn;
  dbg_putstr(" 0x");
  dbg_puthex((unsigned long)co->pcb.tcp);
  dbg_putstr("\r\n");
  if(co->pcb.tcp == NULL) {
    dbg_putstr("do_bind 1\r\n");
    switch(co->type) {
    case NETCONN_UDPLITE:
      dbg_putstr("do_bind 20\r\n");
      co->pcb.udp = udp_new();
      udp_setflags(co->pcb.udp, UDP_FLAGS_UDPLITE);
      udp_recv(co->pcb.udp, recv_udp, co);
      break;
    case NETCONN_UDPNOCHKSUM:
      dbg_putstr("do_bind 21\r\n");
      co->pcb.udp = udp_new();
      udp_setflags(co->pcb.udp, UDP_FLAGS_NOCHKSUM);
      udp_recv(co->pcb.udp, recv_udp, co);
      break;
    case NETCONN_UDP:
      dbg_putstr("do_bind 22\r\n");
      co->pcb.udp = udp_new();
      udp_recv(co->pcb.udp, recv_udp, co);
      break;
    case NETCONN_TCP:
      dbg_putstr("do_bind 23\r\n");
      co->pcb.tcp = tcp_new();
      setup_tcp(co);
      break;
    }
  }
  dbg_putstr("do_bind 3\r\n");
  switch(co->type) {
  case NETCONN_UDPLITE:
    /* FALLTHROUGH */
  case NETCONN_UDPNOCHKSUM:
    /* FALLTHROUGH */
  case NETCONN_UDP:
    udp_bind(co->pcb.udp, msg->msg.bc.ipaddr, msg->msg.bc.port);
    break;
  case NETCONN_TCP:
    dbg_putstr("do_bind 4\r\n");
    co->err = tcp_bind(co->pcb.tcp,msg->msg.bc.ipaddr, msg->msg.bc.port);
    break;
  }
  dbg_putstr("do_bind 5\r\n");
  llC_sys_mbox_post(co->mbox, NULL);
  dbg_putstr("do_bind 6\r\n");
}
/*-----------------------------------------------------------------------------------*/
static err_t
do_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
  struct netconn *conn;

  conn = arg;

  if(conn == NULL) {
    return ERR_VAL;
  }
  
  conn->err = err;

  if(conn->type == NETCONN_TCP && err == ERR_OK) {
    setup_tcp(conn);
  }    
  
  llC_sys_mbox_post(conn->mbox, NULL);
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static void
do_connect(struct api_msg_msg *msg)
{
  struct netconn *co=msg->conn;
  if(co->pcb.tcp == NULL) {
    switch(co->type) {
    case NETCONN_UDPLITE:
      co->pcb.udp = udp_new();
      if(co->pcb.udp == NULL) {
	co->err = ERR_MEM;
	llC_sys_mbox_post(co->mbox, NULL);
	return;
      }
      udp_setflags(co->pcb.udp, UDP_FLAGS_UDPLITE);
      udp_recv(co->pcb.udp, recv_udp, co);
      break;
    case NETCONN_UDPNOCHKSUM:
      co->pcb.udp = udp_new();
      if(co->pcb.udp == NULL) {
	co->err = ERR_MEM;
	llC_sys_mbox_post(co->mbox, NULL);
	return;
      }
      udp_setflags(co->pcb.udp, UDP_FLAGS_NOCHKSUM);
      udp_recv(co->pcb.udp, recv_udp, co);
      break;
    case NETCONN_UDP:
      co->pcb.udp = udp_new();
      if(co->pcb.udp == NULL) {
	co->err = ERR_MEM;
	llC_sys_mbox_post(co->mbox, NULL);
	return;
      }
      udp_recv(co->pcb.udp, recv_udp, co);
      break;
    case NETCONN_TCP:
      co->pcb.tcp = tcp_new();
      if(co->pcb.tcp == NULL) {
	co->err = ERR_MEM;
	llC_sys_mbox_post(co->mbox, NULL);
	return;
      }
      break;
    }
  }
  switch(co->type) {
  case NETCONN_UDPLITE:
    /* FALLTHROUGH */
  case NETCONN_UDPNOCHKSUM:
    /* FALLTHROUGH */
  case NETCONN_UDP:
    udp_connect(co->pcb.udp, msg->msg.bc.ipaddr, msg->msg.bc.port);
    llC_sys_mbox_post(co->mbox, NULL);
    break;
  case NETCONN_TCP:
    tcp_arg(co->pcb.tcp, co);
    tcp_connect(co->pcb.tcp, msg->msg.bc.ipaddr, msg->msg.bc.port,
		do_connected);
    /*tcp_output(co->pcb.tcp);*/
    break;
  }
}
/*-----------------------------------------------------------------------------------*/
static void
do_listen(struct api_msg_msg *msg)
{
  struct netconn *co=msg->conn;
  if(co->pcb.tcp != NULL) {
    switch(co->type) {
    case NETCONN_UDPLITE:
      /* FALLTHROUGH */
    case NETCONN_UDPNOCHKSUM:
      /* FALLTHROUGH */
    case NETCONN_UDP:
      DEBUGF(API_MSG_DEBUG, ("api_msg: listen UDP: cannot listen for UDP.\n"));
      break;
    case NETCONN_TCP:
      co->pcb.tcp = tcp_listen(co->pcb.tcp);
      if(co->pcb.tcp == NULL) {
	co->err = ERR_MEM;
      } else {
	if(co->acceptmbox == LLC_SYS_MBOX_NULL) {
	  co->acceptmbox = llC_sys_mbox_new();
	  if(co->acceptmbox == LLC_SYS_MBOX_NULL) {
	    co->err = ERR_MEM;
	    break;
	  }
	}
	tcp_arg(co->pcb.tcp, (void *)&(co->acceptmbox));
	tcp_accept(co->pcb.tcp, accept_function);
      }
      break;
    }
  }
  llC_sys_mbox_post(co->mbox, NULL);
}
/*-----------------------------------------------------------------------------------*/
static void
do_accept(struct api_msg_msg *msg)
{
  struct netconn *co=msg->conn;
  if(co->pcb.tcp != NULL) {
    switch(co->type) {
    case NETCONN_UDPLITE:
      /* FALLTHROUGH */
    case NETCONN_UDPNOCHKSUM:
      /* FALLTHROUGH */
    case NETCONN_UDP:    
      DEBUGF(API_MSG_DEBUG, ("api_msg: accept UDP: cannot accept for UDP.\n"));
      break;
    case NETCONN_TCP:
      break;
    }
  }
}
/*-----------------------------------------------------------------------------------*/
static void
do_send(struct api_msg_msg *msg)
{
  struct netconn *co=msg->conn;
  if(co->pcb.tcp != NULL) {
    switch(co->type) {
    case NETCONN_UDPLITE:
      /* FALLTHROUGH */
    case NETCONN_UDPNOCHKSUM:
      /* FALLTHROUGH */
    case NETCONN_UDP:
      udp_send(co->pcb.udp, msg->msg.p);
      break;
    case NETCONN_TCP:
      break;
    }
  }
  llC_sys_mbox_post(co->mbox, NULL);
}
/*-----------------------------------------------------------------------------------*/
static void
do_recv(struct api_msg_msg *msg)
{
  struct netconn *co=msg->conn;
  dbg_putstr("do_recv 0\r\n");
  if(co->pcb.tcp != NULL) {
    if(co->type == NETCONN_TCP) {
      dbg_putstr("do_recv 1\r\n");
      tcp_recved(co->pcb.tcp, msg->msg.len);
      dbg_putstr("do_recv 2\r\n");
    }
  }
  dbg_putstr("do_recv 3\r\n");
  llC_sys_mbox_post(co->mbox, NULL);
  dbg_putstr("do_recv 3\r\n");
}
/*-----------------------------------------------------------------------------------*/
static void
do_write(struct api_msg_msg *msg)
{
  err_t err;
  struct netconn *co=msg->conn;
  struct tcp_pcb *tc;
  if(co->pcb.tcp != NULL) {
    switch(co->type) {
    case NETCONN_UDPLITE:
      /* FALLTHROUGH */
    case NETCONN_UDPNOCHKSUM:
      /* FALLTHROUGH */
    case NETCONN_UDP:
      co->err = ERR_VAL;
      break;
    case NETCONN_TCP:      
      err = tcp_write(co->pcb.tcp, msg->msg.w.dataptr,
                      msg->msg.w.len, msg->msg.w.copy);
      /* This is the Nagle algorithm: inhibit the sending of new TCP
	 segments when new outgoing data arrives from the user if any
	 previously transmitted data on the connection remains
	 unacknowledged. */
      tc=co->pcb.tcp;
      if(err == ERR_OK && tc->unacked == NULL) {
	tcp_output(co->pcb.tcp);
      }
      co->err = err;
      break;
    }
  }
  llC_sys_mbox_post(co->mbox, NULL);
}
/*-----------------------------------------------------------------------------------*/
static void
do_close(struct api_msg_msg *msg)
{
  err_t err;
  struct netconn *co=msg->conn;
  struct tcp_pcb *tc;
  dbg_putstr("do_close 0\r\n");
  if(co->pcb.tcp != NULL) {
    dbg_putstr("do_close 1\r\n");
    switch(co->type) {
    case NETCONN_UDPLITE:
      /* FALLTHROUGH */
    case NETCONN_UDPNOCHKSUM:
      /* FALLTHROUGH */
    case NETCONN_UDP:
      break;
    case NETCONN_TCP:
      dbg_putstr("do_close 2\r\n");
      tc=co->pcb.tcp;
      if(tc->state == LISTEN) {
        dbg_putstr("do_close 3\r\n");
	err = tcp_close(co->pcb.tcp);
      } else {
        dbg_putstr("do_close 4\r\n");
	err = tcp_close(co->pcb.tcp);
      }
      co->err = err;      
      break;
    }
  }
  dbg_putstr("do_close 5\r\n");
  llC_sys_mbox_post(co->mbox, NULL);
}
/*-----------------------------------------------------------------------------------*/
#if 0
typedef void (* api_msg_decode)(struct api_msg_msg *msg);
static api_msg_decode decode[API_MSG_MAX] = {
  do_newconn,
  do_delconn,
  do_bind,
  do_connect,
  do_listen,
  do_accept,
  do_send,
  do_recv,
  do_write,
  do_close
  };
#endif
void
api_msg_input(struct api_msg *msg)
{  
  dbg_putstr("api_msg_input 0x");
  dbg_puthex(msg->type);
  dbg_putstr("\r\n");
  switch(msg->type) {
  case API_MSG_NEWCONN : do_newconn(&(msg->msg)); break;
  case API_MSG_DELCONN : do_delconn(&(msg->msg)); break;
  case API_MSG_BIND : do_bind(&(msg->msg)); break;
  case API_MSG_CONNECT : do_connect(&(msg->msg)); break;
  case API_MSG_LISTEN : do_listen(&(msg->msg)); break;
  case API_MSG_ACCEPT : do_accept(&(msg->msg)); break;
  case API_MSG_SEND : do_send(&(msg->msg)); break;
  case API_MSG_RECV : do_recv(&(msg->msg)); break;
  case API_MSG_WRITE : do_write(&(msg->msg)); break;
  case API_MSG_CLOSE : do_close(&(msg->msg)); break;
  case API_MSG_MAX :
  default : break;
  }
  // decode[msg->type](&(msg->msg));
  dbg_putstr("api_msg_input 1\r\n");
}
/*-----------------------------------------------------------------------------------*/
