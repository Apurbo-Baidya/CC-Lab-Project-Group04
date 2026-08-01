#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/* ---------- allocation helpers ---------- */

static Node *alloc_node(NodeKind kind, int line) {
    Node *n = (Node *)calloc(1, sizeof(Node));
    n->kind = kind;
    n->line = line;
    n->type = TYPE_UNKNOWN;
    return n;
}

static Node *alloc_block_like(NodeKind kind) {
    Node *n = alloc_node(kind, 0);
    n->stmt_capacity = 4;
    n->stmts = (Node **)malloc(sizeof(Node *) * n->stmt_capacity);
    n->stmt_count = 0;
    return n;
}

Node *new_program(void) { return alloc_block_like(NODE_PROGRAM); }
Node *new_block(void)   { return alloc_block_like(NODE_BLOCK); }

void block_add(Node *block, Node *stmt) {
    if (stmt == NULL) return; /* allows silently ignoring nodes from error recovery */
    if (block->stmt_count == block->stmt_capacity) {
        block->stmt_capacity *= 2;
        block->stmts = (Node **)realloc(block->stmts, sizeof(Node *) * block->stmt_capacity);
    }
    block->stmts[block->stmt_count++] = stmt;
}

Node *new_var_decl(DataType t, const char *name, int line) {
    Node *n = alloc_node(NODE_VAR_DECL, line);
    n->decl_type = t;
    n->name = strdup(name);
    return n;
}

Node *new_assign(const char *name, Node *expr, int line) {
    Node *n = alloc_node(NODE_ASSIGN, line);
    n->name = strdup(name);
    n->expr = expr;
    return n;
}

Node *new_if(Node *cond, Node *then_block, Node *else_block, int line) {
    Node *n = alloc_node(NODE_IF, line);
    n->expr = cond;
    n->then_block = then_block;
    n->else_block = else_block;
    return n;
}

Node *new_while(Node *cond, Node *body, int line) {
    Node *n = alloc_node(NODE_WHILE, line);
    n->expr = cond;
    n->then_block = body;
    return n;
}

Node *new_print(Node *expr, int line) {
    Node *n = alloc_node(NODE_PRINT, line);
    n->expr = expr;
    return n;
}

Node *new_binop(const char *op, Node *left, Node *right, int line) {
    Node *n = alloc_node(NODE_BINOP, line);
    n->op = strdup(op);
    n->left = left;
    n->right = right;
    return n;
}

Node *new_unop(const char *op, Node *operand, int line) {
    Node *n = alloc_node(NODE_UNOP, line);
    n->op = strdup(op);
    n->left = operand;
    return n;
}

Node *new_int_lit(long v, int line) {
    Node *n = alloc_node(NODE_INT_LIT, line);
    n->int_val = v;
    n->type = TYPE_INT;
    return n;
}

Node *new_float_lit(double v, int line) {
    Node *n = alloc_node(NODE_FLOAT_LIT, line);
    n->float_val = v;
    n->type = TYPE_FLOAT;
    return n;
}

Node *new_bool_lit(int v, int line) {
    Node *n = alloc_node(NODE_BOOL_LIT, line);
    n->bool_val = v;
    n->type = TYPE_BOOL;
    return n;
}

Node *new_ident(const char *name, int line) {
    Node *n = alloc_node(NODE_IDENT, line);
    n->name = strdup(name);
    return n;
}

const char *type_to_str(DataType t) {
    switch (t) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_BOOL: return "bool";
        default: return "unknown";
    }
}

static void indent(int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
}

/* Text-based indented AST printer, as required by Section 4.3 of the manual. */
void print_ast(Node *n, int depth) {
    if (!n) return;
    switch (n->kind) {
        case NODE_PROGRAM:
            indent(depth); printf("Program\n");
            for (int i = 0; i < n->stmt_count; i++) print_ast(n->stmts[i], depth + 1);
            break;
        case NODE_BLOCK:
            indent(depth); printf("Block\n");
            for (int i = 0; i < n->stmt_count; i++) print_ast(n->stmts[i], depth + 1);
            break;
        case NODE_VAR_DECL:
            indent(depth); printf("VarDecl(%s %s) [line %d]\n", type_to_str(n->decl_type), n->name, n->line);
            break;
        case NODE_ASSIGN:
            indent(depth); printf("Assign(%s) [line %d]\n", n->name, n->line);
            print_ast(n->expr, depth + 1);
            break;
        case NODE_IF:
            indent(depth); printf("If [line %d]\n", n->line);
            indent(depth + 1); printf("Cond:\n");
            print_ast(n->expr, depth + 2);
            indent(depth + 1); printf("Then:\n");
            print_ast(n->then_block, depth + 2);
            if (n->else_block) {
                indent(depth + 1); printf("Else:\n");
                print_ast(n->else_block, depth + 2);
            }
            break;
        case NODE_WHILE:
            indent(depth); printf("While [line %d]\n", n->line);
            indent(depth + 1); printf("Cond:\n");
            print_ast(n->expr, depth + 2);
            indent(depth + 1); printf("Body:\n");
            print_ast(n->then_block, depth + 2);
            break;
        case NODE_PRINT:
            indent(depth); printf("Print [line %d]\n", n->line);
            print_ast(n->expr, depth + 1);
            break;
        case NODE_BINOP:
            indent(depth); printf("BinaryOp(%s) [line %d]\n", n->op, n->line);
            print_ast(n->left, depth + 1);
            print_ast(n->right, depth + 1);
            break;
        case NODE_UNOP:
            indent(depth); printf("UnaryOp(%s) [line %d]\n", n->op, n->line);
            print_ast(n->left, depth + 1);
            break;
        case NODE_INT_LIT:
            indent(depth); printf("IntLiteral(%ld)\n", n->int_val);
            break;
        case NODE_FLOAT_LIT:
            indent(depth); printf("FloatLiteral(%g)\n", n->float_val);
            break;
        case NODE_BOOL_LIT:
            indent(depth); printf("BoolLiteral(%s)\n", n->bool_val ? "true" : "false");
            break;
        case NODE_IDENT:
            indent(depth); printf("Ident(%s) [line %d]\n", n->name, n->line);
            break;
    }
}

void free_ast(Node *n) {
    if (!n) return;
    switch (n->kind) {
        case NODE_PROGRAM:
        case NODE_BLOCK:
            for (int i = 0; i < n->stmt_count; i++) free_ast(n->stmts[i]);
            free(n->stmts);
            break;
        case NODE_VAR_DECL:
            free(n->name);
            break;
        case NODE_ASSIGN:
            free(n->name);
            free_ast(n->expr);
            break;
        case NODE_IF:
            free_ast(n->expr);
            free_ast(n->then_block);
            free_ast(n->else_block);
            break;
        case NODE_WHILE:
            free_ast(n->expr);
            free_ast(n->then_block);
            break;
        case NODE_PRINT:
            free_ast(n->expr);
            break;
        case NODE_BINOP:
            free(n->op);
            free_ast(n->left);
            free_ast(n->right);
            break;
        case NODE_UNOP:
            free(n->op);
            free_ast(n->left);
            break;
        case NODE_IDENT:
            free(n->name);
            break;
        default:
            break;
    }
    free(n);
}
