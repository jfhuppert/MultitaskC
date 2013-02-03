/*--------------------------------------------------*/
/*
	auto.c
*/
/*--------------------------------------------------*/
/*
 *
 * Copyright (C) 1992-2013 Hugo Delchini.
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
#include <general.h>

#define InLoop	((char)0)
#define InFork	((char)1)
#define InBlock	((char)2)
#define InSwtch	((char)3)

extern void
	XFree(),
	SetOffset(),
	CodeGen();

extern char
	*FileName(),
	*Me;

extern Offset
	GetOffset();

extern FILE
	*gotof,
	*ptreef;

extern Inst
	IBuff;

int
	Labz=0;

FILE
	*CInput,
	*COutput;

StkDecl(char,OSTKSIZE)
	FStack;

StkDecl(Offset,SNSTKSIZE)
	SStack;

StkDecl(Offset,OSTKSIZE)
	BStack,
	CStack;

/*--------------------------------------------------*/
#ifdef DEBUG

extern int
	DebugInfo;

char
	DOfmt[256];

int
	Level=1;

#endif
/*--------------------------------------------------*/
void GetPTNode(o,ptn)
Offset
	o;

PTNodePt
	ptn;
{

	SetOffset(ptreef,o,F_BEG);
	ROff(ptn->h.next,ptreef);
	RChr(ptn->h.type,ptreef);

	switch(ptn->h.type) {

	case T_BREAK : case T_CONTINUE : case T_RET0 :
		break;

	case T_BLBRK :
		RInt(ptn->b.b,ptreef);
		break;

	case T_JOIN :
		RSynP(ptn->b.s,ptreef);
		break;

	case T_WHEN :
		RSynP(ptn->b._1s2o.s,ptreef);
		ROff(ptn->b._1s2o.o0,ptreef);
		ROff(ptn->b._1s2o.o1,ptreef);
		break;

	case T_EXPRESSION : case T_RET1 :
		ROff(ptn->b.e.beg,ptreef); RInt(ptn->b.e.size,ptreef);
		break;

	case T_GOTO :
		RLabP(ptn->b.l,ptreef);
		break;

	case T_LABEL :
		RLabP(ptn->b._1o1l.l0,ptreef);
		ROff(ptn->b._1o1l.o0,ptreef);
		break;

	case T_BLOCK :
		RInt(ptn->b._1o1i.i0,ptreef);
		ROff(ptn->b._1o1i.o0,ptreef);
		break;

	case T_FOR0 :
		ROff(ptn->b.o,ptreef);
		break;

	case T_FOR1 : case T_FOR2 : case T_FOR3 :
	case T_WHILE : case T_DO : case T_IF0 :
	case T_CASE :
		ROff(ptn->b._1o1e.e0.beg,ptreef);
		RInt(ptn->b._1o1e.e0.size,ptreef);
		ROff(ptn->b._1o1e.o0,ptreef);
		break;

	case T_SWITCH :
		RInt(ptn->b._1o1i1e.i0,ptreef);
		ROff(ptn->b._1o1i1e.e0.beg,ptreef);
		RInt(ptn->b._1o1i1e.e0.size,ptreef);
		ROff(ptn->b._1o1i1e.o0,ptreef);
		break;

	case T_FOR4 : case T_FOR5 : case T_FOR6 :
		ROff(ptn->b._1o2e.e0.beg,ptreef);
		RInt(ptn->b._1o2e.e0.size,ptreef);
		ROff(ptn->b._1o2e.e1.beg,ptreef);
		RInt(ptn->b._1o2e.e1.size,ptreef);
		ROff(ptn->b._1o2e.o0,ptreef);
		break;

	case T_FOR7 :
		ROff(ptn->b._1o3e.e0.beg,ptreef);
		RInt(ptn->b._1o3e.e0.size,ptreef);
		ROff(ptn->b._1o3e.e1.beg,ptreef);
		RInt(ptn->b._1o3e.e1.size,ptreef);
		ROff(ptn->b._1o3e.e2.beg,ptreef);
		RInt(ptn->b._1o3e.e2.size,ptreef);
		ROff(ptn->b._1o3e.o0,ptreef);
		break;

	case T_CEXEC :
		ROff(ptn->b._2o0e.o0,ptreef);
		ROff(ptn->b._2o0e.o1,ptreef);
		break;

	case T_IF1 :
		ROff(ptn->b._2o1e.e0.beg,ptreef);
		RInt(ptn->b._2o1e.e0.size,ptreef);
		ROff(ptn->b._2o1e.o0,ptreef);
		ROff(ptn->b._2o1e.o1,ptreef);
		break;

	}
}
/*--------------------------------------------------*/
void WriteExpr(expr)
ExprPt
	expr;
{
	WOff(expr->beg,COutput);
	WInt(expr->size,COutput);
}
/*--------------------------------------------------*/
void WriteHead(type)
char
	type;
{
	if(Labz) { Labelize(type); ULabNI(); }
	WChr(type,COutput);
}
/*--------------------------------------------------*/
Offset WriteIfGoto(lab,expr)
Offset
	lab;

ExprPt
	expr;
{
	WriteHead(IC_IFG);
	WOff(lab,COutput);
	WriteExpr(expr);
	return(GetOffset(COutput)-(sizeof(Offset)+sizeof(int)+sizeof(Offset)));
}
/*--------------------------------------------------*/
Offset WriteBlockBeg(id,lab)
int
	id;

Offset
	lab;
{
	WriteHead(IC_BNG);
	WInt(id,COutput);
	WOff(lab,COutput);
	return(GetOffset(COutput)-sizeof(Offset));
}
/*--------------------------------------------------*/
Offset WriteGoto(lab)
Offset
	lab;
{
	WriteHead(IC_GTO);
	WOff(lab,COutput);
	return(GetOffset(COutput)-sizeof(Offset));
}
/*--------------------------------------------------*/
Offset WriteBreak(lab)
Offset
	lab;
{
	WriteHead(IC_BRK);
	WOff(lab,COutput);
	return(GetOffset(COutput)-sizeof(Offset));
}
/*--------------------------------------------------*/
Offset WriteFork(lab)
Offset
	lab;
{
int
	dummy=0;

	WriteHead(IC_FRK);
	WOff(lab,COutput);
	WInt(dummy,COutput);
	WOff(lab,COutput);
	WInt(dummy,COutput);
	return(GetOffset(COutput)-((2*sizeof(Offset))+(2*sizeof(int))));
}
/*--------------------------------------------------*/
UpdtGotoList(state,first)
Offset
	state,
	first;
{
	if(first==NULLOFF) return(0);
	else {
	int
		ret=0;

		do {

			SetOffset(COutput,first,F_BEG);
			SetOffset(CInput,first,F_BEG);
			ROff(first,CInput);
			WOff(state,COutput);
			ret+=1;

		} while(first!=NULLOFF);

		SetOffset(COutput,0L,F_END);
		return(ret);
	}
}
/*--------------------------------------------------*/
UpdtBCList(bl,cl)
Offset
	bl,
	cl;
{
	UpdtGotoList(bl,StkTop(BStack));
	UpdtGotoList(cl,StkTop(CStack));
}
/*--------------------------------------------------*/
void UpdtOffsetAndInt(a,o,i)
Offset
	a,
	o;

int
	i;
{
	SetOffset(COutput,a,F_BEG);
	WOff(o,COutput);
	WInt(i,COutput);
	SetOffset(COutput,0L,F_END);
}
/*--------------------------------------------------*/
void UpdtOffset(a,o)
Offset
	a,
	o;
{
	SetOffset(COutput,a,F_BEG);
	WOff(o,COutput);
	SetOffset(COutput,0L,F_END);
}
/*--------------------------------------------------*/
Offset UpdtCase(a,e,o)
Offset
	a,
	o;

ExprPt
	e;
{
char
	type=IC_CAS;

	SetOffset(COutput,a,F_BEG);
	WChr(type,COutput);
	WOff(e->beg,COutput);
	WInt(e->size,COutput);
	WOff(o,COutput);
	a=GetOffset(COutput);
	SetOffset(COutput,0L,F_END);
	return(a);
}
/*--------------------------------------------------*/
void WriteSwitch(nc,e,b,c)
int
	nc;

ExprPt
	e;

Offset
	*b,
	*c;
{
char
	type=IC_CAS;

int
	gdef;

Offset
	o=NULLOFF;

	gdef=(nc<0);
	nc=(nc<0?-nc:nc);

	WriteHead(IC_SWH);
	WInt(nc,COutput);
	WOff(e->beg,COutput);
	WInt(e->size,COutput);

	if(gdef) {
		WChr(type,COutput);
		WOff(o,COutput);
		gdef=0; WInt(gdef,COutput);
		*b=GetOffset(COutput);
		WOff(o,COutput);
		nc--;
	}
	else *b=NULLOFF;

	*c=GetOffset(COutput);

	while(nc--) {
		WChr(type,COutput);
		WOff(o,COutput);
		WInt(gdef,COutput);
		WOff(o,COutput);
	}
}
/*--------------------------------------------------*/
static Offset BAuto(cn,mnb,cmnb)
Offset
	cn;

int
	*mnb,
	*cmnb;
{
PTNode
	ptn;

Offset
	ret0,
	ret1,
	blab;

#ifdef DEBUG
	if(DebugInfo) Level++;
#endif

BAutoBeg:

#ifdef DEBUG
	if(DebugInfo) {
		sprintf(DOfmt,"%%%dsI node %%ld\n",Level-1);
		fprintf(stdout,DOfmt," ",(long)cn);
	}
#endif

	GetPTNode(cn,&ptn);
	blab=GetOffset(COutput);

	switch(ptn.h.type) {

	case T_CASE :
		StkTop(SStack)=UpdtCase(StkTop(SStack),&ptn.b._1o1e.e0,blab);
		LabNI();
		if(ptn.b._1o1e.o0!=NULLOFF) BAuto(ptn.b._1o1e.o0,mnb,cmnb);
		break;

	case T_SWITCH :
		if(ptn.b._1o1i1e.o0!=NULLOFF) {
			StkPush(FStack,InSwtch); StkPush(CStack,NULLOFF);
			WriteSwitch(ptn.b._1o1i1e.i0,&ptn.b._1o1i1e.e0,&ret0,&ret1);
			StkPush(BStack,ret0);
			StkPush(SStack,ret1);
			LabNI();
			ret0=BAuto(ptn.b._1o1i1e.o0,mnb,cmnb);
			UpdtGotoList(ret0,StkTop(BStack));
			StkPop(SStack);
			ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);
			if(ret0!=NULLOFF) LabNI();
		}
		else {
			WriteHead(IC_EXP);
			WriteExpr(&ptn.b._1o1i1e.e0);
		}
		break;

	case T_BREAK :
		if(!StkEmpty(BStack)) {
			switch(StkTop(FStack)) {
			case InLoop :
			case InSwtch :
				StkTop(BStack)=WriteGoto(StkTop(BStack));
				break;

			case InFork :
				StkTop(BStack)=WriteBreak(StkTop(BStack));
				break;

			case InBlock :
				fprintf(stderr,"%s : illegal break, quit.\n",Me);
				LeaveApp(1);

			default :
				fprintf(stderr,"%s : spurious value in FStack in BAuto(), quit.\n",Me);
				LeaveApp(1);
			}
		}
		break;

	case T_CONTINUE :
		if(!StkEmpty(CStack)) {
			switch(StkTop(FStack)) {
			case InLoop :
				StkTop(CStack)=WriteGoto(StkTop(CStack));
				break;

			case InFork :
			case InSwtch :
				break;

			case InBlock :
				fprintf(stderr,"%s : illegal continue, quit.\n",Me);
				LeaveApp(1);

			default :
				fprintf(stderr,"%s : spurious value in FStack in BAuto(), quit.\n",Me);
				LeaveApp(1);
			}
		}
		break;

	case T_RET0 :
		WriteHead(IC_RTN);
		break;

	case T_JOIN :
		WriteHead(IC_JOI);
		WSynP(ptn.b.s,COutput);
		break;

	case T_WHEN :
		WriteHead(IC_WHE);
		WSynP(ptn.b._1s2o.s,COutput);

		blab=GetOffset(COutput);
		ret0=blab+(sizeof(Offset)*2);
		WOff(ret0,COutput);
		WOff(ret0,COutput);
		ret0=blab+sizeof(Offset);

		if(ptn.b._1s2o.o0!=NULLOFF) {
			BAuto(ptn.b._1s2o.o0,mnb,cmnb);
			if(ptn.b._1s2o.o1!=NULLOFF) {
				ret1=WriteGoto(NULLOFF);
				UpdtOffset(ret0,GetOffset(COutput));
				BAuto(ptn.b._1s2o.o1,mnb,cmnb);
				UpdtOffset(ret1,GetOffset(COutput));
				LabNI();
			}
			else {
				UpdtOffset(ret0,GetOffset(COutput));
			}
		}
		else {
			if(ptn.b._1s2o.o1!=NULLOFF) {
				BAuto(ptn.b._1s2o.o1,mnb,cmnb);
				UpdtOffset(blab,GetOffset(COutput));
			}
		}
		break;

	case T_EXPRESSION :
		WriteHead(IC_EXP);
		WriteExpr(&ptn.b.e);
		break;

	case T_RET1 :
		WriteHead(IC_ERT);
		WriteExpr(&ptn.b.e);
		break;

	case T_GOTO :
		if(ptn.b.l->state==NULLOFF) {
			ptn.b.l->node=WriteGoto(ptn.b.l->node);
		}
		else {
			ptn.b.l->nlink-=1;
			WriteGoto(ptn.b.l->state);
			if(ptn.b.l->nlink==0) XFree(ptn.b.l,sizeof(*ptn.b.l));
		}
		break;

	case T_LABEL :
		if(ptn.b._1o1l.l0!=(LabelPt)NULL) {
			ptn.b._1o1l.l0->state=blab;
			ptn.b._1o1l.l0->nlink-=UpdtGotoList(blab,ptn.b._1o1l.l0->node);
			if(ptn.b._1o1l.l0->nlink==0) XFree(ptn.b._1o1l.l0,sizeof(*ptn.b._1o1l.l0));
			LabNI();
		}
		if(ptn.b._1o1l.o0!=NULLOFF) {
			BAuto(ptn.b._1o1l.o0,mnb,cmnb);
		}
		break;

	case T_BLBRK :
		WriteHead(IC_BBR);
		WInt(ptn.b.b,COutput);
		break;

	case T_BLOCK :
		if(ptn.b._1o1i.o0!=NULLOFF) {
			StkPush(FStack,InBlock); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);
			ret0=WriteBlockBeg(ptn.b._1o1i.i0,NULLOFF);
			(*cmnb)+=1;
			if((*cmnb)>(*mnb)) (*mnb)=(*cmnb);
			ret1=BAuto(ptn.b._1o1i.o0,mnb,cmnb);
			(*cmnb)-=1;
			WriteHead(IC_BND);
			UpdtOffset(ret0,ret1+IS_BND);
			ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);
		}
		break;

	case T_FOR0 :
		ret0=NULLOFF;
		LabNI();
		if(ptn.b.o!=NULLOFF) {
			StkPush(FStack,InLoop); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);
			ret0=BAuto(ptn.b.o,mnb,cmnb)+IS_GTO;
			UpdtBCList(ret0,blab);
			ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);
		}
		WriteGoto(blab);
		if(ret0!=NULLOFF) LabNI();
		break;

	case T_FOR1 :
		ret0=NULLOFF;
		LabNI();
		if(ptn.b._1o1e.o0!=NULLOFF) {
			ret1=NULLOFF;
			StkPush(FStack,InLoop); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);
			ret0=BAuto(ptn.b._1o1e.o0,mnb,cmnb);
			UpdtBCList(ret0+IS_GTO+IS_EXP,ret0);
			ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);
			if(ret1!=NULLOFF) LabNI();
		}
		WriteHead(IC_EXP); WriteExpr(&ptn.b.e);
		WriteGoto(blab);
		if(ret0!=NULLOFF) LabNI();
		break;

	case T_FOR2 :
	case T_WHILE :
		LabNI();
		if(ptn.b._1o1e.o0!=NULLOFF) {
			ptn.b._1o1e.e0.size=(-ptn.b._1o1e.e0.size);
			ret0=WriteIfGoto(NULLOFF,&ptn.b._1o1e.e0);
			StkPush(FStack,InLoop); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);
			ret1=BAuto(ptn.b._1o1e.o0,mnb,cmnb);
			UpdtOffset(ret0,ret1+IS_GTO);
			UpdtBCList(ret1+IS_GTO,blab);
			ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);
			WriteGoto(blab);
			LabNI();
		}
		else {
			WriteIfGoto(blab,&ptn.b._1o1e.e0);
		}
		break;

	case T_FOR3 :
		ret0=NULLOFF;
		WriteHead(IC_EXP); WriteExpr(&ptn.b.e);
		LabNI();
		if(ptn.b._1o1e.o0!=NULLOFF) {
			StkPush(FStack,InLoop); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);
			ret0=BAuto(ptn.b._1o1e.o0,mnb,cmnb)+IS_GTO;
			UpdtBCList(ret0,blab+IS_EXP);
			ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);
		}
		WriteGoto(blab+IS_EXP);
		if(ret0!=NULLOFF) LabNI();
		break;

	case T_DO :
		ret0=NULLOFF;
		LabNI();
		if(ptn.b._1o1e.o0!=NULLOFF) {
			StkPush(FStack,InLoop); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);
			ret0=BAuto(ptn.b._1o1e.o0,mnb,cmnb);
			UpdtBCList(ret0+IS_IFG,ret0);
			ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);
			if(ret1!=NULLOFF) LabNI();
		}
		WriteIfGoto(blab,&ptn.b._1o1e.e0);
		if(ret0!=NULLOFF) LabNI();
		break;

	case T_FOR4 :
		LabNI();
		ptn.b._1o2e.e0.size=(-ptn.b._1o2e.e0.size);
		ret0=WriteIfGoto(blab+IS_EXP+IS_GTO+IS_IFG,&ptn.b._1o2e.e0);
		if(ptn.b._1o2e.o0!=NULLOFF) {
			StkPush(FStack,InLoop); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);
			ret1=BAuto(ptn.b._1o2e.o0,mnb,cmnb);
			UpdtOffset(ret0,ret1+IS_GTO+IS_EXP);
			UpdtBCList(ret1+IS_GTO+IS_EXP,ret1);
			ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);
			if(ret1!=NULLOFF) LabNI();
		}
		WriteHead(IC_EXP); WriteExpr(&ptn.b._1o2e.e1);
		WriteGoto(blab);
		LabNI();
		break;

	case T_FOR5 :
		ret0=NULLOFF;
		WriteHead(IC_EXP); WriteExpr(&ptn.b._1o2e.e0);
		LabNI();
		if(ptn.b._1o2e.o0!=NULLOFF) {
			StkPush(FStack,InLoop); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);
			ret0=BAuto(ptn.b._1o2e.o0,mnb,cmnb);
			UpdtBCList(ret0+IS_EXP+IS_GTO,ret0);
			ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);
			if(ret1!=NULLOFF) LabNI();
		}
		WriteHead(IC_EXP); WriteExpr(&ptn.b._1o2e.e1);
		WriteGoto(blab+IS_EXP);
		if(ret0!=NULLOFF) LabNI();
		break;

	case T_FOR6 :
		WriteHead(IC_EXP); WriteExpr(&ptn.b._1o2e.e0);
		blab+=IS_EXP;
		LabNI();
		if(ptn.b._1o2e.o0!=NULLOFF) {
			ptn.b._1o2e.e1.size=(-ptn.b._1o2e.e1.size);
			ret0=WriteIfGoto(NULLOFF,&ptn.b._1o2e.e1);
			StkPush(FStack,InLoop); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);
			ret1=BAuto(ptn.b._1o2e.o0,mnb,cmnb);
			UpdtOffset(ret0,ret1+IS_GTO);
			UpdtBCList(ret1+IS_GTO,blab);
			ret0=StkPop(BStack); ret0=StkPop(CStack); StkPop(FStack);
			WriteGoto(blab);
			LabNI();
		} else {
			WriteIfGoto(blab,&ptn.b._1o2e.e1);
		}
		break;

	case T_FOR7 :
		WriteHead(IC_EXP); WriteExpr(&ptn.b._1o3e.e0);
		blab+=IS_EXP;
		LabNI();
		ptn.b._1o3e.e1.size=(-ptn.b._1o3e.e1.size);
		ret0=WriteIfGoto(blab+IS_EXP+IS_IFG+IS_GTO,&ptn.b._1o3e.e1);
		if(ptn.b._1o3e.o0!=NULLOFF) {
			StkPush(FStack,InLoop); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);
			ret1=BAuto(ptn.b._1o3e.o0,mnb,cmnb);
			UpdtOffset(ret0,ret1+IS_EXP+IS_GTO);
			UpdtBCList(ret1+IS_GTO+IS_EXP,ret1);
			ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);
			if(ret1!=NULLOFF) LabNI();
		}
		WriteHead(IC_EXP); WriteExpr(&ptn.b._1o3e.e2);
		WriteGoto(blab);
		LabNI();
		break;

	case T_CEXEC :
		if(ptn.b._2o0e.o0!=NULLOFF) {
			if(ptn.b._2o0e.o1!=NULLOFF) {

				/* ------------------------------------------------------- */
				/* ---------- Generation code des deux branche. ---------- */
				/* ------------------------------------------------------- */

			int
				_mnb=0,
				_cmnb=0;

				/* Preparation des piles de "continue" et "break". */
				StkPush(FStack,InFork); StkPush(BStack,NULLOFF); StkPush(CStack,NULLOFF);

				/* Generation de l'instruction "fork". */
				ret0=WriteFork(NULLOFF);

				/* Generation code premiere branche. */
				BAuto(ptn.b._2o0e.o0,&_mnb,&_cmnb);

				/* Generation du marqueur de l'etat terminal. */
				WriteHead(IC_HLT);

				/* Mise a jour de l'adresse de debut de la branche 2 dans l'instruction "fork". */
				/* Mise a jour du nombre max de block imbrique pour la premiere branche. */
				UpdtOffsetAndInt(ret0,GetOffset(COutput),_mnb);

				_mnb=0;
				_cmnb=0;

				/* Generation code deuxieme branche. */
				BAuto(ptn.b._2o0e.o1,&_mnb,&_cmnb);

				/* Generation du marqueur de l'etat terminal. */
				WriteHead(IC_HLT);

				/* Mise a jour de l'adresse de fin de fork dans l'instruction "fork". */
				/* Mise a jour du nombre max de block imbrique pour la deuxieme branche. */
				ret0+=(sizeof(Offset)+sizeof(int));
				UpdtOffsetAndInt(ret0,GetOffset(COutput),_mnb);

				/* Mise a jour des "break" vers l'etat terminal. */
				UpdtGotoList(GetOffset(COutput),StkTop(BStack));

				/* Restauration des piles de "continue" et "break". */
				ret0=StkPop(BStack); ret1=StkPop(CStack); StkPop(FStack);

				/* Etiquetage de la prochaine instruction si l'etat terminal est rejoint. */
				if(ret0!=NULLOFF) LabNI();

			}
			else BAuto(ptn.b._2o0e.o0,mnb,cmnb);
		}
		else {
			if(ptn.b._2o0e.o1!=NULLOFF) BAuto(ptn.b._2o0e.o1,mnb,cmnb);
		}
		break;

	case T_IF0 :
		if(ptn.b._1o1e.o0!=NULLOFF) {
			ptn.b._1o1e.e0.size=(-ptn.b._1o1e.e0.size);
			ret0=WriteIfGoto(NULLOFF,&ptn.b._1o1e.e0);
			ret1=BAuto(ptn.b._1o1e.o0,mnb,cmnb);
			UpdtOffset(ret0,ret1);
			LabNI();
		}
		else {
			WriteHead(IC_EXP);
			WriteExpr(&ptn.b._1o1e.e0);
		}
		break;

	case T_IF1 :
		if(ptn.b._2o1e.o0!=NULLOFF) {
			if(ptn.b._2o1e.o1!=NULLOFF) {
				ret0=WriteIfGoto(NULLOFF,&ptn.b._2o1e.e0);
				ret1=BAuto(ptn.b._2o1e.o1,mnb,cmnb)+IS_GTO;
				UpdtOffset(ret0,ret1);
				ret0=WriteGoto(NULLOFF);
				LabNI();
			}
			else {
				ptn.b._2o1e.e0.size=(-ptn.b._2o1e.e0.size);
				ret0=WriteIfGoto(NULLOFF,&ptn.b._2o1e.e0);
			}
			ret1=BAuto(ptn.b._2o1e.o0,mnb,cmnb);
			UpdtOffset(ret0,ret1);
			LabNI();
		}
		else {
			if(ptn.b._2o1e.o1!=NULLOFF) {
				ret0=WriteIfGoto(NULLOFF,&ptn.b._2o1e.e0);
				ret1=BAuto(ptn.b._2o1e.o1,mnb,cmnb);
				UpdtOffset(ret0,ret1);
				LabNI();
			}
			else {
				WriteHead(IC_EXP);
				WriteExpr(&ptn.b._2o1e.e0);
			}
		}
		break;

	default :
		fprintf(
		stderr,
		"%s : fatal error, unknown ptree node type <%d> in <%s>.\n",
		Me,
		(int)ptn.h.type,
		FileName(PTREEFN,TMPEXT)
		);
		LeaveApp(1);

	}

	cn=ptn.h.next;
	if(cn==NULLOFF) {
#ifdef DEBUG
		if(DebugInfo) Level--;
#endif
		return(GetOffset(COutput));
	}
	goto BAutoBeg;
}
/*--------------------------------------------------*/
void BuildAuto(se,sourceFile)
SeqPt
	se;

char
	*sourceFile;
{
int
	_mnb=0,
	_cmnb=0;

	fclose(ptreef);
	if((ptreef=fopen(FileName(PTREEFN,TMPEXT),"r+"))==NULL) {
		fprintf(stderr,"%s : can't open <%s>, quit.\n",Me,FileName(PTREEFN,TMPEXT));
		LeaveApp(1);
	}

	if((COutput=fopen(FileName(CODEFN,TMPEXT),"w+"))==NULL) {
		fprintf(stderr,"%s : can't open <%s>, quit.\n",Me,FileName(CODEFN,TMPEXT));
		LeaveApp(1);
	}
	else InitFile(COutput);

	if((CInput=fopen(FileName(CODEFN,TMPEXT),"r+"))==NULL) {
		fprintf(stderr,"%s : can't open <%s>, quit.\n",Me,FileName(CODEFN,TMPEXT));
		LeaveApp(1);
	}

#ifdef DEBUG
	if(DebugInfo)
		fprintf(stdout,"\n--- Current function (first at %ld)\n",(long)se->first);
#endif

	/* Initialisation de la pile pour les case dans les switch. */
	StkInit(SStack,SNSTKSIZE);

	/* Initialisation des piles pour "break" et "continue". */
	StkInit(FStack,OSTKSIZE);
	StkInit(BStack,OSTKSIZE);
	StkInit(CStack,OSTKSIZE);

	if(se->first!=NULLOFF) BAuto(se->first,&_mnb,&_cmnb);

#ifdef DEBUG
	if(DebugInfo)
		fprintf(stdout,"--- End of current function\n\n");
#endif

	fclose(ptreef);
	fclose(COutput);
	fclose(CInput);

	/* Generation du code. */
	CodeGen(sourceFile);

#ifdef DEBUG
	if(DebugInfo) rename(FileName(CODEFN,TMPEXT),LCODEFN);
	else
#endif
	unlink(FileName(CODEFN,TMPEXT));

#ifdef DEBUG
	if(DebugInfo) rename(FileName(PTREEFN,TMPEXT),LPTREEFN);
#endif
	if((ptreef=fopen(FileName(PTREEFN,TMPEXT),"w+"))==NULL) {
		fprintf(stderr,"%s : can't open <%s>, quit.\n",Me,FileName(PTREEFN,TMPEXT));
		LeaveApp(1);
	}
	else InitFile(ptreef);
	
#ifdef DEBUG
	if(DebugInfo) rename(FileName(GOTOFN,TMPEXT),LGOTOFN);
#endif
	if((gotof=fopen(FileName(GOTOFN,TMPEXT),"w+"))==NULL) {
		fprintf(stderr,"%s : can't open <%s>, quit.\n",Me,FileName(GOTOFN,TMPEXT));
		LeaveApp(1);
	}

}
/*--------------------------------------------------*/
