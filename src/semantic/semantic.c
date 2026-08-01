#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "semantic.h"
#include "../symbol_table/symbol_table.h"

static int error_count = 0;

static void sem_error(int line, const char *fmt, ...) {
    error_count++;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Semantic Error (line %d): ", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

static int is_numeric(DataType t) { return t == TYPE_INT || t == TYPE_FLOAT; }

static void analyze_block(Node *block);   /* forward */
static DataType analyze_expr(Node *e);    /* forward */

static DataType analyze_expr(Node *e) {
    if (!e) return TYPE_UNKNOWN;

    switch (e->kind) {
        case NODE_INT_LIT:
        case NODE_FLOAT_LIT:
        case NODE_BOOL_LIT:
            return e->type; /* already set at construction time */

        case NODE_IDENT: {
            Symbol *s = symtab_lookup(e->name);
            if (!s) {
                sem_error(e->line, "undeclared variable '%s'", e->name);
                e->type = TYPE_UNKNOWN;
            } else {
                e->type = s->type;
            }
            return e->type;
        }

        case NODE_UNOP: {
            DataType t = analyze_expr(e->left);
            if (strcmp(e->op, "!") == 0) {
                if (t != TYPE_BOOL && t != TYPE_UNKNOWN)
                    sem_error(e->line, "operator '!' requires a bool operand, got %s", type_to_str(t));
                e->type = TYPE_BOOL;
            } else { /* uminus */
                if (!is_numeric(t) && t != TYPE_UNKNOWN)
                    sem_error(e->line, "unary '-' requires a numeric operand, got %s", type_to_str(t));
                e->type = (t == TYPE_UNKNOWN) ? TYPE_UNKNOWN : t;
            }
            return e->type;
        }

        case NODE_BINOP: {
            DataType lt = analyze_expr(e->left);
            DataType rt = analyze_expr(e->right);
            const char *op = e->op;

            int arithmetic = (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
                               strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0);
            int relational = (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
                               strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0);
            int equality   = (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0);
            int logical    = (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0);

            if (lt == TYPE_UNKNOWN || rt == TYPE_UNKNOWN) {
                e->type = TYPE_UNKNOWN; /* error already reported downstream; don't cascade */
                return e->type;
            }

            if (arithmetic) {
                if (!is_numeric(lt) || !is_numeric(rt)) {
                    sem_error(e->line, "operator '%s' requires numeric operands, got %s and %s",
                               op, type_to_str(lt), type_to_str(rt));
                    e->type = TYPE_UNKNOWN;
                } else if (strcmp(op, "%") == 0 && (lt == TYPE_FLOAT || rt == TYPE_FLOAT)) {
                    sem_error(e->line, "operator '%%' requires integer operands, got %s and %s",
                               type_to_str(lt), type_to_str(rt));
                    e->type = TYPE_UNKNOWN;
                } else {
                    /* int op int -> int ; anything with a float -> float (implicit widening) */
                    e->type = (lt == TYPE_FLOAT || rt == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
                }
            } else if (relational) {
                if (!is_numeric(lt) || !is_numeric(rt)) {
                    sem_error(e->line, "operator '%s' requires numeric operands, got %s and %s",
                               op, type_to_str(lt), type_to_str(rt));
                }
                e->type = TYPE_BOOL;
            } else if (equality) {
                if (lt != rt && !(is_numeric(lt) && is_numeric(rt))) {
                    sem_error(e->line, "operator '%s' cannot compare %s with %s",
                               op, type_to_str(lt), type_to_str(rt));
                }
                e->type = TYPE_BOOL;
            } else if (logical) {
                if (lt != TYPE_BOOL || rt != TYPE_BOOL) {
                    sem_error(e->line, "operator '%s' requires bool operands, got %s and %s",
                               op, type_to_str(lt), type_to_str(rt));
                }
                e->type = TYPE_BOOL;
            } else {
                e->type = TYPE_UNKNOWN;
            }
            return e->type;
        }

        default:
            return TYPE_UNKNOWN;
    }
}

static void analyze_stmt(Node *s) {
    if (!s) return;
    switch (s->kind) {
        case NODE_VAR_DECL:
            if (!symtab_insert(s->name, s->decl_type, s->line)) {
                Symbol *prev = symtab_lookup_current_scope(s->name);
                sem_error(s->line, "redeclaration of '%s' (already declared at line %d in this scope)",
                           s->name, prev->line_declared);
            }
            break;

        case NODE_ASSIGN: {
            Symbol *sym = symtab_lookup(s->name);
            DataType rhs_type = analyze_expr(s->expr);
            if (!sym) {
                sem_error(s->line, "assignment to undeclared variable '%s'", s->name);
                break;
            }
            if (rhs_type == TYPE_UNKNOWN) break; /* error already reported */
            if (sym->type == rhs_type) {
                /* ok */
            } else if (sym->type == TYPE_FLOAT && rhs_type == TYPE_INT) {
                /* ok: implicit int -> float widening */
            } else {
                sem_error(s->line, "type mismatch: cannot assign %s to variable '%s' of type %s",
                           type_to_str(rhs_type), s->name, type_to_str(sym->type));
            }
            break;
        }

        case NODE_IF: {
            DataType cond_t = analyze_expr(s->expr);
            if (cond_t != TYPE_BOOL && cond_t != TYPE_UNKNOWN)
                sem_error(s->line, "if condition must be bool, got %s", type_to_str(cond_t));
            analyze_block(s->then_block);
            if (s->else_block) analyze_block(s->else_block);
            break;
        }

        case NODE_WHILE: {
            DataType cond_t = analyze_expr(s->expr);
            if (cond_t != TYPE_BOOL && cond_t != TYPE_UNKNOWN)
                sem_error(s->line, "while condition must be bool, got %s", type_to_str(cond_t));
            analyze_block(s->then_block);
            break;
        }

        case NODE_PRINT:
            analyze_expr(s->expr);
            break;

        case NODE_BLOCK:
            analyze_block(s);
            break;

        default:
            break;
    }
}

static void analyze_block(Node *block) {
    symtab_push_scope();
    for (int i = 0; i < block->stmt_count; i++) analyze_stmt(block->stmts[i]);
    symtab_pop_scope();
}

int run_semantic_analysis(Node *program) {
    error_count = 0;
    symtab_init();
    /* The program's top-level statements share the global scope (level 0),
     * so we do NOT push an extra scope here -- symtab_init() already
     * created scope 0. Only nested blocks (if/while bodies) get their
     * own scope via analyze_block(). */
    for (int i = 0; i < program->stmt_count; i++) analyze_stmt(program->stmts[i]);
    return error_count;
}
