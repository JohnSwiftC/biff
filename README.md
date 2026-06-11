# biff

A language that compiles to brainfuck.

> This is skipping over how the IR language is implemented, as its implementation is much simpler than the compiler. I'll write it later.

# Biff -> IR Overview

## The Lexer

Biff uses a simple, white-space delimited lexer (with the exception of punctuation characters).

`Tokens` and `TokenTypes` are defined in `lexer.h` and `lexer.cpp`. The lexer is fed an input file line by line, and dumps its full token stream with `emtpy()`, which can be seen in `biffc.cpp`.

## Parsing

Biff's AST defines two main types: `Stmt` and `Expr`, statements and expressions respectively. These two virtual classes are implemented by different types of statements and expressions in `ast.h`, `ast.cpp`, and `codegen.cpp`.

> `codegen.cpp` specifically defines the implemented `generate` function on derived `Stmt` classes.

In `biffc.cpp`, the `Parser` class is fed the raw token stream produced from the lexer, and produces an array of `StmtPtr`s which can then be used the `Compiler` class for code generation.

Tokens are parsed in chunks, how so depends on which individual parsing function is fired. This is almost always decided by the type of token that the parser is currently pointing to in the token stream. Tokens of type `LOOP` instantly allow the parser to know it should invoke `parse_loop()`, which will create a `LoopStmt`, the same occurs for deciding when to parse tokens as an if statement, an assignment, a built-in function call, etc.

### Compound Expressions

Expressions are tied together with operators and the `BinaryExpr` type to build expression trees for compound expressions. The parser's `parse_expression()` function builds an expression tree from the token stream which correctly follows the order of operations. `parse_additive()` and `parse_factor()` are how the order of operations are maintained along with optional parentheses.

> The specific implementation can be found in `parser.cpp`

## CodeGen

The final step in the pipeline, moving from a vector of `StmtPtr` to outputted IR.

Each `Stmt` derived class implements the virtual `generate(std::ostream* out, Compiler* compiler)` function. These are defined for each `Stmt` in `codegen.cpp`. `Compiler` is provided the output vector from the parser, and iteratively calls generate on each of the statements it is given to compile the full program.

### The Compiler Class and Scope

In `compiler.h` and `compiler.cpp`, the `Compiler` class is created, along with the `Scope` class. These classes are responsible for keeping track of variable scopes and memory allocations that may be needed when doing codegen for any of the statements within the program.

For example, when generating IR for assignment statements, the provided variable name is checked with the current scope known within the compiler. If a pre-existing variable is found, that address is the target of the assignment. If not, the next usable address is bumped, and a new variable name and address are added to the compiler's scope.

In `biffc.cpp`, `Compiler::generate_program(std::ostream* out)` is used to finally emit IR to an output stream.
