#include "parse.h"
#include "biffc.h"
#include "lexer.h"

#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>

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

void AssignStmt::generate(std::ostream *out, Compiler *compiler) {
  ExprType type = val->get_type();
  Scope &scope = compiler->get_scope();
  if (type == ExprType::STRING) {
    if (scope.contains_var(name)) {
      throw std::runtime_error("attempted to redefine string const: " + name);
    }

    StringExpr *string_expr = static_cast<StringExpr *>(val.get());
    size_t string_size = string_expr->val.length();
    size_t dest = scope.get_next_free();
    *out << "INSERT_STRING: " << dest << ", " << string_expr->val << '\n';

    scope.set_var_addr(name, dest);
    scope.bump_next_free(string_size);
    return;
  } else if (type == ExprType::NUMBER) {
    size_t dest{};
    if (scope.contains_var(name)) {
      dest = scope.get_var_addr(name);
    } else {
      dest = scope.get_next_free();
      scope.set_var_addr(name, dest);
      scope.bump_next_free(1);
    }

    NumberExpr *number_expr = static_cast<NumberExpr *>(val.get());

    *out << "MOV: " << dest << ", " << number_expr->val << '\n';
    return;
  }

  throw std::runtime_error("assignment case not implemented yet");
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

void LoopStmt::generate(std::ostream *out, Compiler *compiler) {}

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

void IfStmt::generate(std::ostream *out, Compiler *compiler) {}

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
  ExprPtr left = parse_additive();

  while (check_type(TokenType::EQ) || check_type(TokenType::NEQ) ||
         check_type(TokenType::LT) || check_type(TokenType::GT) ||
         check_type(TokenType::LEQ) || check_type(TokenType::GEQ)) {
    std::string op = advance().get_val();
    ExprPtr right = parse_additive();
    left = std::make_unique<BinaryExpr>(std::move(op), std::move(left),
                                        std::move(right));
  }

  return left;
}

ExprPtr Parser::parse_additive() {
  ExprPtr left = parse_term();

  while (check_type(TokenType::ADD) || check_type(TokenType::SUB)) {
    std::string op = advance().get_val();
    ExprPtr right = parse_term();
    left = std::make_unique<BinaryExpr>(std::move(op), std::move(left),
                                        std::move(right));
  }

  return left;
}

ExprPtr Parser::parse_term() {
  ExprPtr left = parse_factor();

  while (check_type(TokenType::MUL) || check_type(TokenType::DIV)) {
    std::string op = advance().get_val();
    ExprPtr right = parse_factor();
    left = std::make_unique<BinaryExpr>(std::move(op), std::move(left),
                                        std::move(right));
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

StmtPtr Parser::parse_assign() {
  Token &ident = expect_type(TokenType::IDENT, "Can't assign non ident");

  expect_type(TokenType::EQUALS, "No matching equals sign for assignment");

  ExprPtr expr = parse_expression();

  expect_type(TokenType::SEMICOLON, "missing semicolon");

  return std::make_unique<AssignStmt>(ident.get_val(), std::move(expr));
}

StmtPtr Parser::parse_loop() {
  expect_type(TokenType::LOOP, "failed loop token");
  expect_type(TokenType::LPAREN, "loop conditions must be in parentheses");

  ExprPtr cond = parse_expression();

  expect_type(TokenType::RPAREN, "loop conditions must be in parenthases");

  expect_type(TokenType::LBRACE, "loop has no opening code block");

  std::vector<StmtPtr> body = parse_program();

  return std::make_unique<LoopStmt>(std::move(cond), std::move(body));
}

StmtPtr Parser::parse_if() {

  expect_type(TokenType::IF, "failed if token");
  expect_type(TokenType::LPAREN, "if conditions must be in parentheses");

  ExprPtr cond = parse_expression();

  expect_type(TokenType::RPAREN, "if conditions must be in parenthases");

  expect_type(TokenType::LBRACE, "if has no opening code block");

  std::vector<StmtPtr> body = parse_program();

  return std::make_unique<IfStmt>(std::move(cond), std::move(body));
}

std::vector<StmtPtr> Parser::parse_program() {
  std::vector<StmtPtr> program;

  while (!at_end()) {
    const Token &curr = peek();

    switch (curr.get_type()) {
    case TokenType::IDENT:
      program.push_back(parse_assign());
      break;

    case TokenType::LOOP:
      program.push_back(parse_loop());
      break;

    case TokenType::IF:
      program.push_back(parse_if());
      break;
    // This only breaks parsing
    // if a block is illegally used
    case TokenType::RBRACE:
      advance();
      return program;
    default:
      throw std::runtime_error("Not implemented");
    }
  }

  return program;
}
