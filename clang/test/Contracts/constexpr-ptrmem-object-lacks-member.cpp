// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Applying a pointer to member to an object that does not contain the
// member is undefined ([expr.mptr.oper]/4), so it is not a core constant
// expression even when no data is read, as for a member function.  Clang
// rejects it; GCC accepts it (stock, gnu_gcc GCC-743, PR121250, PR105277,
// deferred), and its watch test is the program below.  Not
// contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-ptrmem-object-lacks-member.C

struct B { int dummy = 0; };
struct D : B { int x = 0; constexpr int h() const { return 2; } };
constexpr int f() {
  int (B::*pb)() const = static_cast<int (B::*)() const>(&D::h);
  B b;
  return (b.*pb)(); // expected-note {{subexpression not valid in a constant expression}}
}
static_assert(f() == 2); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'f()'}}
