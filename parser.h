#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

#include "ast.h"
#include "lexer.h"

class Parser {
private:
  std::vector<Token> m_stream;
  size_t m_pointer;
  size_t m_size;

  bool check(const Token &token) const;
  bool check_type(const TokenType &type) const;
  bool at_end() const;

  const Token &peek() const;
  // expect also advances, but only
  // if the token at ptr matches the input token
  // throws an exception if its bad, used to handle
  // error paths
  Token &expect(const Token &token, std::string on_fail);
  Token &expect_type(const TokenType &type, std::string on_fail);
  Token &advance();

  ExprPtr parse_var_expr();
  ExprPtr parse_expression();
  ExprPtr parse_additive();
  ExprPtr parse_term();
  ExprPtr parse_unary();
  ExprPtr parse_factor();

  StmtPtr parse_assign(AssignType type);
  StmtPtr parse_array_creation(std::string name, int line_number,
                               std::optional<std::string> &type_name,
                               AssignType assign_type);
  StmtPtr parse_array_assignment(ExprPtr target_val_expr);
  StmtPtr parse_loop();
  StmtPtr parse_if();
  StmtPtr parse_print_str();
  StmtPtr parse_print_val();
  StmtPtr parse_define_struct();

  // Types may contain [] characters, which dont lex
  // into a single ident. this hadles this case and returns the string
  // that represents a type
  std::string parse_type_string();

public:
  Parser(std::vector<Token> stream);

  std::vector<StmtPtr> parse_program();
};

#endif
