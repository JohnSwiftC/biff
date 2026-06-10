#include "compiler.h"

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
