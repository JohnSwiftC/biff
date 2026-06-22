#ifndef COMPILER_H
#define COMPILER_H

#include <cstddef>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "types.h"

class Scope {
private:
  struct Variable {
    size_t addr;
    Type *type;
  };
  std::unordered_map<std::string, Variable> m_vars;
  size_t m_next_free;

public:
  Scope();
  Scope(size_t next_free);

  size_t get_var_addr(std::string &var) const;
  void set_var_addr(std::string var, size_t addr, Type *type);

  size_t get_next_free() const;
  void set_next_free(size_t addr);
  void bump_next_free(size_t size);
  bool contains_var(std::string &var) const;
};

class Compiler {
private:
  std::vector<Scope> m_scope_stack;
  std::vector<StmtPtr> m_program;
  std::unordered_map<std::string, TypePtr> m_type_pool;

public:
  Compiler(std::vector<StmtPtr> program);

  Scope &get_scope();
  void add_scope();
  void remove_scope();
  void generate_program(std::ostream *out);

  bool contains_var(std::string &name) const;
  size_t get_var(std::string &name) const;
  void set_var(std::string &name, size_t addr, Type *type);

  void add_type(std::string &type_name, TypePtr type);
  bool contains_type(std::string &name) const;
  Type *get_type(std::string &name) const;
};

#endif
