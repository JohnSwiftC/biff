#include "lexer.h"

#include <fstream>
#include <iostream>

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

  for (const Token &token : token_stream) {
    *out << token << '\n';
  }

  return 0;
}
