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
 * $Id: httpd.c,v 1.1 2001/12/12 10:00:53 adam Exp $
 */

#include "lwip/debug.h"

#include "lwip/stats.h"

#include "httpd.h"

#include "lwip/tcp.h"

#include "fs.h"

struct http_state {
  char *file;
  u32_t left;
};

/*-----------------------------------------------------------------------------------*/
static void
conn_err(void *arg, err_t err)
{
  struct http_state *hs;

  hs = arg;
  mem_free(hs);
}
/*-----------------------------------------------------------------------------------*/
static void
close_conn(struct tcp_pcb *pcb, struct http_state *hs)
{
  tcp_arg(pcb, NULL);
  tcp_sent(pcb, NULL);
  tcp_recv(pcb, NULL);
  mem_free(hs);
  tcp_close(pcb);
}
/*-----------------------------------------------------------------------------------*/
static void
send_data(struct tcp_pcb *pcb, struct http_state *hs)
{
  err_t err;
  u16_t len;

  /* We cannot send more data than space avaliable in the send
     buffer. */     
  if(tcp_sndbuf(pcb) < hs->left) {
    len = tcp_sndbuf(pcb);
  } else {
    len = hs->left;
  }

  err = tcp_write(pcb, hs->file, len, 0);
  
  if(err == ERR_OK) {
    hs->file += len;
    hs->left -= len;
  }
}
/*-----------------------------------------------------------------------------------*/
static err_t
http_poll(void *arg, struct tcp_pcb *pcb)
{
  if(arg == NULL) {
    tcp_close(pcb);
  } else {
    send_data(pcb, (struct http_state *)arg);
  }

  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static err_t
http_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
  struct http_state *hs;

  hs = arg;

  if(hs->left > 0) {    
    send_data(pcb, hs);
  } else {
    close_conn(pcb, hs);
  }

  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static err_t
http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
  int i, j;
  char *data;
  char fname[40];
  struct fs_file file;
  struct http_state *hs;

  hs = arg;

  dbg_putstr("http_recv 0\r\n");
  if(err == ERR_OK && p != NULL) {

    dbg_putstr("http_recv 1\r\n");
    /* Inform TCP that we have taken the data. */
    tcp_recved(pcb, p->tot_len);
    
    dbg_putstr("http_recv 2\r\n");
    if(hs->file == NULL) {
      data = p->payload;
      
      dbg_putstr("http_recv 3\r\n");
      if(strncmp(data, "GET ", 4) == 0) {
        dbg_putstr("http_recv 4\r\n");
	for(i = 0; i < 40; i++) {
	  if(((char *)data + 4)[i] == ' ' ||
	     ((char *)data + 4)[i] == '\r' ||
	     ((char *)data + 4)[i] == '\n') {
	    ((char *)data + 4)[i] = 0;
	  }
	}
        dbg_putstr("http_recv 5\r\n");
	i = 0;
	do {
	  fname[i] = "/http"[i];
	  i++;
	} while(fname[i - 1] != 0 && i < 40);
        dbg_putstr("http_recv 6\r\n");
	i--;
	j = 0;
	do {
	  fname[i] = ((char *)data + 4)[j];
	  j++;
	  i++;
	} while(fname[i - 1] != 0 && i < 40);
        dbg_putstr("http_recv 7\r\n");
	pbuf_free(p);
        dbg_putstr("http_recv 8\r\n");
	
        dbg_putstr("http_recv 9\r\n");
	if(!fs_open(fname, &file)) {
          dbg_putstr("http_recv 10\r\n");
	  fs_open("/http/index.html", &file);
	}
	hs->file = file.data;
	hs->left = file.len;

        dbg_putstr("http_recv 11\r\n");
	send_data(pcb, hs);

        dbg_putstr("http_recv 12\r\n");
	/* Tell TCP that we wish be to informed of data that has been
	   successfully sent by a call to the http_sent() function. */
	tcp_sent(pcb, http_sent);
        dbg_putstr("http_recv 13\r\n");
      } else {
        dbg_putstr("http_recv 14\r\n");
	close_conn(pcb, hs);
      }
    } else {
      dbg_putstr("http_recv 15\r\n");
      pbuf_free(p);
    }
  }

  dbg_putstr("http_recv 16\r\n");
  if(err == ERR_OK && p == NULL) {
    dbg_putstr("http_recv 17\r\n");
    close_conn(pcb, hs);
  }
  dbg_putstr("http_recv 18\r\n");
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static err_t
http_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
  struct http_state *hs;

  dbg_putstr("http_accept 0\r\n");

  /* Allocate memory for the structure that holds the state of the
     connection. */
  hs = mem_malloc(sizeof(struct http_state));

  dbg_putstr("http_accept 1\r\n");
  if(hs == NULL) {
    dbg_putstr("http_accept: Out of memory\r\n");
    return ERR_MEM;
  }
  
  dbg_putstr("http_accept 2\r\n");
  /* Initialize the structure. */
  hs->file = NULL;
  hs->left = 0;

  /* Tell TCP that this is the structure we wish to be passed for our
     callbacks. */
  dbg_putstr("http_accept 3\r\n");
  tcp_arg(pcb, hs);

  dbg_putstr("http_accept 4\r\n");
  /* Tell TCP that we wish to be informed of incoming data by a call
     to the http_recv() function. */
  tcp_recv(pcb, http_recv);

  dbg_putstr("http_accept 5\r\n");
  tcp_err(pcb, conn_err);
  
  dbg_putstr("http_accept 6\r\n");
  tcp_poll(pcb, http_poll, 10);
  dbg_putstr("http_accept 7\r\n");
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
void
httpd_init(void)
{
  struct tcp_pcb *pcb;

  pcb = tcp_new();
  tcp_bind(pcb, IP_ADDR_ANY, 80);
  pcb = tcp_listen(pcb);
  tcp_accept(pcb, http_accept);
}
/*-----------------------------------------------------------------------------------*/
