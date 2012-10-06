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
 * $Id: api_lib.c,v 1.1 2001/12/12 10:00:54 adam Exp $
 */

/* This is the part of the API that is linked with
   the application */

#include "lwip/debug.h"
#include "lwip/api.h"
#include "lwip/api_msg.h"
#include "lwip/mem.h"
#include "lwip/memp.h"

/*-----------------------------------------------------------------------------------*/
int llC_test_and_set_mbox(llC_sys_mbox_t *m,void **p)
{
  volatile void *val;

  bcopy(m,&val,sizeof(llC_sys_mbox_t));

#if 0
  dbg_putstr("tasm 0 0x");
  dbg_puthex((unsigned long)m);
  dbg_putstr(" 0x");
  dbg_puthex((unsigned long)val);
  dbg_putstr("\r\n");
#endif

  if(val==LLC_SYS_MBOX_EMPTY) return(1);
  if(p!=0) *p=(void *)val;
  val=LLC_SYS_MBOX_EMPTY;
  bcopy(&val,m,sizeof(llC_sys_mbox_t));
  return(0);
} 
/*-----------------------------------------------------------------------------------*/
struct
netbuf *netbuf_new(void)
{
  struct netbuf *buf;

  buf = memp_mallocp(MEMP_NETBUF);
  if(buf != NULL) {
    buf->p = NULL;
    buf->ptr = NULL;
    return buf;
  } else {
    return NULL;
  }
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_delete(struct netbuf *buf)
{
  if(buf != NULL) {
    if(buf->p != NULL) {
      pbuf_free(buf->p);
      buf->p = buf->ptr = NULL;
    }
    memp_freep(MEMP_NETBUF, buf);
  }
}
/*-----------------------------------------------------------------------------------*/
void *
netbuf_alloc(struct netbuf *buf, u16_t size)
{
  struct pbuf *tmp_pbuf;
  /* Deallocate any previously allocated memory. */
  if(buf->p != NULL) {
    pbuf_free(buf->p);
  }
  buf->p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);
  if(buf->p == NULL) {
     return NULL;
  }
  buf->ptr = buf->p;
  tmp_pbuf=buf->p;
  return tmp_pbuf->payload;
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_free(struct netbuf *buf)
{
  if(buf->p != NULL) {
    pbuf_free(buf->p);
  }
  buf->p = buf->ptr = NULL;
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_ref(struct netbuf *buf, void *dataptr, u16_t size)
{
  struct pbuf *tmp_pbuf;
  if(buf->p != NULL) {
    pbuf_free(buf->p);
  }
  buf->p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_ROM);
  tmp_pbuf=buf->p;
  tmp_pbuf->payload = dataptr;
  tmp_pbuf->len = tmp_pbuf->tot_len = size;
  buf->ptr = buf->p;
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_chain(struct netbuf *head, struct netbuf *tail)
{
  pbuf_chain(head->p, tail->p);
  head->ptr = head->p;
  memp_freep(MEMP_NETBUF, tail);
}
/*-----------------------------------------------------------------------------------*/
u16_t
netbuf_len(struct netbuf *buf)
{
  struct pbuf *tmp_pbuf;
  tmp_pbuf=buf->p;
  return tmp_pbuf->tot_len;
}
/*-----------------------------------------------------------------------------------*/
err_t
netbuf_data(struct netbuf *buf, void **dataptr, u16_t *len)
{
  struct pbuf *tmp_pbuf;
  if(buf->ptr == NULL) {
    return ERR_BUF;
  }
  tmp_pbuf=buf->ptr;
  *dataptr = tmp_pbuf->payload;
  *len = tmp_pbuf->len;
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
s8_t
netbuf_next(struct netbuf *buf)
{
  struct pbuf *tmp_pbuf;
  tmp_pbuf=buf->ptr;
  if(tmp_pbuf->next == NULL) {
    return -1;
  }
  buf->ptr = tmp_pbuf->next;
  tmp_pbuf=buf->ptr;
  if(tmp_pbuf->next == NULL) {
    return 1;
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_first(struct netbuf *buf)
{
  buf->ptr = buf->p;
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_copy(struct netbuf *buf, void *dataptr, u16_t len)
{
  struct pbuf *p;
  unsigned long i, left;
  unsigned char *from,*to;

  left = 0;

  dbg_putstr("netbuf_copy 0\r\n");
  if(buf == NULL) {
    return;
  }
  
  /* This implementation is bad. It should use bcopy
     instead. */
  dbg_putstr("netbuf_copy 1 0x");
  dbg_puthex((unsigned long)buf);
  dbg_putstr(" 0x");
  dbg_puthex((unsigned long)buf->p);
  dbg_putstr("\r\n");
  for(p = buf->p; left < len && p != NULL; p = p->next) {
    dbg_putstr("netbuf_copy 20\r\n");
    to=(unsigned char *)(((unsigned long)dataptr)+left);
    dbg_putstr("netbuf_copy 20\r\n");
    from=(unsigned char *)p->payload;
    dbg_putstr("netbuf_copy 20\r\n");
    for(i = 0; i < p->len; ++i) {
      // ((char *)dataptr)[left] = ((char *)p->payload)[i];
      dbg_putstr("netbuf_copy 21\r\n");
      *to++=*from++;
      dbg_putstr("netbuf_copy 22\r\n");
      if(++left >= len) {
        dbg_putstr("netbuf_copy 3\r\n");
	return;
      }
    }
    dbg_putstr("netbuf_copy 4\r\n");
  }
  dbg_putstr("netbuf_copy 5\r\n");
  return;
}
/*-----------------------------------------------------------------------------------*/
struct ip_addr *
netbuf_fromaddr(struct netbuf *buf)
{
  return buf->fromaddr;
}
/*-----------------------------------------------------------------------------------*/
u16_t
netbuf_fromport(struct netbuf *buf)
{
  return buf->fromport;
}
/*-----------------------------------------------------------------------------------*/
struct
netconn *netconn_new(enum netconn_type t)
{
  struct netconn *conn;

  dbg_putstr("netconn_new 0\r\n");
  conn = memp_mallocp(MEMP_NETCONN);
  dbg_putstr("netconn_new 0x");
  dbg_puthex((unsigned long)conn);
  dbg_putstr("\r\n");
  if(conn == NULL) {
    dbg_putstr("netconn_new 1\r\n");
    return NULL;
  }
  conn->type = t;
  conn->pcb.tcp = NULL;

  if((conn->mbox = llC_sys_mbox_new()) == LLC_SYS_MBOX_NULL) {
    memp_freep(MEMP_NETCONN, conn);
    dbg_putstr("netconn_new 2\r\n");
    return NULL;
  }
  conn->recvmbox = LLC_SYS_MBOX_NULL;
  conn->sem = LLC_SYS_SEM_NULL;
  conn->state = NETCONN_NONE;
  dbg_putstr("netconn_new 3\r\n");
  return conn;
}
/*-----------------------------------------------------------------------------------*/
#if 0
err_t
netconn_delete(struct netconn *conn)
{
  struct api_msg *msg;
  void *mem;
  
  if(conn == NULL) {
    return ERR_OK;
  }
  
  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) {
    return ERR_MEM;
  }
  
  msg->type = API_MSG_DELCONN;
  msg->msg.conn = conn;
  llC_api_msg_post(msg);  
  llC_sys_mbox_fetch(conn->mbox, NULL);
  memp_free(MEMP_API_MSG, msg);

  /* Drain the recvmbox. */
  if(conn->recvmbox != LLC_SYS_MBOX_NULL) {
    while(sys_arch_mbox_fetch(conn->recvmbox, &mem, 1) != 0) {
      if(conn->type == NETCONN_TCP) {
	pbuf_free((struct pbuf *)mem);
      } else {
	netbuf_delete((struct netbuf *)mem);
      }
    }
    llC_sys_mbox_free(conn->recvmbox);
    conn->recvmbox = LLC_SYS_MBOX_NULL;
  }
 

  /* Drain the acceptmbox. */
  if(conn->acceptmbox != LLC_SYS_MBOX_NULL) {
    while(sys_arch_mbox_fetch(conn->acceptmbox, &mem, 1) != 0) {
      netconn_delete((struct netconn *)mem);
    }
    
    llC_sys_mbox_free(conn->acceptmbox);
    conn->acceptmbox = LLC_SYS_MBOX_NULL;
  }

  llC_sys_mbox_free(conn->mbox);
  conn->mbox = LLC_SYS_MBOX_NULL;
  llC_sys_sem_free(conn->sem);
  /*  conn->sem = LLC_SYS_SEM_NULL;*/
  memp_free(MEMP_NETCONN, conn);
  return ERR_OK;
}
#endif
/*-----------------------------------------------------------------------------------*/
enum netconn_type
netconn_type(struct netconn *conn)
{
  return conn->type;
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_peer(struct netconn *conn, struct ip_addr **addr,
	     u16_t *port)
{
  struct tcp_pcb *tmp_tcp_pcb;
  struct udp_pcb *tmp_udp_pcb;
  switch(conn->type) {
  case NETCONN_UDPLITE:
  case NETCONN_UDPNOCHKSUM:
  case NETCONN_UDP:
    tmp_udp_pcb=conn->pcb.udp;
    *addr = &(tmp_udp_pcb->dest_ip);
    *port = tmp_udp_pcb->dest_port;
    break;
  case NETCONN_TCP:
    tmp_tcp_pcb=conn->pcb.tcp;
    *addr = &(tmp_tcp_pcb->dest_ip);
    *port = tmp_tcp_pcb->dest_port;
    break;
  }
  return (conn->err = ERR_OK);
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_addr(struct netconn *conn, struct ip_addr **addr,
	     u16_t *port)
{
  struct tcp_pcb *tmp_tcp_pcb;
  struct udp_pcb *tmp_udp_pcb;
  switch(conn->type) {
  case NETCONN_UDPLITE:
  case NETCONN_UDPNOCHKSUM:
  case NETCONN_UDP:
    tmp_udp_pcb=conn->pcb.udp;
    *addr = &(tmp_udp_pcb->local_ip);
    *port = tmp_udp_pcb->local_port;
    break;
  case NETCONN_TCP:
    tmp_tcp_pcb=conn->pcb.tcp;
    *addr = &(tmp_tcp_pcb->local_ip);
    *port = tmp_tcp_pcb->local_port;
    break;
  }
  return (conn->err = ERR_OK);
}
/*-----------------------------------------------------------------------------------*/
#if 0
err_t
netconn_bind(struct netconn *conn, struct ip_addr *addr,
	    u16_t port)
{
  struct api_msg *msg;

  if(conn == NULL) {
    return ERR_VAL;
  }

  if(conn->type != NETCONN_TCP &&
     conn->recvmbox == LLC_SYS_MBOX_NULL) {
    if((conn->recvmbox = llC_sys_mbox_new()) == LLC_SYS_MBOX_NULL) {
      return ERR_MEM;
    }
  }
  
  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) {
    return (conn->err = ERR_MEM);
  }
  msg->type = API_MSG_BIND;
  msg->msg.conn = conn;
  msg->msg.msg.bc.ipaddr = addr;
  msg->msg.msg.bc.port = port;
  llC_api_msg_post(msg);
  llC_sys_mbox_fetch(conn->mbox, NULL);
  memp_free(MEMP_API_MSG, msg);
  return conn->err;
}
#endif
/*-----------------------------------------------------------------------------------*/
#if 0
err_t
netconn_connect(struct netconn *conn, struct ip_addr *addr,
		   u16_t port)
{
  struct api_msg *msg;
  
  if(conn == NULL) {
    return ERR_VAL;
  }


  if(conn->recvmbox == LLC_SYS_MBOX_NULL) {
    if((conn->recvmbox = llC_sys_mbox_new()) == LLC_SYS_MBOX_NULL) {
      return ERR_MEM;
    }
  }
  
  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) {
    return ERR_MEM;
  }
  msg->type = API_MSG_CONNECT;
  msg->msg.conn = conn;  
  msg->msg.msg.bc.ipaddr = addr;
  msg->msg.msg.bc.port = port;
  llC_api_msg_post(msg);
  llC_sys_mbox_fetch(conn->mbox, NULL);
  memp_free(MEMP_API_MSG, msg);
  return conn->err;
}
#endif
/*-----------------------------------------------------------------------------------*/
#if 0
err_t
netconn_listen(struct netconn *conn)
{
  struct api_msg *msg;

  if(conn == NULL) {
    return ERR_VAL;
  }

  if(conn->acceptmbox == LLC_SYS_MBOX_NULL) {
    conn->acceptmbox = llC_sys_mbox_new();
    if(conn->acceptmbox == LLC_SYS_MBOX_NULL) {
      return ERR_MEM;
    }
  }
  
  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) {
    return (conn->err = ERR_MEM);
  }
  msg->type = API_MSG_LISTEN;
  msg->msg.conn = conn;
  llC_api_msg_post(msg);
  llC_sys_mbox_fetch(conn->mbox, NULL);
  memp_free(MEMP_API_MSG, msg);
  return conn->err;
}
#endif
/*-----------------------------------------------------------------------------------*/
#if 0
struct netconn *
netconn_accept(struct netconn *conn)
{
  struct netconn *newconn;
  
  if(conn == NULL) {
    return NULL;
  }
  
  llC_sys_mbox_fetch(conn->acceptmbox, (void **)&newconn);

  return newconn;
}
#endif
/*-----------------------------------------------------------------------------------*/
#if 0
struct netbuf *
netconn_recv(struct netconn *conn)
{
  struct api_msg *msg;
  struct netbuf *buf;
  struct pbuf *p;
    
  if(conn == NULL) {
    return NULL;
  }
  
  if(conn->recvmbox == LLC_SYS_MBOX_NULL) {
    conn->err = ERR_CONN;
    return NULL;
  }

  if(conn->err != ERR_OK) {
    return NULL;
  }

  if(conn->type == NETCONN_TCP) {
    if(conn->pcb.tcp->state == LISTEN) {
      conn->err = ERR_CONN;
      return NULL;
    }


    buf = memp_mallocp(MEMP_NETBUF);

    if(buf == NULL) {
      conn->err = ERR_MEM;
      return NULL;
    }
    
    llC_sys_mbox_fetch(conn->recvmbox, (void **)&p);
    
    /* If we are closed, we indicate that we no longer wish to recieve
       data by setting conn->recvmbox to LLC_SYS_MBOX_NULL. */
    if(p == NULL) {
      memp_freep(MEMP_NETBUF, buf);
      llC_sys_mbox_free(conn->recvmbox);
      conn->recvmbox = LLC_SYS_MBOX_NULL;
      return NULL;
    }

    buf->p = p;
    buf->ptr = p;
    buf->fromport = 0;
    buf->fromaddr = NULL;

    /* Let the stack know that we have taken the data. */
    if((msg = memp_malloc(MEMP_API_MSG)) == NULL) {
      conn->err = ERR_MEM;
      return buf;
    }
    msg->type = API_MSG_RECV;
    msg->msg.conn = conn;
    if(buf != NULL) {
      msg->msg.msg.len = buf->p->tot_len;
    } else {
      msg->msg.msg.len = 1;
    }
    llC_api_msg_post(msg);

    llC_sys_mbox_fetch(conn->mbox, NULL);
    memp_free(MEMP_API_MSG, msg);
  } else {
    llC_sys_mbox_fetch(conn->recvmbox, (void **)&buf);
  }

  

    
  DEBUGF(API_LIB_DEBUG, ("netconn_recv: received %p (err %d)\n", buf, conn->err));


  return buf;
}
#endif
/*-----------------------------------------------------------------------------------*/
#if 0
err_t
netconn_send(struct netconn *conn, struct netbuf *buf)
{
  struct api_msg *msg;

  if(conn == NULL) {
    return ERR_VAL;
  }

  if(conn->err != ERR_OK) {
    return conn->err;
  }

  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) {
    return (conn->err = ERR_MEM);
  }

  DEBUGF(API_LIB_DEBUG, ("netconn_send: sending %d bytes\n", buf->p->tot_len));
  msg->type = API_MSG_SEND;
  msg->msg.conn = conn;
  msg->msg.msg.p = buf->p;
  llC_api_msg_post(msg);

  llC_sys_mbox_fetch(conn->mbox, NULL);
  memp_free(MEMP_API_MSG, msg);
  return conn->err;
}
#endif
/*-----------------------------------------------------------------------------------*/
#if 0
err_t
netconn_write(struct netconn *conn, void *dataptr, u16_t size, u8_t copy)
{
  struct api_msg *msg;
  u16_t len;
  
  if(conn == NULL) {
    return ERR_VAL;
  }

  if(conn->sem == LLC_SYS_SEM_NULL) {
    conn->sem = llC_sys_sem_new(0);
    if(conn->sem == LLC_SYS_SEM_NULL) {
      return ERR_MEM;
    }
  }

  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) {
    return (conn->err = ERR_MEM);
  }
  msg->type = API_MSG_WRITE;
  msg->msg.conn = conn;
        

  conn->state = NETCONN_WRITE;
  while(conn->err == ERR_OK && size > 0) {
    msg->msg.msg.w.dataptr = dataptr;
    msg->msg.msg.w.copy = copy;
    
    if(conn->type == NETCONN_TCP) {
      if(tcp_sndbuf(conn->pcb.tcp) == 0) {
	llC_sys_sem_wait(conn->sem);
	if(conn->err != ERR_OK) {
	  goto ret;
	}
      }
      if(size > tcp_sndbuf(conn->pcb.tcp)) {
	/* We cannot send more than one send buffer's worth of data at a
	   time. */
	len = tcp_sndbuf(conn->pcb.tcp);
      } else {
	len = size;
      }
    } else {
      len = size;
    }
    
    DEBUGF(API_LIB_DEBUG, ("netconn_write: writing %d bytes (%d)\n", len, copy));
    msg->msg.msg.w.len = len;
    llC_api_msg_post(msg);
    llC_sys_mbox_fetch(conn->mbox, NULL);    
    if(conn->err == ERR_OK) {
      dataptr = (void *)((char *)dataptr + len);
      size -= len;
    } else if(conn->err == ERR_MEM) {
      conn->err = ERR_OK;
      llC_sys_sem_wait(conn->sem);
    } else {
      goto ret;
    }
  }
 ret:
  memp_free(MEMP_API_MSG, msg);
  conn->state = NETCONN_NONE;
  if(conn->sem != LLC_SYS_SEM_NULL) {
    llC_sys_sem_free(conn->sem);
    conn->sem = LLC_SYS_SEM_NULL;
  }
  return conn->err;
}
#endif
/*-----------------------------------------------------------------------------------*/
#if 0
err_t
netconn_close(struct netconn *conn)
{
  struct api_msg *msg;

  if(conn == NULL) {
    return ERR_VAL;
  }
  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) {
    return (conn->err = ERR_MEM);
  }

  conn->state = NETCONN_CLOSE;
 again:
  msg->type = API_MSG_CLOSE;
  msg->msg.conn = conn;
  llC_api_msg_post(msg);
  llC_sys_mbox_fetch(conn->mbox, NULL);
  if(conn->err == ERR_MEM &&
     conn->sem != LLC_SYS_SEM_NULL) {
    llC_sys_sem_wait(conn->sem);
    goto again;
  }
  conn->state = NETCONN_NONE;
  memp_free(MEMP_API_MSG, msg);
  return conn->err;
}
#endif
/*-----------------------------------------------------------------------------------*/
err_t
netconn_err(struct netconn *conn)
{
  return conn->err;
}
/*-----------------------------------------------------------------------------------*/




