/*--------------------------------------------------*/
/*
    ifork.c
    Shift version : limited to 31 tasks in "execute and"
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
#include <symset.h>

/*--------------------------------------------------*/

#define MAXFFSIZE    1024

typedef struct { int cpn; char *i; } FFElem;

int
    LFIn,
    LFOut,
    LNFElem,
    RFIn,
    RFOut,
    RNFElem;

FFElem 
    LFifo[MAXFFSIZE],
    RFifo[MAXFFSIZE];

#define LF_Init()    (LNFElem=LFIn=LFOut=0)
#define LF_Empty()    (LNFElem==0)
#define LF_NEmpty()    (LNFElem!=0)
#define LF_Full()    (LNFElem>=MAXFFSIZE)
#define LF_Put(x)     (LFifo[LFIn]=(x),LFIn=(LFIn>=(MAXFFSIZE-1)?0:LFIn+1),LNFElem+=1)
#define LF_Get(x)     ((x)=LFifo[LFOut],LFOut=(LFOut>=(MAXFFSIZE-1)?0:LFOut+1),LNFElem-=1)
#define LF_First()    (LFifo[LFOut])

#define RF_Init()    (RNFElem=RFIn=RFOut=0)
#define RF_Empty()    (RNFElem==0)
#define RF_NEmpty()    (RNFElem!=0)
#define RF_Full()    (RNFElem>=MAXFFSIZE)
#define RF_Put(x)     (RFifo[RFIn]=(x),RFIn=(RFIn>=(MAXFFSIZE-1)?0:RFIn+1),RNFElem+=1)
#define RF_Get(x)     ((x)=RFifo[RFOut],RFOut=(RFOut>=(MAXFFSIZE-1)?0:RFOut+1),RNFElem-=1)
#define RF_First()    (RFifo[RFOut])

/*--------------------------------------------------*/

#define ROOTNUM    0

extern void
    XFree(),
    Indent(),
    GenReturn(),
    LeaveApp(),
    (*EPrint)(),
    FatalMemErr();

extern char
    *XMalloc(),
    *Me,
    *CodeEnd,
    *CodeBase,
    *CInst,
    *EndStateAdd,
    *CFuncName;

extern int
#ifdef DEBUG
    DebugInfo,
    Trace,
#endif
    Results,
    DebugMode,
    NextMeetingId,
    Labz;

extern FILE
#ifdef DEBUG
    *SrcFile,
#endif
    *AOutput,
    *Output;


extern TaState
    SyncSymSet;

int
    FN;

/*--------------------------------------------------*/
static void PHead(output,h,cpcn)
int
    cpcn;

char
    *h;

FILE
    *output;
{
    fprintf(output,
        "%sif(_%s_%d_%s_%d.type==%d) switch (_%s_%d_%s_%d.b.l.pc) {\n",
        h,CFuncName,FN,SRNAME,cpcn,SN_LEAF,CFuncName,FN, SRNAME,cpcn
    );
}
/*--------------------------------------------------*/
static void GenChildAct(cpcn,cinst)
char
    *cinst;

int
    cpcn;
{
FFElem 
    FEl;

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
            fprintf(Output,"   case 0x%lx :\n",IOff(lab));
            fprintf(Output,"     return ;\n");
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
            FEl.cpn=(cpcn<<1)+1;
            FEl.i=cinst;
            LF_Put(FEl);
            FEl.cpn=(cpcn<<1)+2;
            FEl.i=IAdd(IBuff.b.f.o0);
            RF_Put(FEl);
            cinst=IAdd(IBuff.b.f.o1);
            continue;

        case IC_GTO :
            SkipOff(cinst);
            break;

        case IC_ERT :
            GetOffAndMove(cinst,IBuff.b.e.beg);
            GetIntAndMove(cinst,IBuff.b.e.size);
            fprintf(Output,"   case 0x%lx :\n",IOff(lab));
            fprintf(Output,"     return ( ");
            EPrint(Output,IBuff.b.e.beg,IBuff.b.e.size,DRET); fprintf(Output," ) ;\n");
            break;

        case IC_EXP :
            GetOffAndMove(cinst,IBuff.b.e.beg);
            GetIntAndMove(cinst,IBuff.b.e.size);
            fprintf(Output,"   case 0x%lx :\n",IOff(lab));
            fprintf(Output,"     "); EPrint(Output,IBuff.b.e.beg,IBuff.b.e.size,DEXP); fprintf(Output," ;\n");
            fprintf(Output,"     _%s_%d_%s_%d.b.l.pc = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IOff(cinst));
            fprintf(Output,"     %s ++ ;\n",PFLAGNAME);
            fprintf(Output,"     break ;\n");
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
FFElem
    LFEl,
    RFEl;

    LF_Init();
    RF_Init();

    LFEl.cpn=(cpcn<<1)+1;
    LFEl.i=IAdd(o1);
    LF_Put(LFEl);
    RFEl.cpn=(cpcn<<1)+2;
    RFEl.i=IAdd(o2);
    RF_Put(RFEl);

    do {

        if(LF_NEmpty()) {
            LF_Get(LFEl);
            cpcn=LFEl.cpn;
            PHead(Output,"   ",cpcn);
            GenChildAct(LFEl.cpn,LFEl.i);
            while(LF_NEmpty() && LF_First().cpn==cpcn) { LF_Get(LFEl); GenChildAct(LFEl.cpn,LFEl.i); }
            fprintf(Output,"   }\n\n");
        }

        if(RF_NEmpty()) {
            RF_Get(RFEl);
            cpcn=RFEl.cpn;
            PHead(Output,"   ",cpcn);
            GenChildAct(RFEl.cpn,RFEl.i);
            while(RF_NEmpty() && RF_First().cpn==cpcn) { RF_Get(RFEl); GenChildAct(RFEl.cpn,RFEl.i); }
            fprintf(Output,"   }\n\n");
        }

    } while( LF_NEmpty() && RF_NEmpty() );
}
/*--------------------------------------------------*/
static void GenChildB(cpcn,cinst)
char
    *cinst;

int
    cpcn;
{
FFElem 
    FEl;

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
            FEl.cpn=(cpcn<<1)+1;
            FEl.i=cinst;
            LF_Put(FEl);
            FEl.cpn=(cpcn<<1)+2;
            FEl.i=IAdd(IBuff.b.f.o0);
            RF_Put(FEl);
            cinst=IAdd(IBuff.b.f.o1);
            continue;

        case IC_GTO :
            GetOffAndMove(cinst,IBuff.b.o);
            fprintf(Output,"   case 0x%lx :\n",IOff(lab));
            fprintf(Output,"     _%s_%d_%s_%d.b.l.pc = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.o);
            fprintf(Output,"     %s ++ ;\n",PFLAGNAME);
            fprintf(Output,"     break ;\n");
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
FFElem
    LFEl,
    RFEl;

    LF_Init();
    RF_Init();

    LFEl.cpn=(cpcn<<1)+1;
    LFEl.i=IAdd(o1);
    LF_Put(LFEl);
    RFEl.cpn=(cpcn<<1)+2;
    RFEl.i=IAdd(o2);
    RF_Put(RFEl);

    do {

        if(LF_NEmpty()) {
            LF_Get(LFEl);
            cpcn=LFEl.cpn;
            PHead(Output,"   ",cpcn);
            GenChildB(LFEl.cpn,LFEl.i);
            while(LF_NEmpty() && LF_First().cpn==cpcn) { LF_Get(LFEl); GenChildB(LFEl.cpn,LFEl.i); }
            fprintf(Output,"   }\n\n");
        }

        if(RF_NEmpty()) {
            RF_Get(RFEl);
            cpcn=RFEl.cpn;
            PHead(Output,"   ",cpcn);
            GenChildB(RFEl.cpn,RFEl.i);
            while(RF_NEmpty() && RF_First().cpn==cpcn) { RF_Get(RFEl); GenChildB(RFEl.cpn,RFEl.i); }
            fprintf(Output,"   }\n\n");
        }

    } while( LF_NEmpty() && RF_NEmpty() );
}
/*--------------------------------------------------*/
static void GenChildT(cpcn,cinst)
char
    *cinst;

int
    cpcn;
{
FFElem 
    FEl;

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
            fprintf(Output,"   case 0x%lx :\n",IOff(lab));
            GetOffAndMove(cinst,IBuff.b.i.o0);
            GetOffAndMove(cinst,IBuff.b.i.e0.beg);
            GetIntAndMove(cinst,IBuff.b.i.e0.size);
            fprintf(Output,"     if ("); EPrint(Output,IBuff.b.i.e0.beg,IBuff.b.i.e0.size,DTEST);
            fprintf(Output,") _%s_%d_%s_%d.b.l.pc = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.i.o0);
            fprintf(Output,"     else _%s_%d_%s_%d.b.l.pc = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IOff(cinst));
            fprintf(Output,"     break ;\n");
            break;

        case IC_SWH : {
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
                    if(o!=NULLOFF) fprintf(Output,"\n      _%s_%d_%s_%d.b.l.pc = 0x%lx ;\n      break ;",CFuncName,FN,SRNAME,cpcn,o);
                    fprintf(Output,"\n");
                }
                fprintf(Output,"     }\n");
                fprintf(Output,"     break ;\n");
            }
            break;

        case IC_FRK :
            GetOffAndMove(cinst,IBuff.b.f.o0);
            SkipInt(cinst);
            GetOffAndMove(cinst,IBuff.b.f.o1);
            SkipInt(cinst);
            FEl.cpn=(cpcn<<1)+1;
            FEl.i=cinst;
            LF_Put(FEl);
            FEl.cpn=(cpcn<<1)+2;
            FEl.i=IAdd(IBuff.b.f.o0);
            RF_Put(FEl);
            cinst=IAdd(IBuff.b.f.o1);
            continue;

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
            "%s : bad instruction code in %s during GenTstSw() <%d>, fatal error.\n",
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
FFElem
    LFEl,
    RFEl;

    LF_Init();
    RF_Init();

    LFEl.cpn=(cpcn<<1)+1;
    LFEl.i=IAdd(o1);
    LF_Put(LFEl);
    RFEl.cpn=(cpcn<<1)+2;
    RFEl.i=IAdd(o2);
    RF_Put(RFEl);

    do {

        if(LF_NEmpty()) {
            LF_Get(LFEl);
            cpcn=LFEl.cpn;
            PHead(Output,"   ",cpcn);
            GenChildT(LFEl.cpn,LFEl.i);
            while(LF_NEmpty() && LF_First().cpn==cpcn) { LF_Get(LFEl); GenChildT(LFEl.cpn,LFEl.i); }
            fprintf(Output,"   }\n\n");
        }

        if(RF_NEmpty()) {
            RF_Get(RFEl);
            cpcn=RFEl.cpn;
            PHead(Output,"   ",cpcn);
            GenChildT(RFEl.cpn,RFEl.i);
            while(RF_NEmpty() && RF_First().cpn==cpcn) { RF_Get(RFEl); GenChildT(RFEl.cpn,RFEl.i); }
            fprintf(Output,"   }\n\n");
        }

    } while( LF_NEmpty() && RF_NEmpty() );
}
/*------------------------------------------------*/
static void
    GenTCB();

static void GenChildTCB(f,cpcn,cinst)
char
    *cinst;

int
    f,
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
            GenChildTCB(f+1,cpcn,IAdd(IBuff.b.f.o1));
            GenTCB(f,cpcn,IOff(cinst),IBuff.b.f.o0);
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
            "%s : bad instruction code in %s during GenChildTCB() <%d>, fatal error.\n",
            Me,
            FileName(CODEFN,TMPEXT),
            (int)IBuff.h

            );
            LeaveApp(1);
        }
    }
}
/*------------------------------------------------*/
static void GenTCB(f,cpcn,o1,o2)
Offset
    o1,
    o2;

int
    f,
    cpcn;
{
    cpcn<<=1;

    cpcn++;
    if(f==0) {
        fprintf(Output,",\n   _%s_%d_%s_%d",CFuncName,FN,SRNAME,cpcn);
        fprintf(AOutput,",\n   _%s_%d_%s_%d",CFuncName,FN,SRNAME,cpcn);
    }
    GenChildTCB(f,cpcn,IAdd(o1));

    cpcn++;
    if(f==0) {
        fprintf(Output,",\n   _%s_%d_%s_%d",CFuncName,FN,SRNAME,cpcn);
        fprintf(AOutput,",\n   _%s_%d_%s_%d",CFuncName,FN,SRNAME,cpcn);
    }
    GenChildTCB(f,cpcn,IAdd(o2));
}
/*------------------------------------------------*/
static void
    GenBS();

static void GenChildBS(f,cbss1m,cbss2m,cpcn,cinst)
char
    *cinst;

int
    f,
    *cbss1m,
    *cbss2m,
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
            GetIntAndMove(cinst,IBuff.b.f.i0);
            GetOffAndMove(cinst,IBuff.b.f.o1);
            GetIntAndMove(cinst,IBuff.b.f.i1);
            if(*cbss1m<IBuff.b.f.i0) *cbss1m=IBuff.b.f.i0;
            if(*cbss2m<IBuff.b.f.i1) *cbss2m=IBuff.b.f.i1;
            GenChildBS(f+1,cbss1m,cbss2m,cpcn,IAdd(IBuff.b.f.o1));
            GenBS(f,cpcn,IOff(cinst),IBuff.b.f.o0,*cbss1m,*cbss2m);
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
            "%s : bad instruction code in %s during GenChildBS() <%d>, fatal error.\n",
            Me,
            FileName(CODEFN,TMPEXT),
            (int)IBuff.h

            );
            LeaveApp(1);
        }
    }
}
/*------------------------------------------------*/
static void GenBS(f,cpcn,o1,o2,bss1,bss2)
Offset
    o1,
    o2;

int
    f,
    bss1,
    bss2,
    cpcn;
{
int
    cbss1m,
    cbss2m;

    cpcn<<=1;

    cpcn++;
    if(bss1 && f==0) fprintf(AOutput,"\n%s _%s_%d_%s_%d[%d];",BSCTNAME,CFuncName,FN,BSNAME,cpcn,bss1);
    if(f==0) cbss1m=cbss2m=0;
    GenChildBS(f,&cbss1m,&cbss2m,cpcn,IAdd(o1));

    cpcn++;
    if(bss2 && f==0) fprintf(AOutput,"\n%s _%s_%d_%s_%d[%d];",BSCTNAME,CFuncName,FN,BSNAME,cpcn,bss2);
    if(f==0) cbss1m=cbss2m=0;
    GenChildBS(f,&cbss1m,&cbss2m,cpcn,IAdd(o2));
}
/*------------------------------------------------*/
static void
    GenInit();

static void GenChildInit(f,cbss1m,cbss2m,cpcn,cinst)
char
    *cinst;

int
    *cbss1m,
    *cbss2m,
    f,
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
            GetIntAndMove(cinst,IBuff.b.f.i0);
            GetOffAndMove(cinst,IBuff.b.f.o1);
            GetIntAndMove(cinst,IBuff.b.f.i1);
            if(*cbss1m<IBuff.b.f.i0) *cbss1m=IBuff.b.f.i0;
            if(*cbss2m<IBuff.b.f.i1) *cbss2m=IBuff.b.f.i1;
            GenChildInit(f+1,cbss1m,cbss2m,cpcn,IAdd(IBuff.b.f.o1));
            GenInit(f,cpcn,IOff(cinst),IBuff.b.f.o0,*cbss1m,*cbss2m);
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
            "%s : bad instruction code in %s during GenChildInit() <%d>, fatal error.\n",
            Me,
            FileName(CODEFN,TMPEXT),
            (int)IBuff.h

            );
            LeaveApp(1);
        }
    }
}
/*------------------------------------------------*/
static void GenInit(f,cpcn,o1,o2,bss1,bss2)
Offset
    o1,
    o2;

int
    f,
    bss1,
    bss2,
    cpcn;
{
int
    cbss1m,
    cbss2m;

    cpcn<<=1;

    cpcn++;
    if(f==0) {
        if(cpcn==((ROOTNUM<<1)+1)) {
            fprintf(AOutput," _%s_%d_%s_%d.type=%d;\n",CFuncName,FN,SRNAME,cpcn,SN_LEAF);
            fprintf(AOutput," _%s_%d_%s_%d.b.l.pc = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,o1);
        }
        else {
            fprintf(AOutput," _%s_%d_%s_%d.type=%d;\n",CFuncName,FN,SRNAME,cpcn,SN_INVA);
            fprintf(AOutput," _%s_%d_%s_%d.b.l.pc = 0x0 ;\n",CFuncName,FN,SRNAME,cpcn);
        }
        fprintf(AOutput," _%s_%d_%s_%d.nb=0;\n",CFuncName,FN,SRNAME,cpcn);
        if(bss1) fprintf(AOutput," _%s_%d_%s_%d.bstack=_%s_%d_%s_%d;\n",CFuncName,FN,SRNAME,cpcn,CFuncName,FN,BSNAME,cpcn);
        else fprintf(AOutput," _%s_%d_%s_%d.bstack=(%s *)0;\n",CFuncName,FN,SRNAME,cpcn,BSCTNAME);
        cbss1m=cbss2m=0;
    }
    GenChildInit(f,&cbss1m,&cbss2m,cpcn,IAdd(o1));

    cpcn++;
    if(f==0) {
        if(cpcn==((ROOTNUM<<1)+2)) {
            fprintf(AOutput," _%s_%d_%s_%d.type=%d;\n",CFuncName,FN,SRNAME,cpcn,SN_LEAF);
            fprintf(AOutput," _%s_%d_%s_%d.b.l.pc = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,o2);
        }
        else {
            fprintf(AOutput," _%s_%d_%s_%d.type=%d;\n",CFuncName,FN,SRNAME,cpcn,SN_INVA);
            fprintf(AOutput," _%s_%d_%s_%d.b.l.pc = 0x0 ;\n",CFuncName,FN,SRNAME,cpcn);
        }
        fprintf(AOutput," _%s_%d_%s_%d.nb=0;\n",CFuncName,FN,SRNAME,cpcn);
        if(bss2) fprintf(AOutput," _%s_%d_%s_%d.bstack=_%s_%d_%s_%d;\n",CFuncName,FN,SRNAME,cpcn,CFuncName,FN,BSNAME,cpcn);
        else fprintf(AOutput," _%s_%d_%s_%d.bstack=(%s *)0;\n",CFuncName,FN,SRNAME,cpcn,BSCTNAME);
        cbss1m=cbss2m=0;
    }
    GenChildInit(f,&cbss1m,&cbss2m,cpcn,IAdd(o2));
}
/*------------------------------------------------*/
static void GenTCBDeclAndInit(cpcn,o1,o2,bss1,bss2,isb,isbe)
Offset
    isbe,
    o1,
    o2;

int
    bss1,
    bss2,
    isb,
    cpcn;
{
    /* Declaration de la racine des structures de controles des taches. */
    fprintf(Output," extern %s\n   _%s_%d_%s_%d",TCBTNAME,CFuncName,FN,SRNAME,cpcn);
    fprintf(AOutput,"%s\n   _%s_%d_%s_%d",TCBTNAME,CFuncName,FN,SRNAME,cpcn);
    GenTCB(0,cpcn,o1,o2);
    fprintf(Output,";\n\n");
    fprintf(AOutput,";\n");

    /* Declaration des piles de blocks si c'est necessaire. */
    if(isb!=(-1)) fprintf(AOutput,"\n%s _%s_%d_%s_%d[1];",BSCTNAME,CFuncName,FN,BSNAME,cpcn);
    GenBS(0,cpcn,o1,o2,bss1,bss2);
    fprintf(AOutput,"\n");

    /* Declaration de la fonction init des structures de controles des taches. */
    fprintf(AOutput,"\nvoid _%s_%d_%s()\n{\n",CFuncName,FN,TCBINITFNAME);
    fprintf(AOutput," _%s_%d_%s_%d.type=%d;\n",CFuncName,FN,SRNAME,cpcn,SN_NODE);
    fprintf(AOutput," _%s_%d_%s_%d.b.s.l=&_%s_%d_%s_%d;\n",CFuncName,FN,SRNAME,cpcn,CFuncName,FN,SRNAME,(cpcn<<1)+1);
    fprintf(AOutput," _%s_%d_%s_%d.b.s.r=&_%s_%d_%s_%d;\n",CFuncName,FN,SRNAME,cpcn,CFuncName,FN,SRNAME,(cpcn<<1)+2);
    if(isb!=(-1)) {
        fprintf(AOutput," _%s_%d_%s_%d.nb=1;\n",CFuncName,FN,SRNAME,cpcn);
        fprintf(AOutput," _%s_%d_%s_%d[0].id=%d;\n",CFuncName,FN,BSNAME,cpcn,isb);
        fprintf(AOutput," _%s_%d_%s_%d[0].pc=0x%lx;\n",CFuncName,FN,BSNAME,cpcn,(long)isbe);
        fprintf(AOutput," _%s_%d_%s_%d.bstack=_%s_%d_%s_%d;\n",CFuncName,FN,SRNAME,cpcn,CFuncName,FN,BSNAME,cpcn);
    }
    else {
        fprintf(AOutput," _%s_%d_%s_%d.nb=0;\n",CFuncName,FN,SRNAME,cpcn);
        fprintf(AOutput," _%s_%d_%s_%d.bstack=(%s *)0;\n",CFuncName,FN,SRNAME,cpcn,BSCTNAME);
    }
    GenInit(0,cpcn,o1,o2,bss1,bss2);
    fprintf(AOutput,"}\n\n");
}
/*--------------------------------------------------*/
static void GenChildUPDT(cpcn,cinst)
char
    *cinst;

int
    cpcn;
{
FFElem 
    FEl;

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
            GetSynAndMove(cinst,IBuff.b.j);
            fprintf(AOutput," case 0x%lx :\n",IOff(lab));
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.h = %d ;\n",CFuncName,FN,SRNAME,cpcn,IC_JOI);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.j.i = %d ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.j->id);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.j.o = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IOff(cinst));
            fprintf(AOutput,"   break ;\n");
            break;

        case IC_WHE :
            GetSynAndMove(cinst,IBuff.b.p.s);
            GetOffAndMove(cinst,IBuff.b.p.o0);
            GetOffAndMove(cinst,IBuff.b.p.o1);
            fprintf(AOutput," case 0x%lx :\n",IOff(lab));
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.h = %d ;\n",CFuncName,FN,SRNAME,cpcn,IC_WHE);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.w.i = %d ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.p.s->id);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.w.o0 = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.p.o0);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.w.o1 = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.p.o1);
            fprintf(AOutput,"   break ;\n");
            break;

        case IC_HLT :
            fprintf(AOutput," case 0x%lx :\n",IOff(lab));
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.h = %d ;\n",CFuncName,FN,SRNAME,cpcn,IC_HLT);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.o = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IOff(cinst));
            fprintf(AOutput,"   break ;\n");
            return;

        case IC_BNG :
            GetIntAndMove(cinst,IBuff.b.bl.i0);
            GetOffAndMove(cinst,IBuff.b.bl.o0);
            fprintf(AOutput," case 0x%lx :\n",IOff(lab));
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.h = %d ;\n",CFuncName,FN,SRNAME,cpcn,IC_BNG);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.b.i = %d ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.bl.i0);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.b.o0 = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.bl.o0);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.b.o1 = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IOff(cinst));
            fprintf(AOutput,"   break ;\n");
            break;

        case IC_BRK :
            GetOffAndMove(cinst,IBuff.b.o);
            fprintf(AOutput," case 0x%lx :\n",IOff(lab));
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.h = %d ;\n",CFuncName,FN,SRNAME,cpcn,IC_BRK);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.o = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.o);
            fprintf(AOutput,"   break ;\n");
            break;

        case IC_BND :
            fprintf(AOutput," case 0x%lx :\n",IOff(lab));
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.h = %d ;\n",CFuncName,FN,SRNAME,cpcn,IC_BND);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.o = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IOff(cinst));
            fprintf(AOutput,"   break ;\n");
            break;

        case IC_BBR :
            GetIntAndMove(cinst,IBuff.b.b);
            fprintf(AOutput," case 0x%lx :\n",IOff(lab));
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.h = %d ;\n",CFuncName,FN,SRNAME,cpcn,IC_BBR);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.r.i = %d ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.b);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.r.o = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IOff(cinst));
            fprintf(AOutput,"   break ;\n");
            break;

        case IC_FRK :
            GetOffAndMove(cinst,IBuff.b.f.o0);
            SkipInt(cinst);
            GetOffAndMove(cinst,IBuff.b.f.o1);
            SkipInt(cinst);
            fprintf(AOutput," case 0x%lx :\n",IOff(lab));
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.h = %d ;\n",CFuncName,FN,SRNAME,cpcn,IC_FRK);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.f.pc1 = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IOff(cinst));
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.f.pc2 = 0x%lx ;\n",CFuncName,FN,SRNAME,cpcn,IBuff.b.f.o0);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.f.tcb1 = &_%s_%d_%s_%d ;\n", CFuncName,FN,SRNAME,cpcn,
                                                                                        CFuncName,FN,SRNAME,(cpcn<<1)+1);
            fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.b.f.tcb2 = &_%s_%d_%s_%d ;\n", CFuncName,FN,SRNAME,cpcn,
                                                                                        CFuncName,FN,SRNAME,(cpcn<<1)+2);
            fprintf(AOutput,"   break ;\n");
            FEl.cpn=(cpcn<<1)+1;
            FEl.i=cinst;
            LF_Put(FEl);
            FEl.cpn=(cpcn<<1)+2;
            FEl.i=IAdd(IBuff.b.f.o0);
            RF_Put(FEl);
            cinst=IAdd(IBuff.b.f.o1);
            continue;

        case IC_RTN :
            break;

        case IC_IFG :
            SkipOff(cinst); SkipOff(cinst); SkipInt(cinst);
            break;

        case IC_SWH :
            GetIntAndMove(cinst,IBuff.b.s.i0); SkipOff(cinst); SkipInt(cinst);
            while(IBuff.b.s.i0--) { SkipChr(cinst); SkipOff(cinst); SkipInt(cinst); SkipOff(cinst); }
            break;

        case IC_GTO :
            SkipOff(cinst);
            break;

        case IC_ERT :
        case IC_EXP :
            SkipOff(cinst);
            SkipInt(cinst);
            break;

        default :
            fprintf(

            stderr,
            "%s : bad instruction code in %s during GenUPDT() <%d>, fatal error.\n",
            Me,
            FileName(CODEFN,TMPEXT),
            (int)IBuff.h

            );
            LeaveApp(1);
        }
    }
}
/*--------------------------------------------------*/
static void PQueue(cpcn)
int
    cpcn;
{
    fprintf(AOutput," default :\n");
    fprintf(AOutput,"   _%s_%d_%s_%d.b.l.i.h = %d ;\n",CFuncName,FN,SRNAME,cpcn,IC_INV);
    fprintf(AOutput,"   break ;\n");
    fprintf(AOutput," }\n\n");
}
/*--------------------------------------------------*/
static void GenUPDT(cpcn,o1,o2)
Offset
    o1,
    o2;

int
    cpcn;
{
FFElem
    LFEl,
    RFEl;

    LF_Init();
    RF_Init();

    LFEl.cpn=(cpcn<<1)+1;
    LFEl.i=IAdd(o1);
    LF_Put(LFEl);
    RFEl.cpn=(cpcn<<1)+2;
    RFEl.i=IAdd(o2);
    RF_Put(RFEl);

    do {

        if(LF_NEmpty()) {
            LF_Get(LFEl);
            cpcn=LFEl.cpn;
            PHead(AOutput," ",cpcn);
            GenChildUPDT(LFEl.cpn,LFEl.i);
            while(LF_NEmpty() && LF_First().cpn==cpcn) { LF_Get(LFEl); GenChildUPDT(LFEl.cpn,LFEl.i); }
            PQueue(cpcn);
        }

        if(RF_NEmpty()) {
            RF_Get(RFEl);
            cpcn=RFEl.cpn;
            PHead(AOutput," ",cpcn);
            GenChildUPDT(RFEl.cpn,RFEl.i);
            while(RF_NEmpty() && RF_First().cpn==cpcn) { RF_Get(RFEl); GenChildUPDT(RFEl.cpn,RFEl.i); }
            PQueue(cpcn);
        }

    } while( LF_NEmpty() && RF_NEmpty() );
}
/*------------------------------------------------*/
static void GenCollFuncDecl(cpcn,o1,o2)
Offset
    o1,
    o2;

int
    cpcn;
{
    /* Declaration de la fonction qui collecte les instructions speciales dans les taches. */
    fprintf(AOutput,"void _%s_%d_%s()\n{\n\n",CFuncName,FN,TCBUPDTFNAME);
    GenUPDT(cpcn,o1,o2);
    fprintf(AOutput,"}\n\n");
}
/*------------------------------------------------*/
static SyncPPt
    ParseTa();

static SyncPPt ParseBt(btroot,id)
BtNodePt
    btroot;

int
    id;
{
SyncPPt
    s;

    if(btroot!=(BtNodePt)NULL) {
        if((s=ParseTa(btroot->next,id))!=NULL) return(s);
        if((s=ParseBt(btroot->l,id))!=NULL) return(s);
        if((s=ParseBt(btroot->r,id))!=NULL) return(s);
    }

    return(NULL);
}
/*-----------------------------------------------------------*/
static SyncPPt ParseTa(taroot,id)
TaStatePt
    taroot;

int
    id;
{
    if(taroot!=(TaStatePt)NULL) {
        if(taroot->value.sval!=NULL && taroot->value.sval->id==id) return(taroot->value.sval);
        else return(ParseBt(taroot->tl,id));
    }
    else return(NULL);
}
/*------------------------------------------------*/
static void GenMTInit()
{
register int
    i;

SyncPPt
    s;

    for(i=0;i<NextMeetingId;i++) {
        s=ParseTa(&SyncSymSet,i);
        if(s==NULL) {
            fprintf(stderr,"%s, strange meeting id in GenMTInit(), quit.\n",Me);
            LeaveApp(1);
        }
        fprintf(Output," %s[%d].card=%d,%s[%d].n=0;\n",MTNAME,i,s->card,MTNAME,i);
    }
}
/*------------------------------------------------*/
void GenIFork(o1,o2,bss1,bss2,h,isb,isbe,esa)
Offset
    o1,
    o2;

int
    bss1,
    bss2,
    h,
    isb;

Offset
    isbe;

char
    *esa;
{
    FN=h;

    /* Generation du debut de block. */
    fprintf(Output," { /* || begin */\n\n");

    /*-----------------------------*/

    /*
        Declarations.
    */

    /* Declarations des structures de controles des taches et fonction init. */
    GenTCBDeclAndInit(ROOTNUM,o1,o2,bss1,bss2,isb,isbe);

    /* Declaration de la fonction qui collecte les instructions speciales dans les taches. */
    GenCollFuncDecl(ROOTNUM,o1,o2);

    /* Declaration de la fonction init des structures de controles des taches. */
    fprintf(Output," extern void\n   _%s_%d_%s();\n\n",CFuncName,h,TCBINITFNAME);

    /* Declaration de la fonction de mise a jour des structures de controles des taches. */
    fprintf(Output," extern void\n   _%s_%d_%s();\n\n",CFuncName,h,TCBUPDTFNAME);

    /* Declaration de la fonction d'extention/reduction de l'etat courant. */
    fprintf(Output," extern int\n   %s();\n\n",EXTREDFNAME);

    /* Declarations de la table des rendez-vous. */
    if(NextMeetingId) fprintf(Output," %s\n   %s[%d];\n\n",MEETTNAME,MTNAME,NextMeetingId);

    /* Declaration de variables pour la gestion des taches. */
    fprintf(Output," register unsigned int\n   %s,\n   %s;\n\n",NTNAME,PFLAGNAME);

    /*-----------------------------*/

    /*
        Instructions.
    */

    /* Initialisation de la table des rendez-vous. */
    GenMTInit();

    /* Generation de l'appel de la fonction init des structures de controles des taches. */
    fprintf(Output,"\n _%s_%d_%s();\n\n",CFuncName,h,TCBINITFNAME);

    /* Generation de la boucle principale. */
    fprintf(Output," for(;;) {\n\n");

    /* Generation de l'appel de la fonction d'extention/reduction de l'etat courant. */
    fprintf(
        Output,
        "   if((%s=%s(%s,&_%s_%d_%s_%d,_%s_%d_%s))==1) break;\n\n",
        NTNAME,
        EXTREDFNAME,
        (NextMeetingId?MTNAME:"0"),
        CFuncName,
        h,
        SRNAME,
        ROOTNUM,
        CFuncName,
        h,
        TCBUPDTFNAME
    );

    /* Generation des switchs d'actions. */
    fprintf(Output,"   %s = 0 ;\n\n",PFLAGNAME);
    GenActSw(ROOTNUM,o1,o2);

    /* Generation du test de progression. */
    fprintf(Output,"   if ( %s != 0 ) continue ;\n\n",PFLAGNAME);

    /* Generation des switchs de branchement. */
    GenBrSw(ROOTNUM,o1,o2);

    /* Generation du test de progression. */
    fprintf(Output,"   if ( %s == %s ) continue ;\n\n",PFLAGNAME,NTNAME);

    /* Generation des switchs de tests. */
    GenTstSw(ROOTNUM,o1,o2);

    /* Generation de la fin de boucle principale. */
    fprintf(Output," }\n\n");

    /*-----------------------------*/

    /* Generation de la fin de block. */
    fprintf(Output," } /* || end */\n",(long)IOff(esa));

    if(Results) {
        fprintf(stdout,"%s : sorry, can't print results for interpreted product.\n",Me);
    }
}
/*--------------------------------------------------*/
