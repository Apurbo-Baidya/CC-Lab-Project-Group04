#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "codegen.h"

#define MAX_QUADS 2000

typedef struct {
    char *op;      /* "+","-","*","/","%","uminus","!","=","goto","ifFalse","label","print" */
    char *arg1;
    char *arg2;    /* "-" when unused */
    char *result;
} Quad;

static Quad quads[MAX_QUADS];
static int quadCount = 0;
static int tempCount = 0;
static int labelCount = 0;
static int foldedCount = 0; /* how many arithmetic ops were folded at compile time */

/* ---------- helpers ---------- */

static char *newTemp(void) {
    char buf[16];
    snprintf(buf, sizeof(buf), "t%d", ++tempCount);
    return strdup(buf);
}

static char *newLabel(void) {
    char buf[16];
    snprintf(buf, sizeof(buf), "L%d", ++labelCount);
    return strdup(buf);
}

static int is_temp(const char *s) {
    if (!s || s[0] != 't') return 0;
    for (int i = 1; s[i]; i++) if (!isdigit((unsigned char)s[i])) return 0;
    return s[1] != '\0';
}

/* Recognises an integer or float literal (optionally negative), which is
 * what makes a place foldable at compile time. Variables and temporaries
 * are never numeric-constant strings, so this cleanly distinguishes them. */
static int is_numeric_const(const char *s) {
    if (!s || !*s) return 0;
    int i = 0;
    if (s[0] == '-') i = 1;
    if (!s[i]) return 0;
    int seen_digit = 0, seen_dot = 0;
    for (; s[i]; i++) {
        if (isdigit((unsigned char)s[i])) seen_digit = 1;
        else if (s[i] == '.' && !seen_dot) seen_dot = 1;
        else return 0;
    }
    return seen_digit;
}

static int is_float_const(const char *s) { return strchr(s, '.') != NULL; }

static char *fold_binop(const char *op, const char *a, const char *b) {
    char buf[64];
    if (is_float_const(a) || is_float_const(b)) {
        double x = atof(a), y = atof(b), r = 0;
        if (strcmp(op, "+") == 0) r = x + y;
        else if (strcmp(op, "-") == 0) r = x - y;
        else if (strcmp(op, "*") == 0) r = x * y;
        else if (strcmp(op, "/") == 0) r = (y != 0) ? x / y : 0;
        snprintf(buf, sizeof(buf), "%g", r);
    } else {
        long x = atol(a), y = atol(b), r = 0;
        if (strcmp(op, "+") == 0) r = x + y;
        else if (strcmp(op, "-") == 0) r = x - y;
        else if (strcmp(op, "*") == 0) r = x * y;
        else if (strcmp(op, "/") == 0) r = (y != 0) ? x / y : 0;
        else if (strcmp(op, "%") == 0) r = (y != 0) ? x % y : 0;
        snprintf(buf, sizeof(buf), "%ld", r);
    }
    foldedCount++;
    return strdup(buf);
}

static void emit(const char *op, const char *arg1, const char *arg2, const char *result) {
    if (quadCount >= MAX_QUADS) { fprintf(stderr, "codegen: quadruple table overflow\n"); exit(1); }
    quads[quadCount].op = strdup(op);
    quads[quadCount].arg1 = strdup(arg1 ? arg1 : "-");
    quads[quadCount].arg2 = strdup(arg2 ? arg2 : "-");
    quads[quadCount].result = strdup(result ? result : "-");
    quadCount++;
}

/* ---------- expression codegen ---------- */

static char *gen_expr(Node *e) {
    if (!e) return strdup("-");
    switch (e->kind) {
        case NODE_INT_LIT: {
            char buf[32]; snprintf(buf, sizeof(buf), "%ld", e->int_val); return strdup(buf);
        }
        case NODE_FLOAT_LIT: {
            char buf[32]; snprintf(buf, sizeof(buf), "%g", e->float_val); return strdup(buf);
        }
        case NODE_BOOL_LIT:
            return strdup(e->bool_val ? "true" : "false");
        case NODE_IDENT:
            return strdup(e->name);

        case NODE_UNOP: {
            char *place = gen_expr(e->left);
            if (strcmp(e->op, "uminus") == 0 && is_numeric_const(place)) {
                char buf[64];
                if (is_float_const(place)) snprintf(buf, sizeof(buf), "%g", -atof(place));
                else snprintf(buf, sizeof(buf), "%ld", -atol(place));
                free(place);
                foldedCount++;
                return strdup(buf);
            }
            char *t = newTemp();
            emit(e->op, place, "-", t);
            free(place);
            return t;
        }

        case NODE_BINOP: {
            char *l = gen_expr(e->left);
            char *r = gen_expr(e->right);
            int arithmetic = (strcmp(e->op, "+") == 0 || strcmp(e->op, "-") == 0 ||
                               strcmp(e->op, "*") == 0 || strcmp(e->op, "/") == 0 ||
                               strcmp(e->op, "%") == 0);
            if (arithmetic && is_numeric_const(l) && is_numeric_const(r)) {
                char *folded = fold_binop(e->op, l, r);
                free(l); free(r);
                return folded; /* no quadruple created at all */
            }
            char *t = newTemp();
            emit(e->op, l, r, t);
            free(l); free(r);
            return t;
        }
        default:
            return strdup("-");
    }
}

/* ---------- statement codegen ---------- */

static void gen_block(Node *block);

static void gen_stmt(Node *s) {
    if (!s) return;
    switch (s->kind) {
        case NODE_VAR_DECL:
            /* Declarations carry no run-time action; the symbol already
             * exists thanks to semantic analysis. Nothing to emit. */
            break;

        case NODE_ASSIGN: {
            char *place = gen_expr(s->expr);
            emit("=", place, "-", s->name);
            free(place);
            break;
        }

        case NODE_IF: {
            char *cond = gen_expr(s->expr);
            if (s->else_block) {
                char *Lelse = newLabel();
                char *Lend = newLabel();
                emit("ifFalse", cond, "-", Lelse);
                gen_block(s->then_block);
                emit("goto", "-", "-", Lend);
                emit("label", "-", "-", Lelse);
                gen_block(s->else_block);
                emit("label", "-", "-", Lend);
                free(Lelse); free(Lend);
            } else {
                char *Lend = newLabel();
                emit("ifFalse", cond, "-", Lend);
                gen_block(s->then_block);
                emit("label", "-", "-", Lend);
                free(Lend);
            }
            free(cond);
            break;
        }

        case NODE_WHILE: {
            char *Lstart = newLabel();
            char *Lend = newLabel();
            emit("label", "-", "-", Lstart);
            char *cond = gen_expr(s->expr);
            emit("ifFalse", cond, "-", Lend);
            free(cond);
            gen_block(s->then_block);
            emit("goto", "-", "-", Lstart);
            emit("label", "-", "-", Lend);
            free(Lstart); free(Lend);
            break;
        }

        case NODE_PRINT: {
            char *place = gen_expr(s->expr);
            emit("print", place, "-", "-");
            free(place);
            break;
        }

        case NODE_BLOCK:
            gen_block(s);
            break;

        default:
            break;
    }
}

static void gen_block(Node *block) {
    for (int i = 0; i < block->stmt_count; i++) gen_stmt(block->stmts[i]);
}

/* ---------- printing ---------- */

static void print_table(int *keep /* NULL = print all */) {
    printf("%-4s %-8s %-8s %-8s %-8s\n", "No.", "Op", "Arg1", "Arg2", "Result");
    for (int i = 0; i < quadCount; i++) {
        if (keep && !keep[i]) continue;
        printf("%-4d %-8s %-8s %-8s %-8s\n", i + 1, quads[i].op, quads[i].arg1, quads[i].arg2, quads[i].result);
    }
}

/* Dead-code elimination: a quadruple whose RESULT is a temporary that is
 * never used as an argument anywhere later (and is not itself a control
 * label / has no side effect) can be dropped. Assignments to real
 * variables, print, goto, ifFalse and label rows always have a visible
 * effect and are always kept. */
static int *dead_code_eliminate(void) {
    int *keep = (int *)malloc(sizeof(int) * quadCount);
    for (int i = 0; i < quadCount; i++) keep[i] = 1;

    for (int i = 0; i < quadCount; i++) {
        if (!is_temp(quads[i].result)) continue; /* always keep non-temp results */
        int used_later = 0;
        for (int j = i + 1; j < quadCount; j++) {
            if (strcmp(quads[j].arg1, quads[i].result) == 0 ||
                strcmp(quads[j].arg2, quads[i].result) == 0) {
                used_later = 1;
                break;
            }
        }
        if (!used_later) keep[i] = 0;
    }
    return keep;
}

void generate_and_print_tac(Node *program) {
    quadCount = 0; tempCount = 0; labelCount = 0; foldedCount = 0;
    gen_block(program);

    printf("\n=== Unoptimized Three-Address Code (Quadruple Table) ===\n");
    print_table(NULL);
    printf("(%d arithmetic operation(s) were already constant-folded during generation)\n", foldedCount);

    int *keep = dead_code_eliminate();
    int removed = 0;
    for (int i = 0; i < quadCount; i++) if (!keep[i]) removed++;

    printf("\n=== Optimized Three-Address Code (after dead-code elimination) ===\n");
    print_table(keep);
    printf("(%d dead quadruple(s) removed)\n", removed);

    free(keep);
}
