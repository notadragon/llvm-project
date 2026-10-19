// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only \
// RUN:   -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun:enforce
// expected-no-diagnostics

// P3100: with the check for operations on (*this) before its bases are
// initialized configured, constant evaluation is unchanged: a member function
// called in a member mem-initializer or the constructor body stays a constant
// expression.  (A call in a base-class mem-initializer is accepted in
// constant evaluation, stock and here alike: CLANG-713.)

struct Base {
  int v;
  constexpr Base(int y) : v(y) {}
};
struct D : Base {
  int j;
  constexpr int f() const { return 7; }
  constexpr D() : Base(1), j(f()) { j += f(); }
};
static_assert(D().j == 14);
