/*---------------------------------------------------------------------------*/
/*
	structs.h
*/
/*---------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------*/
#include <icsn.h>
/*---------------------------------------------------------------------------*/
typedef long Offset;
/*---------------------------------------------------------------------------*/
/* Une action ou une condition du fichier source : une expression C. */
typedef struct {

	Offset
		beg;	/* Offset a partir du debut du fichier. */

	int
		size;	/* Taille en octets. */

} Expr, *ExprPt;
/*---------------------------------------------------------------------------*/
/* Une etiquette de branchement. */
typedef struct lab {

	int
		nlink,
		level;

	Offset
		node,
		state;

} Label, *LabelPt;
/*---------------------------------------------------------------------------*/
/* Un point de synchronisation. */
typedef struct syn {

	int
		n,
		id,
		card;

} SyncP, *SyncPPt;
/*---------------------------------------------------------------------------*/
typedef struct {

	Offset
		first,
		last;

} Seq, *SeqPt;
/*---------------------------------------------------------------------------*/
typedef struct { int i0; Offset o0; } _1O1I;
typedef struct { int i0; Expr e0; Offset o0; } _1O1I1E;
typedef struct { LabelPt l0; Offset o0; } _1O1L;
typedef struct { Expr e0; Offset o0; } _1O1E;
typedef struct { Expr e0,e1; Offset o0; } _1O2E;
typedef struct { Expr e0,e1,e2; Offset o0; } _1O3E;
typedef struct { Offset o0,o1; } _2O0E;
typedef struct { Expr e0; Offset o0,o1; } _2O1E;
typedef struct { SyncPPt s; Offset o0,o1; } _1S2O;
/*---------------------------------------------------------------------------*/
typedef struct {

	Offset
		next;

	char
		type;

} PTNodeHdr;
/*---------------------------------------------------------------------------*/
#define PTNHDRSIZE		(sizeof(char)+sizeof(Offset))
/*---------------------------------------------------------------------------*/
typedef struct {

	PTNodeHdr
		h;

	union {

		int		b;
		Expr	e;
		LabelPt	l;
		Offset	o;
		SyncPPt	s;

		_1O1I   _1o1i;
		_1O1L   _1o1l;
		_1O1E   _1o1e;
		_1O2E   _1o2e;
		_1O3E   _1o3e;
		_2O0E   _2o0e;
		_2O1E   _2o1e;

		_1S2O   _1s2o;

		_1O1I1E _1o1i1e;

	} b;

} PTNode, *PTNodePt;
/*---------------------------------------------------------------------------*/
#define T_BREAK			((char)1)
#define T_CONTINUE		((char)2)
#define T_RET0			((char)3)
#define T_EXPRESSION	((char)4)
#define T_RET1			((char)5)
#define T_GOTO			((char)6)
#define T_LABEL			((char)7)
#define T_FOR0			((char)8)
#define T_FOR1			((char)9)
#define T_FOR2			((char)10)
#define T_FOR3			((char)11)
#define T_WHILE			((char)12)
#define T_DO			((char)13)
#define T_FOR4			((char)14)
#define T_FOR5			((char)15)
#define T_FOR6			((char)16)
#define T_FOR7			((char)17)
#define T_CEXEC			((char)18)
#define T_IF0			((char)19)
#define T_IF1			((char)20)
#define T_JOIN			((char)21)
#define T_WHEN			((char)22)
#define T_BLBRK			((char)23)
#define T_BLOCK			((char)24)
#define T_SWITCH		((char)25)
#define T_CASE			((char)26)
/*---------------------------------------------------------------------------*/
typedef struct { Offset o0; int i0; } BlBng;
typedef struct { Offset o0; Expr e0; } IfGt;
typedef struct { Offset o0; int i0; Offset o1; int i1; } Frk;
typedef struct { int i0; Expr e0; } Swh;
/*---------------------------------------------------------------------------*/
typedef struct {

	char
		h;

	union {

		int		b;
		BlBng	bl;
		Frk		f;
		IfGt	i;
		Offset	o;
		Expr	e;
		SyncPPt	j;
		_1S2O	p;
		Swh		s;

	} b;

} Inst, *InstPt;
/*---------------------------------------------------------------------------*/
#define IS_HLT			(sizeof(char))
#define IS_RTN			(sizeof(char))
#define IS_IFG			(2*sizeof(Offset)+sizeof(int)+sizeof(char))
#define IS_GTO			(sizeof(Offset)+sizeof(char))
#define IS_ERT			(sizeof(Offset)+sizeof(int)+sizeof(char))
#define IS_EXP			(sizeof(Offset)+sizeof(int)+sizeof(char))
#define IS_FRK			(2*sizeof(Offset)+2*sizeof(int)+sizeof(char))
#define IS_BRK			(sizeof(Offset)+sizeof(char))
#define IS_JOI			(sizeof(SyncPPt)+sizeof(char))
#define IS_WHE			(sizeof(SyncPPt)+sizeof(char)+sizeof(Offset)+sizeof(Offset))
#define IS_BBR			(sizeof(int)+sizeof(char))
#define IS_BNG			(sizeof(Offset)+sizeof(int)+sizeof(char))
#define IS_BND			(sizeof(char))
#define IS_SWH			(sizeof(Offset)+2*sizeof(int)+sizeof(char))
#define IS_CAS			(2*sizeof(Offset)+sizeof(int)+sizeof(char))
/*---------------------------------------------------------------------------*/
#define Labelized(h)	(((int) h)&(0x80))
#define Labelize(h)		((h)=(char)((int)(h) | 0x80))
#define Labiz(h)		((char)((int)(h) | 0x80))
#define UnLabelize(h)	((h)=(char)((int)(h) & 0x7f))
/*---------------------------------------------------------------------------*/
typedef struct selem {

	int
		id;

	Offset
		lab;

	struct selem
		*n;

} SElem, *SElemPt;
/*---------------------------------------------------------------------------*/
typedef struct sn {

	char
		type;

	SElemPt
		blocks;

	union {

		struct	{ struct sn *r,*l; } s;
		char	*i;

	} b;

} SNode, *SNodePt;
/*---------------------------------------------------------------------------*/
typedef struct sbtn {

	char
		color; /* BLACK ou WHITE, couleur du noeud pour les reequilibrages. */
	
	int
		lab;

	SNodePt
		state;

	struct sbtn
		*l, /* Fils gauche du noeud. */
		*r; /* Fils droit du noeud. */
		
} SBtNode, *SBtNodePt;
/*---------------------------------------------------------------------------*/
typedef struct pbtn {

	char
		color; /* BLACK ou WHITE, couleur du noeud pour les reequilibrages. */
	
	int
		pc;

	struct pbtn
		*l, /* Fils gauche du noeud. */
		*r; /* Fils droit du noeud. */
		
} PBtNode, *PBtNodePt;
/*---------------------------------------------------------------------------*/
typedef struct slist {

	SBtNodePt
		sbtn;

	struct slist
		*n;

} SListElem, *SListElemPt;
/*----------------------------------------------------------------------------*/
typedef struct {

	int
		nc,
		nd;

} SCCount, *SCCountPt;
/*----------------------------------------------------------------------------*/
typedef struct {

	char
		*s;

	int
		f,
		t,
		bn;

	Offset
		o,
		ido;

} Ident, *IdentPt;
/*----------------------------------------------------------------------------*/
