/*--------------------------------------------------*/
/*
	gfork.c
*/
/*--------------------------------------------------*/
/*
 *
 * Copyright (C) 1992-2012 Hugo Delchini.
 *
 * This file is part of the MultitaskC project.
 *
 * Foobar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultitaskC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultitaskC.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*--------------------------------------------------*/

#include <stdio.h>
#include <structs.h>
#include <general.h>
#include <symset.h>

extern void
	SFree(),
	XFree(),
	(*EPrint)(),
	FatalMemErr();

extern char
	*XMalloc(),
	*Me;
	
extern int
#ifdef DEBUG
	DebugInfo,
	Trace,
#endif
	Results,
	DebugMode,
	Labz;

extern char
	*CodeEnd,
	*CodeBase,
	*CInst;

extern FILE
	*Output;

char
#ifdef DEBUG
#define LINELENGTH	80
	Line0[LINELENGTH],
	Line1[LINELENGTH],
	Line2[LINELENGTH],
	CurL[LINELENGTH],
#endif
	*EndStateAdd,
	*CState[CSTATESIZE],
	*TState[CSTATESIZE],
	*TS[CSTATESIZE+1],
	LFmt[LFMTSIZE];

int
	BrkIds[MAXBRKIDS],
	NFBTree,
	NFSList,
	NStates,
	NTrans,
	NElem,
	NInst,
	NextLabel=0;

SBtNodePt
	BrTreeStateSet,
	StateSet;

SBtNode
	EndBtNode;

SNode
	EndState;

static StkDecl(SBtNodePt *,SBTSTKSIZE)
	Stk;

/*-------------------------------------------------*/
#ifdef DEBUG

#define MAXFSIZE 128

SElemPt
	Blks[MAXFSIZE];

typedef struct {

	int
		l,
		o,
		x;

	SNodePt
		s;

} FElem;

int
	FIn,
	FOut,
	NFElem;

FElem 
	Fifo[MAXFSIZE];

#define F_Init()	(NFElem=FIn=FOut=0)
#define F_Empty()	(NFElem==0)
#define F_NEmpty()	(NFElem!=0)
#define F_Full()	(NFElem>=MAXFSIZE)
#define F_Put(x) 	(Fifo[FIn]=(x),FIn=(FIn>=(MAXFSIZE-1)?0:FIn+1),NFElem+=1)
#define F_Get(x) 	((x)=Fifo[FOut],FOut=(FOut>=(MAXFSIZE-1)?0:FOut+1),NFElem-=1)

/*-------------------------------------------------*/
void PutLine(l)
char
	l[];
{
int
	i;

	for(i=0;i<LINELENGTH;i++) fprintf(stdout,"%c",l[i]);
	fprintf(stdout,"\n");
}
/*--------------------------------------------------*/
void PrintState(s,sn,b,e)
char
	*b,
	*e;

int
	sn;

SNodePt
	s;
{
char
	*ptr;

int
	nn=0,
	cl=0,
	i;

SElemPt
	p;

FElem
	cur,
	ww;

	F_Init();
	cur.x=LINELENGTH/2;
	cur.o=LINELENGTH/4;
	cur.s=s;
	cur.l=cl;
	F_Put(cur);

	for(i=0;i<LINELENGTH;i++) Line0[i]=Line1[i]=Line2[i]=CurL[i]=' ';

	fprintf(stdout,b);

	do {

		F_Get(cur);

		if(cur.l!=cl) {
			cl=cur.l;
			PutLine(CurL);
			PutLine(Line0);
			PutLine(Line1);
			PutLine(Line2);
			for(i=0;i<LINELENGTH;i++) Line0[i]=Line1[i]=Line2[i]=CurL[i]=' ';
		}

		switch(cur.s->type) {

		case SN_LEAF :

			Blks[nn]=cur.s->blocks;
			ptr=&CurL[cur.x]; sprintf(&CurL[cur.x],"(%d,%x)",nn++,(int)IOff(cur.s->b.i));
			CurL[cur.x+strlen(ptr)]=' ';

			break;

		case SN_NODE :

			Blks[nn]=cur.s->blocks;
			ptr=&CurL[cur.x];
			sprintf(&CurL[cur.x],"(%d)",nn++);
			CurL[cur.x+strlen(ptr)]=' ';
			Line0[cur.x]='|';

			ww.l=cl+1; ww.x=cur.x-cur.o; ww.o=cur.o/2; ww.s=cur.s->b.s.l;
			for(i=ww.x;i<=cur.x;i++) Line1[i]='-';
			Line2[ww.x]='|';
			if(F_Full()) FatalMemErr("Fifo overflow in PrintState()");
			F_Put(ww);

			ww.l=cl+1; ww.x=cur.x+cur.o; ww.o=cur.o/2; ww.s=cur.s->b.s.r;
			for(i=cur.x;i<=ww.x;i++) Line1[i]='-';
			Line2[ww.x]='|';
			if(F_Full()) FatalMemErr("Fifo overflow in PrintState()");
			F_Put(ww);

			break;

		}
		
	} while(F_NEmpty());
	
	PutLine(CurL);
	if(sn>=0) { fprintf(stdout,"\nState Label : "); fprintf(stdout,LFmt,sn); }
	fprintf(stdout,"\nState Block(s) :\n");
	for(i=0;i<nn;i++) {
		fprintf(stdout," Block(s) of (%d)",i);
		for(p=Blks[i];p!=(SElemPt)NULL;p=p->n) fprintf(stdout,"-(%d,%x)",p->id,(int)p->lab);
		fprintf(stdout,"\n");
	}
	fprintf(stdout,e);
}
/*-------------------------------------------------*/
#endif
/*--------------------------------------------------*/
GenLabel(n,s)
int
	n;

SNodePt
	s;
{
extern char
	**GetCState();

char
	*cs[CSTATESIZE];

int
	nt,
	i;

	nt=0; GetCState(s,cs,&nt);
	fprintf(Output,LFmt,n);
	fprintf(Output," : /*");
	for(i=0;i<nt;i++) fprintf(Output," _%lx_",(long)IOff(cs[i]));
	fprintf(Output," */\n");
}
/*--------------------------------------------------*/
void GenGoto(lab)
int
	lab;
{
	fprintf(Output," goto ");
	fprintf(Output,LFmt,lab);
	fprintf(Output," ;\n");
}
/*--------------------------------------------------*/
void Indent(output,i)
FILE
	*output;

int
	i;
{
	while(i--) putc(' ',output);
}
/*-------------------------------------------------*/
SNodePt BuildOneLeaf(o)
Offset
	o;
{
SNodePt
	state;

	state=(SNodePt)XMalloc(sizeof(SNode));
	if(state==(SNodePt)NULL) FatalMemErr("malloc() error in BuildOneLeaf()");
	state->type=SN_LEAF;
	state->blocks=(SElemPt)NULL;
	state->b.i=IAdd(o);

	return(state);
}
/*-------------------------------------------------*/
SNodePt BuildInitState(o1,o2,isb,isbe)
Offset
	o1,
	o2;

int
	isb;

Offset
	isbe;
{
SNodePt
	state;

	state=(SNodePt)XMalloc(sizeof(SNode));
	if(state==(SNodePt)NULL) FatalMemErr("malloc1() error in BuildInitState()");
	state->type=SN_NODE;

	if(isb!=-1) {
		state->blocks=(SElemPt)XMalloc(sizeof(SElem));
		if(state->blocks==(SElemPt)NULL) FatalMemErr("malloc2() error in BuildInitState()");
		state->blocks->id=isb;
		state->blocks->lab=isbe;
		state->blocks->n=(SElemPt)NULL;
	}
	else state->blocks=(SElemPt)NULL;

	state->b.s.l=BuildOneLeaf(o1);
	state->b.s.r=BuildOneLeaf(o2);

	return(state);
}
/*--------------------------------------------------*/
void BSFree(r)
SElemPt
	r;
{
SElemPt
	f;

	while(r!=(SElemPt)NULL) { f=r; r=r->n; XFree(f,sizeof(*f)); }
}
/*--------------------------------------------------*/
SElemPt BSCopy(r)
SElemPt
	r;
{
SElemPt
	*c,
	head;

	head=NULL;
	c=&head;
	while(r!=(SElemPt)NULL) { 
		*c=(SElemPt)XMalloc(sizeof(SElem));
		if((*c)==(SElemPt)NULL) FatalMemErr("malloc() error in BSCopy()");
		(*c)->id=r->id;
		(*c)->lab=r->lab;
		(*c)->n=(SElemPt)NULL;
		c=&(*c)->n;
		r=r->n;
	}
	return(head);
}
/*--------------------------------------------------*/
SNodePt BuildState(model,ll,n)
SNodePt
	model;

char
	*ll[];

int
	*n;
{
SNodePt
	state;

	state=(SNodePt)XMalloc(sizeof(SNode));
	if(state==(SNodePt)NULL) FatalMemErr("malloc() error in BuildState()");
	state->type=model->type;
	state->blocks=BSCopy(model->blocks);

	switch(model->type) {

	case SN_LEAF :
		state->b.i=ll[*n];
		(*n)+=1;
		break;

	case SN_NODE :
		state->b.s.l=BuildState(model->b.s.l,ll,n);
		state->b.s.r=BuildState(model->b.s.r,ll,n);
		break;

	}

	return(state);
}
/*--------------------------------------------------*/
SNodePt BuildBtState(ll,n,c)
char
	*ll[];

int
	c,
	n;
{
SNodePt
	state;

	state=(SNodePt)XMalloc(sizeof(SNode));
	if(state==(SNodePt)NULL) FatalMemErr("malloc1() error in BuildBtState()");

	state->type=SN_NODE;
	state->blocks=(SElemPt)NULL;

	state->b.s.l=(SNodePt)XMalloc(sizeof(SNode));
	if(state->b.s.l==(SNodePt)NULL) FatalMemErr("malloc2() error in BuildBtState()");
	state->b.s.l->type=SN_LEAF;
	state->b.s.l->blocks=(SElemPt)NULL;
	state->b.s.l->b.i=ll[c];

	if(c<(n-2)) {
		state->b.s.r=BuildBtState(ll,n,c+1);
	}
	else {
		state->b.s.r=(SNodePt)XMalloc(sizeof(SNode));
		if(state->b.s.r==(SNodePt)NULL) FatalMemErr("malloc3() error in BuildBtState()");
		state->b.s.r->type=SN_LEAF;
		state->b.s.r->blocks=(SElemPt)NULL;
		state->b.s.r->b.i=ll[c+1];
	}

	return(state);
}
/*--------------------------------------------------*/
void BSPush(s,id,lab)
SElemPt
	*s;

int
	id;

Offset
	lab;
{
SElemPt
	n;

	n=(SElemPt)XMalloc(sizeof(SElem));
	if(n==(SElemPt)NULL) FatalMemErr("malloc() error in BSPush()");
	n->n=(*s);
	n->id=id;
	n->lab=lab;
	*s=n;
}
/*--------------------------------------------------*/
void BSPop(s)
SElemPt
	*s;
{
SElemPt
	f;

	f=(*s);
	(*s)=(*s)->n;
	XFree(f,sizeof(*f));
}
/*-------------------------------------------------*/
void FreeAllBS(state)
SNodePt
	state;
{
	BSFree(state->blocks);
	state->blocks=(SElemPt)NULL;

	switch(state->type) {

	case SN_LEAF :
		break;

	case SN_NODE :
		FreeAllBS(state->b.s.l);
		FreeAllBS(state->b.s.r);
		break;

	}
}
/*--------------------------------------------------*/
int SCmp(state1,state2)
SNodePt
	state1,
	state2;
{
int
	tmp;

	switch(state1->type) {
	case SN_LEAF :
		switch(state2->type) {
		case SN_LEAF : return(state1->b.i-state2->b.i);
		case SN_NODE : return(-1);
		}
		break;
	case SN_NODE :
		switch(state2->type) {
		case SN_LEAF : return(1);
		case SN_NODE :
			tmp=SCmp(state1->b.s.l,state2->b.s.l);
			if(tmp==0) return(SCmp(state1->b.s.r,state2->b.s.r));
			return(tmp);
		}
		break;
	}
}
/*------------------------------------------------*/
void Brk(s,id)
SNodePt
	s;

int
	id;
{
void
	SFree();

SElemPt
	c,
	f;

	c=s->blocks;

	switch(s->type) {
	case SN_LEAF :
		while(c!=(SElemPt)NULL) if(c->id==id) break; else c=c->n;
		if(c!=(SElemPt)NULL) {
			s->b.i=IAdd(c->lab);
			while(s->blocks!=c) { f=s->blocks; s->blocks=s->blocks->n; XFree(f,sizeof(*f)); }
			f=s->blocks;
			s->blocks=s->blocks->n;
			XFree(f,sizeof(*f));
		}
		break;

	case SN_NODE :
		while(c!=(SElemPt)NULL) if(c->id==id) break; else c=c->n;
		if(c!=(SElemPt)NULL) {
			SFree(s->b.s.l);
			SFree(s->b.s.r);
			s->type=SN_LEAF;
			s->b.i=IAdd(c->lab);
			while(s->blocks!=c) { f=s->blocks; s->blocks=s->blocks->n; XFree(f,sizeof(*f)); }
			f=s->blocks;
			s->blocks=s->blocks->n;
			XFree(f,sizeof(*f));
		}
		else {
			Brk(s->b.s.l,id);
			Brk(s->b.s.r,id);
		}
		break;
	}
}
/*------------------------------------------------*/
int CBrkList(s,cur)
SNodePt
	s;

int
	cur;
{
	switch(s->type) {
	case SN_LEAF :
		while(*s->b.i==IC_BBR || *s->b.i==Labiz(IC_BBR)) {
			if(cur>=MAXBRKIDS) {
				fprintf(stderr,"%s : BrkIds[] table overflow in CBrkList(), quit.\n",Me);
				LeaveApp(1);
			}
			SkipChr(s->b.i);
			GetIntAndMove(s->b.i,BrkIds[cur++]);
		}
		break;

	case SN_NODE :
		cur=CBrkList(s->b.s.l,cur);
		cur=CBrkList(s->b.s.r,cur);
		break;
	}

	return(cur);
}
/*------------------------------------------------*/
int BrkRed(state)
SNodePt
	state;
{
int
	nprog=0,
	i,
	n;

	while((n=CBrkList(state,0))!=0) { 
		nprog+=n;
		for(i=0;i<n;i++) {
			Brk(state,BrkIds[i]);
		}
	}

	return(nprog);
}
/*------------------------------------------------*/
char *ExtendReduce(state,nprog)
SNodePt
	state;

int
	*nprog;
{
char
	*r;

Inst
	IBuff;

	if(state->type==SN_NODE) {

	SExtendReduce :

		/* Extention/Reduction des deux fils. */
		if((r=ExtendReduce(state->b.s.l,nprog))!=NULL) return(r);
		if((r=ExtendReduce(state->b.s.r,nprog))!=NULL) return(r);

		/* Extention/Reduction du noeud courant. */
		switch(state->b.s.l->type) {
		case SN_LEAF :
			/* Si une feuille est un break, on reduit. */
			if(*(state->b.s.l->b.i)==IC_BRK || *(state->b.s.l->b.i)==Labiz(IC_BRK)) {
				/* Reduction du noeud courant. */
				*nprog+=1;
				GetChrAndMove(state->b.s.l->b.i,IBuff.h);
				GetOffAndMove(state->b.s.l->b.i,IBuff.b.o);
				state->type=SN_LEAF;
				SFree(state->b.s.l);
				SFree(state->b.s.r);
				state->b.i=IAdd(IBuff.b.o);
				goto LeafNode;
			}
			else {
				switch(state->b.s.r->type) {
				case SN_LEAF :
					/* Si une feuille est un break, on reduit. */
					if(*(state->b.s.r->b.i)==IC_BRK || *(state->b.s.r->b.i)==Labiz(IC_BRK)) {
						/* Reduction du noeud courant. */
						*nprog+=1;
						GetChrAndMove(state->b.s.r->b.i,IBuff.h);
						GetOffAndMove(state->b.s.r->b.i,IBuff.b.o);
						state->type=SN_LEAF;
						SFree(state->b.s.l);
						SFree(state->b.s.r);
						state->b.i=IAdd(IBuff.b.o);
						goto LeafNode;
					}
					else {
						/* Si les deux feuilles sont des halt, on reduit. */
						switch(*(state->b.s.l->b.i)) {
						case IC_HLT :
						case Labiz(IC_HLT) :
							switch(*(state->b.s.r->b.i)) {
							case IC_HLT :
							case Labiz(IC_HLT) :
								/* Reduction du noeud courant. */
								*nprog+=1;
								IBuff.b.o=(Offset)IOff(state->b.s.r->b.i);
								state->type=SN_LEAF;
								SFree(state->b.s.l);
								SFree(state->b.s.r);
								state->b.i=IAdd(IBuff.b.o+1);
								goto LeafNode;

							default :
								break;
							}
						default :
							break;
						}
					}
					break;
				case SN_NODE :
					break;
				}
			}

			break;

		case SN_NODE :

			switch(state->b.s.r->type) {
			case SN_LEAF :
				/* Si une feuille est un break, on reduit. */
				if(*(state->b.s.r->b.i)==IC_BRK || *(state->b.s.r->b.i)==Labiz(IC_BRK)) {
					/* Reduction du noeud courant. */
					*nprog+=1;
					GetChrAndMove(state->b.s.r->b.i,IBuff.h);
					GetOffAndMove(state->b.s.r->b.i,IBuff.b.o);
					state->type=SN_LEAF;
					SFree(state->b.s.l);
					SFree(state->b.s.r);
					state->b.i=IAdd(IBuff.b.o);
					goto LeafNode;
				}
				break;
			case SN_NODE :
				break;
			}

			break;

		}
	}
	else {

	LeafNode :

	if(state->b.i!=EndStateAdd) {

		LeafSwitch :

			switch(*state->b.i) {

			case IC_FRK :
			case Labiz(IC_FRK) :
				/* Creation d'un nouveau noeud. */
				*nprog+=1;
				GetChrAndMove(state->b.i,IBuff.h);
				GetOffAndMove(state->b.i,IBuff.b.f.o0);
				SkipInt(state->b.i);
				GetOffAndMove(state->b.i,IBuff.b.f.o1);
				SkipInt(state->b.i);
				IBuff.b.f.o1=(Offset)IOff(state->b.i);
				state->type=SN_NODE;
				state->b.s.l=BuildOneLeaf(IBuff.b.f.o1);
				state->b.s.r=BuildOneLeaf(IBuff.b.f.o0);

				/* Extention/Reduction des deux nouveaux fils. */
				goto SExtendReduce;

			case IC_BNG :
			case Labiz(IC_BNG) :
				do {
					*nprog+=1;
					GetChrAndMove(state->b.i,IBuff.h);
					GetIntAndMove(state->b.i,IBuff.b.bl.i0);
					GetOffAndMove(state->b.i,IBuff.b.bl.o0);
					BSPush(&state->blocks,IBuff.b.bl.i0,IBuff.b.bl.o0);
				} while(*state->b.i==IC_BNG || *state->b.i==Labiz(IC_BNG));
				goto LeafSwitch;

			case IC_BND :
			case Labiz(IC_BND) :
				do {
					*nprog+=1;
					GetChrAndMove(state->b.i,IBuff.h);
					BSPop(&state->blocks);
				} while(*state->b.i==IC_BND || *state->b.i==Labiz(IC_BND));
				goto LeafSwitch;

			case IC_RTN :
			case Labiz(IC_RTN) :
			case IC_ERT :
			case Labiz(IC_ERT) :
				*nprog+=1;
				return(state->b.i);

			}

		}
	}

	return(NULL);
}
/*--------------------------------------------------*/
void SCopy(d,s,n)
char
	*d[],
	*s[];

int
	n;
{
int
	i;

	for(i=0;i<n;i++) d[i]=s[i];
}
/*--------------------------------------------------*/
char **GetCState(state,csp,n)
SNodePt
	state;

char
	**csp;

int
	*n;
{
	switch(state->type) {

	case SN_LEAF :
		*csp++=state->b.i;
		if(*n<CSTATESIZE) (*n)+=1;
		else {
			fprintf(stderr,"%s : Fatal error, CState[] table overflow in GetCState(), quit.\n",Me);
			LeaveApp(1);
		}
		break;

	case SN_NODE :
		csp=GetCState(state->b.s.l,csp,n);
		csp=GetCState(state->b.s.r,csp,n);
		break;

	}

	return(csp);
}
/*-------------------------------------------------*/
char **SUpdate(state,leafs)
SNodePt
	state;

char
	**leafs;
{
	switch(state->type) {

	case SN_LEAF :
		state->b.i=*leafs++;
		break;

	case SN_NODE :
		leafs=SUpdate(state->b.s.l,leafs);
		leafs=SUpdate(state->b.s.r,leafs);
		break;

	}

	return(leafs);
}
/*--------------------------------------------------*/
int ResolvSync(state)
SBtNodePt
	state;
{
SyncPPt
	sp[CSTATESIZE];

char
	*cs[CSTATESIZE],
	*last[CSTATESIZE],
	*c;

int
	n,
	nprog,
	cnprog,
	i,
	size,
	nsp;

Offset
	beg;

	/* Recuperation des feuilles de l'etat courant. */
	n=0; GetCState(state,cs,&n);

	/* Recopie de l'etat original dans l'etat de travail. */
	SCopy(last,cs,n);

	/* On considere, au debut, qu'il n'y a pas de rendez-vous. */
	for(i=0;i<n;i++) sp[i]=(SyncPPt)NULL;

	nprog=0;

	FOREVER {

		cnprog=0;

		/* On cherche toutes les demandes de rendez-vous. */
		for(i=0,nsp=0;i<n;i++) {
			if(*last[i]==IC_JOI || *last[i]==Labiz(IC_JOI) || *last[i]==IC_WHE || *last[i]==Labiz(IC_WHE)) {
				c=last[i];
				SkipChr(c);
				GetSyn(c,sp[i]);
				nsp++;
			}
		}

		/* Si on en a pas trouve, c'est termine. */
		if(nsp==0) break;
	
		/* Initialisation des compteurs pour chaque rendez-vous. */
		for(i=0;i<n;i++) if(sp[i]!=(SyncPPt)NULL) sp[i]->n=0;

		/*
		  On a trouve au moins une demande de rendez-vous,
		  donc on compte le nombre de participant pour chaque rendez-vous.
		*/
		for(i=0;i<n;i++) if(sp[i]!=(SyncPPt)NULL) sp[i]->n++;
			
		/* Si le rendez-vous est realise, chaque participant progresse. */
		for(i=0;i<n;i++) {
			if(sp[i]!=(SyncPPt)NULL) {
				if(sp[i]->n>=sp[i]->card) {

					switch(*last[i]) {
					
					case IC_JOI :
					case Labiz(IC_JOI) :
						SkipChr(last[i]);
						SkipSyn(last[i]);
						cnprog+=1;
						break;

					case IC_WHE :
					case Labiz(IC_WHE) :
						SkipChr(last[i]);
						SkipSyn(last[i]);
						GetOffAndMove(last[i],beg);
						last[i]=IAdd(beg);
						cnprog+=1;
						break;

					}

				}
				else {

					if(*last[i]==IC_WHE || *last[i]==Labiz(IC_WHE)) {
						SkipChr(last[i]);
						SkipSyn(last[i]);
						SkipOff(last[i]);
						GetOffAndMove(last[i],beg);
						last[i]=IAdd(beg);
						cnprog+=1;
					}

				}
			}
		}

		/* Initialisation des compteurs pour chaque rendez-vous. */
		for(i=0;i<n;i++) if(sp[i]!=(SyncPPt)NULL) { sp[i]->n=0; sp[i]=(SyncPPt)NULL; }

		/* Si il n'y a eu aucune progression, c'est termine. */
		if(cnprog==0) break;

		/* On recommence apres avoir progresse. */
		SCopy(cs,last,n);
		nprog+=cnprog;

	}

	if(nprog) SUpdate(state,last);

	return(nprog);
}
/*------------------------------------------------*/
static void ExtRed(state)
SNodePt
	state;
{
int
	nmod;

char
	*r;

	if(state->type==SN_LEAF && state->b.i==EndStateAdd) return;

	do {

		nmod=0;

		r=ExtendReduce(state,&nmod);

		if(r!=NULL) {
			SFree(state->b.s.l);
			SFree(state->b.s.r);
			state->type=SN_LEAF;
			BSFree(state->blocks);
			state->blocks=(SElemPt)NULL;
			state->b.i=r;
			return;
		}

		if(state->type==SN_LEAF && state->b.i==EndStateAdd) return;

		nmod+=BrkRed(state);

		if(state->type==SN_LEAF && state->b.i==EndStateAdd) return;

		nmod+=ResolvSync(state);

	} while(nmod);
}
/*------------------------------------------------*/
int SF(state,bt,node)
SNodePt
	state;
	
SBtNodePt
	bt,
	*node;
{
int
	cmp;

	while(bt!=(SBtNodePt)NULL) {
		cmp=SCmp(state,bt->state);
		if(cmp==0) { *node=bt; return(1); }
		else if(cmp<0) bt=bt->l;
			 else bt=bt->r;
	}

	*node=(SBtNodePt)NULL;
	return(0);
}
/*------------------------------------------------*/
static int SFind(state,bt,node)
SNodePt
	state;
	
SBtNodePt
	bt,
	*node;
{
	/* Extention/Reduction de l'etat. */
	ExtRed(state);

	if(state->type==SN_LEAF && state->b.i==EndStateAdd) {
		if(EndBtNode.lab==ENDLABELNOTREACHED) EndBtNode.lab=NextLabel++;
		BSFree(state->blocks);
		state->blocks=NULL;
		*node=&EndBtNode;
		return(1);
	}

	return(SF(state,bt,node));
}
/*------------------------------------------------*/
SBtNodePt SInsert(state,bt,lab)
SNodePt
	state;
	
SBtNodePt
	*bt;

int
	lab;
{
char
	cnpos,
	unpos;

int
	cmp;

SBtNodePt
	ret,
	*cn,	/* Noeud courant. */
	un,		/* Oncle du noeud courant. */
	fa,		/* Pere du noeud courant. */
	*gfa;	/* Grand pere du noeud courant. */

	/* Recherche du point d'insertion eventuel. */
	StkInit(Stk,SBTSTKSIZE);
	cn=bt;
	while(*cn!=(SBtNodePt)NULL) {

		if(StkFull(Stk)) FatalMemErr("stack overflow in SInsert()");
		else StkPush(Stk,cn);

		cmp=SCmp(state,(*cn)->state);

		if(cmp==0) {
#ifdef DEBUG
			PrintState(
				state,
				-1,
				"Tried to insert :\n\n",
				"\n"
			);
#endif
			return(*cn);
		}
		else if(cmp<0) cn=(&(*cn)->l);
		     else cn=(&(*cn)->r);
	}

	/*
	   Ici on connait le point d'insertion et l'element n'existait pas,
	   donc on insere.
	*/
	if((*cn=(SBtNodePt)XMalloc(sizeof(SBtNode)))==(SBtNodePt)NULL) FatalMemErr("malloc() error in SInsert()");
	(*cn)->state=state;
	(*cn)->l=(*cn)->r=(SBtNodePt)NULL;
	(*cn)->color=WHITE;
	(*cn)->lab=lab;
	ret=(*cn);

	/* Reequilibrage de l'arbre. */
	FOREVER {
		/* Si pas de pere dans pile, cn==la racine donc on a fini. */
		if(StkEmpty(Stk)) { (*cn)->color=BLACK; break; }
		
		/* Recuperation du pere du noeud courant, s'il est noir, c'est fini. */
		fa=(*(StkPop(Stk)));
		if(fa->color==BLACK) break;
		
		/*
		   Si le noeud courant n'a pas de grand pere, c'est un fils
		   direct de la racine, donc on lui laisse sa couleur,
		   mais on a fini.
		*/
		if(StkEmpty(Stk)) break;
		
		/* Recuperation du grand pere du noeud courant, s'il est blanc, c'est fini. */
		gfa=StkPop(Stk);
		if((*gfa)->color==WHITE) break;
		
		/* On met a jour un pointeur vers l'oncle du noeud courant. */
		if((*gfa)->r==fa) { un=(*gfa)->l; unpos=LS; }
		else { un=(*gfa)->r; unpos=RS; }
	
		/* On determine si le noeud courant est fils gauche ou droit. */
		if(fa->r==(*cn)) cnpos=RS;
		else cnpos=LS;
		
		/*
		   8 cas differents maintenant. Mais de toutes facons,
		   le grand pere devient blanc.
		*/
		(*gfa)->color=WHITE;
		if(un!=(SBtNodePt)NULL && un->color==WHITE) {

			/* cas ou l'oncle est blanc. */

			/* cas alpha1, alpha2, alpha1', alpha2'. */
			un->color=fa->color=BLACK;

			/*
			   Ici on reboucle avec comme nouveau noeud courant,
			   le grand pere du noeud courant actuel.
			*/
			cn=gfa;
		}
		else {

			/* cas ou l'oncle est noir. */

			if(unpos==RS) {
				if(cnpos==LS) {
					/* cas beta1. */
					fa->color=BLACK;
					(*gfa)->l=fa->r;
					fa->r=(*gfa);
					(*gfa)=fa;
				}
				else {
					/* cas beta2. */
					(*cn)->color=BLACK;
					(*gfa)->l=(*cn)->r;
					(*cn)->r=(*gfa);
					(*gfa)=(*cn);
					fa->r=(*cn)->l;
					(*gfa)->l=fa;
				}
			}
			else {
				if(cnpos==LS) {
					/* cas beta2'. */
					(*cn)->color=BLACK;
					(*gfa)->r=(*cn)->l;
					(*cn)->l=(*gfa);
					(*gfa)=(*cn);
					fa->l=(*cn)->r;
					(*gfa)->r=fa;
				}
				else {
					/* cas beta1'. */
					fa->color=BLACK;
					(*gfa)->r=fa->l;
					fa->l=(*gfa);
					(*gfa)=fa;
				}
			}
			
			/* Ici on a forcement termine. */
			break;
		}
	}
	
	return(ret);
}
/*-------------------------------------------------*/
static int SLookUp(state,bt,node)
SNodePt
	state;
	
SBtNodePt
	*bt,
	*node;
{
	if(SFind(state,(*bt),node)) { return(1); }
	else { *node=SInsert(state,bt,NextLabel++); return(0); }
}
/*-------------------------------------------------*/
void SFree(state)
SNodePt
	state;
{
	BSFree(state->blocks);
	state->blocks=(SElemPt)NULL;

	switch(state->type) {

	case SN_LEAF :
		break;

	case SN_NODE :
		SFree(state->b.s.l);
		SFree(state->b.s.r);
		break;

	}

	XFree(state,sizeof(*state));
}
/*-------------------------------------------------*/
void SBtFree(btroot)
SBtNodePt
	btroot;
{
	if(btroot!=(SBtNodePt)NULL) {
		SBtFree(btroot->l);
		SBtFree(btroot->r);
		SFree(btroot->state);
		XFree(btroot,sizeof(*btroot));
	}
}
/*--------------------------------------------------*/
void GenReturn(output,inst)
FILE
	*output;

char
	*inst;
{
Inst
	IBuff;

	switch(*inst) {

	case IC_RTN :
	case Labiz(IC_RTN) :
		fprintf(output," return ;\n");
		break;

	case IC_ERT :
	case Labiz(IC_ERT) :
		SkipChr(inst);
		GetOffAndMove(inst,IBuff.b.e.beg);
		GetIntAndMove(inst,IBuff.b.e.size);
		fprintf(output," return ( ");
		EPrint(output,IBuff.b.e.beg,IBuff.b.e.size,DRET);
		fprintf(output," ) ;\n");
		break;

	default :
		fprintf(stderr,"%s : bad instruction code in GenReturn(), quit.\n",Me);
		LeaveApp(1);

	}
}
/*--------------------------------------------------*/
SListElemPt NewSListElem(sbtn)
SBtNodePt
	sbtn;
{
SListElemPt
	elem;

	elem=(SListElemPt)XMalloc(sizeof(SListElem));
	if(elem==(SListElemPt)NULL) FatalMemErr("malloc() error in NewSListElem()");
	elem->sbtn=sbtn;
	elem->n=(SListElemPt)NULL;
	return(elem);
}
/*--------------------------------------------------*/
void PLCopy(d,s,n)
char
	**d,
	**s;

int
	n;
{
	while(n--) *d++=*s++;
}
/*--------------------------------------------------*/
static SListElemPt *GenBranchTree(slip,model,modlab,cs,cur,n,ind)
SListElemPt
	*slip;

SNodePt
	model;

char
	*cs[];

int
	modlab,
	cur,
	n,
	ind;
{
int
	cslab;

SBtNodePt
	rep;

SNodePt
	istate;

	if(cur<n) { /* On est pas au bout de la liste d'instructions. */

	char
		*c;

		c=cs[cur];

		switch(*c) {

		case IC_RTN :
		case IC_ERT :
		case IC_EXP :
		case IC_GTO :
		case IC_FRK :
		case IC_BRK :
		case IC_WHE :
		case IC_BBR :
		case IC_BNG :
		case IC_BND :
		case IC_HLT :
		case IC_JOI :
		case Labiz(IC_RTN) :
		case Labiz(IC_ERT) :
		case Labiz(IC_EXP) :
		case Labiz(IC_GTO) :
		case Labiz(IC_FRK) :
		case Labiz(IC_BRK) :
		case Labiz(IC_WHE) :
		case Labiz(IC_BBR) :
		case Labiz(IC_BNG) :
		case Labiz(IC_BND) :
		case Labiz(IC_HLT) :
		case Labiz(IC_JOI) :
			TState[cur]=c;
			slip=GenBranchTree(slip,model,modlab,cs,cur+1,n,ind);
			break;

		case IC_IFG :
		case Labiz(IC_IFG) :
		case IC_SWH :
		case Labiz(IC_SWH) : {
			int
				ninst;

			Inst
				IBuff;

#ifdef BTFACT
				PLCopy(TS,TState,cur); PLCopy(&TS[cur],&cs[cur],n-cur);

				cslab=0; istate=BuildState(model,TS,&cslab);

				ExtRed(istate);

				if(istate->type==SN_LEAF) {
					Indent(Output,ind);
					if(istate->b.i==EndStateAdd) {
						if(EndBtNode.lab==ENDLABELNOTREACHED) EndBtNode.lab=NextLabel++;
#ifdef DEBUG
						if(Trace)
							PrintState(
								istate,
								EndBtNode.lab,
								"------- End State Reached in Branch Tree Factorization : \n\n",
								"\n"
							);
#endif
						GenGoto(EndBtNode.lab);
						SFree(istate);
						return(slip);
					}
					else {
						if(SF(istate,StateSet,&rep)) {
#ifdef DEBUG
							if(Trace)
								PrintState(
									istate,
									rep->lab,
									"------- Existing Return State Reached in Branch Tree Factorization : \n\n",
									"\n"
								);
#endif
							SFree(istate);
							GenGoto(rep->lab);
						}
						else {
#ifdef DEBUG
							if(Trace)
								PrintState(
									istate,
									NextLabel,
									"------- New Return State Reached in Branch Tree Factorization : \n\n",
									"\n"
								);
#endif
							GenLabel(NextLabel,istate);
							Indent(Output,ind); GenReturn(Output,istate->b.i);
							SInsert(istate,&StateSet,NextLabel++);
						}
						return(slip);
					}
				}

				ninst=0; GetCState(istate,TS,&ninst);
				SFree(istate);

				cslab=0; while(cslab<ninst) if(TS[cslab]==cs[cur]) break; else cslab+=1;
				cslab=(cslab>=ninst?(-1):cslab);

				TS[ninst]=(char *)cslab; ninst+=1;
				istate=BuildBtState(TS,ninst,0);

				if(SF(istate,BrTreeStateSet,&rep)) {
					NFBTree+=1;
					Indent(Output,ind);
					fprintf(Output," goto ");
					fprintf(Output,LFmt,rep->lab);
					fprintf(Output," ;\n");
					SFree(istate);
					return(slip);
				}
				else {
					cslab=NextLabel++;
					Indent(Output,ind);
					GenLabel(cslab,istate);
				}
#endif

				/* Recuperation de l'instruction. */
				GetChrAndMove(c,IBuff.h);

				switch(IBuff.h) {

				case IC_IFG :
				case Labiz(IC_IFG) : /* Un simple if(). */
					GetOffAndMove(c,IBuff.b.i.o0); GetOffAndMove(c,IBuff.b.i.e0.beg); GetIntAndMove(c,IBuff.b.i.e0.size);
					Indent(Output,ind); fprintf(Output," if("); EPrint(Output,IBuff.b.i.e0.beg,IBuff.b.i.e0.size,DTEST); fprintf(Output,")\n");
					/* construction de la suite de l'etat pour la branche "then". */
					TState[cur]=IAdd(IBuff.b.i.o0);
					slip=GenBranchTree(slip,model,modlab,cs,cur+1,n,ind+1);
					Indent(Output,ind);
					fprintf(Output," else\n");
					/* construction de la suite de l'etat pour la branche "else". */
					TState[cur]=c;
					slip=GenBranchTree(slip,model,modlab,cs,cur+1,n,ind+1);
					break;

				case IC_SWH :
				case Labiz(IC_SWH) : { /* Un switch(). */
					char h; int size; Offset o;
					GetIntAndMove(c,IBuff.b.s.i0); GetOffAndMove(c,IBuff.b.s.e0.beg); GetIntAndMove(c,IBuff.b.s.e0.size);
					Indent(Output,ind); fprintf(Output," switch ("); EPrint(Output,IBuff.b.s.e0.beg,IBuff.b.s.e0.size,DEXP); fprintf(Output,") {\n");
					/* construction de la suite de l'etat pour toutes les branches. */
					while(IBuff.b.s.i0--) {
						GetChrAndMove(c,h); GetOffAndMove(c,o); GetIntAndMove(c,size);
						Indent(Output,ind);
						if(o!=NULLOFF) { fprintf(Output," case ("); EPrint(Output,o,size,DEXP); fprintf(Output,") :\n"); }
						else { fprintf(Output," default :\n"); }
						GetOffAndMove(c,o);
						if(o!=NULLOFF) {
							TState[cur]=IAdd(o);
							slip=GenBranchTree(slip,model,modlab,cs,cur+1,n,ind+1);
						}
					}
					Indent(Output,ind);
					fprintf(Output," }\n");
					}
					break;

				}

#ifdef BTFACT
				SInsert(istate,&BrTreeStateSet,cslab);
#endif
			}
			break;

		default : {
			char
				h;

				GetChr(c,h);
				fprintf(stderr,"%s : spurious instruction <0x%x> in GenBranchTree(), quit.\n",Me,h);
				LeaveApp(0);
			}
		}

	}
	else { /* On est au bout de la liste d'instructions. */

		cslab=0; istate=BuildState(model,TState,&cslab);

#ifdef DEBUG
	if(Trace)
		PrintState(
			istate,
			-1,
			"------- State Reached : \n\n",
			"\n"
		);
#endif

		cslab=SLookUp(istate,&StateSet,&rep);

#ifdef DEBUG
	if(Trace)
		PrintState(
			rep->state,
			(int)rep->lab,
			"After Ext/Red and SLookUp() in GenBranchTree() : \n\n",
			"\n"
		);
#endif

		Indent(Output,ind);
		NTrans+=1;

		if(cslab!=0) { SFree(istate); GenGoto(rep->lab); }
		else {
			if(rep->state->type==SN_LEAF) {
				GenLabel(rep->lab,rep->state);
				Indent(Output,ind); GenReturn(Output,rep->state->b.i);
			}
			else {
				GenGoto(rep->lab);
				*slip=NewSListElem(rep); slip=(&((*slip)->n)); NElem+=1;
			}
		}
	}

	return(slip);
}
/*--------------------------------------------------*/
static void GenState(sbtn)
SBtNodePt
	sbtn;
{
int
	nprog,
	i;

Inst
	IBuff;

SBtNodePt
	rep;

SNodePt
	istate;

SListElemPt
	ElToFree,
	SList;

	GenStateBeg:

#ifdef DEBUG
	if(Trace)
		PrintState(
			sbtn->state,
			(int)sbtn->lab,
			"---------------------------------- GenState() ----------------------------------\n\n",
			"\n"
		);
#endif

	/* Generation de l'etiquette de l'etat. */
	GenLabel((int)sbtn->lab,sbtn->state);

	/* Generation des actions de l'etat. */
	FOREVER {

		/* Recuperation de l'adresse de chaque instruction. */
		NInst=0;
		GetCState(sbtn->state,CState,&NInst);

		/* Pas de progression pour l'instant. */
		nprog=0;

		/* On boucle sur toutes les instructions. */
		for(i=0;i<NInst;i++) {
	
			/* Passage des instructions. */
			if(*CState[i]==IC_EXP || *CState[i]==Labiz(IC_EXP)) {
				GetChrAndMove(CState[i],IBuff.h); GetOffAndMove(CState[i],IBuff.b.e.beg); GetIntAndMove(CState[i],IBuff.b.e.size);
				fprintf(Output," "); EPrint(Output,IBuff.b.e.beg,IBuff.b.e.size,DEXP); fprintf(Output," ;\n");
				nprog+=1;
			}
	
		}

		/* Si il y a eu au moins une progression. */
		if(nprog) {

			i=0; istate=BuildState(sbtn->state,CState,&i);

#ifdef DEBUG
			if(Trace)
				PrintState(
					istate,
					-1,
					"------- Intermediate State Reached : \n\n",
					"\n"
				);
#endif

			/* On note l'etat intermediaire rejoint. */
			i=SLookUp(istate,&StateSet,&rep);

#ifdef DEBUG
			if(Trace)
				PrintState(
					rep->state,
					(int)rep->lab,
					"After Ext/Red and SLookUp() in GenState() : \n\n",
					"\n"
				);
#endif

			if(i) {

				/* On a atteind un etat qui a deja ete genere, on le rejoint. */
				SFree(istate);
				NFSList+=1;
				GenGoto(rep->lab);
				return;

			}
			else {

				GenLabel((int)rep->lab,rep->state);

				/* Si on a atteind un "return", c'est termine. */
				if(rep->state->type==SN_LEAF) { GenReturn(Output,rep->state->b.i); return; }
				else {
					/* sinon, on recommence avec un nouvel etat courant. */
					FreeAllBS(sbtn->state);
					sbtn=rep;
				}

			}

		}
		else break; /* Pas de progression. */

	} /* FOREVER */


	nprog=0;

	/* On boucle sur toutes les instructions. */
	for(i=0;i<NInst;i++) {

		/* Passage des branchements imperatifs. */
		if(*CState[i]==IC_GTO || *CState[i]==Labiz(IC_GTO)) {
			GetChrAndMove(CState[i],IBuff.h); GetOffAndMove(CState[i],IBuff.b.o); CState[i]=IAdd(IBuff.b.o);
			nprog+=1;
		}

	}

	/* Si il y a eu que des progressions. */
	if(nprog==NInst) {

		i=0; istate=BuildState(sbtn->state,CState,&i);

#ifdef DEBUG
		if(Trace)
			PrintState(
				istate,
				-1,
				"------- Intermediate State Reached : \n\n",
				"\n"
			);
#endif

		/* On note l'etat intermediaire rejoint. */
		i=SLookUp(istate,&StateSet,&rep);

#ifdef DEBUG
		if(Trace)
			PrintState(
				rep->state,
				(int)rep->lab,
				"After Ext/Red and SLookUp() in GenState() : \n\n",
				"\n"
			);
#endif

		NTrans+=1;

		if(i) {

			/* On a atteind un etat qui a deja ete genere, on le rejoint. */
			SFree(istate);
			GenGoto(rep->lab);
			return;

		}
		else {

			NStates+=1;

			/* Si on a atteind un "return", c'est termine. */
			if(rep->state->type==SN_LEAF) { 
				GenLabel((int)rep->lab,rep->state);
				GenReturn(Output,rep->state->b.i);
				return;
			}
			else {
				/* sinon, on recommence avec un nouvel etat courant. */
				FreeAllBS(sbtn->state);
				sbtn=rep;
				goto GenStateBeg;
			}

		}

	}

	/* Generation de l'arbre de branchements de l'etat. */
	/* Toutes les instructions expressions (IC_EXP ou Labiz(IC_EXP)) on ete generees. */
	SList=(SListElemPt)NULL; NElem=0;
	GenBranchTree(&SList,sbtn->state,sbtn->lab,CState,0,NInst,0);

	/* Liberation des piles de blocs de l'etat courant. */
	FreeAllBS(sbtn->state);

	/* Generation des etats rejoint par l'etat. */
	while(SList!=(SListElemPt)NULL) {
		NStates+=1;
		GenState(SList->sbtn);
		ElToFree=SList;
		SList=SList->n;
		XFree(ElToFree,sizeof(*ElToFree));
	}
}
/*--------------------------------------------------*/
void GenGFork(o1,o2,h,isb,isbe,esa)
Offset
	o1,
	o2;

int
	h,
	isb;

Offset
	isbe;

char
	*esa;
{
SBtNodePt
	rep;

SNodePt
	state;

	NFBTree=0;
	NFSList=0;
	NTrans=0;
	NStates=1;

	/* Initialisation de l'ensemble des etats. */
	BrTreeStateSet=(SBtNodePt)NULL;
	StateSet=(SBtNodePt)NULL;

	/* Initialisation du numero d'etiquettes. */
	NextLabel=0;

	/* L'etat terminal n'a pas encore ete rejoint. */
	EndStateAdd=esa;
	EndBtNode.color=WHITE;
	EndBtNode.lab=ENDLABELNOTREACHED;
	EndBtNode.state=&EndState;
	EndState.type=SN_LEAF;
	EndState.blocks=NULL;
	EndState.b.i=EndStateAdd;
	EndBtNode.l=EndBtNode.r=NULL;

	/* Initialisation du format d'impression des etiquettes. */
	sprintf(LFmt,"_%x%%x",h);

	/* Construction de l'etat initial. */
	state=BuildInitState(o1,o2,isb,isbe);

	/* Insertion de l'etat initial dans l'ensemble des etats. */
	if(!SLookUp(state,&StateSet,&rep)) {
		if(rep->state->type==SN_LEAF) GenReturn(Output,rep->state->b.i);
		else GenState(rep);
	}

	/* Generation de l'etiquette de l'etat terminal. */
	if(EndBtNode.lab!=ENDLABELNOTREACHED) {
		fprintf(Output,LFmt,EndBtNode.lab); fprintf(Output," : /* _%lx_ */\n",(long)IOff(esa));
		NStates+=1;
	}

	/* Liberation de l'ensemble des etats. */
	SBtFree(BrTreeStateSet);
	SBtFree(StateSet);

#ifdef DEBUG
	if(Trace) fprintf(stdout,"--------------------------------------------------------------------------------\n\n");
#endif

	if(Results) {
		fprintf(stdout,"%s, results :\n %d state(s), %d transition(s).\n",Me,NStates,NTrans);
		fprintf(stdout," %d branch tree factorization(s).\n",NFBTree);
		fprintf(stdout," %d expression list factorization(s).\n",NFSList);
	}
	
}
/*--------------------------------------------------*/
