// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// Inside a requires-expression in a contract predicate, a variable declared
// outside the contract is const ([expr.prim.id.unqual]), so
// `requires { ++x; }` is false -- in a function template and a member of a
// class template too, where the requirement is substituted on instantiation.
// (The non-template form is ill-formed and diagnosed.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constify-requires-expr.C

#include <contracts>

static int handled = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++handled;
}

template <class T> void free_pre(T x) pre(!requires { ++x; }) {}
template <class T> void body_assert(T x) { contract_assert(!requires { ++x; }); }
template <class T> struct S {
  void mem_pre(T x) pre(!requires { ++x; }) {}
  T m;
  void mem_this() pre(!requires { ++m; }) {}
};

int main() {
  free_pre(1);
  body_assert(1);
  S<int> s{};
  s.mem_pre(1);
  s.mem_this();
  if (handled != 0)
    __builtin_abort();
}
