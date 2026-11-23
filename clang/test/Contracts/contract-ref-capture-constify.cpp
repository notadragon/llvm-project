// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A variable or reference named in a contract_assert through a by-reference
// capture of the enclosing lambda (explicit, init-capture, implicit; plain,
// generic and template lambdas) is const ([expr.prim.id.unqual]).  The
// structured-binding shapes are left out: Clang misses those, which is
// CLANG-500 in this fork.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-ref-capture-constify.C

void g(int &r) {
  contract_assert(++r);                  // expected-error {{cannot assign to}}
  [&r] { contract_assert(++r); }();      // expected-error {{cannot assign to}}
  [&k = r] { contract_assert(++k); }();  // expected-error {{cannot assign to}}
  int v = 0;
  [&v] { contract_assert(++v); }();      // expected-error {{cannot assign to}}
}

void a0(int x) {
  auto l = [&] { contract_assert(++x > 0); (void)x; }; // expected-error {{cannot assign to}}
  l();
}

void a1(int x) {
  auto l = [&](auto u) { contract_assert(++x > u); (void)x; }; // expected-error {{cannot assign to}}
  l(0);
}

template <class T> void b(T x) {
  auto l = [&] { contract_assert(++x > 0); (void)x; }; // expected-error {{cannot assign to}} \
                                                       // expected-note {{while substituting into a lambda expression here}}
  l();
}

template void b<int>(int); // expected-note {{in instantiation of function template specialization 'b<int>' requested here}}
