#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "../ast/ast.h"

/* Walks the AST built by the parser, using the symbol table to:
 *   - register declarations (and catch redeclaration in the same scope)
 *   - check that every identifier used has been declared (and is in scope)
 *   - type-check expressions and assignments
 *   - push/pop a scope for every block (if/while bodies)
 *
 * Returns the number of semantic errors found. main() uses this to
 * decide whether to proceed to TAC generation.
 */
int run_semantic_analysis(Node *program);

#endif
