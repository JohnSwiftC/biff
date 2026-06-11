#ifndef LEXER_H
#define LEXER_H

#include <ostream>
#include <sstream>
#include <string>
#include <vector>

enum class TokenType {
  IDENT,
  EQUALS,
  STRING,
  NUMBER,
  MUL,
  DIV,
  ADD,
  SUB,
  MOD,
  EQ,
  NEQ,
  LT,
  GT,
  LEQ,
  GEQ,
  SEMICOLON,
  LOOP,
  IF,
  LBRACE,
  RBRACE,
  LPAREN,
  RPAREN,
  PRINT_STR,
  PRINT_VAL,
};

class Token {
private:
  TokenType m_type;
  std::string m_val;

  friend std::ostream &operator<<(std::ostream &out, const Token &in);

public:
  Token(TokenType type, std::string val) : m_type{type}, m_val{val} {}

  const TokenType &get_type() const;
  const std::string &get_val() const;
};

class Lexer {
private:
  std::vector<Token> m_token_stream;

  bool is_numeric(std::string_view word);
  static bool is_punct(char c);
  TokenType parse_word(std::string_view word);

public:
  Lexer();

  void feed(std::string line);

  std::vector<Token> empty();
};

#endif
