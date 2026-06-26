#include "parser.h"
#include "ast.h"
#include "compexcept.h"
#include "lexer.h"

#include <memory>
#include <optional>

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

ExprPtr Parser::parse_var_expr() {
  Token &var_name_ident = expect_type(TokenType::IDENT, "expected ident");

  std::string name = var_name_ident.get_val();
  std::vector<std::string> fields;

  while (check_type(TokenType::DOT)) {
    advance();
    Token &field =
        expect_type(TokenType::IDENT, "expected ident following dot operator");
    fields.emplace_back(field.get_val());
  }

  return std::make_unique<VarExpr>(std::move(name), std::move(fields),
                                   var_name_ident.get_line());
}

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
    ExprPtr var_expr = parse_var_expr();
    if (check_type(TokenType::LBRACKET)) {
      advance();
      ExprPtr index_expr = parse_expression();
      expect_type(TokenType::RBRACKET, "missing closing bracket");
      return std::make_unique<ArrayVarExpr>(
          std::move(var_expr), std::move(index_expr), token.get_line());
    }

    return var_expr;
  }
  if (check_type(TokenType::LPAREN)) {
    advance(); // eat "("
    ExprPtr inner = parse_expression();
    expect_type(TokenType::RPAREN, "expected ')'");
    return inner;
  }

  throw CompilerException(token.get_line(), "expected an expression");
}

StmtPtr Parser::parse_assign(AssignType type) {
  Token &ident = expect_type(TokenType::IDENT, "Can't assign non ident");

  if (check_type(TokenType::LBRACKET)) {
    return parse_array_assignment(ident);
  }

  std::optional<std::string> type_name;

  if (!check_type(TokenType::COLON)) {
    type_name = std::nullopt;
  } else {
    type_name = parse_type_string();
  }

  expect_type(TokenType::EQUALS, "No matching equals sign for assignment");

  if (check_type(TokenType::LBRACKET)) {
    // duct tape fix lol, dont want people trying to assign
    // random types to arrays which dont actually do anything,
    // so we just prop up the type and throw a compiler exception
    // if this happens
    return parse_array_creation(ident, type_name);
  }

  ExprPtr expr = parse_expression();

  expect_type(TokenType::SEMICOLON, "missing semicolon");

  return std::make_unique<AssignStmt>(ident.get_val(), std::move(type_name),
                                      std::move(expr), type, ident.get_line());
}

StmtPtr Parser::parse_array_creation(Token &ident,
                                     std::optional<std::string> &type_name) {

  // duct tape escape hatch
  if (type_name) {
    throw CompilerException(ident.get_line(),
                            "arrays cannot be assigned a type");
  }

  expect_type(TokenType::LBRACKET,
              "failing LBRACKET when declaring a new array");

  ExprPtr size_expr = parse_expression();

  expect_type(TokenType::RBRACKET, "no closing RBRACKET on array declaration");
  expect_type(TokenType::SEMICOLON, "missing semicolon");

  return std::make_unique<CreateArrayStmt>(
      ident.get_val(), std::move(size_expr), ident.get_line());
}

StmtPtr Parser::parse_array_assignment(Token &ident) {
  expect_type(TokenType::LBRACKET, "failed LBRACKET in index");

  ExprPtr index_expr = parse_expression();

  expect_type(TokenType::RBRACKET, "expected closing bracket on array index");
  expect_type(TokenType::EQUALS, "expected equals for assignment");

  ExprPtr target = parse_expression();

  expect_type(TokenType::SEMICOLON, "missing semicolon");

  return std::make_unique<AssignArrayStmt>(ident.get_val(),
                                           std::move(index_expr),
                                           std::move(target), ident.get_line());
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

StmtPtr Parser::parse_define_struct() {

  expect_type(TokenType::STRUCT, "broken opening struct block");
  Token &ident =
      expect_type(TokenType::IDENT, "expected identifier after struct keyword");
  expect_type(TokenType::LBRACE, "expected { after struct name");

  using Field = DefineStructStmt::Field;

  std::vector<Field> fields;

  while (!check_type(TokenType::RBRACE)) {
    std::string type_name = parse_type_string();
    Token &field_name_ident = expect_type(
        TokenType::IDENT, "expected field name in struct definition");
    fields.emplace_back(Field{field_name_ident.get_val(), type_name});

    expect_type(TokenType::COMMA, "expected comma following field entry");
  }

  expect_type(TokenType::RBRACE, "missing } after struct definition");

  return std::make_unique<DefineStructStmt>(
      std::move(ident.get_val()), std::move(fields), ident.get_line());
}

std::string Parser::parse_type_string() {

  if (check_type(TokenType::LBRACKET)) {
    std::string result;
    advance();
    result += "[";
    Token &number_val = expect_type(
        TokenType::NUMBER, "expected numeric value following open bracket");
    result += number_val.get_val();
    expect_type(TokenType::RBRACKET, "expected closing bracket");
    result += "]";
    return result;
  }

  Token &ident = expect_type(TokenType::IDENT, "expected ident in type name");
  return ident.get_val();
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
      program.push_back(parse_assign(AssignType::NEW));
      break;

    case TokenType::IDENT:
      program.push_back(parse_assign(AssignType::SET));
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

    case TokenType::STRUCT:
      program.push_back(parse_define_struct());
      break;

      // This only breaks parsing
      // if a block is illegally used
    case TokenType::RBRACE:
      advance();
      return program;
    default:
      throw CompilerException(curr.get_line(),
                              "token not implemented: " + curr.get_val());
    }
  }

  return program;
}
