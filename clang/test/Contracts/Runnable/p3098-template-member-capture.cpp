// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098: A postcondition capture referenced in the predicate of a member
// function of a CLASS TEMPLATE.  This is valid and works for free function
// templates and members of non-template classes, so it must work here too.
//
// (Previously BUG-1: Clang mishandled the capture for class-template members --
// during instantiation the predicate's capture reference was not remapped to the
// instantiated capture (it stayed the dependent pattern capture), so codegen hit
// "DeclRefExpr for Decl not entered in LocalDeclMap?".  FindInstantiatedDecl now
// treats a PostconditionCaptureDecl as a local decl and remaps it via the
// instantiation scope, like ResultNameDecl.)

#include <contracts>
#include <cstdio>

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

template <typename T>
struct S {
  T bump(T x) post [old = x] (r: r == old + 1) { return x + 1; }
  T bad(T x)  post [old = x] (r: r == old + 2) { return x + 1; } // predicate false
};

int main() {
  S<int> s;

  violation_count = 0;
  if (s.bump(1) != 2) __builtin_abort();       // old = 1, 2 == 1+1 -> true
  if (violation_count != 0) __builtin_abort();

  violation_count = 0;
  if (s.bad(1) != 2) __builtin_abort();        // old = 1, 2 == 1+2 -> false
  if (violation_count != 1) __builtin_abort(); // capture bound correctly

  std::printf("PASS\n");
}
