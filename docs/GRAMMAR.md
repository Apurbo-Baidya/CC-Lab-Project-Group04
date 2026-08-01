# Language Grammar (EBNF)

This is the context-free grammar implemented in `src/parser/parser.y`.
Terminals are quoted or UPPERCASE; `{...}` means zero-or-more.

```
program      -> stmt_list

stmt_list    -> { stmt }

stmt         -> decl
              | assign
              | if_stmt
              | while_stmt
              | print_stmt
              | block

type_spec    -> "int" | "float" | "bool"

decl         -> type_spec ID ";"

assign       -> ID "=" expr ";"

if_stmt      -> "if" "(" expr ")" block
              | "if" "(" expr ")" block "else" block

while_stmt   -> "while" "(" expr ")" block

print_stmt   -> "print" expr ";"

block        -> "{" stmt_list "}"

expr         -> expr "+" expr
              | expr "-" expr
              | expr "*" expr
              | expr "/" expr
              | expr "%" expr
              | expr "<" expr
              | expr ">" expr
              | expr "<=" expr
              | expr ">=" expr
              | expr "==" expr
              | expr "!=" expr
              | expr "&&" expr
              | expr "||" expr
              | "!" expr
              | "-" expr               (unary minus)
              | "(" expr ")"
              | ID
              | INT_LIT
              | FLOAT_LIT
              | "true"
              | "false"
```

## Operator precedence (lowest to highest)

Declared in `parser.y` from lowest to highest, matching the pattern
taught in Lab 2 Sec 2.5:

| Level | Operators           | Associativity |
|-------|---------------------|---------------|
| 1 (lowest) | `\|\|`          | left |
| 2     | `&&`                 | left |
| 3     | `==` `!=`            | non-associative |
| 4     | `<` `>` `<=` `>=`    | non-associative |
| 5     | `+` `-`              | left |
| 6     | `*` `/` `%`          | left |
| 7 (highest) | unary `!`, unary `-` (`UMINUS`) | right |

Relational and equality operators are declared `%nonassoc` deliberately:
this makes `a < b < c` a **syntax error**, which is the conventional
choice for a C-like language (the result of `a < b` is a `bool`, and
`bool < c` is meaningless), and it also avoids an unnecessary
shift/reduce ambiguity.

## Why `%prec UMINUS` is needed

`-` is declared as both a binary operator (subtraction, precedence
level 5) and part of the unary-minus rule. Without `%prec UMINUS`,
Bison would use `-`'s own (low, level-5) precedence for the unary
rule too, which would make `-2 * 3` parse incorrectly. `%prec UMINUS`
tells Bison "treat this specific production as if its precedence were
that of the placeholder token `UMINUS`", which is declared at the
highest level.

## Type rules enforced by the semantic analyzer (`src/semantic/semantic.c`)

| Category | Operators | Operand types required | Result type |
|---|---|---|---|
| Arithmetic | `+ - * /` | both `int` or `float` (mixed allowed; result widens to `float`) | `int` or `float` |
| Integer-only | `%` | both `int` | `int` |
| Relational | `< > <= >=` | both numeric | `bool` |
| Equality | `== !=` | same type, or both numeric | `bool` |
| Logical | `&& \|\|` | both `bool` | `bool` |
| Unary `!` | | `bool` | `bool` |
| Unary `-` | | numeric | same as operand |
| Assignment | `=` | RHS type equals LHS type, OR RHS is `int` and LHS is `float` (implicit widening) | — |
| `if` / `while` condition | | must be `bool` | — |

## Error categories detected

- **Lexical** (`lexer.l`): any character not matched by any token rule.
  Reported and scanning continues (no token returned), so later
  lexical errors are still found in the same pass.
- **Syntax** (`parser.y`): any token sequence the grammar rejects.
  Recovered via `stmt_list: stmt_list error ';'`, which discards
  tokens up to the next `;` and resumes parsing the next statement.
- **Semantic** (`semantic.c`): undeclared-variable use, redeclaration
  in the same scope, type mismatch in assignment, non-bool `if`/`while`
  condition, and operator/operand type mismatches.

Each phase's errors are collected (not just the first one), and later
phases are skipped entirely if an earlier phase found any errors
(matching the "front end must succeed before intermediate code
generation" rule from Lab 1 Sec 2.2 / Lab 4 Sec 2.1).
