#include "parse.h"

#include <iostream>

VarExpr::VarExpr(std::string name) : name{std::move(name)} {}
void VarExpr::display() const { std::cout << "VarExpr (" << name << ")"; }

NumberExpr::NumberExpr(std::string val) : val{std::move(val)} {}
void NumberExpr::display() const { std::cout << "NumberExpr (" << val << ")"; }

StringExpr::StringExpr(std::string val) : val{val} {}
void StringExpr::display() const { std::cout << "StringExpr (" << val << ")"; }

BinaryExpr::BinaryExpr(char op, ExprPtr left, ExprPtr right)
    : op{op}, left{std::move(left)}, right{std::move(right)} {}
void BinaryExpr::display() const { std::cout << "BinaryExpr (" << op << ")"; }

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
