#include "lexer.h"
#include <cctype>
#include <ostream>
#include <string_view>

Lexer::Lexer() : m_token_stream{} {}

std::ostream &operator<<(std::ostream &out, const Token &in) {
  switch (in.m_type) {
  case TokenType::ADD:
    out << "ADD (" << in.m_val << ')';
    break;
  case TokenType::SUB:
    out << "SUB (" << in.m_val << ')';
    break;
  case TokenType::MUL:
    out << "MUL (" << in.m_val << ')';
    break;
  case TokenType::DIV:
    out << "DIV (" << in.m_val << ')';
    break;
  case TokenType::EQUALS:
    out << "EQUALS (" << in.m_val << ')';
    break;
  case TokenType::IDENT:
    out << "IDENT (" << in.m_val << ')';
    break;
  case TokenType::STRING:
    out << "STRING (" << in.m_val << ')';
    break;
  case TokenType::NUMBER:
    out << "NUMBER (" << in.m_val << ')';
    break;
  }

  return out;
}

bool Lexer::is_numeric(std::string_view word) {
  return (word[0] >= 48 && word[0] <= 57);
}

TokenType Lexer::parse_word(std::string_view word) {
  if (word[0] == '"') {
    return TokenType::STRING;
  }

  if (word == "=") {
    return TokenType::EQUALS;
  } else if (word == "+") {
    return TokenType::ADD;
  } else if (word == "-") {
    return TokenType::SUB;
  } else if (word == "*") {
    return TokenType::MUL;
  } else if (word == "/") {
    return TokenType::DIV;
  }

  if (is_numeric(word)) {
    return TokenType::NUMBER;
  }

  return TokenType::IDENT;
}

void Lexer::feed(std::string line) {
  std::size_t i = 0;
  while (i < line.length()) {
    if (std::isspace(static_cast<unsigned char>(line[i]))) {
      ++i;
      continue;
    }

    std::string word;
    if (line[i] == '"') {
      word += line[i++];
      while (i < line.length() && line[i] != '"') {
        word += line[i++];
      }
      if (i < line.length()) {
        word += line[i++];
      }
    } else {
      while (i < line.length() &&
             !std::isspace(static_cast<unsigned char>(line[i]))) {
        word += line[i++];
      }
    }

    TokenType type{parse_word(word)};
    m_token_stream.emplace_back(type, std::move(word));
  }
}

std::vector<Token> Lexer::empty() { return std::move(m_token_stream); }
