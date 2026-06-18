#include "ast.h"

#include <iostream>

Expr::Expr(int line_number) : line_number{line_number} {}
Stmt::Stmt(int line_number) : line_number{line_number} {}

VarExpr::VarExpr(std::string name, int line_number)
    : Expr(line_number), name{std::move(name)} {}
void VarExpr::display() const { std::cout << "VarExpr (" << name << ")"; }
ExprType VarExpr::get_type() const { return ExprType::VAR; }

ArrayVarExpr::ArrayVarExpr(std::string name, ExprPtr index_expr,
                           int line_number)
    : Expr(line_number), name{std::move(name)},
      index_expr{std::move(index_expr)} {}
void ArrayVarExpr::display() const {
  std::cout << "ArrayVarExpr (" << name << ": ";
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

AssignStmt::AssignStmt(std::string name, ExprPtr val, AssignType type,
                       int line_number)
    : Stmt(line_number), name{std::move(name)}, val{std::move(val)},
      type{type} {}
void AssignStmt::display() const {
  std::cout << "AssignStmt (" << name << " ";
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
                                 int line_number)
    : Stmt(line_number), name{std::move(name)},
      size_expr{std::move(size_expr)} {}
void CreateArrayStmt::display() const {
  std::cout << "CreateArrayStmt (";
  size_expr->display();
  std::cout << ")";
}

AssignArrayStmt::AssignArrayStmt(std::string name, ExprPtr index_expr,
                                 ExprPtr target, int line_number)
    : Stmt(line_number), name{std::move(name)},
      index_expr{std::move(index_expr)}, target{std::move(target)} {}
void AssignArrayStmt::display() const {
  std::cout << "AssignArrayStmt (";
  index_expr->display();
  std::cout << " ";
  target->display();
  std::cout << ")";
}
