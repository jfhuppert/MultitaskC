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

#include <icsn.h>
#include <general.h>

#define __ER__ExtRed	__er__extred
#define __ER__Brk		__er__brk
#define __ER__Sync		__er__sync
#define __ER__GetBrk	__er__getbrk
#define __ER__ApplyBrk	__er__applybrk
#define __ER__MFind		__er__mfind
#define __ER__MTest		__er__mtest
#define __ER__MClear	__er__mclear

static void __ER__ExtRed(), __ER__Brk(), __ER__Sync();

int EXTREDRFNAME(mt,r,cf) __MEET__ mt[]; __TCB__ *r; void (*cf)();
{
unsigned int nch, nt;

	do {
		cf();
		nch=nt=0;
		__ER__ExtRed(r,&nt,&nch); if(r->type==0) return(1);
		__ER__Brk(r,&nch); if(r->type==0) return(1);
		__ER__Sync(r,mt,&nch); if(r->type==0) return(1);
	} while(nch);

	return(nt);
}

static void __ER__ExtRed(r,n,nch) __TCB__ *r; unsigned int *nch, *n;
{
	if(r->type==SN_NODE) {
		if(r->b.s.l->type==SN_LEAF) {
			if(r->b.s.l->b.l.i.h==IC_BRK) {
				*nch+=1;
				r->type=SN_LEAF;
				r->b.l.pc=r->b.s.l->b.l.i.b.o;
				r->b.l.i.h=IC_INV;
				return;
			}
			else {
				if(r->b.s.r->type==SN_LEAF) {
					if(r->b.s.r->b.l.i.h==IC_BRK) {
						r_leaf_brk:
						*nch+=1;
						r->type=SN_LEAF;
						r->b.l.pc=r->b.s.r->b.l.i.b.o;
						r->b.l.i.h=IC_INV;
						return;
					}
					else {
						if(r->b.s.l->b.l.i.h==IC_HLT && r->b.s.r->b.l.i.h==IC_HLT) {
							*nch+=1;
							r->type=SN_LEAF;
							r->b.l.pc=r->b.s.r->b.l.i.b.o;
							r->b.l.i.h=IC_INV;
							return;
						}
					}
				}
			}
		}
		else if(r->b.s.r->type==SN_LEAF && r->b.s.r->b.l.i.h==IC_BRK) goto r_leaf_brk;

		__ER__ExtRed(r->b.s.l,n,nch); __ER__ExtRed(r->b.s.r,n,nch);

	}
	else {
		switch(r->b.l.i.h) {
		case IC_FRK :
			*nch+=1;
			r->type=SN_NODE;
			r->b.l.i.b.f.tcb1->type=r->b.l.i.b.f.tcb2->type=SN_LEAF;
			r->b.l.i.b.f.tcb1->nb=r->b.l.i.b.f.tcb2->nb=0;
			r->b.l.i.b.f.tcb1->b.l.i.h=r->b.l.i.b.f.tcb2->b.l.i.h=IC_INV;
			r->b.l.i.b.f.tcb1->b.l.pc=r->b.l.i.b.f.pc1;
			r->b.l.i.b.f.tcb2->b.l.pc=r->b.l.i.b.f.pc2;
			{ __TCB__ *t1,*t2; t1=r->b.l.i.b.f.tcb1; t2=r->b.l.i.b.f.tcb2; r->b.s.l=t1; r->b.s.r=t2; }
			break;
		case IC_BNG :
			*nch+=1;
			r->bstack[r->nb].id=r->b.l.i.b.b.i;
			r->bstack[r->nb].pc=r->b.l.i.b.b.o0;
			r->b.l.pc=r->b.l.i.b.b.o1;;
			r->nb+=1;
			break;
		case IC_BND :
			*nch+=1;
			r->b.l.pc=r->b.l.i.b.o;
			r->nb-=1;
			break;
		default :
			*n+=1;
			break;
		}
	}
}

static void __ER__GetBrk(r,id) __TCB__ *r; int *id;
{
	if(r->type==SN_LEAF) {
		if(r->b.l.i.h==IC_BBR) *id=r->b.l.i.b.r.i,r->b.l.i.h=IC_INV,r->b.l.pc=r->b.l.i.b.r.o;
	}
	else { __ER__GetBrk(r->b.s.l,id); if((*id)!=(-1)) return; __ER__GetBrk(r->b.s.r,id); }
}

static void __ER__ApplyBrk(r,id) __TCB__ *r; int id;
{
register unsigned int i;

	for(i=0;(i<r->nb && r->bstack[i].id!=id);i++);

	if(i<r->nb) {
		if(r->type==SN_NODE) { r->type=SN_LEAF; r->b.s.l->type=r->b.s.r->type=SN_INVA; }
		r->b.l.pc=r->bstack[i].pc;
		r->nb=i;
	}
	else { if(r->type==SN_NODE) { __ER__ApplyBrk(r->b.s.l,id); __ER__ApplyBrk(r->b.s.r,id); } }
}

static void __ER__Brk(r,nch) __TCB__ *r; unsigned int *nch;
{
int id;

	for(;;) {
		id=(-1); __ER__GetBrk(r,&id);
		if(id==(-1)) return;
		else { *nch+=1; __ER__ApplyBrk(r,id); }
	}
}

static int __ER__MFind(r,mt) __TCB__ *r; __MEET__ mt[];
{
	if(r->type==SN_LEAF) {
		switch(r->b.l.i.h) {
		case IC_JOI :
			mt[r->b.l.i.b.j.i].n+=1;
			return(1);
		case IC_WHE :
			mt[r->b.l.i.b.w.i].n+=1;
			return(1);
		default :
			return(0);
		}
	}
	else return(__ER__MFind(r->b.s.l,mt)+__ER__MFind(r->b.s.r,mt));
}

static int __ER__MTest(r,mt) __TCB__ *r; __MEET__ mt[];
{
	if(r->type==SN_LEAF) {
		switch(r->b.l.i.h) {
		case IC_JOI :
			if(mt[r->b.l.i.b.j.i].n>=mt[r->b.l.i.b.j.i].card) { r->b.l.pc=r->b.l.i.b.j.o; return(1); }
			else return(0);
		case IC_WHE :
			if(mt[r->b.l.i.b.w.i].n>=mt[r->b.l.i.b.w.i].card) r->b.l.pc=r->b.l.i.b.w.o0;
			else r->b.l.pc=r->b.l.i.b.w.o1;
			return(1);
		default :
			return(0);
		}
	}
	else return(__ER__MTest(r->b.s.l,mt)+__ER__MTest(r->b.s.r,mt));
}

static void __ER__MClear(r,mt) __TCB__ *r; __MEET__ mt[];
{
	if(r->type==SN_LEAF) {
		switch(r->b.l.i.h) {
		case IC_JOI :
			mt[r->b.l.i.b.j.i].n=0;
			r->b.l.i.h=IC_INV;
			break;
		case IC_WHE :
			mt[r->b.l.i.b.w.i].n=0;
			r->b.l.i.h=IC_INV;
			break;
		}
	}
	else __ER__MClear(r->b.s.l,mt),__ER__MClear(r->b.s.r,mt);
}

static void __ER__Sync(r,mt,nch) __TCB__ *r; __MEET__ mt[]; unsigned int *nch;
{
	for(;;) {
		if(__ER__MFind(r,mt)==0) return;
		else { *nch+=__ER__MTest(r,mt); __ER__MClear(r,mt); }
	}
}
