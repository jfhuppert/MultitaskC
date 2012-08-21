%{
/*----------------------------------------------------------------------------*/
/*
	pclexan.l
*/
/*----------------------------------------------------------------------------*/
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

#include <general.h>
#include <structs.h>
#include <pcsynan.h> /* Fichier des tokens genere par yacc. */
#include <symset.h>
#include <strings.h>

extern YYSTYPE pcyylval;

extern void
	MIdUsed(),
	FatalMemErr(),
	LeaveApp();

extern char
	CurrentFileName[],
	LastToken[],
	*XMalloc(),
	*Me;

extern int
#ifdef DEBUG
	DebugInfo,
#endif
	SwitchFlag,
	Warn;

extern StkDecl(SymSet,BNSTKSIZE)
	SSStk;

extern TaVal
	TaLookUp();

extern Offset
	GetOffset();

void
	PcEchoFunc();

static void
	NumLine(),
	Comment(),
	StringConstant();

Ident
	MIdent;

int
	MRFlag=0,
	PcLineNo;

FILE
	*PcResult,
	*PcStatement,
	*PcLexOut;

StkDecl(RBuf,BNSTKSIZE)
	RBStk;

RBuf
	NRBuf;

static Offset
	PO();

/*----------------------------------------------------------------------------*/
%}

ID		[A-Za-z_][A-Za-z0-9_]*
D		[0-9]
IS		[Ll]?[Uu]?
HD		[0-9a-fA-F]
HH		0[Xx]
E		[DEde][-+]?{D}+
FT		[FfLl]

%%

"'"			{ StringConstant('\''); return(pcyylval.offse=PO(pcyytext),CONSTANT); }
"\""		{ StringConstant('\"'); return(pcyylval.offse=PO(pcyytext),CONSTANT); }
"/""*"		{ Comment(); }

" "*		{ PcEchoFunc(0,pcyytext,pcyyleng); }
"\t"*		{ PcEchoFunc(0,pcyytext,pcyyleng); }
"\n"		{ PcEchoFunc(0,pcyytext,pcyyleng); PcLineNo++; }
""		{ PcEchoFunc(0,pcyytext,pcyyleng); }

auto		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),AUTO); }
register	{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),REGISTER); }
static		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),STATIC); }
extern		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),EXTERN); }
typedef		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),TYPEDEF); }
module		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),MODULE); }

void		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),VOID); }
char		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),CHAR); }
short		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),SHORT); }
int			{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),INT); }
long		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),LONG); }
float		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),FLOAT); }
double		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),DOUBLE); }
signed		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),SIGNED); }
unsigned	{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),UNSIGNED); }
struct		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),STRUCT); }
union		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),UNION); }
enum		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),ENUM); }

const		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),CONST); }
volatile	{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),VOLATILE); }

execute		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=EXECUTE); }
and			{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=AND); }

join		{ MRFlag=1; PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=JOIN); }
when		{ MRFlag=1; PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=WHEN); }
meeting		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=0,MEETING); }

if			{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=IF); }
else		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=ELSE); }
goto		{ NumLine(); PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=GOTO); }
while		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=WHILE); }
do			{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=DO); }
for			{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=FOR); }
block		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=BLOCK); }
group		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=GROUP); }
continue 	{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=CONTINUE); }
break		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=BREAK); }
return		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=RETURN); }

switch		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=SWITCH); }
case		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=CASE); }
default		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=DEFAULT); }

sizeof		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),SIZEOF); }

"."".""."	{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=DOTDOTDOT); }
"*""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=STAREQUAL); }
"/""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=SLASHEQUAL); }
"%""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=PERCENTEQUAL); }
"+""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=PLUSEQUAL); }
"-""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=MINUSEQUAL); }
"<""<""="	{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=LOWLOWEQUAL); }
">"">""="	{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=GRTGRTEQUAL); }
"&""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=AMPERSANDEQUAL); }
"^""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=HATEQUAL); }
"|""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=PIPEEQUAL); }
"|""|"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=PIPEPIPE); }
"&""&"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=AMPERSANDAMPERSAND); }
"=""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=EQUALEQUAL); }
"!""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=EMARKEQUAL); }
"<""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=LOWEQUAL); }
">""="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=GRTEQUAL); }
"<""<"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=LOWLOW); }
">"">"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=GRTGRT); }
"+""+"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),PLUSPLUS); }
"-""-"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),MINUSMINUS); }
"-"">"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=MINUSGRT); }

";"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=SEMICOLON); }
"{"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),OBRACE); }
"}"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),CBRACE); }
","		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=COMMA); }
"="		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=EQUAL); }
":"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=COLON); }
"("		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),OBRACKET); }
")"		{ if(MRFlag) MIdUsed(&MIdent),MRFlag=0; PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=CBRACKET); }
"["		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=OSQRBRACKET); }
"]"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=CSQRBRACKET); }
"*"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),STAR); }
"."		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=DOT); }
"/"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=SLASH); }
"%"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=PERCENT); }
"+"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),PLUS); }
"-"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),MINUS); }
"<"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=LOW); }
">"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=GRT); }
"&"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),AMPERSAND); }
"^"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=HAT); }
"|"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=PIPE); }
"?"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.integ=QMARK); }
"!"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),EMARK); }
"~"		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),TILDE); }

^#" "?[0-9]+" ""\"".*"\"".*$ { /* Un nouveau numero de ligne donne par cpp. */

int
	t;

char
	*p,
	*b;

	if(sscanf(pcyytext+1,"%d",&t)!=1) {
		fprintf(stderr,"%s : bad cpp statement near line %d of cpp result.\n",Me,PcLineNo);
		LeaveApp(1);
	}

	p=pcyytext+1;
	while(*p==' ') p++;
	while(isdigit(*p)) p++;
	while(*p==' ') p++;
	if(*p!='"') {
        fprintf(stderr,"%s : bad cpp statement near line %d of cpp result.\n",Me,PcLineNo);
        LeaveApp(1);
    }
	p++;

	b=CurrentFileName;
	while(*p!='"') *b++=*p++;
	*b=0;

	PcLineNo=t-1;

	PcEchoFunc(0,pcyytext,pcyyleng);

}

^#.*$ { /* On ignore les autres directives du preprocesseur C. */

	PcEchoFunc(0,pcyytext,pcyyleng);

}

{ID} {

TaVal
	tokenvalue;

SymSetPt
	symSet;

	if((pcyylval.ident.s=(char *)XMalloc(pcyyleng+1))==NULL) FatalMemErr("malloc() error in pclexan()");
	strcpy(pcyylval.ident.s,pcyytext);

	symSet=StkSP(SSStk);

	for(symSet=StkSP(SSStk);!StkEP(SSStk,symSet);symSet++) {

		tokenvalue.ival=TaLookUp(pcyytext,&symSet->SymSet).ival;

		switch(SS_GT(tokenvalue.ival)) {

		case 0 :
			break;

		case T_MIDENTIF :
			pcyylval.ident.t=(symSet==StkSP(SSStk)?L_MIDENT:G_MIDENT);
			pcyylval.ident.bn=symSet->SSNum;
			pcyylval.ident.f=SS_GF(tokenvalue.ival);
			PcEchoFunc(pcyyleng,pcyytext,pcyyleng);
			pcyylval.ident.o=pcyylval.ident.ido=PO(pcyytext);
			MIdent=pcyylval.ident;
			return(IDENTIFIER);

		case T_FIDENTIF :
			pcyylval.ident.t=(symSet==StkSP(SSStk)?L_FIDENT:G_FIDENT);
			pcyylval.ident.bn=SS_NREN;
			pcyylval.ident.f=SS_GF(tokenvalue.ival);
			PcEchoFunc(pcyyleng,pcyytext,pcyyleng);
			pcyylval.ident.o=pcyylval.ident.ido=PO(pcyytext);
			MIdent=pcyylval.ident;
			return(IDENTIFIER);

		case T_TYPENAME :
			pcyylval.ident.t=(symSet==StkSP(SSStk)?L_IDENT:G_IDENT);
			pcyylval.ident.bn=symSet->SSNum;
			pcyylval.ident.f=SS_GF(tokenvalue.ival);
			PcEchoFunc(pcyyleng,pcyytext,pcyyleng);
			pcyylval.ident.o=pcyylval.ident.ido=PO(pcyytext);
			MIdent=pcyylval.ident;
			return(TYPEDEFNAME);

		case T_IDENTIF :
			pcyylval.ident.t=(symSet==StkSP(SSStk)?L_IDENT:G_IDENT);
			pcyylval.ident.bn=symSet->SSNum;
			pcyylval.ident.f=SS_GF(tokenvalue.ival);
			PcEchoFunc(pcyyleng,pcyytext,pcyyleng);
			pcyylval.ident.o=pcyylval.ident.ido=PO(pcyytext);
			MIdent=pcyylval.ident;
			return(IDENTIFIER);

		default :
			fprintf(stderr,"%s : invalid token type <%d> for <%s> line %d of cpp result.\n",Me,tokenvalue.ival,pcyytext,PcLineNo);
			LeaveApp(1);
			break;

		}

	}

	pcyylval.ident.t=U_IDENT;
	pcyylval.ident.bn=SS_NREN;
	pcyylval.ident.f=0;
	PcEchoFunc(pcyyleng,pcyytext,pcyyleng);
	pcyylval.ident.o=pcyylval.ident.ido=PO(pcyytext);
	MIdent=pcyylval.ident;
	return(IDENTIFIER);

}

{D}+{IS}	{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),CONSTANT); }
{HH}{HD}+	{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),CONSTANT); }

{D}*"."{D}+({E})?({FT})?	|
{D}+"."{D}*({E})?({FT})?	|
{D}+{E}({FT})?		{ PcEchoFunc(0,pcyytext,pcyyleng); return(pcyylval.offse=PO(pcyytext),CONSTANT); }

.		{ return(pcyylval.integ=UNKNOWNTOKEN); }

%%
/*----------------------------------------------------------------------------*/
Offset PO(s)
char
	*s;
{
	return(GetOffset(PcResult));
}
/*----------------------------------------------------------------------------*/
char *RBRealloc(rb,siz)
RBufPt
	rb;

int
	siz;
{
char
	*p;

	p=XMalloc(rb->siz+siz+1);
	if(p==NULL) FatalMemErr("RBRealloc in pre-comp");
	strcpy(p,rb->buf);
	XFree(rb->buf,rb->siz+1);
	rb->buf=p;
	rb->siz+=siz;
	return(rb->buf);
}
/*----------------------------------------------------------------------------*/
void PcEchoFunc(mem,t,ts)
int
	ts,
	mem;

char
	*t;
{
#ifdef DEBUG
static char
	result[]="result",
	statement[]="statement",
	*CFN=result;
#endif

	switch(SwitchFlag) {
	
	case F_DSWITCH :
#ifdef DEBUG
		if(DebugInfo) fprintf(stdout,"DSwitchFlag on token \"%s\" from %s to %s\n",LastToken,CFN,CFN==result?statement:result);
		CFN=(CFN==result?statement:result);
		if(!StkEmpty(RBStk)) { fprintf(stdout,"!!!!!!!!! F_DSWITCH on %s !!!!!!!!!\n",LastToken); }
#endif
		fprintf(PcLexOut,"%s",LastToken),LastToken[0]='\0';
		PcLexOut=(PcLexOut==PcResult?PcStatement:PcResult);
		SwitchFlag=0;
		break;

	case F_NSWITCH :
#ifdef DEBUG
		if(DebugInfo) fprintf(stdout,"NSwitchFlag on token \"%s\" from %s to %s\n",StkEmpty(RBStk)?LastToken:StkTop(RBStk).buf,CFN,CFN==result?statement:result);
		CFN=(CFN==result?statement:result);
#endif
		PcLexOut=(PcLexOut==PcResult?PcStatement:PcResult);
		SwitchFlag=0;
	case 0 :
		if(StkEmpty(RBStk)) fprintf(PcLexOut,"%s",LastToken),LastToken[0]='\0';
		break;

	default :
		fprintf(stderr,"%s : pre-compiler : Unknown SwitchFlag on token \"%s\"\n",StkEmpty(RBStk)?LastToken:StkTop(RBStk).buf);
		LeaveApp(1);

	}

	if(mem!=0) {
#ifdef DEBUG
		if(DebugInfo) fprintf(stdout,"starting MemOutput for \"%s\"\n",t);
#endif
		if(StkFull(RBStk)) FatalMemErr("overflow in pre-comp for RBStk");
		StkPush(RBStk,NRBuf);
		StkTop(RBStk).buf=XMalloc(ts+1);
		if(StkTop(RBStk).buf==NULL) FatalMemErr("XMalloc() in pre-comp for StkTop(RBStk).buf");
		StkTop(RBStk).siz=ts;
		*StkTop(RBStk).buf='\0';
	}
	else if(!StkEmpty(RBStk)) RBRealloc(&StkTop(RBStk),ts);

	strcat(StkEmpty(RBStk)?LastToken:StkTop(RBStk).buf,t);
}
/*----------------------------------------------------------------------------*/
static void StringConstant(end)
char
	end;
{
char
	chr,
	*p,
	SCBuf[ITMAXLEN];

int
	msgflg=1;

	strcpy(SCBuf,pcyytext);
	p=(SCBuf+pcyyleng);

	FOREVER {

		chr=(char)input();

		if((p-SCBuf)>=(ITMAXLEN-2)) {
			if(msgflg) {
				if(Warn) fprintf(stderr,"%s : warning, string too long line %d, truncated.\n",Me,PcLineNo);
				msgflg=0;
				*p++=end;
			}
		}
		else *p++=chr;

		switch(chr) {

		case '\\' :
			chr=(char)input();
			if((p-SCBuf)>=(ITMAXLEN-2)) {
				if(msgflg) {
					if(Warn) fprintf(stderr,"%s : warning, string too long line %d, truncated.\n",Me,PcLineNo);
					msgflg=0;
					*p++=end;
				}
			}
			else *p++=chr;
			break;

		default :
			if(chr==end) {
				*p='\0';
				PcEchoFunc(0,SCBuf,(int)(p-SCBuf));
				return;
			}
			break;
		}
	}
}
/*----------------------------------------------------------------------------*/
static void Comment()
{
char
	chr;

	FOREVER {
		chr=(char)input();
testlabel:
		if(chr=='*') {
			chr=(char)input();
			if(chr=='/') return;
			else goto testlabel;
		}
	}
}
/*----------------------------------------------------------------------------*/
static void NumLine()
{
char
	str[MAXOFLEN];

	PcEchoFunc(0,str,sprintf(str,"\n# %d \"%s\"\n",PcLineNo,CurrentFileName));
}
/*----------------------------------------------------------------------------*/
