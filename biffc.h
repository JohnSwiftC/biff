#ifndef BIFFC_H
#define BIFFC_H

#include <string>
#include <unordered_map>
#include <vector>
class Scope {
private:
  std::unordered_map<std::string, size_t> m_vars;
  size_t m_next_free;

public:
  Scope();

  size_t get_var_addr(std::string &var) const;
  void set_var_addr(std::string var, size_t addr);

  size_t get_next_free() const;
  void set_next_free(size_t addr);
  void bump_next_free(size_t size);
};

class Compiler {
private:
  std::vector<Scope> m_scope_stack;

public:
  Compiler();

  Scope &get_scope();
  void add_scope();
  void remove_scope();
};

#endif
