#include "ast.h"

#include <iostream>

Expr::Expr(int line_number) : line_number{line_number} {}
Stmt::Stmt(int line_number) : line_number{line_number} {}

VarExpr::VarExpr(std::string name, std::vector<std::string> fields,
                 int line_number)
    : Expr(line_number), name{std::move(name)}, fields{std::move(fields)} {}
void VarExpr::display() const { std::cout << "VarExpr (" << name << ")"; }
ExprType VarExpr::get_type() const { return ExprType::VAR; }

ArrayVarExpr::ArrayVarExpr(ExprPtr var_expr, ExprPtr index_expr,
                           int line_number)
    : Expr(line_number), var_expr{std::move(var_expr)},
      index_expr{std::move(index_expr)} {}
void ArrayVarExpr::display() const {
  std::cout << "ArrayVarExpr (";
  var_expr->display();
  std::cout << ": ";
  index_expr->display();
  std::cout << ")";
}
ExprType ArrayVarExpr::get_type() const { return ExprType::ARRAYVAR; }

NumberExpr::NumberExpr(std::string val, int line_number)
    : Expr(line_number), val{std::move(val)} {}
void NumberExpr::display() const { std::cout << "NumberExpr (" << val << ")"; }
ExprType NumberExpr::get_type() const { return ExprType::NUMBER; }

StringExpr::StringExpr(std::string val, int line_number)
    : Expr(line_number), val{val} {}
void StringExpr::display() const { std::cout << "StringExpr (" << val << ")"; }
ExprType StringExpr::get_type() const { return ExprType::STRING; }

BinaryExpr::BinaryExpr(std::string op, ExprPtr left, ExprPtr right,
                       int line_number)
    : Expr(line_number), op{std::move(op)}, left{std::move(left)},
      right{std::move(right)} {}
void BinaryExpr::display() const {
  if (left) {
    std::cout << '(';
    left->display();
  }

  std::cout << " " << op << " ";

  if (right) {
    right->display();
    std::cout << ')';
  }
}
ExprType BinaryExpr::get_type() const { return ExprType::BINARY; }

UnaryExpr::UnaryExpr(std::string op, ExprPtr operand, int line_number)
    : Expr(line_number), op{std::move(op)}, operand{std::move(operand)} {}
void UnaryExpr::display() const {
  std::cout << '(' << op;
  operand->display();
  std::cout << ')';
}
ExprType UnaryExpr::get_type() const { return ExprType::UNARY; }

AssignStmt::AssignStmt(ExprPtr target_var_expr,
                       std::optional<std::string> type_name, ExprPtr val,
                       AssignType assign_type, int line_number)
    : Stmt(line_number), target_var_expr{std::move(target_var_expr)},
      type_name{std::move(type_name)}, val{std::move(val)},
      assign_type{assign_type} {}
void AssignStmt::display() const {
  std::cout << "AssignStmt ( ";
  target_var_expr->display();
  val->display();
  std::cout << ")";
}

LoopStmt::LoopStmt(ExprPtr cond, std::vector<StmtPtr> body, int line_number)
    : Stmt(line_number), cond{std::move(cond)}, body{std::move(body)} {}
void LoopStmt::display() const {
  std::cout << "LoopStmt (";
  cond->display();
  std::cout << " ";

  for (const StmtPtr &s : body) {
    s->display();
  }

  std::cout << ")";
}

IfStmt::IfStmt(ExprPtr cond, std::vector<StmtPtr> body, int line_number)
    : Stmt(line_number), cond{std::move(cond)}, body{std::move(body)} {}
void IfStmt::display() const {
  std::cout << "IfStmt (";
  cond->display();
  std::cout << " ";

  for (const StmtPtr &s : body) {
    s->display();
  }

  std::cout << ")";
}

PrintStrStmt::PrintStrStmt(ExprPtr target, int line_number)
    : Stmt(line_number), target{std::move(target)} {}

void PrintStrStmt::display() const {
  std::cout << "PrintStrStmt (";
  target->display();
  std::cout << ")";
}

PrintValStmt::PrintValStmt(ExprPtr target, int line_number)
    : Stmt(line_number), target{std::move(target)} {}

void PrintValStmt::display() const {
  std::cout << "PrintValStmt (";
  target->display();
  std::cout << ")";
}

CreateArrayStmt::CreateArrayStmt(std::string name, ExprPtr size_expr,
                                 AssignType assign_type, int line_number)
    : Stmt(line_number), name{std::move(name)}, size_expr{std::move(size_expr)},
      assign_type{assign_type} {}
void CreateArrayStmt::display() const {
  std::cout << "CreateArrayStmt (";
  size_expr->display();
  std::cout << ")";
}

AssignArrayStmt::AssignArrayStmt(ExprPtr target_var_expr, ExprPtr index_expr,
                                 ExprPtr target_expr, int line_number)
    : Stmt(line_number), target_var_expr{std::move(target_var_expr)},
      index_expr{std::move(index_expr)}, target_expr{std::move(target_expr)} {}
void AssignArrayStmt::display() const {
  std::cout << "AssignArrayStmt (";
  index_expr->display();
  std::cout << " ";
  target_expr->display();
  std::cout << ")";
}

DefineStructStmt::DefineStructStmt(std::string name, std::vector<Field> fields,
                                   int line_number)
    : Stmt(line_number), name{std::move(name)}, fields{std::move(fields)} {}
void DefineStructStmt::display() const {
  std::cout << "DisplayStructStmt (";
  std::cout << name << ": ";

  for (const Field &field : fields) {
    std::cout << field.name << " " << field.type << ", ";
  }

  std::cout << ")";
}

DefineFunctionStmt::DefineFunctionStmt(std::string name,
                                       std::vector<StmtPtr> body,
                                       int line_number)
    : Stmt(line_number), name{std::move(name)}, body{std::move(body)} {}
void DefineFunctionStmt::display() const {
  std::cout << "DefineFunctionStmt (";
  std::cout << name << ": ";

  for (const StmtPtr &stmt : body) {
    stmt->display();
    std::cout << ", ";
  }

  std::cout << ")";
}
