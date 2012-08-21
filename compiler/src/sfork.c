/*--------------------------------------------------*/
/*
	sfork.c
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
#ifdef DEBUG
	PrintState(),
#endif
	XFree(),
	Indent(),
	GenReturn(),
	SFree(),
	BSFree(),
	(*EPrint)(),
	SBtFree(),
	FreeAllBS(),
	FatalMemErr();

extern char
	*XMalloc(),
	*FileName(),
	*Me,
	*ExtendReduce(),
	**GetCState(),
	*CodeEnd,
	*CodeBase,
	*CInst,
	*EndStateAdd,
	*CState[],
	*TState[],
	*TS[],
	LFmt[];

extern int
#ifdef DEBUG
	DebugInfo,
	Trace,
#endif
	SCmp(),
	Results,
	DebugMode,
	Labz,
	BrkIds[],
	NFBTree,
	NFSList,
	NStates,
	NTrans,
	NElem,
	NInst,
	NextLabel;

extern FILE
#ifdef DEBUG
	*SrcFile,
#endif
	*Output;

extern SNodePt
	BuildBtState(),
	BuildState(),
	BuildInitState();

extern SBtNodePt
	SInsert(),
	BrTreeStateSet,
	StateSet;

extern SListElemPt
	NewSListElem();

extern SBtNode
	EndBtNode;

extern SNode
	EndState;

char
	*ISo1,
	*ISo2;

int
	NIState;

FILE
	*TransFile;

PBtNodePt
	PcSet;

static StkDecl(PBtNodePt *,SBTSTKSIZE)
	PcStk;

/*--------------------------------------------------*/
char
	TstOptCmd[]="( \
sort -u | awk \' \
BEGIN { pcs[\"\"]=0; pred=\"X X X\"; nlev=0; first=1; }\
NF==1 { pcs[$1]=1; next }\
{\
	n=split(pred,pr);\
\
	if(n<3) {\
		printf(\"awk error, NULL record %s line %d.\\n\",pred,NR-1) > \"/dev/tty\";\
		exit(1);\
	}\
\
	for(i=0;i<(NF-2);i++) if(pr[i+1]!=$(i+1)) break;\
	ncl=(nlev-i); nlev=i;\
	while(ncl--) { for(j=0;j<(ncl+nlev);j++) printf(\" \"); printf(\"   }\\n\"); }\
\
	if(first) first=0; else { for(j=0;j<nlev;j++) printf(\" \"); printf(\"   else\\n\"); }\
\
	while(i<(NF-2)) {\
		for(j=0;j<i;j++) printf(\" \");\
		printf(\"   if(%s) {\\n\",$(i+1));\
		i+=1;\
		nlev+=1;\
	}\
\
	for(j=0;j<i;j++) printf(\" \");\
	printf(\"   \");\
	if($(i+1)==\"G\") printf(\"goto %s ;\\n\",$(i+2));\
	else {\
		n=split($(i+2),pr,\",\");\
		f=1;\
		for(j=1;j<=n;j++) {\
			split(pr[j],two,\"=\");\
			if(pcs[two[1]]==1) if(f) { printf(\"%s\",pr[j]); f=0; } else printf(\",%s\",pr[j]);\
		}\
		printf(\" ;\\n\");\
	}\
\
	pred=$0;\
	next;\
}\
END { while(nlev--) { for(j=0;j<nlev;j++) printf(\" \"); printf(\"   }\\n\"); } }\
') > XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

char
	TstOptResult[]="\
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
/*--------------------------------------------------*/
void BuildTstOptCmd()
{
	sprintf(TstOptCmd+(sizeof(TstOptCmd)-sizeof(TstOptResult)),FileName(STFN,TMPEXT));
	sprintf(TstOptResult,FileName(STFN,TMPEXT));
}
/*--------------------------------------------------*/
int GetNT(state)
SNodePt
	state;
{
	switch(state->type) {
	case SN_LEAF : return(1);
	case SN_NODE : return(GetNT(state->b.s.l)+GetNT(state->b.s.r));
	}
}
/*--------------------------------------------------*/
SNodePt DupState(model)
SNodePt
	model;
{
SNodePt
	state;

	state=(SNodePt)XMalloc(sizeof(SNode));
	if(state==(SNodePt)NULL) FatalMemErr("malloc() error in DupState()");
	state->type=model->type;
	state->blocks=NULL;

	switch(model->type) {

	case SN_LEAF :
		state->b.i=model->b.i;
		break;

	case SN_NODE :
		state->b.s.l=DupState(model->b.s.l);
		state->b.s.r=DupState(model->b.s.r);
		break;

	}

	return(state);
}
/*------------------------------------------------*/
PTTest(s,nt)
SNodePt
	s;

int
	nt;
{
	switch(s->type) {

	case SN_LEAF :
		fprintf(TransFile," %s%d==0x%lx",PCNAME,nt,IOff(s->b.i));
		break;

	case SN_NODE :
		PTTest(s->b.s.l,(nt<<1)+1);
		PTTest(s->b.s.r,(nt<<1)+2);
		break;

	}
}
/*------------------------------------------------*/
void PTNull(s,nt)
SNodePt
	s;

int
	nt;
{
	switch(s->type) {

	case SN_LEAF :
		fprintf(TransFile,",%s%d=0x0",PCNAME,nt);
		break;

	case SN_NODE :
		PTNull(s->b.s.l,(nt<<1)+1);
		PTNull(s->b.s.r,(nt<<1)+2);
		break;

	}
}
/*------------------------------------------------*/
void PTAff(s,nt,ds,f)
SNodePt
	s,
	ds;

int
	*f,
	nt;
{
	switch(s->type) {
	case SN_LEAF :
		switch(ds->type) {
		case SN_LEAF :
			if(ds->b.i!=s->b.i) fprintf(TransFile,"%s%s%d=0x%lx",(*f==0)?" ":",",PCNAME,nt,IOff(s->b.i)),*f=1;
			break;
		case SN_NODE :
			fprintf(TransFile,"%s%s%d=0x%lx",(*f==0)?" ":",",PCNAME,nt,IOff(s->b.i)),*f=1;
			PTNull(ds->b.s.l,(nt<<1)+1);
			PTNull(ds->b.s.r,(nt<<1)+2);
			break;
		}
		break;

	case SN_NODE :
		switch(ds->type) {
		case SN_LEAF :
			fprintf(TransFile,"%s%s%d=0x0",(*f==0)?" ":",",PCNAME,nt),*f=1;
			PTAff(s->b.s.l,(nt<<1)+1,ds,f);
			PTAff(s->b.s.r,(nt<<1)+2,ds,f);
			break;
		case SN_NODE :
			PTAff(s->b.s.l,(nt<<1)+1,ds->b.s.l,f);
			PTAff(s->b.s.r,(nt<<1)+2,ds->b.s.r,f);
			break;
		}
		break;
	}
}
/*------------------------------------------------*/
void GenTrans(ds,s)
SNodePt
	ds,
	s;
{
int
	f,
	nto,
	ntn;

	nto=GetNT(ds);
	PTTest(ds,0);

	if(s->type==SN_LEAF) {
		fprintf(TransFile," G ");
		fprintf(TransFile,LFmt,IOff(s->b.i));
	}
	else {
		fprintf(TransFile," A");
		f=0; PTAff(s,0,ds,&f);
		ntn=GetNT(s);
		if(nto!=ntn) fprintf(TransFile,",%s=%d",NTNAME,ntn);
	}

	fprintf(TransFile,"\n");
}
/*------------------------------------------------*/
int FindRetPc(s,r,nt)
SNodePt
	s;

char
	*r;

int
	nt;
{
int
	ret;

	switch(s->type) {
	case SN_LEAF :
		if(s->b.i==r) return(nt);
		else return(0);

	case SN_NODE :
		if(ret=FindRetPc(s->b.s.l,r,(nt<<1)+1)) return(ret);
		return(FindRetPc(s->b.s.r,r,(nt<<1)+2));
	}
}
/*------------------------------------------------*/
static int ExtRed(state)
SNodePt
	state;
{
void
	PcLookUp();

int
	totmod,
	nmod;

char
	*r;

	if(state->type==SN_LEAF && state->b.i==EndStateAdd) return(0);

	totmod=0;

	do {

		nmod=0;

		r=ExtendReduce(state,&nmod);

		if(r!=NULL) {
			totmod=FindRetPc(state,r,0);
			if(totmod==0) {
				fprintf(stderr,"%s : strange case in ExtRed() -s option, can't find return.",Me);
				LeaveApp(1);
			}
			PcLookUp(totmod,&PcSet);
			SFree(state->b.s.l);
			SFree(state->b.s.r);
			state->type=SN_LEAF;
			BSFree(state->blocks);
			state->blocks=(SElemPt)NULL;
			state->b.i=r;
			return(0);
		}

		if(state->type==SN_LEAF && state->b.i==EndStateAdd) { return(1); }

		nmod+=BrkRed(state);

		if(state->type==SN_LEAF && state->b.i==EndStateAdd) { return(1); }

		nmod+=ResolvSync(state);

		totmod+=nmod;

	} while(nmod);

	return(totmod);

}
/*------------------------------------------------*/
int NotInitState(s)
SNodePt
	s;
{
	if(s->type==SN_NODE && s->b.s.l->type==SN_LEAF && s->b.s.r->type==SN_LEAF && s->b.s.l->b.i==ISo1 && s->b.s.r->b.i==ISo2) {
		NIState+=1;
		return(0);
	}
	else return(1);
}
/*------------------------------------------------*/
static int SFind(state,bt,node)
SNodePt
	state;
	
SBtNodePt
	bt,
	*node;
{
int
	nis;

SNodePt
	dstate;

	/* Extention/Reduction de l'etat. */
	nis=NotInitState(state);
	dstate=DupState(state);
	if(ExtRed(state) && nis) GenTrans(dstate,state);
	SFree(dstate);

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
/*--------------------------------------------------*/
static void
	GenActSw();

static void GenChildAct(f,cpcn,cinst)
int
	f;

char
	*cinst;

int
	cpcn;
{
char
	*lab;

Inst
	IBuff;

	FOREVER {

		lab=cinst;
		GetChrAndMove(cinst,IBuff.h);

		if(Labelized(IBuff.h)) { UnLabelize(IBuff.h); }

		switch(IBuff.h) {

		case IC_JOI :
			SkipSyn(cinst);
			break;

		case IC_WHE :
			SkipSyn(cinst); SkipOff(cinst); SkipOff(cinst);
			break;

		case IC_HLT :
			if(f) fprintf(Output,"   }\n\n");
			return;

		case IC_BNG :
			SkipInt(cinst); SkipOff(cinst);
			break;

		case IC_BRK :
			SkipOff(cinst);
			break;

		case IC_BND :
			break;

		case IC_RTN :
			if(f) {
				fprintf(Output,"   case 0x%lx :\n",IOff(lab));
				fprintf(Output,"     return ;\n");
			}
			break;

		case IC_BBR :
			SkipInt(cinst);
			break;

		case IC_IFG :
			SkipOff(cinst); SkipOff(cinst); SkipInt(cinst);
			break;

		case IC_SWH :
			GetIntAndMove(cinst,IBuff.b.s.i0); SkipOff(cinst); SkipInt(cinst);
			while(IBuff.b.s.i0--) { SkipChr(cinst); SkipOff(cinst); SkipInt(cinst); SkipOff(cinst); }
			break;

		case IC_FRK :
			GetOffAndMove(cinst,IBuff.b.f.o0);
			SkipInt(cinst);
			GetOffAndMove(cinst,IBuff.b.f.o1);
			SkipInt(cinst);
			GenChildAct(f,cpcn,IAdd(IBuff.b.f.o1));
			GenActSw(cpcn,IOff(cinst),IBuff.b.f.o0);
			return;

		case IC_GTO :
			SkipOff(cinst);
			break;

		case IC_ERT :
			if(f) {
				GetOffAndMove(cinst,IBuff.b.e.beg);
				GetIntAndMove(cinst,IBuff.b.e.size);
				fprintf(Output,"   case 0x%lx :\n",IOff(lab));
				fprintf(Output,"     return ( ");
				EPrint(Output,IBuff.b.e.beg,IBuff.b.e.size,DRET); fprintf(Output," ) ;\n");
			}
			else {
				SkipOff(cinst); SkipInt(cinst);
			}
			break;

		case IC_EXP :
			if(f) {
				GetOffAndMove(cinst,IBuff.b.e.beg);
				GetIntAndMove(cinst,IBuff.b.e.size);
				fprintf(Output,"   case 0x%lx :\n",IOff(lab));
				fprintf(Output,"     "); EPrint(Output,IBuff.b.e.beg,IBuff.b.e.size,DEXP); fprintf(Output," ;\n");
				fprintf(Output,"     %s%d = 0x%lx ;\n",PCNAME,cpcn,IOff(cinst));
				fprintf(Output,"     %s ++ ;\n",PFLAGNAME);
				fprintf(Output,"     break ;\n");
			}
			else {
				SkipOff(cinst); SkipInt(cinst);
			}
			break;

		default :
			fprintf(

			stderr,
			"%s : bad instruction code in %s during GenActSw() <%d>, fatal error.\n",
			Me,
			FileName(CODEFN,TMPEXT),
			(int)IBuff.h

			);
			LeaveApp(1);
		}
	}
}
/*--------------------------------------------------*/
static void GenActSw(cpcn,o1,o2)
Offset
	o1,
	o2;

int
	cpcn;
{
int
	f;

	cpcn<<=1;

	cpcn++;
	f=FindPc(cpcn,PcSet);
	if(f) fprintf(Output,"   switch ( %s%d ) {\n",PCNAME,cpcn);
	GenChildAct(f,cpcn,IAdd(o1));

	cpcn++;
	f=FindPc(cpcn,PcSet);
	if(f) fprintf(Output,"   switch ( %s%d ) {\n",PCNAME,cpcn);
	GenChildAct(f,cpcn,IAdd(o2));
}
/*--------------------------------------------------*/
static void
	GenBrSw();

static void GenChildB(f,cpcn,cinst)
int
	f;

char
	*cinst;

int
	cpcn;
{
char
	*lab;

Inst
	IBuff;

	FOREVER {

		lab=cinst;
		GetChrAndMove(cinst,IBuff.h);

		if(Labelized(IBuff.h)) { UnLabelize(IBuff.h); }

		switch(IBuff.h) {

		case IC_JOI :
			SkipSyn(cinst);
			break;

		case IC_WHE :
			SkipSyn(cinst); SkipOff(cinst); SkipOff(cinst);
			break;

		case IC_HLT :
			if(f) fprintf(Output,"   }\n\n");
			return;

		case IC_BNG :
			SkipInt(cinst); SkipOff(cinst);
			break;

		case IC_BRK :
			SkipOff(cinst);
			break;

		case IC_RTN :
		case IC_BND :
			break;

		case IC_BBR :
			SkipInt(cinst);
			break;

		case IC_IFG :
			SkipOff(cinst); SkipOff(cinst); SkipInt(cinst);
			break;

		case IC_SWH :
			GetIntAndMove(cinst,IBuff.b.s.i0); SkipOff(cinst); SkipInt(cinst);
			while(IBuff.b.s.i0--) { SkipChr(cinst); SkipOff(cinst); SkipInt(cinst); SkipOff(cinst); }
			break;

		case IC_FRK :
			GetOffAndMove(cinst,IBuff.b.f.o0);
			SkipInt(cinst);
			GetOffAndMove(cinst,IBuff.b.f.o1);
			SkipInt(cinst);
			GenChildB(f,cpcn,IAdd(IBuff.b.f.o1));
			GenBrSw(cpcn,IOff(cinst),IBuff.b.f.o0);
			return;

		case IC_GTO :
			if(f) {
				GetOffAndMove(cinst,IBuff.b.o);
				fprintf(Output,"   case 0x%lx :\n",IOff(lab));
				fprintf(Output,"     %s%d = 0x%lx ;\n",PCNAME,cpcn,IBuff.b.o);
				fprintf(Output,"     %s ++ ;\n",PFLAGNAME);
				fprintf(Output,"     break ;\n");
			}
			else {
				SkipOff(cinst);
			}
			break;

		case IC_ERT :
		case IC_EXP :
			SkipOff(cinst); SkipInt(cinst);
			break;

		default :
			fprintf(

			stderr,
			"%s : bad instruction code in %s during GenBrSw() <%d>, fatal error.\n",
			Me,
			FileName(CODEFN,TMPEXT),
			(int)IBuff.h

			);
			LeaveApp(1);
		}
	}
}
/*--------------------------------------------------*/
static void GenBrSw(cpcn,o1,o2)
Offset
	o1,
	o2;

int
	cpcn;
{
int
	f;

	cpcn<<=1;

	cpcn++;
	f=FindPc(cpcn,PcSet);
	if(f) fprintf(Output,"   switch ( %s%d ) {\n",PCNAME,cpcn);
	GenChildB(f,cpcn,IAdd(o1));

	cpcn++;
	f=FindPc(cpcn,PcSet);
	if(f) fprintf(Output,"   switch ( %s%d ) {\n",PCNAME,cpcn);
	GenChildB(f,cpcn,IAdd(o2));
}
/*--------------------------------------------------*/
static void
	GenTstSw();

static void GenChildT(f,cpcn,cinst)
int
	f;

char
	*cinst;

int
	cpcn;
{
char
	*lab;

Inst
	IBuff;

	FOREVER {

		lab=cinst;
		GetChrAndMove(cinst,IBuff.h);

		if(Labelized(IBuff.h)) { UnLabelize(IBuff.h); }

		switch(IBuff.h) {

		case IC_JOI :
			SkipSyn(cinst);
			break;

		case IC_WHE :
			SkipSyn(cinst); SkipOff(cinst); SkipOff(cinst);
			break;

		case IC_HLT :
			if(f) fprintf(Output,"   }\n\n");
			return;

		case IC_BNG :
			SkipInt(cinst); SkipOff(cinst);
			break;

		case IC_BRK :
			SkipOff(cinst);
			break;

		case IC_RTN :
		case IC_BND :
			break;

		case IC_BBR :
			SkipInt(cinst);
			break;

		case IC_IFG :
			if(f) {
				fprintf(Output,"   case 0x%lx :\n",IOff(lab));
				GetOffAndMove(cinst,IBuff.b.i.o0);
				GetOffAndMove(cinst,IBuff.b.i.e0.beg);
				GetIntAndMove(cinst,IBuff.b.i.e0.size);
				fprintf(Output,"     if ("); EPrint(Output,IBuff.b.i.e0.beg,IBuff.b.i.e0.size,DTEST);
				fprintf(Output,") %s%d = 0x%lx ;",PCNAME,cpcn,IBuff.b.i.o0);
				fprintf(Output," else %s%d = 0x%lx ;\n",PCNAME,cpcn,IOff(cinst));
				fprintf(Output,"     break ;\n");
			}
			else {
				SkipOff(cinst); SkipOff(cinst); SkipInt(cinst);
			}
			break;

		case IC_SWH :
			if(f) {
				char h; int size; Offset o;
				fprintf(Output,"   case 0x%lx :\n",IOff(lab));
				GetIntAndMove(cinst,IBuff.b.s.i0);
				GetOffAndMove(cinst,IBuff.b.s.e0.beg);
				GetIntAndMove(cinst,IBuff.b.s.e0.size);
				fprintf(Output,"     switch ("); EPrint(Output,IBuff.b.s.e0.beg,IBuff.b.s.e0.size,DEXP); fprintf(Output,") {\n");
				while(IBuff.b.s.i0--) {
					GetChrAndMove(cinst,h);
					GetOffAndMove(cinst,o);
					GetIntAndMove(cinst,size);
					if(o!=NULLOFF) { fprintf(Output,"     case ("); EPrint(Output,o,size,DEXP); fprintf(Output,") :"); }
					else { fprintf(Output,"     default :"); }
					GetOffAndMove(cinst,o);
					if(o!=NULLOFF) fprintf(Output,"\n      %s%d = 0x%lx ;\n      break ;",PCNAME,cpcn,o);
					fprintf(Output,"\n");
				}
				fprintf(Output,"     }\n");
				fprintf(Output,"     break ;\n");
			}
			else {
				GetIntAndMove(cinst,IBuff.b.s.i0); SkipOff(cinst); SkipInt(cinst);
				while(IBuff.b.s.i0--) { SkipChr(cinst); SkipOff(cinst); SkipInt(cinst); SkipOff(cinst); }
			}
			break;

		case IC_FRK :
			GetOffAndMove(cinst,IBuff.b.f.o0);
			SkipInt(cinst);
			GetOffAndMove(cinst,IBuff.b.f.o1);
			SkipInt(cinst);
			GenChildT(f,cpcn,IAdd(IBuff.b.f.o1));
			GenTstSw(cpcn,IOff(cinst),IBuff.b.f.o0);
			return;

		case IC_GTO :
			SkipOff(cinst);
			break;

		case IC_ERT :
		case IC_EXP :
			SkipOff(cinst); SkipInt(cinst);
			break;

		default :
			fprintf(

			stderr,
			"%s : bad instruction code in %s during GenTstBrSw() <%d>, fatal error.\n",
			Me,
			FileName(CODEFN,TMPEXT),
			(int)IBuff.h

			);
			LeaveApp(1);
		}
	}
}
/*--------------------------------------------------*/
static void GenTstSw(cpcn,o1,o2)
Offset
	o1,
	o2;

int
	cpcn;
{
int
	f;

	cpcn<<=1;

	cpcn++;
	f=FindPc(cpcn,PcSet);
	if(f) fprintf(Output,"   switch ( %s%d ) {\n",PCNAME,cpcn);
	GenChildT(f,cpcn,IAdd(o1));

	cpcn++;
	f=FindPc(cpcn,PcSet);
	if(f) fprintf(Output,"   switch ( %s%d ) {\n",PCNAME,cpcn);
	GenChildT(f,cpcn,IAdd(o2));
}
/*------------------------------------------------*/
void PcLookUp(pc,bt)
int
	pc;
	
PBtNodePt
	*bt;
{
char
	cnpos,
	unpos;

PBtNodePt
	*cn,	/* Noeud courant. */
	un,		/* Oncle du noeud courant. */
	fa,		/* Pere du noeud courant. */
	*gfa;	/* Grand pere du noeud courant. */

	/* Recherche du point d'insertion eventuel. */
	StkInit(PcStk,SBTSTKSIZE);
	cn=bt;
	while(*cn!=(PBtNodePt)NULL) {

		if(StkFull(PcStk)) FatalMemErr("stack overflow in PcLookUp()");
		else StkPush(PcStk,cn);

		if(pc==(*cn)->pc) return;
		else if(pc<(*cn)->pc) cn=(&(*cn)->l);
		else cn=(&(*cn)->r);
	}

	/*
	   Ici on connait le point d'insertion et l'element n'existait pas,
	   donc on insere.
	*/
	if((*cn=(PBtNodePt)XMalloc(sizeof(PBtNode)))==(PBtNodePt)NULL) FatalMemErr("malloc() error in PcLookUp()");
	(*cn)->pc=pc;
	(*cn)->l=(*cn)->r=(PBtNodePt)NULL;
	(*cn)->color=WHITE;

	/* Reequilibrage de l'arbre. */
	FOREVER {
		/* Si pas de pere dans pile, cn==la racine donc on a fini. */
		if(StkEmpty(PcStk)) { (*cn)->color=BLACK; break; }
		
		/* Recuperation du pere du noeud courant, s'il est noir, c'est fini. */
		fa=(*(StkPop(PcStk)));
		if(fa->color==BLACK) break;
		
		/*
		   Si le noeud courant n'a pas de grand pere, c'est un fils
		   direct de la racine, donc on lui laisse sa couleur,
		   mais on a fini.
		*/
		if(StkEmpty(PcStk)) break;
		
		/* Recuperation du grand pere du noeud courant, s'il est blanc, c'est fini. */
		gfa=StkPop(PcStk);
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
		if(un!=(PBtNodePt)NULL && un->color==WHITE) {

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
}
/*------------------------------------------------*/
int FindPc(pc,cn)
int
	pc;
	
PBtNodePt
	cn;
{
	while(cn!=(PBtNodePt)NULL) {
		if(pc==cn->pc) return(1);
		else if(pc<cn->pc) cn=cn->l;
		else cn=cn->r;
	}
	return(0);
}
/*--------------------------------------------------*/
char **SGetCState(state,csp,n,nt)
SNodePt
	state;

char
	**csp;

int
	nt,
	*n;
{
	switch(state->type) {

	case SN_LEAF :
		*csp++=state->b.i;
		if(*n<CSTATESIZE) (*n)+=1;
		else {
			fprintf(stderr,"%s : Fatal error, CState[] table overflow in SGetCState(), quit.\n",Me);
			LeaveApp(1);
		}
		PcLookUp(nt,&PcSet);
		break;

	case SN_NODE :
		csp=SGetCState(state->b.s.l,csp,n,(nt<<1)+1);
		csp=SGetCState(state->b.s.r,csp,n,(nt<<1)+2);
		break;

	}

	return(csp);
}
/*--------------------------------------------------*/
static SListElemPt *GenBranchTree(mode,slip,model,modlab,cs,cur,n)
SListElemPt
	*slip;

SNodePt
	model;

char
	*cs[];

int
	mode,
	modlab,
	cur,
	n;
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
			slip=GenBranchTree(mode,slip,model,modlab,cs,cur+1,n);
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
				if(mode==0) {

					PLCopy(TS,TState,cur); PLCopy(&TS[cur],&cs[cur],n-cur);
	
					cslab=0; istate=BuildState(model,TS,&cslab);
	
					ExtRed(istate);
	
					if(istate->type==SN_LEAF) {
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
							SFree(istate);
							mode=1;
							goto btfact_end;
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
								SInsert(istate,&StateSet,NextLabel++);
							}

							mode=1;
							goto btfact_end;
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
						SFree(istate);
						mode=1;
					}
					else {
						cslab=NextLabel++;
					}

				}

				btfact_end:
#endif

				/* Recuperation de l'instruction. */
				GetChrAndMove(c,IBuff.h);

				switch(IBuff.h) {

				case IC_IFG :
				case Labiz(IC_IFG) : /* Un simple if(). */
					GetOffAndMove(c,IBuff.b.i.o0); GetOffAndMove(c,IBuff.b.i.e0.beg); GetIntAndMove(c,IBuff.b.i.e0.size);
					/* construction de la suite de l'etat pour la branche "then". */
					TState[cur]=IAdd(IBuff.b.i.o0);
					slip=GenBranchTree(mode,slip,model,modlab,cs,cur+1,n);
					/* construction de la suite de l'etat pour la branche "else". */
					TState[cur]=c;
					slip=GenBranchTree(mode,slip,model,modlab,cs,cur+1,n);
					break;

				case IC_SWH :
				case Labiz(IC_SWH) : { /* Un switch(). */
					char h; int size; Offset o;
					GetIntAndMove(c,IBuff.b.s.i0); GetOffAndMove(c,IBuff.b.s.e0.beg); GetIntAndMove(c,IBuff.b.s.e0.size);
					/* construction de la suite de l'etat pour toutes les branches. */
					while(IBuff.b.s.i0--) {
						GetChrAndMove(c,h); GetOffAndMove(c,o); GetIntAndMove(c,size); GetOffAndMove(c,o);
						if(o!=NULLOFF) {
							TState[cur]=IAdd(o);
							slip=GenBranchTree(mode,slip,model,modlab,cs,cur+1,n);
						}
					}
					}
					break;

				}

#ifdef BTFACT
				if(mode==0) SInsert(istate,&BrTreeStateSet,cslab);
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

		if(mode==1) {
			if(cslab!=0) { SFree(istate); }
#ifdef DEBUG
			else {
				PrintState(
					istate,
					-1,
					"------- Strange State Reached (mode==1 in GenBranchTree()) : \n\n",
					"\n------------------------------------------------------------\n"
				);
			}
#endif
		}
		else {

			NTrans+=1;

			if(cslab!=0) { SFree(istate); }
			else {
				if(rep->state->type!=SN_LEAF) {
					*slip=NewSListElem(rep); slip=(&((*slip)->n)); NElem+=1;
				}
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

	/* Generation des actions de l'etat. */
	FOREVER {

		/* Recuperation de l'adresse de chaque instruction. */
		NInst=0;
		SGetCState(sbtn->state,CState,&NInst,0);

		/* Pas de progression pour l'instant. */
		nprog=0;

		/* On boucle sur toutes les instructions. */
		for(i=0;i<NInst;i++) {
	
			/* Passage des instructions. */
			if(*CState[i]==IC_EXP || *CState[i]==Labiz(IC_EXP)) {
				GetChrAndMove(CState[i],IBuff.h); GetOffAndMove(CState[i],IBuff.b.e.beg); GetIntAndMove(CState[i],IBuff.b.e.size);
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
				return;

			}
			else {

				/* Si on a atteind un "return", c'est termine. */
				if(rep->state->type==SN_LEAF) { return; }
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
			return;

		}
		else {

			NStates+=1;

			/* Si on a atteind un "return", c'est termine. */
			if(rep->state->type==SN_LEAF) { return; }
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
	GenBranchTree(0,&SList,sbtn->state,sbtn->lab,CState,0,NInst);

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
GetPcInit(pc,nt,s)
SNodePt
	s;

int
	pc,
	nt;
{
int
	r;

	switch(s->type) {
	case SN_LEAF :
		if(pc==nt) return(IOff(s->b.i));
		else return(0);

	case SN_NODE :
		if(pc==nt) return(0);
		else if(r=GetPcInit(pc,(nt<<1)+1,s->b.s.l)) return(r);
		return(GetPcInit(pc,(nt<<1)+2,s->b.s.r));
	}
}
/*--------------------------------------------------*/
void GenPcD(pc,is)
PBtNodePt
	pc;

SNodePt
	is;
{
	if(pc!=(PBtNodePt)NULL) {
		fprintf(Output,",\n   %s%d = 0x%x",PCNAME,pc->pc,GetPcInit(pc->pc,0,is));
		GenPcD(pc->l,is);
		GenPcD(pc->r,is);
	}
}
/*--------------------------------------------------*/
void PBtFree(pc)
PBtNodePt
	pc;
{
	if(pc!=(PBtNodePt)NULL) {
		PBtFree(pc->l);
		PBtFree(pc->r);
		XFree(pc,sizeof(*pc));
	}
}
/*--------------------------------------------------*/
void GenPcDecl(pc,is)
PBtNodePt
	pc;

SNodePt
	is;
{
register
	i;

	if(pc==NULL) {
		fprintf(stderr,"%s : strange value for \"PcSet\" (NULL).\n",Me);
		LeaveApp(1);
	}

	fprintf(Output," register int\n   %s,\n   %s=%d",PFLAGNAME,NTNAME,GetNT(is));
	GenPcD(pc,is);
	fprintf(Output,";\n\n");
}
/*--------------------------------------------------*/
void GenDeclPc(f,pc)
FILE
	*f;

PBtNodePt
	pc;
{
	if(pc!=(PBtNodePt)NULL) {
		fprintf(f,"  %s%d\n",PCNAME,pc->pc);
		GenDeclPc(f,pc->l);
		GenDeclPc(f,pc->r);
	}
}
/*--------------------------------------------------*/
void GenST()
{
char
	chr;

	/* Ouverture du fichier des transitions speciales. */
	if((TransFile=fopen(TstOptResult,"r+"))==NULL) {
		fprintf(stderr,"%s : can't open <%s>, quit.\n",Me,TstOptResult);
		LeaveApp(1);
	}

	/* Ecriture du fichier transitions speciales. */
	while(RdFile(&chr,1,1,TransFile)) WrFile(&chr,1,1,Output);

	fclose(TransFile);
	unlink(TstOptResult);
}
/*--------------------------------------------------*/
void GenSFork(o1,o2,h,isb,isbe,esa)
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
SNodePt
	state;

SBtNodePt
	rep;

	NFBTree=0;
	NFSList=0;
	NTrans=0;
	NStates=1;

	/* Initialisation de l'ensemble des etats. */
	StateSet=(SBtNodePt)NULL;
	BrTreeStateSet=(SBtNodePt)NULL;

	PcSet=(PBtNodePt)NULL;

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
	ISo1=IAdd(o1);
	ISo2=IAdd(o2);
	NIState=0;

	BuildTstOptCmd();
	if((TransFile=popen(TstOptCmd,"w"))==NULL) {
		fprintf(stderr,"%s : can't pipe result file to tstopt command, quit.\n",Me);
		LeaveApp(1);
	}

	/* Insertion de l'etat initial dans l'ensemble des etats. */
	if(!SLookUp(state,&StateSet,&rep)) {
		if(rep->state->type==SN_LEAF) GenReturn(Output,rep->state->b.i);
		else {

			/* Evaluation de l'automate. */
			GenState(rep);
			if(NIState>1) {
				state=BuildInitState(o1,o2,isb,isbe);
				if(SCmp(state,rep->state)!=0) GenTrans(state,rep->state);
				SFree(state);
			}
			GenDeclPc(TransFile,PcSet);
			fprintf(TransFile,"  %s\n",NTNAME);
			if(pclose(TransFile)) { fprintf(stderr,"%s : problem while tstopt command, quit.\n",Me); LeaveApp(1); }
			SBtFree(BrTreeStateSet);

			/* Generation du debut de block. */
			fprintf(Output," { /* || begin */\n");

			/* Generation des declarations de PC. */
			GenPcDecl(PcSet,rep->state);
			SBtFree(StateSet);

			/* Generation de la boucle principale. */
			fprintf(Output,"\n for(;;) {\n\n");

			/* Recopie du fichier des transitions speciales. */
			GenST();
			fprintf(Output,"\n");

			/* Generation des switchs d'actions. */
			fprintf(Output,"   %s = 0 ;\n\n",PFLAGNAME);
			GenActSw(0,o1,o2);

			/* Generation du test de progression. */
			fprintf(Output,"   if ( %s != 0 ) continue ;\n\n",PFLAGNAME);

			/* Generation des switchs de branchement. */
			GenBrSw(0,o1,o2);

			/* Generation du test de progression. */
			fprintf(Output,"   if ( %s == %s ) continue ;\n\n",PFLAGNAME,NTNAME);

			/* Generation des switchs de tests. */
			GenTstSw(0,o1,o2);

			/* Generation de la fin de boucle principale. */
			fprintf(Output," }\n\n");

			/* Generation de l'etiquette de l'etat terminal. */
			if(EndBtNode.lab!=ENDLABELNOTREACHED) {
				fprintf(Output,LFmt,IOff(esa)); fprintf(Output," : ;\n");
				NStates+=1;
			}

			/* Generation de la fin de block. */
			fprintf(Output,"\n } /* || end */\n",(long)IOff(esa));

		}
	}
	else if(pclose(TransFile)) { fprintf(stderr,"%s : problem while tstopt command, quit.\n",Me); LeaveApp(1); }

	PBtFree(PcSet);

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
