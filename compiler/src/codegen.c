/*--------------------------------------------------*/
/*
	codegen.c
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
#include <sys/types.h>
#include <sys/stat.h>

extern void
	XFree(),
	GenGFork(),
	GenSFork(),
	GenIFork(),
	SetOffset(),
	FatalMemErr();

extern char
	*FileName(),
	*XMalloc(),
	*Me;
	
extern int
#ifdef DEBUG
	DebugInfo,
#endif
	NIFOptions,
	ForkOption,
	LabSwPurge,
	Interm,
	Labz;

extern Offset
	GetOffset();

extern FILE
	*LexOut,
	*CInput;

void
	(*EPrint)();

char
	Head,
	*CodeEnd,
	*CodeBase,
	*CInst;

int
	CodeSize,
	ForkNum=0;

Offset
	Cycle=CYCLEOFF;

FILE
	*Output,
	*AOutput,
	*SrcFile;

Inst
	IBuff;

struct stat
	SBuff;

/*--------------------------------------------------*/
char
	LabSwpFmt[]="( \
tee %s | awk \'/goto _[0-9a-fA-F]+_? ;$/ { printf(\"LABEL %%s\\n\",$(NF-1)); } \
/goto _[0-9a-fA-F]+_? ; }$/ { printf(\"LABEL %%s\\n\",$(NF-2)); } \
/block beg [0-9]+ _[0-9a-fA-F]+_/ { printf(\"LABEL %%s\\n\",$(NF-1)); } \
/fork _[0-9a-fA-F]+_ _[0-9a-fA-F]+_/ { printf(\"LABEL %%s\\n\",$(NF-1)); printf(\"LABEL %%s\\n\",$(NF-2)); } \
/break _[0-9a-fA-F]+_/ { printf(\"LABEL %%s\\n\",$(NF-1)); }\' | sort -u > %s ; \
cat %s %s | awk \'BEGIN { labels[\"\"]=0; } /^LABEL/ { labels[$2]=1; next; } \
/^ *_[0-9a-fA-F]+_? :( \\/\\* .* \\*\\/)?$/ { if(labels[$1]==1) print $0; next; } \
/^   switch \\( %s[1-9][0-9]* \\) \\{$/ { \
cur=$0; getline; if($0 ~ /^   \\}$/) { getline; if(NF!=0) print $0; } else { print cur ORS $0; } next; } \
/^   if\\(_[_a-zA-Z]+_[0-9]+_%s_[0-9]+\\.type==%d\\) switch \\(_[_a-zA-Z]+_[0-9]+_%s_[0-9]+\\.b\\.l\\.pc\\) \\{$/ { \
cur=$0; getline; if($0 ~ /^   \\}$/) { getline; if(NF!=0) print $0; } else { print cur ORS $0; } next; } \
{ print }\' ) > %s";

char
	LabSwpCmd[BIGBUFF];
/*--------------------------------------------------*/
void BuildLabSwpCmd()
{
char
	f1[BIGBUFF],
	f2[BIGBUFF];

	strcpy(f1,FileName(DIFN,TMPEXT));
	strcpy(f2,FileName(LABFN,TMPEXT));

	sprintf(
		LabSwpCmd,
		LabSwpFmt,
		f1,
		f2,
		f2,
		f1,
		PCNAME,
		SRNAME,
		(int)SN_LEAF,
		SRNAME,
		FileName(LSFN,TMPEXT)
	);
}
/*--------------------------------------------------*/
void SEPrint(output,beg,size,t)
FILE
	*output;

Offset
	beg;

int
	size,
	t;
{
#ifdef DEBUG
	if(DebugInfo)
		fprintf(stdout,
			"Copying expression at %ld, %d bytes long\n",
			(long)beg,
			size>=0?size:(-size)
		);
#endif
	SetOffset(SrcFile,beg,F_BEG);
	if(size>0) while(size--) putc(getc(SrcFile),output);
	else {
		if(size!=0) {
			fprintf(output,"!(");
			size=(-size);
			while(size--) putc(getc(SrcFile),output);
			fprintf(output,")");
		}
	}
}
/*--------------------------------------------------*/
void DEPrint(output,beg,size,t)
FILE
	*output;

Offset
	beg;

int
	size,
	t;
{
char
	c;

int
	s;

#ifdef DEBUG
	if(DebugInfo)
		fprintf(stdout,
			"Copying expression at %ld, %d bytes long\n",
			(long)beg,
			size>=0?size:(-size)
		);
#endif

	SetOffset(SrcFile,beg,F_BEG);

	if(size>0) {
		s=size;
		fprintf(output,"printf(\"%s",t==DTEST?"TEST ":(t==DRET?"RET ":""));
		while(size--) {
			switch(c=getc(SrcFile)) {
			case '%' :
				putc(c,output);
				putc(c,output);
				break;

			case '\"' :
			case '\\' :
			case '\'' :
				putc('\\',output);

			default :
				putc(c,output);
				break;
			}
		}
		fprintf(output,"\\n\"),");
		SetOffset(SrcFile,beg,F_BEG);
		while(s--) putc(getc(SrcFile),output);
	}
	else {
		if(size!=0) {
			size=(-size);
			s=size;
			fprintf(output,"printf(\"%s!(",t==DTEST?"TEST ":(t==DRET?"RET ":""));
			while(size--) {
				switch(c=getc(SrcFile)) {
				case '%' :
					putc(c,output);
					putc(c,output);
					break;

				case '\"' :
				case '\\' :
				case '\'' :
					putc('\\',output);

				default :
					putc(c,output);
					break;
				}
			}
			fprintf(output,")\\n\"),");
			SetOffset(SrcFile,beg,F_BEG);
			fprintf(output,"!(");
			while(s--) putc(getc(SrcFile),output);
			fprintf(output,")");
		}
	}
}
/*--------------------------------------------------*/
Offset UnChain(beg)
Offset
	beg;
{
char
	*next;

Offset
	cur;

	next=CodeBase+(int)beg;

	if(next==CodeEnd) return(beg);
	GetChrAndMove(next,Head);

	switch(Head) {

	case IC_GTO :
	case Labiz(IC_GTO) :
		GetOff(next,cur);
		if(cur==CYCLEOFF) return(beg);
		PutOff(next,Cycle);
		beg=UnChain(cur);
		PutOff(next,beg);
		break;

	default :
		break;

	}

	return(beg);
}
/*--------------------------------------------------*/
void LoadCodeFile()
{
unsigned int
	size;

	/* Ouverture du fichier contenant le code. */
	if((CInput=fopen(FileName(CODEFN,TMPEXT),"r+"))==NULL) {
		fprintf(stderr,"%s : can't open <%s>, quit.\n",Me,FileName(CODEFN,TMPEXT));
		LeaveApp(1);
	}

	if(fstat(fileno(CInput),&SBuff)==(-1)) {
		fprintf(stderr,"%s : can't fstat <%s>, quit.\n",Me,FileName(CODEFN,TMPEXT));
		LeaveApp(1);
	}

	CodeBase=(char *)XMalloc(size=(unsigned int)SBuff.st_size);

	if(CodeBase==(char *)NULL) {
		fprintf(stderr,"%s : can't map <%s> into memory, quit.\n",Me,FileName(CODEFN,TMPEXT));
		LeaveApp(1);
	}

	CodeSize=size;

	RdFile(CodeBase,1,size,CInput);
	fclose(CInput);

	CodeEnd=CodeBase+size;
}
/*--------------------------------------------------*/
char *ChainCases(n,ci)
int
	n;

char
	*ci;
{
char
	h,
	*pci;

Offset
	po,
	co;

	n-=1;
	GetChrAndMove(ci,h);
	if(h!=IC_CAS) {
		fprintf(stderr,"%s : bad code <%d> during ChainCases().\n",Me,(int)h);
		LeaveApp(1);
	}
	SkipOff(ci);
	SkipInt(ci);
	GetOff(ci,co);
	/* --- */
	po=co;
	pci=ci;
	SkipOff(ci);

	while(n--) {

		GetChrAndMove(ci,h);
		if(h!=IC_CAS) {
			fprintf(stderr,"%s : bad code <%d> during ChainCases().\n",Me,(int)h);
			LeaveApp(1);
		}
		SkipOff(ci);
		SkipInt(ci);
		GetOff(ci,co);

		if(co!=po) { PutOff(pci,Cycle); po=UnChain(po); }
		else { po=NULLOFF; }
		PutOff(pci,po);

		po=co;
		pci=ci;
		SkipOff(ci);

	}

	PutOff(pci,Cycle); po=UnChain(po);
	PutOff(pci,po);

	return(ci);
}
/*--------------------------------------------------*/
void UnChainGotos()
{
	CInst=CodeBase+(int)FillerLength();
	while(CInst!=CodeEnd) {

		GetChrAndMove(CInst,IBuff.h);

		switch(IBuff.h) {

		case IC_IFG :
		case Labiz(IC_IFG) :
			GetOff(CInst,IBuff.b.i.o0);
			IBuff.b.i.o0=UnChain(IBuff.b.i.o0);
			PutOffAndMove(CInst,IBuff.b.i.o0);
			SkipOff(CInst);
			SkipInt(CInst);
			break;

		case IC_GTO :
		case Labiz(IC_GTO) :
			GetOff(CInst,IBuff.b.o);
			PutOff(CInst,Cycle);
			IBuff.b.o=UnChain(IBuff.b.o);
			PutOffAndMove(CInst,IBuff.b.o);
			break;

		case IC_SWH :
		case Labiz(IC_SWH) :
			GetIntAndMove(CInst,IBuff.b.s.i0);
			SkipOff(CInst);
			SkipInt(CInst);
			CInst=ChainCases(IBuff.b.s.i0,CInst);
			break;

		case IC_JOI :
		case Labiz(IC_JOI) :
			SkipSyn(CInst);
			break;

		case IC_WHE :
		case Labiz(IC_WHE) :
			SkipSyn(CInst);
			SkipOff(CInst);
			SkipOff(CInst);
			break;

		case IC_RTN :
		case Labiz(IC_RTN) :
		case IC_HLT :
		case Labiz(IC_HLT) :
		case IC_BND :
		case Labiz(IC_BND) :
			break;

		case IC_FRK :
		case Labiz(IC_FRK) :
			SkipOff(CInst);
			SkipInt(CInst);
			SkipOff(CInst);
			SkipInt(CInst);
			break;

		case IC_BRK :
		case Labiz(IC_BRK) :
			SkipOff(CInst);
			break;

		case IC_ERT :
		case Labiz(IC_ERT) :
		case IC_EXP :
		case Labiz(IC_EXP) :
			SkipOff(CInst);
			SkipInt(CInst);
			break;

		case IC_BNG :
		case Labiz(IC_BNG) :
			SkipInt(CInst);
			SkipOff(CInst);
			break;

		case IC_BBR :
		case Labiz(IC_BBR) :
			SkipInt(CInst);
			break;

		default :
			fprintf(

			stderr,
			"%s : bad instruction code in %s during UnChainGotos() <%d>, fatal error.\n",
			Me,
			FileName(CODEFN,TMPEXT),
			(int)IBuff.h

			);
			LeaveApp(1);
		}
	}
}
/*--------------------------------------------------*/
void OutputCode(sourceFile)
char
	*sourceFile;
{
int
	IStateBlock=-1,
	lab;

	/* Ouverture du fichier source. */
	if((SrcFile=fopen(sourceFile,"r+"))==NULL) {
		fprintf(stderr,"%s : OutputCode() can't reopen <%s>, quit.\n",Me,sourceFile);
		LeaveApp(1);
	}

	/* Generation du code. */
	CInst=CodeBase+(int)FillerLength();
	fprintf(Output,"\n");

	while(CInst!=CodeEnd) {

		lab=CInst-CodeBase;
		GetChrAndMove(CInst,IBuff.h);

		if(Labelized(IBuff.h) || LabSwPurge==0) { fprintf(Output,"_%x_ :\n",lab); UnLabelize(IBuff.h); }

		switch(IBuff.h) {

		case IC_JOI :
			if(Interm) {
				GetSynAndMove(CInst,IBuff.b.j);
				fprintf(Output," join %d %d ;\n",IBuff.b.j->id,IBuff.b.j->card);
			}
			else { SkipSyn(CInst); }
			break;

		case IC_WHE :
			if(Interm) {
				GetSynAndMove(CInst,IBuff.b.p.s);
				GetOffAndMove(CInst,IBuff.b.p.o0);
				GetOffAndMove(CInst,IBuff.b.p.o1);
				fprintf(Output," pass %d %d ",IBuff.b.p.s->id,IBuff.b.p.s->card);
				fprintf(Output,"_%x_ _%x_ ;\n",(int)IBuff.b.p.o0,(int)IBuff.b.p.o1);
			}
			else { SkipSyn(CInst); SkipOff(CInst); SkipOff(CInst); }
			break;

		case IC_HLT :
			if(Interm) { fprintf(Output," halt ;\n"); }
			else {
				fprintf(stderr,"%s : spurious IC_HLT instruction during OutputCode(), fatal error.\n",Me);
				LeaveApp(1);
			}
			break;

		case IC_BRK :
			if(Interm) { GetOffAndMove(CInst,IBuff.b.o); fprintf(Output," break _%x_ ;\n",(int)IBuff.b.o); }
			else {
				fprintf(stderr,"%s : spurious IC_BRK instruction during OutputCode(), fatal error.\n",Me);
				LeaveApp(1);
			}
			break;

		case IC_BNG :
			GetIntAndMove(CInst,IBuff.b.bl.i0);
			GetOffAndMove(CInst,IBuff.b.bl.o0);
			if(Interm) fprintf(Output," block beg %d _%x_ ;\n",IBuff.b.bl.i0,(int)IBuff.b.bl.o0);
			else {
				if(*CInst==IC_FRK || *CInst==Labiz(IC_FRK)) { IStateBlock=IBuff.b.bl.i0; }
			}
			break;

		case IC_BND :
			if(Interm) { fprintf(Output," block end ;\n"); }
			break;

		case IC_BBR :
			if(Interm) { GetIntAndMove(CInst,IBuff.b.b); fprintf(Output," block break %d ;\n",IBuff.b.b); }
			else SkipInt(CInst);
			break;

		case IC_RTN :
			fprintf(Output," return ;\n");
			break;

		case IC_IFG :
			GetOffAndMove(CInst,IBuff.b.i.o0);
			GetOffAndMove(CInst,IBuff.b.i.e0.beg);
			GetIntAndMove(CInst,IBuff.b.i.e0.size);
			fprintf(Output," if (");
			EPrint(Output,IBuff.b.i.e0.beg,IBuff.b.i.e0.size,DTEST);
			fprintf(Output,") goto _%x_ ;\n",(int)IBuff.b.i.o0);
			break;

		case IC_SWH : {
				char h; int size; Offset o;
				GetIntAndMove(CInst,IBuff.b.s.i0);
				GetOffAndMove(CInst,IBuff.b.s.e0.beg);
				GetIntAndMove(CInst,IBuff.b.s.e0.size);
				fprintf(Output," switch (");
				EPrint(Output,IBuff.b.s.e0.beg,IBuff.b.s.e0.size,DEXP);
				fprintf(Output,") {\n");
				while(IBuff.b.s.i0--) {
					GetChrAndMove(CInst,h);
					GetOffAndMove(CInst,o);
					GetIntAndMove(CInst,size);
					if(o!=NULLOFF) { fprintf(Output," case ("); EPrint(Output,o,size,DEXP); fprintf(Output,") :"); }
					else { fprintf(Output," default :"); }
					GetOffAndMove(CInst,o);
					if(o!=NULLOFF) fprintf(Output,"\n  goto _%x_ ;",(int)o);
					fprintf(Output,"\n");
				}
				fprintf(Output," }\n");
			}
			break;

		case IC_FRK :
			GetOffAndMove(CInst,IBuff.b.f.o0);
			GetIntAndMove(CInst,IBuff.b.f.i0);
			GetOffAndMove(CInst,IBuff.b.f.o1);
			GetIntAndMove(CInst,IBuff.b.f.i1);
			if(Interm) fprintf(Output," fork _%x_ %d _%x_ %d ;\n",(int)IBuff.b.f.o0,IBuff.b.f.i0,(int)IBuff.b.f.o1,IBuff.b.f.i1);
			else {

				switch(ForkOption) {

				case COMPFORK :
					GenGFork(
						(Offset)(CInst-CodeBase),
						IBuff.b.f.o0,
						ForkNum++,
						IStateBlock,
						IBuff.b.f.o1,
						IAdd(IBuff.b.f.o1)
					);
					break;

				case SIMFORK :
					GenSFork(
						(Offset)(CInst-CodeBase),
						IBuff.b.f.o0,
						ForkNum++,
						IStateBlock,
						IBuff.b.f.o1,
						IAdd(IBuff.b.f.o1)
					);
					break;

				case INTERFORK :
					NIFOptions+=1;
					GenIFork(
						(Offset)(CInst-CodeBase),
						IBuff.b.f.o0,
						IBuff.b.f.i0,
						IBuff.b.f.i1,
						ForkNum++,
						IStateBlock,
						IBuff.b.f.o1,
						IAdd(IBuff.b.f.o1)
					);
					break;

				}

				IStateBlock=-1;
				CInst=IAdd(IBuff.b.f.o1);
			}
			break;

		case IC_GTO :
			GetOffAndMove(CInst,IBuff.b.o);
			fprintf(Output," goto _%x_ ;\n",IBuff.b.o);
			break;

		case IC_ERT :
			GetOffAndMove(CInst,IBuff.b.e.beg);
			GetIntAndMove(CInst,IBuff.b.e.size);
			fprintf(Output," return ( ");
			EPrint(Output,IBuff.b.e.beg,IBuff.b.e.size,DRET);
			fprintf(Output," ) ;\n");
			break;

		case IC_EXP :
			GetOffAndMove(CInst,IBuff.b.e.beg);
			GetIntAndMove(CInst,IBuff.b.e.size);
			fprintf(Output," ");
			EPrint(Output,IBuff.b.e.beg,IBuff.b.e.size,DEXP);
			fprintf(Output," ;\n");
			break;

		default :
			fprintf(

			stderr,
			"%s : bad instruction code in %s during OutputCode() <%d>, fatal error.\n",
			Me,
			FileName(CODEFN,TMPEXT),
			(int)IBuff.h

			);
			LeaveApp(1);
		}
	}

	fprintf(Output,"_%x_ :\n ;\n",(int)CInst-(int)CodeBase);

#ifdef DEBUG
	if(DebugInfo) putc('\n',stdout);
#endif

	fclose(SrcFile);
}
/*--------------------------------------------------*/
void AppendRes()
{
int
	c;

FILE
	*in;

	if((in=fopen(FileName(LSFN,TMPEXT),"r+"))==NULL) {
		fprintf(stderr,"%s : can't open labswp result %s, quit.\n",Me,FileName(LSFN,TMPEXT));
		LeaveApp(1);
	}

	while((c=getc(in))!=EOF) putc(c,LexOut);

	fclose(in);
}
/*--------------------------------------------------*/
void AppendAO()
{
int
	c;

FILE
	*in;

	if((in=fopen(FileName(AOFN,TMPEXT),"r+"))==NULL) {
		fprintf(stderr,"%s : can't open %s, quit.\n",Me,FileName(AOFN,TMPEXT));
		LeaveApp(1);
	}

	while((c=getc(in))!=EOF) putc(c,LexOut);

	fclose(in);
}
/*--------------------------------------------------*/
void CodeGen(sourceFile)
char
	*sourceFile;
{
	/* Chargement du code en memoire. */
	LoadCodeFile();

	/* Elimination des chainages de "goto". */
	UnChainGotos();

	/*
		Redirection du fichier resultat vers la commande
		d'elinination des etiquettes inutiles, si necessaire.
	*/
	if(LabSwPurge) {
		BuildLabSwpCmd();
		/* fprintf(stdout,"\n\n%s\n\n",LabSwpCmd); */
		if((Output=popen(LabSwpCmd,"w"))==NULL) {
			fprintf(stderr,"%s : can't pipe result file to labswp command, quit.\n",Me);
			LeaveApp(1);
		}
	}
	else Output=LexOut;

	/* Ouverture du fichier devant etre colle pour l'option -e. */
	if((AOutput=fopen(FileName(AOFN,TMPEXT),"w+"))==NULL) {
		fprintf(stderr,"%s : can't open %s, quit.\n",Me,FileName(AOFN,TMPEXT));
		LeaveApp(1);
	}

	/* Generation du code. */
	OutputCode(sourceFile);

	/* Liberation de la memoire. */
	XFree(CodeBase,CodeSize);

	/* Fermeture du fichier devant etre colle pour l'option -e. */
	fclose(AOutput);

	/*
		Recopie du fichier intermedaire a la fin du fichier
		resultat si c'est necessaire.
	*/
	if(LabSwPurge) {
		if(pclose(Output)) {
			fprintf(stderr,"%s : problem while labswp command, quit.\n",Me);
			LeaveApp(1);
		}
		AppendRes();
	}

	/* Fermeture de la fonction C courante. */
	fprintf(LexOut,"}\n\n");

	/* recopie du fichier devant etre colle pour l'option -e. */
	AppendAO();

	ForkNum=0;
}
/*--------------------------------------------------*/
