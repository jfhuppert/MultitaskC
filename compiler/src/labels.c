/*--------------------------------------------------*/
/*
	labels.c
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
#include <strings.h>

/*--------------------------------------------------*/
struct {

	int
		lleng,
		lineno,
		bnode;

	Offset
		node;

} GRec;
/*--------------------------------------------------*/

extern void
	SetOffset(),
	TaInsert();

extern char
	*XMalloc(),
	*FileName(),
	LastToken[],
	*Me;

extern int
#ifdef DEBUG
	DebugInfo,
#endif
	LineNo;

extern TaVal
	TaLookUp();

extern FILE
	*ptreef,
	*gotof;

int
	NextBNNum;

TaState
	LabelSymSet;

/*--------------------------------------------------*/
LabelPt NewLabel(id,symset,level)
char
	*id;

TaStatePt
	symset;

int
	level;
{
int
	notinserted;

TaVal
	tokenvalue;

	tokenvalue.lval=(LabelPt)XMalloc(sizeof(Label));
	if(tokenvalue.lval==(LabelPt)NULL) {
		fprintf(
			stderr,
			"%s : NewLabel() memory allocation error, quit.\n",
			Me
		);
		LeaveApp(1);
	}

	TaInsert(id,symset,&notinserted,&tokenvalue);
	if(notinserted) {
		fprintf(
			stderr,
			"%s : error line %d, label <%s> already defined.\n",
			Me,
			LineNo,
			id
		);
		LeaveApp(1);
	}

	tokenvalue.lval->state=NULLOFF;
	tokenvalue.lval->nlink=0;
	tokenvalue.lval->level=level;

#ifdef DEBUG
	if(DebugInfo)
		fprintf(stdout,
			"New label <%s> at level %d line %d\n",
			id,
			level,
			LineNo
		);
#endif

	return(tokenvalue.lval);
}
/*--------------------------------------------------*/
StoreGoto(label,bnode,lineno,node)
char
	*label;

int
	bnode,
	lineno;

Offset
	node;
{
int
	leng;

	leng=strlen(label);
	WInt(leng,gotof); WInt(bnode,gotof);
	WInt(lineno,gotof); WOff(node,gotof);
	WrFile(label,leng,1,gotof);

#ifdef DEBUG
	if(DebugInfo)
		fprintf(stdout,
			"Go to label <%s> line %d, level %d, tree offset %ld\n",
			label,
			lineno,
			bnode,
			(long)node
		);
#endif
}
/*--------------------------------------------------*/
CheckGotos()
{
TaVal
	token;

	fclose(gotof);

	if((gotof=fopen(FileName(GOTOFN,TMPEXT),"r+"))==NULL) {
		fprintf(stderr,"%s : can't open <%s>, quit.\n",Me,FileName(GOTOFN,TMPEXT));
		LeaveApp(1);
	}

	while(RInt(GRec.lleng,gotof)) {

		RInt(GRec.bnode,gotof); RInt(GRec.lineno,gotof);
		ROff(GRec.node,gotof); RdFile(LastToken,GRec.lleng,1,gotof);
		LastToken[GRec.lleng]='\0';

#ifdef DEBUG
	if(DebugInfo)
		fprintf(stdout,
			"Checking <goto %s>, line %d\n",
			LastToken,
			GRec.lineno
		);
#endif

		token.lval=TaLookUp(LastToken,&LabelSymSet).lval;
		if(token.lval==(LabelPt)NULL) {
			fprintf(
				stderr,
				"%s : impossible <goto %s> (unknown label), line %d, quit.\n",
				Me,
				LastToken,
				GRec.lineno
			);
			LeaveApp(1);
		}

		/*
		On accepte les goto que vers soi-meme.
		*/
		if(GRec.bnode!=token.lval->level) {
			fprintf(
				stderr,
				"%s : impossible <goto %s> (bad scope), line %d, quit.\n",
				Me,
				LastToken,
				GRec.lineno
			);
			LeaveApp(1);
		}

		/*
		   Le goto est possible, donc on augmente le nombre de liens
		   de l'etiquette.
		*/
		token.lval->nlink+=1;

		/* MAJ du noeud du goto dans l'arbre syntaxique. */
		GRec.node+=(Offset)PTNHDRSIZE;
		SetOffset(ptreef,GRec.node,F_BEG);
		WLabP(token.lval,ptreef);

	}

	fclose(gotof);
}
/*--------------------------------------------------*/
