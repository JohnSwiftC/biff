#ifndef CODEGEN_H
#define CODEGEN_H

#include <cstddef>
#include <ostream>

#include "ast.h"

class Compiler;

// These are used when evaluating the
// expression tree recursively
enum class EvalType {
  // The address of a named variable's cell. Read-only:
  // it must never be operated on in place.
  ADDRESS,
  CONST,
  // A scratch cell owned by the evaluator, safe to
  // accumulate into in place.
  TEMP,
};

struct EvalResult {
  EvalType type;
  size_t val;
};

// Evaluates a var_expr with its variable name to resolve a base address,
// and uses its fields to find the true address being referenced by
// the expression
size_t eval_var_expr(Compiler *compiler, Expr *var_expr);

// Recursively evaluates an expression tree, emitting any code
// needed to compute it. Temps it allocates are reclaimed by the
// caller with Scope::set_next_free once the result is consumed
EvalResult eval(std::ostream *out, Compiler *compiler, Expr *expr);
EvalResult eval_unary(std::ostream *out, Compiler *compiler, UnaryExpr *unary);
void handle_new_struct(Compiler *compiler, VarExpr* var_expr, Type* struct_type, Expr* val_expr);

#endif
