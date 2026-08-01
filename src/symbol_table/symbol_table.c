#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

static Scope *current = NULL;

void symtab_init(void) {
    current = NULL;
    symtab_push_scope(); /* global scope, level 0 */
}

void symtab_push_scope(void) {
    Scope *s = (Scope *)malloc(sizeof(Scope));
    s->symbols = NULL;
    s->parent = current;
    s->level = current ? current->level + 1 : 0;
    current = s;
}

void symtab_pop_scope(void) {
    if (!current) return;
    Scope *dead = current;
    current = current->parent;

    Symbol *sym = dead->symbols;
    while (sym) {
        Symbol *next = sym->next;
        free(sym->name);
        free(sym);
        sym = next;
    }
    free(dead);
}

int symtab_current_level(void) {
    return current ? current->level : -1;
}

Symbol *symtab_lookup_current_scope(const char *name) {
    if (!current) return NULL;
    for (Symbol *s = current->symbols; s; s = s->next)
        if (strcmp(s->name, name) == 0) return s;
    return NULL;
}

Symbol *symtab_lookup(const char *name) {
    for (Scope *sc = current; sc; sc = sc->parent)
        for (Symbol *s = sc->symbols; s; s = s->next)
            if (strcmp(s->name, name) == 0) return s;
    return NULL;
}

int symtab_insert(const char *name, DataType type, int line) {
    if (symtab_lookup_current_scope(name) != NULL) {
        return 0; /* redeclaration in the same scope */
    }
    Symbol *s = (Symbol *)malloc(sizeof(Symbol));
    s->name = strdup(name);
    s->type = type;
    s->scope_level = current->level;
    s->line_declared = line;
    s->next = current->symbols;
    current->symbols = s;
    return 1;
}

void symtab_print_all(void) {
    printf("\n=== Symbol Table (innermost scope first) ===\n");
    printf("%-20s %-8s %-8s %-6s\n", "Name", "Type", "Scope", "Line");
    for (Scope *sc = current; sc; sc = sc->parent)
        for (Symbol *s = sc->symbols; s; s = s->next)
            printf("%-20s %-8s %-8d %-6d\n", s->name, type_to_str(s->type), s->scope_level, s->line_declared);
}

void symtab_destroy(void) {
    while (current) symtab_pop_scope();
}
