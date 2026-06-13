#include "codegen.h"
#include "ast.h"
#include "compiler.h"

#include <sstream>
#include <stdexcept>

// This function and expression evaluation are pretty nasty so i think it
// deserves an explaination. This recursively searches through the expression
// tree and returns an EvalResult at each level. CONST means that both branches
// could be done by the compiler and have been smashed at compile time.
//
// The important bits are ADDRESS and TEMP. ADDRESS refers to a variable address
// which must be maintained. These are only returned when evaluating an Expr of
// derived type VarExpr, and when this function runs on a BinaryExpr node, if a
// type ADDRESS is returned, it creates and allocates a new byte of type TEMP.
// This notes that the address can be written to and modified, and this is where
// operations accumulate into a new cell.
//
// This, of course, results in a possible junk cell. Look at the AssignStmt's
// generate function to see how the final return value of this function can be
// handled in all of these cases.
EvalResult eval(std::ostream *out, Compiler *compiler, Expr *expr)
{
  if (expr->get_type() == ExprType::STRING)
  {
    throw std::runtime_error("illegal string literal in compound expression");
  }

  if (expr->get_type() == ExprType::NUMBER)
  {
    NumberExpr *num_expr = static_cast<NumberExpr *>(expr);
    std::stringstream stream(num_expr->val);
    size_t val;
    stream >> val;
    return EvalResult{EvalType::CONST, val};
  }

  if (expr->get_type() == ExprType::VAR)
  {
    VarExpr *var_expr = static_cast<VarExpr *>(expr);
    size_t addr{compiler->get_var(var_expr->name)};

    return EvalResult{EvalType::ADDRESS, addr};
  }

  if (expr->get_type() != ExprType::BINARY)
  {
    throw std::runtime_error("illegal expression tree configuration");
  }

  BinaryExpr *bin_expr = static_cast<BinaryExpr *>(expr);
  Scope &scope = compiler->get_scope();

  EvalResult left = eval(out, compiler, bin_expr->left.get());
  EvalResult right = eval(out, compiler, bin_expr->right.get());

  if (left.type == EvalType::CONST && right.type == EvalType::CONST)
  {
    if (bin_expr->op == "+")
    {
      return EvalResult{EvalType::CONST, left.val + right.val};
    }

    if (bin_expr->op == "-")
    {
      return EvalResult{EvalType::CONST, left.val - right.val};
    }

    if (bin_expr->op == "*")
    {
      return EvalResult{EvalType::CONST, left.val * right.val};
    }

    if (bin_expr->op == "/")
    {
      return EvalResult{EvalType::CONST, left.val / right.val};
    }

    if (bin_expr->op == "%")
    {
      return EvalResult{EvalType::CONST, left.val % right.val};
    }

    if (bin_expr->op == "==")
    {
      return EvalResult{EvalType::CONST, left.val == right.val};
    }

    if (bin_expr->op == "!=")
    {
      return EvalResult{EvalType::CONST, left.val != right.val};
    }

    if (bin_expr->op == "<")
    {
      return EvalResult{EvalType::CONST, left.val < right.val};
    }

    if (bin_expr->op == ">")
    {
      return EvalResult{EvalType::CONST, left.val > right.val};
    }

    if (bin_expr->op == ">=")
    {
      return EvalResult{EvalType::CONST, left.val >= right.val};
    }

    if (bin_expr->op == "<=")
    {
      return EvalResult{EvalType::CONST, left.val <= right.val};
    }

    throw std::runtime_error("unaccounted operator in const eval: " +
                             bin_expr->op);
  }

  // At least one side lives on the tape. Get the left operand into
  // a temp cell we own, then fold the right operand into it in place.
  // ADD/SUB preserve src, and MUL/DIV/MOD zero their dumps, so
  // variables and cells above next_free stay intact.
  size_t acc{};
  if (left.type == EvalType::TEMP)
  {
    acc = left.val;
  }
  else
  {
    acc = scope.get_next_free();
    scope.bump_next_free(1);

    if (left.type == EvalType::CONST)
    {
      *out << "MOV: " << acc << ", " << left.val << '\n';
    }
    else
    {
      *out << "MOV: " << acc << ", 0\n";
      *out << "ADD: " << acc << ", " << left.val << '\n';
    }
  }

  // Cells at next_free and above are zero, so they can
  // serve as the dump scratch the ops require
  size_t dump = scope.get_next_free();

  if (right.type == EvalType::CONST)
  {
    if (bin_expr->op == "+")
    {
      *out << "ADD_CONST: " << acc << ", " << right.val << '\n';
    }
    else if (bin_expr->op == "-")
    {
      *out << "SUB_CONST: " << acc << ", " << right.val << '\n';
    }
    else if (bin_expr->op == "*")
    {
      *out << "MUL_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
    }
    else if (bin_expr->op == "/")
    {
      *out << "DIV_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
    }
    else if (bin_expr->op == "%")
    {
      *out << "MOD_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
    }
    else if (bin_expr->op == "==")
    {
      *out << "EQ_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    }
    else if (bin_expr->op == "!=")
    {
      *out << "NEQ_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    }
    else if (bin_expr->op == "<")
    {
      *out << "LESS_CONST: " << acc << ", " << right.val << ", " << dump
           << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    }
    else if (bin_expr->op == ">")
    {
      *out << "GREATER_CONST: " << acc << ", " << right.val << ", " << dump
           << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    }
    else
    {
      throw std::runtime_error("unaccounted operator in eval: " + bin_expr->op);
    }
  }
  else
  {
    if (bin_expr->op == "+")
    {
      *out << "ADD: " << acc << ", " << right.val << '\n';
    }
    else if (bin_expr->op == "-")
    {
      *out << "SUB: " << acc << ", " << right.val << '\n';
    }
    else if (bin_expr->op == "*")
    {
      *out << "MUL: " << acc << ", " << right.val << ", " << dump << '\n';
    }
    else if (bin_expr->op == "/")
    {
      *out << "DIV: " << acc << ", " << right.val << ", " << dump << '\n';
    }
    else if (bin_expr->op == "%")
    {
      *out << "MOD: " << acc << ", " << right.val << ", " << dump << '\n';
    }
    else if (bin_expr->op == "==")
    {
      *out << "EQ: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    }
    else if (bin_expr->op == "!=")
    {
      *out << "NEQ: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    }
    else if (bin_expr->op == "<")
    {
      *out << "LESS: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    }
    else if (bin_expr->op == ">")
    {
      *out << "GREATER: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    }
    else
    {
      throw std::runtime_error("unaccounted operator in eval: " + bin_expr->op);
    }
  }

  if (right.type == EvalType::TEMP)
  {
    // Release the right-hand temp. It must be zeroed so it can
    // later be reused as dump scratch
    *out << "MOV: " << right.val << ", 0\n";
    if (right.val + 1 == scope.get_next_free())
    {
      scope.set_next_free(right.val);
    }
  }

  return EvalResult{EvalType::TEMP, acc};
}

void AssignStmt::generate(std::ostream *out, Compiler *compiler)
{

  // top level scope
  Scope &scope = compiler->get_scope();
  size_t snapshot{scope.get_next_free()};

  switch (type)
  {
  case AssignType::NEW:
  {

    if (val->get_type() == ExprType::STRING)
    {
      StringExpr *string_expr = static_cast<StringExpr *>(val.get());

      size_t dest{};
      dest = snapshot;
      scope.set_var_addr(name, snapshot);
      scope.bump_next_free(string_expr->val.size());

      *out << "INSERT_STRING: " << dest << ", " << string_expr->val << '\n';
      return;
    }

    EvalResult result = eval(out, compiler, val.get());
    size_t dest{};

    if (scope.contains_var(name))
    {
      dest = scope.get_var_addr(name);
      scope.set_next_free(snapshot);
    }
    else
    {
      dest = snapshot;
      scope.set_var_addr(name, snapshot);
      scope.set_next_free(snapshot + 1);
    }

    switch (result.type)
    {
    case EvalType::CONST:
    {
      *out << "MOV: " << dest << ", " << result.val << '\n';
      break;
    }

    case EvalType::ADDRESS:
    {
      if (result.val == dest)
      {
        break;
      }

      *out << "MOV: " << dest << ", 0\n";
      *out << "ADD: " << dest << ", " << result.val << '\n';
      break;
    }

    case EvalType::TEMP:
    {
      if (result.val == dest)
      {
        break;
      }

      *out << "MOV: " << dest << ", 0\n";
      *out << "ADD: " << dest << ", " << result.val << '\n';
      *out << "MOV: " << result.val << ", 0\n";
      break;
    }
    }

    break;
  }

  case AssignType::SET:
  {

    if (val->get_type() == ExprType::STRING)
    {
      throw std::runtime_error(
          "attempted to redfine variable with a string literal: " + name);
    }

    EvalResult result = eval(out, compiler, val.get());

    // throws an exception if this name
    // does not exist anywhere in the scope
    size_t dest = compiler->get_var(name);

    switch (result.type)
    {
    case EvalType::ADDRESS:
    {
      if (dest == result.val)
      {
        break;
      }

      *out << "MOV: " << dest << ", 0\n";
      *out << "ADD: " << dest << ", " << result.val << '\n';

      break;
    }

    case EvalType::CONST:
    {
      *out << "MOV: " << dest << ", " << result.val << '\n';
      break;
    }

    case EvalType::TEMP:
    {
      *out << "MOV: " << dest << ", 0\n";
      *out << "ADD: " << dest << ", " << result.val << '\n';
      *out << "MOV: " << result.val << ", 0\n";
      scope.set_next_free(snapshot);
      break;
    }
    }
  }
  }
}

void LoopStmt::generate(std::ostream *out, Compiler *compiler)
{
  ExprType type = cond->get_type();

  if (type == ExprType::VAR)
  {
    VarExpr *var_expr = static_cast<VarExpr *>(cond.get());
    size_t addr{compiler->get_var(var_expr->name)};

    compiler->add_scope();

    *out << "LOOP: " << addr << '\n';

    for (const StmtPtr &stmt : body)
    {
      stmt->generate(out, compiler);
    }

    *out << "ENDLOOP: " << addr << '\n';

    compiler->remove_scope();

    return;
  }

  if (type == ExprType::NUMBER)
  {
    NumberExpr *number_expr = static_cast<NumberExpr *>(cond.get());
    Scope &scope = compiler->get_scope();

    size_t addr{scope.get_next_free()};
    scope.bump_next_free(1);

    *out << "MOV: " << addr << ", " << number_expr->val << '\n';

    compiler->add_scope();

    *out << "LOOP: " << addr << '\n';

    for (const StmtPtr &stmt : body)
    {
      stmt->generate(out, compiler);
    }

    *out << "ENDLOOP: " << addr << '\n';

    compiler->remove_scope();

    return;
  }

  throw std::runtime_error(
      "loop conditions can only be number literals or single variables");
}

void IfStmt::generate(std::ostream *out, Compiler *compiler)
{
  ExprType type = cond->get_type();

  if (type == ExprType::STRING)
  {
    throw std::runtime_error(
        "string literals cannot be evaluated as a condition");
  }

  Scope &scope = compiler->get_scope();
  size_t snapshot{scope.get_next_free()};
  EvalResult result = eval(out, compiler, cond.get());

  switch (result.type)
  {
  case EvalType::CONST:
  {
    scope.bump_next_free(1);
    *out << "MOV: " << snapshot << ", " << result.val << '\n';
    break;
  }

  case EvalType::ADDRESS:
  {
    *out << "ADD: " << snapshot << ", " << result.val << '\n';
    break;
  }

  case EvalType::TEMP:
  {
    if (result.val == snapshot)
    {
      break;
    }

    // snapshot is a released temp and therefore already zero, so the
    // ADD acts as a copy. The source temp has to be zeroed because it
    // ends up above next_free once the flag is reserved
    *out << "ADD: " << snapshot << ", " << result.val << '\n';
    *out << "MOV: " << result.val << ", 0\n";
    break;
  }
  }

  scope.set_next_free(snapshot + 1);
  *out << "DOIF: " << snapshot << '\n';

  compiler->add_scope();

  for (const StmtPtr &stmt : body)
  {
    stmt->generate(out, compiler);
  }

  compiler->remove_scope();

  *out << "ENDIF: " << snapshot << '\n';
}

void PrintStrStmt::generate(std::ostream *out, Compiler *compiler)
{

  if (target->get_type() == ExprType::STRING)
  {
    throw std::runtime_error("can't use string literals with print_str. "
                             "this is due to copying that "
                             "would occur if this was allowed. assign a "
                             "variable to the string to "
                             "print it.");
  }

  if (target->get_type() != ExprType::VAR)
  {
    throw std::runtime_error(
        "print_str can only take a single variable argument!");
  }

  VarExpr *var_expr = static_cast<VarExpr *>(target.get());

  size_t addr = compiler->get_var(var_expr->name);

  *out << "OUZ: " << addr << ", .\n";
}

void PrintValStmt::generate(std::ostream *out, Compiler *compiler)
{

  EvalResult result = eval(out, compiler, target.get());
  Scope &scope = compiler->get_scope();

  size_t snapshot{scope.get_next_free()};
  switch (result.type)
  {
  case EvalType::CONST:
  {
    size_t temp = snapshot;
    *out << "ADD_CONST: " << temp << ", " << result.val << '\n';
    *out << "ITOA: " << temp << ", " << temp + 1 << '\n';
    *out << "MOV: " << temp << ", 0\n";
    break;
  }

  case EvalType::ADDRESS:
  {
    *out << "ITOA: " << result.val << ", " << scope.get_next_free() << '\n';
    break;
  }

  case EvalType::TEMP:
  {
    if (snapshot == result.val)
    {
      *out << "ITOA: " << result.val << ", " << result.val + 1 << '\n';
      *out << "MOV: " << result.val << ", 0\n";
      break;
    }

    *out << "ADD: " << snapshot << ", " << result.val << '\n';
    *out << "MOV: " << result.val << ", 0\n";
    *out << "ITOA: " << snapshot << ", " << snapshot + 1 << '\n';
    *out << "MOV: " << snapshot << ", 0\n";
    break;
  }
  }
}
