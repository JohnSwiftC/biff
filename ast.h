#ifndef AST_H
#define AST_H

#include "types.h"
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

class Compiler;

enum class ExprType {
  VAR,
  ARRAYVAR,
  NUMBER,
  STRING,
  BINARY,
  UNARY,
  READ_CHAR,
};

struct Expr {
  int line_number;
  virtual ~Expr() = default;

  virtual void display() const = 0;
  virtual ExprType get_type() const = 0;

  Expr(int line_number);
};

struct Stmt {
  int line_number;
  virtual ~Stmt() = default;

  virtual void display() const = 0;
  virtual void generate(std::ostream *out, Compiler *compiler) = 0;

  Stmt(int line_number);
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// also handles struct field accesses in expressions
struct VarExpr : Expr {
  std::string name;
  std::vector<std::string> fields;

  VarExpr(std::string val, std::vector<std::string> fields, int line_number);

  void display() const override;
  ExprType get_type() const override;
};

struct ArrayVarExpr : Expr {
  ExprPtr var_expr;
  ExprPtr index_expr;

  ArrayVarExpr(ExprPtr var_expr, ExprPtr index_expr, int line_number);

  void display() const override;
  ExprType get_type() const override;
};

struct NumberExpr : Expr {
  std::string val;

  NumberExpr(std::string val, int line_number);

  void display() const override;
  ExprType get_type() const override;
};

struct StringExpr : Expr {
  std::string val;

  StringExpr(std::string val, int line_number);

  void display() const override;
  ExprType get_type() const override;
};

struct BinaryExpr : Expr {
  std::string op;
  ExprPtr left;
  ExprPtr right;

  BinaryExpr(std::string op, ExprPtr left, ExprPtr right, int line_number);

  void display() const override;
  ExprType get_type() const override;
};

struct UnaryExpr : Expr {
  std::string op;
  ExprPtr operand;

  UnaryExpr(std::string op, ExprPtr operand, int line_number);

  void display() const override;
  ExprType get_type() const override;
};

struct ReadCharExpr : Expr {
  ReadCharExpr(int line_number);

  void display() const override;
  ExprType get_type() const override;
};

enum class AssignType {
  NEW,
  SET,
};

struct AssignStmt : Stmt {
  ExprPtr target_var_expr;
  std::optional<std::string> type_name;
  ExprPtr val;
  AssignType assign_type;

  AssignStmt(ExprPtr target_var_expr, std::optional<std::string> type_name,
             ExprPtr val, AssignType assign_type, int line_number);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

struct LoopStmt : Stmt {
  ExprPtr cond;
  std::vector<StmtPtr> body;

  LoopStmt(ExprPtr cond, std::vector<StmtPtr> body, int line_number);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

struct IfStmt : Stmt {
  ExprPtr cond;
  std::vector<StmtPtr> body;

  IfStmt(ExprPtr cond, std::vector<StmtPtr> body, int line_number);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

struct PrintStrStmt : Stmt {
  ExprPtr target;

  PrintStrStmt(ExprPtr target, int line_number);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

struct PrintValStmt : Stmt {
  ExprPtr target;

  PrintValStmt(ExprPtr target, int line_number);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

struct CreateArrayStmt : Stmt {
  std::string name;
  ExprPtr size_expr;
  AssignType assign_type;

  CreateArrayStmt(std::string name, ExprPtr size_expr, AssignType assign_type,
                  int line_number);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

struct AssignArrayStmt : Stmt {
  ExprPtr target_var_expr;
  ExprPtr index_expr;
  ExprPtr target_expr;

  AssignArrayStmt(ExprPtr target_vat_expr, ExprPtr index_expr,
                  ExprPtr target_expr, int line_number);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

struct DefineStructStmt : Stmt {
  struct Field {
    std::string name;
    std::string type;
  };

  std::string name;
  std::vector<Field> fields;

  DefineStructStmt(std::string name, std::vector<Field> fields,
                   int line_number);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

struct DefineFunctionStmt : Stmt {

  struct Argument {
    std::string arg_type;
    std::string arg_name;
  };

  std::string return_type;
  std::string name;
  std::vector<Argument> args;
  std::vector<StmtPtr> body;

  DefineFunctionStmt(std::string return_type, std::string name,
                     std::vector<Argument> args, std::vector<StmtPtr> body,
                     int line_number);

  void display() const override;
  void generate(std::ostream *out, Compiler *compiler) override;
};

#endif
