%{
/* ============================================================
 * parser.y -- Grammar for the mini-language.
 *
 * The parser's ONLY job is to build an AST (Node*). It does not
 * evaluate expressions, type-check, or generate code -- those
 * are separate passes (semantic.c, codegen.c) run afterwards by
 * main.c, matching the phase pipeline from Lab 1 Sec 2.2.
 *
 * Error recovery: the 'error' token lets the parser skip to the
 * next ';' (statement error recovery) so one syntax error does
 * not abort the whole parse -- multiple syntax errors can be
 * reported in a single pass, just like GCC.
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include "../ast/ast.h"

int yylex(void);
extern int yylineno;
extern char *yytext;
void yyerror(const char *s);

int syntax_error_count = 0;
Node *ast_root = NULL;
%}

%union {
    long ival;
    double dval;
    char *sval;
    struct Node *node;
}

%token T_INT T_FLOAT T_BOOL T_IF T_ELSE T_WHILE T_PRINT
%token <ival> T_TRUE T_FALSE INT_LIT
%token <dval> FLOAT_LIT
%token <sval> ID
%token LE GE EQ NE AND OR

%type <node> program stmt_list stmt block decl assign if_stmt while_stmt print_stmt expr
%type <ival> type_spec

%left OR
%left AND
%nonassoc EQ NE
%nonassoc '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right UMINUS NOT

%%

program:
    stmt_list { ast_root = $1; }
    ;

stmt_list:
    /* empty */                { $$ = new_program(); }
    | stmt_list stmt            { block_add($1, $2); $$ = $1; }
    | stmt_list error ';'       { $$ = $1; /* yyerror() already counted this error; just resume after the ';' */ }
    ;

stmt:
    decl
    | assign
    | if_stmt
    | while_stmt
    | print_stmt
    | block
    ;

type_spec:
    T_INT    { $$ = TYPE_INT; }
    | T_FLOAT { $$ = TYPE_FLOAT; }
    | T_BOOL  { $$ = TYPE_BOOL; }
    ;

decl:
    type_spec ID ';' { $$ = new_var_decl((DataType)$1, $2, yylineno); free($2); }
    ;

assign:
    ID '=' expr ';' { $$ = new_assign($1, $3, yylineno); free($1); }
    ;

if_stmt:
    T_IF '(' expr ')' block                 { $$ = new_if($3, $5, NULL, yylineno); }
    | T_IF '(' expr ')' block T_ELSE block   { $$ = new_if($3, $5, $7, yylineno); }
    ;

while_stmt:
    T_WHILE '(' expr ')' block { $$ = new_while($3, $5, yylineno); }
    ;

print_stmt:
    T_PRINT expr ';' { $$ = new_print($2, yylineno); }
    ;

block:
    '{' stmt_list '}' { $2->kind = NODE_BLOCK; $$ = $2; }
    ;

expr:
      expr '+' expr   { $$ = new_binop("+", $1, $3, yylineno); }
    | expr '-' expr   { $$ = new_binop("-", $1, $3, yylineno); }
    | expr '*' expr   { $$ = new_binop("*", $1, $3, yylineno); }
    | expr '/' expr   { $$ = new_binop("/", $1, $3, yylineno); }
    | expr '%' expr   { $$ = new_binop("%", $1, $3, yylineno); }
    | expr '<' expr   { $$ = new_binop("<", $1, $3, yylineno); }
    | expr '>' expr   { $$ = new_binop(">", $1, $3, yylineno); }
    | expr LE  expr   { $$ = new_binop("<=", $1, $3, yylineno); }
    | expr GE  expr   { $$ = new_binop(">=", $1, $3, yylineno); }
    | expr EQ  expr   { $$ = new_binop("==", $1, $3, yylineno); }
    | expr NE  expr   { $$ = new_binop("!=", $1, $3, yylineno); }
    | expr AND expr   { $$ = new_binop("&&", $1, $3, yylineno); }
    | expr OR  expr   { $$ = new_binop("||", $1, $3, yylineno); }
    | '!' expr %prec NOT    { $$ = new_unop("!", $2, yylineno); }
    | '-' expr %prec UMINUS { $$ = new_unop("uminus", $2, yylineno); }
    | '(' expr ')'    { $$ = $2; }
    | ID              { $$ = new_ident($1, yylineno); free($1); }
    | INT_LIT         { $$ = new_int_lit($1, yylineno); }
    | FLOAT_LIT       { $$ = new_float_lit($1, yylineno); }
    | T_TRUE          { $$ = new_bool_lit(1, yylineno); }
    | T_FALSE         { $$ = new_bool_lit(0, yylineno); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Syntax Error (line %d): %s near '%s'\n", yylineno, s, yytext);
    syntax_error_count++;
}
