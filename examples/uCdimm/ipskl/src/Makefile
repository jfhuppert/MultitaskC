# Copyright (c) 2001, Swedish Institute of Computer Science.
# All rights reserved. 
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions 
# are met: 
#
# 1. Redistributions of source code must retain the above copyright 
#    notice, this list of conditions and the following disclaimer. 
# 2. Redistributions in binary form must reproduce the above copyright 
#    notice, this list of conditions and the following disclaimer in the 
#    documentation and/or other materials provided with the distribution. 
# 3. Neither the name of the Institute nor the names of its contributors 
#    may be used to endorse or promote products derived from this software 
#    without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
# ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
# SUCH DAMAGE. 
#
# This file is part of the lwIP TCP/IP stack.
# 
# Author: Adam Dunkels <adam@sics.se>
#
# $Id: Makefile,v 1.1 2001/12/12 10:00:53 adam Exp $

CCDEP=m68k-elf-gcc
CC=m68k-elf-gcc
AR=m68k-elf-ar
CFLAGS=-Wall -m68000 -DIPv4 -fpack-struct -O2 -fomit-frame-pointer -fno-strength-reduce -pipe
ARCH=llC
ARFLAGS=rs
LWIPDIR=../lwipsrc

CFLAGS4:=$(CFLAGS) -I$(LWIPDIR)/include -I$(LWIPDIR)/arch/llC/include -I$(LWIPDIR)/include/ipv4 -I.

%.o:
	$(CC) $(CFLAGS4) -c $(<:.o=.c)

ACFILES=$(wildcard $(LWIPDIR)/arch/llC/*.c $(LWIPDIR)/arch/llC/netif/*.c)

DACOBJECTS=$(ACFILES:.c=.o)
ACOBJECTS=$(notdir $(DACOBJECTS))

LIB=liblwip4llC.a

all ipv4 compile: main
.PHONY: all

clean:
	rm -f *.o $(LIB4) $(LIB) main *.s .depend* *.core core

depend dep: .depend .depend.llC

include .depend.llC
include Makefile.lwip

$(LIB): $(ACOBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $?

# $(CC) $(CFLAGS4) -o main main.o $(LIB) $(LIB4)
main: .depend.llC $(LIB4) main.o $(LIB)
	touch main

.depend.llC: main.c $(ACFILES)
	$(CCDEP) $(CFLAGS4) -MM $^ > .depend.llC || rm -f .depend.llC
