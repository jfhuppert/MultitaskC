%{
/*----------------------------------------------------------------------------*/

/*
	synan.y

Dans cette grammaire, les declarations ne sont pas permises
dans les blocks d'une fonction.

*/

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
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <structs.h>
#include <general.h>
#include <symset.h>
#include <strings.h>

extern void
	TaInsert(),
	XFree(),
	FatalMemErr(),
	SyncPointInsert(),
	SetOffset(),
	BuildAuto(),
	FTaFree(),
	MkExprNode(),
	MkConcNode(),
	MkSwitchNode(),
	MkCaseNode(),
	MkIf0Node(),
	MkIf1Node(),
	MkWhileNode(),
	MkDoNode(),
	MkFor0Node(),
	MkFor1Node(),
	MkFor2Node(),
	MkFor3Node(),
	MkFor4Node(),
	MkFor5Node(),
	MkFor6Node(),
	MkFor7Node(),
	MkContNode(),
	MkBreakNode(),
	MkBlBrkNode(),
	MkRet0Node(),
	MkRet1Node(),
	MkGotoNode(),
	MkLabelNode(),
	MkBlockNode(),
	MkJoinNode(),
	MkPassNode(),
	MkSeqNode();

extern char
	*Me,
	LastToken[],
	CompSrcFileName[],
	CurrentFileName[];

extern int
	MDecl,
	FreeMemSiz,
	NextMeetingId,
	NextBLKId,
	BlkNameLookUp(),
	StoreGoto(),
	CheckGotos(),
	(*EchoFunc)(),
	NullFunc(),
	OutputFunc(),
#ifdef DEBUG
	DebugInfo,
#endif
	Warn,
	yyleng,
	LineNo;

extern LabelPt
	NewLabel();

extern SyncPPt
	SyncPointFind();

extern FILE
	*LexIn;

extern Offset
	GetOffset();

extern TaState
	BlkSymSet,
	SyncSymSet,
	LabelSymSet,
	LocalSymSet,
	GlobalSymSet;

char
	*CFuncName;

int
	DeclScope,
	SyncCard;

StkDecl(int,BNSTKSIZE)
	BNodeStk;

StkDecl(SCCount,SNSTKSIZE)
	SNodeStk;

#ifdef DEBUG
FILE
	*DSrcFile;
#endif

/*----------------------------------------------------------------------------*/
%}
	
%union {

	char
		*stri;

	int
		integ;

	Offset
		offse;

	struct { Offset off; char *str; }
		tok;

	ExprPt
		exprep;

	Expr
		expre;

	Seq
		seque;

	LabelPt
		label;

}

%token <integ> AUTO REGISTER STATIC EXTERN TYPEDEF
%token <integ> VOID CHAR SHORT INT LONG FLOAT DOUBLE SIGNED UNSIGNED
%token <integ> STRUCT UNION ENUM TYPEDEFNAME
%token <integ> CONST VOLATILE MEETING

%token <integ> EXECUTE AND GROUP
%token <integ> JOIN WHEN
%token <integ> IF ELSE GOTO
%token <integ> SWITCH CASE DEFAULT
%token <integ> WHILE DO FOR BLOCK
%token <integ> CONTINUE BREAK RETURN

%token <tok> IDENTIFIER 
%token <offse> SIZEOF PLUSPLUS MINUSMINUS CONSTANT
%token <offse> STAR PLUS MINUS AMPERSAND EMARK TILDE OBRACKET

%token <integ> DOTDOTDOT STAREQUAL SLASHEQUAL PERCENTEQUAL PLUSEQUAL
%token <integ> LOWLOWEQUAL GRTGRTEQUAL AMPERSANDEQUAL HATEQUAL PIPEEQUAL
%token <integ> AMPERSANDAMPERSAND EQUALEQUAL EMARKEQUAL LOWEQUAL GRTEQUAL
%token <integ> GRTGRT MINUSGRT LOW GRT HAT PIPE QMARK LOWLOW PIPEPIPE
%token <integ> SEMICOLON CBRACE COMMA EQUAL COLON CBRACKET
%token <integ> OSQRBRACKET CSQRBRACKET DOT SLASH PERCENT MINUSEQUAL
%token <exprep> OBRACE

%token <integ> UNKNOWNTOKEN

%type <integ> translation_unit
%type <integ> external_declaration
%type <integ> function_definition
%type <integ> function_body
%type <integ> declaration
%type <integ> declaration_list
%type <integ> fb_declaration_list
%type <integ> meeting_declaration
%type <integ> meeting_list
%type <integ> declaration_specifiers
%type <integ> storage_class_specifier
%type <integ> type_specifier
%type <integ> type_qualifier
%type <integ> struct_or_union_specifier
%type <integ> struct_or_union
%type <integ> struct_declaration_list
%type <integ> init_declarator_list
%type <stri> init_declarator
%type <integ> struct_declaration
%type <integ> specifier_qualifier_list
%type <integ> struct_declarator_list
%type <integ> struct_declarator
%type <integ> enum_specifier
%type <integ> enumerator_list
%type <integ> enumerator
%type <stri> declarator
%type <stri> direct_declarator
%type <integ> pointer
%type <integ> type_qualifier_list
%type <integ> parameter_type_list
%type <integ> parameter_list
%type <integ> parameter_declaration
%type <integ> identifier_list
%type <integ> initializer
%type <integ> initializer_list
%type <integ> type_name
%type <integ> abstract_declarator
%type <integ> direct_abstract_declarator
%type <seque> statement
%type <label> one_label
%type <seque> sync_statement
%type <seque> labeled_statement
%type <seque> compound_statement
%type <seque> concurrent_statement
%type <seque> selection_statement
%type <seque> iteration_statement
%type <seque> jump_statement
%type <seque> expression_statement
%type <seque> statement_list
%type <expre> nstatement_expr
%type <offse> expression
%type <offse> assignment_expression
%type <integ> assignment_operator
%type <offse> conditional_expression
%type <integ> constant_expression
%type <offse> logical_OR_expression
%type <offse> logical_AND_expression
%type <offse> inclusive_OR_expression
%type <offse> exclusive_OR_expression
%type <offse> AND_expression
%type <offse> equality_expression
%type <offse> relational_expression
%type <offse> shift_expression
%type <offse> additive_expression
%type <offse> multiplicative_expression
%type <offse> cast_expression
%type <offse> unary_expression
%type <offse> unary_operator
%type <offse> postfix_expression
%type <offse> primary_expression
%type <offse> argument_expression_list

%start translation_unit

%%

translation_unit :
	external_declaration
	| translation_unit external_declaration
	;

external_declaration :
	function_definition
	| declaration
	;

function_definition :
	declarator { CFuncName=$<stri>1; DeclScope=SCOPE_L; } function_body { XFree($<stri>1,strlen($<stri>1)+1); $<integ>$=0; }
	| declaration_specifiers declarator { CFuncName=$<stri>2; DeclScope=SCOPE_L; } function_body { XFree($<stri>2,strlen($<stri>2)+1); $<integ>$=0; }
	| declarator { CFuncName=$<stri>1; DeclScope=SCOPE_L; } declaration_list function_body { XFree($<stri>1,strlen($<stri>1)+1); $<integ>$=0; }
	| declaration_specifiers declarator { CFuncName=$<stri>2; DeclScope=SCOPE_L; } declaration_list function_body { XFree($<stri>2,strlen($<stri>2)+1); $<integ>$=0; }
	;

function_body :
	OBRACE CBRACE
		{
		DeclScope=SCOPE_G;
		FTaFree(&BlkSymSet,0); TaInit(BlkSymSet); NextBLKId=1;
		FTaFree(&SyncSymSet,2); TaInit(SyncSymSet); NextMeetingId=0;
		FTaFree(&LocalSymSet,0); TaInit(LocalSymSet);
		FTaFree(&LabelSymSet,1); TaInit(LabelSymSet);
		$<integ>$=0;
		}
	| OBRACE fb_declaration_list CBRACE
		{
		DeclScope=SCOPE_G;
		FTaFree(&BlkSymSet,0); TaInit(BlkSymSet); NextBLKId=1;
		FTaFree(&SyncSymSet,2); TaInit(SyncSymSet); NextMeetingId=0;
		FTaFree(&LocalSymSet,0); TaInit(LocalSymSet);
		FTaFree(&LabelSymSet,1); TaInit(LabelSymSet);
		$<integ>$=0;
		}
	| OBRACE
		{
		EchoFunc=NullFunc;
		}
		statement_list CBRACE
		{
		FTaFree(&LocalSymSet,0); TaInit(LocalSymSet);
		CheckGotos();
		FTaFree(&LabelSymSet,1); TaInit(LabelSymSet);
		BuildAuto(&$<seque>3,CompSrcFileName);
		FTaFree(&BlkSymSet,0); TaInit(BlkSymSet); NextBLKId=1;
		FTaFree(&SyncSymSet,2); TaInit(SyncSymSet); NextMeetingId=0;
		EchoFunc=OutputFunc;
		strcpy(LastToken,"");
		DeclScope=SCOPE_G;
		$<integ>$=0;
		}
	| OBRACE fb_declaration_list
		{
		EchoFunc=NullFunc;
		}
		statement_list CBRACE
		{
		FTaFree(&LocalSymSet,0); TaInit(LocalSymSet);
		CheckGotos();
		FTaFree(&LabelSymSet,1); TaInit(LabelSymSet);
		BuildAuto(&$<seque>4,CompSrcFileName);
		FTaFree(&BlkSymSet,0); TaInit(BlkSymSet); NextBLKId=1;
		FTaFree(&SyncSymSet,2); TaInit(SyncSymSet); NextMeetingId=0;
		EchoFunc=OutputFunc;
		strcpy(LastToken,"");
		DeclScope=SCOPE_G;
		$<integ>$=0;
		}
	;

declaration :
	declaration_specifiers SEMICOLON
	| declaration_specifiers init_declarator_list SEMICOLON
	;

fb_declaration_list :
	meeting_declaration
	| fb_declaration_list meeting_declaration
	| declaration
	| fb_declaration_list declaration
	;

meeting_declaration :
	MEETING meeting_list SEMICOLON { EchoFunc=OutputFunc; MDecl=0; }
	;

meeting_list :
	IDENTIFIER COLON CONSTANT
		{
		SyncCard=(int)($<offse>3);
		SyncPointInsert($<tok>1.str,SyncCard);
		XFree($<tok>1.str,strlen($<tok>1.str)+1);
		$<integ>$=0;
		}
	| meeting_list COMMA IDENTIFIER COLON CONSTANT
		{
		SyncCard=(int)($<offse>5);
		SyncPointInsert($<tok>3.str,SyncCard);
		XFree($<tok>3.str,strlen($<tok>3.str)+1);
		$<integ>$=0;
		}
	;

declaration_list :
	declaration
	| declaration_list declaration
	;

declaration_specifiers :
	storage_class_specifier
	| storage_class_specifier declaration_specifiers { $<integ>$=$<integ>1+$<integ>2; }
	| type_specifier
	| type_specifier declaration_specifiers { $<integ>$=$<integ>1+$<integ>2; }
	| type_qualifier
	| type_qualifier declaration_specifiers { $<integ>$=$<integ>1+$<integ>2; }
	;

storage_class_specifier :
	AUTO
	| REGISTER
	| STATIC
	| EXTERN
	| TYPEDEF
	;

type_specifier :
	VOID
	| CHAR
	| SHORT
	| INT
	| LONG
	| FLOAT
	| DOUBLE
	| SIGNED
	| UNSIGNED
	| struct_or_union_specifier
	| enum_specifier
	| TYPEDEFNAME
	;

type_qualifier :
	CONST
	| VOLATILE
	;

struct_or_union_specifier :
	struct_or_union OBRACE struct_declaration_list CBRACE
	| struct_or_union IDENTIFIER { XFree($<tok>2.str,strlen($<tok>2.str)+1); }
	| struct_or_union IDENTIFIER { XFree($<tok>2.str,strlen($<tok>2.str)+1); } OBRACE struct_declaration_list CBRACE
	| struct_or_union TYPEDEFNAME
	| struct_or_union TYPEDEFNAME OBRACE struct_declaration_list CBRACE
	;

struct_or_union :
	STRUCT
	| UNION
	;

struct_declaration_list :
	struct_declaration
	| struct_declaration_list struct_declaration
	;

init_declarator_list :
	init_declarator
		{
		if($<integ>0) {
		int notinserted;
		TaVal tokenvalue;
			tokenvalue.ival=T_TYPENAME;
			TaInsert($<stri>1,DeclScope==SCOPE_L?&LocalSymSet:&GlobalSymSet,&notinserted,&tokenvalue);
		}
		XFree($<stri>1,strlen($<stri>1)+1);
		$<integ>$=0;
		}
	| init_declarator_list COMMA init_declarator
		{
		if($<integ>0) {
		int notinserted;
		TaVal tokenvalue;
			tokenvalue.ival=T_TYPENAME;
			TaInsert($<stri>3,DeclScope==SCOPE_L?&LocalSymSet:&GlobalSymSet,&notinserted,&tokenvalue);
		}
		XFree($<stri>3,strlen($<stri>3)+1);
		$<integ>$=0;
		}
	;

init_declarator :
	declarator
	| declarator EQUAL initializer
	;

struct_declaration :
	specifier_qualifier_list SEMICOLON
	| specifier_qualifier_list struct_declarator_list SEMICOLON
	;

specifier_qualifier_list :
	type_specifier
	| type_specifier specifier_qualifier_list { $<integ>$=$<integ>1+$<integ>2; }
	| type_qualifier
	| type_qualifier specifier_qualifier_list { $<integ>$=$<integ>1+$<integ>2; }
	;

struct_declarator_list :
	struct_declarator
	| struct_declarator_list COMMA struct_declarator
	;

struct_declarator :
	declarator { XFree($<stri>1,strlen($<stri>1)+1); $<integ>$=0; }
	| COLON constant_expression
	| declarator COLON constant_expression { XFree($<stri>1,strlen($<stri>1)+1); $<integ>$=0; }
	;

enum_specifier :
	ENUM OBRACE enumerator_list CBRACE
	| ENUM IDENTIFIER { XFree($<tok>2.str,strlen($<tok>2.str)+1); }
	| ENUM IDENTIFIER { XFree($<tok>2.str,strlen($<tok>2.str)+1); } OBRACE enumerator_list CBRACE
	| ENUM TYPEDEFNAME
	| ENUM TYPEDEFNAME OBRACE enumerator_list CBRACE
	;

enumerator_list :
	enumerator
	| enumerator_list COMMA enumerator
	;

enumerator :
	IDENTIFIER { XFree($<tok>1.str,strlen($<tok>1.str)+1); $<integ>$=0; }
	| IDENTIFIER { XFree($<tok>1.str,strlen($<tok>1.str)+1); } EQUAL constant_expression { $<integ>$=0; }
	;

declarator :
	direct_declarator
	| pointer direct_declarator { $<stri>$=$<stri>2; }
	;

direct_declarator :
	IDENTIFIER { $<stri>$=$<tok>1.str; }
	| OBRACKET declarator CBRACKET { $<stri>$=$<stri>2; }
	| direct_declarator OSQRBRACKET constant_expression CSQRBRACKET
	| direct_declarator OSQRBRACKET CSQRBRACKET
	| direct_declarator OBRACKET parameter_type_list CBRACKET
	| direct_declarator OBRACKET CBRACKET
	| direct_declarator OBRACKET identifier_list CBRACKET
	;

pointer :
	STAR { $<integ>$=0; }
	| STAR type_qualifier_list { $<integ>$=0; }
	| STAR pointer { $<integ>$=0; }
	| STAR type_qualifier_list pointer { $<integ>$=0; }
	;

type_qualifier_list :
	type_qualifier
	| type_qualifier_list type_qualifier
	;

parameter_type_list :
	parameter_list
	| parameter_list COMMA DOTDOTDOT
	;

parameter_list :
	parameter_declaration
	| parameter_list COMMA parameter_declaration
	;

parameter_declaration :
	declaration_specifiers
	| declaration_specifiers declarator { XFree($<stri>2,strlen($<stri>2)+1); }
	| declaration_specifiers abstract_declarator
	;

identifier_list :
	IDENTIFIER { XFree($<tok>1.str,strlen($<tok>1.str)+1); $<integ>$=0; }
	| identifier_list COMMA IDENTIFIER { XFree($<tok>3.str,strlen($<tok>3.str)+1); $<integ>$=0; }
	;

initializer :
	assignment_expression { $<integ>$=0; }
	| OBRACE initializer_list CBRACE { $<integ>$=0; }
	| OBRACE initializer_list COMMA CBRACE { $<integ>$=0; }
	;

initializer_list :
	initializer
	| initializer_list COMMA initializer
	;

type_name :
	specifier_qualifier_list
	| specifier_qualifier_list abstract_declarator
	;

abstract_declarator :
	pointer
	| direct_abstract_declarator
	| pointer direct_abstract_declarator
	;

direct_abstract_declarator :
    OBRACKET CBRACKET { $<integ>$=0; }  
    | OBRACKET abstract_declarator CBRACKET { $<integ>$=0; }
    | OBRACKET parameter_type_list CBRACKET { $<integ>$=0; }
    | direct_abstract_declarator OBRACKET CBRACKET
    | direct_abstract_declarator OBRACKET parameter_type_list CBRACKET
    | OSQRBRACKET CSQRBRACKET
    | OSQRBRACKET constant_expression CSQRBRACKET
    | direct_abstract_declarator OSQRBRACKET CSQRBRACKET
    | direct_abstract_declarator OSQRBRACKET constant_expression CSQRBRACKET
    ;

statement :
	sync_statement
	| labeled_statement
	| expression_statement
	| compound_statement
	| selection_statement
	| iteration_statement
	| jump_statement
	| concurrent_statement
	;

sync_statement :
	JOIN OBRACKET IDENTIFIER CBRACKET SEMICOLON { MkJoinNode(&$$,SyncPointFind($<tok>3.str)); XFree($<tok>3.str,strlen($<tok>3.str)+1); }
	| WHEN OBRACKET IDENTIFIER CBRACKET statement { MkPassNode(&$$,SyncPointFind($<tok>3.str),&$5,NULL); XFree($<tok>3.str,strlen($<tok>3.str)+1); }
	| WHEN OBRACKET IDENTIFIER CBRACKET statement ELSE statement { MkPassNode(&$$,SyncPointFind($<tok>3.str),&$5,&$7); XFree($<tok>3.str,strlen($<tok>3.str)+1); }
	| WHEN OBRACKET EMARK IDENTIFIER CBRACKET statement { MkPassNode(&$$,SyncPointFind($<tok>4.str),NULL,&$6); XFree($<tok>4.str,strlen($<tok>4.str)+1); }
	| WHEN OBRACKET EMARK IDENTIFIER CBRACKET statement ELSE statement { MkPassNode(&$$,SyncPointFind($<tok>4.str),&$8,&$6); XFree($<tok>4.str,strlen($<tok>4.str)+1); }
	;

one_label :
	IDENTIFIER COLON { $<label>$=NewLabel($<tok>1.str,&LabelSymSet,StkTop(BNodeStk)); XFree($<tok>1.str,strlen($<tok>1.str)+1); }
	;

labeled_statement :
	one_label statement { MkLabelNode(&$$,$<label>1,&$2); }
	| CASE nstatement_expr COLON statement
		{
		if(StkEmpty(SNodeStk)) {
			fprintf(stderr,"%s : case without switch() near line %d of \"%s\".\n",Me,LineNo,CurrentFileName);
			LeaveApp(1);
		}
		StkTop(SNodeStk).nc++;
		MkCaseNode(&$$,$<expre>2.beg,$<expre>2.size,&$4);
		}
	| DEFAULT COLON statement
		{
		if(StkEmpty(SNodeStk)) {
			fprintf(stderr,"%s : default without switch() near line %d of \"%s\".\n",Me,LineNo,CurrentFileName);
			LeaveApp(1);
		}
		StkTop(SNodeStk).nd++;
		MkCaseNode(&$$,NULLOFF,0,&$3);
		}
	| BLOCK OBRACKET IDENTIFIER CBRACKET
		{
		$<integ>$=BlkNameLookUp($<tok>3.str);
		XFree($<tok>3.str,strlen($<tok>3.str)+1);
		}
		statement
		{
		StkPop(BNodeStk);
		MkBlockNode(&$$,$<integ>5,&$6);
		}
	;

compound_statement :
	OBRACE CBRACE { $$.first=NULLOFF; $$.last=NULLOFF; }
	| OBRACE statement_list CBRACE { $$=$2; }
	;

concurrent_statement :
	EXECUTE statement AND statement { StkPop(BNodeStk); MkConcNode(&$$,&$2,&$4); }
	;

selection_statement :
	IF OBRACKET nstatement_expr CBRACKET statement { MkIf0Node(&$$,&$3,&$5); }
	| IF OBRACKET nstatement_expr CBRACKET statement ELSE statement { MkIf1Node(&$$,&$3,&$5,&$7); }
	| SWITCH OBRACKET nstatement_expr CBRACKET statement
		{
		if((StkTop(SNodeStk).nc+StkTop(SNodeStk).nd)==0) {
			if(Warn) fprintf(stderr,"%s : warning, switch() without any case or default line %d of \"%s\".\n",Me,$<integ>1,CurrentFileName);
		}
		if(StkTop(SNodeStk).nd>1) {
			fprintf(stderr,"%s : switch() with more than one default line %d of \"%s\".\n",Me,$<integ>1,CurrentFileName);
			LeaveApp(1);
		}
		MkSwitchNode(&$$,&$3,&$5,StkTop(SNodeStk).nc,StkTop(SNodeStk).nd);
		StkPop(SNodeStk);
		}
	;

iteration_statement :
	WHILE OBRACKET nstatement_expr CBRACKET statement { MkWhileNode(&$$,&$3,&$5); }
	| DO statement WHILE OBRACKET nstatement_expr CBRACKET SEMICOLON { MkDoNode(&$$,&$5,&$2); }
	| FOR OBRACKET SEMICOLON SEMICOLON CBRACKET statement { MkFor0Node(&$$,&$6); }
	| FOR OBRACKET SEMICOLON SEMICOLON nstatement_expr CBRACKET statement { MkFor1Node(&$$,&$5,&$7); }
	| FOR OBRACKET SEMICOLON nstatement_expr SEMICOLON CBRACKET statement { MkFor2Node(&$$,&$4,&$7); }
	| FOR OBRACKET nstatement_expr SEMICOLON SEMICOLON CBRACKET statement { MkFor3Node(&$$,&$3,&$7); }
	| FOR OBRACKET SEMICOLON nstatement_expr SEMICOLON nstatement_expr CBRACKET statement { MkFor4Node(&$$,&$4,&$6,&$8); }
	| FOR OBRACKET nstatement_expr SEMICOLON SEMICOLON nstatement_expr CBRACKET statement { MkFor5Node(&$$,&$3,&$6,&$8); }
	| FOR OBRACKET nstatement_expr SEMICOLON nstatement_expr SEMICOLON CBRACKET statement { MkFor6Node(&$$,&$3,&$5,&$8); }
	| FOR OBRACKET nstatement_expr SEMICOLON nstatement_expr SEMICOLON nstatement_expr CBRACKET statement { MkFor7Node(&$$,&$3,&$5,&$7,&$9); }
	;

jump_statement :
	CONTINUE SEMICOLON { MkContNode(&$$); }
	| BREAK SEMICOLON { MkBreakNode(&$$); }
	| BREAK OBRACKET IDENTIFIER CBRACKET SEMICOLON { MkBlBrkNode(&$$,BlkNameLookUp($<tok>3.str)); XFree($<tok>3.str,strlen($<tok>3.str)+1); }
	| RETURN SEMICOLON
		{
#ifdef DEBUG
		if(DebugInfo)
			PrintExpr(NULLOFF,0L,"Return");
#endif
		MkRet0Node(&$$);
		}
	| RETURN nstatement_expr SEMICOLON
		{
#ifdef DEBUG
		if(DebugInfo)
			PrintExpr($<expre>2.beg,(long)$<expre>2.size,"Return");
#endif
		MkRet1Node(&$$,$<expre>2.beg,(int)$<expre>2.size);
		}
	| GOTO IDENTIFIER SEMICOLON { MkGotoNode(&$$); StoreGoto($<tok>2.str,StkTop(BNodeStk),LineNo,$$.first); XFree($<tok>2.str,strlen($<tok>2.str)+1); }
	;

expression_statement :
	SEMICOLON
		{
#ifdef DEBUG
		if(DebugInfo)
			PrintExpr(NULLOFF,0L,"Null");
#endif
		$$.first=NULLOFF;
		$$.last=NULLOFF;
		}
	| expression SEMICOLON
		{
#ifdef DEBUG
		if(DebugInfo)
			PrintExpr($1,(long)(GetOffset(LexIn)-$1-1),"Statement");
#endif
		MkExprNode(&$$,$1,(int)(GetOffset(LexIn)-$1-1));
		}
	| GROUP OBRACE CBRACE
		{
#ifdef DEBUG
		if(DebugInfo)
			PrintExpr($2->beg,(long)$2->size,"Escaped statement");
#endif
		MkExprNode(&$$,$2->beg,$2->size);
		}
	;

statement_list :
	statement
	| statement_list statement { MkSeqNode(&$$,&$1,&$2); }
	;

nstatement_expr :
	expression
		{
#ifdef DEBUG
		if(DebugInfo)
			PrintExpr($<offse>1,(long)(GetOffset(LexIn)-$<offse>1-1),"Expression");
#endif
		$<expre>$.beg=$<offse>1;
		$<expre>$.size=(int)(GetOffset(LexIn)-$<offse>1-1);
		}
	;

expression :
	assignment_expression
	| expression COMMA assignment_expression
	;

assignment_expression :
	conditional_expression
	| unary_expression assignment_operator assignment_expression
	;

assignment_operator :
	EQUAL
	| STAREQUAL
	| SLASHEQUAL
	| PERCENTEQUAL
	| PLUSEQUAL
	| MINUSEQUAL
	| LOWLOWEQUAL
	| GRTGRTEQUAL
	| AMPERSANDEQUAL
	| HATEQUAL
	| PIPEEQUAL
	;

conditional_expression :
	logical_OR_expression
	| logical_OR_expression QMARK expression COLON conditional_expression
	;

constant_expression :
	conditional_expression
		{
		$<integ>$=0;
		}
	;

logical_OR_expression :
	logical_AND_expression
	| logical_OR_expression PIPEPIPE logical_AND_expression
	;

logical_AND_expression :
	inclusive_OR_expression
	| logical_AND_expression AMPERSANDAMPERSAND inclusive_OR_expression
	;

inclusive_OR_expression :
	exclusive_OR_expression
	| inclusive_OR_expression PIPE exclusive_OR_expression
	;

exclusive_OR_expression :
	AND_expression
	| exclusive_OR_expression HAT AND_expression
	;

AND_expression :
	equality_expression
	| AND_expression AMPERSAND equality_expression
	;

equality_expression :
	relational_expression
	| equality_expression EQUALEQUAL  relational_expression
	| equality_expression EMARKEQUAL  relational_expression
	;

relational_expression :
	shift_expression
	| relational_expression LOW shift_expression
	| relational_expression GRT shift_expression
	| relational_expression LOWEQUAL shift_expression
	| relational_expression GRTEQUAL shift_expression
	;

shift_expression :
	additive_expression
	| shift_expression LOWLOW additive_expression
	| shift_expression GRTGRT additive_expression
	;

additive_expression :
	multiplicative_expression
	| additive_expression PLUS multiplicative_expression
	| additive_expression MINUS multiplicative_expression
	;

multiplicative_expression :
	cast_expression
	| multiplicative_expression STAR cast_expression
	| multiplicative_expression SLASH cast_expression
	| multiplicative_expression PERCENT cast_expression
	;

cast_expression :
	unary_expression
	| OBRACKET type_name CBRACKET cast_expression
	;

unary_expression :
	postfix_expression
	| PLUSPLUS unary_expression
	| MINUSMINUS unary_expression
	| unary_operator cast_expression
	| SIZEOF unary_expression
	| SIZEOF OBRACKET type_name CBRACKET
	;

unary_operator :
	AMPERSAND
	| STAR
	| PLUS
	| MINUS
	| TILDE
	| EMARK
	;

postfix_expression :
	primary_expression
	| postfix_expression OSQRBRACKET expression CSQRBRACKET
	| postfix_expression OBRACKET CBRACKET
	| postfix_expression OBRACKET argument_expression_list CBRACKET
	| postfix_expression DOT IDENTIFIER { XFree($<tok>3.str,strlen($<tok>3.str)+1); }
	| postfix_expression MINUSGRT IDENTIFIER { XFree($<tok>3.str,strlen($<tok>3.str)+1); }
	| postfix_expression PLUSPLUS
	| postfix_expression MINUSMINUS
	;

primary_expression :
	IDENTIFIER { XFree($<tok>1.str,strlen($<tok>1.str)+1); $<offse>$=$<tok>1.off; }
	| CONSTANT
	| OBRACKET expression CBRACKET
	;

argument_expression_list :
	assignment_expression
	| argument_expression_list COMMA assignment_expression
	;

%%
/*-------------------------------------------------------------------------*/
yyerror(str)
char
	*str;
{
	fprintf(
		stderr,
		"%s : compiler : %s line %d of \"%s\", token <%d>.\n",
		Me,
		str,
		LineNo,
		CurrentFileName,
		yychar
	);

	LeaveApp(1);
}
/*------------------------------------------------------------------------*/
#ifdef DEBUG
int PrintExpr(beg,size,type)
Offset
	beg;

long
	size;

char
	*type;
{
int
	chr,
	i;

	if(type!=NULL) {
		if(beg==NULLOFF) {
			fprintf(stdout,"%s statement line %d.\n",type,LineNo);
			return;
		}
		fprintf(stdout,"%s at %ld (%ld) : ",type,(long)beg,size);
	}

	SetOffset(DSrcFile,beg,0);
	for(i=0;i<size;i++) {
		switch(chr=getc(DSrcFile)) {

		case '\n' :
			fprintf(stdout,"\\n");
			break;

		case '\t' :
			fprintf(stdout,"\\t");
			break;

		default :
			putc(chr,stdout);
			break;
		}
	}
	if(type!=NULL) putc('\n',stdout);

	return(0);
}
#endif
/*------------------------------------------------------------------------*/
