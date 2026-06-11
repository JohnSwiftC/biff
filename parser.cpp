#include "parser.h"
#include "ast.h"
#include "lexer.h"

#include <memory>
#include <stdexcept>

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

StmtPtr Parser::parse_assign(AssignStmt::AssignType type) {
  Token &ident = expect_type(TokenType::IDENT, "Can't assign non ident");

  expect_type(TokenType::EQUALS, "No matching equals sign for assignment");

  ExprPtr expr = parse_expression();

  expect_type(TokenType::SEMICOLON, "missing semicolon");

  return std::make_unique<AssignStmt>(ident.get_val(), std::move(expr), type);
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

StmtPtr Parser::parse_print_str() {
  expect_type(TokenType::PRINT_STR, "failed print_str token");
  expect_type(TokenType::LPAREN, "built-in function requires open paren");

  ExprPtr target = parse_expression();

  expect_type(TokenType::RPAREN, "print_str statement not closed");
  expect_type(TokenType::SEMICOLON, "missing semicolon");

  return std::make_unique<PrintStrStmt>(std::move(target));
}

StmtPtr Parser::parse_print_val() {
  expect_type(TokenType::PRINT_VAL, "failed print_val token");
  expect_type(TokenType::LPAREN, "built-in function requires open paren");

  ExprPtr target = parse_expression();

  expect_type(TokenType::RPAREN, "print_val statement not closed");
  expect_type(TokenType::SEMICOLON, "missing semicolon");

  return std::make_unique<PrintValStmt>(std::move(target));
}

Parser::Parser(std::vector<Token> stream)
    : m_stream{std::move(stream)}, m_pointer{0}, m_size{m_stream.size()} {}

std::vector<StmtPtr> Parser::parse_program() {
  std::vector<StmtPtr> program;

  while (!at_end()) {
    const Token &curr = peek();

    switch (curr.get_type()) {

    case TokenType::LET:
      advance();
      expect_type(TokenType::IDENT, "no identifier following let keyword");
      program.push_back(parse_assign(AssignStmt::AssignType::NEW));
      break;

    case TokenType::IDENT:
      program.push_back(parse_assign(AssignStmt::AssignType::SET));
      break;

    case TokenType::LOOP:
      program.push_back(parse_loop());
      break;

    case TokenType::IF:
      program.push_back(parse_if());
      break;

    case TokenType::PRINT_STR:
      program.push_back(parse_print_str());
      break;
    case TokenType::PRINT_VAL:
      program.push_back(parse_print_val());
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
