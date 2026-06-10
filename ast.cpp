#include "ast.h"

#include <iostream>

VarExpr::VarExpr(std::string name) : name{std::move(name)} {}
void VarExpr::display() const { std::cout << "VarExpr (" << name << ")"; }
ExprType VarExpr::get_type() const { return ExprType::VAR; }

NumberExpr::NumberExpr(std::string val) : val{std::move(val)} {}
void NumberExpr::display() const { std::cout << "NumberExpr (" << val << ")"; }
ExprType NumberExpr::get_type() const { return ExprType::NUMBER; }

StringExpr::StringExpr(std::string val) : val{val} {}
void StringExpr::display() const { std::cout << "StringExpr (" << val << ")"; }
ExprType StringExpr::get_type() const { return ExprType::STRING; }

BinaryExpr::BinaryExpr(std::string op, ExprPtr left, ExprPtr right)
    : op{std::move(op)}, left{std::move(left)}, right{std::move(right)} {}
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

AssignStmt::AssignStmt(std::string name, ExprPtr val)
    : name{std::move(name)}, val{std::move(val)} {}
void AssignStmt::display() const {
  std::cout << "AssignStmt (" << name << " ";
  val->display();
  std::cout << ")";
}

LoopStmt::LoopStmt(ExprPtr cond, std::vector<StmtPtr> body)
    : cond{std::move(cond)}, body{std::move(body)} {}
void LoopStmt::display() const {
  std::cout << "LoopStmt (";
  cond->display();
  std::cout << " ";

  for (const StmtPtr &s : body) {
    s->display();
  }

  std::cout << ")";
}

IfStmt::IfStmt(ExprPtr cond, std::vector<StmtPtr> body)
    : cond{std::move(cond)}, body{std::move(body)} {}
void IfStmt::display() const {
  std::cout << "IfStmt (";
  cond->display();
  std::cout << " ";

  for (const StmtPtr &s : body) {
    s->display();
  }

  std::cout << ")";
}
