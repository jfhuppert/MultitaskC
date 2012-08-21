/*--------------------------------------------------*/
/*
	symset.c
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
#include <symset.h>
#include <general.h>
#include <structs.h>
#include <pcsynan.h>
#include <errno.h>
#include <string.h>

extern void
	XFree(),
	SetOffset();

extern char
	CurrentFileName[],
	LastToken[],
	*RBRealloc(),
	*XMalloc(),
	*Me;

extern FILE
	*ptreef;

extern int
#ifdef DEBUG
	DebugInfo,
#endif
	PcLineNo,
	SwitchFlag,
	Warn,
	LineNo;

extern SymSet
	EmptySymSet;

extern StkDecl(SymSet,BNSTKSIZE)
	SSStk;

extern StkDecl(RBuf,BNSTKSIZE)
	RBStk;

extern Offset
	GetOffset();

extern FILE
	*PcStatement,
	*PcResult;

int
	NextBLKId,
	NextMeetingId;

static StkDecl(BtNodePt *,BTSTKSIZE)
	Stk;

TaState
	BlkSymSet,
	SyncSymSet,
	GlobalSymSet,
	LocalSymSet;
	
/*--------------------------------------------------*/
void FatalMemErr(str)
char
	*str;
{
	fprintf(
		stderr,
		"%s : fatal memory allocation error <%s>, quit.\n",
		Me,
		str
	);
	LeaveApp(errno);
}
/*------------------------------------------------*/
/*---- Fonction de recherche d'une transition ----*/
/*------------------------------------------------*/
static TaStatePt TransNext(chr,bt)
char
	chr;
	
BtNodePt
	*bt;
{
	/* Recherche d'une transition avec chr. */
	while(*bt!=(BtNodePt)NULL) {
		
		if(chr==(*bt)->chr) {
			return((*bt)->next);
		}

		if(chr<(*bt)->chr) bt=(&(*bt)->l);
		else bt=(&(*bt)->r);
	}
	
	return((TaStatePt)NULL);
}
/*------------------------------------------------*/
/*--- Fonction d'insertion d'une transition ------*/
/*------------------------------------------------*/
static TaStatePt TransLookUp(chr,bt,flg)
char
	chr;
	
BtNodePt
	*bt;

int
	*flg;
{
char
	cnpos,
	unpos;

TaStatePt
	retval;
	
BtNodePt
	*cn,	/* Noeud courant. */
	un,	/* Oncle du noeud courant. */
	fa,	/* Pere du noeud courant. */
	*gfa;	/* Grand pere du noeud courant. */

	*flg=0;
	
	/* Recherche du point d'insertion eventuel. */
	StkInit(Stk,BTSTKSIZE);
	cn=bt;
	while(*cn!=(BtNodePt)NULL) {

		if(StkFull(Stk)) FatalMemErr("stack overflow in TransLookUp()");
		else StkPush(Stk,cn);

		if(chr==(*cn)->chr) return((*cn)->next);
		if(chr<(*cn)->chr) cn=(&(*cn)->l);
		else cn=(&(*cn)->r);
	}
	
	/*
	   Ici on connait le point d'insertion et l'element n'existait pas,
	   donc on insere.
	*/
	if((*cn=(BtNodePt)XMalloc(sizeof(BtNode)))==(BtNodePt)NULL) FatalMemErr("malloc1() error in TransLookUp()");
	(*cn)->chr=chr;
	(*cn)->l=(*cn)->r=(BtNodePt)NULL;
	(*cn)->color=WHITE;
	if(((*cn)->next=(TaStatePt)XMalloc(sizeof(TaState)))==(TaStatePt)NULL) FatalMemErr("malloc2() error in TransLookUp()");
	retval=(*cn)->next;
	*flg=1;
	
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
		if(un!=(BtNodePt)NULL && un->color==WHITE) {

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
	
	return(retval);
}
/*-------------------------------------------------*/
static void BtFree(btroot,flg)
BtNodePt
	btroot;

int
	flg;
{
void
	TaFree();

	if(btroot!=(BtNodePt)NULL) {
		BtFree(btroot->l,flg);
		BtFree(btroot->r,flg);
		TaFree(btroot->next,flg);
		XFree(btroot,sizeof(*btroot));
	}
}
/*-----------------------------------------------------------*/
void TaFree(taroot,flg)
TaStatePt
	taroot;

int
	flg;
{
	if(taroot!=(TaStatePt)NULL) {

		BtFree(taroot->tl,flg);

		if(flg==1 && taroot->value.lval!=(LabelPt)NULL) {
			if(taroot->value.lval->nlink==0) {
			LabelPt
				ptr=(LabelPt)NULL;

				taroot->value.lval->node+=(Offset)PTNHDRSIZE;
				SetOffset(ptreef,taroot->value.lval->node,F_BEG);
				WLabP(ptr,ptreef);
				XFree(taroot->value.lval,sizeof(*taroot->value.lval));
			}
			else {
				taroot->value.lval->node=NULLOFF;
				taroot->value.lval->state=NULLOFF;
			}
		}

		if(flg==2 && taroot->value.sval!=NULL)
			XFree(taroot->value.sval,sizeof(*taroot->value.sval));

		XFree(taroot,sizeof(*taroot));
	}
}
/*-----------------------------------------------------------*/
/* Fonction de liberation des lexemes d'un automate en arbre */
/*-----------------------------------------------------------*/
void FTaFree(taroot,flg)
TaStatePt
	taroot;

int
	flg;
{
	BtFree(taroot->tl,flg);
}
/*-----------------------------------------------------------*/
/* Fonction d'insertion d'un lexeme dans l'automate en arbre */
/*-----------------------------------------------------------*/
TaValPt PTaInsert(token,curs,flg,value)
char
	*token;
	
TaStatePt
	curs;
	
int
	*flg;

TaValPt
	value;
{
int
	created;
	
	*flg=0;
	
	while(*token!='\0') {
	
		curs=TransLookUp(*token,&curs->tl,&created);
		token++;
				
		if(created) {
			while(*token!='\0') {;
				curs->value.ival=0;
				BtInit(curs->tl);
				curs=TransLookUp(*token,&curs->tl,&created);
				token++;
			}
			
			curs->value=(*value);
			BtInit(curs->tl);
			return(&curs->value);
		}
	}
	
	if(UnKownValue(curs->value)) { curs->value=(*value); return(&curs->value); }
	else { *flg=1; (*value)=curs->value; return(&curs->value); }
}
/*-----------------------------------------------------------*/
/* Fonction d'insertion d'un lexeme dans l'automate en arbre */
/*-----------------------------------------------------------*/
void TaInsert(token,curs,flg,value)
char
	*token;
	
TaStatePt
	curs;
	
int
	*flg;

TaValPt
	value;
{
int
	created;
	
	*flg=0;
	
	while(*token!='\0') {
	
		curs=TransLookUp(*token,&curs->tl,&created);
		token++;
				
		if(created) {
			while(*token!='\0') {;
				curs->value.ival=0;
				BtInit(curs->tl);
				curs=TransLookUp(*token,&curs->tl,&created);
				token++;
			}
			
			curs->value=(*value);
			BtInit(curs->tl);
			return;
		}
	}
	
	if(UnKownValue(curs->value)) { curs->value=(*value); return; }
	else { *flg=1; (*value)=curs->value; return; }
}
/*------------------------------------------------------------*/
/* Fonction de recherche d'un lexeme dans l'automate en arbre */
/*------------------------------------------------------------*/
TaValPt PTaLookUp(chr,curs)
char
	*chr;
	
TaStatePt
	curs;
{
TaStatePt
	lasts;
	
	lasts=curs;
	while(*chr) {
		curs=TransNext(*chr,&curs->tl);
		if(curs==(TaStatePt)NULL) break;
		chr+=1;
		lasts=curs;	
	}

	return((*chr)?(TaValPt)NULL:&lasts->value);
}
/*------------------------------------------------------------*/
/* Fonction de recherche d'un lexeme dans l'automate en arbre */
/*------------------------------------------------------------*/
TaVal TaLookUp(chr,curs)
char
	*chr;
	
TaStatePt
	curs;
{
TaVal
	ret;

TaStatePt
	lasts;
	
	lasts=curs;
	while(*chr) {
		curs=TransNext(*chr,&curs->tl);
		if(curs==(TaStatePt)NULL) break;
		chr+=1;
		lasts=curs;	
	}

	ret.ival=0;
	return((*chr)?ret:lasts->value);
}
/*------------------------------------------------*/
void SyncPointInsert(name,card)
char
	*name;

int
	card;
{
int
	notinserted;

TaVal
	tokenvalue;

	tokenvalue.sval=(SyncPPt)XMalloc(sizeof(SyncP));
	if(tokenvalue.sval==(SyncPPt)NULL) FatalMemErr("malloc() error in SyncPointInsert()");
	tokenvalue.sval->card=card;
	tokenvalue.sval->id=NextMeetingId++;
	tokenvalue.sval->n=0;
	TaInsert(name,&SyncSymSet,&notinserted,&tokenvalue);

	if(notinserted) {
		fprintf(stderr,"%s : redeclaration of meeting <%s> near line %d, quit.\n",Me,name,LineNo);
		LeaveApp(1);
	}

#ifdef DEBUG
	if(DebugInfo)
		fprintf(stdout,"New Meeting <%s>, cardinality %d, id %d.\n",name,card,tokenvalue.sval->id);
#endif

}
/*------------------------------------------------*/
SyncPPt SyncPointFind(name)
char
	*name;
{
TaVal
	tokenvalue;

	tokenvalue.sval=TaLookUp(name,&SyncSymSet).sval;

	if(tokenvalue.sval==(SyncPPt)NULL) {
		fprintf(stderr,"%s : meeting not declared <%s> near line %d, quit.\n",Me,name,LineNo);
		LeaveApp(1);
	}

#ifdef DEBUG
	if(DebugInfo)
		fprintf(stdout,"Meeting <%s> used line %d, cardinality %d, id %d.\n",name,LineNo,tokenvalue.sval->card,tokenvalue.sval->id);
#endif

	/* On retourne le point de sync. */
	return(tokenvalue.sval);
}
/*------------------------------------------------*/
int BlkNameLookUp(name)
char
	*name;
{
int
	notinserted;

TaVal
	tokenvalue;

	tokenvalue.ival=NextBLKId;
	TaInsert(name,&BlkSymSet,&notinserted,&tokenvalue);

	if(notinserted) {
#ifdef DEBUG
		if(DebugInfo)
			fprintf(stdout,"Block Name <%s> is reused line %d, id %d.\n",name,LineNo,tokenvalue.ival);
#endif
	}
	else {
		NextBLKId++;
#ifdef DEBUG
		if(DebugInfo)
			fprintf(stdout,"New Block Name <%s> line %d, id %d.\n",name,LineNo,tokenvalue.ival);
#endif
	}

	return(tokenvalue.ival);
}
/*------------------------------------------------*/
int StrSize(n)
int
	n;
{
int
	ss=1;

	while(n>=10) n/=10,ss++;
	return(ss);
}
/*------------------------------------------------*/
void RBPrefix(rb,bn)
RBufPt
	rb;

int
	bn;
{
	if(bn==SS_NREN) {
		rb->siz=0;
		rb->buf=XMalloc(1);
		if(rb->buf==NULL) FatalMemErr("XMalloc(1) in RBPrefix()");
		*rb->buf='\0';
	}
	else {
		rb->siz=(StrSize(bn))+2;
		rb->buf=XMalloc(rb->siz+1);
		if(rb->buf==NULL) FatalMemErr("XMalloc(rb->siz+1) in RBPrefix()");
		sprintf(rb->buf,"_%d_",bn);
	}
}
/*------------------------------------------------*/
void RenameId(id,f)
IdentPt 
    id;

int
	f;
{
RBuf
	nrbuf,
	prbuf;

#ifdef DEBUG
	if(DebugInfo) fprintf(stdout,"renaming for \"%s\", current Memory Output is \"%s\"\n",id->s,StkTop(RBStk).buf);
#endif

	prbuf=StkPop(RBStk);

	RBPrefix(&nrbuf,id->bn);
	RBRealloc(&nrbuf,prbuf.siz);
	strcat(nrbuf.buf,prbuf.buf);
	XFree(prbuf.buf,prbuf.siz+1);

	if(StkEmpty(RBStk)) {
		if(nrbuf.siz>=ITMAXLEN) FatalMemErr("RenameId() and StkEmpty(RBStk)");
		strcpy(LastToken,nrbuf.buf);
	}
	else {
		RBRealloc(&StkTop(RBStk),nrbuf.siz);
		strcat(StkTop(RBStk).buf,nrbuf.buf);
	}

	XFree(nrbuf.buf,nrbuf.siz+1);
	if(f) XFree(id->s,strlen(id->s)+1);
}
/*------------------------------------------------*/
void SUELookUp(id,mode)
IdentPt
    id;

int
	mode;
{
TaVal
	tokenvalue;

SymSetPt
	symSet;

	symSet=StkSP(SSStk);

	for(symSet=StkSP(SSStk);!StkEP(SSStk,symSet);symSet++) {
		switch(mode) {
		case 0 : tokenvalue.ival=TaLookUp(id->s,&symSet->SSSet).ival; break;
		case 1 : tokenvalue.ival=TaLookUp(id->s,&symSet->USSet).ival; break;
		case 2 : tokenvalue.ival=TaLookUp(id->s,&symSet->ESSet).ival; break;
		}
		switch(tokenvalue.ival) {
		case 0 : break;
		case T_SIDENTIF : id->t=(symSet==StkSP(SSStk)?L_SIDENT:G_SIDENT); id->bn=symSet->SSNum; return;
		case T_UIDENTIF : id->t=(symSet==StkSP(SSStk)?L_UIDENT:G_UIDENT); id->bn=symSet->SSNum; return;
		case T_EIDENTIF : id->t=(symSet==StkSP(SSStk)?L_EIDENT:G_EIDENT); id->bn=symSet->SSNum; return;
		default :
			fprintf(stderr,"%s : invalid token type <%d> in SUELookUp().\n",Me,tokenvalue.ival);
			LeaveApp(1);
			break;
		}
	}

	id->t=U_IDENT;
	id->bn=SS_NREN;
}
/*------------------------------------------------*/
void SUId(id,un,def)
IdentPt
	id;

int
	un,
	def;
{
int
	notinserted;

TaVal
	tokenvalue;

	SUELookUp(id,un); /* un==0 -> STRUCT, un==1 -> UNION. */

	if(id->t==U_IDENT) {
		tokenvalue.ival=un?T_UIDENTIF:T_SIDENTIF;
		TaInsert(id->s,un?&StkTop(SSStk).USSet:&StkTop(SSStk).SSSet,&notinserted,&tokenvalue);
		id->bn=StkTop(SSStk).SSNum;
	}

#ifdef DEBUG
	if(un) IdentInfo(id,PcLineNo,"union identifier",id->bn,CurrentFileName);
	else IdentInfo(id,PcLineNo,"struct identifier",id->bn,CurrentFileName);
#endif

	RenameId(id,1);
}
/*------------------------------------------------*/
void EId(id,def)
IdentPt
	id;

int
	def;
{
int
	notinserted;

TaVal
	tokenvalue;

	SUELookUp(id,2);

	if(id->t==U_IDENT) {
		tokenvalue.ival=T_EIDENTIF;
		TaInsert(id->s,&StkTop(SSStk).ESSet,&notinserted,&tokenvalue);
		id->bn=StkTop(SSStk).SSNum;
	}

#ifdef DEBUG
	IdentInfo(id,PcLineNo,"enum identifier",id->bn,CurrentFileName);
#endif

	RenameId(id,1);
}
/*------------------------------------------------*/
void IdUsed(id)
IdentPt
	id;
{
	switch(id->t) {
	case G_FIDENT : /* Pas de renomage (id->bn==SS_NREN) : Identificateur de fonction declare au dessus. */
	case L_FIDENT : /* Pas de renomage (id->bn==SS_NREN) : Identificateur de fonction declare en local. */
	case G_IDENT : /* Renomage en fonction de id->bn : Identificateur declare au dessus. */
	case L_IDENT : /* Renomage en fonction de id->bn : Identificateur declare en local. */
		break;
	case U_IDENT : /* Pas de renomage : Warning : Identificateur non declare. */
		if(Warn) fprintf(stderr,"%s : warning, unknown identifier <%s> line %d.\n",Me,id->s,PcLineNo);
		break;
	case U_FIDENT : /* Erreur : impossible normalement. */
	case L_MIDENT : /* Erreur : Identificateur de meeting deja declare en local. */
	case G_MIDENT : /* Erreur : Identificateur de meeting deja declare au dessus. */
		fprintf(stderr,"%s : illegal use of <%s> line %d.\n",Me,id->s,PcLineNo);
		LeaveApp(1);
	}

#ifdef DEBUG
	IdentInfo(id,PcLineNo,"identifier used",id->bn,CurrentFileName);
#endif

	RenameId(id,1);
}
/*------------------------------------------------*/
void MIdUsed(id)
IdentPt
	id;
{
	switch(id->t) {
	case G_MIDENT : /* Renomage en fonction de id->bn : Identificateur de meeting declare au dessus. */
	case L_MIDENT : /* Renomage en fonction de id->bn : Identificateur de meeting declare en local. */
		break;
	case G_FIDENT : /* Erreur : Identificateur de fonction declare au dessus. */
	case L_FIDENT : /* Erreur : Identificateur de fonction declare en local. */
	case G_IDENT : /* Erreur : Identificateur declare au dessus. */
	case L_IDENT : /* Erreur : Identificateur declare en local. */
	case U_IDENT : /* Erreur : Identificateur non declare. */
	case U_FIDENT : /* Erreur : impossible normalement. */
		fprintf(stderr,"%s : illegal use of <%s> line %d.\n",Me,id->s,PcLineNo);
		LeaveApp(1);
	}

#ifdef DEBUG
	IdentInfo(id,PcLineNo,"meeting used",id->bn,CurrentFileName);
#endif

	RenameId(id,1);
}
/*------------------------------------------------*/
void TIdUsed(id)
IdentPt
	id;
{
	/* Renomage en fonction de id->bn. */

#ifdef DEBUG
	IdentInfo(id,PcLineNo,"typename used",id->bn,CurrentFileName);
#endif

	RenameId(id,1);
}
/*------------------------------------------------*/
void MIdDecl(id)
IdentPt
	id;
{
int
	notinserted;

TaVal
	tokenvalue;

	switch(id->t) {
	case G_FIDENT : /* Renomage en fonction de StkTop(SSStk).SSNum : Identificateur de fonction declare au dessus, redeclare ici. */
	case G_IDENT : /* Renomage en fonction de StkTop(SSStk).SSNum : Identificateur declare au dessus, redeclare ici. */
	case G_MIDENT : /* Renomage en fonction de StkTop(SSStk).SSNum : Identificateur de meeting declare au dessus, redeclare ici. */
	case U_IDENT : /* Renomage en fonction de StkTop(SSStk).SSNum : Identificateur non declare. */
		break;
	case L_FIDENT : /* Erreur : Identificateur de fonction declare en local. */
	case U_FIDENT : /* Erreur : Impossible normalement a cause de la syntaxe. */
	case L_MIDENT : /* Erreur : Identificateur de meeting deja declare en local. */
	case L_IDENT : /* Erreur : Identificateur deja declare en local. */
		fprintf(stderr,"%s : redeclaration of <%s> line %d.\n",Me,id->s,PcLineNo);
		LeaveApp(1);
	}

#ifdef DEBUG
	IdentInfo(id,PcLineNo,"meeting declaration",StkTop(SSStk).SSNum,CurrentFileName);
#endif

	tokenvalue.ival=T_MIDENTIF;
	TaInsert(id->s,&StkTop(SSStk).SymSet,&notinserted,&tokenvalue);

	id->bn=StkTop(SSStk).SSNum;
	RenameId(id,1);
}
/*------------------------------------------------*/
void SIdDecl(id)
IdentPt
	id;
{
int
	notinserted;

TaVal
	tokenvalue;

	if(id->t<0) id->t=(-id->t);

	switch(id->t) {
	case G_FIDENT : /* Renomage en fonction de StkTop(SSStk).SSNum==SS_NREN : Identificateur de fonction declare au dessus, redeclare ici. */
	case G_IDENT : /* Renomage en fonction de StkTop(SSStk).SSNum==SS_NREN : Identificateur declare au dessus, redeclare ici. */
	case G_MIDENT : /* Renomage en fonction de StkTop(SSStk).SSNum==SS_NREN : Identificateur de meeting declare au dessus, redeclare ici. */
	case U_IDENT : /* Renomage en fonction de StkTop(SSStk).SSNum==SS_NREN : Identificateur non declare. */
		break;
	case L_FIDENT : /* Erreur : impossible car on refuse les U_FIDENT. */
	case U_FIDENT : /* Erreur : pas de fonction dans une structure/union. */
	case L_MIDENT : /* Erreur : impossible avec la syntaxe. */
	case L_IDENT : /* Erreur : Identificateur deja declare en local. */
		fprintf(stderr,"%s : redeclaration of <%s> line %d.\n",Me,id->s,PcLineNo);
		LeaveApp(1);
	}

#ifdef DEBUG
	IdentInfo(id,PcLineNo,"struct/union member declaration",StkTop(SSStk).SSNum,CurrentFileName);
#endif

	tokenvalue.ival=T_IDENTIF;
	TaInsert(id->s,&StkTop(SSStk).SymSet,&notinserted,&tokenvalue);

	id->bn=StkTop(SSStk).SSNum;
	RenameId(id,1);
}
/*------------------------------------------------*/
void IdDecl(id,te,f)
IdentPt
	id;

int
	te,
	f;
{
int
	bn,
	notinserted;

TaVal
	tokenvalue;

	if(id->t<0) id->t=(-id->t);

	switch(id->t) {
	case U_FIDENT : /* Renomage en fonction de StkTop(SSStk).SSNum si TYPEDEFNAME : Identificateur de fonction non declare. */
		bn=((te&SS_TYP)?((te&SS_EXT)?SS_NREN:StkTop(SSStk).SSNum):SS_NREN);
		tokenvalue.ival=((te&SS_TYP)?T_TYPENAME:T_FIDENTIF);
		break;
	case G_FIDENT : /* Renomage en fonction de StkTop(SSStk).SSNum : Identificateur de fonction declare au dessus, redeclare ici. */
	case G_IDENT : /* Renomage en fonction de StkTop(SSStk).SSNum : Identificateur declare au dessus, redeclare ici. */
	case G_MIDENT : /* Renomage en fonction de StkTop(SSStk).SSNum : Identificateur de meeting declare au dessus, redeclare ici. */
	case U_IDENT : /* Renomage en fonction de StkTop(SSStk).SSNum : Identificateur non declare. */
		bn=(te&SS_EXT)?SS_NREN:StkTop(SSStk).SSNum;
		tokenvalue.ival=((te&SS_TYP)?T_TYPENAME:T_IDENTIF);
		break;
	case L_FIDENT : /* Erreur : Identificateur de fonction deja declare en local. */
	case L_MIDENT : /* Erreur : Identificateur de meeting deja declare en local. */
	case L_IDENT : /* Erreur : Identificateur deja declare en local. */
		if((te&SS_EXT)) {
			bn=SS_NREN;
			tokenvalue.ival=((te&SS_TYP)?T_TYPENAME:T_IDENTIF);
		}
		else {
			fprintf(stderr,"%s : redeclaration of <%s> line %d.\n",Me,id->s,PcLineNo);
			LeaveApp(1);
		}
		break;
	}

#ifdef DEBUG
	IdentInfo(id,PcLineNo,(te&SS_TYP)?"typename declaration":"declaration",bn,CurrentFileName);
#endif

	TaInsert(id->s,&StkTop(SSStk).SymSet,&notinserted,&tokenvalue);

	id->bn=bn;
	RenameId(id,f);
}
/*------------------------------------------------*/
void FIdDecl(id,te)
IdentPt
	id;

int
	te;
{
int
	notinserted;

TaVal
	tokenvalue;

	if((te&SS_TYP)||(te&SS_EXT)) {
		fprintf(stderr,"%s : invalid declaration of function <%s> line %d.\n",Me,id->s,PcLineNo);
		LeaveApp(1);
	}

	if(id->t<0) id->t=(-id->t);

	switch(id->t) {
	case U_FIDENT : /* Pas de renomage : Identificateur de fonction non declare. */
		break;
	case G_FIDENT : /* Erreur : Identificateur de fonction declare au dessus, redeclare ici. */
	case G_IDENT : /* Erreur : Identificateur declare au dessus, redeclare ici. */
	case G_MIDENT : /* Erreur : Identificateur de meeting declare au dessus, redeclare ici. */
	case U_IDENT : /* Erreur : Identificateur non declare. */
	case L_FIDENT : /* Erreur : Identificateur de fonction deja declare en local. */
	case L_MIDENT : /* Erreur : Identificateur de meeting deja declare en local. */
	case L_IDENT : /* Erreur : Identificateur deja declare en local. */
		fprintf(stderr,"%s : invalid declaration <%s> line %d.\n",Me,id->s,PcLineNo);
		LeaveApp(1);
	}

#ifdef DEBUG
	IdentInfo(id,PcLineNo,"function declaration",SS_NREN,CurrentFileName);
#endif

	tokenvalue.ival=T_FIDENTIF;
	TaInsert(id->s,&StkTop(SSStk).SymSet,&notinserted,&tokenvalue);

	id->bn=SS_NREN;
	RenameId(id,1);
}
/*------------------------------------------------*/
char *PrefixId(bn,id)
int
	bn;

char
	*id;
{
static char
	t[BIGBUFF];

	if(bn==SS_NREN) {
		if((1+strlen(id))>BIGBUFF) FatalMemErr("PrefixId()");
		t[0]='\0';
	}
	else {
		if((StrSize(bn)+3+strlen(id))>BIGBUFF) FatalMemErr("PrefixId()");
		sprintf(t,"_%d_",bn);
	}

	return(strcat(t,id));
}
/*--------------------------------------------------*/
void RString(beg,size,output)
Offset
	beg;

int
	size;

FILE
	*output;
{
	fflush(PcResult);
	SetOffset(PcResult,beg,F_BEG);
	while(size--) putc(getc(PcResult),output);
	SetOffset(PcResult,0L,F_END);
}
/*--------------------------------------------------*/
void TRString(beg,size,output,tmp,ido,ids)
Offset
	ido,
	beg;

int
	ids,
	size;

FILE
	*output;

char
	*tmp;
{
	fflush(PcResult);
	SetOffset(PcResult,beg,F_BEG);
	while(GetOffset(PcResult)<ido) putc(getc(PcResult),output),size-=1;
	fprintf(output,"%s",tmp);
	SetOffset(PcResult,(long)ids,F_CUR);
	size-=ids;
	while(size--) putc(getc(PcResult),output);
	SetOffset(PcResult,0L,F_END);
}
/*------------------------------------------------*/
void InitDecl(dso,dss,ds,id,ids,io,is,gr)
Offset
	dso,
	io;

int
	gr,
	ds,
	is,
	dss;

IdentPt
	id;
{
char
	*t=PrefixId(id->bn,id->s);

#ifdef DEBUG
	if(DebugInfo)
		if(StkTop(SSStk).SSNum!=SS_NREN && !(ds&SS_STA)) {
			fprintf(stdout,"init \"%s\" (%s) : ",t,gr?"copy block":"assign");
			RString(dso,dss,stdout);
			RString(id->o,ids,stdout);
			fprintf(stdout," = ");
			RString(io,is,stdout);
			fprintf(stdout,"%s;\n",gr?"}":"");
		}
#endif

	if(StkTop(SSStk).SSNum!=SS_NREN && !(ds&SS_STA)) {
		if(gr || SS_TF(id->f,SS_ARR)) {
			if((ds&SS_TYP)||(ds&SS_EXT)) {
				fprintf(stderr,"%s : invalid initialization of <%s> before line %d.\n",Me,id->s,PcLineNo);
				LeaveApp(1);
			}
			fprintf(PcStatement,"group {{ ");
			RString(dso,dss,PcStatement);
			TRString(id->o,ids,PcStatement,"tmp",id->ido,strlen(t));
			fprintf(PcStatement,"=");
			RString(io,is,PcStatement);
			fprintf(
				PcStatement,
				"%s; register unsigned int i=sizeof(%s); register unsigned char *t=(char *)&(%s),*f=(char *)&tmp; while(i--) *t++=*f++; }} ",
				gr?"}":"",
				t,
				t
			);
		}
		else {
			if((ds&SS_TYP)||(ds&SS_EXT)) {
				fprintf(stderr,"%s : invalid initialization of <%s> before line %d.\n",Me,id->s,PcLineNo);
				LeaveApp(1);
			}
			fprintf(PcStatement,"%s=",t);
			RString(io,is,PcStatement);
			fprintf(PcStatement,"; ");
		}
	}

	XFree(id->s,strlen(id->s)+1);
}
/*------------------------------------------------*/
void CDSize(s,o)
int
	*s;

Offset
	o;
{
	(*s)=(int)(GetOffset(PcResult)-o)+strlen(LastToken)-1;
}
/*------------------------------------------------*/
void CDSSize(s,o,dso)
int
	*s;

Offset
	o,
	dso;
{
	if((*s)==0) (*s)=(int)(o-dso); 
}
/*------------------------------------------------*/
void NSS(nbn)
int
	nbn;
{
#ifdef DEBUG
	if(DebugInfo)
		if(nbn==SS_NREN) fprintf(stdout,"NewSymSet line %d : no renaming\n",PcLineNo);
		else fprintf(stdout,"NewSymSet line %d : block # %d for renaming\n",PcLineNo,nbn);
#endif

	if(StkFull(SSStk)) FatalMemErr("stack overflow in pre-comp");
	EmptySymSet.SSNum=nbn;
	StkPush(SSStk,EmptySymSet);
}
/*------------------------------------------------*/
void KSS()
{
	FTaFree(&StkTop(SSStk).SSSet,0);
	FTaFree(&StkTop(SSStk).USSet,0);
	FTaFree(&StkTop(SSStk).ESSet,0);
	FTaFree(&StkTop(SSStk).SymSet,0);
	StkPop(SSStk);
}
/*------------------------------------------------*/
#ifdef DEBUG
IdentInfo(id,lno,str,bn,cfn)
IdentPt
	id;

int
	lno,
	bn;

char
	*cfn,
	*str;
{
	if(!DebugInfo) return(0);

	fprintf(stdout,"%s line %d of %s : <",str,lno,cfn);
	if(bn!=SS_NREN) fprintf(stdout,"_%d_",bn);
	fprintf(stdout,"%s>",id->s);

	fprintf(stdout,", type ");

	switch(id->t) {
	case U_IDENT :
		fprintf(stdout,"U_IDENT");
		break;
	case L_IDENT :
		fprintf(stdout,"L_IDENT");
		break;
	case G_IDENT :
		fprintf(stdout,"G_IDENT");
		break;
	case U_FIDENT :
		fprintf(stdout,"U_FIDENT");
		break;
	case L_FIDENT :
		fprintf(stdout,"L_FIDENT");
		break;
	case G_FIDENT :
		fprintf(stdout,"G_FIDENT");
		break;
	case L_MIDENT :
		fprintf(stdout,"L_MIDENT");
		break;
	case G_MIDENT :
		fprintf(stdout,"G_MIDENT");
		break;
	case L_SIDENT :
		fprintf(stdout,"L_SIDENT");
		break;
	case G_SIDENT :
		fprintf(stdout,"G_SIDENT");
		break;
	case L_UIDENT :
		fprintf(stdout,"L_UIDENT");
		break;
	case G_UIDENT :
		fprintf(stdout,"G_UIDENT");
		break;
	case L_EIDENT :
		fprintf(stdout,"L_EIDENT");
		break;
	case G_EIDENT :
		fprintf(stdout,"G_EIDENT");
		break;
	default :
		fprintf(stdout,"unknown");
		break;
	}

	fprintf(stdout,"\n");
}
#endif
/*------------------------------------------------------------------------*/
