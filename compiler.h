#ifndef COMPILER_H
#define COMPILER_H

#include <cstddef>
#include <deque>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "types.h"

class Scope {
public:
  struct Variable {
    size_t addr;
    Type *type;
  };
  Scope();
  Scope(size_t next_free);

  size_t get_var_addr(std::string &var) const;
  void set_var_addr(std::string var, size_t addr, Type *type);

  const Variable &get_var(std::string &var) const;
  void set_var(std::string name, Variable var);

  size_t get_next_free() const;
  void set_next_free(size_t addr);
  void bump_next_free(size_t size);
  bool contains_var(std::string &var) const;

private:
  std::unordered_map<std::string, Variable> m_vars;
  size_t m_next_free;
};

class Compiler {
public:
  struct DefinedFunction {
    std::string return_type;
    std::vector<DefineFunctionStmt::Argument> args;
    // owned by the DefineFunctionStmt, which lives in
    // m_program for the whole compilation
    const std::vector<StmtPtr> *body;
  };

  // Compile-time state for a call currently being inlined. The
  // stack of these detects recursion (direct or mutual), fences
  // variable lookups off from the caller's scopes, and carries the
  // return cell between the ReturnStmt and the call site
  struct ActiveCall {
    std::string name;
    // index of the function's frame scope in m_scope_stack. lookups
    // stop here so bodies only see their own params and locals
    size_t scope_barrier;
    bool returns_value;
    bool has_returned;
    size_t ret_addr;
  };

private:
  // deque, not vector: statement generators hold Scope references
  // across eval() calls, and a function call inside the expression
  // pushes a frame scope. deque growth never invalidates
  // references to the scopes that are still alive
  std::deque<Scope> m_scope_stack;
  std::vector<StmtPtr> m_program;
  std::unordered_map<std::string, TypePtr> m_type_pool;

  std::unordered_map<std::string, DefinedFunction> functions;

  // deque for the same reason as m_scope_stack: ReturnStmt holds an
  // ActiveCall pointer across eval(), which can push nested calls
  std::deque<ActiveCall> m_call_stack;

public:
  Compiler(std::vector<StmtPtr> program);

  Scope &get_scope();
  void add_scope();
  void remove_scope();
  void generate_program(std::ostream *out);

  bool contains_var(std::string &name) const;
  const Scope::Variable &get_var(std::string &name) const;
  void set_var(std::string &name, size_t addr, Type *type);

  void add_type(std::string &type_name, TypePtr type);
  bool contains_type(std::string &name) const;
  Type *get_type(std::string &name);
  Type *get_builtin_integer() const;

  void add_function(std::string name, DefinedFunction function_info);
  bool contains_function(const std::string &name) const;
  const DefinedFunction &get_function(const std::string &name) const;

  // pushes the function's frame scope along with its call entry
  void begin_call(std::string name, bool returns_value);
  // pops both again. next_free is carried up like remove_scope,
  // the call site is expected to reclaim the frame itself
  void end_call();
  ActiveCall *current_call();
  bool call_in_progress(const std::string &name) const;
  size_t scope_depth() const;
};

#endif
