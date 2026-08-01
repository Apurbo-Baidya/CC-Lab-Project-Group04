# Design Notes / Architecture

## Pipeline (mirrors Lab 1 Sec 2.2 "Phases of a Compiler")

```
source.txt
   |
   v
lexer.l  (Flex)  -->  token stream          [Lab 1 Sec 2.4, Lab 2 Sec 2.3]
   |
   v
parser.y (Bison) -->  Abstract Syntax Tree   [Lab 1 Sec 2.5, Lab 2 Sec 2.4-2.6]
   |
   v
semantic.c       -->  type-checked AST +    [Lab 2 Sec 2.2 symbol table idea,
                       populated symbol      extended to full type checking]
                       table
   |
   v
codegen.c        -->  Quadruple table (TAC) [Lab 4 Sec 2.1-2.5]
                       + constant folding
                       + dead-code elimination [Lab 4 Sec 2.6]
```

Each phase is a **separate translation unit** with its own header, and
`main.c` only calls the next phase if the previous one reported zero
errors -- this is the "front end must succeed before you hand off to
codegen" rule every lab manual repeats.

## Why the parser only builds an AST (it doesn't evaluate or emit code)

Labs 2 and 4 show grammar actions doing the real work directly (e.g.
`expr: expr '+' expr { $$ = $1 + $3; }` evaluates immediately, and
`tac_gen.y` emits TAC immediately during parsing). That one-pass style
works for a single self-contained tool, but it means the parser must
already know about types and code generation.

This project instead has the parser build a **plain AST** (`ast.h` /
`ast.c`), and does semantic analysis and code generation as later,
independent tree-walks over that AST. This is the same overall
front-end architecture as Lab 1 Sec 2.2's phase table (Lexical ->
Syntax -> Semantic -> Intermediate Code, each with its own well
defined input/output), just implemented with an explicit intermediate
data structure instead of doing everything inside grammar actions. It
also makes it possible to print the parse tree (an explicit, textbook
deliverable) as a separate step from type-checking or codegen.

## Symbol table scoping

`symbol_table.c` implements scopes as a **stack of linked lists**
(`Scope` nodes, each pointing to its parent). `symtab_push_scope()` /
`symtab_pop_scope()` are called on entry/exit of every `{ ... }`
block (see `analyze_block()` in `semantic.c`), which is what gives
variables declared inside an `if`/`while` body their own scope: they
are visible inside that block and inside any nested block, but
disappear (their `Symbol` nodes are freed) once the block ends.

`symtab_lookup()` walks outward through parent scopes (implements
"visible in this block and all enclosing blocks"); `symtab_insert()`
only checks the *current* scope before inserting, which is what makes
shadowing a variable name in an inner block legal while redeclaring it
**twice in the same block** is a semantic error.

## Constant folding

Constant folding happens **while the quadruple table is being built**,
inside `gen_expr()` in `codegen.c`: before emitting a binary-op
quadruple, it checks whether both operand "places" are literal numeric
strings (`is_numeric_const`). If so, it computes the result in C
immediately and returns that value as the "place" -- **no quadruple is
ever created** for that subexpression. This matches the Lab 4 Sec 2.6
hint: "before appending a new row in `emit()`, check whether `arg1`
and `arg2` both look numeric... the quadruple is never even created."

## Dead-code elimination

Runs as a **second pass**, after the whole table exists (`codegen.c`,
`dead_code_eliminate()`), exactly because you cannot know whether a
temporary is "dead" until you have seen the entire rest of the program.
A quadruple is kept unless: its result is a temporary (`t<N>`, checked
by `is_temp()`) **and** that temporary never appears as `arg1` or
`arg2` of any later quadruple. Quadruples whose result is a real
variable, or that represent control flow / I/O (`label`, `goto`,
`ifFalse`, `print`), always have a visible effect and are never
removed.

Note: because this generator consumes every temporary immediately
(there is no common-subexpression sharing), genuinely dead temporaries
are rare in straight-line code from this generator -- the pass is
correct and will report `0 dead quadruple(s) removed` on most/all of
the provided examples. This is expected, not a bug; it's a natural
consequence of not implementing CSE (Lab 4 Sec 2.6 lists CSE as a
*different*, not-required optimization). You can demonstrate a
non-trivial removal by extending `emit()` to reuse a previously
computed identical `(op, arg1, arg2)` (i.e. adding CSE) -- see
Reflection Question 3 below.

## Known simplification: shadowed variables share a TAC name

`codegen.c` emits a variable's own source name (e.g. `x`) directly
into the quadruple table (`gen_expr(NODE_IDENT)` just returns
`strdup(e->name)`). This is correct for *semantic checking*, because
`symtab_lookup()` correctly resolves each use of `x` to the right
`Symbol` (inner vs outer) using scope nesting. But the **generated
TAC does not rename shadowed variables**, so if you look at
`examples/valid_04_scoping_and_widening.txt`'s output you will see
both the inner and outer `x` printed simply as `x` in the quadruple
table, even though they are different variables.

A production compiler avoids this by generating a unique internal
name per symbol-table entry (e.g. `x_L1_0`) instead of reusing the
source name. This is a natural, well-scoped extension if you want to
go further than the base requirements -- see Reflection Question 4
below.

## Answers sketch for common checkpoint-style questions

These are starting points, not full answers -- you are expected to run
the examples yourself and confirm/expand on these in your own words
for the viva.

- **Why Bison before Flex in the build?** Flex's generated scanner
  needs the token integer constants (`T_INT`, `ID`, ...), which Bison
  defines in `parser.tab.h`. `lexer.l` `#include`s that header, so it
  must already exist before `flex` even compiles (see the `Makefile`
  dependency: `lex.yy.c` depends on `parser.tab.h`).
- **Why does `%` require both operands to be `int`?** Following normal
  C semantics, the modulo operator is undefined for floating-point
  values, so `semantic.c` explicitly rejects a `float` operand on `%`
  with a distinct error message.
- **Why is assigning a `float` value to an `int` variable an error, but
  not the reverse?** Assigning `int` to `float` is a widening
  conversion that loses no information (`5` -> `5.0`); the reverse is
  narrowing and would silently truncate, so it is rejected as a type
  mismatch instead of a silent conversion.
- **What happens on a syntax error?** `yyerror()` reports it and
  `stmt_list: stmt_list error ';'` discards tokens up to the next
  `;`, so parsing resumes at the next statement instead of aborting
  immediately -- try `examples/error_syntax.txt`.
- **(Reflection Q4) How would you make shadowed variables get distinct
  TAC names?** Store a unique integer with each `Symbol` at insertion
  time (e.g. a global counter incremented in `symtab_insert`), and
  have `codegen.c` look the identifier up in the symbol table and emit
  `name_id` instead of just `name`. This requires giving `codegen.c`
  access to the same scope stack `semantic.c` used, which in turn
  means either re-running scope push/pop during codegen (mirroring
  `analyze_block`) or attaching each `NODE_IDENT`'s resolved `Symbol*`
  onto the AST node during semantic analysis so codegen can reuse it
  directly -- the second approach is the standard technique real
  compilers use (annotating the AST once, reading the annotation many
  times).
