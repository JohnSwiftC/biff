#include "compiler.h"
#include "types.h"
#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>

Scope::Scope() : m_vars{}, m_next_free{1} {}
Scope::Scope(size_t next_free) : m_vars{}, m_next_free{next_free} {}

size_t Scope::get_var_addr(std::string &var) const {
  return m_vars.at(var).addr;
}

const Scope::Variable &Scope::get_var(std::string &var) const {
  return m_vars.at(var);
}

void Scope::set_var_addr(std::string var, size_t addr, Type *type) {
  m_vars[var] = Variable{addr, type};
}

void Scope::set_var(std::string name, Scope::Variable var) {
  m_vars[name] = var;
}

size_t Scope::get_next_free() const { return m_next_free; }
void Scope::set_next_free(size_t addr) { m_next_free = addr; }
void Scope::bump_next_free(size_t size) { m_next_free += size; }
bool Scope::contains_var(std::string &var) const {
  return (m_vars.count(var) > 0);
}

Compiler::Compiler(std::vector<StmtPtr> program)
    : m_scope_stack{Scope()}, m_program{std::move(program)}, m_type_pool{},
      functions{} {
  m_type_pool["Integer"] = std::make_unique<IntegerType>();
}
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
  // when a function is being inlined, lookups stop at its frame
  // scope. bodies only see their own params and locals, never
  // whatever happens to be live at the call site
  size_t barrier{m_call_stack.empty() ? 0 : m_call_stack.back().scope_barrier};

  for (size_t i{m_scope_stack.size()}; i-- > barrier;) {
    if (m_scope_stack[i].contains_var(name)) {
      return true;
    }
  }

  return false;
}

const Scope::Variable &Compiler::get_var(std::string &name) const {
  size_t barrier{m_call_stack.empty() ? 0 : m_call_stack.back().scope_barrier};

  for (size_t i{m_scope_stack.size()}; i-- > barrier;) {
    if (m_scope_stack[i].contains_var(name)) {
      return m_scope_stack[i].get_var(name);
    }
  }

  throw std::runtime_error("var " + name +
                           " is not defined in any compiler scope");
}

void Compiler::set_var(std::string &name, size_t addr, Type *type) {
  size_t top_scope{m_scope_stack.size() - 1};
  m_scope_stack[top_scope].set_var_addr(name, addr, type);
}

void Compiler::add_type(std::string &name, TypePtr type) {
  m_type_pool[name] = std::move(type);
}

bool Compiler::contains_type(std::string &name) const {

  size_t name_size{name.size()};

  if (name[0] == '[' && name[name_size - 1] == ']') {
    return true;
  }

  return (m_type_pool.count(name) != 0);
}

Type *Compiler::get_type(std::string &name) {

  size_t name_size{name.size()};

  // just create array types on the fly
  if (name[0] == '[' && name[name_size - 1] == ']') {
    if (m_type_pool.count(name) == 0) {
      std::string array_len = name.substr(1, name_size - 1);
      std::stringstream sstream(array_len);
      size_t result;
      sstream >> result;

      m_type_pool[name] = std::make_unique<ArrayType>(result);
    }
  }

  return (m_type_pool.at(name).get());
}

Type *Compiler::get_builtin_integer() const {
  return m_type_pool.at("Integer").get();
}

void Compiler::add_function(std::string name, DefinedFunction function_info) {
  functions[name] = function_info;
}

bool Compiler::contains_function(const std::string &name) const {
  return (functions.count(name) != 0);
}

const Compiler::DefinedFunction &
Compiler::get_function(const std::string &name) const {
  return functions.at(name);
}

void Compiler::begin_call(std::string name, bool returns_value) {
  add_scope();
  m_call_stack.push_back(ActiveCall{std::move(name), m_scope_stack.size() - 1,
                                    returns_value, false, 0});
}

void Compiler::end_call() {
  remove_scope();
  m_call_stack.pop_back();
}

Compiler::ActiveCall *Compiler::current_call() {
  if (m_call_stack.empty()) {
    return nullptr;
  }

  return &m_call_stack.back();
}

bool Compiler::call_in_progress(const std::string &name) const {
  for (const ActiveCall &call : m_call_stack) {
    if (call.name == name) {
      return true;
    }
  }

  return false;
}

size_t Compiler::scope_depth() const { return m_scope_stack.size(); }
