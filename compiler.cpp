#include "compiler.h"
#include <cstddef>
#include <stdexcept>

Scope::Scope() : m_vars{}, m_next_free{1} {}
Scope::Scope(size_t next_free) : m_vars{}, m_next_free{next_free} {}

size_t Scope::get_var_addr(std::string &var) const { return m_vars.at(var); }
void Scope::set_var_addr(std::string var, size_t addr) { m_vars[var] = addr; }

size_t Scope::get_next_free() const { return m_next_free; }
void Scope::set_next_free(size_t addr) { m_next_free = addr; }
void Scope::bump_next_free(size_t size) { m_next_free += size; }
bool Scope::contains_var(std::string &var) const {
  return (m_vars.count(var) > 0);
}

Compiler::Compiler(std::vector<StmtPtr> program)
    : m_scope_stack{Scope()}, m_program{std::move(program)} {}
Scope &Compiler::get_scope() { return m_scope_stack.back(); }
void Compiler::add_scope() {
  m_scope_stack.emplace_back(m_scope_stack.back().get_next_free());
}
void Compiler::remove_scope() {
  size_t carry{m_scope_stack.back().get_next_free()};
  m_scope_stack.pop_back();
  m_scope_stack.back().set_next_free(carry);
}
void Compiler::generate_program(std::ostream *out) {
  for (StmtPtr &stmt : m_program) {
    stmt->generate(out, this);
  }
}

bool Compiler::contains_var(std::string &name) const {
  for (size_t i{m_scope_stack.size()}; i-- > 0;) {
    if (m_scope_stack[i].contains_var(name)) {
      return true;
    }
  }

  return false;
}

size_t Compiler::get_var(std::string &name) const {
  for (size_t i{m_scope_stack.size()}; i-- > 0;) {
    if (m_scope_stack[i].contains_var(name)) {
      return m_scope_stack[i].get_var_addr(name);
    }
  }

  throw std::runtime_error("var " + name +
                           " is not defined in any compiler scope");
}

void Compiler::set_var(std::string &name, size_t addr) {
  size_t top_scope{m_scope_stack.size() - 1};
  m_scope_stack[top_scope].set_var_addr(name, addr);
}
