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
  case TokenType::MOD:
    out << "MOD (" << in.m_val << ')';
    break;
  case TokenType::EQUALS:
    out << "EQUALS (" << in.m_val << ')';
    break;
  case TokenType::EQ:
    out << "EQ (" << in.m_val << ')';
    break;
  case TokenType::NEQ:
    out << "NEQ (" << in.m_val << ')';
    break;
  case TokenType::LT:
    out << "LT (" << in.m_val << ')';
    break;
  case TokenType::GT:
    out << "GT (" << in.m_val << ')';
    break;
  case TokenType::LEQ:
    out << "LEQ (" << in.m_val << ')';
    break;
  case TokenType::GEQ:
    out << "GEQ (" << in.m_val << ')';
    break;
  case TokenType::OR:
    out << "OR";
    break;
  case TokenType::AND:
    out << "AND";
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
  case TokenType::SEMICOLON:
    out << "SEMICOLON (" << in.m_val << ')';
    break;
  case TokenType::COLON:
    out << "COLON (" << in.m_val << ')';
    break;
  case TokenType::LOOP:
    out << "LOOP (" << in.m_val << ')';
    break;
  case TokenType::IF:
    out << "IF (" << in.m_val << ')';
    break;
  case TokenType::NOT:
    out << "NOT (" << in.m_val << ')';
    break;
  case TokenType::LET:
    out << "LET";
    break;
  case TokenType::LBRACE:
    out << "LBRACE (" << in.m_val << ')';
    break;
  case TokenType::RBRACE:
    out << "RBRACE (" << in.m_val << ')';
    break;
  case TokenType::LPAREN:
    out << "LPAREN (" << in.m_val << ')';
    break;
  case TokenType::RPAREN:
    out << "RPAREN (" << in.m_val << ')';
    break;
  case TokenType::LBRACKET:
    out << "LBRACKET (" << in.m_val << ')';
    break;
  case TokenType::RBRACKET:
    out << "RBRACKET (" << in.m_val << ')';
    break;
  case TokenType::PRINT_STR:
    out << "PRINT_STR";
    break;
  case TokenType::PRINT_VAL:
    out << "PRINT_VAL";
    break;
  case TokenType::DEF:
    out << "DEF";
    break;
  case TokenType::RETURN:
    out << "RETURN";
    break;
  case TokenType::STRUCT:
    out << "STRUCT";
    break;
  case TokenType::COMMA:
    out << "COMMA";
    break;
  case TokenType::DOT:
    out << "DOT";
    break;
  case TokenType::TRUE:
    out << "TRUE";
    break;
  case TokenType::FALSE:
    out << "FALSE";
    break;
  case TokenType::READ_CHAR:
    out << "READ_CHAR";
    break;
  case TokenType::CHAR:
    out << "CHAR (" << in.m_val << ")";
    break;
  }

  return out;
}

const TokenType &Token::get_type() const { return m_type; }
const std::string &Token::get_val() const { return m_val; }
std::string Token::take_val() { return std::move(m_val); }
int Token::get_line() const { return m_token_line; }

bool Lexer::is_numeric(std::string_view word) {
  return (word[0] >= 48 && word[0] <= 57);
}

bool Lexer::is_punct(char c) {
  return c == ';' || c == '{' || c == '}' || c == '(' || c == ')' || c == '[' ||
         c == ']' || c == '!' || c == ':' || c == ',' || c == '.';
}

TokenType Lexer::parse_word(std::string_view word) {
  if (word[0] == '"') {
    return TokenType::STRING;
  }

  if (word[0] == '\'') {
    return TokenType::CHAR;
  }

  if (word == ";") {
    return TokenType::SEMICOLON;
  } else if (word == ":") {
    return TokenType::COLON;
  } else if (word == "{") {
    return TokenType::LBRACE;
  } else if (word == "}") {
    return TokenType::RBRACE;
  } else if (word == "(") {
    return TokenType::LPAREN;
  } else if (word == ")") {
    return TokenType::RPAREN;
  } else if (word == "[") {
    return TokenType::LBRACKET;
  } else if (word == "]") {
    return TokenType::RBRACKET;
  } else if (word == "=") {
    return TokenType::EQUALS;
  } else if (word == "==") {
    return TokenType::EQ;
  } else if (word == "!=") {
    return TokenType::NEQ;
  } else if (word == "<") {
    return TokenType::LT;
  } else if (word == ">") {
    return TokenType::GT;
  } else if (word == "<=") {
    return TokenType::LEQ;
  } else if (word == ">=") {
    return TokenType::GEQ;
  } else if (word == "||") {
    return TokenType::OR;
  } else if (word == "&&") {
    return TokenType::AND;
  } else if (word == "+") {
    return TokenType::ADD;
  } else if (word == "-") {
    return TokenType::SUB;
  } else if (word == "*") {
    return TokenType::MUL;
  } else if (word == "/") {
    return TokenType::DIV;
  } else if (word == "%") {
    return TokenType::MOD;
  } else if (word == "loop") {
    return TokenType::LOOP;
  } else if (word == "if") {
    return TokenType::IF;
  } else if (word == "!") {
    return TokenType::NOT;
  } else if (word == "let") {
    return TokenType::LET;
  } else if (word == "print_str") {
    return TokenType::PRINT_STR;
  } else if (word == "print_val") {
    return TokenType::PRINT_VAL;
  } else if (word == "def") {
    return TokenType::DEF;
  } else if (word == "return") {
    return TokenType::RETURN;
  } else if (word == "struct") {
    return TokenType::STRUCT;
  } else if (word == ",") {
    return TokenType::COMMA;
  } else if (word == ".") {
    return TokenType::DOT;
  } else if (word == "true") {
    return TokenType::TRUE;
  } else if (word == "false") {
    return TokenType::FALSE;
  } else if (word == "read_char") {
    return TokenType::READ_CHAR;
  }

  if (is_numeric(word)) {
    return TokenType::NUMBER;
  }

  return TokenType::IDENT;
}

void Lexer::feed(std::string line) {
  line_count++;
  std::size_t i = 0;
  while (i < line.length()) {
    if (std::isspace(static_cast<unsigned char>(line[i]))) {
      ++i;
      continue;
    }

    std::string word;
    if (is_punct(line[i])) {
      word += line[i++];
      // the consequences of writing a bad lexer
      if (word == "!" && i < line.length() && line[i] == '=') {
        word += line[i++];
      }
    } else if (line[i] == '"' || line[i] == '\'') {
      char open_quote = line[i];
      word += line[i++];
      while (i < line.length() && line[i] != open_quote) {
        if (line[i] == '\\' && i + 1 < line.length()) {
          word += line[i++];
        }
        word += line[i++];
      }
      if (i < line.length()) {
        word += line[i++];
      }
    } else {
      while (i < line.length() &&
             !std::isspace(static_cast<unsigned char>(line[i])) &&
             !is_punct(line[i])) {
        word += line[i++];
      }
    }

    TokenType type{parse_word(word)};
    m_token_stream.emplace_back(type, std::move(word), line_count);
  }
}

std::vector<Token> Lexer::empty() { return std::move(m_token_stream); }
