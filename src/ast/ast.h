#ifndef AST_H
#define AST_H

/* ============================================================
 * Abstract Syntax Tree node definitions.
 *
 * The AST is the shared data structure that the parser builds,
 * and that both the semantic analyzer and the TAC generator
 * later walk. Every node carries a line number so that semantic
 * errors and (eventually) codegen diagnostics can be traced back
 * to the source.
 * ============================================================ */

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_UNKNOWN   /* used when a type error has already been reported,
                       so we don't cascade the same error endlessly   */
} DataType;

typedef enum {
    /* Statements */
    NODE_PROGRAM,
    NODE_BLOCK,
    NODE_VAR_DECL,
    NODE_ASSIGN,
    NODE_IF,
    NODE_WHILE,
    NODE_PRINT,

    /* Expressions */
    NODE_BINOP,
    NODE_UNOP,
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_BOOL_LIT,
    NODE_IDENT
} NodeKind;

typedef struct Node {
    NodeKind kind;
    int line;

    /* Filled in by the semantic analyzer for expression nodes */
    DataType type;

    /* --- statement lists / blocks --- */
    struct Node **stmts;
    int stmt_count;
    int stmt_capacity;

    /* --- var decl --- */
    DataType decl_type;
    char *name;              /* declaration / assignment / identifier name */

    /* --- binop / unop --- */
    char *op;                /* "+", "-", "&&", "!", "uminus", ... */
    struct Node *left;
    struct Node *right;      /* NULL for unary ops */

    /* --- literals --- */
    long int_val;
    double float_val;
    int bool_val;

    /* --- assign --- */
    struct Node *expr;       /* rhs of assignment, or the print operand,
                                 or the loop/if condition                */

    /* --- if / while --- */
    struct Node *then_block;
    struct Node *else_block; /* NULL if no else */
} Node;

Node *new_program(void);
Node *new_block(void);
void  block_add(Node *block, Node *stmt);

Node *new_var_decl(DataType t, const char *name, int line);
Node *new_assign(const char *name, Node *expr, int line);
Node *new_if(Node *cond, Node *then_block, Node *else_block, int line);
Node *new_while(Node *cond, Node *body, int line);
Node *new_print(Node *expr, int line);

Node *new_binop(const char *op, Node *left, Node *right, int line);
Node *new_unop(const char *op, Node *operand, int line);
Node *new_int_lit(long v, int line);
Node *new_float_lit(double v, int line);
Node *new_bool_lit(int v, int line);
Node *new_ident(const char *name, int line);

const char *type_to_str(DataType t);
void print_ast(Node *n, int depth);
void free_ast(Node *n);

#endif
