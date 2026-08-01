/* ============================================================
 * main.c -- Compiler driver.
 *
 * Pipeline (mirrors Lab 1 Sec 2.2):
 *   1. Lexing + Parsing (flex + bison)  -> builds AST, or reports
 *      lexical/syntax errors.
 *   2. Semantic Analysis (semantic.c)   -> type-checks the AST
 *      using the symbol table; reports semantic errors.
 *   3. Intermediate Code Generation (codegen.c) -> emits TAC as
 *      a quadruple table, applies constant folding + dead-code
 *      elimination, and prints both tables.
 *
 * Each phase only runs if the previous phase found zero errors,
 * exactly the way a real compiler stops before handing broken
 * input to the next phase.
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include "ast/ast.h"
#include "semantic/semantic.h"
#include "codegen/codegen.h"
#include "symbol_table/symbol_table.h"

extern FILE *yyin;
extern int yyparse(void);
extern int yylex_destroy(void);
extern int syntax_error_count;
extern int lexical_error_count;
extern Node *ast_root;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        fprintf(stderr, "Error: cannot open file '%s'\n", argv[1]);
        return 1;
    }

    printf("=== Compiling '%s' ===\n\n", argv[1]);

    yyparse();
    fclose(yyin);

    if (lexical_error_count > 0 || syntax_error_count > 0) {
        printf("\nCompilation halted: %d lexical error(s), %d syntax error(s).\n",
               lexical_error_count, syntax_error_count);
        yylex_destroy();
        return 1;
    }

    printf("Lexing and parsing succeeded. Abstract Syntax Tree:\n\n");
    print_ast(ast_root, 0);

    int sem_errors = run_semantic_analysis(ast_root);
    if (sem_errors > 0) {
        printf("\nCompilation halted: %d semantic error(s) found.\n", sem_errors);
        free_ast(ast_root);
        yylex_destroy();
        return 1;
    }
    printf("\nSemantic analysis passed with 0 errors.\n");

    generate_and_print_tac(ast_root);

    printf("\n=== Compilation finished successfully ===\n");

    symtab_destroy();
    free_ast(ast_root);
    yylex_destroy();
    return 0;
}
