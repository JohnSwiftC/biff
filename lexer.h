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
  OR,
  AND,
  SEMICOLON,
  COLON,
  LOOP,
  IF,
  NOT,
  LET,
  LBRACE,
  RBRACE,
  LPAREN,
  RPAREN,
  LBRACKET,
  RBRACKET,
  PRINT_STR,
  PRINT_VAL,
  DEF,
  STRUCT,
  COMMA,
  DOT,
  TRUE,
  FALSE,
};

class Token {
private:
  TokenType m_type;
  std::string m_val;
  int m_token_line;

  friend std::ostream &operator<<(std::ostream &out, const Token &in);

public:
  Token(TokenType type, std::string val, int token_line)
      : m_type{type}, m_val{val}, m_token_line{token_line} {}

  const TokenType &get_type() const;
  const std::string &get_val() const;
  int get_line() const;
};

class Lexer {
private:
  std::vector<Token> m_token_stream;
  int line_count{};

  bool is_numeric(std::string_view word);
  static bool is_punct(char c);
  TokenType parse_word(std::string_view word);

public:
  Lexer();

  void feed(std::string line);

  std::vector<Token> empty();
};

#endif
