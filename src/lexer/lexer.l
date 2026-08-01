%{
/* ============================================================
 * lexer.l -- Lexical analyzer for the mini-language.
 *
 * Recognises keywords, identifiers, integer/float literals,
 * booleans, operators, punctuation, comments (both line "//"
 * and block comment forms) and reports lexical errors on any character
 * that matches none of the rules below, WITHOUT stopping the
 * scanner (so multiple lexical errors can be reported in one
 * pass, same philosophy as Lab 2 Sec 2.3).
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ast/ast.h"
#include "../parser/parser.tab.h"

int lexical_error_count = 0;
extern int yylineno;

%}

%option noyywrap
%option yylineno
%option noinput
%option nounput

DIGIT     [0-9]
LETTER    [a-zA-Z_]
ID        {LETTER}({LETTER}|{DIGIT})*
INT       {DIGIT}+
FLOAT     {DIGIT}+"."{DIGIT}+
WS        [ \t\r]+

%x BLOCKCOMMENT

%%

"/*"                     { BEGIN(BLOCKCOMMENT); }
<BLOCKCOMMENT>"*/"        { BEGIN(INITIAL); }
<BLOCKCOMMENT>\n          { /* still counted via yylineno */ }
<BLOCKCOMMENT>.           { /* consume comment body */ }
<BLOCKCOMMENT><<EOF>>     {
                            fprintf(stderr, "Lexical Error (line %d): unterminated block comment\n", yylineno);
                            lexical_error_count++;
                            yyterminate();
                          }

"//".*                   { /* single-line comment: ignored */ }

"int"                     { return T_INT; }
"float"                   { return T_FLOAT; }
"bool"                    { return T_BOOL; }
"if"                      { return T_IF; }
"else"                    { return T_ELSE; }
"while"                   { return T_WHILE; }
"print"                   { return T_PRINT; }
"true"                    { yylval.ival = 1; return T_TRUE; }
"false"                   { yylval.ival = 0; return T_FALSE; }

{FLOAT}                   { yylval.dval = atof(yytext); return FLOAT_LIT; }
{INT}                     { yylval.ival = atol(yytext); return INT_LIT; }
{ID}                      { yylval.sval = strdup(yytext); return ID; }

"<="                      { return LE; }
">="                      { return GE; }
"=="                      { return EQ; }
"!="                      { return NE; }
"&&"                      { return AND; }
"||"                      { return OR; }

"+"                       { return '+'; }
"-"                       { return '-'; }
"*"                       { return '*'; }
"/"                       { return '/'; }
"%"                       { return '%'; }
"<"                       { return '<'; }
">"                       { return '>'; }
"!"                       { return '!'; }
"="                       { return '='; }
";"                       { return ';'; }
"{"                       { return '{'; }
"}"                       { return '}'; }
"("                       { return '('; }
")"                       { return ')'; }

{WS}                      { /* skip whitespace */ }
\n                        { /* newline: yylineno auto-increments; no token needed */ }

.                          {
                            fprintf(stderr, "Lexical Error (line %d): unexpected character '%s'\n", yylineno, yytext);
                            lexical_error_count++;
                            /* do not return a token: keep scanning so the
                               parser can still find further real tokens
                               and later lexical errors can be reported. */
                          }

%%
