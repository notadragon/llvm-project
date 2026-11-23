// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcxx-exceptions -fsyntax-only \
// RUN:   -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:except.handle.handler.ctor.dtor:enforce

// P3100: with the function-try-block handler check configured, constant
// evaluation is unchanged: a constructor whose handler refers to a member
// stays usable in a constant expression when the handler is not entered.
// (Clang has no constexpr exceptions, so GCC's entered handler cannot be
// mirrored.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-handler-member-constexpr.C)

struct A {
  constexpr A(int v) try : x(v) {
  } catch (int) {
    throw y; // expected-warning {{cannot refer to a non-static member from the handler of a constructor function try block}}
  }
  int x;
  int y = 42;
};
constexpr int f(int v) {
  A a(v);
  return a.x;
}
static_assert(f(5) == 5);
