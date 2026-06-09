#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "biffc.h"

class Token;
enum class TokenType;

enum class ExprType {
  VAR,
  NUMBER,
  STRING,
  BINARY,
};

struct Expr {
  virtual ~Expr() = default;

  virtual void display() const = 0;
  virtual ExprType get_type() const = 0;
};

struct Stmt {
  virtual ~Stmt() = default;

  virtual void display() const = 0;
  virtual void generate(std::ostream *out, Compiler *compiler) = 0;
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct VarExpr : Expr {
  std::string name;

  VarExpr(std::string val);

  void display() const override;
  ExprType get_type() const override;
};

struct NumberExpr : Expr {
  std::string val;

  NumberExpr(std::string val);

  void display() const override;
  ExprType get_type() const override;
};

struct StringExpr : Expr {
  std::string val;

  StringExpr(std::string val);

  void display() const override;
  ExprType get_type() const override;
};

struct BinaryExpr : Expr {
  std::string op;
  ExprPtr left;
  ExprPtr right;

  BinaryExpr(std::string op, ExprPtr left, ExprPtr right);

  void display() const override;
  ExprType get_type() const override;
};

// These are used when evaluating the
// expression tree recursively
enum class EvalType {
  // The address of a named variable's cell. Read-only:
  // it must never be operated on in place.
  ADDRESS,
  CONST,
  // A scratch cell owned by the evaluator, safe to
  // accumulate into in place.
  TEMP,
};

struct EvalResult {
  EvalType type;
  size_t val;
};

// Recursively evaluates an expression tree, emitting any code
// needed to compute it. Temps it allocates are reclaimed by the
// caller with Scope::set_next_free once the result is consumed
EvalResult eval(std::ostream *out, Compiler *compiler, Expr *expr);

struct AssignStmt : Stmt {
  std::string name;
  ExprPtr val;

  AssignStmt(std::string name, ExprPtr val);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

struct LoopStmt : Stmt {
  ExprPtr cond;
  std::vector<StmtPtr> body;

  LoopStmt(ExprPtr cond, std::vector<StmtPtr> body);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

struct IfStmt : Stmt {
  ExprPtr cond;
  std::vector<StmtPtr> body;

  IfStmt(ExprPtr cond, std::vector<StmtPtr> body);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

class Parser {
private:
  std::vector<Token> m_stream;
  size_t m_pointer;
  size_t m_size;

  bool check(const Token &token) const;
  bool check_type(const TokenType &type) const;
  bool at_end() const;

  const Token &peek() const;
  // expect also advances, but only
  // if the token at ptr matches the input token
  // throws an exception if its bad, used to handle
  // error paths
  Token &expect(const Token &token, std::string on_fail);
  Token &expect_type(const TokenType &type, std::string on_fail);
  Token &advance();

  ExprPtr parse_term();
  ExprPtr parse_factor();

  ExprPtr parse_additive();
  ExprPtr parse_expression();
  StmtPtr parse_assign();
  StmtPtr parse_loop();
  StmtPtr parse_if();

public:
  Parser(std::vector<Token> stream);

  std::vector<StmtPtr> parse_program();
};

#endif
