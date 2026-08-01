# CC-Lab-Project-Group04 — Mini-Language Compiler Front End

**Compiler Construction Lab, CSE 416 — Metropolitan University, Bangladesh**
Project: *Design and Implement a Mini Programming Language Compiler using Flex and Bison*

A Flex + Bison front end for the fixed language specified in `Compiler Construction Lab Project Manual.pdf`
(Section 5): **lexer → parser (builds an AST) → semantic analysis (scoped symbol table + type
checking) → intermediate code generation (Three-Address Code as a quadruple table)**, with
constant folding and dead-code elimination applied as bonus optimizations (Section 14).

## Team — Group 04

| Name | GitHub Username | Role (primary) |
|---|---|---|
| Shahanur Skhter Simu | Shahanur2001 | Lexer & Symbol Table |
| Sanjida Choudhury Safa| safachy128 | Parser / AST & Semantic Analysis |
| Apurbo Baidya Sachchho | Apurbo-Baidya | Intermediate Code Generation & Testing/Docs |

> Roles are "primary" only — see `docs/DESIGN_NOTES.md` and the commit history for how work
> actually overlapped. All three members contributed commits across every module (see the
> "Git Workflow" section below); this table is for the report/README, not a strict division of labor.

## Project layout

```
project-root/
├── docs/
│   ├── GRAMMAR.md            full EBNF grammar, precedence table, type rules
│   ├── DESIGN_NOTES.md       architecture rationale, viva-prep answers
│   └── Project_Report.md     the written report (Section 12 of the manual)
├── src/
│   ├── lexer/lexer.l         Flex specification
│   ├── parser/parser.y       Bison grammar → builds the AST
│   ├── ast/                  AST node types, constructors, printer
│   ├── symbol_table/         scoped (nested-block) symbol table
│   ├── semantic/             type-checking / scope-checking pass
│   ├── codegen/               TAC generation, constant folding, dead-code elim.
│   └── main.c                 driver: runs all phases in order
├── tests/
│   ├── valid/                 *.md description + *.txt runnable source + actual_output/
│   └── invalid/                one case per required semantic/lexical/syntax error category
├── examples/
│   ├── valid/                  representative sample program
│   └── invalid/                representative invalid sample program
├── Makefile
└── README.md (this file)
```

## The language, briefly

```c
int x;
int y;
bool flag;

x = 10;
y = 0;
flag = true;

while (x > 0) {
    y = y + x;
    x = x - 1;
}

if (flag == true) {
    print y;
} else {
    print x;
}
```

Full spec: types `int|float|bool`; statements: declaration, assignment, `if`/`if-else`,
`while`, `print`, nested `{ }` blocks with proper scoping; operators `+ - * / %`,
`< > <= >= == !=`, `&& || !`. See `docs/GRAMMAR.md` for the complete CFG and precedence table.

## Building

Requires `gcc`, `flex`, `bison`, `make` (see `INSTALL.md`).

```bash
make
```

This runs Bison first (to generate `parser.tab.h`, which the lexer needs), then Flex, then
compiles everything into a single executable: **`./compiler`**.

```bash
make clean   # removes build/, the compiler binary, and all generated Flex/Bison output
```

## Running

```bash
./compiler <path-to-source-file>
```

Example:

```bash
./compiler tests/valid/complete_program.txt
```

For every input, the compiler prints, in order:

1. Lexical/syntax errors (if any) — compilation halts here if either phase reports errors.
2. The Abstract Syntax Tree (indented text form).
3. Semantic analysis result — either `Semantic analysis passed with 0 errors.` or a list of
   every semantic error found (undeclared variable, redeclaration, scope violation, type
   mismatch, invalid assignment/expression), each with a line number.
4. If semantic analysis passed: the unoptimized TAC (quadruple table), then the optimized
   TAC after constant folding + dead-code elimination.

Try it against every provided test case:

```bash
for f in tests/valid/*.txt tests/invalid/*.txt; do echo "== $f =="; ./compiler "$f"; echo; done
```

Each `tests/{valid,invalid}/*.txt` has a matching `.md` file describing the case and its
expected error category, and a captured reference run under `tests/{valid,invalid}/actual_output/`.

## Design highlights (see `docs/DESIGN_NOTES.md` for the full writeup)

- **Three independent passes over one AST**, not one-pass evaluation: the parser (`parser.y`)
  only builds the tree; `semantic.c` and `codegen.c` are separate tree-walks. This mirrors how
  a real compiler front end is structured and is why semantic analysis can run to completion
  and report *every* error in the program, not just the first one.
- **Nested-scope symbol table** (`symbol_table.c`): a stack of scopes, pushed on block entry
  and popped on block exit, so a variable declared inside `if`/`while` is correctly invisible
  once that block ends (see `tests/invalid/scope_violation`).
- **Quadruple-table TAC** with labels and `goto`/`ifFalse` for `if`, `if-else`, and `while`
  (see `docs/GRAMMAR.md` and the sample TAC output in `tests/valid/complete_program.md`).
- **Bonus**: constant folding (during TAC emission) and dead-code elimination (post-pass over
  the quadruple table), each independently toggleable in `codegen.c`.

## Error recovery

- **Lexical**: an unmatched character is reported with its line number; the scanner discards
  it and continues, so multiple lexical errors in one file are all reported.
- **Syntax**: `parser.y` uses Bison's `error` token to resynchronize at the next `;`, so one
  syntax error doesn't necessarily stop the whole file from being checked.
- **Semantic**: every statement is checked; errors accumulate and are all printed together.

## AI Usage Disclosure

Per the manual's AI Usage Policy (Section 10), this project's implementation was developed
with the assistance of Claude (Anthropic). All group members reviewed and understand every
module and are prepared to explain any part of the implementation during the individual viva.

## Git Workflow

See the commit history for evidence of ongoing, distributed contribution from all three group
members (not one massive last-minute commit) — required by Section 9 of the manual.

## License

See `LICENSE`.
