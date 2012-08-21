/*---------------------------------------------------------------------------*/
/*
	general.h
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

#define MAXLEN		80
#define MAXOFLEN	256
#define BIGBUFF		2048
#define ITMAXLEN	8192
#define FIRSTEXL	1
#define FOREVER		for(;;)
#define LabNI()		Labz=1
#define ULabNI()	Labz=0

/*
  Un Fichier d'instructions ne commence pas en 0.
  On saute quelques octets pour se reserver des offset
  speciaux (voir definitions suivantes).
*/
#define InitFile(f)		WrFile("  ",2,1,f)
#define SkipFiller(f)	SetOffset(f,2L,0)
/* Longueur de l'espace reserve (en rapport avec les deux definitions precedentes). */
#define FillerLength()	2L

/* Offset reserves (speciaux). */
#define NULLOFF			((Offset)0)
#define CYCLEOFF		((Offset)1)

#define IAdd(o)		(CodeBase+(int)(o))
#define IOff(a)		((a)-CodeBase)

#define SCOPE_G		(0)
#define SCOPE_L		(1)

#define ENDLABELNOTREACHED (-1)

#define TMPEXT		".PmT"
#define DEFTMPDIR	"."

#define DEFCPP		"/lib/cpp"
#define CPPOUT		"CO"

#define F_BEG		0
#define F_CUR		1
#define F_END		2

#define COMPFORK	0
#define SIMFORK		1
#define INTERFORK	2

#define BEE_COMPF	"BEE_COMPF"
#define BEE_SIMF	"BEE_SIMF"
#define BEE_INTERF	"BEE_INTERF"

#define GOTOFN		"G"
#define LGOTOFN		"./XXLGotoX.las"

#define PTREEFN		"P"
#define LPTREEFN	"./XXLPTreeX.las"

#define CODEFN		"C"
#define LCODEFN		"./XXLCodeX.las"

#define LSFN		"LS"
#define LABFN		"LA"
#define DIFN		"DI"
#define STFN		"ST"
#define AOFN		"AO"
#define PCFN		"PCR"

/*--------------------------------------------------*/
#define ROff(o,f)		RdFile(&o,sizeof(Offset),1,f)
#define RInt(i,f)		RdFile(&i,sizeof(int),1,f)
#define RChr(t,f)		RdFile(&t,sizeof(char),1,f)
#define RLabP(l,f)		RdFile(&l,sizeof(LabelPt),1,f)
#define RSynP(s,f)		RdFile(&s,sizeof(SyncPPt),1,f)
#define RBNodeP(n,f)	RdFile(&n,sizeof(BNodePt),1,f)
/*--------------------------------------------------*/
#define WOff(o,f)		WrFile(&o,sizeof(Offset),1,f)
#define WInt(i,f)		WrFile(&i,sizeof(int),1,f)
#define WChr(t,f)		WrFile(&t,sizeof(char),1,f)
#define WLabP(l,f)		WrFile(&l,sizeof(LabelPt),1,f)
#define WSynP(s,f)		WrFile(&s,sizeof(SyncPPt),1,f)
#define WBNodeP(n,f)	WrFile(&n,sizeof(BNodePt),1,f)
/*--------------------------------------------------*/
#define StkDecl(t,s)	struct { t buf[(s)]; t *sp; t *down; }
#define StkInit(st,s)	(st).sp=(st).down=(&(st).buf[s])
#define StkPush(st,e)	(*(--(st).sp))=e
#define StkPop(st)		(*(st).sp++)
#define StkTop(st)		(*(st).sp)
#define StkSP(st)		((st).sp)
#define StkEP(st,p)		((p)>=(st).down)
#define StkNElem(st)	((st).down-(st).sp)
#define StkEmpty(st)	((st).sp>=(st).down)
#define StkFull(st)		((st).sp<=(&(st).buf[0]))

#define OSTKSIZE		(512*1024)
#define BNSTKSIZE		(512*1024)
#define SNSTKSIZE		(512*1024)
#define BTSTKSIZE		(1024*1024)
#define SBTSTKSIZE		(1024*1024)
/*--------------------------------------------------*/
#define SkipChr(a1)				( a1+=sizeof(char) )
#define SkipInt(a1)				( a1+=sizeof(int) )
#define SkipOff(a1)				( a1+=sizeof(Offset) )
#define SkipSyn(a1)				( a1+=sizeof(SyncPPt) )

#define GetChrAndMove(a1,a2)	( memcpy((char *)&(a2),(a1),sizeof(char)) , SkipChr(a1) )
#define GetIntAndMove(a1,a2)	( memcpy((char *)&(a2),(a1),sizeof(int)) , SkipInt(a1) )
#define GetOffAndMove(a1,a2)	( memcpy((char *)&(a2),(a1),sizeof(Offset)) , SkipOff(a1) )
#define GetSynAndMove(a1,a2)	( memcpy((char *)&(a2),(a1),sizeof(SyncPPt)) , SkipSyn(a1) )

#define GetChr(a1,a2)			( memcpy((char *)&(a2),(a1),sizeof(char)) )
#define GetInt(a1,a2)			( memcpy((char *)&(a2),(a1),sizeof(int)) )
#define GetOff(a1,a2)			( memcpy((char *)&(a2),(a1),sizeof(Offset)) )
#define GetSyn(a1,a2)			( memcpy((char *)&(a2),(a1),sizeof(SyncPPt)) )

#define PutChrAndMove(a1,a2)	( memcpy((a1),(char *)&(a2),sizeof(char)) , SkipChr(a1) )
#define PutIntAndMove(a1,a2)	( memcpy((a1),(char *)&(a2),sizeof(int)) , SkipInt(a1) )
#define PutOffAndMove(a1,a2)	( memcpy((a1),(char *)&(a2),sizeof(Offset)) , SkipOff(a1) )
#define PutSynAndMove(a1,a2)	( memcpy((a1),(char *)&(a2),sizeof(SyncPPt)) , SkipSyn(a1) )

#define PutChr(a1,a2)			( memcpy((a1),(char *)&(a2),sizeof(char)) )
#define PutInt(a1,a2)			( memcpy((a1),(char *)&(a2),sizeof(int)) )
#define PutOff(a1,a2)			( memcpy((a1),(char *)&(a2),sizeof(Offset)) )
#define PutSyn(a1,a2)			( memcpy((a1),(char *)&(a2),sizeof(SyncPPt)) )
/*--------------------------------------------------*/
/* Taille max du format d'impression des etiquettes. */
#define LFMTSIZE	64

/* Taille max de l'etat courant sous forme de liste. */
#define CSTATESIZE	64

/* Taille max de la table des identificateurs de breaks. */
#define MAXBRKIDS	64
/*--------------------------------------------------*/
/* Trace debug flags. */
#define DEXP	0
#define DTEST	1
#define DRET	2
/*------------------------------------------------*/
#define PCNAME			"__pc__"
#define PFLAGNAME		"__pf__"
#define NTNAME			"__nt__"
/*--------------------------------------------------*/
#define INSTSNAME		"__INST_struct"
#define INSTTNAME		"__INST__"
#define BSCSNAME		"__BSC_struct"
#define BSCTNAME		"__BSC__"
#define TCBSNAME		"__TCB_struct"
#define TCBTNAME		"__TCB__"
#define MEETSNAME		"__MEET_struct"
#define MEETTNAME		"__MEET__"
/*--------------------------------------------------*/
#define MTNAME			"__mt__"
#define TCBINITFNAME	"init"
#define TCBUPDTFNAME	"collect"
#define EXTREDFNAME		"__er__" /* !!!!! DOIT ETRE LE MEME QUE LE SUIVANT -> EXTREDRFNAME */
#define EXTREDRFNAME	__er__
#define SRNAME			"state"
#define BSNAME			"stack"
/*--------------------------------------------------*/
