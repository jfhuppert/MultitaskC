/*--------------------------------------------------*/
/*
	ptree.c
*/
/*--------------------------------------------------*/
/*
 *
 * Copyright (C) 1992-2012 Hugo Delchini.
 *
 * This file is part of the MultitaskC project.
 *
 * MultitaskC is free software: you can redistribute it and/or modify
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
#include <symset.h>
#include <general.h>

extern void
	LeaveApp(),
	SetOffset();

extern char
	*Me;

#ifdef DEBUG
extern int
	DebugInfo;
#endif

extern Offset
	GetOffset();

extern FILE
	*ptreef;

PTNode
	ptn={{NULLOFF,0}};

/*--------------------------------------------------*/
void PutPTNode(se,type)
SeqPt
	se;

char
	type;
{
	ptn.h.type=type;
	SetOffset(ptreef,NULLOFF,F_END);
	se->first=se->last=GetOffset(ptreef);
	WOff(ptn.h.next,ptreef);
	WChr(ptn.h.type,ptreef);

	switch(type) {

	case T_BREAK : case T_CONTINUE : case T_RET0 :
		break;

	case T_BLBRK :
		WInt(ptn.b.b,ptreef);
		break;

	case T_JOIN :
		WSynP(ptn.b.s,ptreef);
		break;

	case T_WHEN :
		WSynP(ptn.b._1s2o.s,ptreef);
		WOff(ptn.b._1s2o.o0,ptreef);
		WOff(ptn.b._1s2o.o1,ptreef);
		break;

	case T_EXPRESSION : case T_RET1 :
		WOff(ptn.b.e.beg,ptreef); WInt(ptn.b.e.size,ptreef);
		break;

	case T_GOTO :
		WLabP(ptn.b.l,ptreef);
		break;

	case T_LABEL :
		WLabP(ptn.b._1o1l.l0,ptreef);
		WOff(ptn.b._1o1l.o0,ptreef);
		break;

	case T_BLOCK :
		WInt(ptn.b._1o1i.i0,ptreef);
		WOff(ptn.b._1o1i.o0,ptreef);
		break;

	case T_FOR0 :
		WOff(ptn.b.o,ptreef);
		break;

	case T_FOR1 : case T_FOR2 : case T_FOR3 :
	case T_WHILE : case T_DO : case T_IF0 :
	case T_CASE :
		WOff(ptn.b._1o1e.e0.beg,ptreef);
		WInt(ptn.b._1o1e.e0.size,ptreef);
		WOff(ptn.b._1o1e.o0,ptreef);
		break;

	case T_SWITCH :
		WInt(ptn.b._1o1i1e.i0,ptreef);
		WOff(ptn.b._1o1i1e.e0.beg,ptreef);
		WInt(ptn.b._1o1i1e.e0.size,ptreef);
		WOff(ptn.b._1o1i1e.o0,ptreef);
		break;

	case T_FOR4 : case T_FOR5 : case T_FOR6 :
		WOff(ptn.b._1o2e.e0.beg,ptreef);
		WInt(ptn.b._1o2e.e0.size,ptreef);
		WOff(ptn.b._1o2e.e1.beg,ptreef);
		WInt(ptn.b._1o2e.e1.size,ptreef);
		WOff(ptn.b._1o2e.o0,ptreef);
		break;

	case T_FOR7 :
		WOff(ptn.b._1o3e.e0.beg,ptreef);
		WInt(ptn.b._1o3e.e0.size,ptreef);
		WOff(ptn.b._1o3e.e1.beg,ptreef);
		WInt(ptn.b._1o3e.e1.size,ptreef);
		WOff(ptn.b._1o3e.e2.beg,ptreef);
		WInt(ptn.b._1o3e.e2.size,ptreef);
		WOff(ptn.b._1o3e.o0,ptreef);
		break;

	case T_CEXEC :
		WOff(ptn.b._2o0e.o0,ptreef);
		WOff(ptn.b._2o0e.o1,ptreef);
		break;

	case T_IF1 :
		WOff(ptn.b._2o1e.e0.beg,ptreef);
		WInt(ptn.b._2o1e.e0.size,ptreef);
		WOff(ptn.b._2o1e.o0,ptreef);
		WOff(ptn.b._2o1e.o1,ptreef);
		break;

	}
}
/*--------------------------------------------------*/
void MkExprNode(se,beg,size)
SeqPt
	se;

Offset
	beg;

int
	size;
{
	ptn.b.e.beg=beg;
	ptn.b.e.size=size;
	PutPTNode(se,T_EXPRESSION);
}
/*--------------------------------------------------*/
void MkConcNode(se,se1,se2)
SeqPt
	se,
	se1,
	se2;
{
	ptn.b._2o0e.o0=se1->first;
	ptn.b._2o0e.o1=se2->first;
	PutPTNode(se,T_CEXEC);
}
/*--------------------------------------------------*/
void MkSwitchNode(se,e,se2,nc,nd)
ExprPt
	e;

SeqPt
	se,
	se2;

int
	nc,
	nd;
{
#ifdef DEBUG
	if(DebugInfo)
		fprintf(stdout,"Switch Node with %d case(s) & %d default.\n",nc,nd);
#endif
	nc+=1;
	ptn.b._1o1i1e.i0=(nd==0?(-nc):nc);
	ptn.b._1o1i1e.e0=(*e);
	ptn.b._1o1i1e.o0=se2->first;
	PutPTNode(se,T_SWITCH);
}
/*--------------------------------------------------*/
void MkCaseNode(se,beg,size,se2)
SeqPt
	se2,
	se;

Offset
	beg;

int
	size;
{
	ptn.b._1o1e.e0.beg=beg;
	ptn.b._1o1e.e0.size=size;
	ptn.b._1o1e.o0=se2->first;
	PutPTNode(se,T_CASE);
}
/*--------------------------------------------------*/
void MkIf0Node(se,e,se2)
ExprPt
	e;

SeqPt
	se,
	se2;
{
	ptn.b._1o1e.e0=(*e);
	ptn.b._1o1e.o0=se2->first;
	PutPTNode(se,T_IF0);
}
/*--------------------------------------------------*/
void MkIf1Node(se,e,se2,se3)
ExprPt
	e;

SeqPt
	se,
	se2,
	se3;
{
	ptn.b._2o1e.e0=(*e);
	ptn.b._2o1e.o0=se2->first;
	ptn.b._2o1e.o1=se3->first;
	PutPTNode(se,T_IF1);
}
/*--------------------------------------------------*/
void MkWhileNode(se,e,se1)
SeqPt
	se1,
	se;

ExprPt
	e;
{
	ptn.b._1o1e.e0=(*e);
	ptn.b._1o1e.o0=se1->first;
	PutPTNode(se,T_WHILE);
}
/*--------------------------------------------------*/
void MkDoNode(se,e,se1)
SeqPt
	se1,
	se;

ExprPt
	e;
{
	ptn.b._1o1e.e0=(*e);
	ptn.b._1o1e.o0=se1->first;
	PutPTNode(se,T_DO);
}
/*--------------------------------------------------*/
void  MkFor0Node(se,se1)
SeqPt
	se1,
	se;
{
	ptn.b.o=se1->first;
	PutPTNode(se,T_FOR0);
}
/*--------------------------------------------------*/
void  MkFor1Node(se,e3,se1)
SeqPt
	se1,
	se;

ExprPt
	e3;
{
	ptn.b._1o1e.e0=(*e3);
	ptn.b._1o1e.o0=se1->first;
	PutPTNode(se,T_FOR1);
}
/*--------------------------------------------------*/
void  MkFor2Node(se,e2,se1)
SeqPt
	se1,
	se;

ExprPt
	e2;
{
	ptn.b._1o1e.e0=(*e2);
	ptn.b._1o1e.o0=se1->first;
	PutPTNode(se,T_FOR2);
}
/*--------------------------------------------------*/
void  MkFor3Node(se,e1,se1)
SeqPt
	se1,
	se;

ExprPt
	e1;
{
	ptn.b._1o1e.e0=(*e1);
	ptn.b._1o1e.o0=se1->first;
	PutPTNode(se,T_FOR3);
}
/*--------------------------------------------------*/
void  MkFor4Node(se,e2,e3,se1)
SeqPt
	se1,
	se;

ExprPt
	e2,
	e3;
{
	ptn.b._1o2e.e0=(*e2);
	ptn.b._1o2e.e1=(*e3);
	ptn.b._1o2e.o0=se1->first;
	PutPTNode(se,T_FOR4);
}
/*--------------------------------------------------*/
void  MkFor5Node(se,e1,e3,se1)
SeqPt
	se1,
	se;

ExprPt
	e1,
	e3;
{
	ptn.b._1o2e.e0=(*e1);
	ptn.b._1o2e.e1=(*e3);
	ptn.b._1o2e.o0=se1->first;
	PutPTNode(se,T_FOR5);
}
/*--------------------------------------------------*/
void  MkFor6Node(se,e1,e2,se1)
SeqPt
	se1,
	se;

ExprPt
	e1,
	e2;
{
	ptn.b._1o2e.e0=(*e1);
	ptn.b._1o2e.e1=(*e2);
	ptn.b._1o2e.o0=se1->first;
	PutPTNode(se,T_FOR6);
}
/*--------------------------------------------------*/
void  MkFor7Node(se,e1,e2,e3,se1)
SeqPt
	se1,
	se;

ExprPt
	e1,
	e2,
	e3;
{
	ptn.b._1o3e.e0=(*e1);
	ptn.b._1o3e.e1=(*e2);
	ptn.b._1o3e.e2=(*e3);
	ptn.b._1o3e.o0=se1->first;
	PutPTNode(se,T_FOR7);
}
/*--------------------------------------------------*/
void MkContNode(se)
SeqPt
	se;
{
	PutPTNode(se,T_CONTINUE);
}
/*--------------------------------------------------*/
void MkBreakNode(se)
SeqPt
	se;
{
	PutPTNode(se,T_BREAK);
}
/*--------------------------------------------------*/
void MkBlBrkNode(se,bn)
SeqPt
	se;

int
	bn;
{
	ptn.b.b=bn;
	PutPTNode(se,T_BLBRK);
}
/*--------------------------------------------------*/
void MkRet0Node(se)
SeqPt
	se;
{
	PutPTNode(se,T_RET0);
}
/*--------------------------------------------------*/
void MkRet1Node(se,beg,size)
SeqPt
	se;

Offset
	beg;

int
	size;
{
	ptn.b.e.beg=beg;
	ptn.b.e.size=size;
	PutPTNode(se,T_RET1);
}
/*--------------------------------------------------*/
void MkGotoNode(se)
SeqPt
	se;
{
	ptn.b.l=(LabelPt)NULL;
	PutPTNode(se,T_GOTO);
}
/*--------------------------------------------------*/
void MkLabelNode(se,l,se1)
SeqPt
	se1,
	se;

LabelPt
	l;
{
	ptn.b._1o1l.l0=l;
	ptn.b._1o1l.o0=se1->first;
	PutPTNode(se,T_LABEL);
	l->node=se->first;
}
/*--------------------------------------------------*/
void MkBlockNode(se,bn,se1)
SeqPt
	se1,
	se;

int
	bn;
{
	ptn.b._1o1i.i0=bn;
	ptn.b._1o1i.o0=se1->first;
	PutPTNode(se,T_BLOCK);
}
/*--------------------------------------------------*/
void MkJoinNode(se,s)
SeqPt
	se;

SyncPPt
	s;
{
	ptn.b.s=s;
	PutPTNode(se,T_JOIN);
}
/*--------------------------------------------------*/
void MkPassNode(se,s,se0,se1)
SeqPt
	se;

SyncPPt
	s;

SeqPt
	se0,
	se1;
{
	ptn.b._1s2o.s=s;

	if(se0==NULL) ptn.b._1s2o.o0=NULLOFF;
	else ptn.b._1s2o.o0=se0->first;

	if(se1==NULL) ptn.b._1s2o.o1=NULLOFF;
	else ptn.b._1s2o.o1=se1->first;

	PutPTNode(se,T_WHEN);
}
/*--------------------------------------------------*/
void MkSeqNode(se,se1,se2)
SeqPt
	se,
	se1,
	se2;
{
	if(se1->first==NULLOFF) {
		if(se2->first==NULLOFF) se->first=se->last=NULLOFF;
		else *se=(*se2);
	}
	else {
		if(se2->first==NULLOFF) *se=(*se1);
		else {
			SetOffset(ptreef,se1->last,F_BEG);
			WOff(se2->first,ptreef);
			se->first=se1->first;
			se->last=se2->last;
		}
	}
}
/*--------------------------------------------------*/
