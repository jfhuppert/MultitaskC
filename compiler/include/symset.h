/*---------------------------------------------------------------------------*/
/*
	symset.h
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

/* Portee d'une declaration, Locale ou Globale. */
#define LOCAL	(1)
#define GLOBAL	(2)

/* Couleurs d'un noeud d'arbre equilibre. */
#define BLACK	((char) 0)
#define WHITE	((char) 1)

/* Type d'un noeud d'arbre equilibre (fils gauche ou fils droit). */
#define RS		((char) 0)
#define LS		((char) 1)

#ifndef NULL
#define NULL	0
#endif

/* Flags pour les types suivant dans les SymSets, ATTENTION aux Macros !!!! */
#define SS_EXT		((int)(1<<8))
#define SS_TYP		((int)(1<<9))
#define SS_STA		((int)(1<<10))
#define SS_DEF		((int)(1<<11))
#define SS_ARR		((int)(1<<12))
#define SS_POI		((int)(1<<13))

/* Token commun a pclexan() et lexan(). */
#define T_TYPENAME	(1)
#define T_IDENTIF	(2)
#define T_FIDENTIF	(3)
#define T_MIDENTIF	(4)

/* Token pour les struct, union et enum. */
#define T_SIDENTIF	(5)
#define T_UIDENTIF	(6)
#define T_EIDENTIF	(7)

/* Macros pour isoler/fixer/tester les flags et les types. */
#define SS_SF(i,f)	((i)=(int)((i)|(f)))
#define SS_GF(i)	((int)((i)&0xff00))
#define SS_TF(i,f)	((int)(((i)&0xff00)&(f)))
#define SS_GT(i)	((int)((i)&0xff))

/*----------------------------------------------------------------------------*/
/* Structure d'un noeud d'arbre equilibre. */
struct s;
typedef struct btnode {

	char
		color; /* BLACK ou WHITE, couleur du noeud pour les reequilibrages. */
	
	char
		chr; /* caractere de la transition. */
		
	struct s
		*next; /* Etat rejoint par la transition (dans l'automate en arbre). */
		
	struct btnode
		*l, /* Fils gauche du noeud. */
		*r; /* Fils droit du noeud. */
		
} BtNode, *BtNodePt;
/*----------------------------------------------------------------------------*/
struct lab;
struct syn;
typedef union {

	int
		ival;

	struct lab
		*lval;

	struct syn
		*sval;

} TaVal, *TaValPt;
/*----------------------------------------------------------------------------*/
/* Structure d'un buffer pour le renomage. */
typedef struct {

	char
		*buf;

	int
		siz;
		
} RBuf, *RBufPt;
/*----------------------------------------------------------------------------*/
/* Structure d'un etat de l'automate en arbre. */
typedef struct s {

	TaVal
		value;

	BtNodePt
		tl; /* Liste des transitions. */
		
} TaState, *TaStatePt;
/*----------------------------------------------------------------------------*/
/* Structure d'un element de la pile des ensembles de symbole pour le pre-comp. */
typedef struct {

	TaState
		SSSet,
		USSet,
		ESSet,
		SymSet;

	int
		SSNum;

} SymSet, *SymSetPt;

#define SS_NREN			((int)-1)

/*---------------------------------------------------------------------------*/
#define UnKownValue(n)	(n.ival==0)
#define BtInit(bt)		bt=(BtNodePt)NULL
#define TaInit(ta)		{ BtInit(ta.tl); ta.value.ival=0; }
/*---------------------------------------------------------------------------*/
