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
 * $Id: api.h,v 1.1 2001/12/12 10:01:00 adam Exp $
 */
#ifndef __LWIP_API_H__
#define __LWIP_API_H__

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"

#include "lwip/ip.h"

#include "lwip/udp.h"
#include "lwip/tcp.h"

#include "lwip/err.h"

#define NETCONN_NOCOPY 0x00
#define NETCONN_COPY   0x01

enum netconn_type {
  NETCONN_TCP,
  NETCONN_UDP,
  NETCONN_UDPLITE,
  NETCONN_UDPNOCHKSUM
};

enum netconn_state {
  NETCONN_NONE,
  NETCONN_WRITE,
  NETCONN_ACCEPT,
  NETCONN_RECV,
  NETCONN_CONNECT,
  NETCONN_CLOSE
};

struct netbuf {
  struct pbuf *p, *ptr;
  struct ip_addr *fromaddr;
  u16_t fromport;
  err_t err;
};

typedef volatile void *llC_sys_mbox_t;
typedef volatile unsigned long llC_sys_sem_t;

#define LLC_SYS_MBOX_EMPTY ((llC_sys_mbox_t)0xfffffffe)
#define LLC_SYS_MBOX_NULL ((llC_sys_mbox_t)0xffffffff)
#define LLC_SYS_SEM_NULL ((llC_sys_sem_t)0xffffffff)

struct netconn {
  enum netconn_type type;
  enum netconn_state state;
  union {
    struct tcp_pcb *tcp;
    struct udp_pcb *udp;
  } pcb;
  err_t err;
  llC_sys_mbox_t mbox;
  llC_sys_mbox_t recvmbox;
  llC_sys_mbox_t acceptmbox;
  llC_sys_sem_t sem;
};

/* Network buffer functions: */
struct netbuf *   netbuf_new      (void);
void              netbuf_delete   (struct netbuf *buf);
void *            netbuf_alloc    (struct netbuf *buf, u16_t size);
void              netbuf_free     (struct netbuf *buf);
void              netbuf_ref      (struct netbuf *buf, void *dataptr, u16_t size);
void              netbuf_chain    (struct netbuf *head, struct netbuf *tail);
u16_t             netbuf_len      (struct netbuf *buf);
err_t             netbuf_data     (struct netbuf *buf, void **dataptr, u16_t *len);
s8_t              netbuf_next     (struct netbuf *buf);
void              netbuf_first    (struct netbuf *buf);
void              netbuf_copy     (struct netbuf *buf, void *dataptr, u16_t len);
struct ip_addr *  netbuf_fromaddr (struct netbuf *buf);
u16_t             netbuf_fromport (struct netbuf *buf);

/* Network connection functions: */
struct netconn *  netconn_new     (enum netconn_type type);
// err_t             netconn_delete  (struct netconn *conn);
enum netconn_type netconn_type    (struct netconn *conn);
err_t             netconn_peer    (struct netconn *conn, struct ip_addr **addr, u16_t *port);
err_t             netconn_addr    (struct netconn *conn, struct ip_addr **addr, u16_t *port);
// err_t             netconn_bind    (struct netconn *conn, struct ip_addr *addr, u16_t port);
// err_t             netconn_connect (struct netconn *conn, struct ip_addr *addr, u16_t port);
// err_t             netconn_listen  (struct netconn *conn);
// struct netconn *  netconn_accept  (struct netconn *conn);
// struct netbuf *   netconn_recv    (struct netconn *conn);
// err_t             netconn_send    (struct netconn *conn, struct netbuf *buf);
// err_t             netconn_write   (struct netconn *conn, void *dataptr, u16_t size, u8_t copy);
// err_t             netconn_close   (struct netconn *conn);
err_t             netconn_err     (struct netconn *conn);

#define llC_api_msg_post(m) _QPost(tcpip_msgq,m)

#define llC_sys_mbox_new()  LLC_SYS_MBOX_EMPTY
#define llC_sys_mbox_free(m)
#define llC_sys_mbox_post(m,p) (m=(void *)p)
#define llC_sys_mbox_fetch(m,p) while(llC_test_and_set_mbox(&m,(void **)p))
#define llC_sys_mbox_fetch_t(r,m,p,t) { \
	volatile int to=t; \
	while(llC_test_and_set_mbox(&m,(void **)p) && to-->0); \
	r=to>0?to:0; \
	}
 
#define llC_sys_sem_new(n) ((unsigned long)n)
#define llC_sys_sem_free(s)
#define llC_sys_sem_signal(s) (s+=1)
#define llC_sys_sem_wait(s) while(s==0?1:s--,0)

#define StkDecl(t,s)    struct { t buf[(s)]; t *sp; t *down; }
#define StkInit(st,s)   (st).sp=(st).down=(&(st).buf[s])
#define StkPush(st,e)   (*(--(st).sp))=e
#define StkPop(st)              (*(st).sp++)
#define StkTop(st)              (*(st).sp)
#define StkSP(st)               ((st).sp)
#define StkEP(st,p)             ((p)>=(st).down)
#define StkNElem(st)    ((st).down-(st).sp)
#define StkEmpty(st)    ((st).sp>=(st).down)
#define StkFull(st)             ((st).sp<=(&(st).buf[0]))

/*------------------------------------------------------------------------*/
#define LLC_NCDEL_STKSIZE 16
#define LLC_NCDEL_TIMEOUT 10000
#define netconn_delete(r,__conn) \
block(netconn_delete) { \
  struct api_msg *msg; \
  void *mem; \
  int res; \
  StkDecl(struct netconn *,LLC_NCDEL_STKSIZE) cstack; \
   \
  if(__conn == NULL) { \
    r=ERR_OK; \
    break(netconn_delete); \
  } \
   \
  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) { \
    r=ERR_MEM; \
    break(netconn_delete); \
  } \
   \
  StkInit(cstack,LLC_NCDEL_STKSIZE); \
  StkPush(cstack,__conn); \
  while(!StkEmpty(cstack)) { \
    group { \
      __conn=StkPop(cstack); \
      dbg_putstr("__conn\r\n"); \
      msg->type = API_MSG_DELCONN; \
      msg->msg.conn = __conn; \
    } \
    llC_api_msg_post(msg); \
    llC_sys_mbox_fetch(__conn->mbox, NULL); \
    memp_free(MEMP_API_MSG, msg); \
   \
    if(__conn->recvmbox != LLC_SYS_MBOX_NULL) { \
      for(;;) { \
        llC_sys_mbox_fetch_t(res,__conn->recvmbox,&mem,LLC_NCDEL_TIMEOUT); \
        if(res!=0) { \
          dbg_putstr("drain recv\r\n"); \
          if(__conn->type == NETCONN_TCP) { \
            pbuf_free((struct pbuf *)mem); \
          } else { \
            netbuf_delete((struct netbuf *)mem); \
          } \
        } else break; \
      } \
      llC_sys_mbox_free(__conn->recvmbox); \
      __conn->recvmbox = LLC_SYS_MBOX_NULL; \
    } \
    if(__conn->acceptmbox != LLC_SYS_MBOX_NULL) { \
      for(;;) { \
        llC_sys_mbox_fetch_t(res,__conn->acceptmbox,&mem,LLC_NCDEL_TIMEOUT); \
        if(res!=0) { \
          dbg_putstr("drain acc\r\n"); \
          StkPush(cstack,(struct netconn *)mem); \
        } else break; \
      } \
      llC_sys_mbox_free(__conn->acceptmbox); \
      __conn->acceptmbox = LLC_SYS_MBOX_NULL; \
    } \
    llC_sys_mbox_free(__conn->mbox); \
    __conn->mbox = LLC_SYS_MBOX_NULL; \
    llC_sys_sem_free(__conn->sem); \
    memp_free(MEMP_NETCONN, __conn); \
  } \
  r=ERR_OK; \
}
/*------------------------------------------------------------------------*/
#define netconn_bind(r,__conn,__addr,__port) \
block(netconn_bind) { \
  struct api_msg *msg; \
 \
  if(__conn == NULL) { \
    r=ERR_VAL; \
    break(netconn_bind); \
  } \
 \
  if(__conn->type != NETCONN_TCP && \
     __conn->recvmbox == LLC_SYS_MBOX_NULL) { \
    if((__conn->recvmbox = llC_sys_mbox_new()) == LLC_SYS_MBOX_NULL) { \
      r=ERR_MEM; \
      break(netconn_bind); \
    } \
  } \
   \
  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) { \
    r=__conn->err=ERR_MEM; \
    break(netconn_bind); \
  } \
  group { \
    msg->type = API_MSG_BIND; \
    msg->msg.conn = __conn; \
    msg->msg.msg.bc.ipaddr = __addr; \
    msg->msg.msg.bc.port = __port; \
  } \
  llC_api_msg_post(msg); \
  llC_sys_mbox_fetch(__conn->mbox, NULL); \
  group { \
    memp_free(MEMP_API_MSG, msg); \
    r=__conn->err; \
  } \
}
/*------------------------------------------------------------------------*/
#define netconn_connect(r,__conn,__addr,__port) \
block(netconn_connect) { \
  struct api_msg *msg; \
 \
  if(__conn == NULL) { \
    r=ERR_VAL; \
    break(netconn_connect); \
  } \
 \
  if(__conn->recvmbox == LLC_SYS_MBOX_NULL) { \
    if((__conn->recvmbox = llC_sys_mbox_new()) == LLC_SYS_MBOX_NULL) { \
      r=ERR_MEM; \
      break(netconn_connect); \
    } \
  } \
   \
  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) { \
    r=__conn->err=ERR_MEM; \
    break(netconn_connect); \
  } \
  group { \
    msg->type = API_MSG_CONNECT; \
    msg->msg.conn = __conn; \
    msg->msg.msg.bc.ipaddr = __addr; \
    msg->msg.msg.bc.port = __port; \
  } \
  llC_api_msg_post(msg); \
  llC_sys_mbox_fetch(__conn->mbox, NULL); \
  group { \
    memp_free(MEMP_API_MSG, msg); \
    r=__conn->err; \
  } \
}
/*------------------------------------------------------------------------*/
#define netconn_listen(r,__conn) \
block(netconn_listen) { \
  struct api_msg *msg; \
 \
  if(__conn == NULL) { \
    r=ERR_VAL; \
    break(netconn_listen); \
  } \
 \
  if(__conn->acceptmbox == LLC_SYS_MBOX_NULL) { \
    __conn->acceptmbox = llC_sys_mbox_new(); \
    if(__conn->acceptmbox == LLC_SYS_MBOX_NULL) { \
      r=ERR_MEM; \
      break(netconn_listen); \
    } \
  } \
   \
  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) { \
    r=__conn->err=ERR_MEM; \
    break(netconn_listen); \
  } \
  group { \
    msg->type = API_MSG_LISTEN; \
    msg->msg.conn = __conn; \
  } \
  llC_api_msg_post(msg); \
  llC_sys_mbox_fetch(__conn->mbox, NULL); \
  group { \
    memp_free(MEMP_API_MSG, msg); \
    r=__conn->err; \
  } \
}
/*------------------------------------------------------------------------*/
#define netconn_accept(__nc,__conn) \
block(netconn_accept) { \
  struct netconn *__newconn; \
   \
  if(__conn == NULL) { \
    __nc=NULL; \
    break(netconn_accept); \
  } \
   \
  dbg_putstr("netconn_accept 0\r\n"); \
  llC_sys_mbox_fetch(__conn->acceptmbox, (void **)&__newconn); \
  dbg_putstr("netconn_accept 1 0x"); \
  dbg_puthex((unsigned long)__newconn); \
  dbg_putstr("\r\n"); \
 \
  __nc=__newconn; \
}
/*------------------------------------------------------------------------*/
#define netconn_recv(__nb,__conn) \
block(netconn_recv) { \
  struct api_msg *msg; \
  struct netbuf *buf; \
  struct pbuf *p,*tp; \
  struct tcp_pcb *tmp_tcp_pcb; \
 \
  if(__conn==NULL) { \
    __nb=NULL; \
    break(netconn_recv); \
  } \
 \
  if(__conn->recvmbox==LLC_SYS_MBOX_NULL) { \
    __conn->err=ERR_CONN; \
    __nb=NULL; \
    break(netconn_recv); \
  } \
 \
  if(__conn->err!=ERR_OK) { \
    __nb=NULL; \
    break(netconn_recv); \
  } \
 \
  if(__conn->type==NETCONN_TCP) { \
    tmp_tcp_pcb=__conn->pcb.tcp; \
    if(tmp_tcp_pcb->state==LISTEN) { \
      group { \
        __conn->err=ERR_CONN; \
        __nb=NULL; \
      } \
      break(netconn_recv); \
    } \
 \
    buf=memp_mallocp(MEMP_NETBUF); \
 \
    if(buf==NULL) { \
      group { \
        __conn->err = ERR_MEM; \
        __nb= NULL; \
      } \
      break(netconn_recv); \
    } \
 \
    dbg_putstr("netconn_recv 0\r\n"); \
    llC_sys_mbox_fetch(__conn->recvmbox,(void **)&p); \
    dbg_putstr("netconn_recv 1\r\n"); \
    if(p==NULL) { \
      group { \
        memp_freep(MEMP_NETBUF,buf); \
        llC_sys_mbox_free(__conn->recvmbox); \
        __conn->recvmbox=LLC_SYS_MBOX_NULL; \
        __nb=NULL; \
      } \
      break(netconn_recv); \
    } \
 \
    group { \
      buf->p=p; \
      buf->ptr=p; \
      buf->fromport=0; \
      buf->fromaddr=NULL; \
    } \
    if((msg=memp_malloc(MEMP_API_MSG))==NULL) { \
      group { \
        __conn->err=ERR_MEM; \
        __nb=buf; \
      } \
      break(netconn_recv); \
    } \
    group { \
      msg->type=API_MSG_RECV; \
      msg->msg.conn=__conn; \
      if(buf!=NULL) { \
        msg->msg.msg.len=(tp=buf->p,tp->tot_len); \
      } else { \
        msg->msg.msg.len=1; \
      } \
      dbg_putstr("netconn_recv 2\r\n"); \
    } \
    llC_api_msg_post(msg); \
    dbg_putstr("netconn_recv 3\r\n"); \
    llC_sys_mbox_fetch(__conn->mbox,NULL); \
    dbg_putstr("netconn_recv 4\r\n"); \
    memp_free(MEMP_API_MSG,msg); \
  } else { \
    llC_sys_mbox_fetch(__conn->recvmbox,(void **)&buf); \
  } \
  dbg_putstr("netconn_recv 5\r\n"); \
  __nb= buf; \
}
/*------------------------------------------------------------------------*/
#define netconn_send(r,conn,buf)
/*------------------------------------------------------------------------*/
#define netconn_write(__r,__conn,__dataptr,__size,__copy) \
block(netconn_write) { \
  struct api_msg *__msg; \
  u16_t __len, ___size=__size; \
  unsigned char *___dataptr=(unsigned char *)__dataptr; \
 \
  if(__conn==NULL) { \
    __r=ERR_VAL; \
    break(netconn_write); \
  } \
 \
  if(__conn->sem==LLC_SYS_SEM_NULL) { \
    __conn->sem=llC_sys_sem_new(0); \
    if(__conn->sem==LLC_SYS_SEM_NULL) { \
      __r=ERR_MEM; \
      break(netconn_write); \
    } \
  } \
 \
  if((__msg=memp_malloc(MEMP_API_MSG)) == NULL) { \
    __r=(__conn->err=ERR_MEM); \
    break(netconn_write); \
  } \
  __msg->type=API_MSG_WRITE; \
  __msg->msg.conn=__conn; \
 \
  __conn->state=NETCONN_WRITE; \
  while(__conn->err==ERR_OK && ___size>0) { \
    __msg->msg.msg.w.dataptr=___dataptr; \
    __msg->msg.msg.w.copy=__copy; \
    if(__conn->type==NETCONN_TCP) { \
      if(tcp_sndbuf(__conn->pcb.tcp)==0) { \
	llC_sys_sem_wait(__conn->sem); \
	if(__conn->err!=ERR_OK) break; \
      } \
      if(___size>tcp_sndbuf(__conn->pcb.tcp)) __len=tcp_sndbuf(__conn->pcb.tcp); \
      else __len=___size; \
    } \
    else __len=___size; \
    __msg->msg.msg.w.len=__len; \
    llC_api_msg_post(__msg); \
    llC_sys_mbox_fetch(__conn->mbox,NULL); \
    if(__conn->err==ERR_OK) { \
      ___dataptr=(void *)((char *)___dataptr+__len); \
      ___size-=__len; \
    } \
    else if(__conn->err==ERR_MEM) { \
      __conn->err=ERR_OK; \
      llC_sys_sem_wait(__conn->sem); \
    } \
    else break; \
  } \
  group { \
    memp_free(MEMP_API_MSG,__msg); \
    __conn->state=NETCONN_NONE; \
    if(__conn->sem!=LLC_SYS_SEM_NULL) { \
      llC_sys_sem_free(__conn->sem); \
      __conn->sem=LLC_SYS_SEM_NULL; \
    } \
    __r=__conn->err; \
  } \
}
/*------------------------------------------------------------------------*/
#define netconn_close(r,__conn) \
block(netconn_close) { \
  struct api_msg *msg; \
 \
  dbg_putstr("netconn_close 0\r\n"); \
  if(__conn == NULL) { \
    r=ERR_VAL; \
    break(netconn_close); \
  } \
  if((msg = memp_malloc(MEMP_API_MSG)) == NULL) { \
    r=__conn->err = ERR_MEM; \
    break(netconn_close); \
  } \
 \
  dbg_putstr("netconn_close 1\r\n"); \
  __conn->state = NETCONN_CLOSE; \
  for(;;) { \
    group { \
      msg->type = API_MSG_CLOSE; \
      msg->msg.conn = __conn; \
      dbg_putstr("netconn_close 2\r\n"); \
    } \
    llC_api_msg_post(msg); \
    dbg_putstr("netconn_close 3 0x"); \
    dbg_puthex((unsigned long)&__conn->mbox); \
    dbg_putstr("\r\n"); \
    llC_sys_mbox_fetch(__conn->mbox, NULL); \
    dbg_putstr("netconn_close 4\r\n"); \
    if(__conn->err == ERR_MEM && \
       __conn->sem != LLC_SYS_SEM_NULL) { \
      dbg_putstr("netconn_close 5\r\n"); \
      llC_sys_sem_wait(__conn->sem); \
    } \
    else break; \
  } \
  group { \
    dbg_putstr("netconn_close 6\r\n"); \
    __conn->state = NETCONN_NONE; \
    memp_free(MEMP_API_MSG, msg); \
    r=__conn->err; \
  } \
}
/*------------------------------------------------------------------------*/

#endif /* __LWIP_API_H__ */
