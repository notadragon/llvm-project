// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3850 %libcxx_flags -o %t
// RUN: %t

// A type-dependent init-capture in a postcondition on an in-class member of a
// class template must not crash.  This is the late-parsed sibling of
// p3098-capture-member-template.cpp: the capture's type is deduced from the
// substituted initializer at class-template instantiation.  Regression: the
// dependent placeholder previously reached CodeGen and asserted.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-capture-member-late.C.)

#include <contracts>

struct Has { int x; };

template <class T>
struct S {
  T m;
  // `m.x` is a member access on a dependent object -> the capture type is
  // deduced at instantiation.
  int g() post [c = m.x] (r : r > c) { return m.x + 1; }
};

int main() {
  S<Has> s{Has{5}};
  if (s.g() != 6)
    __builtin_abort();
  return 0;
}
