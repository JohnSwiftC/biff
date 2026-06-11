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
EvalResult eval(std::ostream *out, Compiler *compiler, Expr *expr) {
  if (expr->get_type() == ExprType::STRING) {
    throw std::runtime_error("illegal string literal in compound expression");
  }

  if (expr->get_type() == ExprType::NUMBER) {
    NumberExpr *num_expr = static_cast<NumberExpr *>(expr);
    std::stringstream stream(num_expr->val);
    size_t val;
    stream >> val;
    return EvalResult{EvalType::CONST, val};
  }

  if (expr->get_type() == ExprType::VAR) {
    VarExpr *var_expr = static_cast<VarExpr *>(expr);
    size_t addr{};

    if (compiler->get_scope().contains_var(var_expr->name)) {
      addr = compiler->get_scope().get_var_addr(var_expr->name);
    } else {
      throw std::runtime_error("var " + var_expr->name +
                               " is not defined in scope");
    }

    return EvalResult{EvalType::ADDRESS, addr};
  }

  if (expr->get_type() != ExprType::BINARY) {
    throw std::runtime_error("illegal expression tree configuration");
  }

  BinaryExpr *bin_expr = static_cast<BinaryExpr *>(expr);
  Scope &scope = compiler->get_scope();

  EvalResult left = eval(out, compiler, bin_expr->left.get());
  EvalResult right = eval(out, compiler, bin_expr->right.get());

  if (left.type == EvalType::CONST && right.type == EvalType::CONST) {
    if (bin_expr->op == "+") {
      return EvalResult{EvalType::CONST, left.val + right.val};
    }

    if (bin_expr->op == "-") {
      return EvalResult{EvalType::CONST, left.val - right.val};
    }

    if (bin_expr->op == "*") {
      return EvalResult{EvalType::CONST, left.val * right.val};
    }

    if (bin_expr->op == "/") {
      return EvalResult{EvalType::CONST, left.val / right.val};
    }

    if (bin_expr->op == "%") {
      return EvalResult{EvalType::CONST, left.val % right.val};
    }

    if (bin_expr->op == "==") {
      return EvalResult{EvalType::CONST, left.val == right.val};
    }

    if (bin_expr->op == "!=") {
      return EvalResult{EvalType::CONST, left.val != right.val};
    }

    if (bin_expr->op == "<") {
      return EvalResult{EvalType::CONST, left.val < right.val};
    }

    if (bin_expr->op == ">") {
      return EvalResult{EvalType::CONST, left.val > right.val};
    }

    throw std::runtime_error("unaccounted operator in const eval: " +
                             bin_expr->op);
  }

  // At least one side lives on the tape. Get the left operand into
  // a temp cell we own, then fold the right operand into it in place.
  // ADD/SUB preserve src, and MUL/DIV/MOD zero their dumps, so
  // variables and cells above next_free stay intact.
  size_t acc{};
  if (left.type == EvalType::TEMP) {
    acc = left.val;
  } else {
    acc = scope.get_next_free();
    scope.bump_next_free(1);

    if (left.type == EvalType::CONST) {
      *out << "MOV: " << acc << ", " << left.val << '\n';
    } else {
      *out << "MOV: " << acc << ", 0\n";
      *out << "ADD: " << acc << ", " << left.val << '\n';
    }
  }

  // Cells at next_free and above are zero, so they can
  // serve as the dump scratch the ops require
  size_t dump = scope.get_next_free();

  if (right.type == EvalType::CONST) {
    if (bin_expr->op == "+") {
      *out << "ADD_CONST: " << acc << ", " << right.val << '\n';
    } else if (bin_expr->op == "-") {
      *out << "SUB_CONST: " << acc << ", " << right.val << '\n';
    } else if (bin_expr->op == "*") {
      *out << "MUL_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
    } else if (bin_expr->op == "/") {
      *out << "DIV_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
    } else if (bin_expr->op == "%") {
      *out << "MOD_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
    } else if (bin_expr->op == "==") {
      *out << "EQ_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    } else if (bin_expr->op == "!=") {
      *out << "NEQ_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    } else if (bin_expr->op == "<") {
      *out << "LESS_CONST: " << acc << ", " << right.val << ", " << dump
           << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    } else if (bin_expr->op == ">") {
      *out << "GREATER_CONST: " << acc << ", " << right.val << ", " << dump
           << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    } else {
      throw std::runtime_error("unaccounted operator in eval: " + bin_expr->op);
    }
  } else {
    if (bin_expr->op == "+") {
      *out << "ADD: " << acc << ", " << right.val << '\n';
    } else if (bin_expr->op == "-") {
      *out << "SUB: " << acc << ", " << right.val << '\n';
    } else if (bin_expr->op == "*") {
      *out << "MUL: " << acc << ", " << right.val << ", " << dump << '\n';
    } else if (bin_expr->op == "/") {
      *out << "DIV: " << acc << ", " << right.val << ", " << dump << '\n';
    } else if (bin_expr->op == "%") {
      *out << "MOD: " << acc << ", " << right.val << ", " << dump << '\n';
    } else if (bin_expr->op == "==") {
      *out << "EQ: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    } else if (bin_expr->op == "!=") {
      *out << "NEQ: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    } else if (bin_expr->op == "<") {
      *out << "LESS: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    } else if (bin_expr->op == ">") {
      *out << "GREATER: " << acc << ", " << right.val << ", " << dump << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    } else {
      throw std::runtime_error("unaccounted operator in eval: " + bin_expr->op);
    }
  }

  if (right.type == EvalType::TEMP) {
    // Release the right-hand temp. It must be zeroed so it can
    // later be reused as dump scratch
    *out << "MOV: " << right.val << ", 0\n";
    if (right.val + 1 == scope.get_next_free()) {
      scope.set_next_free(right.val);
    }
  }

  return EvalResult{EvalType::TEMP, acc};
}

void AssignStmt::generate(std::ostream *out, Compiler *compiler) {
  ExprType type = val->get_type();
  Scope &scope = compiler->get_scope();
  if (type == ExprType::STRING) {
    if (scope.contains_var(name)) {
      throw std::runtime_error("attempted to redefine string const: " + name);
    }

    StringExpr *string_expr = static_cast<StringExpr *>(val.get());
    size_t string_size = string_expr->val.length();
    size_t dest = scope.get_next_free();
    *out << "INSERT_STRING: " << dest << ", " << string_expr->val << '\n';

    scope.set_var_addr(name, dest);
    scope.bump_next_free(string_size);
    return;
  } else if (type == ExprType::NUMBER) {
    size_t dest{};
    if (scope.contains_var(name)) {
      dest = scope.get_var_addr(name);
    } else {
      dest = scope.get_next_free();
      scope.set_var_addr(name, dest);
      scope.bump_next_free(1);
    }

    NumberExpr *number_expr = static_cast<NumberExpr *>(val.get());

    *out << "MOV: " << dest << ", " << number_expr->val << '\n';
    return;
  }

  size_t snapshot = scope.get_next_free();
  EvalResult result = eval(out, compiler, val.get());

  switch (result.type) {
  case EvalType::CONST: {
    size_t dest{};
    if (scope.contains_var(name)) {
      dest = scope.get_var_addr(name);
    } else {
      dest = scope.get_next_free();
      scope.set_var_addr(name, dest);
      scope.bump_next_free(1);
    }

    *out << "MOV: " << dest << ", " << result.val << '\n';
    break;
  }

  case EvalType::ADDRESS: {
    size_t dest{};
    if (scope.contains_var(name)) {
      dest = scope.get_var_addr(name);
    } else {
      dest = scope.get_next_free();
      scope.set_var_addr(name, dest);
      scope.bump_next_free(1);
    }

    if (dest == result.val) {
      // x = x;
      break;
    }

    *out << "MOV: " << dest << ", 0\n";
    *out << "ADD: " << dest << ", " << result.val << '\n';
    break;
  }

  case EvalType::TEMP: {
    if (!scope.contains_var(name)) {
      // The address the temp was accumulated in is already in the newest
      // possible position, so any moving is redundant
      if (result.val == snapshot) {
        scope.set_var_addr(name, result.val);
        scope.set_next_free(result.val + 1);
        break;
      }

      *out << "ADD: " << snapshot << ", " << result.val << '\n';
      *out << "MOV: " << result.val << ", 0\n";
      scope.set_var_addr(name, snapshot);
      scope.set_next_free(snapshot + 1);
      break;
    }

    // As explained in the eval function doc,
    // this handles the case where an existing variable is being reset
    // to a temp value. set dest to 0, add the temp, and importantly,
    // drop the temp value.
    size_t dest = scope.get_var_addr(name);
    *out << "MOV: " << dest << ", 0\n";
    *out << "ADD: " << dest << ", " << result.val << '\n';
    *out << "MOV: " << result.val << ", 0\n";
    scope.set_next_free(snapshot);
    break;
  }
  }
}

void LoopStmt::generate(std::ostream *out, Compiler *compiler) {}

void IfStmt::generate(std::ostream *out, Compiler *compiler) {
  ExprType type = cond->get_type();

  if (type == ExprType::STRING) {
    throw std::runtime_error(
        "string literals cannot be evaluated as a condition");
  }

  Scope &scope = compiler->get_scope();
  size_t snapshot{scope.get_next_free()};
  EvalResult result = eval(out, compiler, cond.get());

  switch (result.type) {
  case EvalType::CONST: {
    scope.bump_next_free(1);
    *out << "MOV: " << snapshot << ", " << result.val << '\n';
    break;
  }

  case EvalType::ADDRESS: {
    *out << "ADD: " << snapshot << ", " << result.val << '\n';
    break;
  }

  case EvalType::TEMP: {
    if (result.val == snapshot) {
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

  for (const StmtPtr &stmt : body) {
    stmt->generate(out, compiler);
  }

  *out << "ENDIF: " << snapshot << '\n';
}
