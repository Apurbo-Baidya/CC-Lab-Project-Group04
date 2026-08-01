#ifndef CODEGEN_H
#define CODEGEN_H

#include "../ast/ast.h"

/* Generates Three-Address Code (as a quadruple table, per Lab 4 Sec 2.3)
 * for the whole AST, including control flow (if/while) using labels and
 * conditional jumps (Lab 4 Sec 2.5). Applies constant folding while
 * building the table, then a dead-code elimination pass, then prints
 * both the unoptimized and optimized tables (Lab 4 Sec 2.6). */
void generate_and_print_tac(Node *program);

#endif
