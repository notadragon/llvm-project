// KNOWN CRASH, pinned so that fixing it shows up as an XPASS.
//
// A postcondition with a result-name-introducer on a function returning a
// REFERENCE crashes Clang during code generation:
//
//   Expr.h: Assertion `(t.isNull() || !t->isReferenceType()) &&
//                      "Expressions can't have reference type"' failed.
//
// EmitPostContracts binds the result name by building an OpaqueValueExpr of
// the result name's type, and for a reference-returning function that type is
// a reference -- which an Expr may not have.  The binding presumably wants the
// referenced type, with the slot holding the pointer.
//
// This is pre-existing (the block dates from the P3850 base commit) and is not
// a regression from the result-binding mutation fix; it was found while
// measuring that fix's coverage.
//
// It needs CODEGEN: -fsyntax-only is clean, which is why it survived.  GCC
// compiles and runs the same program correctly (measured 2026-09-02: the
// mutation below is observed, giving 105).
//
// XFAIL: *
// RUN: %clangxx -std=c++26 %s -fcontracts %libcxx_flags -o %t && %t

static int g = 100;

// Plain use of the result name -- also crashes, so the mutation is not the
// trigger; naming the result of a reference-returning function is.
int &plain() post(r : r > 0) { return g; }

int &mutating() post(r : (const_cast<int &>(r) += 5, true)) { return g; }

int main() {
  if (plain() != 100)
    return 1;
  if (mutating() != 105)
    return 1;
  return 0;
}
