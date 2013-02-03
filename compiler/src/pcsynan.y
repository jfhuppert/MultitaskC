%{
/*----------------------------------------------------------------------------*/

/*
	pcsynan.y

	Cette grammaire est presque exactement celle du langage C avec les extentions
	de MultitaskC en plus.

*/

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
/*----------------------------------------------------------------------------*/

#include <stdio.h>
#include <general.h>
#include <symset.h>
#include <structs.h>
#include <strings.h>

extern int
	PcLineNo;

extern void
	CDSize(),
	CDSSize(),
	PcFEnd(),
	RenameId(),
	FIdDecl(),
	MIdDecl(),
	SIdDecl(),
	IdDecl(),
	TIdUsed(),
	IdUsed(),
	EId(),
	InitDecl(),
	XFree(),
	SUId(),
	NSS(),
	KSS();

extern char
	*Me,
	*XMalloc(),
	CurrentFileName[];

extern int
#ifdef DEBUG
	DebugInfo,
#endif
	NextBNNum;

extern FILE
	*PcResult;

int
	SwitchFlag;

StkDecl(SymSet,BNSTKSIZE)
	SSStk;

/*----------------------------------------------------------------------------*/
%}

%union {

	int
		integ;

	char
		*stri;

	Ident
		ident;

	Offset
		offse;

	struct { int t; Offset o; int s; }
		dspec;

	struct { int f; Offset o; int s; }
		initi;

}

%token <offse> AUTO REGISTER STATIC EXTERN TYPEDEF
%token <offse> VOID CHAR SHORT INT LONG FLOAT DOUBLE SIGNED UNSIGNED
%token <offse> STRUCT UNION ENUM
%token <offse> CONST VOLATILE

%token <integ> MEETING

%token <ident> TYPEDEFNAME

%token <integ> EXECUTE AND GROUP
%token <integ> JOIN WHEN MODULE
%token <integ> IF ELSE GOTO
%token <integ> SWITCH CASE DEFAULT
%token <integ> WHILE DO FOR BLOCK
%token <integ> CONTINUE BREAK RETURN

%token <ident> IDENTIFIER 
%token <integ> L_MIDENT G_MIDENT 
%token <integ> U_IDENT L_IDENT G_IDENT
%token <integ> L_FIDENT G_FIDENT U_FIDENT
%token <integ> L_SIDENT G_SIDENT
%token <integ> L_UIDENT G_UIDENT
%token <integ> L_EIDENT G_EIDENT

%token <offse> SIZEOF PLUSPLUS MINUSMINUS CONSTANT
%token <offse> STAR PLUS MINUS AMPERSAND EMARK TILDE OBRACKET

%token <integ> DOTDOTDOT STAREQUAL SLASHEQUAL PERCENTEQUAL PLUSEQUAL
%token <integ> LOWLOWEQUAL GRTGRTEQUAL AMPERSANDEQUAL HATEQUAL PIPEEQUAL
%token <integ> AMPERSANDAMPERSAND EQUALEQUAL EMARKEQUAL LOWEQUAL GRTEQUAL
%token <integ> GRTGRT MINUSGRT LOW GRT HAT PIPE QMARK LOWLOW PIPEPIPE
%token <integ> SEMICOLON COMMA EQUAL COLON CBRACKET
%token <integ> OSQRBRACKET CSQRBRACKET DOT SLASH PERCENT MINUSEQUAL
%token <offse> OBRACE CBRACE

%token <integ> UNKNOWNTOKEN

%token <integ> F_NSWITCH F_DSWITCH

%type <integ> translation_unit
%type <integ> external_declaration
%type <integ> function_definition
%type <integ> declaration
%type <integ> parameter_declaration_list
%type <integ> declaration_list
%type <integ> meeting_declaration
%type <integ> meeting_list
%type <dspec> declaration_specifiers
%type <dspec> storage_class_specifier
%type <dspec> type_specifier
%type <dspec> type_qualifier
%type <dspec> struct_or_union_specifier
%type <dspec> struct_or_union
%type <integ> struct_declaration_list
%type <integ> init_declarator_list
%type <integ> init_declarator
%type <integ> struct_declaration
%type <integ> specifier_qualifier_list
%type <integ> struct_declarator_list
%type <integ> struct_declarator
%type <dspec> enum_specifier
%type <integ> enumerator_list
%type <integ> enumerator
%type <ident> declarator
%type <ident> direct_declarator
%type <offse> pointer
%type <integ> type_qualifier_list
%type <integ> parameter_type_list
%type <integ> parameter_list
%type <integ> parameter_declaration
%type <integ> identifier_list
%type <initi> initializer
%type <integ> initializer_list
%type <integ> type_name
%type <integ> abstract_declarator
%type <integ> direct_abstract_declarator
%type <integ> statement
%type <integ> one_label
%type <integ> sync_statement
%type <integ> labeled_statement
%type <integ> compound_statement
%type <integ> concurrent_statement
%type <integ> selection_statement
%type <integ> iteration_statement
%type <integ> jump_statement
%type <integ> expression_statement
%type <integ> statement_list
%type <integ> expression
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
%type <integ> argument_expression_list

%start translation_unit

%%

translation_unit :
	external_declaration
	| translation_unit external_declaration
	;

external_declaration :
	function_definition { NextBNNum=1; }
	| declaration
	;

function_definition :
	declarator { SwitchFlag=F_DSWITCH; FIdDecl(&$1,0); NSS(SS_NREN); }
		compound_statement { PcFEnd(); $$=0; }
	| declaration_specifiers declarator { SwitchFlag=F_DSWITCH; FIdDecl(&$2,$1.t); NSS(SS_NREN); }
		compound_statement { PcFEnd(); $$=0; }
	| declarator { FIdDecl(&$1,0); NSS(SS_NREN); } parameter_declaration_list { SwitchFlag=F_DSWITCH; NSS(SS_NREN); }
		compound_statement { KSS(); PcFEnd(); $$=0; }
	| declaration_specifiers declarator { FIdDecl(&$2,$1.t); NSS(SS_NREN); } parameter_declaration_list { SwitchFlag=F_DSWITCH; NSS(SS_NREN); }
		compound_statement { KSS(); PcFEnd(); $$=0; }
	;

declaration :
	declaration_specifiers SEMICOLON { $$=0; }
	| declaration_specifiers init_declarator_list SEMICOLON { $$=0; }
	;

meeting_declaration :
	MEETING meeting_list SEMICOLON
	;

meeting_list :
	IDENTIFIER COLON CONSTANT { MIdDecl(&$1); $$=0; }
	| meeting_list COMMA IDENTIFIER COLON CONSTANT { MIdDecl(&$3); }
	;

parameter_declaration_list :
	declaration
	| parameter_declaration_list declaration
	;

declaration_list :
	{ SwitchFlag=F_NSWITCH; } declaration { $$=0; }
	| { SwitchFlag=F_NSWITCH; } meeting_declaration { $$=0; }
	| declaration_list meeting_declaration
	| declaration_list declaration
	;

declaration_specifiers :
	storage_class_specifier
	| storage_class_specifier declaration_specifiers { $$=$1; $$.t=$1.t|$2.t; }
	| type_specifier
	| type_specifier declaration_specifiers { $$=$1; $$.t=$1.t|$2.t; }
	| type_qualifier
	| type_qualifier declaration_specifiers { $$=$1; $$.t=$1.t|$2.t; }
	;

storage_class_specifier :
	AUTO { $$.t=0; $$.o=$1; $$.s=0; }
	| REGISTER { $$.t=0; $$.o=$1; $$.s=0; }
	| STATIC { $$.t=SS_STA; $$.o=$1; $$.s=0; }
	| EXTERN { $$.t=SS_EXT; $$.o=$1; $$.s=0; }
	| TYPEDEF { $$.t=SS_TYP; $$.o=$1; $$.s=0; }
	;

type_specifier :
	VOID { $$.t=0; $$.o=$1; $$.s=0; }
	| CHAR { $$.t=0; $$.o=$1; $$.s=0; }
	| SHORT { $$.t=0; $$.o=$1; $$.s=0; }
	| INT { $$.t=0; $$.o=$1; $$.s=0; }
	| LONG { $$.t=0; $$.o=$1; $$.s=0; }
	| FLOAT { $$.t=0; $$.o=$1; $$.s=0; }
	| DOUBLE { $$.t=0; $$.o=$1; $$.s=0; }
	| SIGNED { $$.t=0; $$.o=$1; $$.s=0; }
	| UNSIGNED { $$.t=0; $$.o=$1; $$.s=0; }
	| struct_or_union_specifier
	| enum_specifier
	| TYPEDEFNAME { TIdUsed(&$1); $$.t=0; $$.o=$1.o; $$.s=0; }
	;

type_qualifier :
	CONST { $$.t=0; $$.o=$1; $$.s=0; }
	| VOLATILE { $$.t=0; $$.o=$1; $$.s=0; }
	;

struct_or_union_specifier :
	struct_or_union { NSS(SS_NREN); } OBRACE struct_declaration_list CBRACE { KSS(); $$=$1; $$.t=0; }
	| struct_or_union IDENTIFIER { SUId(&$2,$1.t,0); $$=$1; $$.t=0; }
	| struct_or_union IDENTIFIER { SUId(&$2,$1.t,1); NSS(SS_NREN); } OBRACE struct_declaration_list CBRACE { KSS(); $$=$1; $$.t=0; }
	| struct_or_union TYPEDEFNAME { SUId(&$2,$1.t,0); $$=$1; $$.t=0; }
	| struct_or_union TYPEDEFNAME { SUId(&$2,$1.t,1); NSS(SS_NREN); } OBRACE struct_declaration_list CBRACE { KSS(); $$=$1; $$.t=0; }
	;

struct_or_union :
	STRUCT { $$.t=0; $$.o=$1; $$.s=0; }
	| UNION { $$.t=1; $$.o=$1; $$.s=0; }
	;

struct_declaration_list :
	struct_declaration
	| struct_declaration_list struct_declaration
	;

init_declarator_list :
	init_declarator
	| init_declarator_list COMMA { $<dspec>$=$<dspec>0; } init_declarator
	;

init_declarator :
	declarator { CDSSize(&$<dspec>0.s,$1.o,$<dspec>0.o); IdDecl(&$1,$<dspec>0.t,1); $$=0; }
	| declarator { CDSSize(&$<dspec>0.s,$1.o,$<dspec>0.o); IdDecl(&$1,$<dspec>0.t,0); CDSize(&$<integ>$,$1.o); }
		EQUAL initializer { InitDecl($<dspec>0.o,$<dspec>0.s,$<dspec>0.t,&$1,$<integ>2,$4.o,$4.s,$4.f); $$=0; }
	;

struct_declaration :
	specifier_qualifier_list SEMICOLON
	| specifier_qualifier_list struct_declarator_list SEMICOLON
	;

specifier_qualifier_list :
	type_specifier { $$=0; }
	| type_specifier specifier_qualifier_list { $$=0; }
	| type_qualifier { $$=0; }
	| type_qualifier specifier_qualifier_list { $$=0; }
	;

struct_declarator_list :
	struct_declarator
	| struct_declarator_list COMMA struct_declarator
	;

struct_declarator :
	declarator { SIdDecl(&$1); $$=0; }
	| COLON constant_expression
	| declarator { SIdDecl(&$1); } COLON constant_expression { $$=0; }
	;

enum_specifier :
	ENUM OBRACE enumerator_list CBRACE { $$.t=0; $$.o=$1; $$.s=0; }
	| ENUM IDENTIFIER { EId(&$2,0); $$.t=0; $$.o=$1; $$.s=0; }
	| ENUM IDENTIFIER { EId(&$2,1); } OBRACE enumerator_list CBRACE { $$.t=0; $$.o=$1; $$.s=0; }
	| ENUM TYPEDEFNAME { EId(&$2,0); $$.t=0; $$.o=$1; $$.s=0; }
	| ENUM TYPEDEFNAME { EId(&$2,1); } OBRACE enumerator_list CBRACE { $$.t=0; $$.o=$1; $$.s=0; }
	;

enumerator_list :
	enumerator
	| enumerator_list COMMA enumerator
	;

enumerator :
	IDENTIFIER { IdDecl(&$1,0,1); $$=0; }
	| IDENTIFIER { IdDecl(&$1,0,1); } EQUAL constant_expression { $$=0; }
	;

declarator :
	direct_declarator
	| pointer direct_declarator { $$=$2; $$.o=$1; $$.t=(-$2.t); }
	;

direct_declarator :
	IDENTIFIER
	| OBRACKET declarator CBRACKET { $$=$2; $$.o=$1; }
	| direct_declarator OSQRBRACKET constant_expression CSQRBRACKET { $$=$1; SS_SF($$.f,SS_ARR); }
	| direct_declarator OSQRBRACKET CSQRBRACKET { $$=$1; SS_SF($$.f,SS_ARR); }
	| direct_declarator OBRACKET parameter_type_list CBRACKET
		{
		$$=$1;
		switch($1.t) { case U_IDENT: case G_IDENT: case G_FIDENT: case L_FIDENT: $$.t=U_FIDENT; $$.bn=SS_NREN; }
		}
	| direct_declarator OBRACKET CBRACKET
		{
		$$=$1;
		switch($1.t) { case U_IDENT: case G_IDENT: case G_FIDENT: case L_FIDENT: $$.t=U_FIDENT; $$.bn=SS_NREN; }
		}
	| direct_declarator OBRACKET identifier_list CBRACKET
		{
		$$=$1;
		switch($1.t) { case U_IDENT: case G_IDENT: case G_FIDENT: case L_FIDENT: $$.t=U_FIDENT; $$.bn=SS_NREN; }
		}
	;

pointer :
	STAR
	| STAR type_qualifier_list
	| STAR pointer
	| STAR type_qualifier_list pointer
	;

type_qualifier_list :
	type_qualifier { $$=0; }
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
	declaration_specifiers { $$=0; }
	| declaration_specifiers declarator { $2.bn=SS_NREN; RenameId(&$2); $$=0; }
	| declaration_specifiers abstract_declarator { $$=0; }
	;

identifier_list :
	IDENTIFIER { $1.bn=SS_NREN; RenameId(&$1); $$=0; }
	| identifier_list COMMA IDENTIFIER { $3.bn=SS_NREN; RenameId(&$3); $$=0; }
	;

initializer :
	assignment_expression { $$.o=$1; $$.s=(int)(GetOffset(PcResult)-$1); $$.f=0; }
	| OBRACE initializer_list CBRACE { $$.o=$1; $$.s=(int)($3-$1); $$.f=1; }
	| OBRACE initializer_list COMMA CBRACE { $$.o=$1; $$.s=(int)($4-$1); $$.f=1; }
	;

initializer_list :
	initializer { $$=0; }
	| initializer_list COMMA initializer
	;

type_name :
	specifier_qualifier_list
	| specifier_qualifier_list abstract_declarator
	;

abstract_declarator :
	pointer { $$=0; }
	| direct_abstract_declarator
	| pointer direct_abstract_declarator { $$=0; }
	;

direct_abstract_declarator :
	OBRACKET CBRACKET { $$=0; }
	| OBRACKET abstract_declarator CBRACKET { $$=0; }
	| OBRACKET parameter_type_list CBRACKET { $$=0; }
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
	| { NSS(NextBNNum++); } compound_statement { $$=0; }
	| selection_statement
	| iteration_statement
	| jump_statement
	| concurrent_statement
	;

sync_statement :
	JOIN OBRACKET IDENTIFIER CBRACKET SEMICOLON
	| WHEN OBRACKET IDENTIFIER CBRACKET statement
	| WHEN OBRACKET IDENTIFIER CBRACKET statement ELSE statement
	| WHEN OBRACKET EMARK IDENTIFIER CBRACKET statement
	| WHEN OBRACKET EMARK IDENTIFIER CBRACKET statement ELSE statement
	;

one_label :
	IDENTIFIER COLON { $1.bn=SS_NREN; RenameId(&$1); $$=0; }
	;

labeled_statement :
	one_label statement
	| CASE expression COLON statement
	| DEFAULT COLON statement
	| BLOCK OBRACKET IDENTIFIER { $3.bn=SS_NREN; RenameId(&$3); } CBRACKET statement
	;

compound_statement :
	OBRACE CBRACE { StkPop(SSStk); $$=0; }
	| OBRACE statement_list CBRACE { KSS(); $$=0; }
	| OBRACE declaration_list { SwitchFlag=F_NSWITCH; } CBRACE { KSS(); $$=0; }
	| OBRACE declaration_list { SwitchFlag=F_NSWITCH; } statement_list CBRACE { KSS(); $$=0; }
	;

concurrent_statement :
	EXECUTE statement AND statement
	;

selection_statement :
	IF OBRACKET expression CBRACKET statement
	| IF OBRACKET expression CBRACKET statement ELSE statement
	| SWITCH OBRACKET expression CBRACKET statement
	;

iteration_statement :
	WHILE OBRACKET expression CBRACKET statement
	| DO statement WHILE OBRACKET expression CBRACKET SEMICOLON
	| FOR OBRACKET SEMICOLON SEMICOLON CBRACKET statement
	| FOR OBRACKET SEMICOLON SEMICOLON expression CBRACKET statement
	| FOR OBRACKET SEMICOLON expression SEMICOLON CBRACKET statement
	| FOR OBRACKET expression SEMICOLON SEMICOLON CBRACKET statement
	| FOR OBRACKET SEMICOLON expression SEMICOLON expression CBRACKET statement
	| FOR OBRACKET expression SEMICOLON SEMICOLON expression CBRACKET statement
	| FOR OBRACKET expression SEMICOLON expression SEMICOLON CBRACKET statement
	| FOR OBRACKET expression SEMICOLON expression SEMICOLON expression CBRACKET statement
	;

jump_statement :
	CONTINUE SEMICOLON
	| BREAK SEMICOLON
	| BREAK OBRACKET IDENTIFIER CBRACKET SEMICOLON { $3.bn=SS_NREN; RenameId(&$3); }
	| RETURN SEMICOLON
	| RETURN expression SEMICOLON
	| GOTO IDENTIFIER SEMICOLON { $2.bn=SS_NREN; RenameId(&$2); }
	;

expression_statement :
	SEMICOLON
	| expression SEMICOLON
	| GROUP { NSS(NextBNNum++); } compound_statement
	;

statement_list :
	statement
	| statement_list statement
	;

expression :
	assignment_expression { $$=0; }
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
	conditional_expression { $$=0; }
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
	| equality_expression EQUALEQUAL relational_expression
	| equality_expression EMARKEQUAL relational_expression
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
	| OBRACKET { NSS(SS_NREN); } type_name { KSS(); } CBRACKET cast_expression
	;

unary_expression :
	postfix_expression
	| PLUSPLUS unary_expression
	| MINUSMINUS unary_expression
	| unary_operator cast_expression
	| SIZEOF unary_expression
	| SIZEOF OBRACKET { NSS(SS_NREN); } type_name { KSS(); } CBRACKET
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
	| postfix_expression DOT IDENTIFIER { $3.bn=SS_NREN; RenameId(&$3); }
	| postfix_expression MINUSGRT IDENTIFIER { $3.bn=SS_NREN; RenameId(&$3); }
	| postfix_expression PLUSPLUS
	| postfix_expression MINUSMINUS
	;

primary_expression :
	IDENTIFIER { IdUsed(&$1); $$=$1.o; }
	| CONSTANT
	| OBRACKET expression CBRACKET
	;

argument_expression_list :
	assignment_expression { $$=0; }
	| argument_expression_list COMMA assignment_expression
	;

%%
/*-------------------------------------------------------------------------*/
pcyyerror(str)
char
	*str;
{
	fprintf(
		stderr,
		"%s : pre-compiler : %s line %d of \"%s\", token <%d>.\n",
		Me,
		str,
		PcLineNo,
		CurrentFileName,
		pcyychar
	);

	LeaveApp(1);
}
/*------------------------------------------------------------------------*/
