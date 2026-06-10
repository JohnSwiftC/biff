#include "parse.h"
#include "biffc.h"
#include "lexer.h"

#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
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

// This function and expression evaluation are pretty nasty so i think it
// deserves an explaination. This recursively searches through the expression
// tree and returns an EvalResult at each level. CONST means that both branches
// could be done by the compiler and have been smashed at compile time.
//
// The important bits are ADDRESS and TEMP. ADDRESS refers to a variable address
// which must be maintained. These are only returned when evaluating an Expr of
// derived type VarExpr, and when this function runs on a BinaryExpr node, if a
// type ADDRESS is returned, it creates and allocates a new byte of type TEMP.
// This notes that the address can be written to and modified, and this is where
// operations accumulate into a new cell.
//
// This, of course, results in a possible junk cell. Look at the AssignStmt's
// generate function to see how the final return value of this function can be
// handled in all of these cases.
//
// Also, for any expression, there will be no temp cells left after the
// expression is fully evaluated. Notice how when a TEMP address is used on the
// right hand side, it is zeroed and the memory it was using is freed in the
// scope
EvalResult eval(std::ostream *out, Compiler *compiler, Expr *expr) {
  if (expr->get_type() == ExprType::STRING) {
    throw std::runtime_error("illegal string literal in compound expression");
  }

  if (expr->get_type() == ExprType::NUMBER) {
    NumberExpr *num_expr = static_cast<NumberExpr *>(expr);
    std::stringstream stream(num_expr->val);
    size_t val;
    stream >> val;
    return EvalResult{EvalType::CONST, val};
  }

  if (expr->get_type() == ExprType::VAR) {
    VarExpr *var_expr = static_cast<VarExpr *>(expr);
    size_t addr{};

    if (compiler->get_scope().contains_var(var_expr->name)) {
      addr = compiler->get_scope().get_var_addr(var_expr->name);
    } else {
      throw std::runtime_error("var " + var_expr->name +
                               " is not defined in scope");
    }

    return EvalResult{EvalType::ADDRESS, addr};
  }

  if (expr->get_type() != ExprType::BINARY) {
    throw std::runtime_error("illegal expression tree configuration");
  }

  BinaryExpr *bin_expr = static_cast<BinaryExpr *>(expr);
  Scope &scope = compiler->get_scope();

  EvalResult left = eval(out, compiler, bin_expr->left.get());
  EvalResult right = eval(out, compiler, bin_expr->right.get());

  if (left.type == EvalType::CONST && right.type == EvalType::CONST) {
    if (bin_expr->op == "+") {
      return EvalResult{EvalType::CONST, left.val + right.val};
    }

    if (bin_expr->op == "-") {
      return EvalResult{EvalType::CONST, left.val - right.val};
    }

    if (bin_expr->op == "*") {
      return EvalResult{EvalType::CONST, left.val * right.val};
    }

    if (bin_expr->op == "/") {
      return EvalResult{EvalType::CONST, left.val / right.val};
    }

    if (bin_expr->op == "%") {
      return EvalResult{EvalType::CONST, left.val % right.val};
    }

    if (bin_expr->op == "==") {
      return EvalResult{EvalType::CONST, left.val == right.val};
    }

    if (bin_expr->op == "!=") {
      return EvalResult{EvalType::CONST, left.val != right.val};
    }

    if (bin_expr->op == "<") {
      return EvalResult{EvalType::CONST, left.val < right.val};
    }

    if (bin_expr->op == ">") {
      return EvalResult{EvalType::CONST, left.val > right.val};
    }

    throw std::runtime_error("unaccounted operator in const eval: " +
                             bin_expr->op);
  }

  // At least one side lives on the tape. Get the left operand into
  // a temp cell we own, then fold the right operand into it in place.
  // ADD/SUB preserve src, and MUL/DIV/MOD zero their dumps, so
  // variables and cells above next_free stay intact.
  size_t acc{};
  if (left.type == EvalType::TEMP) {
    acc = left.val;
  } else {
    acc = scope.get_next_free();
    scope.bump_next_free(1);

    if (left.type == EvalType::CONST) {
      *out << "MOV: " << acc << ", " << left.val << '\n';
    } else {
      *out << "MOV: " << acc << ", 0\n";
      *out << "ADD: " << acc << ", " << left.val << '\n';
    }
  }

  // Cells at next_free and above are zero, so they can
  // serve as the dump scratch the ops require
  size_t dump = scope.get_next_free();

  if (right.type == EvalType::CONST) {
    if (bin_expr->op == "+") {
      *out << "ADD_CONST: " << acc << ", " << right.val << '\n';
    } else if (bin_expr->op == "-") {
      *out << "SUB_CONST: " << acc << ", " << right.val << '\n';
    } else if (bin_expr->op == "*") {
      *out << "MUL_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
    } else if (bin_expr->op == "/") {
      *out << "DIV_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
    } else if (bin_expr->op == "%") {
      *out << "MOD_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
    } else {
      throw std::runtime_error("unaccounted operator in eval: " + bin_expr->op);
    }
  } else {
    if (bin_expr->op == "+") {
      *out << "ADD: " << acc << ", " << right.val << '\n';
    } else if (bin_expr->op == "-") {
      *out << "SUB: " << acc << ", " << right.val << '\n';
    } else if (bin_expr->op == "*") {
      *out << "MUL: " << acc << ", " << right.val << ", " << dump << '\n';
    } else if (bin_expr->op == "/") {
      *out << "DIV: " << acc << ", " << right.val << ", " << dump << '\n';
    } else if (bin_expr->op == "%") {
      *out << "MOD: " << acc << ", " << right.val << ", " << dump << '\n';
    } else {
      throw std::runtime_error("unaccounted operator in eval: " + bin_expr->op);
    }
  }

  if (right.type == EvalType::TEMP) {
    // Release the right-hand temp. It must be zeroed so it can
    // later be reused as dump scratch
    *out << "MOV: " << right.val << ", 0\n";
    if (right.val + 1 == scope.get_next_free()) {
      scope.set_next_free(right.val);
    }
  }

  return EvalResult{EvalType::TEMP, acc};
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

  size_t snapshot = scope.get_next_free();
  EvalResult result = eval(out, compiler, val.get());

  switch (result.type) {
  case EvalType::CONST: {
    size_t dest{};
    if (scope.contains_var(name)) {
      dest = scope.get_var_addr(name);
    } else {
      dest = scope.get_next_free();
      scope.set_var_addr(name, dest);
      scope.bump_next_free(1);
    }

    *out << "MOV: " << dest << ", " << result.val << '\n';
    break;
  }

  case EvalType::ADDRESS: {
    size_t dest{};
    if (scope.contains_var(name)) {
      dest = scope.get_var_addr(name);
    } else {
      dest = scope.get_next_free();
      scope.set_var_addr(name, dest);
      scope.bump_next_free(1);
    }

    if (dest == result.val) {
      // x = x;
      break;
    }

    *out << "MOV: " << dest << ", 0\n";
    *out << "ADD: " << dest << ", " << result.val << '\n';
    break;
  }

  case EvalType::TEMP: {
    if (!scope.contains_var(name)) {
      // The address the temp was accumulated in is already in the newest
      // possible position, so any moving is redundant
      if (result.val == snapshot) {
        scope.set_var_addr(name, result.val);
        scope.set_next_free(result.val + 1);
        break;
      }

      *out << "ADD: " << snapshot << ", " << result.val << '\n';
      *out << "MOV: " << result.val << ", 0\n";
      scope.set_var_addr(name, snapshot);
      scope.set_next_free(snapshot + 1);
      break;
    }

    // As explained in the eval function doc,
    // this handles the case where an existing variable is being reset
    // to a temp value. set dest to 0, add the temp, and importantly,
    // drop the temp value.
    size_t dest = scope.get_var_addr(name);
    *out << "MOV: " << dest << ", 0\n";
    *out << "ADD: " << dest << ", " << result.val << '\n';
    *out << "MOV: " << result.val << ", 0\n";
    scope.set_next_free(snapshot);
    break;
  }
  }
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

  while (check_type(TokenType::MUL) || check_type(TokenType::DIV) ||
         check_type(TokenType::MOD)) {
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
