/*--------------------------------------------------*/
/*
	main.c
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
#include <errno.h>
#include <strings.h>

extern void
	FTaFree(),
	(*EPrint)(),
	NSS(),
	PcEchoFunc(),
	KSS(),
	SEPrint(),
	DEPrint();

extern char
	*getenv(),
	ExtRedFStr[],
	*CFuncName,
	LastToken[];

extern int
#ifdef YYDEBUG
	pcyydebug,
	yydebug,
#endif
	DeclScope,
	SwitchFlag,
	PcLineNo,
	NextBNNum,
	NextBLKId,
	NextMeetingId,
	LineNo,
	EscStatement,
	MDecl,
	NullFunc(),
	OutputFunc(),
	(*EchoFunc)(),
	pcyyparse(),
	yyparse();

extern FILE
#ifdef DEBUG
	*DSrcFile,
#endif
	*pcyyin,
	*PcResult,
	*PcStatement,
	*PcLexOut,
	*LexIn,
	*LexOut;

extern StkDecl(RBuf,BNSTKSIZE)
	RBStk;

extern TaState
	BlkSymSet,
	SyncSymSet,
	LabelSymSet,
	LocalSymSet,
	GlobalSymSet;
	
extern StkDecl(int,BNSTKSIZE)
	BNodeStk;

extern StkDecl(SCCount,SNSTKSIZE)
	SNodeStk;

extern StkDecl(SymSet,BNSTKSIZE)
	SSStk;

int
#ifdef DEBUG
	DebugInfo=0,
	Trace=0,
#endif
	MaxMem,
	NIFOptions=0,
	OutputERF=1,
	CurMemSiz,
	AllocMenSiz,
	FreeMemSiz,
	ForkOption=COMPFORK,
	LabSwPurge=1,
	DelInput=1,
	Interm=0,
	FirstPassOnly=0,
	MemStat=0,
	Results=0,
	DebugMode=0,
	Warn=1;

char
	TmpDir[MAXOFLEN]=DEFTMPDIR,
	Cpp[MAXOFLEN]=DEFCPP,
	CppP[MAXOFLEN]="",
	RInput[MAXOFLEN]="",
	CurrentFileName[MAXOFLEN]="",
	CompSrcFileName[MAXOFLEN]="",
	*OutputFile=NULL,
	*InputFile=NULL,
	*Me;
	
FILE
	*ptreef,
	*gotof;

SymSet
	EmptySymSet;

/*------------------------------------------------*/

static char
StdTypesFmt[]="\
\n\
struct %s;\n\
typedef struct %s {\n\
\n\
	unsigned char h;\n\
	union {\n\
		unsigned int  i;\n\
		unsigned long o;\n\
		struct { unsigned int i; unsigned long o; } r;\n\
		struct { unsigned int i; unsigned long o; } j;\n\
		struct { unsigned int i; unsigned long o0,o1; } w;\n\
		struct { unsigned int i; unsigned long o0,o1; } b;\n\
		struct { unsigned long pc1,pc2; struct %s *tcb1,*tcb2; } f;\n\
	} b;\n\
\n\
} %s ;\n\
\n\
typedef struct %s {\n\
\n\
	unsigned short id;\n\
	unsigned long  pc;\n\
\n\
} %s ;\n\
\n\
typedef struct %s {\n\
\n\
	unsigned int card;\n\
	unsigned int n;\n\
\n\
} %s ;\n\
\n\
typedef struct %s {\n\
\n\
	unsigned char type;\n\
	unsigned int nb;\n\
	%s *bstack;\n\
	union {\n\
		struct { struct %s *l,*r; } s;\n\
		struct { unsigned long pc; %s i; } l;\n\
	} b;\n\
\n\
} %s ;\n\
\n\
";

char
	StdTypes[sizeof(StdTypesFmt)+4096];

/*------------------------------------------------*/

static char
StdHeader[]="";

/*------------------------------------------------*/
/* Pour lex. */
int yywrap() { return(1); }
int pcyywrap() { return(1); }
/*------------------------------------------------*/
char *XMalloc(s)
unsigned
	s;
{
char
	*malloc(),
	*p;

	p=malloc(s);
	AllocMenSiz+=s;
	CurMemSiz+=s;
	if(CurMemSiz>MaxMem) MaxMem=CurMemSiz;
	return(p);
}
/*------------------------------------------------*/
void XFree(p,s)
char
	*p;

unsigned
	s;
{
void
	free();

	FreeMemSiz+=s;
	CurMemSiz-=s;
	free(p);
}
/*------------------------------------------------*/
char *FileName(p,e)
char
	*p,
	*e;
{
static char
	buff[MAXOFLEN];

int
	getpid();

	sprintf(buff,"%s/%s%d%s",TmpDir,p,getpid(),e);
	return((char *)buff);
}
/*------------------------------------------------*/
LeaveApp(c)
int
	c;
{

#ifdef DEBUG
	if(DebugInfo==0) {
		unlink(LPTREEFN);
		unlink(LGOTOFN);
		unlink(LCODEFN);
	}
#endif

	if(DelInput) unlink(RInput);

	unlink(FileName(PTREEFN,TMPEXT));
	unlink(FileName(GOTOFN,TMPEXT));
	unlink(FileName(CODEFN,TMPEXT));
	unlink(FileName(STFN,TMPEXT));
	unlink(FileName(LSFN,TMPEXT));
	unlink(FileName(AOFN,TMPEXT));
	unlink(FileName(LABFN,TMPEXT));
	unlink(FileName(DIFN,TMPEXT));
	unlink(FileName(PCFN,TMPEXT));

	if(c!=0) {
		unlink(OutputFile);
	}

#ifdef DEBUG
	if(DebugInfo) {
		putc('\n',stdout);
		if(FirstPassOnly==0) fclose(DSrcFile);
	}
#endif

	exit(c);
}
/*------------------------------------------------*/
RdFile(ptr,size,n,file)
void
	*ptr;

unsigned int
	size,
	n;

FILE
	*file;
{
unsigned int
	ret;

	ret=fread(ptr,size,n,file);
	if(ret==0 && ferror(file)) {
		fprintf(
			stderr,
			"%s : fatal fread() error %d, quit.\n",
			Me,
			ferror(file)
		);
		LeaveApp(errno);
	}
	return(ret);
}
/*------------------------------------------------*/
WrFile(ptr,size,n,file)
void
	*ptr;

unsigned int
	size,
	n;

FILE
	*file;
{
unsigned int
	ret;

	ret=fwrite(ptr,size,n,file);
	if(ret==0 && ferror(file)) {
		fprintf(
			stderr,
			"%s : fatal fwrite() error %d, quit.\n",
			Me,
			ferror(file)
		);
		LeaveApp(errno);
	}
	return(ret);
}
/*------------------------------------------------*/
Offset GetOffset(file)
FILE
	*file;
{
Offset
	ret;

	if((ret=(Offset)ftell(file))==(Offset)EOF) {
		fprintf(
			stderr,
			"%s : ftell() error, fatal.\n",
			Me
		);
		LeaveApp(errno);
	}

	return(ret);
}
/*------------------------------------------------*/
void SetOffset(file,o,f)
FILE
	*file;

Offset
	o;

int
	f;
{
	if(fseek(file,(long)o,f)==EOF) {
		fprintf(
			stderr,
			"%s : fseek() error, fatal.\n",
			Me
		);
		LeaveApp(errno);
	}
}
/*------------------------------------------------*/
usage(name,str)
char
	*name,
	*str;
{
	fprintf(stderr,"\n%s : %s\n",name,str);
	fprintf(stderr,"usage : %s {[opts]} input {[opts]} output {[opts]}.\n",name);
	fprintf(stderr,"\nstandard cpp options are :\n");
	fprintf(stderr,"  I : specifies a new directory name for cpp.\n");
	fprintf(stderr,"  D : specifies a definition.\n");
	fprintf(stderr,"  U : undefine something.\n");

	fprintf(stderr,"\nnon standard options are :\n");
	fprintf(stderr,"  1 : first pass only.\n");
#ifdef DEBUG
	fprintf(stderr,"  d : debug, print info while compiling.\n");
#endif
	fprintf(stderr,"  f : don't output ER function with -e option.\n");
	fprintf(stderr,"  g : produce debug output (execution trace).\n");
	fprintf(stderr,"  h : help, print this message.\n");
	fprintf(stderr,"  i : generate intermediate code.\n");
	fprintf(stderr,"  m : print memory stat.\n");
	fprintf(stderr,"  n : don't call cpp.\n");
	fprintf(stderr,"  p : specifies a new cpp.\n");
	fprintf(stderr,"  r : print results (nb states, ...).\n");
	fprintf(stderr,"  c : generates compiled synchronous product (default).\n");
	fprintf(stderr,"  s : generates semi-static synchronous product.\n");
	fprintf(stderr,"  e : generates interpreted synchronous product.\n");
	fprintf(stderr,"  t : specifies a directory for tmp files.\n");
	fprintf(stderr,"  u : don't purge labels and switches.\n");
	fprintf(stderr,"  v : verbose, trace synchronous product.\n");
	fprintf(stderr,"  w : no warnings.\n");
#ifdef YYDEBUG
	fprintf(stderr,"  y : debug, print syntax analyser info.\n");
#endif

	fprintf(stderr,"\n");

	LeaveApp(1);
}
/*------------------------------------------------*/
void OutputERFunc()
{
	WrFile(ExtRedFStr,1,strlen(ExtRedFStr),LexOut);
}
/*------------------------------------------------*/
PreProcessInput(h,t,in,cppc)
char
	*h,
	*t,
	*in,
	*cppc;
{
char
	chr;

FILE
	*input,
	*output;

	/* ouverture du fichier source. */
	if((input=fopen(in,"r+"))==NULL) {
		fprintf(stderr,"%s : can't open input file <%s>, quit.\n",Me,in);
		return(1);
	}

	/* Creation d'un process "cpp". */
	if((output=popen(cppc,"w"))==NULL) {
		fprintf(stderr,"%s : can't pipe input file to cpp command <%s>, quit.\n",Me,cppc);
		return(1);
	}

	/* Numerotation de la premiere ligne du source. */
	fprintf(output,"\n#line 1 \"standard_header\"\n");

	/* Ecriture du header standard. */
	WrFile(h,1,strlen(h),output);

	/* Ecriture des declarations de types standard. */
	WrFile(t,1,strlen(t),output);

	/* Numerotation de la premiere ligne du source. */
	fprintf(output,"\n#line 1 \"%s\"\n",InputFile);

	/* Ecriture du fichier source. */
	while(RdFile(&chr,1,1,input)) WrFile(&chr,1,1,output);

	/* Fermeture des fichiers. */

	fclose(input);

	if(pclose(output)) {
		fprintf(stderr,"%s : problem while cpp command, quit.\n",Me);
		return(1);
	}

	return(0);
}
/*------------------------------------------------*/
void PcFEnd()
{
int
	c;

	if(!StkEmpty(RBStk)) fprintf(stdout,"%s : Memory Output !!! in PcFEnd().",Me);

	PcEchoFunc(0,"",0);
	fclose(PcStatement);

	/* Ouverture du fichier instruction pour le pre-comp : statement. */
	if((PcStatement=fopen(FileName(CODEFN,TMPEXT),"r+"))==NULL) {
		fprintf(
			stderr,
			"%s : can't open <%s>, quit.\n",
			Me,
			FileName(CODEFN,TMPEXT)
		);
		LeaveApp(errno);
	}
	
	PcLexOut=PcResult;
	SwitchFlag=0;
	while((c=getc(PcStatement))!=EOF) putc(c,PcLexOut);

	fclose(PcStatement);

	/* Ouverture du fichier instruction pour le pre-comp : statement. */
	if((PcStatement=fopen(FileName(CODEFN,TMPEXT),"w+"))==NULL) {
		fprintf(
			stderr,
			"%s : can't open <%s>, quit.\n",
			Me,
			FileName(CODEFN,TMPEXT)
		);
		LeaveApp(errno);
	}
}
/*------------------------------------------------*/
int InsertBuiltinsTypes(ss)
TaStatePt
        ss;
{
static char
	*builtins[]={
		"__builtin_va_list",
		NULL
	};

char
	**cb;

TaVal
	tokenvalue;

int
	notinserted;

	cb=builtins;
	while(*cb!=NULL) {
		notinserted=0;
		tokenvalue.ival=T_TYPENAME;
		TaInsert(*cb,ss,&notinserted,&tokenvalue);
		if(notinserted) {
			fprintf(stderr,"Builtin %s not inserted in global symset !\n",*cb);
			LeaveApp(1);
		}
		cb++;
	}
}
/*------------------------------------------------*/
/* Point d'entree. */
int main(argc,argv)
int
	argc;
	
char
	*argv[];
{
char
	errbuff[MAXLEN],
	*curchr,
	*cppp,
	**v;

	MaxMem=0;
	CurMemSiz=0;
	AllocMenSiz=0;
	FreeMemSiz=0;
	MemStat=0;

	/* Initialisation de la fonction de generation d'expression. */
	EPrint=SEPrint;

	Me=argv[0];

	/* Analyse de la liste des arguments. */
	cppp=CppP;
	v=argv;
nextarg:
	while((curchr=(*++v))!=NULL) {
	
		if(*curchr=='-') {
		
			/* Traitement d'une liste d'options. */
			while(*++curchr) {
				switch(*curchr) {
				
				case '?' :
				case 'h' :
					usage(argv[0],"");
					
				case 'w' :
					Warn=0;
					break;

				case 'i' :
					Interm=1;
					break;

				case 'f' :
					OutputERF=0;
					break;

				case 'v' :
					Trace=1;
					break;

				case 'm' :
					MemStat=1;
					break;

				case 'n' :
					DelInput=0;
					break;

				case 'r' :
					Results=1;
					break;

				case 'g' :
					EPrint=DEPrint;
					DebugMode=1;
					break;

				case 'u' :
					LabSwPurge=0;
					break;

				case 'c' :
					ForkOption=COMPFORK;
					break;

				case 's' :
					ForkOption=SIMFORK;
					break;

				case '1' :
					FirstPassOnly=1;
					break;

				case 'e' :
					ForkOption=INTERFORK;
					break;

				case 't' :
					if(*(curchr+1)=='\0')
						usage(argv[0],"-t option without directory name");
					strncpy(TmpDir,curchr+1,MAXOFLEN);
					goto nextarg;

				case 'p' :
					if(*(v+1)==NULL)
						usage(argv[0],"-p option without a cpp name");
					strncpy(Cpp,*(++v),MAXOFLEN);
					goto nextarg;
					
				case 'D' :
					if(*(curchr+1)=='\0')
						usage(argv[0],"-D option without something to def");
					*cppp++='-';
					strcpy(cppp,curchr);
					cppp+=strlen(curchr);
					*cppp++=' ';
					goto nextarg;

				case 'U' :
					if(*(curchr+1)=='\0')
						usage(argv[0],"-U option without something to undef");
					*cppp++='-';
					strcpy(cppp,curchr);
					cppp+=strlen(curchr);
					*cppp++=' ';
					goto nextarg;

				case 'I' :
					if(*(curchr+1)=='\0')
						usage(argv[0],"-I or -Y option without a path");
					*cppp++='-';
					strcpy(cppp,curchr);
					cppp+=strlen(curchr);
					*cppp++=' ';
					goto nextarg;

#ifdef DEBUG
				case 'd' :
					DebugInfo=1;
					break;
#endif

#ifdef YYDEBUG
				case 'y' :
					pcyydebug=1;
					yydebug=1;
					break;
#endif

				/* Options futures ici. */
				
				default :
					sprintf(errbuff,"unknown option -%c",*curchr);
					usage(argv[0],errbuff);
					
				}
			}

		}
		else {
		
			/* Traitement d'un argument normal. */
			if(InputFile==NULL) InputFile=curchr;
			else if(OutputFile==NULL) OutputFile=curchr;
			else
			 usage(
				 argv[0],
				 "too many file names (2 max, one for input, one for output)"
			 );
			
		}
	}

	if(InputFile==NULL) {
		usage(argv[0],"no file to compile");
	}
	else strcpy(CurrentFileName,InputFile);

	if(OutputFile==NULL) {
		usage(argv[0],"no output file");
	}

	if(getenv(BEE_COMPF)!=NULL) ForkOption=COMPFORK;
	else if(getenv(BEE_SIMF)!=NULL) ForkOption=SIMFORK;
	else if(getenv(BEE_INTERF)!=NULL) ForkOption=INTERFORK;

	fclose(stdin);

#ifdef DEBUG
	if(DebugInfo) {
		fprintf(stdout,"\n%s : %d argument(s) given\n",Me,argc-1);
	}
#endif

	if(DelInput) {
		strcat(Cpp," ");
		strcat(Cpp,CppP);
		strcat(Cpp," > ");
		strcpy(RInput,FileName(CPPOUT,TMPEXT));
		strcat(Cpp,RInput);

#ifdef DEBUG
		if(DebugInfo) {
			fprintf(stdout,"cpp command : <%s>\n",Cpp);
			fprintf(stdout, "output file name : <%s>\n",OutputFile);
		}
#endif

		sprintf(
			StdTypes,
			StdTypesFmt,
			TCBSNAME,
			INSTSNAME,
			TCBSNAME,
			INSTTNAME,
			BSCSNAME,
			BSCTNAME,
			MEETSNAME,
			MEETTNAME,
			TCBSNAME,
			BSCTNAME,
			TCBSNAME,
			INSTTNAME,
			TCBTNAME
		);

		if(PreProcessInput(StdHeader,StdTypes,InputFile,Cpp)) {
			fprintf(stderr,"%s : cpp error.\n",Me);
			LeaveApp(1);
		}
	}
	else {
		strcpy(RInput,InputFile);

#ifdef DEBUG
		if(DebugInfo) {
			fprintf(stdout,"cpp <%s> is not called\n",Cpp);
		}
#endif

	}
	
	/* Ouverture du fichier source pour le pre-comp. */
	if((pcyyin=fopen(RInput,"r+"))==NULL) {
		fprintf(
			stderr,
			"%s : can't open <%s>, quit.\n",
			argv[0],
			RInput
		);
		LeaveApp(errno);
	}
	
	/* Ouverture du fichier resultat pour le pre-comp : result. */
	strcpy(CompSrcFileName,FileName(PCFN,TMPEXT));
	unlink(CompSrcFileName);
	if((PcResult=fopen(CompSrcFileName,"a+"))==NULL) {
		fprintf(
			stderr,
			"%s : can't open <%s>, quit.\n",
			argv[0],
			CompSrcFileName
		);
		LeaveApp(errno);
	}
	
	/* Ouverture du fichier instruction pour le pre-comp : statement. */
	if((PcStatement=fopen(FileName(CODEFN,TMPEXT),"w+"))==NULL) {
		fprintf(
			stderr,
			"%s : can't open <%s>, quit.\n",
			argv[0],
			FileName(CODEFN,TMPEXT)
		);
		LeaveApp(errno);
	}
	
	/* Initialisation du pre-comp. */
	PcLexOut=PcResult;
	SwitchFlag=0;
	PcLineNo=1;
	NextBNNum=1;
	LastToken[0]='\0';
	
	/* Initialisation de la pile des buffers de renomage. */
	StkInit(RBStk,BNSTKSIZE);

	/* Initialisation de la pile des ensembles de symboles par block. */
	TaInit(EmptySymSet.SymSet);
	TaInit(EmptySymSet.SSSet);
	TaInit(EmptySymSet.USSet);
	TaInit(EmptySymSet.ESSet);
	StkInit(SSStk,BNSTKSIZE);
	NSS(SS_NREN);

	/* Insertion dans l'ensemble des symboles globaux de type predefinis. */
	InsertBuiltinsTypes(&StkTop(SSStk).SymSet);

	/* Lancement du pre-comp sur le fichier source. */
	if((argc=pcyyparse())!=0) {
		/* Impossible normalement. */
		fprintf(stderr,"%s : spurious return of pcyyparse() (%d).\n",Me,argc);
	}

	/* Liberation de l'ensemble global des symboles. */
	KSS();

	PcEchoFunc(0,"",0);

	if(!StkEmpty(RBStk)) fprintf(stdout,"%s : Memory Output !!! after pre-compiler pass.",Me);
	if(PcLexOut!=PcResult) fprintf(stdout,"%s : PcLexOut!=PcResult !!! after pre-compiler pass.",Me);

	fclose(PcResult);
	fclose(PcStatement);
	fclose(pcyyin);

	unlink(FileName(CODEFN,TMPEXT));

	if(FirstPassOnly) {
		rename(CompSrcFileName,OutputFile);
		goto main_end;
	}

	/* Ouverture du fichier resultat du pre-comp : source du compilateur. */
	if((LexIn=fopen(CompSrcFileName,"r+"))==NULL) {
		fprintf(
			stderr,
			"%s : can't open <%s>, quit.\n",
			argv[0],
			CompSrcFileName
		);
		LeaveApp(errno);
	}
	
#ifdef DEBUG
	if(DebugInfo) {
		fprintf(stdout,"compiler input file is <%s>\n",CompSrcFileName);
		if((DSrcFile=fopen(CompSrcFileName,"r+"))==NULL) {
			fprintf(
				stderr,
				"%s : can't reopen <%s>, quit.\n",
				Me,
				CompSrcFileName
			);
			LeaveApp(errno);
		}
	}
#endif

	/* Ouverture du fichier resultat. */
	if((LexOut=fopen(OutputFile,"w+"))==NULL) {
		fprintf(
			stderr,
			"%s : can't open <%s>, quit.\n",
			argv[0],
			OutputFile
		);
		LeaveApp(errno);
	}

	/* Ouverture d'un fichier temporaire. */
	if((gotof=fopen(FileName(GOTOFN,TMPEXT),"w+"))==NULL) {
		fprintf(
			stderr,
			"%s : can't open <%s>, quit.\n",
			argv[0],
			FileName(GOTOFN,TMPEXT)
		);
		LeaveApp(errno);
	}
	
	/* Ouverture d'un fichier temporaire. */
	if((ptreef=fopen(FileName(PTREEFN,TMPEXT),"w+"))==NULL) {
		fprintf(
			stderr,
			"%s : can't open <%s>, quit.\n",
			argv[0],
			FileName(PTREEFN,TMPEXT)
		);
		LeaveApp(errno);
	}
	else InitFile(ptreef);
	
	/* Initialisation de l'analyseur. */
	EscStatement=0;
	MDecl=0;
	DeclScope=SCOPE_G;
	LineNo=1;
	NextBNNum=1;
	NextBLKId=1;
	NextMeetingId=0;
	EchoFunc=OutputFunc;
	CFuncName=NULL;
	LastToken[0]='\0';

	/* Initialisation de la pile des # de case dans les switch. */
	StkInit(SNodeStk,SNSTKSIZE);

	/* Initialisation de la pile des numeros de blocks. */
	StkInit(BNodeStk,BNSTKSIZE);
	StkPush(BNodeStk,0);

	/* Initialisation de l'ensemble des symboles globaux. */
	TaInit(GlobalSymSet);

	/* Insertion dans l'ensemble des symboles globaux de type predefinis. */
	InsertBuiltinsTypes(&GlobalSymSet);
	
	/* Initialisation des ensembles de symboles locaux. */
	TaInit(BlkSymSet);
	TaInit(SyncSymSet);
	TaInit(LocalSymSet);
	TaInit(LabelSymSet);
	
	/* Lancement de l'analyseur syntaxique sur le fichier source. */
	if((argc=yyparse())!=0) {
		/* Impossible normalement. */
		fprintf(stderr,"%s : spurious return of yyparse() (%d).\n",Me,argc);
	}

	/* Liberation de l'ensemble global des TYPEDEFNAME. */
	FTaFree(&GlobalSymSet,0);

	if(OutputERF && NIFOptions) OutputERFunc();

main_end:

	if(MemStat) {
		fprintf(stdout,"%s : max memory        = %d.\n",Me,MaxMem);
		fprintf(stdout,"%s : total XMalloc()   = %d.\n",Me,AllocMenSiz);
		fprintf(stdout,"%s : total XFree()     = %d.\n",Me,FreeMemSiz);
		fprintf(stdout,"%s : XMalloc()-XFree() = %d.\n",Me,AllocMenSiz-FreeMemSiz);
	}

	/* Arret de l'execution sans erreur. */
	LeaveApp(0);
}
/*------------------------------------------------*/
