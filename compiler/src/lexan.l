%{
/*----------------------------------------------------------------------------*/
/*
	lexan.l
*/
/*----------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------*/

#include <structs.h>
#include <general.h>
#include <synan.h> /* Fichier des tokens genere par yacc. */
#include <symset.h>
#include <strings.h>

/*----------------------------------------------------------------------------*/
#ifdef YY_INPUT
#undef YY_INPUT
#endif
#define YY_INPUT(buf,result,max_size) {\
	int c;\
\
	c=getc(LexIn);\
\
	result=(c==EOF)?YY_NULL:(buf[0]=c,1);\
}
/*----------------------------------------------------------------------------*/

extern void
	FatalMemErr();

extern char
	*XMalloc(),
	CurrentFileName[],
	*Me;

extern int
	Warn,
	NextBNNum;

extern StkDecl(int,BNSTKSIZE)
	BNodeStk;

extern StkDecl(SCCount,SNSTKSIZE)
	SNodeStk;

extern TaVal
	TaLookUp();

extern TaState
	LocalSymSet,
	GlobalSymSet;

extern Offset
	GetOffset();

static ExprPt
	SkipStatement();
	
static void
	Comment(),
	StringConstant();

char
	LastToken[ITMAXLEN];

int
	LineNo,
	EscStatement,
	MDecl,
	NullFunc(),
	OutputFunc(),
	(*EchoFunc)();

SCCount
	ZSCCount={0,0};

FILE
	*LexIn,
	*LexOut;

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

"'"	{

	if(MDecl) {
		fprintf(stderr,"%s : bad meeting declaration line %d.\n",Me,LineNo);
		LeaveApp(1);
	}
	else yylval.offse=GetOffset(LexIn)-1;
	StringConstant('\'');
	return(CONSTANT);

}

"\"" {

	if(MDecl) {
		fprintf(stderr,"%s : bad meeting declaration line %d.\n",Me,LineNo);
		LeaveApp(1);
	}
	else yylval.offse=GetOffset(LexIn)-1;
	StringConstant('\"');
	return(CONSTANT);

}

"/""*"		{ Comment(); }

" "*		{ EchoFunc(LexOut,yytext); }
"\t"*		{ EchoFunc(LexOut,yytext); }
"\n"		{ EchoFunc(LexOut,yytext); LineNo++; }
""		{ EchoFunc(LexOut,yytext); }

auto		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,AUTO); }
register	{ EchoFunc(LexOut,yytext); return(yylval.integ=0,REGISTER); }
static		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,STATIC); }
extern		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,EXTERN); }
typedef		{ EchoFunc(LexOut,yytext); return(yylval.integ=1,TYPEDEF); }

void		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,VOID); }
char		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,CHAR); }
short		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,SHORT); }
int			{ EchoFunc(LexOut,yytext); return(yylval.integ=0,INT); }
long		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,LONG); }
float		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,FLOAT); }
double		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,DOUBLE); }
signed		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,SIGNED); }
unsigned	{ EchoFunc(LexOut,yytext); return(yylval.integ=0,UNSIGNED); }
struct		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,STRUCT); }
union		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,UNION); }
enum		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,ENUM); }

const		{ EchoFunc(LexOut,yytext); return(yylval.integ=0,CONST); }
volatile	{ EchoFunc(LexOut,yytext); return(yylval.integ=0,VOLATILE); }

execute		{ if(StkFull(BNodeStk)) FatalMemErr("stack overflow in lexan()"); StkPush(BNodeStk,NextBNNum++); return(yylval.integ=EXECUTE); }
and			{ StkTop(BNodeStk)=NextBNNum++; return(yylval.integ=AND); }

join		{ return(yylval.integ=JOIN); }
when		{ return(yylval.integ=WHEN); }
meeting		{ EchoFunc=NullFunc; MDecl=1; return(yylval.integ=0,MEETING); }

if			{ return(yylval.integ=IF); }
else		{ return(yylval.integ=ELSE); }
goto		{ return(yylval.integ=GOTO); }
while		{ return(yylval.integ=WHILE); }
do			{ return(yylval.integ=DO); }
for			{ return(yylval.integ=FOR); }
block		{ if(StkFull(BNodeStk)) FatalMemErr("stack overflow in lexan()"); StkPush(BNodeStk,NextBNNum++); return(yylval.integ=BLOCK); }
group		{ EscStatement=1; return(yylval.integ=GROUP); }
continue 	{ return(yylval.integ=CONTINUE); }
break		{ return(yylval.integ=BREAK); }
return		{ return(yylval.integ=RETURN); }

switch		{ if(StkFull(SNodeStk)) FatalMemErr("stack overflow in lexan()"); StkPush(SNodeStk,ZSCCount); yylval.integ=LineNo; return(SWITCH); }
case		{ return(yylval.integ=CASE); }
default		{ return(yylval.integ=DEFAULT); }

sizeof		{ EchoFunc(LexOut,yytext); yylval.offse=GetOffset(LexIn)-7; return(SIZEOF); }

"."".""."	{ EchoFunc(LexOut,yytext); return(yylval.integ=DOTDOTDOT); }
"*""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=STAREQUAL); }
"/""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=SLASHEQUAL); }
"%""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=PERCENTEQUAL); }
"+""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=PLUSEQUAL); }
"-""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=MINUSEQUAL); }
"<""<""="	{ EchoFunc(LexOut,yytext); return(yylval.integ=LOWLOWEQUAL); }
">"">""="	{ EchoFunc(LexOut,yytext); return(yylval.integ=GRTGRTEQUAL); }
"&""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=AMPERSANDEQUAL); }
"^""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=HATEQUAL); }
"|""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=PIPEEQUAL); }
"|""|"		{ EchoFunc(LexOut,yytext); return(yylval.integ=PIPEPIPE); }
"&""&"		{ EchoFunc(LexOut,yytext); return(yylval.integ=AMPERSANDAMPERSAND); }
"=""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=EQUALEQUAL); }
"!""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=EMARKEQUAL); }
"<""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=LOWEQUAL); }
">""="		{ EchoFunc(LexOut,yytext); return(yylval.integ=GRTEQUAL); }
"<""<"		{ EchoFunc(LexOut,yytext); return(yylval.integ=LOWLOW); }
">"">"		{ EchoFunc(LexOut,yytext); return(yylval.integ=GRTGRT); }
"+""+"		{ EchoFunc(LexOut,yytext); yylval.offse=GetOffset(LexIn)-2; return(PLUSPLUS); }
"-""-"		{ EchoFunc(LexOut,yytext); yylval.offse=GetOffset(LexIn)-2; return(MINUSMINUS); }
"-"">"		{ EchoFunc(LexOut,yytext); return(yylval.integ=MINUSGRT); }

";"		{ EchoFunc(LexOut,yytext); return(yylval.integ=SEMICOLON); }
"{"		{ EchoFunc(LexOut,yytext); yylval.exprep=SkipStatement(); return(OBRACE); }
"}"		{ EchoFunc(LexOut,yytext); return(yylval.integ=CBRACE); }
","		{ EchoFunc(LexOut,yytext); return(yylval.integ=COMMA); }
"="		{ EchoFunc(LexOut,yytext); return(yylval.integ=EQUAL); }
":"		{ EchoFunc(LexOut,yytext); return(yylval.integ=COLON); }
"("		{ EchoFunc(LexOut,yytext); yylval.offse=GetOffset(LexIn)-1; return(OBRACKET); }
")"		{ EchoFunc(LexOut,yytext); return(yylval.integ=CBRACKET); }
"["		{ EchoFunc(LexOut,yytext); return(yylval.integ=OSQRBRACKET); }
"]"		{ EchoFunc(LexOut,yytext); return(yylval.integ=CSQRBRACKET); }
"*"		{ EchoFunc(LexOut,yytext); yylval.offse=GetOffset(LexIn)-2; return(STAR); }
"."		{ EchoFunc(LexOut,yytext); return(yylval.integ=DOT); }
"/"		{ EchoFunc(LexOut,yytext); return(yylval.integ=SLASH); }
"%"		{ EchoFunc(LexOut,yytext); return(yylval.integ=PERCENT); }
"+"		{ EchoFunc(LexOut,yytext); yylval.offse=GetOffset(LexIn)-2; return(PLUS); }
"-"		{ EchoFunc(LexOut,yytext); yylval.offse=GetOffset(LexIn)-2; return(MINUS); }
"<"		{ EchoFunc(LexOut,yytext); return(yylval.integ=LOW); }
">"		{ EchoFunc(LexOut,yytext); return(yylval.integ=GRT); }
"&"		{ EchoFunc(LexOut,yytext); yylval.offse=GetOffset(LexIn)-2; return(AMPERSAND); }
"^"		{ EchoFunc(LexOut,yytext); return(yylval.integ=HAT); }
"|"		{ EchoFunc(LexOut,yytext); return(yylval.integ=PIPE); }
"?"		{ EchoFunc(LexOut,yytext); return(yylval.integ=QMARK); }
"!"		{ EchoFunc(LexOut,yytext); yylval.offse=GetOffset(LexIn)-2; return(EMARK); }
"~"		{ EchoFunc(LexOut,yytext); yylval.offse=GetOffset(LexIn)-1; return(TILDE); }

^#" "?[0-9]+" ""\"".*"\"".*$ { /* Un nouveau numero de ligne donne par cpp. */
	int
		t;

	char
		*p,
		*b;

	if(sscanf(yytext+1,"%d",&t)!=1) {
		fprintf(stderr,"%s : bad cpp statement near line %d of cpp result.\n",Me,LineNo);
		LeaveApp(1);
	}

	p=yytext+1;
	while(*p==' ') p++;
	while(isdigit(*p)) p++;
	while(*p==' ') p++;
	if(*p!='"') {
        fprintf(stderr,"%s : bad cpp statement near line %d of cpp result.\n",Me,LineNo);
        LeaveApp(1);
    }
	p++;

	b=CurrentFileName;
	while(*p!='"') *b++=*p++;
	*b=0;

	LineNo=t-1;
	EchoFunc(LexOut,yytext);

}

^#.*$ { /* On ignore les autres directives du preprocesseur C. */

	EchoFunc(LexOut,yytext);

}

{ID} {

TaVal
	tokenvalue;

	EchoFunc(LexOut,yytext);

	tokenvalue.ival=TaLookUp(yytext,&LocalSymSet).ival;
	if(tokenvalue.ival==T_TYPENAME) return(yylval.integ=0,TYPEDEFNAME);

	tokenvalue.ival=TaLookUp(yytext,&GlobalSymSet).ival;
	if(tokenvalue.ival==T_TYPENAME) return(yylval.integ=0,TYPEDEFNAME);

	yylval.tok.off=GetOffset(LexIn)-(Offset)yyleng-1;
	if((yylval.tok.str=(char *)XMalloc(yyleng+1))==NULL) FatalMemErr("malloc() error in lexan()");
	strcpy(yylval.tok.str,yytext);
	return(IDENTIFIER);

}

{D}+{IS} {

	EchoFunc(LexOut,yytext);
	if(MDecl) {
		int c;
		if(sscanf(yytext,"%d",&c)!=1) {
			fprintf(stderr,"%s : bad meeting declaration line %d.\n",Me,LineNo);
			LeaveApp(1);
		}
		else yylval.offse=(Offset)c;
	}
	else yylval.offse=GetOffset(LexIn)-(Offset)yyleng-1;
	return(CONSTANT);

}

{HH}{HD}+ {

	if(MDecl) {
		fprintf(stderr,"%s : bad meeting declaration line %d.\n",Me,LineNo);
		LeaveApp(1);
	}
	EchoFunc(LexOut,yytext);
	yylval.offse=GetOffset(LexIn)-(Offset)yyleng-1;
	return(CONSTANT);

}

{D}*"."{D}+({E})?({FT})?	|
{D}+"."{D}*({E})?({FT})?	|
{D}+{E}({FT})? {

	if(MDecl) {
		fprintf(stderr,"%s : bad meeting declaration line %d.\n",Me,LineNo);
		LeaveApp(1);
	}
	EchoFunc(LexOut,yytext);
	yylval.offse=GetOffset(LexIn)-(Offset)yyleng-1;
	return(CONSTANT);

}

.		{ return(yylval.integ=UNKNOWNTOKEN); }

%%
/*----------------------------------------------------------------------------*/
NullFunc(f,t) FILE *f; char *t; { }
/*----------------------------------------------------------------------------*/
OutputFunc(f,t)
FILE
	*f;

char
	*t;
{
	fprintf(f,"%s",LastToken);
	strcpy(LastToken,t);
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

	strcpy(SCBuf,yytext);
	p=(SCBuf+yyleng);

	FOREVER {

		chr=(char)input();

		if((p-SCBuf)>=(ITMAXLEN-2)) {
			if(msgflg) {
				if(Warn) fprintf(stderr,"%s : warning, string too long line %d, truncated.\n",Me,LineNo);
				msgflg=0;
				*p++=end;
			}
		}
		else if(EchoFunc==OutputFunc) *p++=chr;

		switch(chr) {

		case '\\' :
			chr=(char)input();
			if((p-SCBuf)>=(ITMAXLEN-2)) {
				if(msgflg) {
					if(Warn) fprintf(stderr,"%s : warning, string too long line %d, truncated.\n",Me,LineNo);
					msgflg=0;
					*p++=end;
				}
			}
			else if(EchoFunc==OutputFunc) *p++=chr;
			break;

		default :
			if(chr==end) {
				*p='\0';
				if(EchoFunc==OutputFunc) fprintf(LexOut,"%s",LastToken),strcpy(LastToken,SCBuf);
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
static ExprPt SkipStatement()
{
char
	chr;

int
	level=1;

static Expr
	ES;

	if(EscStatement==0) return(NULL);

	ES.beg=GetOffset(LexIn);
	ES.size=0;

	do {

		if((chr=(char)input())==(char)0) return(&ES);
		ES.size++;
		if(chr=='{') level++;
		else if(chr=='}') level--;

	} while(level);

	unput(chr);
	ES.size--;
	EscStatement=0;
	return(&ES);
}
/*----------------------------------------------------------------------------*/
