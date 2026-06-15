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

// Recursively evaluates an expression tree, emitting any code
// needed to compute it. Temps it allocates are reclaimed by the
// caller with Scope::set_next_free once the result is consumed
EvalResult eval(std::ostream *out, Compiler *compiler, Expr *expr);
EvalResult eval_unary(std::ostream *out, Compiler *compiler, UnaryExpr *unary);

#endif
