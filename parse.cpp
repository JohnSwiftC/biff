#include "parse.h"
#include "lexer.h"

#include <iostream>
#include <stdexcept>

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

Parser::Parser(std::vector<Token> stream)
    : m_stream{std::move(stream)}, m_pointer{0}, m_size{m_stream.size()} {}

bool Parser::check(const Token &token) const {
  return (token.get_type() == m_stream[m_pointer].get_type() &&
          token.get_val() == m_stream[m_pointer].get_val());
}

bool Parser::check_type(const TokenType &type) const {
  if (at_end()) {
    return false;
  }
  return (type == m_stream[m_pointer].get_type());
}

bool Parser::at_end() const { return m_pointer >= m_size; }

const Token &Parser::peek() const { return m_stream[m_pointer]; }

Token &Parser::expect(const Token &token, std::string on_fail) {
  if (check(token)) {
    return m_stream[m_pointer++];
  }

  throw std::runtime_error(on_fail.c_str());
}

Token &Parser::expect_type(const TokenType &type, std::string on_fail) {
  if (check_type(type)) {
    return m_stream[m_pointer++];
  }

  throw std::runtime_error(on_fail.c_str());
}

Token &Parser::advance() { return m_stream[m_pointer++]; }

ExprPtr Parser::parse_expression() {
  ExprPtr left = parse_term();

  while (check_type(TokenType::ADD) || check_type(TokenType::SUB)) {
    char op = advance().get_val()[0];
    ExprPtr right = parse_term();
    left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
  }

  return left;
}

ExprPtr Parser::parse_term() {
  ExprPtr left = parse_factor();

  while (check_type(TokenType::MUL) || check_type(TokenType::DIV)) {
    char op = advance().get_val()[0];
    ExprPtr right = parse_factor();
    left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
  }

  return left;
}

ExprPtr Parser::parse_factor() {
  if (check_type(TokenType::NUMBER)) {
    return std::make_unique<NumberExpr>(advance().get_val());
  }
  if (check_type(TokenType::STRING)) {
    return std::make_unique<StringExpr>(advance().get_val());
  }
  if (check_type(TokenType::IDENT)) {
    return std::make_unique<VarExpr>(advance().get_val());
  }
  if (check_type(TokenType::LPAREN)) {
    advance(); // eat "("
    ExprPtr inner = parse_expression();
    expect_type(TokenType::RPAREN, "expected ')'");
    return inner;
  }

  throw std::runtime_error("expected an expression");
}
