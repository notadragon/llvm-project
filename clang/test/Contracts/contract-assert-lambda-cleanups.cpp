// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce \
// RUN:   -emit-llvm -o /dev/null %s

// A contract_assert in a lambda in a member of a class template, whose
// predicate creates a temporary with a nontrivial destructor, once crashed
// code generation because the cleanups the evaluation needs were not taken
// into account.

__attribute__((weak)) void unknown();
struct S {
  S();
  bool foo() const;
  ~S();
};

namespace ONS {
  struct Tag {};
  struct O {};

  Tag operator==(O, O);
  Tag operator!=(O, O);
}

template <class T>
struct X {
  auto foo() const {
    [&]() { contract_assert(S{}.foo() && __is_same(decltype(T{} == T{}), ONS::Tag)); }();
    return S().foo();
  }
};


void f() {
  X<ONS::O> x;
  x.foo();

}
