// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe %s %libcxx_flags -o %t && %t

// P3098 captures whose initializer names a member of a dependent object,
// scalar and pack, in a function template with a deduced return type and a
// result name, and in a member template defined outside its class.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3098-capture-deduced-return.C
// in the gnu_gcc fork (GCC-477), whose CLANG-647 row asked for it.

#include <contracts>

int violations;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct S { int m; };

template <class T>
auto a(T t) post [c = t.m] (r: c > 0 && r == c) { return t.m; }
template <class T>
auto b(T t) post [c = t.m] (r: true) { return 0; }
template <class... T>
auto p(T... t) post [...c = t.m] (r: ((c > 0) && ...) && r == 0) { return 0; }
template <class... T>
auto q(T... t) post [...c = t.m + 1] (r: ((c > 1) && ...)) { return 0; }

template <class C> struct X {
  template <class... T>
  auto g(T... t) post [...c = t.m] (r: ((c > 0) && ...));
};
template <class C>
template <class... T>
auto X<C>::g(T... t) post [...c = t.m] (r: ((c > 0) && ...)) { return 0; }

int main() {
  S one{1}, zero{0};
  X<int> x;
  a(one); b(one); p(one, one); q(one); x.g(one, one);
  if (violations != 0)
    __builtin_abort();
  a(zero); p(one, zero); q(zero, one); x.g(zero);
  if (violations != 4)
    __builtin_abort();
}
