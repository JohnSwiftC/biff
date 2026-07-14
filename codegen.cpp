#include "codegen.h"
#include "ast.h"
#include "compexcept.h"
#include "compiler.h"
#include "types.h"

#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

// returns the address being referred too by a varexpr
size_t eval_var_expr(Compiler *compiler, Expr *expr, Type **out_type) {

  if (expr->get_type() != ExprType::VAR) {
    throw CompilerException(
        expr->line_number,
        "illegal expression type: should be a variable expression");
  }

  VarExpr *var_expr = static_cast<VarExpr *>(expr);

  if (!compiler->contains_var(var_expr->name)) {
    throw CompilerException(var_expr->line_number,
                            "variable has not yet been defined");
  }

  const Scope::Variable &var = compiler->get_var(var_expr->name);

  size_t curr_addr = var.addr;
  Type *curr_type = var.type;

  // quick check to see if we are in a userdefed struct and the such
  if (curr_type->type_class == TypeClass::USERDEF &&
      var_expr->fields.size() == 0) {
    throw CompilerException(
        var_expr->line_number,
        "variables of a struct type must be accessed by field");
  }

  for (const std::string &field : var_expr->fields) {
    if (curr_type->type_class != TypeClass::USERDEF) {
      throw CompilerException(var_expr->line_number,
                              "illegal field access in non-struct type");
    }

    StructType *struct_type = static_cast<StructType *>(curr_type);

    if (struct_type->fields.count(field) == 0) {
      throw CompilerException(var_expr->line_number,
                              "field: " + field + " does not exist in type");
    }

    StructType::Field &field_data = struct_type->fields.at(field);
    curr_type = field_data.type;
    curr_addr += field_data.offset;
  }

  if (out_type) {
    *out_type = curr_type;
  }

  return curr_addr;
}

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
    throw CompilerException(expr->line_number,
                            "illegal string literal in compound expression");
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
    size_t addr{eval_var_expr(compiler, static_cast<VarExpr *>(expr))};

    return EvalResult{EvalType::ADDRESS, addr};
  }

  if (expr->get_type() == ExprType::ARRAYVAR) {
    ArrayVarExpr *array_var_expr = static_cast<ArrayVarExpr *>(expr);

    Scope &scope = compiler->get_scope();
    Type *base_type = nullptr;
    size_t base{
        eval_var_expr(compiler, array_var_expr->var_expr.get(), &base_type)};
    if (base_type->type_class != TypeClass::ARRAY) {
      throw CompilerException(array_var_expr->line_number,
                              "cannot index into a non-array type");
    }
    size_t dest{scope.get_next_free()};

    EvalResult index = eval(out, compiler, array_var_expr->index_expr.get());
    if (index.type == EvalType::CONST) {
      *out << "RA_CONST: " << base << ", " << index.val << ", " << dest << '\n';
      scope.bump_next_free(1);
    } else if (index.type == EvalType::TEMP) {
      if (index.val == dest) {
        *out << "RA: " << base << ", " << index.val << ", " << dest << '\n';
      } else {
        *out << "RA: " << base << ", " << index.val << ", " << dest << '\n';
        *out << "MOV: " << index.val << ", 0\n";
        scope.set_next_free(dest + 1);
      }
    } else if (index.type == EvalType::ADDRESS) {
      *out << "RA: " << base << ", " << index.val << ", " << dest << '\n';
      scope.bump_next_free(1);
    }

    return EvalResult{EvalType::TEMP, dest};
  }

  if (expr->get_type() == ExprType::READ_CHAR) {
    Scope &scope = compiler->get_scope();

    // The cell doesn't need to be zeroed first: a bf read
    // overwrites the cell rather than accumulating into it
    size_t dest{scope.get_next_free()};
    scope.bump_next_free(1);

    *out << "READ_CHAR: " << dest << '\n';

    return EvalResult{EvalType::TEMP, dest};
  }

  if (expr->get_type() == ExprType::CALL) {
    CallExpr *call = static_cast<CallExpr *>(expr);
    std::optional<size_t> ret = generate_call(out, compiler, call);

    if (!ret) {
      throw CompilerException(call->line_number,
                              "function: " + call->name +
                                  " does not return a value and cannot be "
                                  "used in an expression");
    }

    return EvalResult{EvalType::TEMP, *ret};
  }

  if (expr->get_type() == ExprType::UNARY) {
    return eval_unary(out, compiler, static_cast<UnaryExpr *>(expr));
  }

  if (expr->get_type() != ExprType::BINARY) {
    throw CompilerException(expr->line_number,
                            "illegal expression tree configuration");
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

    if (bin_expr->op == ">=") {
      return EvalResult{EvalType::CONST, left.val >= right.val};
    }

    if (bin_expr->op == "<=") {
      return EvalResult{EvalType::CONST, left.val <= right.val};
    }

    if (bin_expr->op == "||") {
      return EvalResult{EvalType::CONST, left.val || right.val};
    }

    if (bin_expr->op == "&&") {
      return EvalResult{EvalType::CONST, left.val && right.val};
    }

    throw CompilerException(bin_expr->line_number,
                            "unaccounted operator in const eval: " +
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
    } else if (bin_expr->op == "<=") {
      *out << "LESS_OR_EQ_CONST: " << acc << ", " << right.val << ", " << dump
           << '\n';
      *out << "MOV: " << acc << ", 0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", 0\n";
    } else if (bin_expr->op == ">=") {
      *out << "GREATER_OR_EQ_CONST: " << acc << ", " << right.val << ", "
           << dump << '\n';
      *out << "MOV: " << acc << ", 0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", 0\n";
    } else if (bin_expr->op == "||") {
      *out << "OR_CONST: " << acc << ", " << right.val << '\n';
    } else if (bin_expr->op == "&&") {
      *out << "AND_CONST: " << acc << ", " << right.val << ", " << dump << '\n';
    } else {
      throw CompilerException(bin_expr->line_number,
                              "unaccounted operator in eval: " + bin_expr->op);
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
    } else if (bin_expr->op == "<=") {
      *out << "LESS_OR_EQ: " << acc << ", " << right.val << ", " << dump
           << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    } else if (bin_expr->op == ">=") {
      *out << "GREATER_OR_EQ: " << acc << ", " << right.val << ", " << dump
           << '\n';
      *out << "MOV: " << acc << ", " << "0\n";
      *out << "ADD: " << acc << ", " << dump << '\n';
      *out << "MOV: " << dump << ", " << "0\n";
    } else if (bin_expr->op == "||") {
      *out << "OR: " << acc << ", " << right.val << '\n';
    } else if (bin_expr->op == "&&") {
      *out << "AND: " << acc << ", " << right.val << ", " << dump << '\n';
    } else {
      throw CompilerException(bin_expr->line_number,
                              "unaccounted operator in eval: " + bin_expr->op);
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

EvalResult eval_unary(std::ostream *out, Compiler *compiler, UnaryExpr *unary) {
  Scope &scope = compiler->get_scope();
  size_t snapshot = scope.get_next_free();
  EvalResult result = eval(out, compiler, unary->operand.get());

  switch (result.type) {
  case EvalType::CONST: {
    size_t new_val{};
    if (unary->op == "!") {
      if (result.val == 0) {
        new_val = 1;
      } else {
        new_val = 0;
      }
    }

    return EvalResult{EvalType::CONST, new_val};
  }

  case EvalType::ADDRESS: {
    size_t dest{snapshot};
    scope.bump_next_free(1);
    if (unary->op == "!") {
      *out << "MOV: " << dest << ", 0\n";
      *out << "ADD: " << dest << ", " << result.val << '\n';
      *out << "FLIP: " << dest << '\n';
    }

    return EvalResult{EvalType::TEMP, dest};
  }

  case EvalType::TEMP: {
    // handle temp bs first

    size_t dest{snapshot};

    if (result.val != dest) {
      *out << "MOV: " << dest << ", 0\n";
      *out << "ADD: " << dest << ", " << result.val << '\n';
      *out << "MOV: " << result.val << ", 0\n";
      scope.set_next_free(result.val);
    }

    if (unary->op == "!") {
      *out << "FLIP: " << dest << '\n';
    }

    return EvalResult{EvalType::TEMP, dest};
  }
  }

  throw std::runtime_error("magic unknown EvalType reached in parse_unary");
}

void handle_new_struct(Compiler *compiler, VarExpr *var_expr, Type *struct_type,
                       Expr *val_expr) {
  Scope &scope = compiler->get_scope();
  if (scope.contains_var(var_expr->name)) {
    throw CompilerException(val_expr->line_number,
                            "attempted to redefine a variable of struct type "
                            "in the top level scope");
  }

  if (val_expr->get_type() != ExprType::NUMBER) {
    throw CompilerException(val_expr->line_number,
                            "structs are created by setting them equal to 0");
  }

  auto number_expr = static_cast<NumberExpr *>(val_expr);

  if (number_expr->val != "0") {
    throw CompilerException(number_expr->line_number,
                            "structs must be set to zero on creation");
  }

  scope.set_var(var_expr->name,
                Scope::Variable{scope.get_next_free(), struct_type});
  scope.bump_next_free(struct_type->size);
}

void AssignStmt::generate(std::ostream *out, Compiler *compiler) {

  // top level scope
  Scope &scope = compiler->get_scope();
  size_t snapshot{scope.get_next_free()};

  if (target_var_expr->get_type() != ExprType::VAR) {
    throw CompilerException(
        line_number, "assignments may only happen on variable expressions");
  }

  VarExpr *var_expr = static_cast<VarExpr *>(target_var_expr.get());

  switch (assign_type) {
  case AssignType::NEW: {

    if (var_expr->fields.size() != 0) {
      throw CompilerException(line_number,
                              "let cannot be used to define new struct fields");
    }

    if (val->get_type() == ExprType::STRING) {
      StringExpr *string_expr = static_cast<StringExpr *>(val.get());

      if (type_name) {
        throw CompilerException(
            line_number,
            "string assignments should not be given an explicit type");
      }

      std::string type_name =
          "[" + std::to_string(string_expr->val.size()) + "]";

      Type *type = nullptr;

      if (compiler->contains_type(type_name)) {
        type = compiler->get_type(type_name);
      } else {
        TypePtr new_type = std::make_unique<ArrayType>(string_expr->val.size());
        type = new_type.get();
        compiler->add_type(type_name, std::move(new_type));
      }

      size_t dest{};
      dest = snapshot;
      scope.set_var_addr(var_expr->name, snapshot, type);
      scope.bump_next_free(string_expr->val.size() + 3);

      *out << "INSERT_STRING: " << dest << ", " << string_expr->val << '\n';
      return;
    }

    if (type_name) {
      if (compiler->contains_type(*type_name)) {
        Type *type = compiler->get_type(*type_name);
        if (type->type_class == TypeClass::USERDEF) {
          handle_new_struct(compiler, var_expr, type, val.get());
          return;
        }
      } else {
        throw CompilerException(line_number, "type: " + *type_name +
                                                 " is not a struct type");
      }
    }

    EvalResult result = eval(out, compiler, val.get());
    size_t dest{};

    if (scope.contains_var(var_expr->name)) {
      dest = scope.get_var_addr(var_expr->name);
      scope.set_next_free(snapshot);
    } else {
      dest = snapshot;
      scope.set_var_addr(var_expr->name, snapshot,
                         compiler->get_builtin_integer());
      scope.set_next_free(snapshot + 1);
    }

    switch (result.type) {
    case EvalType::CONST: {
      *out << "MOV: " << dest << ", " << result.val << '\n';
      break;
    }

    case EvalType::ADDRESS: {
      if (result.val == dest) {
        break;
      }

      *out << "MOV: " << dest << ", 0\n";
      *out << "ADD: " << dest << ", " << result.val << '\n';
      break;
    }

    case EvalType::TEMP: {
      if (result.val == dest) {
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

  case AssignType::SET: {

    if (val->get_type() == ExprType::STRING) {
      throw CompilerException(
          val->line_number,
          "attempted to redfine variable with a string literal: " +
              var_expr->name);
    }

    EvalResult result = eval(out, compiler, val.get());

    size_t dest = eval_var_expr(compiler, var_expr);

    switch (result.type) {
    case EvalType::ADDRESS: {
      if (dest == result.val) {
        break;
      }

      *out << "MOV: " << dest << ", 0\n";
      *out << "ADD: " << dest << ", " << result.val << '\n';

      break;
    }

    case EvalType::CONST: {
      *out << "MOV: " << dest << ", " << result.val << '\n';
      break;
    }

    case EvalType::TEMP: {
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

void LoopStmt::generate(std::ostream *out, Compiler *compiler) {
  Scope &scope = compiler->get_scope();
  size_t snapshot{scope.get_next_free()};
  EvalResult result = eval(out, compiler, cond.get());
  size_t dest{};

  // Find the address that will be used for the loop instruction,
  // allocate a new byte if needed. Also note how we allow addresses to
  // be modified here per how i want loops to work with solo variable args
  switch (result.type) {
  case EvalType::ADDRESS: {
    dest = result.val;
    break;
  }

  case EvalType::TEMP: {
    dest = snapshot;
    scope.set_next_free(snapshot + 1);
    if (snapshot == result.val) {
      break;
    }

    *out << "MOV: " << snapshot << ", 0\n";
    *out << "ADD: " << snapshot << ", " << result.val << "\n";
    *out << "MOV: " << result.val << ", 0\n";

    break;
  }

  case EvalType::CONST: {
    dest = snapshot;
    scope.set_next_free(snapshot + 1);

    *out << "MOV: " << snapshot << ", " << result.val << '\n';
    break;
  }
  }

  *out << "LOOP: " << dest << '\n';
  compiler->add_scope();

  for (const StmtPtr &stmt : body) {
    stmt->generate(out, compiler);
  }

  *out << "ENDLOOP: " << dest << '\n';
  compiler->remove_scope();
}

void IfStmt::generate(std::ostream *out, Compiler *compiler) {
  ExprType type = cond->get_type();

  if (type == ExprType::STRING) {
    throw CompilerException(
        cond->line_number,
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

  compiler->add_scope();

  for (const StmtPtr &stmt : body) {
    stmt->generate(out, compiler);
  }

  compiler->remove_scope();

  *out << "ENDIF: " << snapshot << '\n';
}

void PrintStrStmt::generate(std::ostream *out, Compiler *compiler) {

  if (target->get_type() == ExprType::STRING) {
    throw CompilerException(target->line_number,
                            "can't use string literals with print_str. "
                            "this is due to copying that "
                            "would occur if this was allowed. assign a "
                            "variable to the string to "
                            "print it.");
  }

  if (target->get_type() != ExprType::VAR) {
    throw CompilerException(
        target->line_number,
        "print_str can only take a single variable argument!");
  }

  VarExpr *var_expr = static_cast<VarExpr *>(target.get());

  size_t addr = eval_var_expr(compiler, var_expr);

  *out << "MOV: " << addr + 3 << ", 0\n";
  *out << "OUZ: " << addr << ", .\n";
}

void PrintValStmt::generate(std::ostream *out, Compiler *compiler) {

  EvalResult result = eval(out, compiler, target.get());
  Scope &scope = compiler->get_scope();

  size_t snapshot{scope.get_next_free()};
  switch (result.type) {
  case EvalType::CONST: {
    size_t temp = snapshot;
    *out << "ADD_CONST: " << temp << ", " << result.val << '\n';
    *out << "ITOA: " << temp << ", " << temp + 1 << '\n';
    *out << "MOV: " << temp << ", 0\n";
    break;
  }

  case EvalType::ADDRESS: {
    *out << "ITOA: " << result.val << ", " << scope.get_next_free() << '\n';
    break;
  }

  case EvalType::TEMP: {
    if (snapshot == result.val) {
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

void CreateArrayStmt::generate(std::ostream *out, Compiler *compiler) {
  if (assign_type == AssignType::SET) {
    throw CompilerException(line_number,
                            "arrays cannot be redefined as a new array");
  }

  // top level scope
  // i need to write down how the scope works
  // lmfao
  Scope &scope = compiler->get_scope();

  if (scope.contains_var(name)) {
    throw CompilerException(
        line_number, "attempted to redefine array in its top level scope");
  }

  size_t base{scope.get_next_free()};

  EvalResult size = eval(out, compiler, size_expr.get());

  if (size.type != EvalType::CONST) {
    throw CompilerException(line_number,
                            "array sizes must be evaluated at compile time "
                            "(only constexprs can be used for array sizes)");
  }

  // Arrays require size + 4 bytes to function
  // (see the IR implementation for further info on why)
  scope.bump_next_free(size.val + 4);
  std::string type_name = "[" + std::to_string(size.val) + "]";
  Type *array_type = compiler->get_type(type_name);

  scope.set_var_addr(name, base, array_type);
}

void AssignArrayStmt::generate(std::ostream *out, Compiler *compiler) {

  if (target_var_expr->get_type() != ExprType::VAR) {
    throw CompilerException(
        target_var_expr->line_number,
        "assignments can only be applied to variables expressions");
  }

  const auto var_expr = static_cast<VarExpr *>(target_var_expr.get());

  Type *base_type = nullptr;
  size_t base = eval_var_expr(compiler, var_expr, &base_type);
  if (base_type->type_class != TypeClass::ARRAY) {
    throw CompilerException(var_expr->line_number,
                            "cannot index into a non-array type");
  }
  Scope &scope = compiler->get_scope();

  size_t snapshot{scope.get_next_free()};

  // Address of the index and targets
  // for the non-const branches
  size_t index_addr{};
  size_t target_addr{};

  EvalResult index = eval(out, compiler, index_expr.get());

  if (index.type == EvalType::TEMP) {
    if (index.val != snapshot) {
      *out << "MOV: " << snapshot << ", 0\n";
      *out << "ADD: " << snapshot << ", " << index.val << '\n';
      *out << "MOV: " << index.val << ", 0\n";
    }

    scope.set_next_free(snapshot + 1);
    index_addr = snapshot;
  } else if (index.type == EvalType::ADDRESS) {
    index_addr = index.val;
  }

  EvalResult target = eval(out, compiler, target_expr.get());

  if (target.type == EvalType::TEMP) {
    if (target.val != snapshot + 1) {
      *out << "MOV: " << snapshot + 1 << ", 0\n";
      *out << "ADD: " << snapshot + 1 << ", " << target.val << '\n';
      *out << "MOV: " << target.val << ", 0\n";
    }

    scope.set_next_free(snapshot + 2);
    target_addr = snapshot + 1;
  } else if (target.type == EvalType::ADDRESS) {
    target_addr = target.val;
  }

  if (index.type == EvalType::CONST && target.type == EvalType::CONST) {
    *out << "SAV_FULL_CONST: " << base << ", " << index.val << ", "
         << target.val << '\n';
    return;
  }

  if (index.type != EvalType::CONST && target.type == EvalType::CONST) {
    *out << "SAV_VAL_CONST: " << base << ", " << index_addr << ", "
         << target.val << '\n';
  } else if (index.type == EvalType::CONST && target.type != EvalType::CONST) {
    *out << "SAV_INDEX_CONST: " << base << ", " << index.val << ", "
         << target_addr << '\n';
  } else if (index.type != EvalType::CONST && target.type != EvalType::CONST) {
    *out << "SAV: " << base << ", " << index_addr << ", " << target_addr
         << '\n';
  }

  // cleanup
  if (index.type == EvalType::TEMP) {
    *out << "MOV: " << index_addr << ", 0\n";
  }

  if (target.type == EvalType::TEMP) {
    *out << "MOV: " << target_addr << ", 0\n";
  }

  scope.set_next_free(snapshot);
}

void DefineStructStmt::generate(std::ostream *out, Compiler *compiler) {
  if (compiler->contains_type(name)) {
    throw CompilerException(line_number,
                            "type '" + name + "' has already been defined");
  }

  auto new_struct_type = std::make_unique<StructType>();

  try {
    for (auto [field_name, type] : fields) {
      if (!compiler->contains_type(type)) {
        throw CompilerException(line_number,
                                type + " is not a currently defined type");
      }

      Type *real_type = compiler->get_type(type);

      new_struct_type->add_field(std::move(field_name), real_type);
    }

    compiler->add_type(name, std::move(new_struct_type));
  } catch (TypeException &e) {
    throw CompilerException(line_number, std::move(e.message));
  }
}

void DefineFunctionStmt::generate(std::ostream *out, Compiler *compiler) {
  if (compiler->contains_function(name)) {
    throw CompilerException(line_number,
                            "function: " + name + " has already been defined");
  }

  if (compiler->current_call()) {
    throw CompilerException(line_number,
                            "functions cannot be defined inside functions");
  }

  if (return_type != "Integer" && return_type != "None") {
    throw CompilerException(line_number,
                            "functions may only return Integer, or None for "
                            "no return value");
  }

  std::unordered_set<std::string> used_names{name};

  for (auto [type, name] : args) {
    if (!compiler->contains_type(type)) {
      throw CompilerException(line_number,
                              "type: " + type + " is not currently defined");
    }

    if (used_names.count(name) != 0) {
      throw CompilerException(line_number,
                              "name " + name +
                                  " cannot be reused in a function definition");
    }

    used_names.insert(name);
  }

  // definitions emit nothing on their own. the body is compiled
  // fresh at every call site by generate_call, which is what lets
  // parameters alias the caller's cells instead of copying
  compiler->add_function(name, Compiler::DefinedFunction{return_type, args,
                                                         &body});
}

std::optional<size_t> generate_call(std::ostream *out, Compiler *compiler,
                                    CallExpr *call) {
  if (!compiler->contains_function(call->name)) {
    throw CompilerException(call->line_number, "function: " + call->name +
                                                   " has not been defined");
  }

  if (compiler->call_in_progress(call->name)) {
    throw CompilerException(call->line_number,
                            "call to function: " + call->name +
                                " while it is already being compiled. "
                                "recursion is not supported");
  }

  const Compiler::DefinedFunction &fn = compiler->get_function(call->name);

  if (call->args.size() != fn.args.size()) {
    throw CompilerException(call->line_number,
                            "function: " + call->name + " takes " +
                                std::to_string(fn.args.size()) +
                                " arguments, " +
                                std::to_string(call->args.size()) + " given");
  }

  size_t frame_base{compiler->get_scope().get_next_free()};

  // Bind each parameter to an address, evaluating in the caller's
  // scope. Plain variables alias the caller's cell directly, so the
  // body reads and writes the caller's memory. Anything else must be
  // an Integer and is materialized into a fresh frame cell
  std::vector<Scope::Variable> bindings;
  bindings.reserve(call->args.size());

  for (size_t i{0}; i < call->args.size(); ++i) {
    std::string param_type_name = fn.args[i].arg_type;
    Type *param_type = compiler->get_type(param_type_name);
    Expr *arg = call->args[i].get();

    if (arg->get_type() == ExprType::VAR) {
      VarExpr *var_arg = static_cast<VarExpr *>(arg);

      size_t addr{};
      Type *arg_type = nullptr;

      // whole structs are legal arguments, so bypass the
      // must-access-structs-by-field check in eval_var_expr
      // when there are no fields to walk
      if (var_arg->fields.size() == 0) {
        if (!compiler->contains_var(var_arg->name)) {
          throw CompilerException(var_arg->line_number,
                                  "variable has not yet been defined");
        }

        const Scope::Variable &var = compiler->get_var(var_arg->name);
        addr = var.addr;
        arg_type = var.type;
      } else {
        addr = eval_var_expr(compiler, var_arg, &arg_type);
      }

      if (arg_type != param_type) {
        throw CompilerException(var_arg->line_number,
                                "type mismatch on argument: " +
                                    fn.args[i].arg_name + " in call to " +
                                    call->name);
      }

      bindings.push_back(Scope::Variable{addr, param_type});
      continue;
    }

    if (param_type != compiler->get_builtin_integer()) {
      throw CompilerException(
          arg->line_number,
          "arguments of array or struct type must be plain variables, "
          "as they are passed by reference");
    }

    Scope &scope = compiler->get_scope();
    size_t slot{scope.get_next_free()};

    EvalResult result = eval(out, compiler, arg);

    // consolidate into the slot, which sits at the old next_free
    // and is therefore already zero
    switch (result.type) {
    case EvalType::CONST: {
      *out << "MOV: " << slot << ", " << result.val << '\n';
      break;
    }

    case EvalType::ADDRESS: {
      *out << "ADD: " << slot << ", " << result.val << '\n';
      break;
    }

    case EvalType::TEMP: {
      if (result.val != slot) {
        *out << "ADD: " << slot << ", " << result.val << '\n';
        *out << "MOV: " << result.val << ", 0\n";
      }
      break;
    }
    }

    scope.set_next_free(slot + 1);
    bindings.push_back(Scope::Variable{slot, param_type});
  }

  bool returns_value{fn.return_type == "Integer"};

  compiler->begin_call(call->name, returns_value);

  Scope &frame = compiler->get_scope();
  for (size_t i{0}; i < bindings.size(); ++i) {
    frame.set_var(fn.args[i].arg_name, bindings[i]);
  }

  for (const StmtPtr &stmt : *fn.body) {
    if (compiler->current_call()->has_returned) {
      throw CompilerException(stmt->line_number,
                              "return must be the final statement in a "
                              "function body");
    }

    stmt->generate(out, compiler);
  }

  Compiler::ActiveCall *finished = compiler->current_call();

  if (returns_value && !finished->has_returned) {
    throw CompilerException(call->line_number,
                            "function: " + call->name +
                                " must end with a return statement");
  }

  std::optional<size_t> ret;
  if (finished->has_returned) {
    ret = finished->ret_addr;
  }

  size_t frame_end{compiler->get_scope().get_next_free()};
  compiler->end_call();

  // The frame is dead, but named locals and materialized args leave
  // junk behind. Wipe everything except the return cell so the
  // zero-invariant holds again and the frame can be fully reclaimed
  for (size_t addr{frame_base}; addr < frame_end; ++addr) {
    if (ret && addr == *ret) {
      continue;
    }

    *out << "MOV: " << addr << ", 0\n";
  }

  compiler->get_scope().set_next_free(ret ? *ret + 1 : frame_base);

  return ret;
}

void ReturnStmt::generate(std::ostream *out, Compiler *compiler) {
  Compiler::ActiveCall *active_call = compiler->current_call();

  if (!active_call) {
    throw CompilerException(line_number,
                            "return used outside of a function body");
  }

  if (!active_call->returns_value) {
    throw CompilerException(line_number,
                            "function: " + active_call->name +
                                " is declared None and cannot return a value");
  }

  // a return buried in an if or loop would need every statement
  // after it fenced off behind a flag to actually skip anything,
  // so only the top level of the body is allowed
  if (compiler->scope_depth() - 1 != active_call->scope_barrier) {
    throw CompilerException(line_number,
                            "return must be at the top level of a function "
                            "body");
  }

  Scope &scope = compiler->get_scope();
  size_t snapshot{scope.get_next_free()};

  EvalResult result = eval(out, compiler, val.get());

  switch (result.type) {
  case EvalType::CONST: {
    *out << "MOV: " << snapshot << ", " << result.val << '\n';
    break;
  }

  case EvalType::ADDRESS: {
    // the snapshot cell is above next_free and therefore
    // zero, so the ADD acts as a copy
    *out << "ADD: " << snapshot << ", " << result.val << '\n';
    break;
  }

  case EvalType::TEMP: {
    if (result.val != snapshot) {
      *out << "ADD: " << snapshot << ", " << result.val << '\n';
      *out << "MOV: " << result.val << ", 0\n";
    }
    break;
  }
  }

  scope.set_next_free(snapshot + 1);

  active_call->ret_addr = snapshot;
  active_call->has_returned = true;
}

void CallStmt::generate(std::ostream *out, Compiler *compiler) {
  size_t snapshot{compiler->get_scope().get_next_free()};

  std::optional<size_t> ret =
      generate_call(out, compiler, static_cast<CallExpr *>(call.get()));

  // the frame is already wiped, so zeroing the discarded return
  // value reclaims the whole call
  if (ret) {
    *out << "MOV: " << *ret << ", 0\n";
    compiler->get_scope().set_next_free(snapshot);
  }
}
