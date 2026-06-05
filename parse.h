#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include <memory>
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

class Parser {
private:
  std::vector<Token> m_stream;
  size_t m_pointer;

  bool check(const TokenType &type) const;
};

#endif
