#include "lexer.h"
#include "parse.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <unordered_map>

class Scope {
private:
  std::unordered_map<std::string, size_t> m_vars;

public:
  Scope() : m_vars() {}

  size_t get_var_addr(std::string &var) const { return m_vars.at(var); }
  void set_var_addr(std::string var, size_t addr) { m_vars[var] = addr; }
};

class Compiler {};

int main(int argc, char **argv) {

  std::istream *in = &std::cin;
  std::ostream *out = &std::cout;
  std::ifstream infile;
  std::ofstream outfile;

  if (argc >= 2 && std::string(argv[1]) != "-") {
    infile.open(argv[1]);
    if (!infile) {
      std::cerr << "error: cannot open input file '" << argv[1] << "'\n";
      return 1;
    }
    in = &infile;
  }

  if (argc >= 3 && std::string(argv[2]) != "-") {
    outfile.open(argv[2]);
    if (!outfile) {
      std::cerr << "error: cannot open output file '" << argv[2] << "'\n";
      return 1;
    }
    out = &outfile;
  }

  Lexer lexer{};

  std::string line;

  while (std::getline(*in, line)) {
    lexer.feed(std::move(line));
  }

  std::vector<Token> token_stream = lexer.empty();

  Parser parser{token_stream};

  ExprPtr expr = parser.parse_expression();

  expr->display();

  StmtPtr stmt = parser.parse_assign();

  stmt->display();

  return 0;
}
