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
  SEMICOLON,
  LOOP,
  IF,
  LBRACE,
  RBRACE,
  LPAREN,
  RPAREN,
};

class Token {
private:
  TokenType m_type;
  std::string m_val;

  friend std::ostream &operator<<(std::ostream &out, const Token &in);

public:
  Token(TokenType type, std::string val) : m_type{type}, m_val{val} {}
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
