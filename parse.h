#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include <memory>
#include <string>
#include <vector>

class Token;
class TokenType;

struct Expr {
  virtual ~Expr() = default;

  virtual void display() const = 0;
};

struct Stmt {
  virtual ~Stmt() = default;

  virtual void display() const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct VarExpr : Expr {
  std::string name;

  VarExpr(std::string val);

  void display() const override;
};

struct NumberExpr : Expr {
  std::string val;

  NumberExpr(std::string val);

  void display() const override;
};

struct StringExpr : Expr {
  std::string val;

  StringExpr(std::string val);

  void display() const override;
};

struct BinaryExpr : Expr {
  char op;
  ExprPtr left;
  ExprPtr right;

  BinaryExpr(char op, ExprPtr left, ExprPtr right);

  void display() const override;
};

struct AssignStmt : Stmt {
  std::string name;
  ExprPtr val;

  AssignStmt(std::string name, ExprPtr val);

  void display() const override;
};

struct LoopStmt : Stmt {
  ExprPtr cond;
  std::vector<StmtPtr> body;

  LoopStmt(ExprPtr cond, std::vector<StmtPtr> body);

  void display() const override;
};

class Parser {
private:
  std::vector<Token> m_stream;
  size_t m_pointer;

  bool check(const TokenType &type) const;
};

#endif
