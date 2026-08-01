#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "../ast/ast.h"

/* ============================================================
 * Symbol table with nested (block) scopes.
 *
 * Implemented as a stack of scopes. Each scope is a simple
 * linked list of symbols (adequate for the small test programs
 * used in this course; a real compiler would use a hash table).
 *
 * push_scope()/pop_scope() are called on block entry/exit
 * (function bodies, if/while bodies, and the global program
 * block), which is what gives the language its nested-scope
 * semantics: a variable declared inside a block is invisible
 * once that block's scope is popped.
 * ============================================================ */

typedef struct Symbol {
    char *name;
    DataType type;
    int scope_level;
    int line_declared;
    struct Symbol *next;
} Symbol;

typedef struct Scope {
    Symbol *symbols;
    struct Scope *parent;
    int level;
} Scope;

void symtab_init(void);
void symtab_push_scope(void);
void symtab_pop_scope(void);

/* Returns 1 on success, 0 if 'name' is already declared in the
 * CURRENT (innermost) scope -- i.e. a redeclaration error.     */
int symtab_insert(const char *name, DataType type, int line);

/* Searches the current scope and all enclosing scopes.
 * Returns NULL if not found anywhere (undeclared variable).    */
Symbol *symtab_lookup(const char *name);

/* Searches ONLY the innermost/current scope (used to detect
 * redeclaration before inserting).                              */
Symbol *symtab_lookup_current_scope(const char *name);

int symtab_current_level(void);

void symtab_print_all(void); /* debugging / documentation aid */
void symtab_destroy(void);

#endif
