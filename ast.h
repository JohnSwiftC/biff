#ifndef AST_H
#define AST_H

#include <memory>
#include <ostream>
#include <string>
#include <vector>

class Compiler;

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

struct AssignStmt : Stmt {
  enum class AssignType {
    NEW,
    SET,
  };

  std::string name;
  ExprPtr val;
  AssignType type;

  AssignStmt(std::string name, ExprPtr val, AssignType type);

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

struct PrintStrStmt : Stmt {
  ExprPtr target;

  PrintStrStmt(ExprPtr target);

  void display() const override;
  void generate(std::ostream *out, Compiler *compuler) override;
};

struct PrintValStmt : Stmt {
  ExprPtr target;

  PrintValStmt(ExprPtr target);

  void display() const override;
  void generate(std::ostream *out, Compiler *compuler) override;
};

#endif
