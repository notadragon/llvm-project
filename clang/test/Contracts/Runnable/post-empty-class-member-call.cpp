// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe -O0 %libcxx_flags -o %t0 && %t0 2>&1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe -O2 %libcxx_flags -o %t2 && %t2 2>&1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=enforce -O2 %libcxx_flags -o %te && not --crash %te 2>&1 | FileCheck %s --check-prefix=ENFORCE

// A postcondition that calls a member function on the result binding of an
// empty class returned by value: the implicit object parameter is the
// reference the binding is bound to.  The reduced form had a class template
// and an ordinary class; a function template is added.  Each postcondition
// is evaluated on every return: none is violated while `ok` holds, each one
// once after it is cleared.
//
// GCC mirror: gnu_gcc bug-reports/gcc-33/gcc-33b-member-call-on-result.cpp
// (GCC-33b, PR126006; an ICE in RTL expand on stock g++).

#include <cstdio>

template <typename> struct generator {
  bool is_valid() const { return ok; }
  static bool ok;
};
template <typename T> bool generator<T>::ok = true;

generator<int> val() post(g : g.is_valid()) {
  generator<int> x = {};
  return x;
}

struct G {
  bool is_valid() const { return ok; }
  static bool ok;
};
bool G::ok = true;

G val2() post(g : g.is_valid()) {
  G x = {};
  return x;
}

template <typename T> T tval() post(g : g.is_valid()) {
  T x = {};
  return x;
}

int main() {
  val();
  val2();
  tval<G>();
  tval<generator<int>>();
  std::fputs("all valid\n", stderr);
  generator<int>::ok = false;
  G::ok = false;
  val();
  val2();
  tval<G>();
  tval<generator<int>>();
  std::fputs("done\n", stderr);
}

// CHECK-NOT: failed
// CHECK: all valid
// CHECK-NEXT: {{.*}}post-empty-class-member-call.cpp:23: post(g : g.is_valid()) failed
// CHECK-NEXT: {{.*}}post-empty-class-member-call.cpp:34: post(g : g.is_valid()) failed
// CHECK-NEXT: {{.*}}post-empty-class-member-call.cpp:39: post(g : g.is_valid()) failed
// CHECK-NEXT: {{.*}}post-empty-class-member-call.cpp:39: post(g : g.is_valid()) failed
// CHECK-NEXT: done

// ENFORCE: all valid
// ENFORCE-NEXT: {{.*}}post-empty-class-member-call.cpp:23: post(g : g.is_valid()) failed
// ENFORCE-NOT: done
