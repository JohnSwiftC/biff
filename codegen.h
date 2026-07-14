#ifndef CODEGEN_H
#define CODEGEN_H

#include <cstddef>
#include <optional>
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
// the expression. When out_type is non-null, it receives the
// fully-resolved type of the referenced variable/field.
size_t eval_var_expr(Compiler *compiler, Expr *var_expr,
                     Type **out_type = nullptr);

// Recursively evaluates an expression tree, emitting any code
// needed to compute it. Temps it allocates are reclaimed by the
// caller with Scope::set_next_free once the result is consumed
EvalResult eval(std::ostream *out, Compiler *compiler, Expr *expr);
EvalResult eval_unary(std::ostream *out, Compiler *compiler, UnaryExpr *unary);

// Inlines a function's body at the current position in the output.
// Plain variable arguments alias the caller's cells (pass by
// reference), any other expression is materialized into a fresh
// cell (pass by value). Returns the address holding the return
// value, or nullopt for functions that don't produce one. On exit,
// every frame cell except the return cell has been zeroed and
// next_free points just past the return cell (or back at the frame
// base when there is none), so the zero-invariant holds
std::optional<size_t> generate_call(std::ostream *out, Compiler *compiler,
                                    CallExpr *call);
void handle_new_struct(Compiler *compiler, VarExpr* var_expr, Type* struct_type, Expr* val_expr);

#endif
