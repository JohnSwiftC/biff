#include "biffc.h"
#include "lexer.h"
#include "parse.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <ostream>
#include <unordered_map>

Scope::Scope() : m_vars{}, m_next_free{1} {}

size_t Scope::get_var_addr(std::string &var) const { return m_vars.at(var); }
void Scope::set_var_addr(std::string var, size_t addr) { m_vars[var] = addr; }

size_t Scope::get_next_free() const { return m_next_free; }
void Scope::set_next_free(size_t addr) { m_next_free = addr; }
void Scope::bump_next_free(size_t size) { m_next_free += size; }
bool Scope::contains_var(std::string &var) { return (m_vars.count(var) > 0); }

Compiler::Compiler(std::vector<StmtPtr> program)
    : m_scope_stack{Scope()}, m_program{std::move(program)} {}
Scope &Compiler::get_scope() { return m_scope_stack.back(); }
void Compiler::add_scope() { m_scope_stack.emplace_back(); }
void Compiler::remove_scope() { m_scope_stack.pop_back(); }
void Compiler::generate_program(std::ostream *out) {
  for (StmtPtr &stmt : m_program) {
    stmt->generate(out, this);
  }
}

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

  std::vector<StmtPtr> program = parser.parse_program();

  Compiler compiler{std::move(program)};

  compiler.generate_program(out);

  return 0;
}
