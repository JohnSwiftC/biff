# biff

A language that compiles to brainfuck.

# Guide

## Variables and assignment

Variables are declared within their scope with the `let` keyword. Variables can be reassigned with just the identifier.

```rust
let a = 12;
a = a + 1;
```

By default, variables are an unsigned 8 bit integer. Strings are defined in the same fashion, however, strings are immutable once declared.

```rust
let hello = "Hello, World!";
hello = "Hi!"; // compiler error
```


As seen later, variables can also be given an explicit type in most cases,

```rust
let my_integer: Integer = 12;
```

## Arrays

Arrays consist of 8 bit integers as well, and the array's size must be able to be evaluated at compile time. Arrays can then be dynamically accessed.

> Note: a string is is also an array.

> Note: currently, biff only supports Integer array types

```rust
let my_array = [100]; // an array of size 100
let my_array_two = [25 * 4]; // also an array of size 100

my_array[1] = 2;
my_array_two[my_array[1]] = 4;
```

## Structs and Types

biff has two built-in types: `Integer` and `[size]`, which represents a sized `Integer` array.

```rust
struct MyStruct {
  Integer a, // integer typed field
  [50] b, // integer array field of size 50
}

struct Composed {
  MyStruct a, // field of type MyStruct
}

// Structs when defined must be set to 0. Otherwise results in
// a compiler error
let instance: Composed = 0;
```

Structs are accessed the same way they would be in any C-like language, with the dot syntax.

## Arithmetic and Comparison Operators

Biff supports all of the normal operators you would find in a standard language: `+`, `-`, `*`, `/`, and `%`. Notable behavior exists within the logical operators, however. Operators like `<`, `<=`, `>`, `>=`, `==`, and `!=` will either evaluate to 1 or 0. The unary `!` operator will flip 0 to 1, and any non-zero value to 0. (This is due to control flow, as shown later)

```rust
let a = 12;
let b = 1;

let comp = a >= !b;
let math = (a + b) * (1 + b);
```

As shown, expressions can also be grouped with parentheses. Also note that these operators must be separated by white-space, with the exceptions being grouping symbols like `(` and `)`, or unary operators, like `!`.

## If Statements

If statements can be evaluated against any expression, including sole variables. Note, when evaluating against a variable alone, an if statement will not modify it (see loops, which DO modify single variable expressions).

If statements fire only once, and only if the expression evaluates to any non-zero value.

```rust
let a = 2;

if (a > 1) {
  // do something
}
```

## Loops

Loops are very similar to if statements, as they can take any valid expression as an argument. They will execute the loop body until the given expression is 0. A special case exists when a loop operates on a single address expression. In this case, that variable will be modified as part of the loop.

```rust
loop (100) {
  // loops 100 times, but have no access to the underlying iteration
}

let a = 13;

loop(a) {
  // loops 13, times, however, accessing a will reveal the current
  // number of remaining iterations.
}

loop (a * 2) {
  // loops 26 times. note, however, because a * 2 is more
  // than a single variable expression, a will not be modified by this loop
}
```

## Scoping and Variable Shadowing

If and loop statements both create their own scope. In these statements, outer variables can still be accessed and modified. Declaring a new variable in the body will shadow any outside variables of the same name. Once outside of an if or loop body, variables created within them will no longer be accessible.

## Built-In Functions

Biff has two built-in functions, `print_str` and `print_val`. `print_str` takes a single string argument, and may not be anything besides a single variable argument.

> Note: print_str should only be used with variables that have been defined as a string literal. Anything otherwise is UB and will almost definitely destroy your program.

`print_val` can take any valid expression as an argument, and it will display the value as a string (think itoa).

## Examples

Examples can be found in /examples, have at it!

## Compiling and Tools

> The entire project can be built with cmake.

Biff's compiler pipeline is split into three different components: `biffc`, `ircompile`, and `bfopt`.

`biffc` compiles biff into its matching IR. `ircompile` compiles IR into its matching brainfuck, and `bfopt` optimizes emitted brainfuck to shrink the final size of your program. A typical full build looks something like this.

`./biffc program.biff | ./ircompile | ./bfopt - program.bf`

Note the leading `-` in `bfopt`'s args. Without this, bfopt (and every other component as well), assumes it should emit output to stdout. '-' and a file name explicitly tells it to emit to a new file.

There is also an included interpreter implementation which is also built in the cmake file for use.

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
