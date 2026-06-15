#include "parser.h"
#include "ast.h"
#include "compexcept.h"
#include "lexer.h"

#include <memory>

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

  const Token &curr_token = peek();
  throw CompilerException(curr_token.get_line(), std::move(on_fail));
}

Token &Parser::expect_type(const TokenType &type, std::string on_fail) {
  if (check_type(type)) {
    return m_stream[m_pointer++];
  }

  const Token &token = peek();
  throw CompilerException(token.get_line(), std::move(on_fail));
}

Token &Parser::advance() { return m_stream[m_pointer++]; }

ExprPtr Parser::parse_expression() {
  ExprPtr left = parse_additive();

  while (check_type(TokenType::EQ) || check_type(TokenType::NEQ) ||
         check_type(TokenType::LT) || check_type(TokenType::GT) ||
         check_type(TokenType::LEQ) || check_type(TokenType::GEQ)) {
    Token &token = advance();
    std::string op = token.get_val();
    ExprPtr right = parse_additive();
    left = std::make_unique<BinaryExpr>(std::move(op), std::move(left),
                                        std::move(right), token.get_line());
  }

  return left;
}

ExprPtr Parser::parse_additive() {
  ExprPtr left = parse_term();

  while (check_type(TokenType::ADD) || check_type(TokenType::SUB)) {
    Token &token = advance();
    std::string op = token.get_val();
    ExprPtr right = parse_term();
    left = std::make_unique<BinaryExpr>(std::move(op), std::move(left),
                                        std::move(right), token.get_line());
  }

  return left;
}

ExprPtr Parser::parse_term() {
  ExprPtr left = parse_unary();

  while (check_type(TokenType::MUL) || check_type(TokenType::DIV) ||
         check_type(TokenType::MOD)) {
    Token &token = advance();
    std::string op = token.get_val();
    ExprPtr right = parse_unary();
    left = std::make_unique<BinaryExpr>(std::move(op), std::move(left),
                                        std::move(right), token.get_line());
  }

  return left;
}

ExprPtr Parser::parse_unary() {
  if (check_type(TokenType::NOT)) {
    Token &token = advance();
    std::string op = token.get_val();
    ExprPtr operand = parse_unary();
    return std::make_unique<UnaryExpr>(std::move(op), std::move(operand),
                                       token.get_line());
  }

  return parse_factor();
}

ExprPtr Parser::parse_factor() {
  const Token &token = peek();
  if (check_type(TokenType::NUMBER)) {
    return std::make_unique<NumberExpr>(advance().get_val(), token.get_line());
  }
  if (check_type(TokenType::STRING)) {
    return std::make_unique<StringExpr>(advance().get_val(), token.get_line());
  }
  if (check_type(TokenType::IDENT)) {
    return std::make_unique<VarExpr>(advance().get_val(), token.get_line());
  }
  if (check_type(TokenType::LPAREN)) {
    advance(); // eat "("
    ExprPtr inner = parse_expression();
    expect_type(TokenType::RPAREN, "expected ')'");
    return inner;
  }

  throw CompilerException(token.get_line(), "expected an expression");
}

StmtPtr Parser::parse_assign(AssignStmt::AssignType type) {
  Token &ident = expect_type(TokenType::IDENT, "Can't assign non ident");

  expect_type(TokenType::EQUALS, "No matching equals sign for assignment");

  ExprPtr expr = parse_expression();

  expect_type(TokenType::SEMICOLON, "missing semicolon");

  return std::make_unique<AssignStmt>(ident.get_val(), std::move(expr), type,
                                      ident.get_line());
}

StmtPtr Parser::parse_loop() {
  expect_type(TokenType::LOOP, "failed loop token");
  expect_type(TokenType::LPAREN, "loop conditions must be in parentheses");

  ExprPtr cond = parse_expression();

  expect_type(TokenType::RPAREN, "loop conditions must be in parenthases");

  expect_type(TokenType::LBRACE, "loop has no opening code block");

  std::vector<StmtPtr> body = parse_program();

  return std::make_unique<LoopStmt>(std::move(cond), std::move(body),
                                    cond->line_number);
}

StmtPtr Parser::parse_if() {

  expect_type(TokenType::IF, "failed if token");
  expect_type(TokenType::LPAREN, "if conditions must be in parentheses");

  ExprPtr cond = parse_expression();

  expect_type(TokenType::RPAREN, "if conditions must be in parenthases");

  expect_type(TokenType::LBRACE, "if has no opening code block");

  std::vector<StmtPtr> body = parse_program();

  return std::make_unique<IfStmt>(std::move(cond), std::move(body),
                                  cond->line_number);
}

StmtPtr Parser::parse_print_str() {
  expect_type(TokenType::PRINT_STR, "failed print_str token");
  expect_type(TokenType::LPAREN, "built-in function requires open paren");

  ExprPtr target = parse_expression();

  expect_type(TokenType::RPAREN, "print_str statement not closed");
  expect_type(TokenType::SEMICOLON, "missing semicolon");

  return std::make_unique<PrintStrStmt>(std::move(target), target->line_number);
}

StmtPtr Parser::parse_print_val() {
  expect_type(TokenType::PRINT_VAL, "failed print_val token");
  expect_type(TokenType::LPAREN, "built-in function requires open paren");

  ExprPtr target = parse_expression();

  expect_type(TokenType::RPAREN, "print_val statement not closed");
  expect_type(TokenType::SEMICOLON, "missing semicolon");

  return std::make_unique<PrintValStmt>(std::move(target), target->line_number);
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
      throw CompilerException(curr.get_line(), "token not implemented");
    }
  }

  return program;
}
