#------------------------------------------------------------------------------------------------------------

# Copyright (C) 1992-2013 Hugo Delchini.

# This file is part of the MultitaskC project.

# MultitaskC is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# MultitaskC is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with MultitaskC.  If not, see <http://www.gnu.org/licenses/>.

CC=gcc
LDFLAGS=-m32
# LDFLAGS=

BTFACT=-DBTFACT
#BTFACT=

HOME=..
IDIR=../include
ODIR=../objs
BIN=$(HOME)/bin

OBJS=$(ODIR)/main.o $(ODIR)/synan.o $(ODIR)/lexan.o \
$(ODIR)/codegen.o $(ODIR)/labels.o $(ODIR)/ptree.o \
$(ODIR)/auto.o $(ODIR)/gfork.o $(ODIR)/sfork.o $(ODIR)/ifork.o $(ODIR)/erstr.o \
$(ODIR)/pcsynan.o $(ODIR)/symset.o $(ODIR)/pclexan.o

#CFLAGS= -I$(IDIR) -O2 $(BTFACT) -fno-builtin
CFLAGS= -I$(IDIR) -O2 $(BTFACT) -fno-builtin -m32
#CFLAGS= -I$(IDIR) -g $(BTFACT)
#CFLAGS= -I$(IDIR)

DEBUG=-DYYDEBUG -DDEBUG
#DEBUG=

#------------------------------------------------------------------------------------------------------------

$(BIN)/mtcc : $(OBJS)
	@echo "linking mtcc ..."
	@$(CC) $(DEBUG) -g -o $(BIN)/mtcc $(OBJS) $(LDFLAGS)

#------------------------------------------------------------------------------------------------------------

$(ODIR)/main.o : main.c $(IDIR)/general.h $(IDIR)/symset.h $(IDIR)/structs.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c main.c $(DEBUG)
	@mv main.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(ODIR)/lexan.o : lexan.c $(IDIR)/structs.h $(IDIR)/synan.h $(IDIR)/general.h $(IDIR)/symset.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c lexan.c $(DEBUG)
	@mv lexan.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

lexan.c : lexan.l
	flex lexan.l
	@mv lex.yy.c lexan.c

#------------------------------------------------------------------------------------------------------------

$(ODIR)/synan.o : synan.c $(IDIR)/general.h $(IDIR)/structs.h $(IDIR)/symset.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c synan.c $(DEBUG)
	@mv synan.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(IDIR)/synan.h : synan.y
	bison -y -d synan.y
	@mv y.tab.h $(IDIR)/synan.h
	@mv y.tab.c synan.c

synan.c : synan.y
	bison -y -d synan.y
	@mv y.tab.h $(IDIR)/synan.h
	@mv y.tab.c synan.c

#------------------------------------------------------------------------------------------------------------

$(ODIR)/pclexan.o : pclexan.c $(IDIR)/pcsynan.h $(IDIR)/general.h $(IDIR)/symset.h $(IDIR)/structs.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c pclexan.c $(DEBUG)
	@mv pclexan.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

pclexan.c : pclexan.l
	flex -Ppcyy pclexan.l
	@mv lex.pcyy.c pclexan.c

#------------------------------------------------------------------------------------------------------------

$(ODIR)/pcsynan.o : pcsynan.c $(IDIR)/general.h $(IDIR)/symset.h $(IDIR)/structs.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c pcsynan.c $(DEBUG)
	@mv pcsynan.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(IDIR)/pcsynan.h : pcsynan.y
	bison -b pcyy -p pcyy -d pcsynan.y
	@mv pcyy.tab.h $(IDIR)/pcsynan.h
	@mv pcyy.tab.c pcsynan.c

pcsynan.c : pcsynan.y
	bison -b pcyy -p pcyy -d pcsynan.y
	@mv pcyy.tab.h $(IDIR)/pcsynan.h
	@mv pcyy.tab.c pcsynan.c

#------------------------------------------------------------------------------------------------------------

$(ODIR)/auto.o : auto.c $(IDIR)/general.h $(IDIR)/structs.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c auto.c $(DEBUG)
	@mv auto.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(ODIR)/symset.o : symset.c $(IDIR)/symset.h $(IDIR)/structs.h $(IDIR)/general.h $(IDIR)/icsn.h $(IDIR)/pcsynan.h
	$(CC) $(CFLAGS) -c symset.c $(DEBUG)
	@mv symset.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(ODIR)/codegen.o : codegen.c $(IDIR)/general.h $(IDIR)/structs.h $(IDIR)/symset.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c codegen.c $(DEBUG)
	@mv codegen.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(ODIR)/labels.o : labels.c $(IDIR)/general.h $(IDIR)/structs.h $(IDIR)/symset.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c labels.c $(DEBUG)
	@mv labels.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(ODIR)/ptree.o : ptree.c $(IDIR)/general.h $(IDIR)/structs.h $(IDIR)/symset.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c ptree.c $(DEBUG)
	@mv ptree.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(ODIR)/gfork.o : gfork.c $(IDIR)/general.h $(IDIR)/structs.h $(IDIR)/symset.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c gfork.c $(DEBUG)
	@mv gfork.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(ODIR)/sfork.o : sfork.c $(IDIR)/general.h $(IDIR)/structs.h $(IDIR)/symset.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c sfork.c $(DEBUG)
	@mv sfork.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(ODIR)/ifork.o : ifork.c $(IDIR)/general.h $(IDIR)/structs.h $(IDIR)/symset.h $(IDIR)/icsn.h
	$(CC) $(CFLAGS) -c ifork.c $(DEBUG)
	@mv ifork.o $(ODIR)

#------------------------------------------------------------------------------------------------------------

$(ODIR)/erstr.o : erstr.c
	$(CC) $(CFLAGS) -c erstr.c $(DEBUG)
	@mv erstr.o $(ODIR)

erstr.c : extred.c $(IDIR)/icsn.h $(IDIR)/general.h
	gcc -E -I$(IDIR) -P extred.c > tmp0.c
	awk 'BEGIN { printf("char ExtRedFStr[]=\"\\\n") } { if(NF) printf("%s\\n\\\n",$$0) } END { printf("\";\n") }' tmp0.c > erstr.c
	@rm tmp0.c

#------------------------------------------------------------------------------------------------------------
clean :
	@-rm -f ./synan.c ./lexan.c ./erstr.c ./pcsynan.c ./pclexan.c $(BIN)/mtcc
	@-( cd $(IDIR) ; make clean )
	@-( cd $(ODIR) ; rm -f *.o )
#------------------------------------------------------------------------------------------------------------
