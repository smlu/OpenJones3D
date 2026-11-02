D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+

%{
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCogParse.h>
#include <sith/Cog/sithCogYacc.h>

#include <std/General/stdUtil.h>

#include <stdio.h>
#include <stdlib.h>

static YY_CHAR aLexerCurSymText[256] = {0};

size_t yylinenum=1;
%}

%%

"break"			{ return(BREAK); }
"continue"		{ return(CONTINUE); }
"do"			{ return(DO); }
"else"			{ return(ELSE); }
"for"			{ return(FOR); }
"goto"			{ return(GOTO); }
"if"			{ return(IF); }
"stop"		    { return(RETURN); }
"return"		{ return(RETURN); }
"while"			{ return(WHILE); }
"end"			{ yyterminate(); }
"call"			{ return(CALL); }

{L}({L}|{D})*	{ sithCogParse_LexerSetSymbol(yytext); STD_STRCPY(aLexerCurSymText, yytext); return(IDENTIFIER); }

0[xX]{H}+		{ sscanf_s(yytext, "%x", &yylval.intValue); return(CONSTANT_INT); }
0{D}+		    { sscanf_s(yytext, "%i", &yylval.intValue); return(CONSTANT_INT); }
{D}+		    { yylval.intValue = atoi(yytext); return(CONSTANT_INT); }

{D}+{E}		        { yylval.floatValue = atof(yytext); return(CONSTANT_FLOAT); }
{D}*"."{D}+({E})?	{ yylval.floatValue = atof(yytext); return(CONSTANT_FLOAT); }
{D}+"."{D}*({E})?	{ yylval.floatValue = atof(yytext); return(CONSTANT_FLOAT); }

\"(\\.|[^\\"])*\"	{ sithCogParse_LexerSetString(yytext); return(STRING_LITERAL); }
\'(\\.|[^\\'])*\'	{ sithCogParse_LexerSetVector(yytext); return(VECTOR_LITERAL); }

"&&"			{ return(AND_OP); }
"||"			{ return(OR_OP); }
"<="			{ return(LE_OP); }
">="			{ return(GE_OP); }
"=="			{ return(EQ_OP); }
"!="			{ return(NE_OP); }
";"			{ return(';'); }
"{"			{ return('{'); }
"}"			{ return('}'); }
","			{ return(','); }
":"			{ return(':'); }
"="			{ return('='); }
"("			{ return('('); }
")"			{ return(')'); }
"["			{ return('['); }
"]"			{ return(']'); }
"."			{ return('.'); }
"&"			{ return('&'); }
"!"			{ return('!'); }
"~"			{ return('~'); }
"-"			{ return('-'); }
"+"			{ return('+'); }
"*"			{ return('*'); }
\/\/(.*)	{ /* comment */ }
#(.*)		{ /* comment */ }
"/"			{ return('/'); }
"%"			{ return('%'); }
"<"			{ return('<'); }
">"			{ return('>'); }
"^"			{ return('^'); }
"|"			{ return('|'); }

[\r\n]		{ yylinenum++; }
[ \t\v\f]	{ }
.			{ /* ignore bad characters */ }

%%
