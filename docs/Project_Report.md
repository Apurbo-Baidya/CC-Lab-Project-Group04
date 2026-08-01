# Compiler Construction Lab — Project Report

**Group 04 — Metropolitan University, Bangladesh**
**Project:** Design and Implement a Mini Programming Language Compiler using Flex and Bison
**Members:** _fill in names, IDs, GitHub usernames_

---

## 1. Introduction

Compilers are the bridge between human-readable source code and a form a machine (or, in
this project's case, a well-defined intermediate representation) can execute. Over the
semester's labs we built each compiler phase in isolation: a Flex scanner, a symbol table,
a Bison expression parser with precedence, and a Three-Address-Code generator. This project's
purpose is to integrate all of those phases into one coherent front end for a single,
fixed language, so that the interfaces between phases — token stream, AST, annotated AST,
TAC — are handled explicitly rather than assumed.

## 2. Objectives

- Implement a complete compiler front end (lexer → parser → AST → semantic analysis →
  intermediate code generation) for the language defined in Section 5 of the project manual.
- Demonstrate correct detection and reporting of lexical, syntax, and semantic errors, with
  line numbers, for every required error category.
- Demonstrate correct Three-Address Code generation for arithmetic, relational, logical,
  and control-flow constructs.
- Maintain a clean, professional, incrementally-committed GitHub repository reflecting
  contribution from every group member.

## 3. Language Specification

**Data types:** `int`, `float`, `bool`.

**Statements:** variable declaration (`int x;`), assignment (`x = 5;`), arithmetic /
relational / logical expressions, `if`, `if-else`, `while`, `print`, and nested `{ }` blocks
with block-local scoping.

**Operators:**

| Category | Operators |
|---|---|
| Arithmetic | `+  -  *  /  %` |
| Relational | `<  >  <=  >=  ==  !=` |
| Logical | `&&  \|\|  !` |

**Lexical elements:** `{ } ( ) ;`, identifiers (`[a-zA-Z_][a-zA-Z0-9_]*`), integer literals,
floating-point literals, boolean literals `true`/`false`, `//` and `/* */` comments.

### Formal Grammar (CFG)

```
program      -> stmt_list
stmt_list    -> { stmt }
stmt         -> decl | assign | if_stmt | while_stmt | print_stmt | block
type_spec    -> "int" | "float" | "bool"
decl         -> type_spec ID ";"
assign       -> ID "=" expr ";"
if_stmt      -> "if" "(" expr ")" block
              | "if" "(" expr ")" block "else" block
while_stmt   -> "while" "(" expr ")" block
print_stmt   -> "print" expr ";"
block        -> "{" stmt_list "}"
expr         -> expr "+" expr | expr "-" expr | expr "*" expr
              | expr "/" expr | expr "%" expr
              | expr "<" expr | expr ">" expr | expr "<=" expr | expr ">=" expr
              | expr "==" expr | expr "!=" expr
              | expr "&&" expr | expr "||" expr
              | "!" expr | "-" expr
              | "(" expr ")"
              | ID | INT_LIT | FLOAT_LIT | "true" | "false"
```

The full precedence table and type rules are in `docs/GRAMMAR.md` and are implemented
verbatim in `src/parser/parser.y` via Bison's `%left`/`%right`/`%nonassoc` declarations.

## 4. Compiler Architecture

```
source file
    |
    v
lexer.l  (Flex)   ->  token stream
    |
    v
parser.y (Bison)  ->  Abstract Syntax Tree (no evaluation, no codegen at this stage)
    |
    v
semantic.c        ->  scoped symbol table populated; AST type-checked; errors collected
    |                  (only proceeds if zero errors)
    v
codegen.c         ->  Quadruple-table TAC, then constant folding + dead-code elimination
```

Each phase is an independent module (`src/<phase>/`) communicating only through the shared
`Node` (AST) type and, in the semantic phase, through the symbol table. `src/main.c` is the
driver that runs the phases in order and halts the pipeline as soon as a phase reports errors,
matching how a real compiler front end behaves.

## 5. Lexer Design

`src/lexer/lexer.l`. Keyword patterns are listed before the generic identifier pattern so
Flex's longest-match/first-rule ordering resolves keyword-vs-identifier ambiguity without a
separate keyword table. `%option yylineno` provides line numbers for every error message.
Single-line (`//`) and block (`/* */`) comments are matched and discarded before reaching any
other rule. The catch-all rule (`.`) reports any unmatched character as a lexical error with
its line number and continues scanning (does not abort), so multiple lexical errors in one
file are all reported in a single pass.

## 6. Parser Design

`src/parser/parser.y`. Bison generates an LALR(1) parser. Operator precedence/associativity
(lowest to highest): `||`, `&&`, `==`/`!=` and `<`/`>`/`<=`/`>=` (both `%nonassoc`, so chained
comparisons like `a < b < c` are syntax errors rather than silently mis-parsed), `+`/`-`,
`*`/`/`/`%`, then unary `!` and unary `-` (`%prec UMINUS`) at the highest level. Error
recovery uses Bison's `error` token in the statement-list rule to resynchronize at the next
`;`, so one syntax error does not necessarily prevent the rest of the file from being checked.

## 7. Abstract Syntax Tree

`src/ast/ast.h` / `ast.c`. Each meaningful construct is its own `NodeKind`
(`NODE_VAR_DECL`, `NODE_ASSIGN`, `NODE_IF`, `NODE_WHILE`, `NODE_BINOP`, ...); parentheses and
semicolons are not represented — they only guided parsing. Every node carries a source line
number. `print_ast()` renders the tree as indented text (see any `tests/valid/*.md` for
sample output). `free_ast()` recursively releases the tree.

## 8. Semantic Analysis

`src/semantic/semantic.c`. A single recursive walk over the AST that:

- pushes a new symbol-table scope on entering every block (`if`/`while` body) and pops it on
  exit, giving correct nested-block scoping;
- inserts each declaration into the current scope, rejecting redeclaration in the *same*
  scope (Section 4.5's "Redeclaration" rule);
- looks up every identifier use through all enclosing scopes, reporting "undeclared variable"
  if not found anywhere, and (implicitly, via scope popping) "scope violation" if a variable
  that exists elsewhere in the tree is no longer visible at the point of use;
- type-checks every expression bottom-up, and every assignment (RHS type must match the
  declared LHS type) and every `if`/`while` condition (must be `bool`).

All errors are accumulated (not just the first) and reported with line numbers; TAC
generation is skipped entirely if any semantic error was found.

## 9. Symbol Table

`src/symbol_table/symbol_table.c`. A stack of `Scope` structs, each holding a singly-linked
list of `Symbol { name, type, scope_level, line_declared }`. `symtab_push_scope()` /
`symtab_pop_scope()` are called on every block entry/exit. `symtab_lookup()` searches the
current scope and then each enclosing scope in turn (giving correct shadowing/visibility
semantics); `symtab_lookup_current_scope()` searches only the innermost scope and is used to
detect redeclaration before insertion.

## 10. Intermediate Code

`src/codegen/codegen.c`. Three-Address Code is stored as a quadruple table
(`op, arg1, arg2, result`) rather than emitted directly as text, so it can be optimized
before being printed. Binary/unary expressions emit one quadruple per operation and return a
new temporary (`t1`, `t2`, ...). `if`, `if-else`, and `while` are lowered using generated
labels (`L1`, `L2`, ...) and `goto` / `ifFalse ... goto` quadruples, following the standard
patterns:

```
if (cond) A else B          while (cond) A
  <cond> -> t                L1: <cond> -> t
  ifFalse t goto L1              ifFalse t goto L2
  <A>                             <A>
  goto L2                        goto L1
L1: <B>                     L2:
L2:
```

**Bonus optimizations** (Section 14): constant folding evaluates an operation immediately in
`emit()` if both operands are numeric literals, so no quadruple is created at all for that
subexpression; dead-code elimination is a post-pass that marks every temporary used as an
operand anywhere (or as the program's final result) as "live" and drops any quadruple whose
result is never marked live. Example (see `tests/valid/arithmetic.md`): `c = a + b * 2`
produces `t1 = b * 2; t2 = a + t1; c = t2` — no constants to fold here since `a`/`b` are
variables, but purely-constant subexpressions elsewhere in a program are folded in place.

## 11. Challenges

**Separating the parser from evaluation.** The Lab 1/2/4 exercises all computed a result or
emitted code directly inside the Bison actions, as the grammar was reduced. For this project
we instead had the parser build a plain AST and do nothing else, so that semantic analysis
could run as its own complete pass afterward. This was a deliberate architectural change from
the labs' style, and it took some trial and error to get every grammar rule returning the
right `Node *` instead of a computed value.

**Unary minus precedence.** `expr: '-' expr` needed `%prec UMINUS` to force it to bind
tighter than binary `-`; without it, Bison's default precedence (inherited from the last
terminal in the rule) made `-a - b` parse ambiguously and produced shift/reduce warnings
during `bison -d`.

**Nested-scope symbol table timing.** Early versions popped a block's scope *before* the
semantic checks for that block's last statement had finished running, which caused variables
declared at the very end of a block to appear "undeclared" to themselves. Moving the
`symtab_pop_scope()` call to strictly after the block's statement list is fully processed
fixed it — this bug only showed up on blocks with a variable declared and used on the same
last line, which took a while to isolate.

**Distinguishing "undeclared" from "out of scope."** A variable declared inside an `if`/`while`
block and used after the block ends is technically a scope-visibility error, not a missing
declaration — but from the symbol table's point of view both look identical (`symtab_lookup`
simply returns `NULL` either way once the block's scope has been popped). We settled on
reporting both as "undeclared variable," since that's what a real lookup failure is at that
point in the program; `tests/invalid/scope_violation.txt` documents this deliberately.

**Keeping the final result "live" during dead-code elimination.** Our first dead-code pass
only marked a temporary as live if it appeared as an operand in a *later* quadruple, which
incorrectly deleted the very last computation in a program (nothing after it ever reads its
result). We fixed this by always marking the final quadruple's result as live regardless of
whether anything references it afterward.

## 12. Testing

`tests/valid/` — 6 programs covering declarations, arithmetic + precedence, assignment
chains, `if`/`else`, `while`, and a combined program exercising all control-flow forms
together. All compile cleanly through to TAC (verified, see `tests/valid/actual_output/`).

`tests/invalid/` — one program per required error category:

| File | Category | Result |
|---|---|---|
| `lexical_error.md` | Lexical | invalid token `@`, reported with line number |
| `syntax_error.md` | Syntax | missing `;` / missing `)`, both reported, parser resynchronizes |
| `undeclared_variable.md` | Semantic — undeclared use | reported |
| `redeclaration.md` | Semantic — redeclaration | reported |
| `scope_violation.md` | Semantic — scope violation | variable used outside its block reported as undeclared at that scope |
| `type_mismatch.md` | Semantic — type mismatch | assigning `int` to `bool` reported |
| `invalid_assignment.md` | Semantic — invalid assignment | assigning `bool` to `int` reported |

Each has a captured actual run under `tests/{valid,invalid}/actual_output/`.

## 13. Conclusion

Building each compiler phase in isolation across Labs 1, 2, and 4 made every individual piece
feel manageable: a scanner that prints tokens, a parser that evaluates an expression, a code
generator that emits TAC from a hard-coded grammar. Integrating them into one pipeline for
this project showed us that most of the real difficulty in building a compiler isn't inside
any single phase — it's at the *boundaries* between them. The AST had to expose exactly the
right information (line numbers, node kinds, child pointers) for semantic analysis to walk it
usefully; semantic analysis, in turn, had to annotate types and validate scope *completely*
before codegen could safely assume every identifier and expression it saw was well-formed.
Getting the symbol table's scope push/pop timing right, in particular, made clear how a
single off-by-one decision in one phase can silently corrupt the correctness of everything
downstream of it.

We also came away with a much better appreciation for why real compilers report *all* the
errors they can find in one pass rather than stopping at the first one — implementing that
ourselves (accumulating semantic errors across the whole AST, and using Bison's `error` token
to resynchronize past a syntax error) required treating error handling as a first-class part
of the design from the start, not something bolted on afterward. Overall, this project turned
four semesters' worth of separate lab exercises into a single, coherent understanding of how a
compiler front end actually fits together.

## 14. References

- Aho, Lam, Sethi, Ullman, *Compilers: Principles, Techniques, and Tools* (2nd ed.).
- Flex Manual: https://westes.github.io/flex/manual/
- Bison Manual: https://www.gnu.org/software/bison/manual/
- Course lab manuals: Lab 1 (environment & tool basics), Lab 2 (symbol tables & Bison
  precedence), Lab 4 (TAC & optimization).

---

_(Sections renumbered to match this report's own flow; content maps 1:1 onto the manual's
required Section 12 chapter list — Introduction, Objectives, Language Specification, Compiler
Architecture, Lexer Design, Parser Design, AST, Semantic Analysis, Symbol Table, Intermediate
Code, Challenges, Testing, Conclusion, References.)_
