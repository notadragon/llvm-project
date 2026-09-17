// General contracts/noexcept correctness (surfaced during D4298 review):
// same guarantee as p4298-noexcept-fn-inline.cpp -- a throwing handler
// inside a noexcept function must terminate -- but exercised through the
// P3097 virtual-override contract-check wrapper's codegen path instead of an
// inline check.  Plain "enforce" semantic, not one of D4298's new semantics:
// this is a general noexcept/contracts interaction.  Confirms the wrapper
// (getOrEmitVirtualContractWrapper) correctly inherits the wrapped
// function's noexcept-ness via arrangeCXXMethodDeclaration + StartFunction,
// with NO contract-semantics-based marking (the naive "mark the whole
// wrapper nounwind if its contracts are nonthrowing" rule was rejected on
// the GCC side -- the wrapper also calls the real implementation, whose
// legitimate exceptions must NOT be forced to terminate).
// (GCC mirror: g++.dg/contracts/cpp26/p4298-noexcept-fn-p3097.C)
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

#include <contracts>
#include <exception>
#include <cstdlib>

struct MyException {};

void my_term()
{
  try { throw; }
  catch (MyException) { std::exit(0); }
}

void handle_contract_violation(const std::contracts::contract_violation&)
{
  throw MyException{};
}

struct base {
  virtual int f(int x) noexcept pre(x > 0) { return x; }
};
struct derived : base {
  int f(int x) noexcept override { return x; }
};

// A call through a base reference goes through P3097's virtual-dispatch
// wrapper (it cannot be resolved to a fixed dynamic type by the front end).
int call_virtually(base& b, int x) { return b.f(x); }

int main()
{
  std::set_terminate(my_term);
  try {
    derived d;
    call_virtually(d, -1);
  } catch (...) {
  }
  __builtin_trap();  // We should not get here.
}
