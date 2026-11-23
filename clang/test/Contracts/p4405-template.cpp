// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s

// P4405: a precondition is analyzed at each call (D4405), so a call in a
// template is diagnosed once per instantiation; a template that is never
// instantiated is not diagnosed.  An inline function that is never emitted
// is not diagnosed either.  Each diagnostic is followed by its
// instantiation's note, as Sema's would be (CLANG-830).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-template.C (GCC
// diagnoses never-emitted inline functions; notadragon_wg21 DECISIONS.md
// E130)

template <typename T>
T f(T x)
  pre(x > 0)    // expected-note 2 {{the predicate of the contract assertion}}
{ return x; }

template <typename T>
T call(T y) {
  return f(y);  // expected-error 2 {{contract assertion cannot be proven}}
}

template <typename T>
T never(T x)
  pre(x > 0)
{ return x; }

template <typename T>
T never_call(T y) {
  return never(y);
}

inline int unused_inline(int x) pre(x > 0) { return x; }
inline int unused_inline_call(int y) { return unused_inline(y); }

int g(int y) {
  int a = call(y);         // expected-note {{in instantiation of function template specialization 'call<int>' requested here}}
  long b = call((long)y);  // expected-note {{in instantiation of function template specialization 'call<long>' requested here}}
  return a + (int)b;
}
